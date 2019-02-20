//
// Created by tong on 19-2-20.
//

#ifndef NOOBWEDSERVER_TIMER_H
#define NOOBWEDSERVER_TIMER_H

#include <memory>

class Timer;

typedef std::shared_ptr<Timer> TimerPtr;

class Timer {
public:
    // timeout表示多长时间后触发callback, 单位是秒.
    Timer(int timeout, std::function<void()> callback);
    ~Timer() = default;
private:
    int __timeout;
    struct timeval __triggerTime;
    std::function<void()> __callback;

    friend class TimerManager;
};


bool timevalCompLess(struct timeval a, struct timeval b){
    if(a.tv_sec < b.tv_sec){
        return true;
    }else if(a.tv_sec == b.tv_sec){
        return a.tv_usec < b.tv_usec;
    }else{
        return false;
    }
}


#endif //NOOBWEDSERVER_TIMER_H
