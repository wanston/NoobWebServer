//
// Created by tong on 19-2-22.
//

#include <unistd.h>
#include <sys/socket.h>
#include <functional>
#include "HttpChannel.h"
#include "Log.h"
#include "Response.h"

using namespace std::placeholders;

HttpChannel::HttpChannel(int fd, int timeout, std::weak_ptr<EventLoop> loop) :
Channel(fd, EPOLLIN, timeout), __eventLoop(loop){
    __parser.callback = std::bind<bool>(&HttpChannel::__parserCallback, this, _1, _2, _3, _4, _5);
}



void HttpChannel::__handleReadEvent() {
    vector<char> recvData;
    int readRet = __readn(__fd, recvData);

    switch(__curState){
        case S_IN:
            if(readRet < 0){ // ERROR
                __nextState = S_CLOSE;
            }else{
                if(recvData.empty()){
                    __nextState = readRet == 0 ? S_CLOSE : S_IN;
                }else{
                    // parse data
                    int n = 1, offset = 0;
                    while(n > 0 && recvData.size() > offset){
                        n = __parser(recvData.data()+offset, recvData.size()-offset);
                        if(n <= 0){ // 报文格式错误 或者 未遇到报文结尾
                            vector<char> response = Response::make_xxx_response(BAD_REQUEST);
                            __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());
                        }else{
                            // 此时报文已经在buffer中
                        }
                        // 此时，发送__sendBuf中的数据
                        int sendLen = __writen(__fd, __sendBuf);
                        if(sendLen < 0){ // write遇到错误
                            __nextState = S_CLOSE;
                        }else if(sendLen == __sendBuf.size()){
                            // 发完了就要关闭连接，但是如果未解析完，仍会继续解析，后面如果未发送完，那么就会更改状态
                            __nextState = readRet == 0 ? S_CLOSE : S_IN;
                            __sendBuf.clear();
                        }else{ // 未发完response
                            __nextState = readRet == 0 ? S_OUT : S_IN_OUT;
                            __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
                        }
                        offset += n;
                    }
                }
            }
            break;
        case S_IN_OUT:
            if(readRet < 0){
                __nextState = S_CLOSE;
            }else{
                if(recvData.empty()){
                    __nextState = readRet == 0 ? S_OUT : S_IN_OUT;
                }else{
                    int n = 1, offset = 0;
                    while(n > 0 && recvData.size() > offset){
                        n = __parser(recvData.data()+offset, recvData.size()-offset);
                        if(n <= 0){ // 报文格式错误
                            vector<char> response = Response::make_xxx_response(BAD_REQUEST);
                            __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());
                        }else{ // 已解析完一条报文
                            // do nothing
                        }
                        // 此时，发送__sendBuf中的报文
                        int sendLen = __writen(__fd, __sendBuf);
                        if(sendLen < 0){ // write遇到错误
                            __nextState = S_CLOSE;
                        }else if(sendLen == __sendBuf.size()){
                            __nextState = readRet == 0 ? S_CLOSE : S_IN;
                            __sendBuf.clear();
                        }else{
                            __nextState = readRet == 0 ? S_OUT : S_IN_OUT;
                            __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
                        }
                        offset += n;
                    }
                }
            }
            break;
        default:
            LOG << "HttpChannel::__handleReadEvent： Wrong __curState.\n";
            exit(0);
            break;
    }

    shared_ptr<EventLoop> p = __eventLoop.lock();
    if(!p){
        LOG << "HttpChannel::__eventLoop is nullptr.\n";
        exit(0);
    }


    if(__nextState == S_CLOSE){
        p->delChannel(shared_from_this());
    }else if(__nextState != __curState){
        uint32_t events = 0;
        if(__nextState == S_IN){
            events = EPOLLIN;
        }else if(__nextState == S_OUT){
            events = EPOLLOUT;
        }else if(__nextState == S_IN_OUT){
            events = EPOLLIN | EPOLLOUT;
        }
        setEvent(events);
        p->modChannel(shared_from_this());
    }
    __curState = __nextState;


    //////////////////////////////////////////////////////////////////////////////////////////////////////
