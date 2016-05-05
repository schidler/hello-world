project(libevent_srv)
cmake_minimum_required(VERSION 2.8)

include_directories(/usr/local/libevent/include)
link_directories(/usr/local/libevent/lib)

aux_source_directory(. SRC_LIST)
set(DEPENDS event)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${DEPENDS})

