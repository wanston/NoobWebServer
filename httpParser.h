//
// Created by tong on 18-12-25.
//

#ifndef NOOBHTTPPARSER_HTTPPARSER_H
#define NOOBHTTPPARSER_HTTPPARSER_H

#include <string>
#include <vector>

enum PARSED_STATE{
    S_PARSING,
    S_SUCCESS,
    S_FAILURE,
};

/**
 * 使用时，若想重新解析新的字符串，则需要调用clear()，清除状态。
 * 如果不清楚，那么对象不会解析新的字符串，会一直持有上一次解析的结果。
 * **/
class ParseCRLF{
private:
    enum CRLF_State{
        CR,
        LF,
        SUCCESS,
        FAILURE
    };
    CRLF_State state = CR;
public:
    int operator()(const char buf[], int sz);
    void clear();
};



class ParseHttpVersion{
private:
    enum VERSION_STATE{
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
    };
    VERSION_STATE state = START;
public:
    std::string version;
    int operator()(const char buf[], int sz);
    void clear();
};



class RequestLineParser{
private:
    enum REQ_LINE_STATE{
        START,
        METHOD, // mean method has been parsed
        SP1,
        URL,
        SP2,
        VERSION,
        CRLF,
        FAILURE
    };
    REQ_LINE_STATE state = START; //TODO: 复习类成员的初始化
    ParseHttpVersion parseHttpVersion;
    ParseCRLF parseCRLF;

public:
    typedef std::string string;
    string method;
    string url;
    string version;
    int operator()(char buf[], int sz);
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
    typedef std::pair<std::string, std::string> Header;

public:
    std::vector<Header> headers;
    PARSED_STATE operator()(const char buf[], int sz, int &used);
    void clear();
};


class IdentityBodyParser{ // 需要cL需要记录已经弄了多少
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
        DATA_START,
        TRAILER,
        DATA,
        CRLF2,
        GOOD,
        BAD
    } state;

public:
    std::vector<char> body;
    PARSED_STATE operator()(char buf[], int sz, int &used);
    void clear();
};


/**
 * 需要支持：
 * 1. Handles persistent streams
 * 2. Decodes chunked encoding
 * **/
class HttpRequestParser{
private:
    enum {
        LINE,
        HEADERS,
        BODY,
        GOOD,
        BAD
    } state;

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

public:
    typedef std::string string;
    typedef std::pair<string, string> Header;

    string requestMethod;
    string requestUrl;
    string httpVersion;
    std::vector<Header> requestHeaders;
    std::vector<char> messageBody; // 是transfer uncode之后的message body

    // 解析的时候需要边解析边检查header，然后采取相应的检测body结尾的方法。
    PARSED_STATE operator()(char buf[], int sz, int &used);
    void clear();
};

#endif //NOOBHTTPPARSER_HTTPPARSER_H
