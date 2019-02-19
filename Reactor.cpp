//
// Created by tong on 19-2-16.
//

#include "Reactor.h"


Reactor::Reactor(unsigned int threads):
__threadsNum(threads),
__eventLoopPtrs(__threadsNum, make_shared<EventLoop>())
{
    if(__threadsNum == 0){
        LOG << "Reactor threads num is 0.\n";
        exit(0);
    }

    for(int i=0; i<__threadsNum; i++){
        shared_ptr<EventLoop> p(__eventLoopPtrs[i]);
        __threadPool.emplace_back(std::thread([p](){
            p->run();
        }));
    }
}


Reactor::~Reactor() {
    //TODO:: 通知关闭每个事件循环
    for(const auto &p : __eventLoopPtrs){
        p->stop();
    }

    for(auto &t : __threadPool){
        t.join();
    }
}


void Reactor::addChannel(ChannelPtr channel) {
    unsigned int minLoad = __eventLoopPtrs[0]->getLoad();
    int idx = 0;

    for(int i=1; i<__threadsNum; i++){
        unsigned int l = __eventLoopPtrs[i]->getLoad();
        if(l < minLoad){
            idx = i;
            minLoad = l;
        }
    }
    // idx 表示最小负载的线程的索引
    __eventLoopPtrs[idx]->addChannel(channel);
}


