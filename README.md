# NoobWebServer
此项目是自己实现的静态资源服务器，支持HEAD、GET请求，支持长连接，实现了异步日志。

# 使用

# 特性
* 采用多线程+事件循环（epoll）的设计。
* 采用优先队列实现定时器，及时关闭超时连接。 
* 实现了简单的异步日志。
* 实现了基于有限状态机的http parser。parser支持长连接，可解析chunked encoding request。

# 设计
## 并发模型
程序可分为主线程和n个子线程；
主线程循环accept新连接，然后异步唤醒子线程，把连接以轮询的方式分发给子线程；
每个子线程运行一个事件循环（epoll），负责解析request、构造response、以及维护定时器移除超时的连接。

## 异步日志
基本的思路是，log函数向缓冲区写数据，后台线程负责把缓冲区中的数据写入日志文件。当写满了足量的数据或者超过一定时间未有数据写入时，后台线程就把数据写入日志文件中。

具体来说，采用多缓冲区设计，使用两个队列a、b和一个指针p管理这些缓冲区。p指向当前正在使用的缓冲区，队列a保存已经写满的缓冲区，队列b保存空缓冲区。
log函数：向p指向的缓冲区写数据，当写满缓冲区时，就将其push进队列a中，然后从队列b中pop出一个空缓冲区，让p指向它。写完数据后，就唤醒后台线程。
后台线程：唤醒后，检查唤醒的原因是超时还是被log函数唤醒。如果是超时，就把p指向的缓冲区push进队列a中，然后从队列b中pop空缓冲区给p。然后pop出a中所有的缓冲区，将其数据写入文件。

关于条件竞争，log函数入口加锁，后台线程中唤醒原因判断为超时的情况下加锁。

## http parser
http parser采用有限自动机解析request，支持长连接，支持chunked encoding。

查阅[RFC2616](https://www.ietf.org/rfc/rfc2616.txt)确定了Request的Augmented BNF定义，但是出于实现的考虑，稍微化简、更改了部分定义。实际采用的定义如下：

```
Request        = Request-Line             
                 *(message-header CRLF) 
                 CRLF
                 [ message-body ]

Request-Line   = Method SP 
                 Request-URI SP 
                 HTTP-Version CRLF

Method         = tocken

Request-URI    = *( "!" | "*" | "'" | "(" | ")" | ";" |
                    ":" | "@" | "&" | "=" | "+" | "$" |
                    "," | "/" | "?" | "#" | "[" | "]" |
                    "-" | "_" | "." | "~" | "%" | ALPHA | DIGIT )

HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

message-header = tocken ":" *( *<any OCTET except CTLs> | LWS )           
    
message-body   = entity-body | Chunked-Body

entity-body    = *OCTET

Chunked-Body   = *chunk
                 last-chunk
                 trailer
                 CRLF

chunk          = chunk-size [ chunk-extension ] CRLF
                 chunk-data CRLF    
                                  
chunk-size     = 1*HEX

chunk-extension = *(";" *<tocken char or "=" >)

chunk-data     = chunk-size(OCTET)               

last-chunk     = 1*("0") [ chunk-extension ] CRLF

trailer        = *(entity-header CRLF)

entity-header  = message-header


OCTET          = <any 8-bit sequence of data>
CHAR           = <any US-ASCII character (octets 0 - 127)>
UPALPHA        = <any US-ASCII uppercase letter "A".."Z">
LOALPHA        = <any US-ASCII lowercase letter "a".."z">
ALPHA          = UPALPHA | LOALPHA
DIGIT          = <any US-ASCII digit "0".."9">
CTL            = <any US-ASCII control character(octets 0 - 31) and DEL (127)>
CR             = <US-ASCII CR, carriage return (13)>
LF             = <US-ASCII LF, linefeed (10)>
SP             = <US-ASCII SP, space (32)>
HT             = <US-ASCII HT, horizontal-tab (9)>
LWS            = [CRLF] 1*( SP | HT )
HEX            = "A" | "B" | "C" | "D" | "E" | "F"
                     | "a" | "b" | "c" | "d" | "e" | "f" | DIGIT
token          = 1*<any CHAR except CTLs or separators>
separators     = "(" | ")" | "<" | ">" | "@"
                     | "," | ";" | ":" | "\" | <">
                     | "/" | "[" | "]" | "?" | "="
                     | "{" | "}" | SP | HT
```

# 测试结果
使用webbench简单地测了一下，结果如图。
！[](./images/Server测试结果.png)