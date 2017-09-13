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

CDebugMemorySearch::CDebugMemorySearch(CDebuggerUI * debugger) :
    CDebugDialog<CDebugMemorySearch>(debugger),
    m_MemoryState(NULL),
    m_MemoryStateSize(0)
{
}

CDebugMemorySearch::~CDebugMemorySearch()
{
    if (m_MemoryState)
    {
        delete m_MemoryState;
    }
}

void CDebugMemorySearch::AddAlignmentOptions(CComboBox  & ctrl)
{
    int Index = ctrl.AddString("32 bits (aligned)");
    ctrl.SetItemData(Index, _32Bit);
    Index = ctrl.AddString("16bits (aligned)");
    ctrl.SetItemData(Index, _16Bit);
    Index = ctrl.AddString("8bits");
    ctrl.SetCurSel(Index);
    ctrl.SetItemData(Index, _8Bit);
}

LRESULT	CDebugMemorySearch::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_PAddrStart.Attach(GetDlgItem(IDC_PADDR_START));
    m_PAddrStart.SetDisplayType(CEditNumber::DisplayHex);
    m_SearchLen.Attach(GetDlgItem(IDC_ADDR_END));
    m_SearchLen.SetDisplayType(CEditNumber::DisplayHex);
    m_SearchValue.Attach(GetDlgItem(IDC_SEARCH_VALUE));
    m_SearchValue.SetDisplayType(CEditNumber::DisplayDec);
    m_SearchValue.SetValue(0);
    m_MaxSearch.Attach(GetDlgItem(IDC_MAX_SEARCH));
    m_MaxSearch.SetDisplayType(CEditNumber::DisplayDec);
    m_MaxSearch.SetValue(50000);
    m_UnknownSize.Attach(GetDlgItem(IDC_UNKNOWN_ALIGN));
    AddAlignmentOptions(m_UnknownSize);
    m_ValueSize.Attach(GetDlgItem(IDC_VALUE_ALIGN));
    AddAlignmentOptions(m_ValueSize);
    m_SearchResults.Attach(GetDlgItem(IDC_LST_RESULTS));
    m_SearchResults.AddColumn("ID", 0);
    m_SearchResults.AddColumn("PAddr", 1);
    m_SearchResults.AddColumn("Value", 2);
    m_SearchResults.SetColumnWidth(0, 50);
    m_SearchResults.SetColumnWidth(1, 75);
    m_SearchResults.SetColumnWidth(2, 75);
    m_SearchResults.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
    m_UnknownOptions.Attach(GetDlgItem(IDC_CMB_UNKNOWN));
    m_HaveResults = false;
    FixUnknownOptions(true);

    SendMessage(GetDlgItem(IDC_RADIO_VALUE), BM_SETCHECK, BST_CHECKED, 0);

    BOOL bHandled;
    OnClicked(0, IDC_BTN_RDRAM, NULL, bHandled);
    OnClicked(0, IDC_RADIO_VALUE, NULL, bHandled);
    WindowCreated();
    return TRUE;
}

