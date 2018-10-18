cmake_minimum_required(VERSION 2.8)

project(RemoteCaptury)
message(STATUS "working on remotecaptury library")
include_directories(../include)
add_library(RemoteCaptury SHARED RemoteCaptury.cpp)
add_library(RemoteCapturyStatic STATIC RemoteCaptury.cpp)
