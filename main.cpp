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

int main(int argc, char *argv[]) {
    unsigned int threads = 4;
    int port = 8080;
    int timeout = 5;
    std::string logPath = "./log.log";
    std::string servePath = "./www";



    // parse args
    char *p;
    int opt;
    const char *str = "n:p:t:l:s:";
    while ((opt = getopt(argc, argv, str))!= -1)
    {
        switch (opt)
        {
            case 'n':
                threads = strtol(optarg, &p, 10);
                if(threads == 0){
                    std::cerr << "Invalid argument. -n" << std::endl;
                    exit(0);
                }
                break;
            case 'p':
                port = strtol(optarg, &p, 10);
                if(port <= 0 || port > 65535){
                    std::cerr << "Invalid option. -p" << std::endl;
                    exit(0);
                }
                break;
            case 't':
                timeout = strtol(optarg, &p, 10);
                if(timeout <= 0){
                    std::cerr << "Invalid option. -t" << std::endl;
                    exit(0);
                }
                break;
            case 'l':
                logPath = optarg;
                break;
            case 's':
                servePath = optarg;
                break;
            default:
                std::cout << "Options:\n"
                        << "-n thread number\n"
                        << "-p port\n"
                        << "-t keep-alive timeout\n"
                        << "-l log file path\n"
                        << "-s serve path" << std::endl;
                exit(0);
                break;
        }
    }

    LOG.setLogFile(logPath);
    Server s(port, threads, timeout, servePath);
    s.run();
    return 0;
}