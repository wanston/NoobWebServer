//
// Created by tong on 19-2-18.
//

#ifndef NOOBWEDSERVER_EVENTLOOP_H
#define NOOBWEDSERVER_EVENTLOOP_H

#include "Channel.h"
#include "TimerManager.h"
#include <vector>
#include <sys/epoll.h>
#include <mutex>
//#include <>

#define MAX_FD 65536

/**
 * Description:
 *   事件循环,包含的功能应该有: 管理fd,执行回调函数,处理超时机制.
 *
 *   addChannel、modChannel、delChannel三个函数需设计为线程安全的。
 *   竞争可能发生在addChannel和addChannel之间，也能发生在addChannel和run之间（registerTimer和runPerTick）
 *
 *   并且需要允许channel的回调函数里面调用mod del自己。
 * **/
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    void run();
    void stop();
    unsigned int getLoad();

    /**
     * 添加channel、添加timer、注册timer到manager
     * **/
    void addChannel(ChannelPtr channel);
    /**
     * 修改channel，不动timer
     * **/
    void modChannel(ChannelPtr channel); // 和real相比应该再做一些检查,确保channel已添加
    /**
     * 删除channel，删除timer，从manager注销timer
     * **/
    void delChannel(ChannelPtr channel);

private:
    void __realAddChannel(ChannelPtr channel);
    void __realModChannel(int fd);
    void __realDelChannel(int fd);
    void __wakeUp();


    bool __quit;
    bool __waiting;
    int __epollFd;
    int __eventFd;
    unsigned int __channelsCount;
    ChannelPtr __channels[MAX_FD];
    TimerPtr __timers[MAX_FD];
    struct epoll_event __events[MAX_FD];// 用来存储epoll_wait的结果
    TimerManager __timerManager;
    int __epollTimeout = 1000; //毫秒

    // 存储需要mod或者del的fd.有条件竞争,因为要避免其他线程调用modChannel delChannel
    std::vector<int> __pendingModFds;
    std::vector<int> __pendingDelFds;
    std::vector<ChannelPtr> __pendingAddChannels;

    std::mutex __addMutex;
    std::mutex __modMutex;
    std::mutex __delMutex;

};


#endif //NOOBWEDSERVER_EVENTLOOP_H
