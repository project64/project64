#include "stdafx.h"

#include "DebuggerUI.h"

#include "CPULog.h"
#include "DMALog.h"
#include "Symbols.h"
#include <sstream>

CPj64Module _Module;

CDebuggerUI::CDebuggerUI() :
    m_MemoryDump(nullptr),
    m_MemoryView(nullptr),
    m_MemorySearch(nullptr),
    m_DebugTLB(nullptr),
    m_CommandsView(nullptr),
    m_Scripts(nullptr),
    m_Symbols(nullptr),
    m_Breakpoints(nullptr),
    m_ScriptSystem(nullptr),
    m_StackTrace(nullptr),
    m_StackView(nullptr),
    m_DMALogView(nullptr),
    m_CPULogView(nullptr),
    m_ExcBreakpoints(nullptr),
    m_DMALog(nullptr),
    m_CPULog(nullptr),
    m_SymbolTable(nullptr),
    m_StepEvent(false)
{
    g_Debugger = this;

    m_Breakpoints = new CBreakpoints();
    m_ScriptSystem = new CScriptSystem(this);

    m_DMALog = new CDMALog();
    m_CPULog = new CCPULog();
    m_SymbolTable = new CSymbolTable(this);

    g_Settings->RegisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
    g_Settings->RegisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)SteppingOpsChanged);
    g_Settings->RegisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunningChanged);
    g_Settings->RegisterChangeCB(Game_GameName, this, (CSettings::SettingChangedFunc)GameNameChanged);
    g_Settings->RegisterChangeCB(GameRunning_CPU_Paused, this, (CSettings::SettingChangedFunc)GamePausedChanged);
    g_Settings->RegisterChangeCB(Debugger_WaitingForStep, this, (CSettings::SettingChangedFunc)WaitingForStepChanged);
}

CDebuggerUI::~CDebuggerUI(void)
{
    g_Settings->UnregisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)SteppingOpsChanged);
    g_Settings->UnregisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
    g_Settings->RegisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunningChanged);
    g_Settings->UnregisterChangeCB(Game_GameName, this, (CSettings::SettingChangedFunc)GameNameChanged);
    g_Settings->UnregisterChangeCB(GameRunning_CPU_Paused, this, (CSettings::SettingChangedFunc)GamePausedChanged);
    g_Settings->UnregisterChangeCB(Debugger_WaitingForStep, this, (CSettings::SettingChangedFunc)WaitingForStepChanged);
    Debug_Reset();
    delete m_MemoryView;
    delete m_CommandsView;
    delete m_Scripts;
    delete m_ScriptSystem;
    delete m_Breakpoints;
    delete m_Symbols;
    delete m_MemorySearch;
    delete m_StackTrace;
    delete m_DMALogView;
    delete m_CPULogView;
    delete m_ExcBreakpoints;
    delete m_DMALog;
    delete m_CPULog;
    delete m_SymbolTable;
}

void CDebuggerUI::SteppingOpsChanged(CDebuggerUI * _this)
{
    if (g_Settings->LoadBool(Debugger_SteppingOps))
    {
        if (!g_Settings->LoadBool(Debugger_SilentBreak))
        {
            _this->OpenCommandWindow();
        }
        g_Settings->SaveBool(Debugger_SilentBreak, false);
    }
}

void CDebuggerUI::GameCpuRunningChanged(CDebuggerUI * _this)
{
    if (!g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        if (_this->m_MemorySearch)
        {
            _this->m_MemorySearch->GameReset();
        }
    }
}

void CDebuggerUI::GameNameChanged(CDebuggerUI * _this)
{
    if (_this->m_MemorySearch)
    {
        _this->m_MemorySearch->GameReset();
    }

    JSHookEmuStateChangeEnv env;
    env.state = JS_EMU_LOADED_ROM;
    _this->m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
}

void CDebuggerUI::GamePausedChanged(CDebuggerUI * _this)
{
    JSHookEmuStateChangeEnv env;
    env.state = g_Settings->LoadBool(GameRunning_CPU_Paused) ? JS_EMU_PAUSED : JS_EMU_RESUMED;
    _this->m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
}

