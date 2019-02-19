//
// Created by tong on 19-2-16.
//

#include "Channel.h"
#include <exception>
#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

Channel::Channel(int fd, uint32_t event) : __fd(fd), __event(event){
    setSocketNonBlocking(__fd);
}


Channel::~Channel() {
    close(__fd);
}


int Channel::getFd() {
    return __fd;
}


uint32_t Channel::getEvent() {
    return __event;
}


//void Channel::setFd(int fd) {
//    __fd = fd;
//}


//void Channel::setEvent(uint32_t event) {
//    __event = event;
//}


void Channel::handleEvents(uint32_t events) {
    if(events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLIN)){
        __handleReadEvent();
    }else if(events & EPOLLOUT){
        __handleWriteEvent();
    }
}


void Channel::__handleReadEvent() {
}


void Channel::__handleWriteEvent() {
}