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

LRESULT	CAddSymbolDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow();

	m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
	m_AddressEdit.SetDisplayType(CEditNumber::DisplayHex);
	m_TypeComboBox.Attach(GetDlgItem(IDC_TYPE_COMBOBOX));
	m_NameEdit.Attach(GetDlgItem(IDC_NAME_EDIT));
	m_DescriptionEdit.Attach(GetDlgItem(IDC_DESC_EDIT));

	for (int i = 0;; i++)
	{
		char* type = CSymbols::SymbolTypes[i];
		if (type == NULL)
		{
			break;
		}
		m_TypeComboBox.AddString(type);
	}
	
	m_AddressEdit.SetWindowTextA("");
	m_AddressEdit.SetFocus();

	if (m_bHaveAddress)
	{
		m_AddressEdit.SetValue(m_InitAddress, false, true);
		m_TypeComboBox.SetFocus();
	}

	if(m_bHaveType)
	{
		m_TypeComboBox.SetCurSel(m_InitType);
		m_NameEdit.SetFocus();
	}
	else
	{
		m_TypeComboBox.SetCurSel(CSymbols::TYPE_DATA);
	}
	
	return FALSE;
}

LRESULT CAddSymbolDlg::OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	switch (wID)
	{
	case IDCANCEL:
		EndDialog(0);
		break;
	case IDOK:
		int addrLen = m_AddressEdit.GetWindowTextLengthA();

		if (!addrLen)
		{
			MessageBox("Address required", "Error", MB_OK);
			return 0;
		}

		uint32_t address = m_AddressEdit.GetValue();
		int type = m_TypeComboBox.GetCurSel();

		int nameLen = m_NameEdit.GetWindowTextLengthA();
		int descLen = m_DescriptionEdit.GetWindowTextLengthA();
		
		if (!nameLen && !descLen)
		{
			MessageBox("Name and/or description required", "Error", MB_OK);
			return 0;
		}

		char name[128];
		char description[256];

		m_NameEdit.GetWindowTextA(name, nameLen + 1);
		m_DescriptionEdit.GetWindowTextA(description, descLen + 1);
		
		CSymbols::EnterCriticalSection();
		CSymbols::Add(type, address, name, description);
		CSymbols::Save();
		CSymbols::LeaveCriticalSection();

		m_Debugger->Debug_RefreshSymbolsWindow();

		EndDialog(0);
		break;
	}
	return 0;
}

INT_PTR CAddSymbolDlg::DoModal(CDebuggerUI* debugger)
{
	m_Debugger = debugger;
	m_bHaveAddress = false;
	m_bHaveType = false;
	return CDialogImpl<CAddSymbolDlg>::DoModal();
}

INT_PTR CAddSymbolDlg::DoModal(CDebuggerUI* debugger, uint32_t initAddress)
{
	m_Debugger = debugger;
	m_bHaveAddress = true;
	m_bHaveType = false;
	m_InitAddress = initAddress;
	return CDialogImpl<CAddSymbolDlg>::DoModal();
}

INT_PTR CAddSymbolDlg::DoModal(CDebuggerUI* debugger, uint32_t initAddress, int initType)
{
	m_Debugger = debugger;
	m_bHaveAddress = true;
	m_bHaveType = true;
	m_InitAddress = initAddress;
	m_InitType = initType;
	return CDialogImpl<CAddSymbolDlg>::DoModal();
}