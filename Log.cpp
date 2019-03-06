//
// Created by tong on 18-12-29.
//

#include "Log.h"
#include <cstring>
#include <iomanip>
#include <unistd.h>


Log* Log::__instance = nullptr;
mutex Log::__singletonLock;
string Log::__filePath;
Log::GC Log::__gc;


Log::Log(int flushInterval)
: flushInterval(chrono::duration<int>(flushInterval)), running(true), fout(nullptr) {
    // fout
    if(!__filePath.empty()){
       fout = new ofstream(__filePath);
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
    if(fout != &cout){
        delete fout;
    }
}


void Log::logThreadFn() {
    while(true){
        mutex m;
        unique_lock<mutex> condLock(m);
        condVar.wait_for(condLock, flushInterval);

        if(!running){
            break;
        }

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

        fout->flush();
    }

    while(buffer.ready()){
        string str = buffer.pop();
        *fout << str;
    }
    if(!buffer.empty()){
        buffer.makeReady();
        string str = buffer.pop();
        *fout << str;
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

string LogWrap::__curDate() {
    time_t t;
    struct tm p;
    time(&t);
    localtime_r(&t, &p); //取得当地具体时间
    ostringstream ss;
    ss << (1900 + p.tm_year) << "年" << p.tm_mon << "月" << p.tm_mday << "日" << setfill('0') << setw(2) << p.tm_hour
                                                                        << ":" << setfill('0') << setw(2) << p.tm_min
                                                                        << ":" << setfill('0') << setw(2) <<  p.tm_sec << ": ";
    return ss.str();
}