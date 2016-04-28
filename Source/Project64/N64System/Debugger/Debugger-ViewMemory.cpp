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

CDebugMemoryView::CDebugMemoryView(CDebuggerUI * debugger) :
CDebugDialog<CDebugMemoryView>(debugger),
m_MemoryList(NULL)
{
    if (m_MemoryList == NULL)
    {
        m_MemoryList = new CListCtrl;
        m_MemoryList->RegisterClass();
    }
}

CDebugMemoryView::~CDebugMemoryView()
{
}

LRESULT	CDebugMemoryView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_DataStartLoc = (DWORD)-1;
    m_CompareStartLoc = (DWORD)-1;
    memset(m_CompareData, 0, sizeof(m_CompareData));
    memset(m_CompareValid, 0, sizeof(m_CompareValid));

    HWND hScrlBar = GetDlgItem(IDC_SCRL_BAR);
    if (hScrlBar)
    {
        SCROLLINFO si;

        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        si.nMin = 0;
        si.nMax = 0xFFFF;
        si.nPos = 0x8000;
        si.nPage = 100;
        ::SetScrollInfo(hScrlBar, SB_CTL, &si, TRUE);
    }

    m_MemAddr.Attach(GetDlgItem(IDC_ADDR_EDIT));
    m_MemAddr.SetDisplayType(CEditNumber::DisplayHex);
    m_MemAddr.SetValue(0x80000000, true, true);

    SendDlgItemMessage(IDC_CHK_VADDR, BM_SETCHECK, BST_CHECKED, 0);

    if (m_MemoryList == NULL)
    {
        m_MemoryList = new CListCtrl;
        m_MemoryList->RegisterClass();
    }
    m_MemoryList->SubclassWindow(GetDlgItem(IDC_MEM_DETAILS));
    m_MemoryList->ShowHeader(false);
    m_MemoryList->SetSortEnabled(FALSE);
    m_MemoryList->AddColumn(_T("Address"), 90);
    m_MemoryList->AddColumn(_T("1"), 20);
    m_MemoryList->AddColumn(_T("2"), 20);
    m_MemoryList->AddColumn(_T("3"), 20);
    m_MemoryList->AddColumn(_T("4"), 20);
    m_MemoryList->AddColumn(_T("-"), 10);
    m_MemoryList->AddColumn(_T("5"), 20);
    m_MemoryList->AddColumn(_T("6"), 20);
    m_MemoryList->AddColumn(_T("7"), 20);
    m_MemoryList->AddColumn(_T("8"), 20);
    m_MemoryList->AddColumn(_T("-"), 10);
    m_MemoryList->AddColumn(_T("9"), 20);
    m_MemoryList->AddColumn(_T("10"), 20);
    m_MemoryList->AddColumn(_T("11"), 20);
    m_MemoryList->AddColumn(_T("12"), 20);
    m_MemoryList->AddColumn(_T("-"), 10);
    m_MemoryList->AddColumn(_T("13"), 20);
    m_MemoryList->AddColumn(_T("14"), 20);
    m_MemoryList->AddColumn(_T("15"), 20);
    m_MemoryList->AddColumn(_T("16"), 35);
    m_MemoryList->AddColumn(_T("Memory Ascii"), 140);
    ::SetWindowLongPtr(m_MemoryList->m_hWnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE);
    RefreshMemory(false);
    int height = m_MemoryList->GetTotalHeight();

    RECT MemoryListRect = { 0 };
    ::GetClientRect(GetDlgItem(IDC_MEM_DETAILS), &MemoryListRect);

    if (height > MemoryListRect.bottom)
    {
        RECT MemoryListWindow = { 0 };
        GetWindowRect(&MemoryListWindow);
        SetWindowPos(NULL, 0, 0, MemoryListWindow.right - MemoryListWindow.left, (MemoryListWindow.bottom - MemoryListWindow.top) + (height - MemoryListRect.bottom), SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOZORDER);

        RECT DlgItemRect = { 0 };
        ::GetWindowRect(GetDlgItem(IDC_BORDER), &DlgItemRect);
        ::SetWindowPos(GetDlgItem(IDC_BORDER), NULL, 0, 0, DlgItemRect.right - DlgItemRect.left, (DlgItemRect.bottom - DlgItemRect.top) + (height - MemoryListRect.bottom), SWP_NOMOVE);

        ::GetWindowRect(GetDlgItem(IDC_MEM_DETAILS), &DlgItemRect);
        ::SetWindowPos(GetDlgItem(IDC_MEM_DETAILS), NULL, 0, 0, DlgItemRect.right - DlgItemRect.left, (DlgItemRect.bottom - DlgItemRect.top) + (height - MemoryListRect.bottom), SWP_NOMOVE);

        ::GetWindowRect(GetDlgItem(IDC_SCRL_BAR), &DlgItemRect);
        ::SetWindowPos(GetDlgItem(IDC_SCRL_BAR), NULL, 0, 0, DlgItemRect.right - DlgItemRect.left, (DlgItemRect.bottom - DlgItemRect.top) + (height - MemoryListRect.bottom), SWP_NOMOVE);
    }
    WindowCreated();
    return TRUE;
}

