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

void CCommandList::Attach(HWND hWndNew)
{
    CListViewCtrl::Attach(hWndNew);

    ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
    SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);

    AddColumn("", COL_ARROWS);
    SetColumnWidth(COL_ARROWS, 30);

    AddColumn("Address", COL_ADDRESS);
    SetColumnWidth(COL_ADDRESS, 70);

    AddColumn("Command", COL_COMMAND);
    SetColumnWidth(COL_COMMAND, 65);

    AddColumn("Parameters", COL_PARAMETERS);
    SetColumnWidth(COL_PARAMETERS, 130);

    AddColumn("Symbol", COL_SYMBOL);
    SetColumnWidth(COL_SYMBOL, 180);
}

CDebugCommandsView* CDebugCommandsView::_this = NULL;
HHOOK CDebugCommandsView::hWinMessageHook = NULL;

CDebugCommandsView::CDebugCommandsView(CDebuggerUI * debugger, SyncEvent &StepEvent) :
    CDebugDialog<CDebugCommandsView>(debugger),
    CToolTipDialog<CDebugCommandsView>(),
    m_StepEvent(StepEvent),
    m_Attached(false)
{
    m_HistoryIndex = -1;
    m_bIgnoreAddrChange = false;
    m_StartAddress = 0x80000000;
    m_Breakpoints = m_Debugger->Breakpoints();
    m_bEditing = false;
    m_CommandListRows = 39;
    m_RowHeight = 13;

    g_Settings->RegisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunningChanged);
}

CDebugCommandsView::~CDebugCommandsView()
{
    g_Settings->UnregisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunningChanged);
}

LRESULT CDebugCommandsView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    g_Settings->RegisterChangeCB(Debugger_WaitingForStep, this, (CSettings::SettingChangedFunc)StaticWaitingForStepChanged);
    g_Settings->RegisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)StaticSteppingOpsChanged);

    m_CommandList.Attach(GetDlgItem(IDC_CMD_LIST));
    m_BreakpointList.Attach(GetDlgItem(IDC_BP_LIST));
    m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
    m_PCEdit.Attach(GetDlgItem(IDC_PC_EDIT));
    m_ViewPCButton.Attach(GetDlgItem(IDC_VIEWPC_BTN));
    m_StepButton.Attach(GetDlgItem(IDC_STEP_BTN));
    m_StepOverButton.Attach(GetDlgItem(IDC_STEPOVER_BTN));
    m_SkipButton.Attach(GetDlgItem(IDC_SKIP_BTN));
    m_GoButton.Attach(GetDlgItem(IDC_GO_BTN));
    m_RegisterTabs.Attach(GetDlgItem(IDC_REG_TABS), m_Debugger);
    m_Scrollbar.Attach(GetDlgItem(IDC_SCRL_BAR));
    m_BackButton.Attach(GetDlgItem(IDC_BACK_BTN));
    m_ForwardButton.Attach(GetDlgItem(IDC_FORWARD_BTN));
    m_OpEdit.Attach(GetDlgItem(IDC_OP_EDIT));

    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_CommandsPos);
    DlgToolTip_Init();

    // Setup address input
    m_AddressEdit.SetDisplayType(CEditNumber32::DisplayHex);
    m_AddressEdit.SetLimitText(8);

    // Setup PC register input
    m_PCEdit.SetDisplayType(CEditNumber32::DisplayHex);
    m_PCEdit.SetLimitText(8);

    m_bIgnorePCChange = true;
    m_PCEdit.SetValue(0x80000180, DisplayMode::ZeroExtend);

    // Setup View PC button
    m_ViewPCButton.EnableWindow(FALSE);
    m_StepButton.EnableWindow(FALSE);
    m_StepOverButton.EnableWindow(FALSE);
    m_SkipButton.EnableWindow(FALSE);
    m_GoButton.EnableWindow(FALSE);

    // Setup breakpoint list
    m_BreakpointList.ModifyStyle(NULL, LBS_NOTIFY);
    RefreshBreakpointList();

    // Setup list scrollbar
    m_Scrollbar.SetScrollRange(0, 100, FALSE);
    m_Scrollbar.SetScrollPos(50, TRUE);

    // Setup history buttons
    ToggleHistoryButtons();

    // Op editor
    m_OpEdit.SetCommandsWindow(this);

    m_bIgnoreAddrChange = true;
    m_AddressEdit.SetValue(0x80000000, DisplayMode::ZeroExtend);
    ShowAddress(0x80000000, TRUE);
    m_bIgnoreAddrChange = false;

    if (isStepping())
    {
        m_ViewPCButton.EnableWindow(TRUE);
        m_StepButton.EnableWindow(TRUE);
        m_StepOverButton.EnableWindow(TRUE);
        m_SkipButton.EnableWindow(TRUE);
        m_GoButton.EnableWindow(TRUE);
    }

    _this = this;

    DWORD dwThreadID = ::GetCurrentThreadId();
    hWinMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)HookProc, NULL, dwThreadID);

    LoadWindowPos();
    RedrawCommandsAndRegisters();
    WindowCreated();
    m_Attached = true;

    RecompilerCheck();

    return TRUE;
}