void CDebuggerUI::WaitingForStepChanged(CDebuggerUI * _this)
{
    if (g_Settings->LoadBool(Debugger_WaitingForStep))
    {
        JSHookEmuStateChangeEnv env;
        env.state = JS_EMU_DEBUG_PAUSED;
        _this->ScriptSystem()->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
    }
    else
    {
        JSHookEmuStateChangeEnv env;
        env.state = JS_EMU_DEBUG_RESUMED;
        _this->ScriptSystem()->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
    }
}

void CDebuggerUI::GameReset(CDebuggerUI * _this)
{
    if (!g_Settings->LoadBool(GameRunning_InReset))
    {
        JSHookEmuStateChangeEnv env;
        env.state = JS_EMU_RESET;
        _this->m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
        return;
    }

    JSHookEmuStateChangeEnv env;
    env.state = JS_EMU_RESETTING;
    _this->m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);

    if (_this->m_CommandsView)
    {
        _this->m_CommandsView->Reset();
    }

    if (_this->m_DMALogView)
    {
        _this->m_DMALogView->RefreshDMALogWindow(true);
    }

    if (_this->m_StackTrace)
    {
        _this->m_StackTrace->ClearEntries();
    }

    if (_this->m_SymbolTable)
    {
        _this->m_SymbolTable->Load();
    }

    if (_this->m_Symbols)
    {
        _this->m_Symbols->Refresh();
    }
}

void CDebuggerUI::Debug_Reset(void)
{
    if (m_MemoryDump)
    {
        m_MemoryDump->HideWindow();
        delete m_MemoryDump;
        m_MemoryDump = nullptr;
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->HideWindow();
        delete m_MemorySearch;
        m_MemorySearch = nullptr;
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->HideWindow();
        delete m_DebugTLB;
        m_DebugTLB = nullptr;
    }
    if (m_MemoryView)
    {
        m_MemoryView->HideWindow();
        delete m_MemoryView;
        m_MemoryView = nullptr;
    }
    if (m_CommandsView)
    {
        m_CommandsView->HideWindow();
        delete m_CommandsView;
        m_CommandsView = nullptr;
    }
    if (m_Scripts)
    {
        m_Scripts->HideWindow();
        delete m_Scripts;
        m_Scripts = nullptr;
    }
    if (m_Symbols)
    {
        m_Symbols->HideWindow();
        delete m_Symbols;
        m_Symbols = nullptr;
    }
    if (m_DMALogView)
    {
        m_DMALogView->HideWindow();
        delete m_DMALogView;
        m_DMALogView = nullptr;
    }
    if (m_CPULogView)
    {
        m_CPULogView->HideWindow();
        delete m_CPULogView;
        m_CPULogView = nullptr;
    }
    if (m_StackTrace)
    {
        m_StackTrace->HideWindow();
        delete m_StackTrace;
        m_StackTrace = nullptr;
    }
    if (m_StackView)
    {
        m_StackView->HideWindow();
        delete m_StackView;
        m_StackView = nullptr;
    }
    if (m_ExcBreakpoints)
    {
        m_ExcBreakpoints->HideWindow();
        delete m_ExcBreakpoints;
        m_ExcBreakpoints = nullptr;
    }
}

void CDebuggerUI::OpenMemoryDump()
{
    if (g_MMU == nullptr)
    {
        return;
    }
    if (m_MemoryDump == nullptr)
    {
        m_MemoryDump = new CDumpMemory(this);
    }
    if (m_MemoryDump)
    {
        m_MemoryDump->ShowWindow();
    }
}

void CDebuggerUI::OpenMemoryWindow(void)
{
    if (m_MemoryView == nullptr)
    {
        m_MemoryView = new CDebugMemoryView(this);
    }
    if (m_MemoryView)
    {
        m_MemoryView->ShowWindow();
    }
}

void CDebuggerUI::Debug_ShowMemoryLocation(uint32_t Address, bool VAddr)
{
    OpenMemoryWindow();
    if (m_MemoryView)
    {
        m_MemoryView->ShowAddress(Address, VAddr);
    }
}

