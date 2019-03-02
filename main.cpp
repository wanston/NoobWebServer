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

void testLog(){

    Log log("log.log");
    log << "abcd" << '\n';
    log << string("test sucess") << '\n';
    log << "end" << '\n';
}


int main() {
    int port = 8080;
    unsigned int threads = 5;
    int timeout = 5;
    Server s(port, threads, timeout);
    s.run();
    return 0;
}