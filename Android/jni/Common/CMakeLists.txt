cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Common")

add_library(Common STATIC
        CriticalSection.cpp
        DateTime.cpp
        DynamicLibrary.cpp
        File.cpp
        HighResTimeStamp.cpp
        IniFile.cpp
        Log.cpp
        md5.cpp
        MemoryManagement.cpp
        path.cpp
        Platform.cpp
        Random.cpp
        StdString.cpp
        SyncEvent.cpp
        Thread.cpp
        Trace.cpp
        Util.cpp)

add_definitions(-DANDROID)

target_link_libraries(Common)
