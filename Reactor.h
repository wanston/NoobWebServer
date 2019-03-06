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
    explicit Reactor(unsigned int threads, int timeout);
    ~Reactor();
    void addChannel(int fd);

private:
    const unsigned int __threadsNum;
    vector<thread> __threadPool;
    vector<shared_ptr<EventLoop>> __eventLoopPtrs;
    int __timeout;
    int __next;
};


#endif //NOOBWEDSERVER_REACTOR_H
