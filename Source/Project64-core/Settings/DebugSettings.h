#pragma once

#include <Project64-core/N64System/N64Types.h>

class CDebugSettings
{
public:
    CDebugSettings();
    virtual ~CDebugSettings();

    static inline bool HaveDebugger(void)
    {
        return m_HaveDebugger;
    }
    static inline bool isDebugging(void)
    {
        return m_Debugging;
    }
    static inline bool isStepping(void)
    {
        return m_Stepping;
    }
    static inline bool SkipOp(void)
    {
        return m_SkipOp;
    }
    static inline bool WaitingForStep(void)
    {
        return m_WaitingForStep;
    }
    static inline bool bRecordRecompilerAsm(void)
    {
        return m_bRecordRecompilerAsm;
    }
    static inline bool bRecordExecutionTimes(void)
    {
        return m_RecordExecutionTimes;
    }
    static inline bool HaveExecutionBP(void)
    {
        return m_HaveExecutionBP;
    }
    static inline bool HaveWriteBP(void)
    {
        return m_HaveWriteBP;
    }
    static inline bool HaveReadBP(void)
    {
        return m_HaveReadBP;
    }
    static inline bool bShowPifRamErrors(void)
    {
        return m_bShowPifRamErrors;
    }
    static inline bool bCPULoggingEnabled(void)
    {
        return m_bCPULoggingEnabled;
    }
    static inline uint32_t ExceptionBreakpoints(void)
    {
        return m_ExceptionBreakpoints;
    }
    static inline uint32_t FpExceptionBreakpoints(void)
    {
        return m_FpExceptionBreakpoints;
    }
    static inline uint32_t IntrBreakpoints(void)
    {
        return m_IntrBreakpoints;
    }
    static inline uint32_t RcpIntrBreakpoints(void)
    {
        return m_RcpIntrBreakpoints;
    }
    static inline bool EndOnPermLoop(void)
    {
        return m_EndOnPermLoop;
    }
    static inline bool FpuExceptionInRecompiler(void)
    {
        return m_FpuExceptionInRecompiler;
    }
    static inline bool BreakOnUnhandledMemory(void)
    {
        return m_BreakOnUnhandledMemory;
    }
    static inline bool BreakOnAddressError(void)
    {
        return m_BreakOnAddressError;
    }
    static inline bool StepOnBreakOpCode(void)
    {
        return m_StepOnBreakOpCode;
    }

private:
    static void StaticRefreshSettings(CDebugSettings * _this)
    {
        _this->RefreshSettings();
    }

    void RefreshSettings(void);

    static bool m_HaveDebugger;
    static bool m_Debugging;
    static bool m_Stepping;
    static bool m_SkipOp;
    static bool m_WaitingForStep;
    static bool m_bRecordRecompilerAsm;
    static bool m_RecordExecutionTimes;
    static bool m_HaveExecutionBP;
    static bool m_HaveWriteBP;
    static bool m_HaveReadBP;
    static bool m_bShowPifRamErrors;
    static bool m_bCPULoggingEnabled;
    static uint32_t m_ExceptionBreakpoints;
    static uint32_t m_FpExceptionBreakpoints;
    static uint32_t m_IntrBreakpoints;
    static uint32_t m_RcpIntrBreakpoints;
    static bool m_EndOnPermLoop;
    static bool m_FpuExceptionInRecompiler;
    static bool m_BreakOnUnhandledMemory;
    static bool m_BreakOnAddressError;
    static bool m_StepOnBreakOpCode;

    static int32_t m_RefCount;
    static bool m_Registered;
};