void CDebuggerUI::OpenTLBWindow(void)
{
    if (g_MMU == nullptr)
    {
        return;
    }
    if (m_DebugTLB == nullptr)
    {
        m_DebugTLB = new CDebugTlb(this);
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->ShowWindow();
    }
}

void CDebuggerUI::Debug_RefreshTLBWindow(void)
{
    if (m_DebugTLB)
    {
        m_DebugTLB->RefreshTLBWindow();
    }
}

void CDebuggerUI::Debug_RefreshDMALogWindow(void)
{
    if (m_DMALogView)
    {
        m_DMALogView->RefreshDMALogWindow();
    }
}

void CDebuggerUI::OpenMemorySearch()
{
    if (m_MemorySearch == nullptr)
    {
        m_MemorySearch = new CDebugMemorySearch(this);
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->ShowWindow();
    }
}

void CDebuggerUI::OpenCommandWindow()
{
    if (m_CommandsView == nullptr)
    {
        m_CommandsView = new CDebugCommandsView(this, m_StepEvent);
    }
    m_CommandsView->ShowWindow();
}

void CDebuggerUI::Debug_ShowCommandsLocation(uint32_t address, bool top)
{
    OpenCommandWindow();
    if (m_CommandsView)
    {
        m_CommandsView->ShowAddress(address, top);
    }
}

void CDebuggerUI::OpenScriptsWindow()
{
    if (m_Scripts == nullptr)
    {
        m_Scripts = new CDebugScripts(this);
    }
    m_Scripts->ShowWindow();
}

void CDebuggerUI::Debug_RefreshScriptsWindow()
{
    if (m_Scripts != nullptr)
    {
        m_Scripts->RefreshList();
    }
}

void CDebuggerUI::Debug_LogScriptsWindow(const char * text)
{
    if (m_Scripts != nullptr)
    {
        m_Scripts->ConsolePrint(text);
    }
}

void CDebuggerUI::Debug_ClearScriptsWindow()
{
    if (m_Scripts != nullptr)
    {
        m_Scripts->ConsoleClear();
    }
}

void CDebuggerUI::OpenSymbolsWindow()
{
    if (m_Symbols == nullptr)
    {
        m_Symbols = new CDebugSymbols(this);
    }
    m_Symbols->ShowWindow();
}

void CDebuggerUI::Debug_RefreshSymbolsWindow()
{
    if (m_Symbols != nullptr)
    {
        m_Symbols->Refresh();
    }
}

void CDebuggerUI::OpenDMALogWindow(void)
{
    if (m_DMALogView == nullptr)
    {
        m_DMALogView = new CDebugDMALogView(this);
    }
    m_DMALogView->ShowWindow();
}

void CDebuggerUI::OpenCPULogWindow(void)
{
    if (m_CPULogView == nullptr)
    {
        m_CPULogView = new CDebugCPULogView(this);
    }
    m_CPULogView->ShowWindow();
}

void CDebuggerUI::OpenExcBreakpointsWindow(void)
{
    if (m_ExcBreakpoints == nullptr)
    {
        m_ExcBreakpoints = new CDebugExcBreakpoints(this);
    }
    m_ExcBreakpoints->ShowWindow();
}

void CDebuggerUI::OpenStackTraceWindow(void)
{
    if (m_StackTrace == nullptr)
    {
        m_StackTrace = new CDebugStackTrace(this);
    }
    m_StackTrace->ShowWindow();
}

void CDebuggerUI::OpenStackViewWindow(void)
{
    if (m_StackView == nullptr)
    {
        m_StackView = new CDebugStackView(this);
    }
    m_StackView->ShowWindow();
}

void CDebuggerUI::Debug_RefreshStackWindow(void)
{
    if (m_StackView != nullptr)
    {
        m_StackView->Refresh();
    }
}

void CDebuggerUI::Debug_RefreshStackTraceWindow(void)
{
    if (m_StackTrace != nullptr && m_StackTrace->m_hWnd != nullptr)
    {
        m_StackTrace->Refresh();
    }
}

