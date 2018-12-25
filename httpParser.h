//
// Created by tong on 18-12-25.
//

#ifndef NOOBHTTPPARSER_HTTPPARSER_H
#define NOOBHTTPPARSER_HTTPPARSER_H

#include <string>

class RequestHeaderParser{
private:
    enum {
        KEY,
        COLON,
        CR,
        LF,
        SUCCESS,
        AHEAD,
        FAILURE,
    } state = KEY;

public:
    std::string key;
    std::string value;
    int operator()(char buf[], int sz);
    void clear();
};

#endif //NOOBHTTPPARSER_HTTPPARSER_H
