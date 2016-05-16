project(epollserver)
cmake_minimum_required(VERSION 2.8)

set(HEADERS epollserver.h)
set(SRCS epollserver.cpp
		 srv_main.cpp)
set(DEPENDS pthread)
add_executable(epollserver ${HEADERS} ${SRCS})
target_link_libraries(epollserver ${DEPENDS})