void CDebuggerUI::Debug_RefreshCPULogWindow(void)
{
    if (m_CPULogView != nullptr)
    {
        m_CPULogView->RefreshList();
    }
}

CBreakpoints * CDebuggerUI::Breakpoints()
{
    return m_Breakpoints;
}

CScriptSystem * CDebuggerUI::ScriptSystem()
{
    return m_ScriptSystem;
}

CDebugScripts * CDebuggerUI::ScriptConsole()
{
    return m_Scripts;
}

CDMALog * CDebuggerUI::DMALog()
{
    return m_DMALog;
}

CCPULog * CDebuggerUI::CPULog()
{
    return m_CPULog;
}

CSymbolTable * CDebuggerUI::SymbolTable()
{
    return m_SymbolTable;
}

SyncEvent & CDebuggerUI::StepEvent()
{
    return m_StepEvent;
}

// CDebugger implementation

void CDebuggerUI::TLBChanged()
{
    Debug_RefreshTLBWindow();
}

// Exception handling - break on exception vector if exception breakpoint is set
void CDebuggerUI::HandleCPUException(void)
{
    int exc = (g_Reg->CAUSE_REGISTER >> 2) & 0x1F;
    int intr = (g_Reg->CAUSE_REGISTER >> 8) & 0xFF;
    int fpExc = (g_Reg->m_FPCR[31] >> 12) & 0x3F;
    int rcpIntr = g_Reg->MI_INTR_REG & 0x2F;

    if ((ExceptionBreakpoints() & (1 << exc)))
    {
        if (exc == 15) // Floating-point exception
        {
            if (fpExc & FpExceptionBreakpoints())
            {
                goto have_bp;
            }
            return;
        }
        else if (exc == 0) // Interrupt exception
        {
            if (intr & IntrBreakpoints())
            {
                if (intr & 0x04) // RCP interrupt (IP2)
                {
                    if (rcpIntr & RcpIntrBreakpoints())
                    {
                        goto have_bp;
                    }
                    return;
                }
                else // Other interrupts
                {
                    goto have_bp;
                }
            }
            return;
        }
        else // Other exceptions
        {
            goto have_bp;
        }
    }

    return;

have_bp:
    if (bCPULoggingEnabled())
    {
        g_Debugger->OpenCPULogWindow();
    }

    g_Settings->SaveBool(Debugger_SteppingOps, true);
}

void CDebuggerUI::HandleCartToRamDMA(void)
{
    COpInfo opInfo(R4300iOp::m_Opcode);

    uint32_t dmaRomAddr = g_Reg->PI_CART_ADDR_REG & 0x0FFFFFFF;
    uint32_t dmaRamAddr = g_Reg->PI_DRAM_ADDR_REG | 0x80000000;
    uint32_t dmaLen = opInfo.GetStoreValueUnsigned() + 1;

    m_DMALog->AddEntry(dmaRomAddr, dmaRamAddr, dmaLen);
    Debug_RefreshDMALogWindow();

    // Break if write breakpoint exists anywhere in target buffer
    if (m_Breakpoints->WriteBPExistsInChunk(dmaRamAddr, dmaLen))
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
    }
}

