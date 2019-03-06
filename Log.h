//
// Created by tong on 18-12-29.
//

#ifndef NOOBHTTPPARSER_LOG_H
#define NOOBHTTPPARSER_LOG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <sstream>

using namespace std;

struct Block{
private:
    char* buf;
    const size_t size;
    size_t cur; // 已用字符数

public:
    Block *pre;
    bool ready;

    explicit Block(size_t size);
    ~Block();
    size_t push(const char* src, size_t sz);
    string pop();
    bool empty();
};


/**
 * 该数据结构内部不含任何同步操作，不是线程安全的。
 * **/
class LogBuffer{
private:
    Block *front;
    Block *back;
    size_t blockSize;
public:
    explicit LogBuffer(size_t sz = 256);
    ~LogBuffer();
    void push(const string &str);
    /**
     * 功能：pop出标记为ready的block内的数据
     * 返回值：pop出的数据的大小，0表示没有满足条件的block
     * **/
    string pop(); // 返回pop的字节数, 0表示pop失败
    void makeReady();
    bool ready();
    bool empty();
    Block *getBlockAfterBack(); // 获得队尾部的空block，此时block已经加入队列，切为空。
};


class Log {
public:
    static Log* getInstance(){
        if(!__instance){
            __singletonLock.lock();
            if(!__instance){
                __instance = new Log(5);
            }
            __singletonLock.unlock();
        }
        return __instance;
    };

    template <typename T>
    Log& operator <<(T obj){
        ostringstream ss;
        ss << obj;
        if(!ss.str().empty()){
            bufLockMtx.lock();
            buffer.push(ss.str());
            bufLockMtx.unlock();
            if(buffer.ready())
                condVar.notify_all();
        }
        return *this;
    };

    static void setLogFile(const string &s){
        if(__filePath.empty()){
            __filePath = s;
        }
    };

private:
    struct GC{
        ~GC(){
            if(__instance){
                delete __instance;
                __instance = nullptr;
            }
        }
    };

    static Log *__instance;
    static mutex __singletonLock;
    static string __filePath;
    static GC __gc;

    explicit Log(int flushInterval);
    ~Log();
    ostream *fout;
    LogBuffer buffer;
    mutex bufLockMtx;
    condition_variable condVar;
    bool running;
    chrono::duration<int> flushInterval;
    void logThreadFn();
    thread logThread;

};


class LogWrap{
public:
    LogWrap():  __buffer(__curDate()), __dateSize(__buffer.size()) {
    };

    ~LogWrap(){
        if(__buffer.size() > __dateSize){
            *Log::getInstance() << __buffer;
        }
    }

    template <typename T>
    LogWrap& operator<<(const T &obj){
        ostringstream ss;
        ss << obj;
        __buffer.append(ss.str());
        return *this;
    }

    void setLogFile(const string &s){
        Log::setLogFile(s);
    };

private:
    string __buffer;
    size_t __dateSize;
    string __curDate();
};


#define LOG LogWrap()
//#define LOG cout

#endif //NOOBHTTPPARSER_LOG_H
