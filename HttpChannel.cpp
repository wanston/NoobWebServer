//
// Created by tong on 19-2-22.
//

#include <unistd.h>
#include <sys/socket.h>
#include "HttpChannel.h"
#include "Log.h"
#include "Response.h"

HttpChannel::HttpChannel(int fd, int timeout, std::weak_ptr<EventLoop> loop) :
Channel(fd, EPOLLIN, timeout), __eventLoop(loop){
    __parser.callback = &std::bind(&HttpChannel::__parserCallback, this);
}


void HttpChannel::__handleReadEvent() {
    vector<char> data;

    int ret = __readn(__fd, data);

    if(ret < 0){ // 发生错误
        __state = NO_RDWR;
    }else if(ret == 0){ // 收到FIN
        __state = NO_RD;
    }else{
        __state = NORMAL;
    }

    // 解析报文，填入__sendBuf，然后尝试发送。该过程中__state状态可能发生改变。
    if(__state != NO_RDWR && !data.empty()){ // 需要解析报文
        // parse() 返回-1，表示解析失败；返回0，表示未读到request结束；返回>0，表示解析成功一次request，返回的是已读字符数目。
        int n = 1, offset = 0;
        while(n > 0 && data.size() > offset){
            n = __parser(data.data()+offset, data.size()-offset);
            if(n < 0){ // 报文格式错误
                __state = NO_RD;
                vector<char> response = make_response(BAD_REQUEST);
                __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());
            }else if(n == 0){ // 未遇到报文的结尾
                if(__state == NO_RD){
                    vector<char> response = make_response(BAD_REQUEST);
                    __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());
                }else{
                    // do nothing
                }
            }else{ // 已解析完一条报文
                // do nothing
            }

            // 此时，发送__sendBuf中的报文
            int sendLen = __writen(__fd, __sendBuf);
            if(sendLen < 0){ // write遇到错误
                __state = NO_RDWR;
            }else{
                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
                // do nothing
            }
            offset += n;
        }
    }

    shared_ptr<EventLoop> p = __eventLoop.lock();
    if(!p){
        throw logic_error("HttpChannel::__eventLoop is nullptr.");
    }
    // 根据写缓冲区是否有数据，以及__state的值，来判断下一步的动作
    if(__state == NO_RDWR){
        p->delChannel(shared_from_this());
    }else if(__state == NO_RD){
        if(__sendBuf.empty()){
            p->delChannel(shared_from_this());
            __state = NO_RDWR;
        }else{
            shutdown(__fd, SHUT_RD);
            if(getEvent() != EPOLLOUT){ // 如果之前关注IN 或者 之前 没关注OUT
                setEvent(EPOLLOUT);
                p->modChannel(shared_from_this());
            }
        }
    }else {
        if(__sendBuf.empty()){ // 仅关注可读
            if(getEvent() != EPOLLIN){
                setEvent(EPOLLIN);
                p->modChannel(shared_from_this());
            }
        }else{ // 继关注可读又关注可写
            if(getEvent() != EPOLLIN|EPOLLOUT){
                setEvent(EPOLLIN|EPOLLOUT);
                p->modChannel(shared_from_this());
            }
        }
    }
}


void HttpChannel::__handleWriteEvent() {
    shared_ptr<EventLoop> p = __eventLoop.lock();
    if(!p){
        throw logic_error("HttpChannel::__eventLoop is nullptr.");
    }

    if(__state == NO_RDWR){
        // 说明handleRead里面已经delChannel了。
        return;
    }else if(__state == NO_RD){
        if(__sendBuf.empty()/*缓冲区没有数据*/){
            __state = NO_RDWR;
            p->delChannel(shared_from_this());
        }else{
            int sendLen = __writen(__fd, __sendBuf);

            if(sendLen < 0 || sendLen == __sendBuf.size()){ // 发送错误 或者 发送完
                __state = NO_RDWR;
                p->delChannel(shared_from_this());
            }else{ // 发送未完
                if(getEvent() != EPOLLOUT){
                    setEvent(EPOLLOUT);
                    p->modChannel(shared_from_this());
                }
                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
            }
        }
    }else{
        if(__sendBuf.empty()){
            if(getEvent() != EPOLLIN){
                setEvent(EPOLLIN);
                p->modChannel(shared_from_this());
            }
        }else{
            int sendLen = __writen(__fd, __sendBuf);

            if(sendLen < 0){ // write发生错误
                __state = NO_RDWR;
                p->delChannel(shared_from_this());
            }else if(sendLen == __sendBuf.size()){ // 发送完
                if(getEvent() != EPOLLIN){
                    setEvent(EPOLLIN);
                    p->modChannel(shared_from_this());
                }
                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
            }else{ // 没发送完
                if(getEvent() != EPOLLIN|EPOLLOUT){
                    setEvent(EPOLLIN|EPOLLOUT);
                    p->modChannel(shared_from_this());
                }
                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
            }
        }
    }
}


int HttpChannel::__parserCallback(void *data, string &method, string &url, string &version, std::vector<Header> &requestHeaders,
                          std::vector<char> &messageBody) {
    // 构造报文，添进buffer，返回值应该使得没调用一次parser()返回一次。
}


// 最好还是，parser每调用一次callback，parser()就返回一次。这样可以根据每个报文的发送是否成功，来控制连接。


/**
 * 返回-1，表示fd发生错误
 * 返回0，表示收到fin
 * 返回1，表示正常读到EAGAIN为止，但是可能未读到数据
 * **/
int HttpChannel::__readn(int fd, vector<char> &buf) const {
    // 读数据，一直到EAGAIN（读完），或者0（断开），或者其他错误。EINTR错误不会发生，因为EINTR只发生在会blocked system call上。
    char tmp[1024];
    while(true){
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if(n > 0){
            buf.insert(buf.end(), tmp, tmp+n);
        }else if(n == 0){ // 对端关闭连接
            return 0;
        }else{
            if(errno == EAGAIN){
                return 1;
            }else{ // 极大可能是Connection reset by peer
                return -1;
            }
        }
    }
}


/**
 * 返回-1，表示发生错误；
 * 返回>=0，表示写进去的字节数；
 * **/
int HttpChannel::__writen(int fd, const vector<char> &buf) const {
    if(buf.empty()){
        return 0;
    }

    ssize_t n = write(fd, buf.data(), buf.size());
    if(n < 0){
        if(errno == EAGAIN){
            return 0;
        }else{
            return -1;
        }
    }else{
        return int(n);
    }
}