// Called from the interpreter core at the beginning of every CPU step
void CDebuggerUI::CPUStepStarted()
{
    if (isStepping() && bCPULoggingEnabled())
    {
        Debug_RefreshCPULogWindow();
    }

    if (m_Breakpoints->NumMemLocks() > 0)
    {
        COpInfo opInfo(R4300iOp::m_Opcode);
        bool bStoreOp = opInfo.IsStoreCommand();

        if (bStoreOp)
        {
            uint32_t storeAddress = bStoreOp ? opInfo.GetLoadStoreAddress() : 0;
            if (m_Breakpoints->MemLockExists(storeAddress, opInfo.NumBytesToStore()))
            {
                // Memory is locked, skip op
                g_Settings->SaveBool(Debugger_SkipOp, true);
                return;
            }
        }
    }

    if (m_ScriptSystem->HaveAppCallbacks())
    {
        JSHookCpuStepEnv hookEnv;
        hookEnv.pc = g_Reg->m_PROGRAM_COUNTER;
        hookEnv.opInfo = COpInfo(R4300iOp::m_Opcode);

        if (m_ScriptSystem->HaveCpuExecCallbacks(hookEnv.pc))
        {
            m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_CPU_EXEC, (void *)&hookEnv);
        }

        if (hookEnv.opInfo.IsLoadCommand() && m_ScriptSystem->HaveCpuReadCallbacks(hookEnv.opInfo.GetLoadStoreAddress()))
        {
            m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_CPU_READ, (void *)&hookEnv);
        }
        else if (hookEnv.opInfo.IsStoreCommand() && m_ScriptSystem->HaveCpuWriteCallbacks(hookEnv.opInfo.GetLoadStoreAddress()))
        {
            m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_CPU_WRITE, (void *)&hookEnv);
        }
    }

    if (CDebugSettings::ExceptionBreakpoints() != 0)
    {
        uint32_t pc = g_Reg->m_PROGRAM_COUNTER;

        if (pc == 0x80000000 || pc == 0x80000080 || pc == 0xA0000100 || pc == 0x80000180)
        {
            if ((g_Reg->STATUS_REGISTER >> 1) & 3) // If EXL/ERL bits are set
            {
                HandleCPUException();
            }
        }
    }

    if (m_Breakpoints->HaveRegBP())
    {
        COpInfo opInfo(R4300iOp::m_Opcode);

        if (m_Breakpoints->HaveAnyGPRWriteBP())
        {
            int nReg = 0;
            opInfo.WritesGPR(&nReg);

            if (nReg != 0 && m_Breakpoints->HaveGPRWriteBP(nReg))
            {
                g_Settings->SaveBool(Debugger_SteppingOps, true);
            }
        }

        if (m_Breakpoints->HaveAnyGPRReadBP())
        {
            int nReg1 = 0, nReg2 = 0;
            opInfo.ReadsGPR(&nReg1, &nReg2);

            if ((nReg1 != 0 && m_Breakpoints->HaveGPRReadBP(nReg1)) || (nReg2 != 0 && m_Breakpoints->HaveGPRReadBP(nReg2)))
            {
                g_Settings->SaveBool(Debugger_SteppingOps, true);
            }
        }

        if (m_Breakpoints->HaveHIWriteBP() && opInfo.WritesHI() || m_Breakpoints->HaveLOWriteBP() && opInfo.WritesLO() || m_Breakpoints->HaveHIReadBP() && opInfo.ReadsHI() || m_Breakpoints->HaveLOReadBP() && opInfo.ReadsLO())
        {
            g_Settings->SaveBool(Debugger_SteppingOps, true);
        }
    }
}

// Called before opcode is executed (not called if SkipOp is set)
void CDebuggerUI::CPUStep()
{
    if (bCPULoggingEnabled())
    {
        m_CPULog->PushState();
    }
}

// Called after opcode has been executed
void CDebuggerUI::CPUStepEnded()
{
    if (m_StackTrace == nullptr)
    {
        return;
    }

    R4300iOpcode Opcode = R4300iOp::m_Opcode;
    uint32_t op = Opcode.op;
    uint32_t funct = Opcode.funct;

    if (op == R4300i_JAL || ((op == R4300i_SPECIAL) && (funct == R4300i_SPECIAL_JALR) && (Opcode.rd == 31))) // JAL or JALR RA, x
    {
        m_StackTrace->PushEntry(g_System->JumpToLocation(), g_Reg->m_PROGRAM_COUNTER);
    }
    else if (funct == R4300i_SPECIAL_JR && Opcode.rs == 31) // JR RA
    {
        m_StackTrace->PopEntry();
    }
    else if (op == R4300i_CP0 && funct == R4300i_COP0_CO_ERET) // TODO: may need more work
    {
        m_StackTrace->ClearEntries();
    }
}

