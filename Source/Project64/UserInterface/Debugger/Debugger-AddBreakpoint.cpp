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

LRESULT CAddBreakpointDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow();
    m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
    m_ReadCheck.Attach(GetDlgItem(IDC_CHK_READ));
    m_WriteCheck.Attach(GetDlgItem(IDC_CHK_WRITE));
    m_ExecuteCheck.Attach(GetDlgItem(IDC_CHK_EXEC));
    return FALSE;
}

LRESULT CAddBreakpointDlg::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDOK:
    {
        char addrStr[9];
        m_AddressEdit.GetWindowTextA(addrStr, 9);
        uint32_t address = strtoul(addrStr, NULL, 16);

        CBreakpoints* breakpoints = m_Debugger->Breakpoints();

        if (m_ReadCheck.GetCheck())
        {
            breakpoints->RBPAdd(address);
        }
        if (m_WriteCheck.GetCheck())
        {
            breakpoints->WBPAdd(address);
        }
        if (m_ExecuteCheck.GetCheck())
        {
            breakpoints->AddExecution(address);
        }
        EndDialog(0);
        break;
    }
    case IDCANCEL:
        EndDialog(0);
        break;
    }
    return FALSE;
}