LRESULT CDebugMemoryView::OnDestroy(void)
{
    if (m_MemoryList)
    {
        m_MemoryList->UnsubclassWindow();
        delete m_MemoryList;
        m_MemoryList = NULL;
    }
    return 0;
}

LRESULT CDebugMemoryView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDC_REFRSH_MEM:
        RefreshMemory(true);
        break;
    case IDC_CHK_VADDR:
        RefreshMemory(false);
        break;
    case IDC_DUMP_MEM:
        m_Debugger->Debug_ShowMemoryDump();
        break;
    case IDC_SEARCH_MEM:
        m_Debugger->Debug_ShowMemorySearch();
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    }
    return FALSE;
}

LRESULT CDebugMemoryView::OnMemoryModified(LPNMHDR lpNMHDR)
{
    CListNotify *pListNotify = reinterpret_cast<CListNotify *>(lpNMHDR);
    int LineNumber = pListNotify->m_nItem;
    int Pos = ((LineNumber << 4) + (pListNotify->m_nSubItem - 1));
    if (pListNotify->m_nSubItem >= 6 && pListNotify->m_nSubItem < 10)
    {
        Pos = ((LineNumber << 4) + (pListNotify->m_nSubItem - 2));
    }
    if (pListNotify->m_nSubItem >= 11 && pListNotify->m_nSubItem < 15)
    {
        Pos = ((LineNumber << 4) + (pListNotify->m_nSubItem - 3));
    }
    if (pListNotify->m_nSubItem >= 16 && pListNotify->m_nSubItem < 20)
    {
        Pos = ((LineNumber << 4) + (pListNotify->m_nSubItem - 4));
    }

    LPCSTR strValue = m_MemoryList->GetItemText(pListNotify->m_nItem, pListNotify->m_nSubItem);
    int Finish = strlen(strValue);
    if (Finish > 8)
    {
        Finish = 8;
    }
    DWORD Value = 0;
    for (int i = 0; i < Finish; i++)
    {
        Value = (Value << 4);
        if (strValue[i] <= '9' && strValue[i] >= '0')
        {
            Value |= strValue[i] - '0';
        }
        else if (strValue[i] <= 'f' && strValue[i] >= 'a')
        {
            Value |= strValue[i] - 'a' + 10;
        }
        else if (strValue[i] <= 'F' && strValue[i] >= 'A')
        {
            Value |= strValue[i] - 'A' + 10;
        }
    }

    if (m_CurrentData[Pos] == Value)
    {
        return 0;
    }

    if (m_CompareStartLoc != m_DataStartLoc ||
        m_CompareVAddrr != m_DataVAddrr)
    {
        // copy current data for change comparison
        m_CompareStartLoc = m_DataStartLoc;
        m_CompareVAddrr = m_DataVAddrr;
        memcpy(m_CompareData, m_CurrentData, sizeof(m_CurrentData));
        memcpy(m_CompareValid, m_DataValid, sizeof(m_CompareValid));
    }

    m_CompareData[Pos] = m_CurrentData[Pos];
    m_CurrentData[Pos] = (BYTE)Value;

    //sb
    if (m_DataVAddrr)
    {
        if (!g_MMU->SB_VAddr(m_DataStartLoc + Pos, (BYTE)Value))
        {
            WriteTrace(TraceUserInterface, TraceError, "failed to store at %X", m_DataStartLoc + Pos);
        }
    }
    else
    {
        if (!g_MMU->SB_PAddr(m_DataStartLoc + Pos, (BYTE)Value))
        {
            WriteTrace(TraceUserInterface, TraceError, "failed to store at %X", m_DataStartLoc + Pos);
        }
    }
    Insert_MemoryLineDump(LineNumber);

    return 0;
}

