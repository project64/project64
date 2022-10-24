cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-rsp-hle")

add_library(Project64-rsp-hle SHARED
    alist.cpp
    alist_audio.cpp
    alist_naudio.cpp
    alist_nead.cpp
    audio.cpp
    cicx105.cpp
    hle.cpp
    jpeg.cpp
    main.cpp
    mem.cpp
    mp3.cpp
    musyx.cpp)

add_definitions(-DANDROID)

target_link_libraries(Project64-rsp-hle)
