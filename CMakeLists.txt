project(NoobHttpParser)

set(CMAKE_CXX_STANDARD 11)

set(SRC_FILES httpParser.cpp main.cpp Log.cpp Log.h)

add_executable(parser ${SRC_FILES})