project(NoobWedServer)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )

set(SRC_FILES httpParser.cpp main.cpp Log.cpp Server.cpp Server.h Reactor.cpp Reactor.h Channel.cpp Channel.h EventLoop.cpp EventLoop.h EventFdChannel.cpp EventFdChannel.h)

add_executable(parser ${SRC_FILES})