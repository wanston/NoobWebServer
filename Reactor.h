//
// Created by tong on 19-2-16.
//

#ifndef NOOBWEDSERVER_REACTOR_H
#define NOOBWEDSERVER_REACTOR_H

#include "Channel.h"
#include "EventLoop.h"
#include "Log.h"
#include <vector>
#include <thread>
#include <memory>

using namespace std;

/**
 * Description：
 *      持有n个线程，和n个eventloop，每个线程执行一个事件循环
 *      必须可以退出
 * **/
class Reactor {
public:
    explicit Reactor(unsigned int threads = 5);
    ~Reactor();
    void addChannel(ChannelPtr channel);

private:
    const unsigned int __threadsNum;
    vector<thread> __threadPool;
    vector<shared_ptr<EventLoop>> __eventLoopPtrs;
};


#endif //NOOBWEDSERVER_REACTOR_H