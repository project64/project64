cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-video")
include_directories(..)
include_directories(../3rdParty)
include_directories(./Renderer/inc)

add_library(Project64-video SHARED
    3dmath.cpp
    Android.cpp
    Combine.cpp
    Config.cpp
    CRC.cpp
    Debugger.cpp
    DepthBufferRender.cpp
    F3DTEXA.cpp
    FBtoScreen.cpp
    Main.cpp
    rdp.cpp
    ScreenResolution.cpp
    Settings.cpp
    TexBuffer.cpp
    TexCache.cpp
    trace.cpp
    turbo3D.cpp
    ucode.cpp
    ucode00.cpp
    ucode01.cpp
    ucode02.cpp
    ucode03.cpp
    ucode04.cpp
    ucode05.cpp
    ucode06.cpp
    ucode07.cpp
    ucode08.cpp
    ucode09.cpp
    ucode09rdp.cpp
    ucodeFB.cpp
    Util.cpp
    Ext_TxFilter.cpp
    Renderer/OGLEScombiner.cpp
    Renderer/OGLESgeometry.cpp
    Renderer/OGLESglitchmain.cpp
    Renderer/OGLEStextures.cpp
    Renderer/OGLESwrappers.cpp
    Renderer/Renderer.cpp
    TextureEnhancer/TxFilterExport.cpp
    TextureEnhancer/TxFilter.cpp
    TextureEnhancer/TxCache.cpp
    TextureEnhancer/TxTexCache.cpp
    TextureEnhancer/TxHiResCache.cpp
    TextureEnhancer/TxQuantize.cpp
    TextureEnhancer/TxUtil.cpp
    TextureEnhancer/TextureFilters.cpp
    TextureEnhancer/TextureFilters_2xsai.cpp
    TextureEnhancer/TextureFilters_hq2x.cpp
    TextureEnhancer/TextureFilters_hq4x.cpp
    TextureEnhancer/TxImage.cpp
    TextureEnhancer/TxReSample.cpp
    TextureEnhancer/TxDbg.cpp
    TextureEnhancer/tc-1.1+/fxt1.c
    TextureEnhancer/tc-1.1+/dxtn.c
    TextureEnhancer/tc-1.1+/wrapper.c
    TextureEnhancer/tc-1.1+/texstore.c)

add_definitions(-DANDROID)
add_definitions(-DNOSSE)
add_definitions(-DUSE_GLES)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/png ${CMAKE_CURRENT_BINARY_DIR}/3rdParty/png)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/zlib ${CMAKE_CURRENT_BINARY_DIR}/3rdParty/zlib)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Common ${CMAKE_CURRENT_BINARY_DIR}/Common)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../Settings ${CMAKE_CURRENT_BINARY_DIR}/Settings)
target_link_libraries(Project64-video png zlib Common settings GLESv2 log)