void CDebuggerUI::FrameDrawn()
{
    //RenderWindow* mainWindow = g_Plugins->MainWindow();
    //HWND hMainWnd = (HWND)mainWindow->GetWindowHandle();
    // todo: m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_GFXUPDATE, ...);
}

void CDebuggerUI::PIFReadStarted(void)
{
    m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_PIFREAD);
}

void CDebuggerUI::RSPReceivedTask(void)
{
    JSHookSpTaskEnv env;

    DebugLoad_VAddr(0xA4000FC0, env.taskType);
    DebugLoad_VAddr(0xA4000FC4, env.taskFlags);
    DebugLoad_VAddr(0xA4000FC8, env.ucodeBootAddress);
    DebugLoad_VAddr(0xA4000FCC, env.ucodeBootSize);
    DebugLoad_VAddr(0xA4000FD0, env.ucodeAddress);
    DebugLoad_VAddr(0xA4000FD4, env.ucodeSize);
    DebugLoad_VAddr(0xA4000FD8, env.ucodeDataAddress);
    DebugLoad_VAddr(0xA4000FDC, env.ucodeDataSize);
    DebugLoad_VAddr(0xA4000FE0, env.dramStackAddress);
    DebugLoad_VAddr(0xA4000FE4, env.dramStackSize);
    DebugLoad_VAddr(0xA4000FE8, env.outputBuffAddress);
    DebugLoad_VAddr(0xA4000FEC, env.outputBuffSize);
    DebugLoad_VAddr(0xA4000FF0, env.dataAddress);
    DebugLoad_VAddr(0xA4000FF4, env.dataSize);
    DebugLoad_VAddr(0xA4000FF8, env.yieldDataAddress);
    DebugLoad_VAddr(0xA4000FFC, env.yieldDataSize);

    m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_RSPTASK, &env);
}

void CDebuggerUI::PIDMAReadStarted(void)
{
    JSHookPiDmaEnv env;

    env.direction = 1;
    DebugLoad_VAddr(0xA4600000, env.dramAddress);
    DebugLoad_VAddr(0xA4600004, env.cartAddress);
    DebugLoad_VAddr(0xA4600008, env.length);

    m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_PIDMA, &env);
}

void CDebuggerUI::PIDMAWriteStarted(void)
{
    JSHookPiDmaEnv env;

    env.direction = 0;
    DebugLoad_VAddr(0xA4600000, env.dramAddress);
    DebugLoad_VAddr(0xA4600004, env.cartAddress);
    DebugLoad_VAddr(0xA460000C, env.length);

    m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_PIDMA, &env);

    HandleCartToRamDMA();
}

void CDebuggerUI::EmulationStarted(void)
{
    JSHookEmuStateChangeEnv env;
    env.state = JS_EMU_STARTED;
    m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
}

void CDebuggerUI::EmulationStopped(void)
{
    JSHookEmuStateChangeEnv env;
    env.state = JS_EMU_STOPPED;
    m_ScriptSystem->InvokeAppCallbacks(JS_HOOK_EMUSTATECHANGE, &env);
}

void CDebuggerUI::WaitForStep(void)
{
    g_Settings->SaveBool(Debugger_WaitingForStep, true);
    m_StepEvent.IsTriggered(SyncEvent::INFINITE_TIMEOUT);
    g_Settings->SaveBool(Debugger_WaitingForStep, false);
}

void CDebuggerUI::StartAutorunScripts(void)
{
    if (m_ScriptSystem == nullptr)
    {
        return;
    }

    m_ScriptSystem->ExecAutorunList();
}

bool CDebuggerUI::ExecutionBP(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->ExecutionBPExists(address, true) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP8(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->ReadBPExists8(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP16(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->ReadBPExists16(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP32(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->ReadBPExists32(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP64(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->ReadBPExists64(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP8(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->WriteBPExists8(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP16(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->WriteBPExists16(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP32(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->WriteBPExists32(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP64(uint32_t address)
{
    return m_Breakpoints != nullptr && m_Breakpoints->WriteBPExists64(address) != CBreakpoints::BP_NOT_SET;
}
