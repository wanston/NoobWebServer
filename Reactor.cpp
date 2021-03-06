//
// Created by tong on 19-2-16.
//

#include "Reactor.h"
#include "HttpChannel.h"
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>


Reactor::Reactor(unsigned int threads, int timeout):
__threadsNum(threads),
__timeout(timeout),
__next(0)
{
    if(__threadsNum == 0){
        LOG << "Reactor threads num is 0.\n";
        exit(0);
    }

    for(int i=0; i<__threadsNum; i++){
        std::shared_ptr<EventLoop> p = make_shared<EventLoop>();
        __eventLoopPtrs.push_back(p);
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


void Reactor::addChannel(int fd) {
    // 设为非阻塞
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1){
        LOG << "Reactor::addChannel fcntl error. " << strerror(errno) << "\n";
        return;
    }

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1){
        LOG << "Reactor::addChannel fcntl error. " << strerror(errno) << "\n";
        return;
    }
    // idx 表示最小负载的线程的索引

    struct sockaddr_in clientAddr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(fd, (struct sockaddr *)&clientAddr, &addr_size);

    int idx = __next;
    __next = (__next + 1) % __threadsNum;

//    unsigned int minLoad = __eventLoopPtrs[0]->getLoad();
//    int idx = 0;

//    for(int i=1; i<__threadsNum; i++){
//        unsigned int l = __eventLoopPtrs[i]->getLoad();
//        if(l < minLoad){
//            idx = i;
//            minLoad = l;
//        }
//    }

    LOG << "Thread id: " << __threadPool[idx].get_id() << " Connected. Thread idx " << idx <<  " Addr " <<  inet_ntoa(clientAddr.sin_addr) << ':' << clientAddr.sin_port << " fd " << fd << '\n';

    __eventLoopPtrs[idx]->addChannel(make_shared<HttpChannel>(fd, __timeout, __eventLoopPtrs[idx]));
}


