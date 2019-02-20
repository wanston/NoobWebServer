//
// Created by tong on 19-2-20.
//

#include "TimerManager.h"
#include <sys/time.h>

void TimerManager::registerTimer(TimerPtr timer) {
    __priorityQueue.push(Node(timer, true));
}


void TimerManager::cancelTimer(TimerPtr timer) {
    // 需要从timer找到node,
    // 存一个map, 从指针 到
}


void TimerManager::runPerTick() {
    struct timeval now;

    while(!__priorityQueue.empty()){
        Node node = __priorityQueue.top();
        gettimeofday(&now, nullptr);

        if(!node.valid){ // 未超时就注销了
            __priorityQueue.pop();
        }else if(timevalCompLess(node.timerPtr->__triggerTime, now)){ // 超时了
            node.timerPtr->__callback();
            __priorityQueue.pop();
        }else{
            return;
        }
    }
}