void CDebugMemoryView::ShowAddress(DWORD Address, bool VAddr)
{
    if (m_hWnd == NULL)
    {
        return;
    }

    SendDlgItemMessage(IDC_CHK_VADDR, BM_SETCHECK, VAddr ? BST_CHECKED : BST_UNCHECKED, 0);
    m_MemAddr.SetValue(Address, true, true);
    RefreshMemory(true);
}

void CDebugMemoryView::Insert_MemoryLineDump(int LineNumber)
{
    if (m_MemoryList == NULL || m_MemoryList->GetColumnCount() == 0)
    {
        return;
    }
    char Output[20], Hex[60], Ascii[20], AsciiAddOn[15];
    sprintf(Output, "0x%08X", m_DataStartLoc + (LineNumber << 4));
    if (m_MemoryList->GetItemCount() <= LineNumber)
    {
        HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
        m_MemoryList->AddItemAt(LineNumber, Output);
        for (int i = 0; i < m_MemoryList->GetColumnCount(); i++)
        {
            m_MemoryList->SetItemFont(LineNumber, i, hFont);
            if (i == 5 || i == 10 || i == 15)
            {
                m_MemoryList->SetItemText(LineNumber, i, "-");
            }
        }
    }
    else
    {
        if (strcmp(Output, m_MemoryList->GetItemText(LineNumber, 0)) != 0)
        {
            m_MemoryList->SetItemText(LineNumber, 0, Output);
        }
    }

    Hex[0] = 0;
    Ascii[0] = 0;
    int CompareStartPos = m_DataStartLoc - m_CompareStartLoc;

    for (int i = 0, col = 1; i < 0x10; i++, col++)
    {
        int Pos = ((LineNumber << 4) + i);
        if (m_DataValid[Pos])
        {
            int ComparePos = CompareStartPos + Pos;
            bool Changed = false;

            if (ComparePos >= 0 && ComparePos < MemoryToDisplay &&
                m_DataVAddrr == m_CompareVAddrr &&
                m_DataValid[ComparePos] &&
                m_CurrentData[Pos] != m_CompareData[ComparePos])
            {
                Changed = true;
            }
            sprintf(Hex, "%02X", m_CurrentData[Pos]);
            m_MemoryList->SetItemText(LineNumber, col, Hex);
            m_MemoryList->SetItemFormat(LineNumber, col, ITEM_FORMAT_EDIT, ITEM_FLAGS_EDIT_HEX);
            m_MemoryList->SetItemMaxEditLen(LineNumber, col, 2);
            m_MemoryList->SetItemColours(LineNumber, col, GetSysColor(COLOR_WINDOW),
                Changed ? RGB(255, 0, 0) : GetSysColor(COLOR_WINDOWTEXT));
            m_MemoryList->SetItemHighlightColours(LineNumber, col,
                Changed ? RGB(255, 0, 0) : GetSysColor(COLOR_HIGHLIGHTTEXT));
            if (m_CurrentData[Pos] < 30)
            {
                strcat(Ascii, ".");
            }
            else
            {
                sprintf(AsciiAddOn, "%c", m_CurrentData[Pos]);
                strcat(Ascii, AsciiAddOn);
            }
        }
        else
        {
            m_MemoryList->SetItemText(LineNumber, col, "**");
            m_MemoryList->SetItemFormat(LineNumber, col, ITEM_FORMAT_NONE, ITEM_FLAGS_NONE);
            m_MemoryList->SetItemColours(LineNumber, col, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOWTEXT));
            strcat(Ascii, "*");
        }
        if (i != 0xF)
        {
            if ((i & 3) == 3)
            {
                col += 1;
            }
        }
    }

    if (strcmp(Ascii, m_MemoryList->GetItemText(LineNumber, 20)) != 0)
    {
        m_MemoryList->SetItemText(LineNumber, 20, Ascii);
    }
}

void CDebugMemoryView::OnAddrChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    RefreshMemory(false);
}

