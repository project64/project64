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
#include "DMALog.h"

CDebugMemoryView* CDebugMemoryView::_this = NULL;
HHOOK CDebugMemoryView::hWinMessageHook = NULL;

CDebugMemoryView::CDebugMemoryView(CDebuggerUI * debugger) :
CDebugDialog<CDebugMemoryView>(debugger),
m_MemoryList(NULL)
{
    if (m_MemoryList == NULL)
    {
        m_MemoryList = new CListCtrl;
        m_MemoryList->RegisterClass();
    }

	m_Breakpoints = m_Debugger->Breakpoints();

}

CDebugMemoryView::~CDebugMemoryView()
{
}

LRESULT	CDebugMemoryView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_SymbolColorStride = 0;
	m_SymbolColorPhase = 0;
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
    m_MemAddr.SetValue(0x80000000, false, true);

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

        ::GetWindowRect(GetDlgItem(IDC_MEM_DETAILS), &DlgItemRect);
        ::SetWindowPos(GetDlgItem(IDC_MEM_DETAILS), NULL, 0, 0, DlgItemRect.right - DlgItemRect.left, (DlgItemRect.bottom - DlgItemRect.top) + (height - MemoryListRect.bottom), SWP_NOMOVE);

        ::GetWindowRect(GetDlgItem(IDC_SCRL_BAR), &DlgItemRect);
        ::SetWindowPos(GetDlgItem(IDC_SCRL_BAR), NULL, 0, 0, DlgItemRect.right - DlgItemRect.left, (DlgItemRect.bottom - DlgItemRect.top) + (height - MemoryListRect.bottom), SWP_NOMOVE);
    }

	m_SymInfo.Attach(GetDlgItem(IDC_SYM_INFO));
	m_DMAInfo.Attach(GetDlgItem(IDC_DMA_INFO));

	_this = this;

	DWORD dwThreadID = ::GetCurrentThreadId();
	hWinMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)HookProc, NULL, dwThreadID);

    WindowCreated();

	m_AutoRefreshThread = CreateThread(NULL, 0, AutoRefreshProc, (void*)this, 0, NULL);
	
    return TRUE;
}

DWORD WINAPI CDebugMemoryView::AutoRefreshProc(void* _this)
{
	CDebugMemoryView* self = (CDebugMemoryView*)_this;
	while (true)
	{
		self->RefreshMemory(true);
		Sleep(100);
	}
}

void CDebugMemoryView::InterceptMouseWheel(WPARAM wParam, LPARAM lParam)
{
	uint32_t newAddress = m_DataStartLoc - ((short)HIWORD(wParam) / WHEEL_DELTA) * 16;

	m_DataStartLoc = newAddress;

	m_MemAddr.SetValue(m_DataStartLoc, false, true);
}

LRESULT CALLBACK CDebugMemoryView::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG *pMsg = (MSG*)lParam;

	if (pMsg->message == WM_MOUSEWHEEL)
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

LRESULT CDebugMemoryView::OnDestroy(void)
{
	if (m_AutoRefreshThread != NULL)
	{
		TerminateThread(m_AutoRefreshThread, 0);
		CloseHandle(m_AutoRefreshThread);
	}
    if (m_MemoryList)
    {
        m_MemoryList->UnsubclassWindow();
        delete m_MemoryList;
        m_MemoryList = NULL;
    }
	UnhookWindowsHookEx(hWinMessageHook);
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
	case IDC_SYMBOLS_BTN:
		m_Debugger->Debug_ShowSymbolsWindow();
		break;
    case IDCANCEL:
        EndDialog(0);
        break;
	case ID_POPUPMENU_TOGGLERBP:
		m_Breakpoints->RBPToggle(m_CtxMenuAddr);
		RefreshMemory(true);
		break;
	case ID_POPUPMENU_TOGGLEWBP:
		m_Breakpoints->WBPToggle(m_CtxMenuAddr);
		RefreshMemory(true);
		break;
	case ID_POPUPMENU_CLEARALLBPS:
		m_Breakpoints->RBPClear();
		m_Breakpoints->WBPClear();
		RefreshMemory(true);
		break;
	case ID_POPUPMENU_VIEWDISASM:
		m_Debugger->Debug_ShowCommandsLocation(m_CtxMenuAddr, true);
		break;
	case ID_POPUPMENU_ADDSYMBOL:
		m_AddSymbolDlg.DoModal(m_Debugger, m_CtxMenuAddr);
		break;
    }
    return FALSE;
}

