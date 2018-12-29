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

using namespace std;

struct Block{
    char *buf;
    int size;
    int cur;
    bool full;
    Block *next;

    Block(int size) : size(size), cur(0) {
        buf = new char[size];
    }

    ~Block(){
        delete[] buf;
    }
};


/**
 * 该数据结构内部不含任何同步操作，不是线程安全的。
 * **/
class LogBuffer{
private:
    bool empty;
    Block *front;
    Block *back;
    int blockSize;
public:
    LogBuffer();
    ~LogBuffer();
    void push(const char buf[], size_t size);
    /**
     * 功能：pop出标记为full的block内的数据
     * 返回值：pop出的数据的大小，0表示没有满足条件的block
     * **/
    size_t pop(char buf[]); // 返回pop的字节数, 0表示pop失败

    bool hasFullBlock();
};


// TODO: 为什么会有虚假唤醒？
class Log {
private:
    string filePath;
    fstream fout;
    LogBuffer buffer;
    mutex bufLockMtx;
    mutex condLockMtx;
    condition_variable condVar;
    bool running;
    char *writeBuf;
    size_t bufSz;
public:
    Log();
    ~Log();
    /**
     * 开一个后台线程写文件
     * **/
    void run();
    template <typename T>
    Log& operator<<(T obj);
};


#endif //NOOBHTTPPARSER_LOG_H
