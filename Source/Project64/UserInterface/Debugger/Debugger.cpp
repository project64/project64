/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "DebuggerUI.h"
#include "ScriptHook.h"

#include "DMALog.h"
#include "Symbols.h"

CPj64Module _Module;

CDebuggerUI::CDebuggerUI() :
    m_MemoryDump(NULL),
    m_MemoryView(NULL),
    m_MemorySearch(NULL),
    m_DebugTLB(NULL),
    m_CommandsView(NULL),
    m_Scripts(NULL),
    m_Symbols(NULL),
    m_Breakpoints(NULL),
    m_ScriptSystem(NULL),
    m_StackTrace(NULL),
    m_StackView(NULL),
    m_DMALogView(NULL),
    m_DMALog(NULL),
    m_StepEvent(false)
{
    g_Debugger = this;

    m_Breakpoints = new CBreakpoints();
    m_ScriptSystem = new CScriptSystem(this);

    m_DMALog = new CDMALog();

    CSymbols::InitializeCriticalSection();
    g_Settings->RegisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
    g_Settings->RegisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)SteppingOpsChanged);
}

CDebuggerUI::~CDebuggerUI(void)
{
    g_Settings->UnregisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)SteppingOpsChanged);
    g_Settings->UnregisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
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
    delete m_DMALog;

    CSymbols::DeleteCriticalSection();
}

void CDebuggerUI::SteppingOpsChanged(CDebuggerUI * _this)
{
    if (g_Settings->LoadBool(Debugger_SteppingOps))
    {
        _this->OpenCommandWindow();
    }
}

void CDebuggerUI::GameReset(CDebuggerUI * _this)
{
    if (!g_Settings->LoadBool(GameRunning_InReset))
    {
        return;
    }

    if (_this->m_CommandsView)
    {
        _this->m_CommandsView->Reset();
    }

    if (_this->m_DMALog)
    {
        _this->m_DMALog->ClearEntries();
    }

    if (_this->m_StackTrace)
    {
        _this->m_StackTrace->ClearEntries();
    }

    CSymbols::EnterCriticalSection();
    CSymbols::Load();
    CSymbols::LeaveCriticalSection();

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
        m_MemoryDump = NULL;
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->HideWindow();
        delete m_MemorySearch;
        m_MemorySearch = NULL;
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->HideWindow();
        delete m_DebugTLB;
        m_DebugTLB = NULL;
    }
    if (m_MemoryView)
    {
        m_MemoryView->HideWindow();
        delete m_MemoryView;
        m_MemoryView = NULL;
    }
    if (m_CommandsView)
    {
        m_CommandsView->HideWindow();
        delete m_CommandsView;
        m_CommandsView = NULL;
    }
    if (m_Scripts)
    {
        m_Scripts->HideWindow();
        delete m_Scripts;
        m_Scripts = NULL;
    }
    if (m_Symbols)
    {
        m_Symbols->HideWindow();
        delete m_Symbols;
        m_Symbols = NULL;
    }
    if (m_DMALogView)
    {
        m_DMALogView->HideWindow();
        delete m_DMALogView;
        m_DMALogView = NULL;
    }
    if (m_StackTrace)
    {
        m_StackTrace->HideWindow();
        delete m_StackTrace;
        m_StackTrace = NULL;
    }
    if (m_StackView)
    {
        m_StackView->HideWindow();
        delete m_StackView;
        m_StackView = NULL;
    }
}

void CDebuggerUI::OpenMemoryDump()
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_MemoryDump == NULL)
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
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_MemoryView == NULL)
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
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_DebugTLB == NULL)
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

void CDebuggerUI::OpenMemorySearch()
{
    if (m_MemorySearch == NULL)
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
    if (m_CommandsView == NULL)
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
    if (m_Scripts == NULL)
    {
        m_Scripts = new CDebugScripts(this);
    }
    m_Scripts->ShowWindow();
}

void CDebuggerUI::Debug_RefreshScriptsWindow()
{
    if (m_Scripts != NULL)
    {
        m_Scripts->RefreshList();
    }
}

void CDebuggerUI::Debug_LogScriptsWindow(const char* text)
{
    if (m_Scripts != NULL)
    {
        m_Scripts->ConsolePrint(text);
    }
}

void CDebuggerUI::Debug_ClearScriptsWindow()
{
    if (m_Scripts != NULL)
    {
        m_Scripts->ConsoleClear();
    }
}

void CDebuggerUI::OpenSymbolsWindow()
{
    if (m_Symbols == NULL)
    {
        m_Symbols = new CDebugSymbols(this);
    }
    m_Symbols->ShowWindow();
}

void CDebuggerUI::Debug_RefreshSymbolsWindow()
{
    if (m_Symbols != NULL)
    {
        m_Symbols->Refresh();
    }
}

void CDebuggerUI::OpenDMALogWindow(void)
{
    if (m_DMALogView == NULL)
    {
        m_DMALogView = new CDebugDMALogView(this);
    }
    m_DMALogView->ShowWindow();
}

void CDebuggerUI::OpenStackTraceWindow(void)
{
    if (m_StackTrace == NULL)
    {
        m_StackTrace = new CDebugStackTrace(this);
    }
    m_StackTrace->ShowWindow();
}

void CDebuggerUI::OpenStackViewWindow(void)
{
    if (m_StackView == NULL)
    {
        m_StackView = new CDebugStackView(this);
    }
    m_StackView->ShowWindow();
}

void CDebuggerUI::Debug_RefreshStackWindow(void)
{
    if (m_StackView != NULL)
    {
        m_StackView->Refresh();
    }
}

void CDebuggerUI::Debug_RefreshStackTraceWindow(void)
{
    if (m_StackTrace != NULL && m_StackTrace->m_hWnd != NULL)
    {
        m_StackTrace->Refresh();
    }
}

