//
// Created by tong on 19-2-18.
//

#include "EventLoop.h"
#include "Log.h"
#include "EventFdChannel.h"
#include <string.h>
#include <unistd.h>
#include <sys/eventfd.h>

EventLoop::EventLoop():
__quit(false),
__channelsCount(0)
{
    if((__epollFd = epoll_create(MAX_FD)) < 0){
        // 这样写是可以的,尽管LOG在另一个线程里面,但是exit()仍然会析构全局对象.
        // 所以只需要LOG对象是一个全局对象,并且析构时打印完输出就可以了.
        LOG << "Error at epoll_create. Error: " << strerror(errno) << '\n';
        exit(0);
    }
    if ((__eventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) < 0) {
        LOG << "Error at eventfd. Error: " << strerror(errno) << '\n';
        exit(0);
    }
    addChannel(make_shared<EventFdChannel>(__eventFd));

    LOG << "EventLoop::ctor __eventFd " << __eventFd << '\n';
}


EventLoop::~EventLoop() {
    close(__epollFd);
    close(__eventFd);
}

/**
 * 执行回调，回调里面可能会调用，
 * addchannel,
 * modchannel
 * delchannel函数。
 *
 * 会涉及timer和channel的添加、注册和删除、注销。
 *
 * 真正把把channel 和 timer删除。
 *
 * 执行仍有效的超时的timer的回调函数（delchannel， 删除、注销timer）。
 *
 * delchannel，del
 *
 *
 *
 * 真正的mod del channel，真正的del timer。
 *
 * **/
void EventLoop::run() {
    while(true){
        // wait
        __waiting = true;
        int eventsCount = epoll_wait(__epollFd, __events, MAX_FD, __epollTimeout);
        if(eventsCount < 0){
            LOG << "Error at epoll_wait. Error: " << strerror(errno) << '\n';
            exit(0);
        }
        __waiting = false;

        // 处理退出，TODO： 更好的退出方式
        if(__quit){
            return;
        }

        // 对于活跃的channel，移除旧的timer，添加新timer
        for(int i=0; i<eventsCount; i++){
            int fd = __events[i].data.fd;
            int timeout = __channels[fd]->getTimeout();
            auto f = std::bind(&EventLoop::__timerCallback, this, __channels[fd]);
            // 注销旧的
            __timerManager.unregisterTimer(__timers[fd]);
            // 注册新的
            TimerPtr timer = make_shared<Timer>(timeout, f);
            __timerManager.registerTimer(timer);
            __timers[fd] = timer;
        }

        // 检查超时的timer，调用timer的回调，回调里面会调用delChannel。
        __timerManager.runPerTick();

        // 调用活跃的channel的回调，回调里面可能会调用delChannel和modChannel
        for(int i=0; i<eventsCount; i++){
            int fd = __events[i].data.fd;
            __channels[fd]->handleEvents(__events[i].events);
        }

        // 先删除
        {
            std::lock_guard<std::mutex> lock(__delMutex);
            for(int fd : __pendingDelFds){
                __realDelChannel(fd);
            }
            __pendingDelFds.clear();
        }
        // 再添加
        {
            std::lock_guard<std::mutex> lock(__addMutex);
            for(ChannelPtr channel : __pendingAddChannels){
                __realAddChannel(channel);
            }
            __pendingAddChannels.clear();
        }

        {
            std::lock_guard<std::mutex> lock(__modMutex);
            for(int fd : __pendingModFds){
                __realModChannel(fd);
            }
            __pendingModFds.clear();
        }
    }
}


/**
 * Description:
 *      函数被调用的位置, 如果在判断完__quit之后, 或者在wait: 那么需要写eventfd;
 *      如果在wait之后, __quit之前, 那么就不需要写.为了方便,直接都写.
 * **/
void EventLoop::stop() {
    if(!__quit){
        __quit = true;
        __wakeUp();
    }
}


unsigned int EventLoop::getLoad() {
    return __channelsCount;
}


// TODO: 考虑处理添加失败的情况，添加重复channel的情况
void EventLoop::addChannel(ChannelPtr channel) {
    std::lock_guard<std::mutex> lock(__addMutex);
    __pendingAddChannels.push_back(channel);
    if(__waiting){
        __wakeUp();
    }
}


void EventLoop::modChannel(ChannelPtr channel) {
    std::lock_guard<std::mutex> lock(__modMutex);
    __pendingModFds.push_back(channel->getFd());
}


void EventLoop::delChannel(ChannelPtr channel) {
    std::lock_guard<std::mutex> lock(__delMutex);
    __pendingDelFds.push_back(channel->getFd());
}


void EventLoop::__realAddChannel(ChannelPtr channel) {
    int fd = channel->getFd();

    if(__channels[fd]){
        return;
    }
    // epoll
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->getEvent();//EPOLLIN | EPOLLET;TODO 检查触发
    if(epoll_ctl(__epollFd, EPOLL_CTL_ADD, fd, &event) < 0) {
        LOG << "Error at epoll_ctl add. Error: " << strerror(errno) << '\n';
        exit(0);
    }
    // timer
    int timeout = channel->getTimeout();
    auto f = std::bind(&EventLoop::__timerCallback, this, channel);
    TimerPtr timer(new Timer(timeout, f));
    __timerManager.registerTimer(timer);
    __timers[fd] = timer; // 肯定有tmer，即使timeout是0
    // channel
    __channels[fd] = channel;
    ++__channelsCount;
}


void EventLoop::__realModChannel(int fd) {
    if(!__channels[fd]){
        return;
    }
    // epoll
    struct epoll_event event;
    event.data.fd =  fd;
    event.events = __channels[fd]->getEvent();
    if(epoll_ctl(__epollFd, EPOLL_CTL_MOD, fd, &event) < 0) {
        LOG << "Error at epoll_ctl mod. Error: " << strerror(errno) << '\n';
        exit(0);
    }
}


void EventLoop::__realDelChannel(int fd) {
    if(!__channels[fd]){
        return;
    }

    // epoll
    struct epoll_event event;
    event.data.fd = fd;
    event.events = __channels[fd]->getEvent(); //TODO: 有个小bug，此时的event可能不是注册时的event
    if(epoll_ctl(__epollFd, EPOLL_CTL_DEL, fd, &event) < 0) {
        LOG << "Error at epoll_ctl del. Error: " << strerror(errno) << '\n';
        exit(0);
    }
    // timer
    __timerManager.unregisterTimer(__timers[fd]);
    __timers[fd].reset();
    // channel
    __channels[fd].reset();
    --__channelsCount;
}


void EventLoop::__wakeUp() {
    // TODO: 处理write返回INTR的错误
    uint64_t one = 1;
    ssize_t r = write(__eventFd, &one, sizeof(one));
    LOG << "__wakeUp fd " << __eventFd << '\n';

    if(r != sizeof(one)){
        LOG << "Error in __wakeUp\n";
        exit(0);
    }
}


void EventLoop::__timerCallback(ChannelPtr channel) {
    LOG << "EventLoop::__timerCallback\n";
    delChannel(channel);
}