//    vector<char> data;
//
//    int ret = __readn(__fd, data);
//
//    if(ret < 0){ // 发生错误
//        __state = NO_RDWR;
//    }else if(ret == 0){ // 收到FIN
//        __state = NO_RD;
//    }else{
//        __state = NORMAL;
//    }
//
//    // 解析报文，填入__sendBuf，然后尝试发送。该过程中__state状态可能发生改变。
//    if(__state != NO_RDWR && !data.empty()){ // 需要解析报文
//        // parse() 返回-1，表示解析失败；返回0，表示未读到request结束；返回>0，表示解析成功一次request，返回的是已读字符数目。
//        int n = 1, offset = 0;
//        while(n > 0 && data.size() > offset){
//            n = __parser(data.data()+offset, data.size()-offset);
//            if(n < 0){ // 报文格式错误
//                __state = NO_RD;
//                vector<char> response = make_response(BAD_REQUEST);
//                __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());
//            }else if(n == 0){ // 未遇到报文的结尾
//                if(__state == NO_RD){
//                    vector<char> response = make_response(BAD_REQUEST);
//                    __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());
//                }else{
//                    // do nothing
//                }
//            }else{ // 已解析完一条报文
//                // do nothing
//            }
//
//            // 此时，发送__sendBuf中的报文
//            int sendLen = __writen(__fd, __sendBuf);
//            if(sendLen < 0){ // write遇到错误
//                __state = NO_RDWR;
//            }else{
//                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
//                // do nothing
//            }
//            offset += n;
//        }
//    }
//
//    shared_ptr<EventLoop> p = __eventLoop.lock();
//    if(!p){
//        throw logic_error("HttpChannel::__eventLoop is nullptr.");
//    }
//    // 根据写缓冲区是否有数据，以及__state的值，来判断下一步的动作
//    if(__state == NO_RDWR){
//        p->delChannel(shared_from_this());
//    }else if(__state == NO_RD){
//        if(__sendBuf.empty()){
//            p->delChannel(shared_from_this());
//            __state = NO_RDWR;
//        }else{
//            shutdown(__fd, SHUT_RD);
//            if(getEvent() != EPOLLOUT){ // 如果之前关注IN 或者 之前 没关注OUT
//                setEvent(EPOLLOUT);
//                p->modChannel(shared_from_this());
//            }
//        }
//    }else {
//        if(__sendBuf.empty()){ // 仅关注可读
//            if(getEvent() != EPOLLIN){
//                setEvent(EPOLLIN);
//                p->modChannel(shared_from_this());
//            }
//        }else{ // 继关注可读又关注可写
//            if(getEvent() != EPOLLIN|EPOLLOUT){
//                setEvent(EPOLLIN|EPOLLOUT);
//                p->modChannel(shared_from_this());
//            }
//        }
//    }
}


