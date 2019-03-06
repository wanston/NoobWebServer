//
// Created by tong on 18-12-28.
//

#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include "Log.h"
#include "Server.h"

using namespace std;

int main() {
    int port = 8080;
    unsigned int threads = 5;
    int timeout = 5;
    LOG.setLogFile("/home/tong/Project/开发/NoobWebServer/test/log.txt");
    Server s(port, threads, timeout, "./www");
    s.run();
    return 0;
}