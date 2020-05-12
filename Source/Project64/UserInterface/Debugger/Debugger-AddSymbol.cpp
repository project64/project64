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

LRESULT CAddSymbolDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow();

    m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
    m_AddressEdit.SetDisplayType(CEditNumber32::DisplayHex);
    m_TypeComboBox.Attach(GetDlgItem(IDC_TYPE_COMBOBOX));
    m_NameEdit.Attach(GetDlgItem(IDC_NAME_EDIT));
    m_DescriptionEdit.Attach(GetDlgItem(IDC_DESC_EDIT));

    for (int i = 0;; i++)
    {
        const char* typeName = CSymbolTable::m_SymbolTypes[i].name;
        if (typeName == NULL)
        {
            break;
        }
        m_TypeComboBox.AddString(stdstr(typeName).ToUTF16().c_str());
    }
    
    m_AddressEdit.SetWindowText(L"");
    m_AddressEdit.SetFocus();

    if (m_bHaveAddress)
    {
        m_AddressEdit.SetValue(m_InitAddress, DisplayMode::ZeroExtend);
        m_TypeComboBox.SetFocus();
    }

    if(m_bHaveType)
    {
        m_TypeComboBox.SetCurSel(m_InitType);
        m_NameEdit.SetFocus();
    }
    else
    {
        m_TypeComboBox.SetCurSel(SYM_DATA);
    }
    
    return FALSE;
}

LRESULT CAddSymbolDlg::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDOK:
        int addrLen = m_AddressEdit.GetWindowTextLength();

        if (!addrLen)
        {
            MessageBox(L"Address required", L"Error", MB_OK);
            return 0;
        }

        uint32_t address = m_AddressEdit.GetValue();
        int type = m_TypeComboBox.GetCurSel();

        int nameLen = m_NameEdit.GetWindowTextLength();
        int descLen = m_DescriptionEdit.GetWindowTextLength();
        
        if (!nameLen && !descLen)
        {
            MessageBox(L"Name and/or description required", L"Error", MB_OK);
            return 0;
        }

        wchar_t name[128];
        wchar_t description[256];

        m_NameEdit.GetWindowText(name, nameLen + 1);
        m_DescriptionEdit.GetWindowText(description, descLen + 1);
        
        m_Debugger->SymbolTable()->AddSymbol(type, address, stdstr().FromUTF16(name).c_str(), stdstr().FromUTF16(description).c_str());
        m_Debugger->SymbolTable()->Save();

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