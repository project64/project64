#pragma once
#include <Project64-core/Debugger.h>
#include <Common/SyncEvent.h>
#include <Project64-core/Settings/DebugSettings.h>
#include "DebugMMU.h"

class CDumpMemory;
class CDebugMemoryView;
class CDebugMemorySearch;
class CDebugTlb;
class CDebugCommandsView;
class CDebugScripts;
class CDebugSymbols;
class CDebugDMALogView;
class CDebugCPULogView;
class CDebugStackView;
class CDebugStackTrace;
class CDebugExcBreakpoints;

class CCPULog;
class CDMALog;
class CSymbolTable;
class CBreakpoints;
class CScriptSystem;

class CDebuggerUI :
    public CDebugger,
    public CDebugSettings,
    public CDebugMMU
{
public:
    CDebuggerUI();
    ~CDebuggerUI();

public:
    void Debug_Reset(void);
    void OpenMemoryDump(void);
    void OpenMemoryWindow(void);
    void Debug_ShowMemoryLocation(uint32_t Address, bool VAddr);
    void OpenMemorySearch(void);
    void OpenTLBWindow(void);
    void Debug_RefreshTLBWindow(void);
    void OpenCommandWindow(void);
    void Debug_ShowCommandsLocation(uint32_t address, bool top);
    void OpenScriptsWindow(void);
    void Debug_LogScriptsWindow(const char* text);
    void Debug_ClearScriptsWindow(void);
    void Debug_RefreshScriptsWindow(void);
    void Debug_RefreshSymbolsWindow(void);
    void OpenSymbolsWindow(void);
    void OpenStackTraceWindow(void);
    void OpenStackViewWindow(void);
    void Debug_RefreshStackWindow(void);
    void Debug_RefreshStackTraceWindow(void);
    void OpenDMALogWindow(void);
    void Debug_RefreshDMALogWindow(void);
    void OpenCPULogWindow(void);
    void Debug_RefreshCPULogWindow(void);
    void OpenExcBreakpointsWindow(void);

    void StartAutorunScripts();

    bool ExecutionBP(uint32_t address);
    bool ReadBP8(uint32_t address);
    bool ReadBP16(uint32_t address);
    bool ReadBP32(uint32_t address);
    bool ReadBP64(uint32_t address);
    bool WriteBP8(uint32_t address);
    bool WriteBP16(uint32_t address);
    bool WriteBP32(uint32_t address);
    bool WriteBP64(uint32_t address);
    void WaitForStep(void);

    CBreakpoints* Breakpoints();
    CDebugSymbols* Symbols();
    CScriptSystem* ScriptSystem();
    CDebugScripts* ScriptConsole();
    CDMALog* DMALog();
    CCPULog* CPULog();
    CSymbolTable* SymbolTable();
    SyncEvent& StepEvent();

    static void GameReset(CDebuggerUI * _this);
    static void GameCpuRunningChanged(CDebuggerUI * _this);
    static void GameNameChanged(CDebuggerUI * _this);
    static void GamePausedChanged(CDebuggerUI * _this);
    static void SteppingOpsChanged(CDebuggerUI * _this);
    static void WaitingForStepChanged(CDebuggerUI * _this);

protected:
    void TLBChanged(void);
    void CPUStepStarted(void);
    void CPUStep(void);
    void CPUStepEnded(void);
    void FrameDrawn(void);
    void PIFReadStarted(void);
    void RSPReceivedTask(void);
    void PIDMAReadStarted(void);
    void PIDMAWriteStarted(void);
    void EmulationStarted(void);
    void EmulationStopped(void);

private:
    CDebuggerUI(const CDebuggerUI&);
    CDebuggerUI& operator=(const CDebuggerUI&);

    CDumpMemory          * m_MemoryDump;
    CDebugMemoryView     * m_MemoryView;
    CDebugMemorySearch   * m_MemorySearch;
    CDebugTlb            * m_DebugTLB;
    CDebugCommandsView   * m_CommandsView;
    CDebugScripts        * m_Scripts;
    CDebugSymbols        * m_Symbols;
    CDebugDMALogView     * m_DMALogView;
    CDebugCPULogView     * m_CPULogView;
    CDebugStackTrace     * m_StackTrace;
    CDebugStackView      * m_StackView;
    CDebugExcBreakpoints * m_ExcBreakpoints;

    CBreakpoints        * m_Breakpoints;
    CScriptSystem       * m_ScriptSystem;
    CSymbolTable        * m_SymbolTable;
    CDMALog             * m_DMALog;
    CCPULog             * m_CPULog;

    SyncEvent m_StepEvent;

    void HandleCPUException(void);
    void HandleCartToRamDMA(void);
};
