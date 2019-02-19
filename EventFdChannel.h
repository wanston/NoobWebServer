//
// Created by tong on 19-2-18.
//

#ifndef NOOBWEDSERVER_EVENTFDCHANNEL_H
#define NOOBWEDSERVER_EVENTFDCHANNEL_H

#include <sys/epoll.h>
#include "Channel.h"

class EventFdChannel : public Channel {
public:
    explicit EventFdChannel(int fd) : Channel(fd) {
        __event = EPOLLIN;
    }
private:
    void __handleReadEvent() override;
};


#endif //NOOBWEDSERVER_EVENTFDCHANNEL_H