void CDebugMemoryView::OnVScroll(int request, short Pos, HWND ctrl)
{
    if (ctrl != GetDlgItem(IDC_SCRL_BAR))
    {
        return;
    }
    DWORD Location = m_MemAddr.GetValue();
    switch (request)
    {
    case SB_LINEDOWN:
        m_MemAddr.SetValue(Location < 0xFFFFFFEF ? Location + 0x10 : 0xFFFFFFFF, true, true);
        break;
    case SB_LINEUP:
        m_MemAddr.SetValue(Location > 0x10 ? Location - 0x10 : 0, true, true);
        break;
    case SB_PAGEDOWN:
        m_MemAddr.SetValue(Location < 0xFFFFFEFF ? Location + 0x100 : 0xFFFFFFFF, true, true);
        break;
    case SB_PAGEUP:
        m_MemAddr.SetValue(Location > 0x100 ? Location - 0x100 : 0, true, true);
        break;
    case SB_THUMBPOSITION:
        m_MemAddr.SetValue((DWORD)Pos << 0x10, true, true);
        break;
    default:
        break;
    }
}

void CDebugMemoryView::RefreshMemory(bool ResetCompare)
{
    if (m_MemoryList && m_MemoryList->GetHasEditItem())
    {
        m_MemoryList->SetFocus();
    }

    DWORD NewAddress = m_MemAddr.GetValue();
    if (NewAddress != m_DataStartLoc)
    {
        HWND hScrlBar = GetDlgItem(IDC_SCRL_BAR);
        if (hScrlBar)
        {
            SCROLLINFO si;

            si.cbSize = sizeof(si);
            si.fMask = SIF_POS;
            si.nPos = NewAddress >> 0x10;
            ::SetScrollInfo(hScrlBar, SB_CTL, &si, TRUE);
        }
    }

    if (ResetCompare)
    {
        // copy current data for change comparison
        m_CompareStartLoc = m_DataStartLoc;
        m_CompareVAddrr = m_DataVAddrr;
        memcpy(m_CompareData, m_CurrentData, sizeof(m_CurrentData));
        memcpy(m_CompareValid, m_DataValid, sizeof(m_CompareValid));
    }

    m_DataStartLoc = m_MemAddr.GetValue();
    if (m_DataStartLoc > 0xFFFFFF00) { m_DataStartLoc = 0xFFFFFF00; }
    int WritePos = 0;

    m_DataVAddrr = (SendDlgItemMessage(IDC_CHK_VADDR, BM_GETCHECK, 0, 0) & BST_CHECKED) != 0;

    if ((m_DataStartLoc & 3) != 0)
    {
        MIPS_WORD word;
        bool ValidData = true;

        if (m_DataVAddrr)
        {
            if (!g_MMU->LW_VAddr(m_DataStartLoc & ~3, word.UW))
            {
                ValidData = false;
            }
        }
        else
        {
            if (!g_MMU->LW_PAddr(m_DataStartLoc & ~3, word.UW))
            {
                ValidData = false;
            }
        }

        int Offset = (m_DataStartLoc & 3);
        for (int i = 0; i < (4 - Offset); i++)
        {
            if (WritePos >= MemoryToDisplay)
            {
                break;
            }
            m_DataValid[WritePos + i] = ValidData;
            if (ValidData)
            {
                m_CurrentData[WritePos + i] = word.UB[3 - (i + Offset)];
            }
        }
        WritePos = 4 - Offset;
    }

    for (DWORD Pos = ((m_DataStartLoc + 3) & ~3); Pos < (m_DataStartLoc + MemoryToDisplay); WritePos += 4, Pos += 4)
    {
        MIPS_WORD word;
        bool ValidData = true;

        if (m_DataVAddrr)
        {
            if (!g_MMU->LW_VAddr(Pos, word.UW))
            {
                ValidData = false;
            }
        }
        else
        {
            if (!g_MMU->LW_PAddr(Pos, word.UW))
            {
                ValidData = false;
            }
        }

        for (int i = 0; i < 4; i++)
        {
            if ((WritePos + i) >= MemoryToDisplay)
            {
                break;
            }
            m_DataValid[WritePos + i] = ValidData;
            if (ValidData)
            {
                m_CurrentData[WritePos + i] = word.UB[3 - i];
            }
        }
    }

    for (int count = 0; count < 16; count++)
    {
        Insert_MemoryLineDump(count);
    }
}