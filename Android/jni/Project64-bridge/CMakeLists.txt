cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-bridge")
include_directories(..)

add_library(Project64-bridge SHARED
        JavaBridge.cpp
        JavaRomList.cpp
        jniBridge.cpp
        jniBridgeSettings.cpp
        Notification.cpp
        SyncBridge.cpp
        UISettings.cpp)

add_definitions(-DANDROID)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/zlib ${CMAKE_CURRENT_BINARY_DIR}/3rdParty/zlib)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Common ${CMAKE_CURRENT_BINARY_DIR}/Common)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Project64-core ${CMAKE_CURRENT_BINARY_DIR}/Project64-core)
target_link_libraries(Project64-bridge zlib Project64-core Common log)
