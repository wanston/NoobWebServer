//
// Created by tong on 19-2-20.
//

#ifndef NOOBWEDSERVER_TIMERMANAGER_H
#define NOOBWEDSERVER_TIMERMANAGER_H

#include "Timer.h"
#include <queue>
#include <vector>

class TimerManager {
public:
    void registerTimer(TimerPtr timer);
    void cancelTimer(TimerPtr timer);
    void runPerTick();
private:

    struct Node{
        TimerPtr timerPtr;
        bool valid;
        Node(TimerPtr t, bool b) : timerPtr(t), valid(b) {}
    };

    struct NodeComp {
        bool operator()(Node a, Node b) const{
            return ! timevalCompLess(a.timerPtr->__triggerTime, b.timerPtr->__triggerTime);
        }
    };
    std::priority_queue<Node, std::vector<Node>, NodeComp> __priorityQueue;
};



#endif //NOOBWEDSERVER_TIMERMANAGER_H
