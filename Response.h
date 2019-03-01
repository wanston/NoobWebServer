//
// Created by tong on 19-2-27.
//

#ifndef NOOBWEDSERVER_RESPONSE_H
#define NOOBWEDSERVER_RESPONSE_H

#include <vector>
#include "HttpParser.h"

#define OK 200
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define NOT_FOUND 404
#define METHOD_NOT_ALLOWD 405
#define INTERNAL_SERVER_ERROR 500
#define HTTP_VERSION_NOT_SUPPORTED 505


struct Response {
public:
    static std::vector<char> make_xxx_response(int errorCode);

    static std::vector<char> make_head_response(std::string &url, std::vector<Header> &requestHeaders,
                                               std::vector<char> &messageBody);

    static std::vector<char> make_get_response(std::string &url, std::vector<Header> &requestHeaders,
                                            std::vector<char> &messageBody);

    static std::vector<char> make_post_response(std::string &url, std::vector<Header> &requestHeaders,
                                            std::vector<char> &messageBody);

private:
    static std::string __statusCodeToReason(int code);
    static std::string __urlToPath(const std::string &url);
    static std::string __pathToContentType(const std::string &path);
    // 返回状态码 200 403 404，通过引用获取文件内容
    static int __readFile(std::string path, std::vector<char> &fileContent);
    // 返回状态码 200 403 404，通过引用获取文件大小
    static int __readFileSize(std::string path, int &size);
};


#endif //NOOBWEDSERVER_RESPONSE_H