void CDebugCommandsView::GameCpuRunningChanged(CDebugCommandsView* _this)
{
    _this->RecompilerCheck();
}

void CDebugCommandsView::RecompilerCheck(void)
{
    if (!g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        return;
    }

    if (!_this->IsWindow())
    {
        return;
    }

    if (g_Settings->LoadBool(Debugger_Enabled) &&
        !g_Settings->LoadBool(Setting_ForceInterpreterCPU) &&
        (CPU_TYPE)g_Settings->LoadDword(Game_CpuType) != CPU_Interpreter)
    {
        MessageBox("Debugger support for the recompiler core is experimental.\n\n"
            "For optimal experience, enable \"Always use interpreter core\" "
            "in advanced settings and restart the emulator.",
            "Warning", MB_ICONWARNING | MB_OK);
    }
}

void CDebugCommandsView::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

LRESULT CDebugCommandsView::OnDestroy(void)
{
    m_Attached = false;
    g_Settings->UnregisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)StaticSteppingOpsChanged);
    g_Settings->UnregisterChangeCB(Debugger_WaitingForStep, this, (CSettings::SettingChangedFunc)StaticWaitingForStepChanged);

    UnhookWindowsHookEx(hWinMessageHook);
    m_OpEdit.Detach();
    m_ForwardButton.Detach();
    m_BackButton.Detach();
    m_Scrollbar.Detach();
    m_RegisterTabs.Detach();
    m_GoButton.Detach();
    m_SkipButton.Detach();
    m_StepButton.Detach();
    m_StepOverButton.Detach();
    m_ViewPCButton.Detach();
    m_PCEdit.Detach();
    m_AddressEdit.Detach();
    m_BreakpointList.Detach();
    m_CommandList.Detach();
    return 0;
}

void CDebugCommandsView::InterceptKeyDown(WPARAM wParam, LPARAM /*lParam*/)
{
    switch (wParam)
    {
    case VK_F1: CPUSkip(); break;
    case VK_F2: 
        if (WaitingForStep())
        {
            m_StepEvent.Trigger();
        }
        break;
    case VK_F3: CPUStepOver(); break;
    case VK_F4: CPUResume(); break;
    }
}

void CDebugCommandsView::InterceptMouseWheel(WPARAM wParam, LPARAM /*lParam*/)
{
    uint32_t newAddress = m_StartAddress - ((short)HIWORD(wParam) / WHEEL_DELTA) * 4;

    m_StartAddress = newAddress;

    m_AddressEdit.SetValue(m_StartAddress, DisplayMode::ZeroExtend);
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
        _this->InterceptMouseWheel(pMsg->wParam, pMsg->lParam);
    }

    if (nCode < 0)
    {
        return CallNextHookEx(hWinMessageHook, nCode, wParam, lParam);
    }

    return 0;
}

