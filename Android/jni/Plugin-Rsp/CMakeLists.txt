cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Plugin-rsp")
include_directories(..)

add_library(Plugin-rsp SHARED
    main.cpp)

add_definitions(-DANDROID)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Common ${CMAKE_CURRENT_BINARY_DIR}/Common)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Project64-rsp-core ${CMAKE_CURRENT_BINARY_DIR}/Project64-rsp-core)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Settings ${CMAKE_CURRENT_BINARY_DIR}/Settings)
target_link_libraries(Plugin-rsp Project64-rsp-core settings Common)
