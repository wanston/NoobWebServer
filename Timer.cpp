//
// Created by tong on 19-2-20.
//

#include "Timer.h"
#include <stdexcept>
#include <sys/time.h>

Timer::Timer(int timeout, std::function<void()> callback) :
__timeout(timeout),
__valid(true),
__callback(std::move(callback))
{
    if(timeout < 0){
        throw std::invalid_argument("Timer::Timer().");
    }

    struct timeval now;
    gettimeofday(&__triggerTime, nullptr);
    __triggerTime.tv_sec += __timeout;
}


bool timevalCompLess(struct timeval a, struct timeval b){
    if(a.tv_sec < b.tv_sec){
        return true;
    }else if(a.tv_sec == b.tv_sec){
        return a.tv_usec < b.tv_usec;
    }else{
        return false;
    }
}