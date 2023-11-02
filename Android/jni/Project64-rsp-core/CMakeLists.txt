cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64.rsp.core")

add_library(Project64.rsp.core STATIC
    Hle/alist.cpp)

add_definitions(-DANDROID)

target_link_libraries(Project64.rsp.core)
