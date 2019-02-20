//
// Created by tong on 19-2-20.
//

#include "Timer.h"
#include <stdexcept>
#include <sys/time.h>

Timer::Timer(int timeout, std::function<void()> callback) : __timeout(timeout), __callback(std::move(callback)){
    if(timeout <= 0){
        throw std::invalid_argument("Timer::Timer().");
    }


    struct timeval now;
    gettimeofday(&__triggerTime, nullptr);
    __triggerTime.tv_sec += __timeout;
}
