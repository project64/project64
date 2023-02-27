#include "stdafx.h"

#include "CPULog.h"
#include "DebuggerUI.h"

#include "Debugger-CPULogView.h"
#include <Project64-core/N64System/Mips/R4300iInstruction.h>

CDebugCPULogView * CDebugCPULogView::_this = nullptr;
HHOOK CDebugCPULogView::hWinMessageHook = nullptr;

CDebugCPULogView::CDebugCPULogView(CDebuggerUI * debugger) :
    CDebugDialog<CDebugCPULogView>(debugger),
    m_CPULogCopy(nullptr),
    m_LogStartIndex(0),
    m_RowHeight(13)
{
}

CDebugCPULogView::~CDebugCPULogView()
{
    if (m_CPULogCopy != nullptr)
    {
        delete m_CPULogCopy;
    }
}

LRESULT CDebugCPULogView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgToolTip_Init();
    DlgSavePos_Init(DebuggerUI_CPULogPos);

    m_CPUListView.Attach(GetDlgItem(IDC_CPU_LIST));
    m_StateInfoEdit.Attach(GetDlgItem(IDC_STATEINFO_EDIT));
    m_EnabledChk.Attach(GetDlgItem(IDC_CHK_ENABLE));
    m_BuffSizeEdit.Attach(GetDlgItem(IDC_BUFFSIZE_EDIT));
    m_Scrollbar.Attach(GetDlgItem(IDC_SCRL_BAR));
    m_ExportBtn.Attach(GetDlgItem(IDC_BTN_EXPORT));

    m_CPUListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_CPUListView.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
    m_CPUListView.AddColumn(L"PC", 0);
    m_CPUListView.AddColumn(L"Command", 1);
    m_CPUListView.AddColumn(L"Parameters", 2);
    m_CPUListView.SetColumnWidth(0, 65);
    m_CPUListView.SetColumnWidth(1, 60);
    m_CPUListView.SetColumnWidth(2, 120);

    bool bLoggingEnabled = g_Settings->LoadBool(Debugger_CPULoggingEnabled);
    uint32_t bufferSize = g_Settings->LoadDword(Debugger_CPULogBufferSize);

    m_EnabledChk.SetCheck(bLoggingEnabled);

    m_BuffSizeEdit.SetDisplayType(CEditNumber32::DisplayDec);
    m_BuffSizeEdit.SetValue(bufferSize);
    m_BuffSizeEdit.EnableWindow(!bLoggingEnabled);

    RefreshList(true);

    m_ExportBtn.EnableWindow(false);

    if (!bLoggingEnabled && m_CPULogCopy != nullptr)
    {
        m_ExportBtn.EnableWindow(true);
    }

    _this = this;
    DWORD dwThreadID = ::GetCurrentThreadId();
    hWinMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)HookProc, nullptr, dwThreadID);

    LoadWindowPos();
    WindowCreated();
    return TRUE;
}

LRESULT CDebugCPULogView::OnDestroy(void)
{
    UnhookWindowsHookEx(hWinMessageHook);

    m_CPUListView.Detach();
    m_StateInfoEdit.Detach();
    m_EnabledChk.Detach();
    m_BuffSizeEdit.Detach();
    m_Scrollbar.Detach();
    m_ExportBtn.Detach();

    if (m_CPULogCopy != nullptr)
    {
        delete m_CPULogCopy;
        m_CPULogCopy = nullptr;
    }
    return 0;
}

LRESULT CALLBACK CDebugCPULogView::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MSG * pMsg = (MSG *)lParam;

    switch (pMsg->message)
    {
    case WM_MOUSEWHEEL:
        _this->InterceptMouseWheel(pMsg->wParam, pMsg->lParam);
        break;
    }

    if (nCode < 0)
    {
        return CallNextHookEx(hWinMessageHook, nCode, wParam, lParam);
    }

    return 0;
}

LRESULT CDebugCPULogView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL & /*bHandled*/)
{
    switch (wID)
    {
    case IDOK:
        EndDialog(0);
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_CHK_ENABLE:
        ToggleLoggingEnabled();
        break;
    case IDC_BTN_EXPORT:
        Export();
        break;
    }

    return FALSE;
}

LRESULT CDebugCPULogView::OnActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    return FALSE;
}

LRESULT CDebugCPULogView::OnListItemChanged(NMHDR * pNMHDR)
{
    NMITEMACTIVATE * pIA = reinterpret_cast<NMITEMACTIVATE *>(pNMHDR);
    int nItem = pIA->iItem;

    ShowRegStates(m_LogStartIndex + nItem);

    return FALSE;
}

