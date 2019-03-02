cmake_minimum_required(VERSION 3.12)

project(NoobWedServer)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )

set(SRC_FILES
        HttpParser.cpp
        main.cpp
        Log.cpp
        Server.cpp
        Reactor.cpp
        Channel.cpp
        EventLoop.cpp
        EventFdChannel.cpp
        TimerManager.cpp
        Timer.cpp
        HttpChannel.cpp
        Response.cpp)

include_directories(.)

add_subdirectory(test)

add_executable(server ${SRC_FILES})
