//
// Created by tong on 19-2-16.
//

#ifndef NOOBWEDSERVER_CHANNEL_H
#define NOOBWEDSERVER_CHANNEL_H

#include <memory>

class Channel {
public:
    explicit Channel(int fd, uint32_t event=0);
    virtual ~Channel();
    inline int getFd();
    inline uint32_t getEvent();
//    inline void setEvent(uint32_t event);
    void handleEvents(uint32_t events);

protected:
    int __fd;
    uint32_t __event; // 表示关心的事件,并不是发生的事件
    virtual void __handleReadEvent();
    virtual void __handleWriteEvent();
};

typedef std::shared_ptr<Channel> ChannelPtr;

#endif //NOOBWEDSERVER_CHANNEL_H
