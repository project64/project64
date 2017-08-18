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

#include "Symbols.h"
#include "Breakpoints.h"
#include "Assembler.h"
#include "OpInfo.h"

#include <Project64-core/N64System/Mips/OpCodeName.h>

CDebugCommandsView* CDebugCommandsView::_this = NULL;
HHOOK CDebugCommandsView::hWinMessageHook = NULL;

CDebugCommandsView::CDebugCommandsView(CDebuggerUI * debugger) :
CDebugDialog<CDebugCommandsView>(debugger),
CToolTipDialog<CDebugCommandsView>()
{
	m_HistoryIndex = -1;
	m_bIgnoreAddrChange = false;
	m_StartAddress = 0x80000000;
	m_Breakpoints = m_Debugger->Breakpoints();
	m_bEditing = false;
}

CDebugCommandsView::~CDebugCommandsView(void)
{

}

LRESULT	CDebugCommandsView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DlgResize_Init(false, true);
	DlgToolTip_Init();

	//m_ptMinTrackSize.x = 580;
	//m_ptMinTrackSize.y = 495;

	m_CommandListRows = 39;
	
	CheckCPUType();
	
	GetWindowRect(&m_DefaultWindowRect);

	// Setup address input

	m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
	m_AddressEdit.SetDisplayType(CEditNumber::DisplayHex);
	m_AddressEdit.SetLimitText(8);

	// Setup PC register input
	
	m_PCEdit.Attach(GetDlgItem(IDC_PC_EDIT));
	m_PCEdit.SetDisplayType(CEditNumber::DisplayHex);
	m_PCEdit.SetLimitText(8);

	m_bIgnorePCChange = true;
	m_PCEdit.SetValue(0x80000180, false, true);

	// Setup View PC button

	m_ViewPCButton.Attach(GetDlgItem(IDC_VIEWPC_BTN));
	m_ViewPCButton.EnableWindow(FALSE);

	// Setup debugging buttons

	m_StepButton.Attach(GetDlgItem(IDC_STEP_BTN));
	m_StepButton.EnableWindow(FALSE);

	m_SkipButton.Attach(GetDlgItem(IDC_SKIP_BTN));
	m_SkipButton.EnableWindow(FALSE);

	m_GoButton.Attach(GetDlgItem(IDC_GO_BTN));
	m_GoButton.EnableWindow(FALSE);

	// Setup register tabs & inputs

	m_RegisterTabs.Attach(GetDlgItem(IDC_REG_TABS));

	// Setup breakpoint list

	m_BreakpointList.Attach(GetDlgItem(IDC_BP_LIST));
	m_BreakpointList.ModifyStyle(NULL, LBS_NOTIFY);
	RefreshBreakpointList();

	// Setup list scrollbar

	m_Scrollbar.Attach(GetDlgItem(IDC_SCRL_BAR));
	m_Scrollbar.SetScrollRange(0, 100, FALSE);
	m_Scrollbar.SetScrollPos(50, TRUE);
	//m_Scrollbar.GetScrollInfo(); // todo bigger thumb size
	//m_Scrollbar.SetScrollInfo();

	// Setup history buttons
	m_BackButton.Attach(GetDlgItem(IDC_BACK_BTN));
	m_ForwardButton.Attach(GetDlgItem(IDC_FORWARD_BTN));
	ToggleHistoryButtons();

	// Setup command list
	m_CommandList.Attach(GetDlgItem(IDC_CMD_LIST));

	// Op editor
	m_OpEdit.Attach(GetDlgItem(IDC_OP_EDIT));
	m_OpEdit.SetCommandsWindow(this);
	
	m_bIgnoreAddrChange = true;
	m_AddressEdit.SetValue(0x80000000, false, true);
	ShowAddress(0x80000000, TRUE);
	
	if (m_Breakpoints->isDebugging())
	{
		m_ViewPCButton.EnableWindow(TRUE);
		m_StepButton.EnableWindow(TRUE);
		m_SkipButton.EnableWindow(TRUE);
		m_GoButton.EnableWindow(TRUE);
	}
	
	_this = this;

	DWORD dwThreadID = ::GetCurrentThreadId();
	hWinMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)HookProc, NULL, dwThreadID);

	WindowCreated();
	return TRUE;
}

LRESULT CDebugCommandsView::OnDestroy(void)
{
	return 0;
}

void CDebugCommandsView::InterceptKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_F1: CPUSkip(); break;
	case VK_F2: CPUStepInto(); break;
	case VK_F3:
		// reserved step over
		break;
	case VK_F4: CPUResume(); break;
	}
}

void CDebugCommandsView::InterceptMouseWheel(WPARAM wParam, LPARAM lParam)
{
	uint32_t newAddress = m_StartAddress - ((short)HIWORD(wParam) / WHEEL_DELTA) * 4;

	m_StartAddress = newAddress;

	m_AddressEdit.SetValue(m_StartAddress, false, true);
}

LRESULT CALLBACK CDebugCommandsView::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG *pMsg = (MSG*)lParam;

	if (pMsg->message == WM_KEYDOWN)
	{
		_this->InterceptKeyDown(pMsg->wParam, pMsg->lParam);
	}
	else if (pMsg->message == WM_MOUSEWHEEL)
	{
		BOOL bHandled = TRUE;
		_this->InterceptMouseWheel(pMsg->wParam, pMsg->lParam);
	}

	if (nCode < 0)
	{
		return CallNextHookEx(hWinMessageHook, nCode, wParam, lParam);
	}

	return 0;
}

LRESULT	CDebugCommandsView::OnOpKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == VK_UP)
	{
		m_SelectedAddress -= 4;
		BeginOpEdit(m_SelectedAddress);
		bHandled = TRUE;
	}
	else if (wParam == VK_DOWN)
	{
		m_SelectedAddress += 4;
		BeginOpEdit(m_SelectedAddress);
		bHandled = TRUE;
	}
	else if (wParam == VK_RETURN)
	{
		int textLen = m_OpEdit.GetWindowTextLengthA();
		char text[256];
		m_OpEdit.GetWindowTextA(text, 255);
		m_OpEdit.SetWindowTextA("");
		uint32_t op;
		bool bValid = CAssembler::AssembleLine(text, &op, m_SelectedAddress);
		if (bValid)
		{
			EditOp(m_SelectedAddress, op);
			m_SelectedAddress += 4;
			BeginOpEdit(m_SelectedAddress);
		}
		bHandled = TRUE;
	}
	else if (wParam == VK_ESCAPE)
	{
		EndOpEdit();
		bHandled = TRUE;
	}
	return 1;
}