LRESULT CDebugMemoryView::OnMemoryRightClicked(LPNMHDR lpNMHDR)
{
	uint32_t address;
	bool bData = GetItemAddress(lpNMHDR, address);

	if (!bData)
	{
		return 0;
	}

	m_CtxMenuAddr = address;

	HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MEM_BP_POPUP));
	HMENU hPopupMenu = GetSubMenu(hMenu, 0);
	
	if (m_Breakpoints->m_RBP.size() == 0 && m_Breakpoints->m_WBP.size() == 0)
	{
		EnableMenuItem(hPopupMenu, ID_POPUPMENU_CLEARALLBPS, MF_DISABLED | MF_GRAYED);
	}
	
	POINT mouse;
	GetCursorPos(&mouse);

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, mouse.x, mouse.y, 0, m_hWnd, NULL);

	DestroyMenu(hMenu);

	return 0;
}

LRESULT CDebugMemoryView::OnHotItemChanged(LPNMHDR lpNMHDR)
{
	uint32_t address;
	bool bData = GetItemAddress(lpNMHDR, address);

	if (!bData)
	{
		return 0;
	}

	CSymbols::EnterCriticalSection();

	CSymbolEntry* lpSymbol = CSymbols::GetEntryByAddress(address);

	stdstr symbolInfo;

	if (lpSymbol != NULL)
	{
		char* desc = lpSymbol->m_Description;
		desc = desc ? desc : "";
		symbolInfo = stdstr_f("%08X: %s %s // %s", address, lpSymbol->TypeName(), lpSymbol->m_Name, desc);
	}
	else
	{
		symbolInfo = stdstr_f("%08X", address);
	}

	CSymbols::LeaveCriticalSection();

	m_SymInfo.SetWindowTextA(symbolInfo.c_str());
	
	uint32_t romAddr, offset;
	DMALOGENTRY* lpEntry = m_Debugger->DMALog()->GetEntryByRamAddress(address, &romAddr, &offset);

	stdstr dmaInfo;

	if (lpEntry != NULL)
	{
		dmaInfo = stdstr_f("Last DMA: %08X -> %08X [%X] (%08X, +%X) ", lpEntry->romAddr, lpEntry->ramAddr, lpEntry->length, romAddr, offset);
	}
	else
	{
		dmaInfo = stdstr_f("Last DMA: ?");
	}

	m_DMAInfo.SetWindowTextA(dmaInfo.c_str());

	return 0;
}

LRESULT CDebugMemoryView::OnMemoryModified(LPNMHDR lpNMHDR)
{
	uint32_t Pos = 0;
	bool bData = GetItemOffset(lpNMHDR, Pos);

	if (!bData)
	{
		return 0;
	}

    CListNotify *pListNotify = reinterpret_cast<CListNotify *>(lpNMHDR);
    int LineNumber = pListNotify->m_nItem;
	
    LPCSTR strValue = m_MemoryList->GetItemText(pListNotify->m_nItem, pListNotify->m_nSubItem);
    int Finish = strlen(strValue);
	
	DWORD Value = strtoull(strValue, NULL, 16);
	
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
		m_MemAddr.SetValue(Location < 0xFFFFFFEF ? Location + 0x10 : 0xFFFFFFFF, false, true);
		break;
	case SB_LINEUP:
		m_MemAddr.SetValue(Location > 0x10 ? Location - 0x10 : 0, false, true);
		break;
	case SB_PAGEDOWN:
		m_MemAddr.SetValue(Location < 0xFFFFFEFF ? Location + 0x100 : 0xFFFFFFFF, false, true);
		break;
	case SB_PAGEUP:
		m_MemAddr.SetValue(Location > 0x100 ? Location - 0x100 : 0, false, true);
		break;
	case SB_THUMBPOSITION:
		m_MemAddr.SetValue((DWORD)Pos << 0x10, false, true);
		break;
	default:
		break;
	}
}

LRESULT CDebugMemoryView::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	WORD type = LOWORD(wParam);

	if (type == WA_INACTIVE)
	{
		return FALSE;
	}

	RefreshMemory(false);

	return FALSE;
}