CBreakpoints* CDebuggerUI::Breakpoints()
{
    return m_Breakpoints;
}

CScriptSystem* CDebuggerUI::ScriptSystem()
{
    return m_ScriptSystem;
}

CDebugScripts* CDebuggerUI::ScriptConsole()
{
    return m_Scripts;
}

CDMALog* CDebuggerUI::DMALog()
{
    return m_DMALog;
}

// CDebugger implementation

void CDebuggerUI::TLBChanged()
{
    Debug_RefreshTLBWindow();
}

// Called from the interpreter core at the beginning of every CPU step
// Returns false when the instruction should be skipped
void CDebuggerUI::CPUStepStarted()
{
    uint32_t PROGRAM_COUNTER = g_Reg->m_PROGRAM_COUNTER;
    uint32_t JumpToLocation = R4300iOp::m_JumpToLocation;

    m_ScriptSystem->HookCPUExec()->InvokeByParamInRange(PROGRAM_COUNTER);
    m_ScriptSystem->HookCPUExecOpcode()->InvokeByParamInRangeWithMaskedValue(PROGRAM_COUNTER, R4300iOp::m_Opcode.Hex);

    // Memory breakpoints

    COpInfo opInfo(R4300iOp::m_Opcode);

    if (opInfo.IsLoadStoreCommand()) // Read and write instructions
    {
        uint32_t memoryAddress = opInfo.GetLoadStoreAddress();

        if (opInfo.IsLoadCommand()) // Read instructions
        {
            m_ScriptSystem->HookCPURead()->InvokeByParamInRange(memoryAddress);
        }
        else // Write instructions
        {
            m_ScriptSystem->HookCPUWrite()->InvokeByParamInRange(memoryAddress);

            // Catch cart -> rdram dma
            if (memoryAddress == 0xA460000C) // PI_WR_LEN_REG
            {
                uint32_t dmaRomAddr = g_Reg->PI_CART_ADDR_REG & 0x0FFFFFFF;
                uint32_t dmaRamAddr = g_Reg->PI_DRAM_ADDR_REG | 0x80000000;
                uint32_t dmaLen = opInfo.GetStoreValueUnsigned() + 1;
                
                m_DMALog->AddEntry(dmaRomAddr, dmaRamAddr, dmaLen);

                if (m_Breakpoints->WriteBPExistsInChunk(dmaRamAddr, dmaLen))
                {
                    goto breakpoint_hit;
                }
            }

            if (m_Breakpoints->MemLockExists(memoryAddress, opInfo.NumBytesToStore()))
            {
                g_Settings->SaveBool(Debugger_SkipOp, true);
            }
        }
    }

    if (!isStepping())
    {
        return;
    }

    if (R4300iOp::m_NextInstruction != JUMP)
    {
        goto breakpoint_hit;
    }

    if (JumpToLocation == PROGRAM_COUNTER + 4)
    {
        // Only pause on delay slots when branch isn't taken
        goto breakpoint_hit;
    }

    return;

breakpoint_hit:
    g_Settings->SaveBool(Debugger_SteppingOps, true);
}

void CDebuggerUI::CPUStep()
{
    OPCODE Opcode = R4300iOp::m_Opcode;
    uint32_t op = Opcode.op;
    uint32_t funct = Opcode.funct;

    if (m_StackTrace == NULL)
    {
        m_StackTrace = new CDebugStackTrace(this);
    }

    if (op == R4300i_JAL || ((op == R4300i_SPECIAL) && (funct == R4300i_SPECIAL_JALR) && (Opcode.rd == 31))) // JAL or JALR RA, x
    {
        m_StackTrace->PushEntry(R4300iOp::m_JumpToLocation, g_Reg->m_PROGRAM_COUNTER);
    }
    else if (funct == R4300i_SPECIAL_JR && Opcode.rs == 31) // JR RA
    {
        m_StackTrace->PopEntry();
    }
    else if (op == R4300i_CP0 && funct == R4300i_COP0_CO_ERET) // TODO may need more work
    {
        m_StackTrace->ClearEntries();
    }
}

void CDebuggerUI::FrameDrawn()
{
    static HWND hMainWnd = NULL;

    static HFONT monoFont = CreateFont(-11, 0, 0, 0,
        FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        PROOF_QUALITY, FF_DONTCARE, "Consolas"
    );

    if (hMainWnd == NULL)
    {
        RenderWindow* mainWindow = g_Plugins->MainWindow();

        if (mainWindow == NULL)
        {
            return;
        }

        hMainWnd = (HWND)mainWindow->GetWindowHandle();
    }

    HDC hdc = GetDC(hMainWnd);

    CRect rt;

    GetClientRect(hMainWnd, &rt);
    SetBkColor(hdc, RGB(0, 0, 0));

    SelectObject(hdc, monoFont);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));

    m_ScriptSystem->SetScreenDC(hdc);
    m_ScriptSystem->HookFrameDrawn()->InvokeAll();

    ReleaseDC(hMainWnd, hdc);
}

void CDebuggerUI::WaitForStep(void)
{
    g_Settings->SaveBool(Debugger_WaitingForStep, true);
    m_StepEvent.IsTriggered(SyncEvent::INFINITE_TIMEOUT);
    g_Settings->SaveBool(Debugger_WaitingForStep, false);
}

bool CDebuggerUI::ExecutionBP(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ExecutionBPExists(address, true) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP8(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists8(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP16(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists16(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP32(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists32(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP64(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists64(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP8(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists8(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP16(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists16(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP32(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists32(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP64(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists64(address) != CBreakpoints::BP_NOT_SET;
}