LRESULT CDebugCommandsView::OnOpKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
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
        char text[256] = { 0 };
        m_OpEdit.GetWindowTextA(text, sizeof(text) - 1);
        uint32_t op;
        bool bValid = CAssembler::AssembleLine(text, &op, m_SelectedAddress);
        if (bValid)
        {
            m_OpEdit.SetWindowTextA("");
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

void CDebugCommandsView::ClearBranchArrows()
{
    m_BranchArrows.clear();
}

void CDebugCommandsView::AddBranchArrow(int startPos, int endPos)
{
    int startMargin = 0;
    int endMargin = 0;
    int margin = 0;

    for (size_t j = 0; j < m_BranchArrows.size(); j++)
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

void CDebugCommandsView::ShowAddress(uint32_t address, bool top, bool bUserInput)
{
    if (top == TRUE)
    {
        m_StartAddress = address - address % 4;

        if (!bUserInput)
        {
            m_bIgnoreAddrChange = true;
            m_AddressEdit.SetValue(address, DisplayMode::ZeroExtend);
        }

        if (!isStepping())
        {
            // Disable buttons
            m_ViewPCButton.EnableWindow(FALSE);
            m_StepButton.EnableWindow(FALSE);
            m_StepOverButton.EnableWindow(FALSE);
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
            m_StartAddress = address - address % 4;
            m_bIgnoreAddrChange = true;
            m_AddressEdit.SetValue(address, DisplayMode::ZeroExtend);
        }

        if (m_History.size() == 0 || m_History[m_HistoryIndex] != m_StartAddress)
        {
            HistoryPushState();
        }

        m_bIgnorePCChange = true;
        m_PCEdit.SetValue(g_Reg->m_PROGRAM_COUNTER, DisplayMode::ZeroExtend);

        // Enable buttons
        m_ViewPCButton.EnableWindow(TRUE);
        m_StepButton.EnableWindow(TRUE);
        m_StepOverButton.EnableWindow(TRUE);
        m_SkipButton.EnableWindow(TRUE);
        m_GoButton.EnableWindow(TRUE);

        m_RegisterTabs.SetColorsEnabled(true);
    }

    m_CommandList.SetRedraw(FALSE);
    m_CommandList.DeleteAllItems();

    ClearBranchArrows();

    m_bvAnnotatedLines.clear();

    for (int i = 0; i < m_CommandListRows; i++)
    {
        uint32_t opAddr = m_StartAddress + i * 4;

        m_CommandList.AddItem(i, CCommandList::COL_ARROWS, " ");

        char addrStr[9];
        sprintf(addrStr, "%08X", opAddr);

        m_CommandList.AddItem(i, CCommandList::COL_ADDRESS, addrStr);

        COpInfo OpInfo;
        OPCODE& OpCode = OpInfo.m_OpCode;

        if (!m_Debugger->DebugLoad_VAddr(opAddr, OpCode.Hex))
        {
            m_CommandList.AddItem(i, CCommandList::COL_COMMAND, "***");
            m_bvAnnotatedLines.push_back(false);
            continue;
        }

        char* command = (char*)R4300iOpcodeName(OpCode.Hex, opAddr);
        char* cmdName = strtok((char*)command, "\t");
        char* cmdArgs = strtok(NULL, "\t");

        CSymbol jalSymbol;

        // Show subroutine symbol name for JAL target
        if (OpCode.op == R4300i_JAL)
        {
            uint32_t targetAddr = (m_StartAddress & 0xF0000000) | (OpCode.target << 2);

            if (m_Debugger->SymbolTable()->GetSymbolByAddress(targetAddr, &jalSymbol))
            {
                cmdArgs = (char*)jalSymbol.m_Name;
            }
        }

        // Detect reads and writes to mapped registers, cart header data, etc
        const char* annotation = NULL;
        bool bLoadStoreAnnotation = false;

        CSymbol memSymbol;

        if (OpInfo.IsLoadStoreCommand())
        {
            for (int offset = -4; offset > -24; offset -= 4)
            {
                OPCODE OpCodeTest;

                if (!m_Debugger->DebugLoad_VAddr(opAddr + offset, OpCodeTest.Hex))
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

                if (m_Debugger->SymbolTable()->GetSymbolByAddress(memAddr, &memSymbol))
                {
                    annotation = memSymbol.m_Name;
                }
                else
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
        CSymbol pcSymbol;
        if (m_Debugger->SymbolTable()->GetSymbolByAddress(opAddr, &pcSymbol))
        {
            m_CommandList.AddItem(i, CCommandList::COL_SYMBOL, pcSymbol.m_Name);
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

    switch (drawStage)
    {
    case CDDS_PREPAINT: return (CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
    case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
    case (CDDS_ITEMPREPAINT | CDDS_SUBITEM): break;
    case CDDS_POSTPAINT:
        DrawBranchArrows(pLVCD->nmcd.hdc);
        return CDRF_DODEFAULT;
    default:
        return CDRF_DODEFAULT;
    }

    uint32_t nItem = (uint32_t)pLVCD->nmcd.dwItemSpec;
    uint32_t nSubItem = pLVCD->iSubItem;

    uint32_t address = m_StartAddress + (nItem * 4);
    uint32_t pc = (g_Reg != NULL) ? g_Reg->m_PROGRAM_COUNTER : 0;

    OPCODE pcOpcode;
    if (!m_Debugger->DebugLoad_VAddr(pc, pcOpcode.Hex))
    {
        pcOpcode.Hex = 0;
    }

    if (nSubItem == CCommandList::COL_ARROWS)
    {
        return CDRF_DODEFAULT;
    }

    if (nSubItem == CCommandList::COL_ADDRESS) // addr
    {
        CBreakpoints::BPSTATE bpState = m_Breakpoints->ExecutionBPExists(address);

        if (bpState == CBreakpoints::BP_SET)
        {
            // breakpoint
            pLVCD->clrTextBk = RGB(0x44, 0x00, 0x00);
            pLVCD->clrText = (address == pc && isDebugging()) ?
                RGB(0xFF, 0xFF, 0x00) : // breakpoint & current pc
                RGB(0xFF, 0xCC, 0xCC);
        }
        else if (bpState == CBreakpoints::BP_SET_TEMP)
        {
            // breakpoint
            pLVCD->clrTextBk = RGB(0x66, 0x44, 0x00);
            pLVCD->clrText = (address == pc && isDebugging()) ?
                RGB(0xFF, 0xFF, 0x00) : // breakpoint & current pc
                RGB(0xFF, 0xEE, 0xCC);
        }
        else if (address == pc && isStepping())
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
    bool bAddrOkay = m_Debugger->DebugLoad_VAddr(address, OpCode.Hex);

    struct {
        COLORREF bg;
        COLORREF fg;
    } colors;

    if (!bAddrOkay)
    {
        colors = { 0xFFFFFF, 0xFF0000 };
    }
    else if (address == pc && isStepping())
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

    if (!isStepping())
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

LRESULT CDebugCommandsView::OnMeasureItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (wParam == IDC_CMD_LIST)
    {
        CClientDC dc(m_hWnd);
        dc.SelectFont(GetFont());
        TEXTMETRIC tm;
        dc.GetTextMetrics(&tm);

        m_RowHeight = tm.tmHeight + tm.tmExternalLeading;

        MEASUREITEMSTRUCT* lpMeasureItem = (MEASUREITEMSTRUCT*)lParam;
        lpMeasureItem->itemHeight = m_RowHeight;
    }
    return FALSE;
}

// Draw branch arrows
void CDebugCommandsView::DrawBranchArrows(HDC listDC)
{
    COLORREF colors[] =
    {
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
    CBrush hBrushBg(CreateSolidBrush(bgColor));
    FillRect(listDC, &paneRect, hBrushBg);

    for (size_t i = 0; i < m_BranchArrows.size(); i++)
    {
        int colorIdx = i % nColors;
        COLORREF color = colors[colorIdx];

        BRANCHARROW arrow = m_BranchArrows[i];

        int begX = baseX - arrow.startMargin * 3;
        int endX = baseX - arrow.endMargin * 3;

        int begY = baseY + arrow.startPos * m_RowHeight;
        int endY = baseY + arrow.endPos * m_RowHeight;

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
        CPen hPenOutline(CreatePen(PS_SOLID, 3, bgColor));
        SelectObject(listDC, hPenOutline);
        MoveToEx(listDC, begX - 1, begY, NULL);
        LineTo(listDC, marginX, begY);
        LineTo(listDC, marginX, endY);
        if (bEndVisible)
        {
            LineTo(listDC, endX + 2, endY);
        }

        // draw fill line
        CPen hPen(CreatePen(PS_SOLID, 1, color));
        SelectObject(listDC, hPen);
        MoveToEx(listDC, begX - 1, begY, NULL);
        LineTo(listDC, marginX, begY);
        LineTo(listDC, marginX, endY);
        if (bEndVisible)
        {
            LineTo(listDC, endX + 2, endY);
        }

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

    CBreakpoints::breakpoints_t ReadBreakPoints = m_Breakpoints->ReadMem();
    for (CBreakpoints::breakpoints_t::iterator itr = ReadBreakPoints.begin(); itr != ReadBreakPoints.end(); itr++)
    {
        sprintf(rowStr, "R %s%08X", itr->second ? "T " : "", itr->first);
        int index = m_BreakpointList.AddString(rowStr);
        m_BreakpointList.SetItemData(index, itr->first);
    }

    CBreakpoints::breakpoints_t WriteBreakPoints = m_Breakpoints->WriteMem();
    for (CBreakpoints::breakpoints_t::iterator itr = WriteBreakPoints.begin(); itr != WriteBreakPoints.end(); itr++)
    {
        sprintf(rowStr, "W %s%08X", itr->second ? "T " : "", itr->first);
        int index = m_BreakpointList.AddString(rowStr);
        m_BreakpointList.SetItemData(index, itr->first);
    }

    CBreakpoints::breakpoints_t ExecutionBreakPoints = m_Breakpoints->Execution();
    for (CBreakpoints::breakpoints_t::iterator itr = ExecutionBreakPoints.begin(); itr != ExecutionBreakPoints.end(); itr++)
    {
        sprintf(rowStr, "E %s%08X", itr->second ? "T " : "", itr->first);
        int index = m_BreakpointList.AddString(rowStr);
        m_BreakpointList.SetItemData(index, itr->first);
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
        m_Breakpoints->RemoveExecution(address);
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
    g_Settings->SaveBool(Debugger_SkipOp, true);
    if (WaitingForStep())
    {
        m_StepEvent.Trigger();
    }
}

void CDebugCommandsView::CPUResume()
{
    g_Settings->SaveBool(Debugger_SteppingOps, false);
    if (WaitingForStep())
    {
        m_StepEvent.Trigger();
    }
}

void CDebugCommandsView::CPUStepOver()
{
    COpInfo opInfo;
    if (!m_Debugger->DebugLoad_VAddr(g_Reg->m_PROGRAM_COUNTER, opInfo.m_OpCode.Hex))
    {
        return;
    }

    if (opInfo.IsJAL())
    {
        // put temp BP on return address and resume
        m_Breakpoints->AddExecution(g_Reg->m_PROGRAM_COUNTER + 8, true);
        CPUResume();
    }
    else
    {
        // normal step
        if (WaitingForStep())
        {
            m_StepEvent.Trigger();
        }
    }
}

LRESULT CDebugCommandsView::OnBackButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    if (m_HistoryIndex > 0)
    {
        m_HistoryIndex--;
        m_AddressEdit.SetValue(m_History[m_HistoryIndex], DisplayMode::ZeroExtend);
        ToggleHistoryButtons();
    }
    return FALSE;
}

LRESULT CDebugCommandsView::OnForwardButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    if (m_History.size() > 0 && m_HistoryIndex < (int)m_History.size() - 1)
    {
        m_HistoryIndex++;
        m_AddressEdit.SetValue(m_History[m_HistoryIndex], DisplayMode::ZeroExtend);
        ToggleHistoryButtons();
    }
    return FALSE;
}

LRESULT CDebugCommandsView::OnViewPCButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    if (g_Reg != NULL && isStepping())
    {
        ShowAddress(g_Reg->m_PROGRAM_COUNTER, TRUE);
    }
    return FALSE;
}

LRESULT CDebugCommandsView::OnSymbolsButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_Debugger->OpenSymbolsWindow();
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuRunTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    // Add temp bp and resume
    m_Breakpoints->AddExecution(m_SelectedAddress, true);
    return FALSE;
}

LRESULT CDebugCommandsView::OnGoButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CPUResume();
    m_AddressEdit.SetFocus();
    return FALSE;
}

LRESULT CDebugCommandsView::OnStepButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    if (WaitingForStep())
    {
        m_StepEvent.Trigger();
    }
    return FALSE;
}

LRESULT CDebugCommandsView::OnStepOverButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CPUStepOver();
    return FALSE;
}

LRESULT CDebugCommandsView::OnSkipButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CPUSkip();
    m_AddressEdit.SetFocus();
    return FALSE;
}

LRESULT CDebugCommandsView::OnClearBPButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_Breakpoints->BPClear();
    RefreshBreakpointList();
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnAddBPButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_AddBreakpointDlg.DoModal(m_Debugger);
    RefreshBreakpointList();
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnRemoveBPButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    RemoveSelectedBreakpoints();
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    EndDialog(0);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    BeginOpEdit(m_SelectedAddress);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuInsertNOP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    EditOp(m_SelectedAddress, 0x00000000);
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuRestore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    RestoreOp(m_SelectedAddress);
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuRestoreAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    RestoreAllOps();
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuAddSymbol(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_AddSymbolDlg.DoModal(m_Debugger, m_SelectedAddress, SYM_CODE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuFollowJump(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    HistoryPushState();
    ShowAddress(m_FollowAddress, TRUE);
    HistoryPushState();
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuViewMemory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_Debugger->Debug_ShowMemoryLocation(m_FollowAddress, true);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuToggleBP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_Breakpoints->EBPToggle(m_SelectedAddress);
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

LRESULT CDebugCommandsView::OnPopupmenuClearBP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_Breakpoints->EBPClear();
    ShowAddress(m_StartAddress, TRUE);
    return FALSE;
}

void CDebugCommandsView::BeginOpEdit(uint32_t address)
{
    uint32_t opcode;
    if (!m_Debugger->DebugLoad_VAddr(address, opcode))
    {
        return;
    }

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
    if (!m_Attached)
    {
        return 0;
    }
    if (m_bIgnoreAddrChange)
    {
        m_bIgnoreAddrChange = false;
        return 0;
    }

    uint32_t address = m_AddressEdit.GetValue();
    ShowAddress(address, TRUE, TRUE);

    return 0;
}

LRESULT CDebugCommandsView::OnPCChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_Attached)
    {
        return 0;
    }
    if (m_bIgnorePCChange)
    {
        m_bIgnorePCChange = false;
        return 0;
    }
    if (g_Reg != NULL && isStepping())
    {
        g_Reg->m_PROGRAM_COUNTER = m_PCEdit.GetValue();
    }
    return 0;
}

LRESULT CDebugCommandsView::OnCommandListClicked(NMHDR* /*pNMHDR*/)
{
    EndOpEdit();
    return 0;
}

LRESULT CDebugCommandsView::OnCommandListDblClicked(NMHDR* pNMHDR)
{
    // Set PC breakpoint
    NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int nItem = pIA->iItem;

    uint32_t address = m_StartAddress + nItem * 4;
    if (m_Breakpoints->ExecutionBPExists(address))
    {
        m_Breakpoints->RemoveExecution(address);
    }
    else
    {
        m_Breakpoints->AddExecution(address);
    }
    // Cancel blue highlight
    m_AddressEdit.SetFocus();
    RefreshBreakpointList();

    return 0;
}

LRESULT CDebugCommandsView::OnCommandListRightClicked(NMHDR* pNMHDR)
{
    EndOpEdit();

    NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int nItem = pIA->iItem;

    uint32_t address = m_StartAddress + nItem * 4;
    m_SelectedAddress = address;

    if (!m_Debugger->DebugLoad_VAddr(m_SelectedAddress, m_SelectedOpCode.Hex))
    {
        return 0;
    }

    HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_OP_POPUP));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);
    
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

    if (m_SelectedOpInfo.IsLoadStoreCommand())
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

    if (m_Breakpoints->Execution().size() == 0)
    {
        EnableMenuItem(hPopupMenu, ID_POPUPMENU_CLEARBPS, MF_DISABLED | MF_GRAYED);
    }

    POINT mouse;
    GetCursorPos(&mouse);

    TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, mouse.x, mouse.y, 0, m_hWnd, NULL);

    DestroyMenu(hMenu);

    return 0;
}

