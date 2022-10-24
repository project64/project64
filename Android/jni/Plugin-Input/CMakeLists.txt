cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-input-android")
include_directories(..)

add_library(Project64-input-android SHARED
    Main.cpp)

add_definitions(-DANDROID)

target_link_libraries(Project64-input-android)
