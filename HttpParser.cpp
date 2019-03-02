#include <iostream>
#include <cstring>
#include <algorithm>

#include "HttpParser.h"




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

inline bool isHex(char c){
    return isdigit(c) || (c >= 'A' && c<= 'F') || (c >= 'a' && c<= 'f');
}


PARSED_STATE ParseHttpVersion::operator()(const char buf[], int sz, int &used){
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

    used = i;
    if(state == DIGIT2){
        return S_SUCCESS;
    }else if(state == FAILURE){
        return S_FAILURE;
    }else{
        return S_PARSING;
    }
}


void ParseHttpVersion::clear(){
    state = START;
    version.clear();
}


// 应该按chunk parse而不是按char
PARSED_STATE RequestLineParser::operator()(char buf[], int sz, int &used){ // -1 失败，0未完，>0是消耗的字符数目
    int res = 0;
    int i = 0;
    PARSED_STATE substate;
    for(; i<sz && state!=GOOD && state!=BAD; i++){
        char c = buf[i];
        switch (state) {
            case METHOD:
                if (isTockenChar(c)) {
                    state = SP1;
                    method.push_back(c);
                }else
                    state = BAD;
                break;
            case SP1:
                if (isSP(c))
                    state = URL;
                else if (isTockenChar(c)) {
                    state = SP1;
                    method.push_back(c);
                }else
                    state = BAD;
                break;
            case URL:
                if (isUrlChar(c)) {
                    state = SP2;
                    url.push_back(c);
                }else
                    state = BAD;
                break;
            case SP2:
                if (isUrlChar(c)) {
                    state = SP2;
                    url.push_back(c);
                }else if (isSP(c))
                    state = VERSION;
                else
                    state = BAD;
                break;
            case VERSION:
                substate = parseHttpVersion(buf+i, sz-i, res);
                i = i + res - 1;
                if (substate == S_PARSING) {
                    state = VERSION;
                } else if (substate == S_SUCCESS) {
                    state = CR;
                    version = parseHttpVersion.version;
                    parseHttpVersion.clear();
                }else{
                    parseHttpVersion.clear();
                    state = BAD;
                }
                break;
            case CR:
                state = (isCR(c) ? LF : BAD);
                break;
            case LF:
                state = (isLF(c) ? GOOD : BAD);
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


void RequestLineParser::clear(){
    state = METHOD;
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
    bool tag = false; // 表示遇到lws时，是否添加SP进value
    for(i=0; i<sz && state!=GOOD && state!=BAD; i++){
        char c = buf[i];
        switch (state){
            case KEY:
                if(isTockenChar(c)){
                    state = COLON;
                    key.push_back(c);
                }else if(isCR(c)){
                    state = LF2;
                }else{
                    state = BAD;
                }
                break;
            case COLON:
                if(isTockenChar(c)){
                    state = COLON;
                    key.push_back(c);
                }else if(c == ':'){
                    tag = false;
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


int HttpRequestParser::operator()(char *buf, int sz) {
    int i = 0;
    for(; i< sz && state!=BAD; i++){
        int ret = 0;
        PARSED_STATE substate;
        switch (state) {
            case LINE:
                substate = requestLineParser(buf + i, sz - i, ret);
                i = i + ret - 1;
                if (substate == S_PARSING) {
                    state = LINE;
                } else if (substate == S_SUCCESS) {
                    state = HEADERS;
                    requestMethod = requestLineParser.method;
                    requestUrl = requestLineParser.url;
                    httpVersion = requestLineParser.version;
                } else {
                    state = BAD;
                }
                break;
            case HEADERS:
                substate = requestHeadersParser(buf + i, sz - i, ret);
                i = i + ret - 1;
                if(substate == S_PARSING){
                    state = HEADERS;
                }else if(substate == S_SUCCESS){
                    requestHeaders = std::move(requestHeadersParser.headers);
                    requestHeadersParser.clear();

                    if(prepareContentLengthAndBodyType()){
                        if(bodyType == NO_BODY){
                            state = LINE;
                            if(callback){
                                if(!callback(requestMethod, requestUrl, httpVersion, requestHeaders, messageBody)){
                                    prepareNextRequest();
                                    return i+1;
                                }
                            }
                            prepareNextRequest();
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
                    i = i + tmp - 1;
                    if(s == S_PARSING){
                        state = BODY;
                    }else if(s == S_SUCCESS){
                        state = LINE;
                        messageBody = std::move(identityBodyParser.body);
                        if(callback){
                            if(!callback(requestMethod, requestUrl, httpVersion, requestHeaders, messageBody)){
                                prepareNextRequest();
                                return i+1;
                            }
                        }
                        prepareNextRequest();
                    }else{
                        state = BAD;
                    }
                }else if(bodyType == CHUNKED){
                    int tmp;
                    PARSED_STATE s = chunkedBodyParser(buf+i, sz-i, tmp);
                    i = i + tmp - 1;
                    if(s == S_PARSING){
                        state = BODY;
                    }else if(s == S_SUCCESS){
                        state = LINE;
                        messageBody = std::move(chunkedBodyParser.body);
                        if(callback){
                            if(!callback(requestMethod, requestUrl, httpVersion, requestHeaders, messageBody)){
                                prepareNextRequest();
                                return i+1;
                            }
                        }
                        prepareNextRequest();
                    }else{
                        state = BAD;
                    }
                }
                break;
            default:
                break;
        }
    }
    return state == BAD ? -i : i;// i表示已读的字符数
}


void HttpRequestParser::reset() {
    state = LINE;
    callback = nullptr;
    bodyType = NO_BODY;
    prepareNextRequest();
}


void HttpRequestParser::prepareNextRequest() {
    contentLength = -1;
    requestMethod.clear();
    requestUrl.clear();
    httpVersion.clear();
    requestHeaders.clear();
    messageBody.clear();
    requestLineParser.clear();
    requestHeadersParser.clear();
    identityBodyParser.clear();
    chunkedBodyParser.clear();
}


bool HttpRequestParser::prepareContentLengthAndBodyType() { // TODO 优化逻辑
    bodyType = NO_BODY;
    contentLength = -1;
    for(Header pair : requestHeaders){
        std::string key = pair.first;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if(key == "transfer-encoding"){
            std::string chunked = "chunked";
            std::string identity = "identity";
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
        used = 1;
        return S_FAILURE;
    }

    if(parsedLength + sz < contentLength){
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
    int i=0, tmp=0;
    PARSED_STATE s;
    for(; i<sz && state!=GOOD && state!=BAD; i++){
        char c = buf[i];
        switch (state){
            case START:
                state = (isHex(c) ? CHUNK_SIZE: BAD);
                chunkSize.push_back(c);
                break;
            case CHUNK_SIZE:
                if(isCR(c)){
                    state = CRLF1;
                }else if(c == ';'){
                    state = EXTENSION;
                }else if(isHex(c)){
                    chunkSize.push_back(c);
                    state = CHUNK_SIZE;
                }else{
                    state = BAD;
                }
                break;
            case EXTENSION:
                if(isCR(c)){
                    state = CRLF1;
                }else if(!(isTockenChar(c) || c == ';' || c == '=')){
                    state = BAD;
                }
                break;
            case CRLF1:
                if(isLF(c)){
                    int cksz;
                    try {
                        cksz = std::stoi(chunkSize, nullptr, 16);
                        chunkSize.clear();
                    }catch (std::invalid_argument &e){
                        cksz = -1;
                    }
                    if(cksz > 0){
                        state = DATA;
                        chunkParser.setContentLength(cksz);
                    }else if(cksz == 0){
                        state = TRAILER;
                    }else{
                        state = BAD;
                    }
                }else{
                    state = BAD;
                }
                break;
            case DATA: // TODO 优化，这里拷贝headers太费时间
                s = chunkParser.operator()(buf+i, sz-i, tmp);
                i = i + tmp - 1;
                if(s == S_SUCCESS){
                    state = CR2;
                    body.insert(body.end(), chunkParser.body.begin(), chunkParser.body.end());
                    chunkParser.clear();
                }else if(s == S_FAILURE){
                    state = BAD;
                }else{
                    state = DATA;
                }
                break;
            case TRAILER:
                s = trailerParser.operator()(buf+i, sz-i, tmp);
                i = i + tmp - 1;
                if(s == S_SUCCESS){
                    state = GOOD;
                    headers.insert(headers.end(), trailerParser.headers.begin(), trailerParser.headers.end());
                    trailerParser.clear();
                }else if(s == S_FAILURE){
                    state = BAD;
                }else{
                   state = TRAILER;
                }
                break;
            case CR2:
                state = (isCR(c) ? LF2 : BAD);
                break;
            case LF2:
                state = (isLF(c) ? START : BAD);
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

void ChunkedBodyParser::clear() {
    state = START;
    chunkSize.clear();
    body.clear();
    headers.clear();
    chunkParser.clear();
    trailerParser.clear();
}