LRESULT CDebugCommandsView::OnListBoxClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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

LRESULT CDebugCommandsView::OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (!m_Attached)
    {
        return false;
    }
    if (LOWORD(wParam) != WA_INACTIVE)
    {
        ShowAddress(m_StartAddress, TRUE);
        RefreshBreakpointList();
    }
    return FALSE;
}

void CDebugCommandsView::RedrawCommandsAndRegisters()
{
    CRect listRect;
    m_CommandList.GetWindowRect(listRect);

    CRect headRect;
    CHeaderCtrl listHead = m_CommandList.GetHeader();
    listHead.GetWindowRect(&headRect);

    int rowsHeight = listRect.Height() - headRect.Height();

    int nRows = (rowsHeight / m_RowHeight);

    if (m_CommandListRows != nRows)
    {
        m_CommandListRows = nRows;
        ShowAddress(m_StartAddress, TRUE);
    }

    m_RegisterTabs.RedrawCurrentTab();

    // Fix cmd list header
    listHead.ResizeClient(listRect.Width(), headRect.Height());
}

LRESULT CDebugCommandsView::OnSizing(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    RedrawCommandsAndRegisters();
    return FALSE;
}

LRESULT CDebugCommandsView::OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    WORD type = LOWORD(wParam);

    switch (type)
    {
    case SB_LINEUP:
        ShowAddress(m_StartAddress - 4, TRUE);
        break;
    case SB_LINEDOWN:
        ShowAddress(m_StartAddress + 4, TRUE);
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

void CDebugCommandsView::WaitingForStepChanged(void)
{
    if (WaitingForStep())
    {
        ShowAddress(g_Reg->m_PROGRAM_COUNTER, false);
        m_Debugger->Debug_RefreshStackWindow();
        m_Debugger->Debug_RefreshStackTraceWindow();
        m_StepButton.EnableWindow(true);
        m_StepOverButton.EnableWindow(true);
        m_GoButton.EnableWindow(true);
        m_AddressEdit.SetFocus();
    }
    else
    {
        m_StepButton.EnableWindow(false);
        m_StepOverButton.EnableWindow(false);
        m_GoButton.EnableWindow(false);
    }
}

void CDebugCommandsView::SteppingOpsChanged(void)
{
    if (!g_Settings->LoadBool(Debugger_SteppingOps))
    {
        m_Debugger->Debug_RefreshStackWindow();
        m_Debugger->Debug_RefreshStackTraceWindow();
        m_RegisterTabs.SetColorsEnabled(false);
        m_RegisterTabs.RefreshEdits();
        ShowAddress(m_StartAddress, TRUE);
    }
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
    for (size_t i = 0; i < m_EditedOps.size(); i++)
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
    if (!m_Debugger->DebugLoad_VAddr(address, currentOp))
    {
        return;
    }

    if (currentOp == op)
    {
        return;
    }

    m_Debugger->DebugStore_VAddr(address, op);

    if (!IsOpEdited(address))
    {
        m_EditedOps.push_back({ address, currentOp });
    }

    ShowAddress(m_StartAddress, TRUE);
}

void CDebugCommandsView::RestoreOp(uint32_t address)
{
    for (size_t i = 0; i < m_EditedOps.size(); i++)
    {
        if (m_EditedOps[i].address == address)
        {
            m_Debugger->DebugStore_VAddr(m_EditedOps[i].address, m_EditedOps[i].originalOp);
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
        m_Debugger->DebugStore_VAddr(m_EditedOps[i].address, m_EditedOps[i].originalOp);
        m_EditedOps.erase(m_EditedOps.begin() + i);
    }
}

void CDebugCommandsView::ShowPIRegTab()
{
    m_RegisterTabs.SetCurSel(2);
    m_RegisterTabs.ShowTab(2);
}

LRESULT CDebugCommandsView::OnRegisterTabChange(NMHDR* /*pNMHDR*/)
{
    int nPage = m_RegisterTabs.GetCurSel();
    m_RegisterTabs.ShowTab(nPage);
    m_RegisterTabs.RedrawCurrentTab();
    m_RegisterTabs.RedrawWindow();
    return FALSE;
}

void CDebugCommandsView::ToggleHistoryButtons()
{
    if (m_BackButton.m_hWnd != NULL)
    {
        m_BackButton.EnableWindow(m_History.size() != 0 && m_HistoryIndex > 0 ? TRUE : FALSE);
    }

    if (m_ForwardButton.m_hWnd != NULL)
    {
        m_ForwardButton.EnableWindow(m_History.size() != 0 && m_HistoryIndex < (int)m_History.size() - 1 ? TRUE : FALSE);
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