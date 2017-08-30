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
	m_DMALog(NULL)
{
	g_Debugger = this;

	m_Breakpoints = new CBreakpoints();
	m_ScriptSystem = new CScriptSystem(this);

	m_DMALog = new CDMALog();

	CSymbols::InitializeCriticalSection();
    g_Settings->RegisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
}

CDebuggerUI::~CDebuggerUI(void)
{
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

void CDebuggerUI::Debug_ShowMemoryDump()
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

void CDebuggerUI::Debug_ShowMemoryWindow(void)
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
    Debug_ShowMemoryWindow();
    if (m_MemoryView)
    {
        m_MemoryView->ShowAddress(Address, VAddr);
    }
}

void CDebuggerUI::Debug_ShowTLBWindow(void)
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

void CDebuggerUI::Debug_ShowMemorySearch()
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

void CDebuggerUI::Debug_ShowCommandsWindow()
{
	if (m_CommandsView == NULL)
	{
		m_CommandsView = new CDebugCommandsView(this);
	}
	m_CommandsView->ShowWindow();
}

void CDebuggerUI::Debug_ShowCommandsLocation(uint32_t address, bool top)
{
	Debug_ShowCommandsWindow();
	if (m_CommandsView)
	{
		m_CommandsView->ShowAddress(address, top);
	}
}

void CDebuggerUI::Debug_ShowScriptsWindow()
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

void CDebuggerUI::Debug_ShowSymbolsWindow()
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

void CDebuggerUI::Debug_ShowDMALogWindow(void)
{
	if (m_DMALogView == NULL)
	{
		m_DMALogView = new CDebugDMALogView(this);
	}
	m_DMALogView->ShowWindow();
}

void CDebuggerUI::Debug_ShowStackTrace(void)
{
	if (m_StackTrace == NULL)
	{
		m_StackTrace = new CDebugStackTrace(this);
	}
	m_StackTrace->ShowWindow();
}

void CDebuggerUI::Debug_ShowStackWindow(void)
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
	if (m_StackTrace != NULL)
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


void CDebuggerUI::BreakpointHit()
{
	m_Breakpoints->KeepDebugging();
	Debug_ShowCommandsLocation(g_Reg->m_PROGRAM_COUNTER, false);
	Debug_RefreshStackWindow();
	Debug_RefreshStackTraceWindow();
	m_Breakpoints->Pause();
}


// CDebugger implementation

void CDebuggerUI::TLBChanged()
{
	Debug_RefreshTLBWindow();
}

// Called from the interpreter core at the beginning of every CPU step
// Returns false when the instruction should be skipped
bool CDebuggerUI::CPUStepStarted()
{
	uint32_t PROGRAM_COUNTER = g_Reg->m_PROGRAM_COUNTER;
	uint32_t JumpToLocation = R4300iOp::m_JumpToLocation;
	
	m_ScriptSystem->HookCPUExec()->InvokeByParamInRange(PROGRAM_COUNTER);
	
	// PC breakpoints

	if (m_Breakpoints->EBPExists(PROGRAM_COUNTER, true))
	{
		goto breakpoint_hit;
	}

	// Memory breakpoints
	
	OPCODE Opcode = R4300iOp::m_Opcode;
	uint32_t op = Opcode.op;

	if (op >= R4300i_LDL && op <= R4300i_SD && op != R4300i_CACHE) // Read and write instructions
	{
		uint32_t memoryAddress = g_Reg->m_GPR[Opcode.base].UW[0] + (int16_t)Opcode.offset;
		
		if ((op <= R4300i_LWU || (op >= R4300i_LL && op <= R4300i_LD))) // Read instructions
		{
			m_ScriptSystem->HookCPURead()->InvokeByParamInRange(memoryAddress);
			
			if (m_Breakpoints->RBPExists(memoryAddress))
			{
				goto breakpoint_hit;
			}
		}
		else // Write instructions
		{
			m_ScriptSystem->HookCPUWrite()->InvokeByParamInRange(memoryAddress);

			if (m_Breakpoints->WBPExists(memoryAddress))
			{
				goto breakpoint_hit;
			}
			
			// Catch cart -> rdram dma
			if (memoryAddress == 0xA460000C) // PI_WR_LEN_REG
			{
				uint32_t dmaRomAddr = g_Reg->PI_CART_ADDR_REG & 0x0FFFFFFF;
				uint32_t dmaRamAddr = g_Reg->PI_DRAM_ADDR_REG | 0x80000000;
				uint32_t dmaLen = g_Reg->m_GPR[Opcode.rt].UW[0] + 1;
				uint32_t endAddr = dmaRamAddr + dmaLen;
				
				m_DMALog->AddEntry(dmaRomAddr, dmaRamAddr, dmaLen);

				for (int i = 0; i < m_Breakpoints->m_nWBP; i++)
				{
					uint32_t wbpAddr = m_Breakpoints->m_WBP[i].address;
					if (wbpAddr >= dmaRamAddr && wbpAddr < endAddr)
					{
						goto breakpoint_hit;
					}
				}
			}
		}
	}

	if (!m_Breakpoints->isDebugging())
	{
		return !m_Breakpoints->isSkipping();
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
	
	return !m_Breakpoints->isSkipping();

breakpoint_hit:
	BreakpointHit();
	return !m_Breakpoints->isSkipping();
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