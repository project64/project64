#include "RspSettings.h"
#include "RspSettingsID.h"
#include <Project64-rsp-core/Recompiler/RspRecompilerCPU.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Settings/Settings.h>

uint16_t Set_AudioHle = 0, Set_GraphicsHle = 0;
bool GraphicsHle = true, AudioHle, ConditionalMove;
bool DebuggingEnabled = false, Profiling, IndvidualBlock, ShowErrors, BreakOnStart = false, LogRDP = false, LogX86Code = false;

void InitializeRspSetting(void)
{
    SetModuleName("RSP");
    Set_GraphicsHle = FindSystemSettingId("HLE GFX");
    Set_AudioHle = FindSystemSettingId("HLE Audio");

    RegisterSetting(Set_BreakOnStart, Data_DWORD_General, "Break on Start", NULL, BreakOnStart, NULL);
    RegisterSetting(Set_CPUCore, Data_DWORD_General, "CPU Method", NULL, g_CPUCore, NULL);
    RegisterSetting(Set_LogRDP, Data_DWORD_General, "Log RDP", NULL, LogRDP, NULL);
    RegisterSetting(Set_LogX86Code, Data_DWORD_General, "Log X86 Code", NULL, LogX86Code, NULL);
    RegisterSetting(Set_Profiling, Data_DWORD_General, "Profiling", NULL, Profiling, NULL);
    RegisterSetting(Set_IndvidualBlock, Data_DWORD_General, "Individual Block", NULL, IndvidualBlock, NULL);
    RegisterSetting(Set_ShowErrors, Data_DWORD_General, "Show Errors", NULL, ShowErrors, NULL);

    // Compiler settings
    RegisterSetting(Set_CheckDest, Data_DWORD_General, "Check Destination Vector", NULL, Compiler.bDest, NULL);
    RegisterSetting(Set_Accum, Data_DWORD_General, "Check Destination Accumulator", NULL, Compiler.bAccum, NULL);
    RegisterSetting(Set_Mmx, Data_DWORD_General, "Use MMX", NULL, Compiler.mmx, NULL);
    RegisterSetting(Set_Mmx2, Data_DWORD_General, "Use MMX2", NULL, Compiler.mmx2, NULL);
    RegisterSetting(Set_Sse, Data_DWORD_General, "Use SSE", NULL, Compiler.sse, NULL);
    RegisterSetting(Set_Sections, Data_DWORD_General, "Use precompiled sections", NULL, Compiler.bSections, NULL);
    RegisterSetting(Set_ReOrdering, Data_DWORD_General, "Reorder opcodes", NULL, Compiler.bReOrdering, NULL);
    RegisterSetting(Set_GPRConstants, Data_DWORD_General, "Detect GPR Constants", NULL, Compiler.bGPRConstants, NULL);
    RegisterSetting(Set_Flags, Data_DWORD_General, "Check Flag Usage", NULL, Compiler.bFlags, NULL);
    RegisterSetting(Set_AlignVector, Data_DWORD_General, "Assume Vector loads align", NULL, Compiler.bAlignVector, NULL);

    RegisterSetting(Set_JumpTableSize, Data_DWORD_Game, "JumpTableSize", NULL, 0x800, NULL);
    RegisterSetting(Set_Mfc0Count, Data_DWORD_Game, "Mfc0Count", NULL, 0x0, NULL);
    RegisterSetting(Set_SemaphoreExit, Data_DWORD_Game, "SemaphoreExit", NULL, 0x0, NULL);

    AudioHle = Set_AudioHle != 0 ? GetSystemSetting(Set_AudioHle) != 0 : false;
    GraphicsHle = Set_GraphicsHle != 0 ? GetSystemSetting(Set_GraphicsHle) != 0 : true;
}
