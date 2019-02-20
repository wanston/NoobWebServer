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
}


EventLoop::~EventLoop() {
    close(__epollFd);
    close(__eventFd);
}


void EventLoop::run() {
    while(true){
        int eventsCount = epoll_wait(__epollFd, __events, MAX_FD, -1);
        if(eventsCount < 0){
            LOG << "Error at epoll_wait. Error: " << strerror(errno) << '\n';
            exit(0);
        }

        if(__quit){ //TODO: 这是强行退出
            return;
        }

        // 处理事件
        for(int i=0; i<eventsCount; i++){
            int fd = __events[i].data.fd;
            __channels[fd]->handleEvents(__events[i].events);
        }

        // 处理超时的timer, timer的回调函数里面会移除channel
        // 如何处理超时
        // 设置epoll 的timeout,每次醒来,就检查时间
//        manager.handleExpiredTimer(){
//            检查到期的timer, 执行回调函数--就是移除channel,调用eventloop里面的delchannel,delchannel负责关闭连接;
//            移除到期的timer
//        }
//
//        另外,channel的两个handler里面需要执行更新timer,(也就是新加一个timer).
//
        __timerManager.runPerTick();


        // TODO: 加锁
        // channel的回调函数(eg: readHandler)里面只把channel标记为待mod/del, 真正的mod/del在这里.
        for(int fd : __pendingModFds) {
            __realModChannel(__channels[fd]);
        }
        __pendingModFds.clear();

        for(int fd : __pendingDelFds){
            __realDelChannel(__channels[fd]);
        }
        __pendingDelFds.clear();
    }
}


/**
 * Description:
 *      函数被调用的位置, 如果在判断完__quit之后, 或者在wait: 那么需要写eventfd;
 *      如果在wait之后, __quit之前, 那么就不需要写.为了方便,直接都写.
 * **/
void EventLoop::stop() {
    __quit = true;
    __wakeUp();
}


void EventLoop::addChannel(ChannelPtr channel) {
    int fd = channel->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->getEvent();//EPOLLIN | EPOLLET;

    if(epoll_ctl(__epollFd, EPOLL_CTL_ADD, fd, &event) < 0) {
        LOG << "Error at epoll_ctl add. Error: " << strerror(errno) << '\n';
        exit(0);
    }
    __channels[fd] = channel;
    ++__channelsCount;

    int timeout = channel->getTimeout();

    // f负责从eventLoop移除channel, 关闭fd是channel析构时做的
    auto f = std::bind(&EventLoop::delChannel, this, channel);
    if(timeout != 0){
        __timerManager.registerTimer(make_shared<Timer>(timeout, f));
    }
}


void EventLoop::modChannel(ChannelPtr channel) {
    // TODO: 加锁
    __pendingModFds.push_back(channel->getFd());
}


void EventLoop::delChannel(ChannelPtr channel) {
    // TODO: 加锁
    __pendingDelFds.push_back(channel->getFd());
}


void EventLoop::__wakeUp() {
    // TODO: 处理write返回INTR的错误
    uint64_t one = 1;
    ssize_t r = write(__eventFd, &one, sizeof(one));
    if(r != sizeof(one)){
        LOG << "Error in __wakeUp\n";
        exit(0);
    }
}


unsigned int EventLoop::getLoad() {
    return __channelsCount;
}


void EventLoop::__realModChannel(ChannelPtr channel) {
    struct epoll_event event;
    event.data.fd =  channel->getFd();
    event.events = channel->getEvent();

    if(epoll_ctl(__epollFd, EPOLL_CTL_MOD, channel->getFd(), &event) < 0) {
        LOG << "Error at epoll_ctl mod. Error: " << strerror(errno) << '\n';
        exit(0);
    }
}


void EventLoop::__realDelChannel(ChannelPtr channel) {
    int fd = channel->getFd();
    __channels[fd].reset();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->getEvent();
    if(epoll_ctl(__epollFd, EPOLL_CTL_DEL, fd, &event) < 0) {
        LOG << "Error at epoll_ctl del. Error: " << strerror(errno) << '\n';
        exit(0);
    }
    --__channelsCount;
}