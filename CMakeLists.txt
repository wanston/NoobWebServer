project(NoobHttpParser)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )

set(SRC_FILES httpParser.cpp main.cpp Log.cpp)

add_executable(parser ${SRC_FILES})