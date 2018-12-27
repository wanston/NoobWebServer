#include <iostream>
#include <cstring>
#include <algorithm>

#include "httpParser.h"




bool isSeparatorChar(char c){
    const char sep[] = "()<>@,;:\\\"/[]?={}\t ";
    for(int i=0; i<19; i++){
        if(c == sep[i])
            return true;
    }
    return false;
}

bool isTockenChar(char c){ // any CHAR except CTLs or separators
    bool except = (c >= 0 && c <= 31) || c == 127 || isSeparatorChar(c);
    return !except;
}

inline bool isSP(char c){
    return c == ' ';
};



inline bool isHT(char c){
    return c == '\t';
};

bool isTextChar(char c){
    return isTockenChar(c) || isSeparatorChar(c);
}

inline bool isCR(char c){
    return c == '\r';
};

inline bool isLF(char c){
    return c == '\n';
};

bool isUrlChar(char c){
    const char url[] = "!*'();:@&=+$,/?#[]-_.~%";

    if(isalpha(c) || isdigit(c))
        return true;

    for(int i=0; i<23; i++){
        if(c == url[i])
            return true;
    }
    return false;
}


int ParseHttpVersion::operator()(const char buf[], int sz){
    int i=0;
    for(; i<sz && state != DIGIT2 && state != FAILURE; i++){
        char c = buf[i];
        version.push_back(c);
        switch (state){
            case START:
                state = (c == 'H' ? CHAR_H : FAILURE);
                break;
            case CHAR_H:
                state = (c == 'T' ? CHAR_T1 : FAILURE);
                break;
            case CHAR_T1:
                state = (c == 'T' ? CHAR_T2 : FAILURE);
                break;
            case CHAR_T2:
                state = (c == 'P' ? CHAR_P : FAILURE);
                break;
            case CHAR_P:
                state = (c == '/' ? SLASH : FAILURE);
                break;
            case SLASH:
                state = (isdigit(c) ? DIGIT1 : FAILURE);
                break;
            case DIGIT1:
                state = (c == '.' ? DOT : FAILURE);
                break;
            case DOT:
                state = (isdigit(c) ? DIGIT2 : FAILURE);
                break;
            default:
                break;
        }
    }

    if(state == DIGIT2){
        return i;
    }
    if(state == FAILURE){
        return -1;
    }

    return 0; // 表示未完成
}


void ParseHttpVersion::clear(){
    state = START;
    version.clear();
}


int ParseCRLF::operator()(const char buf[], int sz){
    for(int i=0; i<sz && state != SUCCESS && state != FAILURE; i++){
        char c = buf[i];
        switch (state){
            case CR:
                state = (c == '\r' ? LF : FAILURE);
                break;
            case LF:
                state = (c == '\n' ? SUCCESS : FAILURE);
            default:
                break;
        }
    }

    if(state == SUCCESS) {
        return 2;
    }else if(state == FAILURE) {
        return -1;
    }else{
        return 0;
    }
}


void ParseCRLF::clear(){
    state = CR;
}


// 应该按chunk parse而不是按char
int RequestLineParser::operator()(char buf[], int sz){ // -1 失败，0未完，>0是消耗的字符数目
    int res = 0;
    for(int i=0; i<sz; i++){
        char c = buf[i];
        switch (state) {
            case START:
                if (isTockenChar(c)) {
                    state = METHOD;
                    method.push_back(c);
                }else
                    return -1;
                break;
            case METHOD:
                if (isSP(c))
                    state = SP1;
                else if (isTockenChar(c)) {
                    state = METHOD;
                    method.push_back(c);
                }else
                    return -1;
                break;
            case SP1:
                if (isUrlChar(c)) {
                    state = URL;
                    url.push_back(c);
                }else
                    return -1;
                break;
            case URL:
                if (isUrlChar(c)) {
                    state = URL;
                    url.push_back(c);
                }else if (isSP(c))
                    state = SP2;
                else
                    return -1;
                break;
            case SP2:
                res = parseHttpVersion(buf+i, sz-i);
                if (res == 0) { // not finished
                    state = SP2;
                    i = sz - 1;
                } else if (res > 0) {  // success，res表示version包含的字符数目
                    state = VERSION;
                    version = parseHttpVersion.version;
                    parseHttpVersion.clear();
                    i = i + res - 1;
                }else{ // failure
                    parseHttpVersion.clear();
                    return -1;
                }
                break;
            case VERSION:
                res = parseCRLF(buf+i, sz-i);
                if(res == 0){
                    state = VERSION;
                    i = sz - 1;
                }else if(res > 0){
                    state = CRLF;
                    parseCRLF.clear();
                    i = i + res - 1; // 当前解析完的字符的索引
                    return i+1; // 返回解析的字符数目
                }else{
                    parseCRLF.clear();
                    return -1;
                }
                break;
            default:
                break;
        }
    }

    return 0; // 表示not finished
}


