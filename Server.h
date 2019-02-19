//
// Created by tong on 19-2-16.
//

#ifndef NOOBWEDSERVER_SERVER_H
#define NOOBWEDSERVER_SERVER_H

#include "Reactor.h"

class Server {
public:
    void Run();
    Server(int port);
private:
    int __port;
    int __listenFd;
    bool __running;
    Reactor __reactor;

    int __BindListenSocket() const;
};


#endif //NOOBWEDSERVER_SERVER_H