LRESULT	CDebugMemorySearch::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_BTN_RDRAM:
        m_PAddrStart.SetValue(0, true, true);
        m_SearchLen.SetValue(g_MMU->RdramSize(), true, true);
        break;
    case IDC_BTN_ROM:
        m_PAddrStart.SetValue(0x10000000, true, true);
        m_SearchLen.SetValue(g_Rom->GetRomSize(), true, true);
        break;
    case IDC_BTN_SPMEM:
        m_PAddrStart.SetValue(0x04000000, true, true);
        m_SearchLen.SetValue(0x2000, true, true);
        break;
    case IDC_SEARCH_HEX:
        {
            bool bChecked = (SendMessage(hWndCtl, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
            m_SearchValue.SetDisplayType(bChecked ? CEditNumber::DisplayHex : CEditNumber::DisplayDec);
        }
        break;
    case ID_POPUP_SHOWINMEMORYVIEWER:
        {
            LONG iItem = m_SearchResults.GetNextItem(-1, LVNI_SELECTED);
            if (iItem == -1)
            {
                break;
            }

            int ItemId = m_SearchResults.GetItemData(iItem);
            SearchResultItem & Result = m_SearchResult[ItemId];
            m_Debugger->Debug_ShowMemoryLocation(Result.PAddr, false);
        }
        break;
    case IDC_RADIO_UNKNOWN:
        EnableUnknownOptions(true);
        EnableValueOptions(false);
        EnableTextOptions(false);
        EnableJalOptions(false);
        break;
    case IDC_RADIO_VALUE:
        EnableUnknownOptions(false);
        EnableValueOptions(true);
        EnableTextOptions(false);
        EnableJalOptions(false);
        break;
    case IDC_RADIO_TEXT:
        EnableUnknownOptions(false);
        EnableValueOptions(false);
        EnableTextOptions(true);
        EnableJalOptions(false);
        break;
    case IDC_RADIO_JAL:
        EnableUnknownOptions(false);
        EnableValueOptions(false);
        EnableTextOptions(false);
        EnableJalOptions(true);
        break;
    case IDC_BTN_SEARCH:
        if (SendMessage(GetDlgItem(IDC_RADIO_UNKNOWN), BM_GETSTATE, 0, 0) == BST_CHECKED)
        {
            g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_SearchMemory);
            SearchForUnknown();
            g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_SearchMemory);
            break;
        }
        if (SendMessage(GetDlgItem(IDC_RADIO_VALUE), BM_GETSTATE, 0, 0) == BST_CHECKED)
        {
            g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_SearchMemory);
            SearchForValue();
            g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_SearchMemory);
            break;
        }
        if (SendMessage(GetDlgItem(IDC_RADIO_TEXT), BM_GETSTATE, 0, 0) == BST_CHECKED)
        {
            g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_SearchMemory);
            SearchForText();
            g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_SearchMemory);
            break;
        }
    case IDC_RESET_BUTTON:
        Reset();
        break;
    }
    return FALSE;
}

LRESULT CDebugMemorySearch::OnResultRClick(LPNMHDR /*lpnmh*/)
{
    LONG iItem = m_SearchResults.GetNextItem(-1, LVNI_SELECTED);
    if (iItem == -1)
    {
        return true;
    }

    //Load the menu
    HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MEM_SEARCH));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);

    //Get the current Mouse location
    POINT Mouse;
    GetCursorPos(&Mouse);

    //Show the menu
    TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);
    return true;
}

LRESULT CDebugMemorySearch::OnResultDblClick(LPNMHDR)
{
	LONG iItem = m_SearchResults.GetNextItem(-1, LVNI_SELECTED);
	if (iItem == -1)
	{
		return true;
	}

	// view in memory
	int ItemId = m_SearchResults.GetItemData(iItem);
	SearchResultItem & Result = m_SearchResult[ItemId];
	m_Debugger->Debug_ShowMemoryLocation(Result.PAddr, false);
    return true;
}

void CDebugMemorySearch::EnableValueOptions(bool Enable)
{
    if (Enable)
    {
        ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), m_HaveResults ? "Search Results" : "Search");
    }
    ::EnableWindow(GetDlgItem(IDC_SEARCH_VALUE), Enable);
    ::EnableWindow(GetDlgItem(IDC_SEARCH_HEX), Enable);
    ::EnableWindow(GetDlgItem(IDC_VALUE_ALIGN), m_HaveResults ? false : Enable);
}

void CDebugMemorySearch::EnableTextOptions(bool Enable)
{
    if (Enable)
    {
        ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), m_HaveResults ? "Search Results" : "Search");
    }
    ::EnableWindow(GetDlgItem(IDC_SEARCH_TEXT), Enable);
    ::EnableWindow(GetDlgItem(IDC_CASE_SENSITIVE), Enable);
}

void CDebugMemorySearch::EnableJalOptions(bool Enable)
{
    if (Enable)
    {
        ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), m_HaveResults ? "Search Results" : "Search");
    }
    ::EnableWindow(GetDlgItem(IDC_JAL_ADDR), Enable);
}

void CDebugMemorySearch::EnableUnknownOptions(bool Enable)
{
    if (m_UnknownOptions.GetCount() > 1)
    {
        ::EnableWindow(GetDlgItem(IDC_UNKNOWN_ALIGN), m_HaveResults ? false : Enable);
        if (Enable)
        {
            ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), m_HaveResults ? "Search Results" : "Search");
        }
    }
    else
    {
        ::EnableWindow(GetDlgItem(IDC_UNKNOWN_ALIGN), false);
        if (Enable)
        {
            ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), "Create");
        }
    }
    ::EnableWindow(GetDlgItem(IDC_CMB_UNKNOWN), Enable);
}

