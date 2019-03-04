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
    explicit LogBuffer(size_t sz = 1024);
    ~LogBuffer();
    void push(const string &str);
    /**
     * 功能：pop出标记为ready的block内的数据
     * 返回值：pop出的数据的大小，0表示没有满足条件的block
     * **/
    string pop(); // 返回pop的字节数, 0表示pop失败

    void makeReady();

    bool ready(); //TODO ready
    bool empty();

    Block *getBlockAfterBack(); // 获得队尾部的空block，此时block已经加入队列，切为空。
};


class Log {
private:
    string filePath;
    ostream *fout;
    LogBuffer buffer;
    mutex bufLockMtx;
    condition_variable condVar;
    bool running;
    chrono::duration<int> flushInterval;
    void logThreadFn();
    thread logThread;
public:
    explicit Log(const string& filePath = "", int flushInterval = 1);
    ~Log();
    template <typename T>
    Log& operator <<(T obj);
};

template <typename T>
Log& Log::operator<<(T obj) {
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

// 不能用静态成员来实现单例模式，因为会多处包含静态成员的定义
std::string curTime();

extern Log logger;

#define LOG logger << curTime()

#endif //NOOBHTTPPARSER_LOG_H
