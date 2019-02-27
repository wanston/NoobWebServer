//
// Created by tong on 19-2-27.
//

#ifndef NOOBWEDSERVER_RESPONSE_H
#define NOOBWEDSERVER_RESPONSE_H

#include <vector>

#define  BAD_REQUEST 400

class Response {
public:
    Response(int errorCode){}
    std::vector<char> to_string();
};

std::vector<char> make_response(int errorCode);

#endif //NOOBWEDSERVER_RESPONSE_H
