//
// Created by tong on 18-12-25.
//

#ifndef NOOBHTTPPARSER_HTTPPARSER_H
#define NOOBHTTPPARSER_HTTPPARSER_H

#include <string>
#include <vector>
#include <functional>

enum PARSED_STATE{
    S_PARSING,
    S_SUCCESS,
    S_FAILURE,
};

typedef std::pair<std::string, std::string> Header;

class ParseHttpVersion{
private:
    enum {
        START,
        CHAR_H,
        CHAR_T1,
        CHAR_T2,
        CHAR_P,
        SLASH,
        DIGIT1,
        DOT,
        DIGIT2,
        FAILURE
    } state = START;
public:
    std::string version;
    PARSED_STATE operator()(const char buf[], int sz, int &used);
    void clear();
};


class RequestLineParser{
private:
    enum {
        METHOD,
        SP1,
        URL,
        SP2,
        VERSION,
        CR,
        LF,
        GOOD,
        BAD
    } state = METHOD;
    ParseHttpVersion parseHttpVersion;

public:
    typedef std::string string;
    string method;
    string url;
    string version;
    PARSED_STATE operator()(char buf[], int sz, int &used);
    void clear();
};


/**
 * 解析的BNF范式： request-header CRLF
 * **/
class RequestHeadersParser{
private:
    enum {
        KEY,
        COLON,
        CR1,
        LF1,
        CR2,
        LF2,
        GOOD,
        BAD
    } state = KEY;

    std::string key;
    std::string value;

public:
    std::vector<Header> headers;
    PARSED_STATE operator()(const char buf[], int sz, int &used);
    void clear();
};


class IdentityBodyParser{
private:
    int parsedLength = 0;
    int contentLength = -1;
public:
    std::vector<char> body;
    PARSED_STATE operator()(char buf[], int sz, int &used);
    void setContentLength(int i);
    void clear();
};


class ChunkedBodyParser{
private:
    enum {
        START,
        CHUNK_SIZE,
        EXTENSION,
        CRLF1,
        DATA,
        TRAILER,
        CR2,
        LF2,
        GOOD,
        BAD
    } state = START;

    std::string chunkSize;
    IdentityBodyParser chunkParser;
    RequestHeadersParser trailerParser;
public:
    std::vector<char> body;
    std::vector<Header> headers;
    PARSED_STATE operator()(char buf[], int sz, int &used);
    void clear();
};


class HttpRequestParser{
public:
    // callback 返回true，表示解析完一条之后继续解析，不反回；返回false，表示解析成功之后即返回。
    typedef std::function<bool (std::string&, std::string&, std::string&, std::vector<Header>&, std::vector<char>&)> CallBackType;
    CallBackType callback = nullptr;
    // 返回<0，表示报文格式错误，解析失败，返回值的绝对值表示已读的字符数，即成功接受的字符数+未成功接受的字符数（即1）；
    // 返回>0的值表示解析未遇到问题，返回值表示已读的字符数。
    // 不会返回0；
    int operator()(char buf[], int sz);
    void reset();
private:
    enum {
        LINE,
        HEADERS,
        BODY,
        BAD
    } state = LINE;

    RequestLineParser requestLineParser;
    RequestHeadersParser requestHeadersParser;
    IdentityBodyParser identityBodyParser;
    ChunkedBodyParser chunkedBodyParser;

    enum BODY_TYPE{
        NO_BODY,
        CHUNKED,
        IDENTITY
    };

    BODY_TYPE bodyType = NO_BODY;
    int contentLength = -1;
    bool prepareContentLengthAndBodyType();
    void prepareNextRequest();
    std::string requestMethod;
    std::string requestUrl;
    std::string httpVersion;
    std::vector<Header> requestHeaders;
    std::vector<char> messageBody; // 是transfer uncode之后的message body
};

#endif //NOOBHTTPPARSER_HTTPPARSER_H