void CDebugCommandsView::CheckCPUType()
{
	CPU_TYPE cpuType;

	if (g_Settings->LoadBool(Setting_ForceInterpreterCPU))
	{
		cpuType = CPU_Interpreter;
	}
	else
	{
		cpuType = g_System->CpuType();
	}
	
	if (cpuType != CPU_Interpreter)
	{
		MessageBox("Interpreter mode required", "Invalid CPU Type", MB_OK);
	}
}

// Check if KSEG0 addr is out of bounds
bool CDebugCommandsView::AddressSafe(uint32_t vaddr)
{
	if (g_MMU == NULL)
	{
		return false;
	}

	if (vaddr >= 0x80000000 && vaddr <= 0x9FFFFFFF)
	{
		if ((vaddr & 0x1FFFFFFF) >= g_MMU->RdramSize())
		{
			return false;
		}
	}

	return true;
}

void CDebugCommandsView::ClearBranchArrows()
{
	m_BranchArrows.clear();
}

void CDebugCommandsView::AddBranchArrow(int startPos, int endPos)
{
	int startMargin = 0;
	int endMargin = 0;
	int margin = 0;

	for (int j = 0; j < m_BranchArrows.size(); j++)
	{
		BRANCHARROW arrow = m_BranchArrows[j];

		// Arrow's start or end pos within another arrow's stride
		if ((startPos >= arrow.startPos && startPos <= arrow.endPos) ||
			(endPos >= arrow.startPos && endPos <= arrow.endPos) ||
			(arrow.startPos <= startPos && arrow.startPos >= endPos))
		{
			if (margin <= arrow.margin)
			{
				margin = arrow.margin + 1;
			}
		}

		if (startPos == arrow.startPos)
		{
			startMargin = arrow.startMargin + 1;
		}

		if (startPos == arrow.endPos)
		{
			startMargin = arrow.endMargin + 1;
		}

		if (endPos == arrow.startPos)
		{
			endMargin = arrow.startMargin + 1;
		}

		if (endPos == arrow.endPos)
		{
			endMargin = arrow.endMargin + 1;
		}
	}

	m_BranchArrows.push_back({ startPos, endPos, startMargin, endMargin, margin });
}

void CDebugCommandsView::HistoryPushState()
{
	m_History.push_back(m_StartAddress);
	m_HistoryIndex = m_History.size() - 1;
	ToggleHistoryButtons();
}

const char* CDebugCommandsView::GetDataAddressNotes(uint32_t vAddr)
{
	switch (vAddr)
	{
	case 0xA3F00000: return "RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG";
	case 0xA3F00004: return "RDRAM_DEVICE_ID_REG";
	case 0xA3F00008: return "RDRAM_DELAY_REG";
	case 0xA3F0000C: return "RDRAM_MODE_REG";
	case 0xA3F00010: return "RDRAM_REF_INTERVAL_REG";
	case 0xA3F00014: return "RDRAM_REF_ROW_REG";
	case 0xA3F00018: return "RDRAM_RAS_INTERVAL_REG";
	case 0xA3F0001C: return "RDRAM_MIN_INTERVAL_REG";
	case 0xA3F00020: return "RDRAM_ADDR_SELECT_REG";
	case 0xA3F00024: return "RDRAM_DEVICE_MANUF_REG";

	case 0xA4040000: return "SP_MEM_ADDR_REG";
	case 0xA4040004: return "SP_DRAM_ADDR_REG";
	case 0xA4040008: return "SP_RD_LEN_REG";
	case 0xA404000C: return "SP_WR_LEN_REG";
	case 0xA4040010: return "SP_STATUS_REG";
	case 0xA4040014: return "SP_DMA_FULL_REG";
	case 0xA4040018: return "SP_DMA_BUSY_REG";
	case 0xA404001C: return "SP_SEMAPHORE_REG";

	case 0xA4080000: return "SP_PC";

	case 0xA4100000: return "DPC_START_REG";
	case 0xA4100004: return "DPC_END_REG";
	case 0xA4100008: return "DPC_CURRENT_REG";
	case 0xA410000C: return "DPC_STATUS_REG";
	case 0xA4100010: return "DPC_CLOCK_REG";
	case 0xA4100014: return "DPC_BUFBUSY_REG";
	case 0xA4100018: return "DPC_PIPEBUSY_REG";
	case 0xA410001C: return "DPC_TMEM_REG";

	case 0xA4300000: return "MI_INIT_MODE_REG/MI_MODE_REG";
	case 0xA4300004: return "MI_VERSION_REG/MI_NOOP_REG";
	case 0xA4300008: return "MI_INTR_REG";
	case 0xA430000C: return "MI_INTR_MASK_REG";

	case 0xA4400000: return "VI_STATUS_REG/VI_CONTROL_REG";
	case 0xA4400004: return "VI_ORIGIN_REG/VI_DRAM_ADDR_REG";
	case 0xA4400008: return "VI_WIDTH_REG/VI_H_WIDTH_REG";
	case 0xA440000C: return "VI_INTR_REG/VI_V_INTR_REG";
	case 0xA4400010: return "VI_CURRENT_REG/VI_V_CURRENT_LINE_REG";
	case 0xA4400014: return "VI_BURST_REG/VI_TIMING_REG";
	case 0xA4400018: return "VI_V_SYNC_REG";
	case 0xA440001C: return "VI_H_SYNC_REG";
	case 0xA4400020: return "VI_LEAP_REG/VI_H_SYNC_LEAP_REG";
	case 0xA4400024: return "VI_H_START_REG/VI_H_VIDEO_REG";
	case 0xA4400028: return "VI_V_START_REG/VI_V_VIDEO_REG";
	case 0xA440002C: return "VI_V_BURST_REG";
	case 0xA4400030: return "VI_X_SCALE_REG";
	case 0xA4400034: return "VI_Y_SCALE_REG";

	case 0xA4500000: return "AI_DRAM_ADDR_REG";
	case 0xA4500004: return "AI_LEN_REG";
	case 0xA4500008: return "AI_CONTROL_REG";
	case 0xA450000C: return "AI_STATUS_REG";
	case 0xA4500010: return "AI_DACRATE_REG";
	case 0xA4500014: return "AI_BITRATE_REG";

	case 0xA4600000: return "PI_DRAM_ADDR_REG";
	case 0xA4600004: return "PI_CART_ADDR_REG";
	case 0xA4600008: return "PI_RD_LEN_REG";
	case 0xA460000C: return "PI_WR_LEN_REG";
	case 0xA4600010: return "PI_STATUS_REG";
	case 0xA4600014: return "PI_BSD_DOM1_LAT_REG";
	case 0xA4600018: return "PI_BSD_DOM1_PWD_REG";
	case 0xA460001C: return "PI_BSD_DOM1_PGS_REG";
	case 0xA4600020: return "PI_BSD_DOM1_RLS_REG";
	case 0xA4600024: return "PI_BSD_DOM2_LAT_REG";
	case 0xA4600028: return "PI_BSD_DOM2_PWD_REG";
	case 0xA460002C: return "PI_BSD_DOM2_PGS_REG";
	case 0xA4600030: return "PI_BSD_DOM2_RLS_REG";

	case 0xA4700000: return "RI_MODE_REG";
	case 0xA4700004: return "RI_CONFIG_REG";
	case 0xA4700008: return "RI_CURRENT_LOAD_REG";
	case 0xA470000C: return "RI_SELECT_REG";
	case 0xA4700010: return "RI_REFRESH_REG/RI_COUNT_REG";
	case 0xA4700014: return "RI_LATENCY_REG";
	case 0xA4700018: return "RI_RERROR_REG";
	case 0xA470001C: return "RI_WERROR_REG";

	case 0xA4800000: return "SI_DRAM_ADDR_REG";
	case 0xA4800004: return "SI_PIF_ADDR_RD64B_REG";
	case 0xA4800010: return "SI_PIF_ADDR_WR64B_REG";
	case 0xA4800018: return "SI_STATUS_REG";

	case 0xA5000500: return "ASIC_DATA";
	case 0xA5000504: return "ASIC_MISC_REG";
	case 0xA5000508: return "ASIC_STATUS";
	case 0xA500050C: return "ASIC_CUR_TK";
	case 0xA5000510: return "ASIC_BM_STATUS";
	case 0xA5000514: return "ASIC_ERR_SECTOR";
	case 0xA5000518: return "ASIC_SEQ_STATUS";
	case 0xA500051C: return "ASIC_CUR_SECTOR";
	case 0xA5000520: return "ASIC_HARD_RESET";
	case 0xA5000524: return "ASIC_C1_S0";
	case 0xA5000528: return "ASIC_HOST_SECBYTE";
	case 0xA500052C: return "ASIC_C1_S2";
	case 0xA5000530: return "ASIC_SEC_BYTE";
	case 0xA5000534: return "ASIC_C1_S4";
	case 0xA5000538: return "ASIC_C1_S6";
	case 0xA500053C: return "ASIC_CUR_ADDR";
	case 0xA5000540: return "ASIC_ID_REG";
	case 0xA5000544: return "ASIC_TEST_REG";
	case 0xA5000548: return "ASIC_TEST_PIN_SEL";

	case 0xB0000004: return "Header: Clock rate";
	case 0xB0000008: return "Header: Game entry point";
	case 0xB000000C: return "Header: Release";
	case 0xB0000010: return "Header: CRC1";
	case 0xB0000014: return "Header: CRC2";
	}

	return NULL;
}

