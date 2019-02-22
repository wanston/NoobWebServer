//
// Created by tong on 19-2-16.
//

#ifndef NOOBWEDSERVER_CHANNEL_H
#define NOOBWEDSERVER_CHANNEL_H

#include <memory>

class Channel {
public:
    explicit Channel(int fd, uint32_t event, int timeout);
    virtual ~Channel();
    int getFd();
    uint32_t getEvent();
    int getTimeout(); // 不能有setTimeout
    void setEvent(uint32_t event);
    void handleEvents(uint32_t events);

protected:
    int __fd;
    uint32_t __event; // 表示添加到epoll的事件
    const int __timeout;
    virtual void __handleReadEvent();
    virtual void __handleWriteEvent();
};

typedef std::shared_ptr<Channel> ChannelPtr;

#endif //NOOBWEDSERVER_CHANNEL_H
