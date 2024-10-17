#include "RSPInfo.h"
#include <Project64-rsp-core/Recompiler/RspProfiling.h>
#include <Project64-rsp-core/Recompiler/RspRecompilerCPU.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/Settings/RspSettingsID.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>

// https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex
#include <intrin.h>

#if defined(_MSC_VER)
#include <Windows.h>
#endif

RSP_INFO RSPInfo;
uint32_t RdramSize = 0;

void ClearAllx86Code(void);

void DetectCpuSpecs(void)
{
    uint32_t Intel_Features = 0;
    uint32_t AMD_Features = 0;

#if defined(_MSC_VER)
    __try
    {
#ifdef _M_IX86
        _asm {
            // Intel features
            mov eax, 1
            cpuid
            mov[Intel_Features], edx

                // AMD features
            mov eax, 80000001h
            cpuid
            or [AMD_Features], edx
        }
#else
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        Intel_Features = cpuInfo[3];
        __cpuid(cpuInfo, 0x80000001);
        AMD_Features = cpuInfo[3];
#endif
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        AMD_Features = Intel_Features = 0;
    }
#else

    /*
    TODO: With GCC, there is <cpuid.h>, but __cpuid() there is a macro and
    needs five arguments, not two.  Also, GCC lacks SEH.
    */

    AMD_Features = Intel_Features = 0;
#endif

    if (Intel_Features & 0x02000000)
    {
        Compiler.mmx2 = true;
        Compiler.sse = true;
    }
    if (Intel_Features & 0x00800000)
    {
        Compiler.mmx = true;
    }
    if (AMD_Features & 0x40000000)
    {
        Compiler.mmx2 = true;
    }
    if (Intel_Features & 0x00008000)
    {
        ConditionalMove = true;
    }
    else
    {
        ConditionalMove = false;
    }
}

void RspPluginLoaded(void)
{
    BreakOnStart = false;
    LogRDP = false;
    LogX86Code = false;
    Profiling = false;
    IndvidualBlock = false;
    ShowErrors = false;

    memset(&Compiler, 0, sizeof(Compiler));

    Compiler.bDest = true;
    Compiler.bAlignVector = false;
    Compiler.bFlags = true;
    Compiler.bReOrdering = false;
    Compiler.bSections = false;
    Compiler.bAccum = true;
    Compiler.bGPRConstants = true;
    DetectCpuSpecs();

    CRSPSettings::InitializeRspSetting();
}

void InitilizeRSP(RSP_INFO & Rsp_Info)
{
    RSPInfo = Rsp_Info;

    AudioHle = Set_AudioHle != 0 ? GetSystemSetting(Set_AudioHle) != 0 : false;
    GraphicsHle = Set_GraphicsHle != 0 ? GetSystemSetting(Set_GraphicsHle) != 0 : true;

    AllocateMemory();
    RSPSystem.Reset(Rsp_Info);
    Build_RSP();
#ifdef GenerateLog
    Start_Log();
#endif
}

void RspRomOpened(void)
{
    CRSPSettings::SetRomOpen(true);
    ClearAllx86Code();

    JumpTableSize = GetSetting(Set_JumpTableSize);
    Mfc0Count = GetSetting(Set_Mfc0Count);
    SemaphoreExit = GetSetting(Set_SemaphoreExit);
    RdramSize = Set_AllocatedRdramSize != 0 ? GetSystemSetting(Set_AllocatedRdramSize) : 0;
    if (RdramSize == 0)
    {
        RdramSize = 0x00400000;
    }
}

void RspRomClosed(void)
{
    CRSPSettings::SetRomOpen(false);
    if (Profiling)
    {
        StopTimer();
        GenerateTimerResults();
    }
    RSPSystem.RomClosed();
    ClearAllx86Code();
    RDPLog.StopLog();
    StopCPULog();

#ifdef GenerateLog
    Stop_Log();
#endif
}

void FreeRSP(void)
{
    FreeMemory();
}