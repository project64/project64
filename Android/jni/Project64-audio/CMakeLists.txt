cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-audio-android")
include_directories(..)

add_library(Project64-audio-android SHARED
    Driver/OpenSLES.cpp
    Driver/SoundBase.cpp
    AudioMain.cpp
    AudioSettings.cpp
    trace.cpp)

add_definitions(-DANDROID)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Common ${CMAKE_CURRENT_BINARY_DIR}/Common)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Settings ${CMAKE_CURRENT_BINARY_DIR}/Settings)
target_link_libraries(Project64-audio-android Common settings log OpenSLES)
