cmake_minimum_required(VERSION 3.12)

project(NoobWedServer)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )

set(SRC_FILES HttpParser.cpp main.cpp Log.cpp Server.cpp Server.h Reactor.cpp Reactor.h Channel.cpp Channel.h EventLoop.cpp EventLoop.h EventFdChannel.cpp EventFdChannel.h TimerManager.cpp TimerManager.h Timer.cpp Timer.h HttpChannel.cpp HttpChannel.h Response.cpp Response.h test/testHttpParser.cpp)

include_directories(.)

add_subdirectory(test)

add_executable(server ${SRC_FILES})
