//
// Created by tong on 19-2-22.
//

#ifndef NOOBWEDSERVER_HTTPCHANNEL_H
#define NOOBWEDSERVER_HTTPCHANNEL_H

#include <sys/epoll.h>
#include <string>
#include "Channel.h"
#include "EventLoop.h"
#include "HttpParser.h"

using namespace std;

class HttpChannel : public Channel{
public:
    HttpChannel(int fd, int timeout, std::weak_ptr<EventLoop> loop);
private:
    std::weak_ptr<EventLoop> __eventLoop; // 用于自身调用delChannel来从其中删除channel。
    HttpRequestParser __parser;
    vector<char> __sendBuf;

    enum{
        NORMAL,
        NO_RD,
        NO_RDWR
    }__state = NORMAL;

    void __handleReadEvent() override;
    void __handleWriteEvent() override;
    int __readn(int fd, vector<char> &buf) const;
    int __writen(int fd, const vector<char> &buf) const;
    int __parserCallback(void *data, string &method, string &url, string &version, std::vector<Header> &requestHeaders, std::vector<char>& messageBody);
};


#endif //NOOBWEDSERVER_HTTPCHANNEL_H
