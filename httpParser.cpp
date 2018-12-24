#include <iostream>

enum ParseState{
    SUCCESS,
    FAILURE,
    NOT_FINISHED
};

enum PARSE_STATE{
    REQ_START = 0,
    REQ_LINE,
    REQ_HEADER,
    REQ_BODY,
    REQ_FAIL
};



enum VERSION_STATE{
    CHAR_H,
    CHAR_T1,
    CHAR_T2,
    CHAR_P,
    DIGIT1,
    DOT,
    DIGIT2
};


enum HEADER_STATE{
    HEADER_START,
    KEY,
    COLON,
    VALUE
};


bool isTockenChar(char c){
    return true;
}

inline bool isSP(char c){
    return c == ' ';
};

bool isUrlChar(char c){
    return true;
}

class ParseHttpVersion{
public:
    int operator()(char c){ // 0 success, -1 fail, 1 not finished
        return 0;
    }
};


class ParseCRLF{
public:
    int operator()(char c){
        return 0;
    }
};



// 应该按chunk parse而不是按char
class ParseRequestLine{
private:
    enum REQ_LINE_STATE{
        LINE_START,
        METHOD, // mean method has been parsed
        SP1,
        URL,
        SP2,
        VERSION,
        CRLF
    };
    REQ_LINE_STATE state = LINE_START; //TODO: 复习类成员的初始化
    ParseHttpVersion parseHttpVersion;
    ParseCRLF parseCRLF;

public:
    ParseState operator()(char c){
        int res = 0;
        switch (state){
            case LINE_START:
                if(isTockenChar(c))
                    state = METHOD;
                else
                    return FAILURE;
                break;
            case METHOD:
                if(isSP(c))
                    state = SP1;
                else if(isTockenChar(c))
                    state = METHOD;
                else
                    return FAILURE;
                break;
            case SP1:
                if(isUrlChar(c))
                    state = URL;
                else
                    return FAILURE;
            case URL:
                if(isUrlChar(c))
                    state = URL;
                else if(isSP(c))
                    state = SP2;
                else
                    return FAILURE;
            case SP2:
                res = parseHttpVersion(c);
                if(res == NOT_FINISHED)
                    state = SP2;
                else if(res == SUCCESS)
                    state = VERSION;
                else
                    return FAILURE;
            case VERSION:
                res = parseCRLF(c);
                if(res == NOT_FINISHED)
                    state = VERSION;
                else if(res == SUCCESS)
                    state = CRLF;
                else
                    return FAILURE;
            default:
                break;
        }

        return state == CRLF ? SUCCESS : NOT_FINISHED;
    }
};



int main() {
    int state = REQ_START;

    char buf[1024];
    int i=0;
    while(i++ < 1024){
        char c = buf[i];

        switch(state){
            case REQ_START:
                parseRequestLine();

;
            case REQ_LINE:
                if(isSP(c)){
                    state = REQ
                }else if(isTockenChar(c))
            default:
                break;
        }

        if()
    }


    return 0;
}