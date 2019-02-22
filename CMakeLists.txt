project(NoobWedServer)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )

set(SRC_FILES HttpParser.cpp main.cpp Log.cpp Server.cpp Server.h Reactor.cpp Reactor.h Channel.cpp Channel.h EventLoop.cpp EventLoop.h EventFdChannel.cpp EventFdChannel.h TimerManager.cpp TimerManager.h Timer.cpp Timer.h HttpChannel.cpp HttpChannel.h)

add_executable(server ${SRC_FILES})