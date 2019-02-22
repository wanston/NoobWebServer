//
// Created by tong on 19-2-16.
//

#ifndef NOOBWEDSERVER_SERVER_H
#define NOOBWEDSERVER_SERVER_H

#include "Reactor.h"

class Server {
public:
    Server(int port);
    void run();
private:
    int __port;
    int __listenFd;
    bool __running;
    Reactor __reactor;

    int __makeListenFd() const;
};


#endif //NOOBWEDSERVER_SERVER_H