void RequestLineParser::clear(){
    state = START;
    method.clear();
    url.clear();
    version.clear();
}



/**
 * 返回值：
 *  返回>=0，解析成功，数值表示采纳的字符的数目；
 *  返回-1表示解析失败；
 *  返回-2表示未完成，仍需要进一步解析。
 * **/
PARSED_STATE RequestHeadersParser::operator()(const char *buf, int sz, int &used) {
    int i = 0;
    bool tag = false;
    for(i=0; i<sz && state!=GOOD && state!=BAD; i++){
        char c = buf[i];
        switch (state){
            case KEY:
                if(isTockenChar(c)){
                    state = COLON;
                    key.push_back(c);
                }else{
                    state = BAD;
                }
                break;
            case COLON:
                if(isTockenChar(c)){
                    state = COLON;
                    key.push_back(c);
                }else if(c == ':'){
                    state = CR1;
                }else{
                    state = BAD;
                }
                break;
            case CR1:
                if(isSP(c) || isHT(c)){
                    state = CR1;
                    if(tag)
                        value.push_back(' ');
                    tag = false;
                }else if(isTextChar(c)){
                    state = CR1;
                    value.push_back(c);
                    tag = true;
                }else if(isCR(c)){
                    state = LF1;
                }else{
                    state = BAD;
                }
                break;
            case LF1:
                state = (isLF(c) ? CR2 : BAD);
                break;
            case CR2:
                if(isSP(c) || isHT(c)){
                    state = CR1;
                    if(tag)
                        value.push_back(' ');
                    tag = false;
                }else if(isTockenChar(c)){
                    state = COLON;
                    headers.emplace_back(key, value);
                    key.assign(1, c);
                    value.clear();
                }else if(isCR(c)){
                    state = LF2;
                    headers.emplace_back(key, value);
                }else{
                    state = BAD;
                }
                break;
            case LF2:
                state = (isLF(c)? GOOD : BAD);
                break;
            default:
                break;
        }
    }

    used = i;
    if(state == GOOD){
        return S_SUCCESS;
    }else if(state == BAD){
        return S_FAILURE;
    }else{
        return S_PARSING;
    }
}


void RequestHeadersParser::clear() {
    state = KEY;
    key.clear();
    value.clear();
    headers.clear();
}


PARSED_STATE HttpRequestParser::operator()(char *buf, int sz, int &used) {
    int i = 0;
    for(; i< sz && state!=GOOD && state!=BAD; i++){
        int ret = 0;
        PARSED_STATE substate;
        switch (state) {
            case LINE:
                ret = requestLineParser(buf + i, sz - i);
                if (ret == 0) { // 未完
                    i = sz - 1;
                } else if (ret > 0) { // 成功
                    state = HEADERS;
                    i = i + ret - 1;
                    requestMethod = requestLineParser.method;
                    requestUrl = requestLineParser.url;
                    httpVersion = requestLineParser.version;
                } else { // 失败
                    state = BAD;
                }
                break;
            case HEADERS:
                substate = requestHeadersParser(buf + i, sz - i, ret);
                if(substate == S_PARSING){
                    i = sz - 1;
                }else if(substate == S_SUCCESS){
                    i = i + ret - 1;
                    requestHeaders = std::move(requestHeadersParser.headers);
                    requestHeadersParser.clear();

                    if(prepareContentLengthAndBodyType()){
                        if(bodyType == NO_BODY){
                            state = GOOD;
                        }else if(bodyType == IDENTITY){
                            state = BODY;
                            identityBodyParser.setContentLength(contentLength);
                        }else{
                            state = BODY;
                        }
                    }else{
                        state = BAD;
                    }
                }else{
                    state = BAD;
                }
                break;
            case BODY:
                if(bodyType == IDENTITY){
                    int tmp;
                    PARSED_STATE s = identityBodyParser(buf+i, sz-i, tmp);
                    if(s == S_PARSING){
                        state = BODY;
                        i = sz- 1;
                    }else if(s == S_SUCCESS){
                        state = GOOD;
                        i = i + tmp - 1;
                        messageBody = std::move(identityBodyParser.body);
                        identityBodyParser.clear();
                    }else{
                        state = BAD;
                    }
                }

                if(bodyType == CHUNKED){
                    int tmp;
                    PARSED_STATE s = chunkedBodyParser(buf+i, sz-i, tmp);
                    if(s == S_PARSING){
                        state = BODY;
                        i = sz- 1;
                    }else if(s == S_SUCCESS){
                        state = GOOD;
                        i = i + tmp - 1;
                        messageBody = std::move(chunkedBodyParser.body);
                        chunkedBodyParser.clear();
                    }else{
                        state = BAD;
                    }
                }
                break;
            default:
                break;
        }
    }

    used = i;
    if(state == GOOD){
        return S_SUCCESS;
    }else if(state == BAD){
        return S_FAILURE;
    }else{
        return S_PARSING;
    }
}