const char* CDebugCommandsView::GetCodeAddressNotes(uint32_t vAddr)
{
	switch (vAddr)
	{
	case 0x80000000: return "Exception: TLB Refill";
	case 0x80000080: return "Exception: XTLB Refill";
	case 0x80000100: return "Exception: Cache error (See A0000100)";
	case 0x80000180: return "Exception: General";

	case 0xA0000100: return "Exception: Cache error";

	case 0xBFC00000: return "Exception: Reset/NMI";
	case 0xBFC00200: return "Exception: TLB Refill (boot)";
	case 0xBFC00280: return "Exception: XTLB Refill (boot)";
	case 0xBFC00300: return "Exception: Cache error (boot)";
	case 0xBFC00380: return "Exception: General (boot)";
	}

	if (g_MMU == NULL)
	{
		return NULL;
	}

	uint8_t* rom = g_Rom->GetRomAddress();
	uint32_t gameEntryPoint = *(uint32_t*)&rom[0x08];

	if (vAddr == gameEntryPoint)
	{
		return "Game entry point";
	}

	return NULL;
}

void CDebugCommandsView::ShowAddress(DWORD address, BOOL top)
{
	if (top == TRUE)
	{
		m_StartAddress = address;

		if (!m_Breakpoints->isDebugging())
		{
			// Disable buttons
			m_ViewPCButton.EnableWindow(FALSE);
			m_StepButton.EnableWindow(FALSE);
			m_SkipButton.EnableWindow(FALSE);
			m_GoButton.EnableWindow(FALSE);

			m_RegisterTabs.SetColorsEnabled(false);
		}
	}
	else
	{
		bool bOutOfView = address < m_StartAddress ||
			address > m_StartAddress + (m_CommandListRows - 1) * 4;

		if (bOutOfView)
		{
			m_StartAddress = address;
			m_bIgnoreAddrChange = true;
			m_AddressEdit.SetValue(address, false, true);
		}

		if (m_History.size() == 0 || m_History[m_HistoryIndex] != m_StartAddress)
		{
			HistoryPushState();
		}

		m_bIgnorePCChange = true;
		m_PCEdit.SetValue(g_Reg->m_PROGRAM_COUNTER, false, true);

		// Enable buttons
		m_ViewPCButton.EnableWindow(TRUE);
		m_StepButton.EnableWindow(TRUE);
		m_SkipButton.EnableWindow(TRUE);
		m_GoButton.EnableWindow(TRUE);

		m_RegisterTabs.SetColorsEnabled(true);
	}
	
	m_CommandList.SetRedraw(FALSE);
	m_CommandList.DeleteAllItems();
	
	ClearBranchArrows();
	
	m_bvAnnotatedLines.clear();

	CSymbols::EnterCriticalSection();

	for (int i = 0; i < m_CommandListRows; i++)
	{
		uint32_t opAddr = m_StartAddress + i * 4;

		m_CommandList.AddItem(i, CCommandList::COL_ARROWS, " ");

		char addrStr[9];
		sprintf(addrStr, "%08X", opAddr);

		m_CommandList.AddItem(i, CCommandList::COL_ADDRESS, addrStr);

		COpInfo OpInfo;
		OPCODE& OpCode = OpInfo.m_OpCode;
		bool bAddrOkay = false;

		if (AddressSafe(opAddr))
		{
			bAddrOkay = g_MMU->LW_VAddr(opAddr, OpCode.Hex);
		}

		if (!bAddrOkay)
		{
			m_CommandList.AddItem(i, CCommandList::COL_COMMAND, "***");
			m_bvAnnotatedLines.push_back(false);
			continue;
		}

		char* command = (char*)R4300iOpcodeName(OpCode.Hex, opAddr);
		char* cmdName = strtok((char*)command, "\t");
		char* cmdArgs = strtok(NULL, "\t");

		// Show subroutine symbol name for JAL target
		if (OpCode.op == R4300i_JAL)
		{
			uint32_t targetAddr = (0x80000000 | (OpCode.target << 2));

			// todo move symbols management to CDebuggerUI
			const char* targetSymbolName = CSymbols::GetNameByAddress(targetAddr);
			if (targetSymbolName != NULL)
			{
				cmdArgs = (char*)targetSymbolName;
			}
		}

		// Detect reads and writes to mapped registers, cart header data, etc
		const char* annotation = NULL;
		bool bLoadStoreAnnotation = false;

		if (OpInfo.IsLoadStore())
		{
			for (int i = -4; i > -24; i -= 4)
			{
				if (!AddressSafe(opAddr + i))
				{
					break;
				}

				OPCODE OpCodeTest;
				bAddrOkay = g_MMU->LW_VAddr(opAddr + i, OpCodeTest.Hex);

				if (!bAddrOkay)
				{
					break;
				}

				if (OpCodeTest.op != R4300i_LUI)
				{
					continue;
				}

				if (OpCodeTest.rt != OpCode.rs)
				{
					continue;
				}

				uint32_t memAddr = (OpCodeTest.immediate << 16) + (short)OpCode.offset;

				annotation = CSymbols::GetNameByAddress(memAddr);

				if (annotation == NULL)
				{
					annotation = GetDataAddressNotes(memAddr);
				}
				break;
			}
		}

		if (annotation == NULL)
		{
			annotation = GetCodeAddressNotes(opAddr);
		}
		else
		{
			bLoadStoreAnnotation = true;
		}
		
		m_CommandList.AddItem(i, CCommandList::COL_COMMAND, cmdName);
		m_CommandList.AddItem(i, CCommandList::COL_PARAMETERS, cmdArgs);

		// Show routine symbol name for this address
		const char* routineSymbolName = CSymbols::GetNameByAddress(opAddr);
		if (routineSymbolName != NULL)
		{
			m_CommandList.AddItem(i, CCommandList::COL_SYMBOL, routineSymbolName);
			m_bvAnnotatedLines.push_back(false);
		}
		else if (annotation != NULL)
		{
			const char* annotationFormat = bLoadStoreAnnotation ? "// (%s)" : "// %s";
			m_CommandList.AddItem(i, CCommandList::COL_SYMBOL, stdstr_f(annotationFormat, annotation).c_str());
			m_bvAnnotatedLines.push_back(true);
		}
		else
		{
			m_bvAnnotatedLines.push_back(false);
		}

		// Add arrow for branch instruction
		if (OpInfo.IsBranch())
		{
			int startPos = i;
			int endPos = startPos + (int16_t)OpCode.offset + 1;

			AddBranchArrow(startPos, endPos);
		}

		// Branch arrow for close J
		if (OpCode.op == R4300i_J)
		{
			uint32_t target = (OpCode.target << 2);
			int dist = target - (opAddr & 0x3FFFFFF);
			if (abs(dist) < 0x10000)
			{
				int startPos = i;
				int endPos = startPos + (dist / 4);
				AddBranchArrow(startPos, endPos);
			}
		}
	}

	CSymbols::LeaveCriticalSection();
	
	if (!top) // update registers when called via breakpoint/stepping
	{
		m_RegisterTabs.RefreshEdits();
	}

	RefreshBreakpointList();
	
	m_CommandList.SetRedraw(TRUE);
}

