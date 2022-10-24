cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("settings")

add_library(settings STATIC
    Settings.cpp)

add_definitions(-DANDROID)

target_link_libraries(settings)
