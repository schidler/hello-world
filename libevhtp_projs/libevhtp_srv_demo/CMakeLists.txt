project(libevhtp_srv_demo)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Debug)

include_directories(/usr/local/libevent-2.0.22/include)
include_directories(/usr/local/libevhtp-1.2.10/include)

link_directories(/usr/local/libevent-2.0.22/lib)
link_directories(/usr/local/libevhtp-1.2.10/lib)

file(GLOB HEADERS *.h)
aux_source_directory(. SRC_LIST)
set(DEPENDS pthread event evhtp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${DEPENDS})
