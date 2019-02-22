//
// Created by tong on 19-2-22.
//

#ifndef NOOBWEDSERVER_HTTPCHANNEL_H
#define NOOBWEDSERVER_HTTPCHANNEL_H

#include <sys/epoll.h>
#include "Channel.h"

class HttpChannel : public Channel{
public:
    HttpChannel(int fd, int timeout): Channel(fd, EPOLLIN, timeout){}
private:
    void __handleReadEvent() override;
    void __handleWriteEvent() override;
};


#endif //NOOBWEDSERVER_HTTPCHANNEL_H
