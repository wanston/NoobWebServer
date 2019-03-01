//
// Created by tong on 19-2-27.
//

#include <algorithm>
#include <fstream>
#include "Response.h"

using namespace std;

//static string codeToReason(int statusCode){
//    switch(statusCode){
//        case 400:
//            return "Bad request.";
//        case 405:
//            return "Method not allowed.";
//        case 505:
//            return "HTTP version not supported.";
//        default:
//            return "";
//    }
//}


vector<char> responseToText(const string &version, const string &statusCode, const string &reason, const std::vector<Header> &headers, const vector<char> &messageBody){
    vector<char> ret;
    ret.insert(ret.end(), version.begin(), version.end());
    ret.push_back(' ');

    ret.insert(ret.end(), statusCode.begin(), statusCode.end());
    ret.push_back(' ');

    ret.insert(ret.end(), reason.begin(), reason.end());
    ret.push_back('\r');
    ret.push_back('\n');

    for(const Header &h : headers){
        ret.insert(ret.end(), h.first.begin(), h.first.end());
        ret.push_back(':');
        ret.insert(ret.end(), h.second.begin(), h.second.end());
        ret.push_back('\r');
        ret.push_back('\n');
    }

    ret.push_back('\r');
    ret.push_back('\n');

    ret.insert(ret.end(), messageBody.begin(), messageBody.end());
}


vector<char> Response::make_xxx_response(int statusCode) {
    std::vector<Header> headers = {Header("Connection", "close"),
                                   Header("Server", "NoobWebServer/1.0"),
                                   Header("Content-Length", "0")};

    return responseToText("HTTP/1.1", to_string(statusCode), __statusCodeToReason(statusCode), headers, vector<char>());
}


std::vector<char> Response::make_get_response(std::string &url, std::vector<Header> &requestHeaders,
                                               std::vector<char> &messageBody) {
    string path = __urlToPath(url);
    vector<char> fileContent;
    int statusCode = __readFile(path, fileContent); // 可能是200 404 403 等

    if(statusCode != OK){
        return make_xxx_response(statusCode);
    }else{
        std::vector<Header> headers = {Header("Server", "NoobWebServer/1.0"),
                                       Header("Content-Length", to_string(fileContent.size())),
                                       Header("Content-Type", __pathToContentType(path))
                                       //TODO: 添加Date、Last-Modified等
        };
        return responseToText("HTTP/1.1", to_string(OK), __statusCodeToReason(OK), headers, fileContent);
    }
}


std::vector<char> Response::make_post_response(std::string &url, std::vector<Header> &requestHeaders,
                                               std::vector<char> &messageBody) {
    return make_get_response(url, requestHeaders, messageBody);
}


std::vector<char> Response::make_head_response(std::string &url, std::vector<Header> &requestHeaders,
                                               std::vector<char> &messageBody) {
    string path = __urlToPath(url);
    int fileSize=0;
    int statusCode = __readFileSize(path, fileSize); // 可能是200 404 403 等

    if(statusCode != OK){
        return make_xxx_response(statusCode);
    }else{
        std::vector<Header> headers = {Header("Server", "NoobWebServer/1.0"),
                                       Header("Content-Length", to_string(fileSize)),
                                       Header("Content-Type", __pathToContentType(path))
                                       //TODO: 添加Date、Last-Modified等
        };
        return responseToText("HTTP/1.1", to_string(OK), __statusCodeToReason(OK), headers, vector<char>());
    }
}


std::string Response::__statusCodeToReason(int code) {
    const char *reason = nullptr;

    switch(code){
        case 200:
            reason = "OK.";
            break;
        case 400:
            reason = "Bad request.";
            break;
        case 403:
            reason = "Forbidden.";
            break;
        case 404:
            reason = "Not Found.";
            break;
        case 405:
            reason =  "Method not allowed.";
            break;
        case 500:
            reason = "Internal Server Error.";
            break;
        case 505:
            reason =  "HTTP version not supported.";
            break;
        default:
            reason = "";
    }
    return reason;
}


std::string Response::__urlToPath(const std::string &url) {
    // TODO 改成可以设置的路径
    string base = "./www";

    if(url == "/"){
        return base.append("/index.html");
    }

    if(url[0] != '/'){
        base.push_back('/');
    }
    return base.append(url);
}


std::string Response::__pathToContentType(const std::string &path) {
    auto it = find(path.rbegin(), path.rend(), '.');

    string extension;
    if(it != path.rend()){
        extension.assign(path.end()-(it-path.rbegin()), path.end());
    }

    string contentType;
    if(extension == "html" || extension == "htm"){
        contentType = "text/html";
    }else if(extension == "css"){
        contentType = "text/css";
    }else if(extension == "png"){
        contentType = "image/png";
    }else if(extension == "jpg" || extension == "jpeg"){
        contentType = "image/jpg";
    }else if(extension == "gif"){
        contentType = "image/gif";
    }else{}

    return contentType;
}


int Response::__readFile(std::string path, std::vector<char> &fileContent) {
    ifstream file(path, std::ios::in | std::ios::binary);
    if(!file.is_open()){
        if(errno == 2){
            return NOT_FOUND;
        }else if(errno == 13){
            return FORBIDDEN;
        }else{
            return INTERNAL_SERVER_ERROR;
        }
    }else{
        fileContent.assign((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return OK;
    }
}


int Response::__readFileSize(std::string path, int &size) {
    ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        if(errno == 2){
            return NOT_FOUND;
        }else if(errno == 13){
            return FORBIDDEN;
        }else{
            return INTERNAL_SERVER_ERROR;
        }
    }else{
        size = file.tellg();
        return OK;
    }
}