project(NoobHttpParser)

set(CMAKE_CXX_STANDARD 11)

set(SRC_FILES httpParser.cpp httpParser.h)

add_executable(parser ${SRC_FILES})