// Highlight command list items & draw branch arrows
LRESULT CDebugCommandsView::OnCustomDrawList(NMHDR* pNMHDR)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	DWORD drawStage = pLVCD->nmcd.dwDrawStage;

	HDC hDC = pLVCD->nmcd.hdc;
	
	switch (drawStage)
	{
	case CDDS_PREPAINT: 
		return (CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
	case CDDS_POSTPAINT:
		DrawBranchArrows(hDC);
		return CDRF_DODEFAULT;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
		break;
	default:
		return CDRF_DODEFAULT;
	}
	
	DWORD nItem = pLVCD->nmcd.dwItemSpec;
	DWORD nSubItem = pLVCD->iSubItem;

	uint32_t address = m_StartAddress + (nItem * 4);
	uint32_t pc = (g_Reg != NULL) ? g_Reg->m_PROGRAM_COUNTER : 0;

	OPCODE pcOpcode;
	if (g_MMU != NULL)
	{
		g_MMU->LW_VAddr(pc, pcOpcode.Hex);
	}
	
	if (nSubItem == CCommandList::COL_ARROWS)
	{
		return CDRF_DODEFAULT;
	}

	if (nSubItem == CCommandList::COL_ADDRESS) // addr
	{
		CBreakpoints::BPSTATE bpState = m_Breakpoints->EBPExists(address);

		if (bpState == CBreakpoints::BP_SET)
		{
			// breakpoint
			pLVCD->clrTextBk = RGB(0x44, 0x00, 0x00);
			pLVCD->clrText = (address == pc) ?
				RGB(0xFF, 0xFF, 0x00) : // breakpoint & current pc
				RGB(0xFF, 0xCC, 0xCC);
		}
		else if (bpState == CBreakpoints::BP_SET_TEMP)
		{
			// breakpoint
			pLVCD->clrTextBk = RGB(0x66, 0x44, 0x00);
			pLVCD->clrText = (address == pc) ?
				RGB(0xFF, 0xFF, 0x00) : // breakpoint & current pc
				RGB(0xFF, 0xEE, 0xCC);
		}
		else if (address == pc && m_Breakpoints->isDebugging())
		{
			// pc
			pLVCD->clrTextBk = RGB(0x88, 0x88, 0x88);
			pLVCD->clrText = RGB(0xFF, 0xFF, 0);
		}
		else
		{
			//default
			pLVCD->clrTextBk = RGB(0xEE, 0xEE, 0xEE);
			pLVCD->clrText = RGB(0x44, 0x44, 0x44);
		}

		return CDRF_DODEFAULT;
	}

	// (nSubItem == 1 || nSubItem == 2) 

	// cmd & args
	COpInfo OpInfo;
	OPCODE& OpCode = OpInfo.m_OpCode;
	bool bAddrOkay = false;

	if (AddressSafe(address))
	{
		bAddrOkay = g_MMU->LW_VAddr(address, OpCode.Hex);
	}
	
	struct {
		COLORREF bg;
		COLORREF fg;
	} colors;
	
	if (!bAddrOkay)
	{
		colors = { 0xFFFFFF, 0xFF0000 };
	}
	else if (address == pc && m_Breakpoints->isDebugging())
	{
		colors = { 0xFFFFAA, 0x222200 };
	}
	else if (IsOpEdited(address))
	{
		colors = { 0xFFEEFF, 0xFF00FF };
	}
	else if (OpInfo.IsStackAlloc())
	{
		colors = { 0xCCDDFF, 0x001144 };
	}
	else if (OpInfo.IsStackFree())
	{
		colors = { 0xFFDDDD, 0x440000 };
	}
	else if (OpInfo.IsNOP())
	{
		colors = { 0xFFFFFF, 0x888888 };
	}
	else if (OpInfo.IsJump())
	{
		colors = { 0xEEFFEE, 0x006600 };
	}
	else if (OpInfo.IsBranch())
	{
		colors = { 0xFFFFFF, 0x337700 };
	}
	else
	{
		colors = { 0xFFFFFF, 0x0000000 };
	}
	
	// Gray annotations
	if (nSubItem == CCommandList::COL_SYMBOL)
	{
		if (m_bvAnnotatedLines[nItem])
		{
			colors.fg = 0x666666;
		}
	}

	pLVCD->clrTextBk = _byteswap_ulong(colors.bg) >> 8;
	pLVCD->clrText = _byteswap_ulong(colors.fg) >> 8;

	if (!m_Breakpoints->isDebugging())
	{
		return CDRF_DODEFAULT;
	}

	// color register usage
	// todo localise to temp register context (dont look before/after jumps and frame shifts)
	COLORREF clrUsedRegister = RGB(0xF5, 0xF0, 0xFF); // light purple
	COLORREF clrAffectedRegister = RGB(0xFF, 0xF0, 0xFF); // light pink

	int pcUsedRegA = 0, pcUsedRegB = 0, pcChangedReg = 0;
	int curUsedRegA = 0, curUsedRegB = 0, curChangedReg = 0;

	if (pcOpcode.op == R4300i_SPECIAL)
	{
		pcUsedRegA = pcOpcode.rs;
		pcUsedRegB = pcOpcode.rt;
		pcChangedReg = pcOpcode.rd;
	}
	else
	{
		pcUsedRegA = pcOpcode.rs;
		pcChangedReg = pcOpcode.rt;
	}

	if (OpCode.op == R4300i_SPECIAL)
	{
		curUsedRegA = OpCode.rs;
		curUsedRegB = OpCode.rt;
		curChangedReg = OpCode.rd;
	}
	else
	{
		curUsedRegA = OpCode.rs;
		curChangedReg = OpCode.rt;
	}

	if (address < pc)
	{
		if (curChangedReg != 0 && (pcUsedRegA == curChangedReg || pcUsedRegB == curChangedReg))
		{
			pLVCD->clrTextBk = clrUsedRegister;
		}
	}
	else if (address > pc)
	{
		if (pcChangedReg != 0 && (curUsedRegA == pcChangedReg || curUsedRegB == pcChangedReg))
		{
			pLVCD->clrTextBk = clrAffectedRegister;
		}
	}
	return CDRF_DODEFAULT;
}