void CDebugMemorySearch::SearchForValue(void)
{
    MemorySize Size = (MemorySize)m_ValueSize.GetItemData(m_ValueSize.GetCurSel());
    DWORD Value = m_SearchValue.GetValue();
    DWORD StartAddress = m_PAddrStart.GetValue();
    DWORD Len = m_SearchLen.GetValue();
    DWORD MaxSearch = m_MaxSearch.GetValue();

    DWORD MoveSize = (Size == _32Bit ? 4 : (Size == _16Bit ? 2 : 1));

    m_UnknownSize.SetCurSel(m_ValueSize.GetCurSel());

    LPCTSTR DisplayStr = "0x%08X";
    if (Size == _16Bit)
    {
        DisplayStr = "0x%04X";
    }
    else if (Size == _8Bit)
    {
        DisplayStr = "0x%04X";
    }

	m_SearchResults.SetRedraw(FALSE);

    if (!m_HaveResults)
    {
        m_HaveResults = true;

        FixUnknownOptions(false);
        m_SearchResults.DeleteAllItems();
        DWORD ItemsAdded = 0;
		
        while (SearchForValue(Value, Size, StartAddress, Len))
        {
            SearchResultItem Result;
            Result.PAddr = StartAddress;
            Result.Value = Value;

            char LocationStr[20];
            sprintf(LocationStr, "%d", ItemsAdded + 1);
            int Index = m_SearchResults.AddItem(ItemsAdded, 0, LocationStr);
            m_SearchResults.SetItemData(Index, m_SearchResult.size());
            m_SearchResult.push_back(Result);
            sprintf(LocationStr, "0x%08X", StartAddress);
            m_SearchResults.SetItemText(Index, 1, LocationStr);
            sprintf(LocationStr, DisplayStr, Value);
            m_SearchResults.SetItemText(Index, 2, LocationStr);
            sprintf(LocationStr, DisplayStr, Value);
            m_SearchResults.SetItemText(Index, 3, LocationStr);
            StartAddress += MoveSize;
            Len -= MoveSize;
            ItemsAdded += 1;
            if (ItemsAdded >= MaxSearch)
            {
                break;
            }
        }
		
        ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), "Search Results");
        ::ShowWindow(GetDlgItem(IDC_RESET_BUTTON), SW_SHOW);
        ::EnableWindow(GetDlgItem(IDC_VALUE_ALIGN), false);
    }
    else
    {
        int ItemCount = m_SearchResults.GetItemCount();
        for (int i = ItemCount - 1; i >= 0; i--)
        {
            int ItemId = m_SearchResults.GetItemData(i);
            SearchResultItem & Result = m_SearchResult[ItemId];

            uint32_t NewValue = 0;
            bool valid = false;

            switch (Size)
            {
            case _8Bit:
                {
                    BYTE mem = 0;
                    valid = g_MMU->LB_PAddr(Result.PAddr, mem);
                    NewValue = mem;
                }
                break;
            case _16Bit:
                {
                    WORD mem = 0;
                    valid = g_MMU->LH_PAddr(Result.PAddr, mem);
                    NewValue = mem;
                }
                break;
            case _32Bit:
                valid = g_MMU->LW_PAddr(Result.PAddr, NewValue);
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            if (Value == NewValue)
            {
                char LocationStr[20];
                sprintf(LocationStr, DisplayStr, NewValue);
                m_SearchResults.SetItemText(i, 2, LocationStr);
                sprintf(LocationStr, DisplayStr, Result.Value);
                m_SearchResults.SetItemText(i, 3, LocationStr);
                Result.Value = NewValue;
            }
            else
            {
                m_SearchResults.DeleteItem(i);
            }
        }
    }

	m_SearchResults.SetRedraw(TRUE);

    ::SetWindowText(GetDlgItem(IDC_BORDER_RESULTS), stdstr_f("Results (%d)", m_SearchResults.GetItemCount()).c_str());
}

