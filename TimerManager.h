//
// Created by tong on 19-2-20.
//

#ifndef NOOBWEDSERVER_TIMERMANAGER_H
#define NOOBWEDSERVER_TIMERMANAGER_H

#include "Timer.h"
#include <queue>
#include <vector>
#include <algorithm>


class TimerManager {
public:
    void registerTimer(TimerPtr timer);
    void unregisterTimer(TimerPtr timer);
    void runPerTick();
private:

    struct Comp {
        bool operator()(TimerPtr a, TimerPtr b) const{
            return ! timevalCompLess(a->__triggerTime, b->__triggerTime);
        }
    };
    std::priority_queue<TimerPtr, std::vector<TimerPtr>, Comp> __priorityQueue;
};



#endif //NOOBWEDSERVER_TIMERMANAGER_H
