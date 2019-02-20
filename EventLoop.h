//
// Created by tong on 19-2-18.
//

#ifndef NOOBWEDSERVER_EVENTLOOP_H
#define NOOBWEDSERVER_EVENTLOOP_H

#include "Channel.h"
#include "TimerManager.h"
#include <vector>
#include <sys/epoll.h>

#define MAX_FD 65536

/**
 * Description:
 *   事件循环,包含的功能应该有: 管理fd,执行回调函数,处理超时机制.
 *   //TODO eventfd唤醒循环
 * **/
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    void run();
    void stop();
    unsigned int getLoad();

    void addChannel(ChannelPtr channel);
    void modChannel(ChannelPtr channel); // 和real相比应该再做一些检查,确保channel已添加
    void delChannel(ChannelPtr channel);

private:
    void __realModChannel(ChannelPtr channel);
    void __realDelChannel(ChannelPtr channel);
    void __wakeUp();


    bool __quit;
    int __epollFd;
    int __eventFd;
    unsigned int __channelsCount;
    ChannelPtr __channels[MAX_FD];
    struct epoll_event __events[MAX_FD];// 用来存储epoll_wait的结果
    TimerManager __timerManager;
    int __epollTimeout; // TODO init
    int __channelTimeout; // TODO init

    // 存储需要mod或者del的fd.有条件竞争,因为要避免其他线程调用modChannel delChannel
    std::vector<int> __pendingModFds;
    std::vector<int> __pendingDelFds;
};


#endif //NOOBWEDSERVER_EVENTLOOP_H