LRESULT	CDebugCommandsView::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == IDC_CMD_LIST)
	{
		MEASUREITEMSTRUCT* lpMeasureItem = (MEASUREITEMSTRUCT*)lParam;
		lpMeasureItem->itemHeight = CCommandList::ROW_HEIGHT;
	}
	return FALSE;
}

// Draw branch arrows
void CDebugCommandsView::DrawBranchArrows(HDC listDC)
{
	COLORREF colors[] = {
		RGB(240, 240, 240), // white
		RGB(30, 135, 255), // blue
		RGB(255, 0, 200), // pink
		RGB(215, 155, 0), // yellow
		RGB(100, 180, 0), // green
		RGB(200, 100, 255), // purple
		RGB(120, 120, 120), // gray
		RGB(0, 220, 160), // cyan
		RGB(255, 100, 0), // orange
		RGB(255, 255, 0), // yellow
	};
	
	int nColors = sizeof(colors) / sizeof(COLORREF);
	
	CRect listRect;
	m_CommandList.GetWindowRect(&listRect);
	ScreenToClient(&listRect);
	
	CRect headRect;
	m_CommandList.GetHeader().GetWindowRect(&headRect);
	ScreenToClient(&headRect);
	
	int colWidth = m_CommandList.GetColumnWidth(CCommandList::COL_ARROWS);

	int baseX = colWidth - 4;
	int baseY = headRect.bottom + 7;
	
	CRect paneRect;
	paneRect.top = headRect.bottom;
	paneRect.left = 0;
	paneRect.right = colWidth;
	paneRect.bottom = listRect.bottom;
	
	COLORREF bgColor = RGB(30, 30, 30);
	HBRUSH hBrushBg = CreateSolidBrush(bgColor);
	FillRect(listDC, &paneRect, hBrushBg);
	DeleteObject(hBrushBg);

	for (int i = 0; i < m_BranchArrows.size(); i++)
	{
		int colorIdx = i % nColors;
		COLORREF color = colors[colorIdx];
		
		BRANCHARROW arrow = m_BranchArrows[i];
	
		int begX = baseX - arrow.startMargin * 3;
		int endX = baseX - arrow.endMargin * 3;

		int begY = baseY + arrow.startPos * CCommandList::ROW_HEIGHT;
		int endY = baseY + arrow.endPos * CCommandList::ROW_HEIGHT;

		bool bEndVisible = true;

		if (endY < headRect.bottom)
		{
			endY = headRect.bottom + 1;
			bEndVisible = false;
		}
		else if (endY > listRect.bottom)
		{
			endY = listRect.bottom - 2;
			bEndVisible = false;
		}

		int marginX = baseX - (4 + arrow.margin * 3);

		// draw start pointer
		SetPixel(listDC, begX + 0, begY - 1, color);
		SetPixel(listDC, begX + 1, begY - 2, color);
		SetPixel(listDC, begX + 0, begY + 1, color);
		SetPixel(listDC, begX + 1, begY + 2, color);

		// draw outline
		HPEN hPenOutline = CreatePen(PS_SOLID, 3, bgColor);
		SelectObject(listDC, hPenOutline);
		MoveToEx(listDC, begX - 1, begY, NULL);
		LineTo(listDC, marginX, begY);
		LineTo(listDC, marginX, endY);
		if (bEndVisible)
		{
			LineTo(listDC, endX + 2, endY);
		}
		DeleteObject(hPenOutline);

		// draw fill line
		HPEN hPen = CreatePen(PS_SOLID, 1, color);
		SelectObject(listDC, hPen);
		MoveToEx(listDC, begX - 1, begY, NULL);
		LineTo(listDC, marginX, begY);
		LineTo(listDC, marginX, endY);
		if (bEndVisible)
		{
			LineTo(listDC, endX + 2, endY);
		}
		DeleteObject(hPen);

		// draw end pointer
		if (bEndVisible)
		{
			SetPixel(listDC, endX - 0, endY - 1, color);
			SetPixel(listDC, endX - 1, endY - 2, color);
			SetPixel(listDC, endX - 1, endY - 1, color);
			SetPixel(listDC, endX - 0, endY + 1, color);
			SetPixel(listDC, endX - 1, endY + 2, color);
			SetPixel(listDC, endX - 1, endY + 1, color);
			SetPixel(listDC, endX - 1, endY + 3, RGB(30, 30, 30));
			SetPixel(listDC, endX - 1, endY - 3, RGB(30, 30, 30));
		}
	}
}

