//
// Created by tong on 19-2-20.
//

#ifndef NOOBWEDSERVER_TIMER_H
#define NOOBWEDSERVER_TIMER_H

#include <memory>

class Timer {
public:
    // timeout表示多长时间后触发callback, 单位是秒.
    Timer(int timeout, std::function<void()> callback);
    ~Timer() = default;
private:
    int __timeout; // 单位s
    bool __valid;
    struct timeval __triggerTime;
    std::function<void()> __callback;

    friend class TimerManager;
};

typedef std::shared_ptr<Timer> TimerPtr;

bool timevalCompLess(struct timeval a, struct timeval b);

#endif //NOOBWEDSERVER_TIMER_H