void CDebugMemorySearch::SearchForUnknown()
{
    SearchMemChangeState Option = (SearchMemChangeState)m_UnknownOptions.GetItemData(m_UnknownOptions.GetCurSel());
    if (Option == SearchChangeState_Reset)
    {
        m_SearchResults.DeleteAllItems();
        SearchSetBaseForChanges();
        FixUnknownOptions(false);
        ::ShowWindow(GetDlgItem(IDC_RESET_BUTTON), SW_SHOW);
        ::EnableWindow(GetDlgItem(IDC_UNKNOWN_ALIGN), true);
        return;
    }
    MemorySize Size = (MemorySize)m_UnknownSize.GetItemData(m_UnknownSize.GetCurSel());
    m_ValueSize.SetCurSel(m_UnknownSize.GetCurSel());
    LPCTSTR DisplayStr = "0x%08X";
    if (Size == _16Bit)
    {
        DisplayStr = "0x%04X";
    }
    else if (Size == _8Bit)
    {
        DisplayStr = "0x%04X";
    }
    if (!m_HaveResults)
    {
        m_HaveResults = true;

        ::EnableWindow(GetDlgItem(IDC_UNKNOWN_ALIGN), false);
        DWORD StartAddress = m_PAddrStart.GetValue();
        DWORD Len = m_SearchLen.GetValue();
        DWORD MaxSearch = m_MaxSearch.GetValue();

        DWORD MoveSize = (Size == _32Bit ? 4 : (Size == _16Bit ? 2 : 1));

        for (int i = 2; i < 10; i++)
        {
            if (!m_SearchResults.DeleteColumn(i))
            {
                break;
            }
        }
        m_SearchResults.AddColumn("New Value", 2);
        m_SearchResults.AddColumn("Old Value", 3);
        m_SearchResults.SetColumnWidth(0, 50);
        m_SearchResults.SetColumnWidth(1, 75);
        m_SearchResults.SetColumnWidth(2, 75);
        m_SearchResults.SetColumnWidth(3, 75);

        m_SearchResults.DeleteAllItems();
        DWORD ItemsAdded = 0, OldValue, NewValue;

        while (SearchForChanges(Option, Size, StartAddress, Len, OldValue, NewValue))
        {
            SearchResultItem Result;
            Result.PAddr = StartAddress;
            Result.Value = NewValue;

            //if list size > max, then break
            char LocationStr[20];
            sprintf(LocationStr, "%d", ItemsAdded + 1);
            int Index = m_SearchResults.AddItem(ItemsAdded, 0, LocationStr);
            m_SearchResults.SetItemData(Index, m_SearchResult.size());
            m_SearchResult.push_back(Result);
            sprintf(LocationStr, "0x%08X", StartAddress);
            m_SearchResults.SetItemText(Index, 1, LocationStr);
            sprintf(LocationStr, DisplayStr, NewValue);
            m_SearchResults.SetItemText(Index, 2, LocationStr);
            sprintf(LocationStr, DisplayStr, OldValue);
            m_SearchResults.SetItemText(Index, 3, LocationStr);
            StartAddress += MoveSize;
            Len -= MoveSize;
            ItemsAdded += 1;
            if (ItemsAdded >= MaxSearch)
            {
                break;
            }
        }
        ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), "Search Results");
        ::ShowWindow(GetDlgItem(IDC_RESET_BUTTON), SW_SHOW);
        ::EnableWindow(GetDlgItem(IDC_RADIO_TEXT), false);
        ::EnableWindow(GetDlgItem(IDC_RADIO_JAL), false);
    }
    else
    {
        int ItemCount = m_SearchResults.GetItemCount();
        for (int i = ItemCount - 1; i >= 0; i--)
        {
            int ItemId = m_SearchResults.GetItemData(i);
            SearchResultItem & Result = m_SearchResult[ItemId];

            bool UpdateResult = false;
            uint32_t NewValue = 0;
            bool valid = false;

            switch (Size)
            {
            case _8Bit:
                {
                    BYTE mem = 0;
                    valid = g_MMU->LB_PAddr(Result.PAddr, mem);
                    NewValue = mem;
                }
                break;
            case _16Bit:
                {
                    WORD mem = 0;
                    valid = g_MMU->LH_PAddr(Result.PAddr, mem);
                    NewValue = mem;
                }
                break;
            case _32Bit:
                valid = g_MMU->LW_PAddr(Result.PAddr, NewValue);
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            switch (Option)
            {
            case SearchChangeState_Changed:
                if (Result.Value != NewValue)
                {
                    UpdateResult = true;
                }
                break;
            case SearchChangeState_Unchanged:
                if (Result.Value == NewValue)
                {
                    UpdateResult = true;
                }
                break;
            case SearchChangeState_Greater:
                if (NewValue > Result.Value)
                {
                    UpdateResult = true;
                }
                break;
            case SearchChangeState_Lessthan:
                if (NewValue < Result.Value)
                {
                    UpdateResult = true;
                }
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            if (UpdateResult)
            {
                char LocationStr[20];
                sprintf(LocationStr, DisplayStr, NewValue);
                m_SearchResults.SetItemText(i, 2, LocationStr);
                sprintf(LocationStr, DisplayStr, Result.Value);
                m_SearchResults.SetItemText(i, 3, LocationStr);
                Result.Value = NewValue;
            }
            else
            {
                m_SearchResults.DeleteItem(i);
            }
        }
    }
    ::SetWindowText(GetDlgItem(IDC_BORDER_RESULTS), stdstr_f("Results (%d)", m_SearchResults.GetItemCount()).c_str());
}

void CDebugMemorySearch::SearchForText()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CDebugMemorySearch::Reset(void)
{
    m_HaveResults = false;
    SendMessage(GetDlgItem(IDC_RADIO_VALUE), BM_SETCHECK, BST_CHECKED, 0);
    EnableUnknownOptions(false);
    EnableValueOptions(true);
    EnableTextOptions(false);
    EnableJalOptions(false);
    ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), "Search");
    ::SetWindowText(GetDlgItem(IDC_BORDER_RESULTS), "Results");
    ::ShowWindow(GetDlgItem(IDC_RESET_BUTTON), SW_HIDE);
    ::EnableWindow(GetDlgItem(IDC_RADIO_UNKNOWN), true);
    ::EnableWindow(GetDlgItem(IDC_RADIO_VALUE), true);
    ::EnableWindow(GetDlgItem(IDC_RADIO_TEXT), false);
    ::EnableWindow(GetDlgItem(IDC_RADIO_JAL), false);
    for (int i = 1; i < 10; i++)
    {
        if (!m_SearchResults.DeleteColumn(i))
        {
            break;
        }
    }
    m_SearchResults.AddColumn("Value", 2);
    m_SearchResults.DeleteAllItems();
    m_SearchResults.SetColumnWidth(0, 50);
    m_SearchResults.SetColumnWidth(1, 75);
    m_SearchResults.SetColumnWidth(2, 75);
    m_SearchResults.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
    FixUnknownOptions(true);
}

