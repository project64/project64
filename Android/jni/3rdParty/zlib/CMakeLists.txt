cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("zlib")

add_library(zlib STATIC
        adler32.c
        compress.c
        crc32.c
        deflate.c
        gzclose.c
        gzlib.c
        gzread.c
        gzwrite.c
        infback.c
        inffast.c
        inflate.c
        inftrees.c
        trees.c
        uncompr.c
        zutil.c
        contrib/minizip/ioapi.c
        contrib/minizip/miniunz.c
        contrib/minizip/minizip.c
        contrib/minizip/mztools.c
        contrib/minizip/unzip.c
        contrib/minizip/zip.c)

add_definitions(-DANDROID)
add_definitions(-DUSE_FILE32API)
add_definitions(-DZ_BUFSIZE=46516)

target_link_libraries(zlib)
