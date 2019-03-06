//
// Created by tong on 19-3-1.
//
#include <iostream>
#include <unistd.h>
#include <mutex>

using namespace std;
// .h
class singleton
{
private:
    singleton(){}; // 禁止直接使用构造函数
    static mutex m;
    static singleton* p;


public:
    struct GC{
        ~GC(){
            if (singleton::p) {
                delete singleton::p;
                singleton::p = nullptr;
            }
        }
    };
    GC gc;


    ~singleton(){
        cout << "~singleton";
    }
    static singleton* getInstance(){
        if (p == nullptr)
        {
            m.lock();
            if (p == nullptr)
                p = new singleton();
            m.unlock();
        }
        return p;
    };
};
// .cpp
mutex singleton::m;
singleton* singleton::p = nullptr;
//singleton::GC singleton::gc;

int main(){
    singleton* p = singleton::getInstance();

    cout << "experiment" << endl;
//    f();
//    A* p =A::get();
//    delete p;
    return 0;
}