LRESULT CDebugCPULogView::OnListDblClicked(NMHDR * pNMHDR)
{
    NMITEMACTIVATE * pIA = reinterpret_cast<NMITEMACTIVATE *>(pNMHDR);
    int nItem = pIA->iItem;

    CPUState * state = m_CPULogCopy->GetEntry(m_LogStartIndex + nItem);

    if (state == nullptr)
    {
        return FALSE;
    }

    m_Debugger->Debug_ShowCommandsLocation(state->pc, true);

    return FALSE;
}

LRESULT CDebugCPULogView::OnMeasureItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/)
{
    if (wParam == IDC_CPU_LIST)
    {
        CClientDC dc(m_hWnd);
        dc.SelectFont(GetFont());
        TEXTMETRIC tm;
        dc.GetTextMetrics(&tm);

        m_RowHeight = tm.tmHeight + tm.tmExternalLeading;

        MEASUREITEMSTRUCT * lpMeasureItem = (MEASUREITEMSTRUCT *)lParam;
        lpMeasureItem->itemHeight = m_RowHeight;
    }
    return FALSE;
}

LRESULT CDebugCPULogView::OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/)
{
    WORD type = LOWORD(wParam);
    HWND hScrollbar = (HWND)lParam;
    WORD scrlId = (WORD)::GetDlgCtrlID(hScrollbar);

    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;

    ::GetScrollInfo(hScrollbar, SB_CTL, &scrollInfo);

    int newPos;

    switch (type)
    {
    case SB_LINEUP: newPos = max(scrollInfo.nMin, scrollInfo.nPos - 1); break;
    case SB_LINEDOWN: newPos = min(scrollInfo.nMax, scrollInfo.nPos + 1); break;
    case SB_THUMBTRACK: newPos = scrollInfo.nTrackPos; break;
    default: return 0;
    }

    m_LogStartIndex = newPos;
    ::SetScrollPos(hScrollbar, SB_CTL, newPos, TRUE);

    if (scrlId == IDC_SCRL_BAR)
    {
        RefreshList(false);
    }

    return 0;
}

void CDebugCPULogView::InterceptMouseWheel(WPARAM wParam, LPARAM /*lParam*/)
{
    int nScroll = -((short)HIWORD(wParam) / WHEEL_DELTA);

    if (MouseHovering(IDC_CPU_LIST) || MouseHovering(IDC_SCRL_BAR))
    {
        int scrlMin, scrlMax;
        m_Scrollbar.GetScrollRange(&scrlMin, &scrlMax);

        int scrollPos = m_Scrollbar.GetScrollPos();
        int newPos = scrollPos + nScroll;

        if (newPos < scrlMin)
        {
            newPos = scrlMin;
        }
        else if (newPos > scrlMax)
        {
            newPos = scrlMax;
        }

        m_LogStartIndex = newPos;
        m_Scrollbar.SetScrollPos(newPos, true);

        RefreshList(false);
    }
}

void CDebugCPULogView::OnExitSizeMove(void)
{
    RefreshList(false);
    SaveWindowPos(true);
}

void CDebugCPULogView::ToggleLoggingEnabled(void)
{
    bool bEnableLogging = (m_EnabledChk.GetCheck() == BST_CHECKED);

    m_BuffSizeEdit.EnableWindow(!bEnableLogging);

    g_Settings->SaveBool(Debugger_CPULoggingEnabled, bEnableLogging);

    if (bEnableLogging)
    {
        uint32_t newSize = m_BuffSizeEdit.GetValue();
        g_Settings->SaveDword(Debugger_CPULogBufferSize, newSize);
        m_Debugger->CPULog()->Reset();
        m_ExportBtn.EnableWindow(false);
    }
    else
    {
        RefreshList(true);
        m_ExportBtn.EnableWindow(m_CPULogCopy != nullptr);
    }
}

