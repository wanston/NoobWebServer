//
// Created by tong on 19-3-1.
//

//
// Created by tong on 18-12-28.
//

#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include "HttpParser.h"

using namespace std;

bool keepGoing = true;

bool callback(string &method, string &url, string &version, std::vector<Header> &requestHeaders, std::vector<char>& messageBody){
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
    return keepGoing;
}

int main() {
    char buf[] = "GET / HTTP/1.1\r\nHost:     wwwcscmuedu\tbaidu\r\n \r\n google\r\nHost: google\r\nConnection: keep-alive\r\ncontent-length:  \t 15\r\nUpgrade-Insecure-Requests: 1\r\n\r\nThis is a body!"
                 "POST /index.html HTTP/1.1\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n25\r\nThis is the data in the first chunk\r\n\r\n1C\r\nand this is the second one\r\n\r\n3\r\ncon\r\n8\r\nsequence\r\n0\r\n\r\n"
                 " E";
    int l = sizeof(buf) - 1;
    int ret = 0;
    HttpRequestParser parser;
    cout << "Buf content: \n" << buf << endl;
    cout << "Buf size: \n" << l << endl;


    parser.callback = HttpRequestParser::CallBackType(callback);
    keepGoing = true;
    ret = parser(buf, l);
    cout << "Ret: " << ret << endl;

    parser.reset();

    parser.callback = &callback;
    keepGoing = false;

    ret = 1;
    int offset = 0;
    while(ret >= 0 && offset < l){
        ret = parser(buf+offset, l-offset);
        offset += ret;
        cout << "Ret: " << ret << endl;
    }

    return 0;
}