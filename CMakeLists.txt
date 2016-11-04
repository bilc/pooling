cmake_minimum_required (VERSION 3.0)
project(thrift-call)
aux_source_directory(. DIR_LIB_SRCS)
add_library(thrift-call ${DIR_LIB_SRCS})