void CDebugCPULogView::RefreshList(bool bUpdateBuffer)
{
    if (bUpdateBuffer)
    {
        if (m_CPULogCopy != nullptr)
        {
            delete m_CPULogCopy;
        }

        m_CPULogCopy = m_Debugger->CPULog()->Clone();
    }

    if (m_CPULogCopy == nullptr)
    {
        return;
    }

    size_t count = m_CPULogCopy->GetCount();
    size_t numVisibleRows = GetNumVisibleRows(m_CPUListView);

    bool bCanDisplayAll = (numVisibleRows >= count);
    int scrollRangeMax = bCanDisplayAll ? 0 : (int)((INT_PTR)(count - numVisibleRows));

    m_Scrollbar.SetScrollRange(0, scrollRangeMax, false);
    m_Scrollbar.EnableWindow(!bCanDisplayAll);

    if (bUpdateBuffer)
    {
        m_LogStartIndex = scrollRangeMax;
        m_Scrollbar.SetScrollPos(m_LogStartIndex, true);
        ShowRegStates(count - 1);
    }

    size_t start = m_Scrollbar.GetScrollPos();
    size_t end = start + numVisibleRows;

    if (end > count)
    {
        end = count;
    }

    m_CPUListView.SetRedraw(FALSE);
    m_CPUListView.DeleteAllItems();

    int nItem = 0;

    for (size_t i = start; i < end; i++)
    {
        CPUState * state = m_CPULogCopy->GetEntry(i);

        if (state == nullptr)
        {
            break;
        }

        char szPC[9];
        sprintf(szPC, "%08X", state->pc);
        R4300iInstruction Instruction(state->pc, state->opcode.Value);

        m_CPUListView.AddItem(nItem, 0, stdstr(szPC).ToUTF16().c_str());
        m_CPUListView.AddItem(nItem, 1, stdstr(Instruction.Name()).ToUTF16().c_str());
        m_CPUListView.AddItem(nItem, 2, stdstr(Instruction.Param()).ToUTF16().c_str());
        nItem++;
    }

    m_CPUListView.SetRedraw(TRUE);
}

void CDebugCPULogView::ShowRegStates(size_t stateIndex)
{
    CPUState * state = m_CPULogCopy->GetEntry(stateIndex);

    if (state == nullptr)
    {
        return;
    }

    char szRegStates[2048];
    char * out = szRegStates;

    out += sprintf(out, "PC: %08X\r\n\r\n", state->pc);

    for (int i = 0; i < 16; i++)
    {
        int regl = i, regr = i + 16;
        out += sprintf(out, "%s: %08X %08X  %s: %08X %08X\r\n",
                       CRegName::GPR[regl], state->gpr[regl].UW[1], state->gpr[regl].UW[0],
                       CRegName::GPR[regr], state->gpr[regr].UW[1], state->gpr[regr].UW[0]);
    }

    out += sprintf(out, "HI: %08X %08X  LO: %08X %08X\r\n\r\n",
                   state->gprHi.UW[1], state->gprHi.UW[0],
                   state->gprLo.UW[1], state->gprLo.UW[0]);

    for (int i = 0; i < 16; i++)
    {
        int regl = i, regr = i + 16;
        out += sprintf(out, "%-3s: %08X  %-3s: %08X\r\n",
                       CRegName::FPR[regl], *(uint32_t *)&state->fpr[regl],
                       CRegName::FPR[regr], *(uint32_t *)&state->fpr[regr]);
    }

    out += sprintf(out, "FPCR: %08X\r\n", state->fpcr);

    m_StateInfoEdit.SetWindowText(stdstr(szRegStates).ToUTF16().c_str());
}

void CDebugCPULogView::Export(void)
{
    if (m_CPULogCopy == nullptr)
    {
        return;
    }

    OPENFILENAMEA openfilename;
    char filePath[255];

    memset(&filePath, 0, sizeof(filePath));
    memset(&openfilename, 0, sizeof(openfilename));

    sprintf(filePath, "CPULOG.txt");

    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = (HWND)m_hWnd;
    openfilename.lpstrFilter = "CPU Log (*.*)\0*.*;\0";
    openfilename.lpstrFile = filePath;
    openfilename.lpstrInitialDir = "Logs";
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_HIDEREADONLY;

    if (GetSaveFileNameA(&openfilename))
    {
        m_CPULogCopy->DumpToFile(filePath);
    }
}

// Utility

int CDebugCPULogView::GetNumVisibleRows(CListViewCtrl & list)
{
    CHeaderCtrl header = list.GetHeader();
    CRect listRect, headRect;
    list.GetWindowRect(&listRect);
    header.GetWindowRect(&headRect);
    int innerHeight = listRect.Height() - headRect.Height();
    return (innerHeight / m_RowHeight);
}

bool CDebugCPULogView::MouseHovering(WORD ctrlId, int xMargin, int yMargin)
{
    CRect rect;
    POINT pointerPos;

    ::GetWindowRect(GetDlgItem(ctrlId), &rect);
    ::GetCursorPos(&pointerPos);

    return (
        pointerPos.x >= rect.left - xMargin && pointerPos.x <= rect.right + xMargin && pointerPos.y >= rect.top - yMargin && pointerPos.y <= rect.bottom + yMargin);
}
