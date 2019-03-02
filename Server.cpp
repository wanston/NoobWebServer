//
// Created by tong on 19-2-16.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory.h>
#include "Server.h"
#include "Log.h"


Server::Server(int port, unsigned int threads, int timeout) : __port(port), __listenFd(-1), __reactor(threads, timeout) {
    // 检查port值，取正确区间范围
    if (__port < 0 || __port > 65535){
        LOG << "Invalid port!\n";
        exit(0);
    }
}


/**
 * Description:
 *  初始化，监听描述符
 * **/
void Server::run() {
    if((__listenFd = __makeListenFd()) == -1){
        LOG << "Invalid listen fd. Error: " << strerror(errno) << "\n";
        exit(0);
    }

    while(__running){ // 需要eventfd来唤醒accept
        struct sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int acceptedFd = 0;
        if ((acceptedFd = accept(__listenFd, (struct sockaddr *) &clientAddr, &clientSize)) <= 0) {
            LOG << "Accept error. Error: " << strerror(errno) << "\n";
        }

        // 在这里要注意timeout
        __reactor.addChannel(acceptedFd);
    }
}


int Server::__makeListenFd() const {
    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    // 消除bind时"Address already in use"错误
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        return -1;

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)__port);
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // 开始监听，最大等待队列长为LISTENQ
    int max_listen_num = 2048;
    if(listen(listen_fd, max_listen_num) == -1)
        return -1;

    // 无效监听描述符
    if(listen_fd == -1)
    {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}
