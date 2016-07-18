PROJECT(MYORC)
cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_COMPILER g++)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O3")
set (LLVM_CXX_FLAGS "-D_DEBUG -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -Wno-write-strings -Wcast-qual -fomit-frame-pointer -fvisibility-inlines-hidden -fPIC -ffunction-sections -fdata-sections ")

include_directories(include)
ADD_SUBDIRECTORY(src)