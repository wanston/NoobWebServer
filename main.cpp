//
// Created by tong on 18-12-28.
//

#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include "HttpParser.h"
#include "Log.h"
#include "Server.h"

using std::cout;
using std::endl;

typedef std::string string;

int callback(void *data, string &method, string &url, string &version, std::vector<Header> &requestHeaders, std::vector<char>& messageBody){
    cout << "----------------------------" << '\n';
    cout << method << ' '
         << url << ' '
         << version << '\n';
    for(auto i : requestHeaders){
        cout << i.first << ":" << i.second << "\n";
    }
    for(char c : messageBody){
        cout << c;
    }
    cout << '\n';
    cout << "----------------------------" << '\n';
    return 1;
}

void testHttpParser() {
    char buf1[] = "GET / HTTP/1.1\r\nHost:     wwwcscmuedu\tbaidu\r\n \r\n google\r\nHost: google\r\nConnection: keep-alive\r\ncontent-length:  \t 15\r\nUpgrade-Insecure-Requests: 1\r\n\r\nThis is a body!POST /index.html HTTP/1.1\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n25\r\nThis is the data in the first chunk\r\n\r\n1C\r\nand this is the second one\r\n\r\n3\r\ncon\r\n8\r\nsequence\r\n0\r\n\r\nG";
    int l1 = sizeof(buf1) - 1;
    int ret = 0;
    cout << "Buf size: " << l1 << endl;

    HttpRequestParser parser;
    parser.callback = &callback;
    ret = parser(buf1, l1);
    cout << "Used: " << ret << endl;

    parser.reset();

    parser.callback = &callback;
    ret = parser(buf1, l1);
}

void testLog(){

    Log log("log.log");
    log << "abcd" << '\n';
    log << string("test sucess") << '\n';
    log << "end" << '\n';
}


int main() {
    Server s(8080);
    s.run();
    return 0;
}