void CDebugCommandsView::RefreshBreakpointList()
{
	m_BreakpointList.ResetContent();
	char rowStr[16];
	for (int i = 0; i < m_Breakpoints->m_nRBP; i++)
	{
		CBreakpoints::BREAKPOINT breakpoint = m_Breakpoints->m_RBP[i];
		bool bTemp = breakpoint.bTemporary;
		sprintf(rowStr, "R %s%08X", bTemp ? "T " : "", breakpoint.address);
		int index = m_BreakpointList.AddString(rowStr);
		m_BreakpointList.SetItemData(index, breakpoint.address);
	}
	for (int i = 0; i < m_Breakpoints->m_nWBP; i++)
	{
		CBreakpoints::BREAKPOINT breakpoint = m_Breakpoints->m_WBP[i];
		bool bTemp = breakpoint.bTemporary;
		sprintf(rowStr, "W %s%08X", bTemp ? "T " : "", breakpoint.address);
		int index = m_BreakpointList.AddString(rowStr);
		m_BreakpointList.SetItemData(index, breakpoint.address);
	}
	for (int i = 0; i < m_Breakpoints->m_nEBP; i++)
	{
		CBreakpoints::BREAKPOINT breakpoint = m_Breakpoints->m_EBP[i];
		bool bTemp = breakpoint.bTemporary;
		sprintf(rowStr, "E %s%08X", bTemp ? "T " : "", breakpoint.address);
		int index = m_BreakpointList.AddString(rowStr);
		m_BreakpointList.SetItemData(index, breakpoint.address);
	}
}

void CDebugCommandsView::RemoveSelectedBreakpoints()
{
	int nItem = m_BreakpointList.GetCurSel();
	
	if (nItem == LB_ERR)
	{
		return;
	}

	char itemText[32];
	m_BreakpointList.GetText(nItem, itemText);

	uint32_t address = m_BreakpointList.GetItemData(nItem);

	switch (itemText[0])
	{
	case 'E':
		m_Breakpoints->EBPRemove(address);
		break;
	case 'W':
		m_Breakpoints->WBPRemove(address);
		break;
	case 'R':
		m_Breakpoints->RBPRemove(address);
		break;
	}

	RefreshBreakpointList();
}

void CDebugCommandsView::CPUSkip()
{
	m_Breakpoints->KeepDebugging();
	m_Breakpoints->Skip();
	m_Breakpoints->Resume();
}

void CDebugCommandsView::CPUStepInto()
{
	m_Debugger->Debug_RefreshStackWindow();
	m_Breakpoints->KeepDebugging();
	m_Breakpoints->Resume();
}

void CDebugCommandsView::CPUResume()
{
	m_Debugger->Debug_RefreshStackWindow();
	m_Breakpoints->StopDebugging();
	m_Breakpoints->Resume();
	m_RegisterTabs.SetColorsEnabled(false);
	m_RegisterTabs.RefreshEdits();
	ShowAddress(m_StartAddress, TRUE);
}

