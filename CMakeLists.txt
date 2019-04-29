cmake_minimum_required(VERSION 2.8)

string(REPLACE ${OpenMP_C_FLAGS} "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string(REPLACE ${OpenMP_CXX_FLAGS} "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

project(RemoteCaptury)
message(STATUS "working on remotecaptury library")
include_directories(../include)
add_library(RemoteCaptury SHARED RemoteCaptury.cpp)
add_library(RemoteCapturyStatic STATIC RemoteCaptury.cpp)
