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

CDebugStackTrace::CDebugStackTrace(CDebuggerUI* debugger) :
CDebugDialog<CDebugStackTrace>(debugger),
m_EntriesIndex(0)
{
}

CDebugStackTrace::~CDebugStackTrace()
{
}

void CDebugStackTrace::PushEntry(uint32_t routineAddress, uint32_t callingAddress)
{
	if (m_EntriesIndex < STACKTRACE_MAX_ENTRIES)
	{
		m_Entries[m_EntriesIndex] = { routineAddress, callingAddress };
		m_EntriesIndex++;
	}
}

void CDebugStackTrace::PopEntry()
{
	if (m_EntriesIndex > 0)
	{
		m_EntriesIndex--;
	}
}

void CDebugStackTrace::ClearEntries()
{
	m_EntriesIndex = 0;
}

LRESULT CDebugStackTrace::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DlgResize_Init();
	
	m_List.Attach(GetDlgItem(IDC_STACKTRACE_LIST));
	m_List.AddColumn("Caller", 0);
	m_List.AddColumn("Routine", 1);
	m_List.AddColumn("Name", 2);
	

	m_List.SetColumnWidth(0, 70);
	m_List.SetColumnWidth(1, 70);
	m_List.SetColumnWidth(2, 160);

	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	WindowCreated();
	return TRUE;
}

LRESULT CDebugStackTrace::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Refresh();
	return FALSE;
}

LRESULT CDebugStackTrace::OnDestroy(void)
{
    m_List.Detach();
	return FALSE;
}

LRESULT CDebugStackTrace::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch (wID)
	{
	case IDCANCEL:
		EndDialog(0);
		break;
	}
	return FALSE;
}

LRESULT CDebugStackTrace::OnListDblClicked(NMHDR* pNMHDR)
{
	NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	int nItem = pIA->iItem;

	uint32_t address = m_List.GetItemData(nItem);

	m_Debugger->Debug_ShowCommandsLocation(address, true);

	return 0;
}

void CDebugStackTrace::Refresh()
{
	if (!m_Debugger->Breakpoints()->isDebugging())
	{
		return;
	}

	SetWindowText(stdstr_f("Stack Trace (%d)", m_EntriesIndex).c_str());

	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	
	CSymbols::EnterCriticalSection();

	for (int i = 0; i < m_EntriesIndex; i++)
	{
		uint32_t routineAddress = m_Entries[i].routineAddress;
		uint32_t callingAddress = m_Entries[i].callingAddress;
		
		char szAddress[9];
		sprintf(szAddress, "%08X", callingAddress);
		m_List.AddItem(i, 0, szAddress);

		sprintf(szAddress, "%08X", routineAddress);
		m_List.AddItem(i, 1, szAddress);

		CSymbolEntry* symbol = CSymbols::GetEntryByAddress(routineAddress);
		if(symbol != NULL)
		{ 
			m_List.AddItem(i, 2, symbol->m_Name);
		}
		else
		{
			m_List.AddItem(i, 2, "");
		}
		
		m_List.SetItemData(i, routineAddress);
	}

	CSymbols::LeaveCriticalSection();

	m_List.SetRedraw(TRUE);
}