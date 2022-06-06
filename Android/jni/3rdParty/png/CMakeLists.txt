cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("png")

add_library(png STATIC
    png.c
    pngerror.c
    pngget.c
    pngmem.c
    pngpread.c
    pngread.c
    pngrio.c
    pngrtran.c
    pngrutil.c
    pngset.c
    pngtest.c
    pngtrans.c
    pngwio.c
    pngwrite.c
    pngwtran.c
    pngwutil.c)

add_definitions(-DANDROID)

target_link_libraries(png)
