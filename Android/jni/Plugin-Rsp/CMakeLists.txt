cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-rsp")
include_directories(..)

add_library(Project64-rsp SHARED
    main.cpp)

add_definitions(-DANDROID)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Settings ${CMAKE_CURRENT_BINARY_DIR}/Settings)
target_link_libraries(Project64-rsp settings)