LRESULT CDebugCommandsView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch (wID)
	{
	case IDC_BACK_BTN:
		if (m_HistoryIndex > 0)
		{
			m_HistoryIndex--;
			m_AddressEdit.SetValue(m_History[m_HistoryIndex], false, true);
			ToggleHistoryButtons();
		}
		break;
	case IDC_FORWARD_BTN:
		if (m_History.size() > 0 && m_HistoryIndex < m_History.size() - 1)
		{
			m_HistoryIndex++;
			m_AddressEdit.SetValue(m_History[m_HistoryIndex], false, true);
			ToggleHistoryButtons();
		}
		break;
	case IDC_VIEWPC_BTN:
		if (g_Reg != NULL && m_Breakpoints->isDebugging())
		{
			ShowAddress(g_Reg->m_PROGRAM_COUNTER, TRUE);
		}
		break;
	case IDC_SYMBOLS_BTN:
		m_Debugger->Debug_ShowSymbolsWindow();
		break;
	case ID_POPUPMENU_RUNTO:
		// Add temp bp and resume
		m_Breakpoints->EBPAdd(m_SelectedAddress, true);
	case IDC_GO_BTN:
		CPUResume();
		m_AddressEdit.SetFocus();
		break;
	case IDC_STEP_BTN:
		CPUStepInto();
		m_AddressEdit.SetFocus();
		break;
	case IDC_SKIP_BTN:
		CPUSkip();
		m_AddressEdit.SetFocus();
		break;
	case IDC_CLEARBP_BTN:
		m_Breakpoints->BPClear();
		RefreshBreakpointList();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case IDC_ADDBP_BTN:
		m_AddBreakpointDlg.DoModal(m_Debugger);
		RefreshBreakpointList();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case IDC_RMBP_BTN:
		RemoveSelectedBreakpoints();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case IDCANCEL:
		EndDialog(0);
		break;
	//popup
	case ID_POPUPMENU_EDIT:
		BeginOpEdit(m_SelectedAddress);
		break;
	case ID_POPUPMENU_INSERTNOP:
		EditOp(m_SelectedAddress, 0x00000000);
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_RESTORE:
		RestoreOp(m_SelectedAddress);
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_RESTOREALL:
		RestoreAllOps();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_ADDSYMBOL:
		m_AddSymbolDlg.DoModal(m_Debugger, m_SelectedAddress, CSymbols::TYPE_CODE);
		break;
	case ID_POPUPMENU_FOLLOWJUMP:
		HistoryPushState();
		ShowAddress(m_FollowAddress, TRUE);
		HistoryPushState();
		break;
	case ID_POPUPMENU_VIEWMEMORY:
		m_Debugger->Debug_ShowMemoryLocation(m_FollowAddress, true);
		break;
	case ID_POPUPMENU_TOGGLEBP:
		m_Breakpoints->EBPToggle(m_SelectedAddress);
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_CLEARBPS:
		m_Breakpoints->EBPClear();
		ShowAddress(m_StartAddress, TRUE);
		break;
	}
	return FALSE;
}

void CDebugCommandsView::GotoEnteredAddress()
{
	char text[9];

	m_AddressEdit.GetWindowTextA(text, 9);

	DWORD address = strtoul(text, NULL, 16);
	address = address - address % 4;
	ShowAddress(address, TRUE);
}

void CDebugCommandsView::BeginOpEdit(uint32_t address)
{
	CRect listRect;
	m_CommandList.GetWindowRect(&listRect);
	ScreenToClient(&listRect);
	
	m_bEditing = true;
	//ShowAddress(address, FALSE);
	int nItem = (address - m_StartAddress) / 4;

	CRect itemRect;
	m_CommandList.GetSubItemRect(nItem, CCommandList::COL_COMMAND, 0, &itemRect);
	//itemRect.bottom += 0;
	itemRect.left += listRect.left + 3;
	itemRect.right += 100;
	
	uint32_t opcode;
	g_MMU->LW_VAddr(address, opcode);
	char* command = (char*)R4300iOpcodeName(opcode, address);

	m_OpEdit.ShowWindow(SW_SHOW);
	m_OpEdit.MoveWindow(&itemRect);
	m_OpEdit.BringWindowToTop();
	m_OpEdit.SetWindowTextA(command);
	m_OpEdit.SetFocus();
	m_OpEdit.SetSelAll();

	m_CommandList.RedrawWindow();
	m_OpEdit.RedrawWindow();
}

void CDebugCommandsView::EndOpEdit()
{
	m_bEditing = false;
	m_OpEdit.SetWindowTextA("");
	m_OpEdit.ShowWindow(SW_HIDE);
}

LRESULT CDebugCommandsView::OnAddrChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_bIgnoreAddrChange)
	{
		m_bIgnoreAddrChange = false;
		return 0;
	}
	GotoEnteredAddress();
	return 0;
}

LRESULT CDebugCommandsView::OnPCChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_bIgnorePCChange)
	{
		m_bIgnorePCChange = false;
		return 0;
	}
	if (g_Reg != NULL && m_Breakpoints->isDebugging())
	{
		g_Reg->m_PROGRAM_COUNTER = m_PCEdit.GetValue();
	}
	return 0;
}

LRESULT	CDebugCommandsView::OnCommandListClicked(NMHDR* pNMHDR)
{
	EndOpEdit();
	return 0;
}

LRESULT	CDebugCommandsView::OnCommandListDblClicked(NMHDR* pNMHDR)
{
	// Set PC breakpoint
	NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	int nItem = pIA->iItem;
	
	uint32_t address = m_StartAddress + nItem * 4;
	if (m_Breakpoints->EBPExists(address))
	{
		m_Breakpoints->EBPRemove(address);
	}
	else
	{
		m_Breakpoints->EBPAdd(address);
	}
	// Cancel blue highlight
	m_AddressEdit.SetFocus();
	RefreshBreakpointList();

	return 0;
}

LRESULT	CDebugCommandsView::OnCommandListRightClicked(NMHDR* pNMHDR)
{
	EndOpEdit();

	if (g_MMU == NULL)
	{
		return 0;
	}

	NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	int nItem = pIA->iItem;

	uint32_t address = m_StartAddress + nItem * 4;
	m_SelectedAddress = address;

	HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_OP_POPUP));
	HMENU hPopupMenu = GetSubMenu(hMenu, 0);
	
	g_MMU->LW_VAddr(m_SelectedAddress, m_SelectedOpCode.Hex);

	if (m_SelectedOpInfo.IsStaticJump())
	{
		m_FollowAddress = (m_SelectedAddress & 0xF0000000) | (m_SelectedOpCode.target * 4);
	}
	else if (m_SelectedOpInfo.IsBranch())
	{
		m_FollowAddress = m_SelectedAddress + ((int16_t)m_SelectedOpCode.offset + 1) * 4;
	}
	else
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_FOLLOWJUMP, MF_DISABLED | MF_GRAYED);
	}

	if (m_SelectedOpInfo.IsLoadStore())
	{
		m_FollowAddress = g_Reg->m_GPR[m_SelectedOpCode.base].UW[0] + (int16_t)m_SelectedOpCode.offset;
	}
	else
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_VIEWMEMORY, MF_DISABLED | MF_GRAYED);
	}

	if (!IsOpEdited(m_SelectedAddress))
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_RESTORE, MF_DISABLED | MF_GRAYED);
	}
	
	if (m_EditedOps.size() == 0)
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_RESTOREALL, MF_DISABLED | MF_GRAYED);
	}

	if (m_SelectedOpInfo.IsNOP())
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_INSERTNOP, MF_DISABLED | MF_GRAYED);
	}

	if (m_Breakpoints->m_nEBP == 0)
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_CLEARBPS, MF_DISABLED | MF_GRAYED);
	}

	POINT mouse;
	GetCursorPos(&mouse);

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, mouse.x, mouse.y, 0, m_hWnd, NULL);

	DestroyMenu(hMenu);
	
	return 0;
}