void CDebugMemorySearch::FixUnknownOptions(bool Reset)
{
    CComboBox & cb = m_UnknownOptions;

    if (!Reset && cb.GetCount() > 1)
    {
        return;
    }
    cb.ResetContent();
    if (Reset)
    {
        cb.SetItemData(cb.AddString("Create compare base"), SearchChangeState_Reset);
        cb.SetCurSel(0);
        return;
    }
    cb.SetItemData(cb.AddString("memory changed"), SearchChangeState_Changed);
    cb.SetItemData(cb.AddString("memory unchanged"), SearchChangeState_Unchanged);
    cb.SetItemData(cb.AddString("Value has increased"), SearchChangeState_Greater);
    cb.SetItemData(cb.AddString("Value has descreased"), SearchChangeState_Lessthan);
    cb.SetCurSel(1);
    ::SetWindowText(GetDlgItem(IDC_BTN_SEARCH), "Search");
}

bool CDebugMemorySearch::SearchSetBaseForChanges(void)
{
    if (m_MemoryState != NULL)
    {
        delete[] m_MemoryState;
    }
    m_MemoryStateSize = g_MMU->RdramSize();
    m_MemoryState = new BYTE[m_MemoryStateSize];
    memcpy(m_MemoryState, g_MMU->Rdram(), m_MemoryStateSize);
    return true;
}

