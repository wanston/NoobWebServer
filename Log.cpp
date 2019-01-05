//
// Created by tong on 18-12-29.
//

#include "Log.h"
#include <cstring>


Log::Log(const string& filePath, int flushInterval)
: filePath(filePath), flushInterval(chrono::duration<int>(flushInterval)), running(true), fout(nullptr) {
    // fout
    if(!filePath.empty()){
       fout = new ofstream(filePath);
       if(!(*fout)){
           throw runtime_error("Log file open error!");
       }
    }else{
        fout = &cout;
    }

    // run
    logThread = thread(&Log::logThreadFn, this);
}


Log::~Log() {
    // join
    running = false;
    logThread.join();
    if(fout != &cout)
        delete fout;
}


void Log::logThreadFn() {
    while(running){
        mutex m;
        unique_lock<mutex> condLock(m);
        condVar.wait_for(condLock, flushInterval);

        if(buffer.ready()){
            while(buffer.ready()){
                string str = buffer.pop();
                *fout << str;
            }
        }else{
            unique_lock<mutex> bufLock(bufLockMtx);
            if(!buffer.ready()){
                if(!buffer.empty()) {
                    buffer.makeReady();
                }else
                    continue;
            }
            bufLock.unlock();
            string str = buffer.pop();
            *fout << str;
        }
    }
}


LogBuffer::LogBuffer(size_t sz) : blockSize(sz) {
    front = back = new Block(sz);
//    back = new Block(sz);
    front->pre = back;
    back->pre = front;
}

LogBuffer::~LogBuffer() {
    Block *p = front->pre;
    while(front != p){
        Block *tmp = p->pre;
        delete p;
        p = tmp;
    }
    delete p;
}

void LogBuffer::push(const string &str) {
    // push的时候从队列尾部push
    size_t  ret = 0;
    while ((ret += back->push(str.c_str()+ret, str.size()-ret)) != str.size()){ // 当前buffer不够的情况
        back->ready = true;
        back = getBlockAfterBack();
        back->ready = false;
    }

    if(back->ready){ // 当前buffer刚好够
        back = getBlockAfterBack();
        back->ready = false;
    }
}

/**
 * pop 处队列头部的ready的block
 * **/
string LogBuffer::pop() {
    string s;
    if(ready()){ // 必须保证队列里面有未ready的block
        s = std::move(front->pop());
        front->ready = false;
        front = front->pre;
    }

    return s;
}

bool LogBuffer::ready() {
    return front->ready;
}


bool LogBuffer::empty() {
    return back->empty();
}


void LogBuffer::makeReady() {
    back->ready = true;
    back = getBlockAfterBack();
}


Block* LogBuffer::getBlockAfterBack() {
    if(back->pre == front){
        auto *p  = new Block(blockSize);
        Block *tmp = back->pre;
        back->pre = p;
        p->pre = tmp;
        p->ready = false;
    }
    return back->pre;
}


Block::Block(size_t size): size(size), cur(0), pre(nullptr), ready(false) {
    buf = new char[size];
}


Block::~Block() {
    delete[] buf;
}


bool Block::empty() {
    return cur == 0;
}


size_t Block::push(const char *src, size_t sz) {
    size_t left = size - cur;
    size_t cpsz = left > sz ? sz : left;
    memcpy(buf+cur, src, cpsz);
    cur += cpsz;
    return cpsz;
}


string Block::pop() {
    string ret(buf, buf+cur);
    cur = 0;
    return ret;
}