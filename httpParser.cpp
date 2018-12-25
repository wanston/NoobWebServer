#include <iostream>
#include <cstring>

#include "httpParser.h"

enum PARSE_STATE{
    REQ_START = 0,
    REQ_LINE,
    REQ_HEADER,
    REQ_BODY,
    REQ_FAIL
};


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

//inline bool isDigit(char c){
//    return c >= '0' && c<= '9';
//}
//
//inline bool isAlpha(char c){
//    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
//}

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
    int operator()(const char buf[], int sz){
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

    void clear(){
        state = START;
        version.clear();
    }
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
    int operator()(const char buf[], int sz){
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

    void clear(){
        state = CR;
    }
};


// 应该按chunk parse而不是按char
class ParseRequestLine{
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

    int operator()(char buf[], int sz){ // -1 失败，0未完，>0是消耗的字符数目
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

    void clear(){
        state = START;
        method.clear();
        url.clear();
        version.clear();
    }

};


/**
 * 返回值：大于0终态，0非终态，-1失败。但是终态不一定是最后一个终态。只有终态不被发现时擦可以
 *      返回1 终态，返回>=0，解析成功，数值表示采纳的字符的数目，返回-1表示解析失败，返回-2表示未完成，仍需要进一步解析。
 * **/
int RequestHeaderParser::operator()(char *buf, int sz) {
    int i = 0;
    bool tag = false;
    for(i=0; i<sz && state!=AHEAD && state!=FAILURE; i++){
        char c = buf[i];
        switch (state){
            case KEY:
                if(isTockenChar(c)){
                    state = COLON;
                    key.push_back(c);
                }else{
                    state = FAILURE;
                }
                break;
            case COLON:
                if(isTockenChar(c)){
                    state = COLON;
                    key.push_back(c);
                }else if(c == ':'){
                    state = CR;
                }else{
                    state = FAILURE;
                }
                break;
            case CR:
                if(isSP(c) || isHT(c)){
                    state = CR;
                    if(tag)
                        value.push_back(' ');
                    tag = false;
                }else if(isTextChar(c)){
                    state = CR;
                    value.push_back(c);
                    tag = true;
                }else if(isCR(c)){
                    state = LF;
                }else{
                    state = FAILURE;
                }
                break;
            case LF:
                state = (isLF(c) ? SUCCESS : FAILURE);
                break;
            case SUCCESS:
                if(isSP(c) || isHT(c)){
                    state = CR;
                    if(tag)
                        value.push_back(' ');
                    tag = false;
                }else if(isTockenChar(c) || isCR(c)){
                    state = AHEAD;
                }else{
                    state = FAILURE;
                }
                break;
            default:
                break;
        }
    }

    if(state == AHEAD){
        return i - 1;
    }else if(state == FAILURE){
        return -1;
    }else{
        return -2;
    }
}


void RequestHeaderParser::clear() {
    state = KEY;
    key.clear();
    value.clear();
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


    char buf[] = "Host:\r\n   wwwcscmuedu\tbaidu\r\n google\r\n\r\n";
    int l = strlen(buf);
    RequestHeaderParser parser;
    int res = parser(buf, l);
    cout << res << endl
         << l << endl
         << parser.key << endl
         << parser.value  << '"' << endl;

    return 0;
}