void HttpChannel::__handleWriteEvent() {
    shared_ptr<EventLoop> p = __eventLoop.lock();
    if(!p){
        LOG << "HttpChannel::__eventLoop is nullptr.\n";
        exit(0);
    }

    switch(__curState){
        case S_OUT:
            if(__sendBuf.empty()){
                __nextState = S_CLOSE;
            }else{
                int sendLen = __writen(__fd, __sendBuf);

                if(sendLen < 0 || sendLen == __sendBuf.size()){ // 发送错误 或者 发送完
                    __nextState = S_CLOSE;
                    __sendBuf.clear();
                }else{ // 发送未完
                    __nextState = S_OUT;
                    __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
                }
            }
            break;
        case S_IN_OUT:
            if(__sendBuf.empty()){
                __nextState = S_IN;
            }else{
                int sendLen = __writen(__fd, __sendBuf);

                if(sendLen < 0){ // write发生错误
                    __nextState = S_CLOSE;
                    p->delChannel(shared_from_this());
                }else if(sendLen == __sendBuf.size()){ // 发送完
                    __nextState = S_IN;
                    __sendBuf.clear();
                }else{ // 没发送完
                    __nextState = S_IN_OUT;
                    __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
                }
            }
            break;
        case S_IN: // 说明__handleWriteEvent需要发送的数据，在__handleReadEvent里面就已经做完了
        case S_CLOSE: // 说明之前执行过__handleReadEvent，并且已经delChannnel。
            return;
        default:
            break;
    }

    if(__nextState == S_CLOSE){
        p->delChannel(shared_from_this());
    }else if(__nextState != __curState){
        uint32_t events = 0;
        if(__nextState == S_IN){
            events = EPOLLIN;
        }else if(__nextState == S_OUT){
            events = EPOLLOUT;
        }else if(__nextState == S_IN_OUT){
            events = EPOLLIN | EPOLLOUT;
        }
        setEvent(events);
        p->modChannel(shared_from_this());
    }
    __curState = __nextState;

//    shared_ptr<EventLoop> p = __eventLoop.lock();
//    if(!p){
//        throw logic_error("HttpChannel::__eventLoop is nullptr.");
//    }
//
//    if(__state == NO_RDWR){
//        // 说明handleRead里面已经delChannel了。
//        return;
//    }else if(__state == NO_RD){
//        if(__sendBuf.empty()/*缓冲区没有数据*/){
//            __state = NO_RDWR;
//            p->delChannel(shared_from_this());
//        }else{
//            int sendLen = __writen(__fd, __sendBuf);
//
//            if(sendLen < 0 || sendLen == __sendBuf.size()){ // 发送错误 或者 发送完
//                __state = NO_RDWR;
//                p->delChannel(shared_from_this());
//            }else{ // 发送未完
//                if(getEvent() != EPOLLOUT){
//                    setEvent(EPOLLOUT);
//                    p->modChannel(shared_from_this());
//                }
//                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
//            }
//        }
//    }else{
//        if(__sendBuf.empty()){
//            if(getEvent() != EPOLLIN){
//                setEvent(EPOLLIN);
//                p->modChannel(shared_from_this());
//            }
//        }else{
//            int sendLen = __writen(__fd, __sendBuf);
//
//            if(sendLen < 0){ // write发生错误
//                __state = NO_RDWR;
//                p->delChannel(shared_from_this());
//            }else if(sendLen == __sendBuf.size()){ // 发送完
//                if(getEvent() != EPOLLIN){
//                    setEvent(EPOLLIN);
//                    p->modChannel(shared_from_this());
//                }
//                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
//            }else{ // 没发送完
//                if(getEvent() != EPOLLIN|EPOLLOUT){
//                    setEvent(EPOLLIN|EPOLLOUT);
//                    p->modChannel(shared_from_this());
//                }
//                __sendBuf.erase(__sendBuf.begin(), __sendBuf.begin()+sendLen);
//            }
//        }
//    }
}


bool HttpChannel::__parserCallback(string &method, string &url, string &version, std::vector<Header> &requestHeaders,
                          std::vector<char> &messageBody) {
    // 构造报文，添进buffer，返回值应该使得没调用一次parser()返回一次。
    vector<char> response;

    if(version != "HTTP/1.1"){
        response = std::move(Response::make_xxx_response(HTTP_VERSION_NOT_SUPPORTED));
    }else{
        if(method == "GET"){
            response = std::move(Response::make_get_response(url, requestHeaders, messageBody));
        }else if(method == "POST"){
            response = std::move(Response::make_post_response(url, requestHeaders, messageBody));
        }else if(method == "HEAD"){
            response = std::move(Response::make_head_response(url, requestHeaders, messageBody));
        }else{
            response = std::move(Response::make_xxx_response(METHOD_NOT_ALLOWD));
        }
    }

    __sendBuf.insert(__sendBuf.end(), response.begin(), response.end());

    bool keepGoing = false;
    return keepGoing;

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