void HttpRequestParser::clear() {
    state = LINE;
    bodyType = NO_BODY;
    contentLength = -1;
    requestMethod.clear();
    requestUrl.clear();
    httpVersion.clear();
    requestHeaders.clear();
    messageBody.clear();
}


bool HttpRequestParser::prepareContentLengthAndBodyType() { // TODO 优化逻辑
    bodyType = NO_BODY;
    contentLength = -1;
    for(Header pair : requestHeaders){
        string key = pair.first;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if(key == "transfer-encoding"){
            string chunked = "chunked";
            string identity = "identity";
            if(std::equal(pair.second.end()-chunked.size(), pair.second.end(), chunked.begin())){
                bodyType = CHUNKED;
                return true;
            }else if(std::equal(pair.second.end()-identity.size(), pair.second.end(), identity.begin())){
                bodyType = IDENTITY;
                if(contentLength >= 0)
                    return true;
            }else{
                return false;
            }
        }

        if(key == "content-length"){
            try {
                contentLength = std::stoi(pair.second);
            }
            catch(std::invalid_argument& e) {
                contentLength = -1;
            }
            if(bodyType == IDENTITY)
                return true;
        }
    }

    if(bodyType == IDENTITY){
        return false;
    }else if(contentLength >= 0){
        bodyType = IDENTITY;
        return true;
    }else{
        bodyType = NO_BODY;
        return true;
    }
}


PARSED_STATE IdentityBodyParser::operator()(char buf[], int sz, int &used){
    if(contentLength < 0){
        return S_FAILURE;
    }

    if(parsedLength + sz <= contentLength){
        body.insert(body.end(), buf, buf+sz);
        parsedLength += sz;
        used = sz;
        return S_PARSING;
    }else{
        int len = contentLength - parsedLength;
        body.insert(body.end(), buf, buf + len);
        parsedLength = contentLength;
        used = len;
        return S_SUCCESS;
    }
}


void IdentityBodyParser::setContentLength(int i){
    contentLength = i;
}


void IdentityBodyParser::clear() {
    contentLength = -1;
    parsedLength = 0;
    body.clear();
}


PARSED_STATE ChunkedBodyParser::operator()(char *buf, int sz, int &used) {
    //TODO
    int i=0;
    for(; i<sz; i++){
        char c = buf[i];
        switch (state){
            case START:
                state = (isHex(c) ? CHUNK_SIZE: BAD);
                break;
            case CHUNK_SIZE:
                break;
            case :
                break;
            case :
                break;
            case :
                break;
            case :
                break;
            default:
                break;
        }
    }
}

void ChunkedBodyParser::clear() {
    //TODO


}










using std::cout;
using std::endl;

int main() {
//    char buf0[] = "GET /~prs";
//    char buf1[] = "/15-441-F15/ HTT";
//    char buf2[] = "P/1.1\r";
//    char buf3[] = "\n";
//    ParseRequestLine parseRequestLine;
//    int ret = 0;
//    ret = parseRequestLine(buf0, strlen(buf0));
//    ret = parseRequestLine(buf1, strlen(buf1));
//    ret = parseRequestLine(buf2, strlen(buf2));
//    ret = parseRequestLine(buf3, strlen(buf3));
//    cout << ret << endl <<
//        parseRequestLine.method << endl <<
//        parseRequestLine.url << endl <<
//        parseRequestLine.version << endl;
//
//
//    cout << endl;
//    parseRequestLine.clear();
//
//    char buf[] = "GET /baidu.com/%as?&ss=kkdsaf HTTP/1.1\r\n";
//    int i = strlen(buf);
//    ret = parseRequestLine(buf, i);
//    cout << ret << endl <<
//         parseRequestLine.method << endl <<
//         parseRequestLine.url << endl <<
//         parseRequestLine.version << endl;

//  "Host:\r\n   wwwcscmuedu\tbaidu\r\n \r\n google\r\n"
    char buf[] = "Host:\r\n   wwwcscmuedu\tbaidu\r\n \r\n google\r\nHost: google\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\n\r\n";
    int l = strlen(buf);
    int ret = 0;
    RequestHeadersParser parser;
    PARSED_STATE s = parser(buf, l, ret);

    cout << s << endl;
    cout << ret << endl;
    cout << l << endl;
    for(auto i : parser.headers){
        cout << i.first << " : " << i.second << "\n";
    }



    return 0;
}