cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

project("Project64-rsp-core")

add_library(Project64-rsp-core STATIC
    cpu/RspClamp.cpp
    cpu/RSPCpu.cpp
    cpu/RSPiInstruction.cpp
    cpu/RSPInterpreterCPU.cpp
    cpu/RSPInterpreterOps.cpp
    cpu/RspLog.cpp
    cpu/RspMemory.cpp
    cpu/RSPRegister.cpp
    cpu/RSPRegisterHandler.cpp
    cpu/RSPRegisterHandlerPlugin.cpp
    cpu/RspTypes.cpp
    Hle/alist.cpp
    Hle/alist_audio.cpp
    Hle/alist_naudio.cpp
    Hle/alist_nead.cpp
    Hle/audio.cpp
    Hle/cicx105.cpp
    Hle/hle.cpp
    Hle/jpeg.cpp
    Hle/mem.cpp
    Hle/mp3.cpp
    Hle/musyx.cpp
    Recompiler/Mmx.cpp
    Recompiler/RspProfiling.cpp
    Recompiler/RspRecompilerAnalysis.cpp
    Recompiler/RspRecompilerCPU.cpp
    Recompiler/RspRecompilerOps.cpp
    Recompiler/RspRecompilerSections.cpp
    Recompiler/Sse.cpp
    Recompiler/X86.cpp
    Settings/RspSettings.cpp
    RSPDebugger.cpp
    RSPInfo.cpp)

add_definitions(-DANDROID)

target_link_libraries(Project64-rsp-core)