LRESULT CDebugCommandsView::OnListBoxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (wID == IDC_BP_LIST)
	{
		int index = m_BreakpointList.GetCaretIndex();
		uint32_t address = m_BreakpointList.GetItemData(index);
		int len = m_BreakpointList.GetTextLen(index);
		char* rowText = (char*)malloc(len + 1);
		rowText[len] = '\0';
		m_BreakpointList.GetText(index, rowText);
		if (*rowText == 'E')
		{
			ShowAddress(address, true);
		}
		else
		{
			m_Debugger->Debug_ShowMemoryLocation(address, true);
		}
		free(rowText);
	}
	return FALSE;
}

LRESULT CDebugCommandsView::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	WORD type = LOWORD(wParam);

	if (type == WA_INACTIVE)
	{
		return FALSE;
	}

	if (type == WA_CLICKACTIVE)
	{
		CheckCPUType();
	}

	ShowAddress(m_StartAddress, TRUE);

	return FALSE;
}

LRESULT	CDebugCommandsView::OnSizing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CRect listRect;
	m_CommandList.GetWindowRect(listRect);

	CRect headRect;
	CHeaderCtrl listHead = m_CommandList.GetHeader();
	listHead.GetWindowRect(&headRect);

	int rowsHeight = listRect.Height() - headRect.Height();
	
	int nRows = (rowsHeight / CCommandList::ROW_HEIGHT);
	
	if (m_CommandListRows != nRows)
	{
		m_CommandListRows = nRows;
		ShowAddress(m_StartAddress, TRUE);
	}
	
	m_RegisterTabs.RedrawCurrentTab();

	// Fix cmd list header
	listHead.ResizeClient(listRect.Width(), headRect.Height());

	return FALSE;
}

LRESULT CDebugCommandsView::OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	WORD type = LOWORD(wParam);

	switch(type)
	{
	case SB_LINEUP:
		ShowAddress(m_StartAddress - 8, TRUE);
		break;
	case SB_LINEDOWN:
		ShowAddress(m_StartAddress + 8, TRUE);
		break;
	case SB_THUMBTRACK:
		{
			//int scrollPos = HIWORD(wParam);
			//ShowAddress(m_StartAddress + (scrollPos - 50) * 4, TRUE);
		}
		break;
	}

	return FALSE;
}

void CDebugCommandsView::Reset()
{
	ClearEditedOps();
	m_History.clear();
	ToggleHistoryButtons();
}

void CDebugCommandsView::ClearEditedOps()
{
	m_EditedOps.clear();
}

BOOL CDebugCommandsView::IsOpEdited(uint32_t address)
{
	for (int i = 0; i < m_EditedOps.size(); i++)
	{
		if (m_EditedOps[i].address == address)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void CDebugCommandsView::EditOp(uint32_t address, uint32_t op)
{
	uint32_t currentOp;
	g_MMU->LW_VAddr(address, currentOp);

	if (currentOp == op)
	{
		return;
	}

	g_MMU->SW_VAddr(address, op);

	if (!IsOpEdited(address))
	{
		m_EditedOps.push_back({ address, currentOp });
	}

	ShowAddress(m_StartAddress, TRUE);
}

void CDebugCommandsView::RestoreOp(uint32_t address)
{
	for (int i = 0; i < m_EditedOps.size(); i++)
	{
		if (m_EditedOps[i].address == address)
		{
			g_MMU->SW_VAddr(m_EditedOps[i].address, m_EditedOps[i].originalOp);
			m_EditedOps.erase(m_EditedOps.begin() + i);
			break;
		}
	}
}

void CDebugCommandsView::RestoreAllOps()
{
	int lastIndex = m_EditedOps.size() - 1;
	for (int i = lastIndex; i >= 0; i--)
	{
		g_MMU->SW_VAddr(m_EditedOps[i].address, m_EditedOps[i].originalOp);
		m_EditedOps.erase(m_EditedOps.begin() + i);
	}
}

void CDebugCommandsView::ShowPIRegTab()
{
	m_RegisterTabs.SetCurSel(2);
	m_RegisterTabs.ShowTab(2);
}

LRESULT CDebugCommandsView::OnRegisterTabChange(NMHDR* pNMHDR)
{
	int nPage = m_RegisterTabs.GetCurSel();
	m_RegisterTabs.ShowTab(nPage);
	m_RegisterTabs.RedrawCurrentTab();
	m_RegisterTabs.RedrawWindow();
	return FALSE;
}

void CDebugCommandsView::ToggleHistoryButtons()
{
	if (m_History.size() != 0 && m_HistoryIndex > 0)
	{
		m_BackButton.EnableWindow(TRUE);
	}
	else
	{
		m_BackButton.EnableWindow(FALSE);
	}

	if (m_History.size() != 0 && m_HistoryIndex < m_History.size() - 1)
	{
		m_ForwardButton.EnableWindow(TRUE);
	}
	else
	{
		m_ForwardButton.EnableWindow(FALSE);
	}
}

// Opcode editor

LRESULT CEditOp::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_CommandsWindow == NULL)
	{
		return FALSE;
	}
	return m_CommandsWindow->OnOpKeyDown(uMsg, wParam, lParam, bHandled);
}

void CEditOp::SetCommandsWindow(CDebugCommandsView* commandsWindow)
{
	m_CommandsWindow = commandsWindow;
}

BOOL CEditOp::Attach(HWND hWndNew)
{
	return SubclassWindow(hWndNew);
}