void CDebugMemoryView::ShowAddress(DWORD Address, bool VAddr)
{
    if (m_hWnd == NULL)
    {
        return;
    }

    SendDlgItemMessage(IDC_CHK_VADDR, BM_SETCHECK, VAddr ? BST_CHECKED : BST_UNCHECKED, 0);
    m_MemAddr.SetValue(Address, false, true);
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

			uint32_t vaddr = 0x80000000 | (m_DataStartLoc + Pos);

			COLORREF bgColor, fgColor, fgHiColor;
			SelectColors(vaddr, Changed, bgColor, fgColor, fgHiColor);
			
			m_MemoryList->SetItemColours(LineNumber, col, bgColor, fgColor);
            m_MemoryList->SetItemHighlightColours(LineNumber, col, fgHiColor);
            
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

void CDebugMemoryView::RefreshMemory(bool ResetCompare)
{
	m_SymbolColorPhase = 0;

	if (g_MMU == NULL)
	{
		return;
	}

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
			if (!AddressSafe(m_DataStartLoc & ~3))
			{
				ValidData = false;
			}
            else if (!g_MMU->LW_VAddr(m_DataStartLoc & ~3, word.UW))
            {
                ValidData = false;
            }
        }
        else
        {
			if ((m_DataStartLoc & ~3) >= g_MMU->RdramSize())
			{
				ValidData = false;
			}
			else if (!g_MMU->LW_PAddr(m_DataStartLoc & ~3, word.UW))
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
			if (!AddressSafe(Pos))
			{
				ValidData = false;
			}
            else if (!g_MMU->LW_VAddr(Pos, word.UW))
            {
                ValidData = false;
            }
        }
        else
        {
			if (Pos >= g_MMU->RdramSize())
			{
				ValidData = false;
			}
            else if (!g_MMU->LW_PAddr(Pos, word.UW))
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

bool CDebugMemoryView::GetItemOffset(LPNMHDR lpNMHDR, uint32_t &offset)
{
	CListNotify *pListNotify = reinterpret_cast<CListNotify *>(lpNMHDR);

	int nRow = pListNotify->m_nItem;
	int nCol = pListNotify->m_nSubItem - 1;

	if (nCol < 0 || (nCol % 5) == 4)
	{
		return false;
	}

	offset = (nRow * 0x10) + (nCol / 5) * 4 + (nCol % 5);

	return true;
}

bool CDebugMemoryView::GetItemAddress(LPNMHDR lpNMHDR, uint32_t &address)
{
	uint32_t offset;
	bool bData = GetItemOffset(lpNMHDR, offset);

	if (!bData)
	{
		return false;
	}

	address = 0x80000000 | (m_DataStartLoc + offset);

	return true;
}

void CDebugMemoryView::SelectColors(uint32_t vaddr, bool changed, COLORREF& bgColor, COLORREF& fgColor, COLORREF& fgHiColor)
{
	CSymbols::EnterCriticalSection();
	CSymbolEntry* lpSymbol = CSymbols::GetEntryByAddress(vaddr);
	
	if (lpSymbol != NULL)
	{
		m_SymbolColorStride = lpSymbol->TypeSize();
		m_SymbolColorPhase = m_SymbolColorPhase ? 0 : 1;
	}

	CSymbols::LeaveCriticalSection();

	bool bHaveReadBP = m_Breakpoints->RBPExists(vaddr);
	bool bHaveWriteBP = m_Breakpoints->WBPExists(vaddr);

	fgHiColor = RGB(0x00, 0x00, 0x00);

	if (bHaveReadBP && bHaveWriteBP)
	{
		bgColor = RGB(0xAA, 0xDD, 0xDD);
	}
	else if (bHaveReadBP)
	{
		bgColor = RGB(0xDD, 0xDD, 0xAA);
	}
	else if (bHaveWriteBP)
	{
		bgColor = RGB(0xAA, 0xAA, 0xDD);
	}
	else if (m_SymbolColorStride > 0)
	{
		bgColor = m_SymbolColorPhase ? RGB(0xD0, 0xF0, 0xD0) : RGB(0xAA, 0xCC, 0xAA);
	}
	else
	{
		bgColor = GetSysColor(COLOR_WINDOW);
		fgHiColor = (changed ? RGB(255, 0, 0) : GetSysColor(COLOR_HIGHLIGHTTEXT));
		fgColor = (changed ? RGB(255, 0, 0) : GetSysColor(COLOR_WINDOWTEXT));
	}

	if (m_SymbolColorStride > 0)
	{
		m_SymbolColorStride--;
	}
}

// Check if KSEG0 addr is out of bounds
bool CDebugMemoryView::AddressSafe(uint32_t vaddr)
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