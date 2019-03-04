//
// Created by tong on 19-3-1.
//
#include <iostream>
#include <unistd.h>

using namespace std;

class A{
public:
    ~A(){
        cout << "~A" << endl;
    }
};

A a;

int main(){
    cout << "experiment" << endl;
    exit(0);
    _exit(0);
}