bool CDebugMemorySearch::SearchForChanges(SearchMemChangeState SearchType, MemorySize Size,
                                          DWORD &StartAddress, DWORD &Len,
                                          DWORD &OldValue, DWORD &NewValue)
{
    if (g_MMU == NULL)
    {
        return false;
    }

    if (SearchType == SearchChangeState_Reset)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (Size == _32Bit) { StartAddress = ((StartAddress + 3) & ~3); }
    if (Size == _16Bit) { StartAddress = ((StartAddress + 1) & ~1); }

    //search memory
    if (StartAddress < g_MMU->RdramSize())
    {
        DWORD EndMemSearchAddr = StartAddress + Len;
        if (EndMemSearchAddr > g_MMU->RdramSize())
        {
            EndMemSearchAddr = g_MMU->RdramSize();
        }

        DWORD pos;
        switch (Size)
        {
        case _32Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos += 4)
            {
                OldValue = *(DWORD *)(m_MemoryState + pos);
                NewValue = *(DWORD *)(g_MMU->Rdram() + pos);
                if ((SearchType == SearchChangeState_Changed && NewValue != OldValue) ||
                    (SearchType == SearchChangeState_Unchanged && NewValue == OldValue) ||
                    (SearchType == SearchChangeState_Greater && NewValue > OldValue) ||
                    (SearchType == SearchChangeState_Lessthan && NewValue < OldValue))
                {
                    *(DWORD *)(m_MemoryState + pos) = NewValue;
                    Len -= pos - StartAddress;
                    StartAddress = pos;
                    return true;
                }
            }
            break;
        case _16Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos += 2)
            {
                OldValue = *(WORD *)(m_MemoryState + (pos ^ 2));
                NewValue = *(WORD *)(g_MMU->Rdram() + (pos ^ 2));
                if ((SearchType == SearchChangeState_Changed && NewValue != OldValue) ||
                    (SearchType == SearchChangeState_Unchanged && NewValue == OldValue) ||
                    (SearchType == SearchChangeState_Greater && NewValue > OldValue) ||
                    (SearchType == SearchChangeState_Lessthan && NewValue < OldValue))
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos;
                    return true;
                }
            }
            break;
        case _8Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos++)
            {
                OldValue = *(BYTE *)(m_MemoryState + (pos ^ 3));
                NewValue = *(BYTE *)(g_MMU->Rdram() + (pos ^ 3));
                if ((SearchType == SearchChangeState_Changed && NewValue != OldValue) ||
                    (SearchType == SearchChangeState_Unchanged && NewValue == OldValue) ||
                    (SearchType == SearchChangeState_Greater && NewValue > OldValue) ||
                    (SearchType == SearchChangeState_Lessthan && NewValue < OldValue))
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos;
                    return true;
                }
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return false;
}

bool CDebugMemorySearch::SearchForValue(DWORD Value, MemorySize Size, DWORD &StartAddress, DWORD &Len)
{
    if (g_MMU == NULL || g_Rom == NULL)
    {
        return false;
    }

    if (Size == _32Bit)
    {
        StartAddress = ((StartAddress + 3) & ~3);
    }
    if (Size == _16Bit)
    {
        StartAddress = ((StartAddress + 1) & ~1);
    }

    //search memory
    if (StartAddress < g_MMU->RdramSize())
    {
        DWORD EndMemSearchAddr = StartAddress + Len;
        if (EndMemSearchAddr > g_MMU->RdramSize())
        {
            EndMemSearchAddr = g_MMU->RdramSize();
        }

        DWORD pos;
        BYTE * RDRAM = g_MMU->Rdram();
        switch (Size)
        {
        case _32Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos += 4)
            {
                if (*(DWORD *)(RDRAM + pos) == Value)
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos;
                    return true;
                }
            }
            break;
        case _16Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos += 2)
            {
                if (*(WORD *)(RDRAM + (pos ^ 2)) == (WORD)Value)
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos;
                    return true;
                }
            }
            break;
        case _8Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos++)
            {
                if (*(BYTE *)(RDRAM + (pos ^ 3)) == (BYTE)Value)
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos;
                    return true;
                }
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    if (StartAddress >= 0x10000000)
    {
        DWORD EndMemSearchAddr = StartAddress + Len - 0x10000000;
        if (EndMemSearchAddr > g_Rom->GetRomSize())
        {
            EndMemSearchAddr = g_Rom->GetRomSize();
        }
        StartAddress -= 0x10000000;

        DWORD pos;
        BYTE * ROM = g_Rom->GetRomAddress();
        switch (Size)
        {
        case _32Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos += 4)
            {
                if (*(DWORD *)(ROM + pos) == Value)
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos + 0x10000000;
                    return true;
                }
            }
            break;
        case _16Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos += 2)
            {
                if (*(WORD *)(ROM + (pos ^ 2)) == (WORD)Value)
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos + 0x10000000;
                    return true;
                }
            }
            break;
        case _8Bit:
            for (pos = StartAddress; pos < EndMemSearchAddr; pos++)
            {
                if (*(BYTE *)(ROM + (pos ^ 3)) == (BYTE)Value)
                {
                    Len -= pos - StartAddress;
                    StartAddress = pos + 0x10000000;
                    return true;
                }
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return false;
}
