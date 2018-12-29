//
// Created by tong on 18-12-29.
//

#include "Log.h"
#include <sstream>

template <typename T>
Log& Log::operator<<(T obj) {
    ostringstream ss;
    ss << obj;
    if(!ss.str().empty()){
        bufLock.lock();
        buffer.push(ss.str().c_str(), ss.str().size());
        bufLock.unlock();
        condVar.notify_all();
    }
    return *this;
}


void Log::run() {
    unique_lock<mutex> condLock(bufLockMtx);
    while(running){
        condVar.wait_for(condLock, );

        if(buffer.hasFullBlock()){
            size_t ret = buffer.pop(writeBuf);
            if(ret > 0){
                
            }
        }

        unique_lock<mutex> condLock(condLockMtx);

    }
}