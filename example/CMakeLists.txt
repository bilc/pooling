cmake_minimum_required(VERSION 3.5)
project(demo)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
execute_process(COMMAND thrift --gen cpp demo.thrift)
execute_process(COMMAND rm gen-cpp/Demo_server.skeleton.cpp)
aux_source_directory(gen-cpp DIR_SRCS)
link_directories(../)

add_executable(client client.cc ${DIR_SRCS})
target_link_libraries(client curl thrift thrift-call)

add_executable(client_common client_common.cc ${DIR_SRCS})
target_link_libraries(client_common thrift)


add_executable(server Demo_server.skeleton.cpp ${DIR_SRCS})
target_link_libraries(server thrift)
