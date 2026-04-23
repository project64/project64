#include "stdafx.h"

#include <Project64-core/Settings/DebugSettings.h>

DebugSettings g_DebugSettings = {};

namespace
{
bool s_Registered = false;

static void DebugSettingsChanged(void * /*Data*/)
{
    if (g_Settings == nullptr)
    {
        return;
    }

    g_DebugSettings.haveDebugger = g_Settings->LoadBool(Debugger_Enabled);
    g_DebugSettings.recordRecompilerAsm = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_RecordRecompilerAsm);
    g_DebugSettings.recordExecutionTimes = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_RecordExecutionTimes);
    g_DebugSettings.stepping = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_SteppingOps);
    g_DebugSettings.skipOp = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_SkipOp);
    g_DebugSettings.waitingForStep = g_Settings->LoadBool(Debugger_WaitingForStep);
    g_DebugSettings.haveExecutionBP = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_HaveExecutionBP);
    g_DebugSettings.haveWriteBP = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_WriteBPExists);
    g_DebugSettings.haveReadBP = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_ReadBPExists);
    g_DebugSettings.showPifRamErrors = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_ShowPifErrors);
    g_DebugSettings.cpuLoggingEnabled = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_CPULoggingEnabled);
    g_DebugSettings.exceptionBreakpoints = g_DebugSettings.haveDebugger ? g_Settings->LoadDword(Debugger_ExceptionBreakpoints) : 0;
    g_DebugSettings.fpExceptionBreakpoints = g_DebugSettings.haveDebugger ? g_Settings->LoadDword(Debugger_FpExceptionBreakpoints) : 0;
    g_DebugSettings.intrBreakpoints = g_DebugSettings.haveDebugger ? g_Settings->LoadDword(Debugger_IntrBreakpoints) : 0;
    g_DebugSettings.rcpIntrBreakpoints = g_DebugSettings.haveDebugger ? g_Settings->LoadDword(Debugger_RcpIntrBreakpoints) : 0;
    g_DebugSettings.endOnPermLoop = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_EndOnPermLoop);
    g_DebugSettings.fpuExceptionInRecompiler = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_FpuExceptionInRecompiler);
    g_DebugSettings.breakOnUnhandledMemory = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_BreakOnUnhandledMemory);
    g_DebugSettings.breakOnAddressError = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_BreakOnAddressError);
    g_DebugSettings.stepOnBreakOpCode = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_StepOnBreakOpCode);
    g_DebugSettings.trackCPUStepStarted = g_DebugSettings.stepping || g_DebugSettings.exceptionBreakpoints != 0 ||
                                          (g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_TrackCPUStepStarted));
    g_DebugSettings.trackCPUStepEnded = g_DebugSettings.haveDebugger && g_Settings->LoadBool(Debugger_TrackCPUStepEnded);

    g_DebugSettings.debugging = g_DebugSettings.haveDebugger &&
                                (g_DebugSettings.haveExecutionBP || g_DebugSettings.waitingForStep ||
                                 g_DebugSettings.haveWriteBP || g_DebugSettings.haveReadBP);
}
} // namespace

void SetupDebugSettings(void)
{
    if (g_Settings == nullptr || s_Registered)
    {
        return;
    }

    g_Settings->RegisterChangeCB(Debugger_Enabled, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_RecordRecompilerAsm, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_RecordExecutionTimes, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_SteppingOps, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_SkipOp, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_HaveExecutionBP, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_WriteBPExists, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_ReadBPExists, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_WaitingForStep, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_ShowPifErrors, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_CPULoggingEnabled, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_ExceptionBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_FpExceptionBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_IntrBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_RcpIntrBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_EndOnPermLoop, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_FpuExceptionInRecompiler, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_BreakOnUnhandledMemory, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_BreakOnAddressError, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_StepOnBreakOpCode, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_TrackCPUStepStarted, nullptr, DebugSettingsChanged);
    g_Settings->RegisterChangeCB(Debugger_TrackCPUStepEnded, nullptr, DebugSettingsChanged);

    DebugSettingsChanged(nullptr);
    s_Registered = true;
}

void ShutdownDebugSettings(void)
{
    if (g_Settings == nullptr || !s_Registered)
    {
        return;
    }

    g_Settings->UnregisterChangeCB(Debugger_Enabled, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_RecordRecompilerAsm, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_RecordExecutionTimes, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_SteppingOps, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_SkipOp, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_HaveExecutionBP, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_WriteBPExists, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_ReadBPExists, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_WaitingForStep, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_ShowPifErrors, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_CPULoggingEnabled, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_ExceptionBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_FpExceptionBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_IntrBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_RcpIntrBreakpoints, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_EndOnPermLoop, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_FpuExceptionInRecompiler, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_BreakOnUnhandledMemory, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_BreakOnAddressError, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_StepOnBreakOpCode, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_TrackCPUStepStarted, nullptr, DebugSettingsChanged);
    g_Settings->UnregisterChangeCB(Debugger_TrackCPUStepEnded, nullptr, DebugSettingsChanged);

    s_Registered = false;
}
