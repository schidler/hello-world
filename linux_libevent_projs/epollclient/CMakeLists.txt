project(epollclient)
cmake_minimum_required(VERSION 2.8)
set(HEADERS epollclient.h)
set(SRCS clt_main.cpp
		epollclient.cpp)
set(DEPENDS pthread)
add_executable(epollclient ${SRCS})
target_link_libraries(epollclient ${DEPENDS})
