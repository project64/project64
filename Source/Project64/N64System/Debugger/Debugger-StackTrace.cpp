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

CDebugStackTrace::CDebugStackTrace(CDebuggerUI* debugger) :
CDebugDialog<CDebugStackTrace>(debugger)
{
}

CDebugStackTrace::~CDebugStackTrace()
{
}

LRESULT CDebugStackTrace::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DlgResize_Init();

	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, 0);

	m_List.Attach(GetDlgItem(IDC_STACKTRACE_LIST));
	m_List.AddColumn("Routine", 0);
	m_List.AddColumn("Name", 1);

	m_List.SetColumnWidth(0, 60);
	m_List.SetColumnWidth(1, 100);

	WindowCreated();
	return TRUE;
}

LRESULT CDebugStackTrace::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	RefreshList();
	return FALSE;
}

LRESULT CDebugStackTrace::OnDestroy(void)
{
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

void CDebugStackTrace::RefreshList()
{
	vector<uint32_t>* stackTrace = m_Debugger->StackTrace();

	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	
	int count = stackTrace->size();

	if (count > 4000)
	{
		count = 4000;
	}

	for (int i = 0; i < count; i++)
	{
		uint32_t address = stackTrace->at(i);

		char szAddress[9];
		sprintf(szAddress, "%08X", address);
		
		m_List.AddItem(i, 0, szAddress);
		m_List.AddItem(i, 1, "symbol");

		m_List.SetItemData(i, address);
	}

	m_List.SetRedraw(TRUE);
}