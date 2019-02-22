//
// Created by tong on 19-2-20.
//

#include "TimerManager.h"
#include <sys/time.h>

void TimerManager::registerTimer(TimerPtr timer) {
    if(timer->__timeout > 0){
        __priorityQueue.push(timer);
    }
}


void TimerManager::unregisterTimer(TimerPtr timer) {
    if(timer){
        timer->__valid = false;
    }
}


void TimerManager::runPerTick() {
    struct timeval now;

    while(!__priorityQueue.empty()){
        TimerPtr timer = __priorityQueue.top();
        gettimeofday(&now, nullptr);

        if(!timer->__valid){ // 失效
            __priorityQueue.pop();
        }else if(timevalCompLess(timer->__triggerTime, now)){ // 有效，并且时间到了
            timer->__valid = false;
            timer->__callback();
            __priorityQueue.pop();
        }else{ // 有效，并且时间未到
            return;
        }
    }
}