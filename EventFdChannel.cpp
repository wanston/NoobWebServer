//
// Created by tong on 19-2-18.
//

#include <unistd.h>
#include <string.h>
#include "EventFdChannel.h"
#include "Log.h"


void EventFdChannel::__handleReadEvent() {
    uint64_t one = 1;
    ssize_t r = read(__fd, &one, sizeof(one));
    if(r != sizeof(one)){
        LOG << "Error in EventFdChannel::__handleReadEvent: " << strerror(errno) << "\n";
        exit(0);
    }
}


