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

#pragma once

// Modal add breakpoint dialog

class CAddBreakpointDlg : public CDialogImpl<CAddBreakpointDlg>
{
public:
    enum { IDD = IDD_Debugger_AddBreakpoint };

    INT_PTR CAddBreakpointDlg::DoModal(CDebuggerUI* debugger)
    {
        m_Debugger = debugger;
        return CDialogImpl<CAddBreakpointDlg>::DoModal();
    }

private:
    CDebuggerUI* m_Debugger;

    CButton m_ReadCheck;
    CButton m_WriteCheck;
    CButton m_ExecuteCheck;
    CEdit   m_AddressEdit;

    LRESULT	OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT	OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDestroy(void)
    {
        return 0;
    }

    BEGIN_MSG_MAP_EX(CAddBreakpointDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MSG_WM_DESTROY(OnDestroy)
    END_MSG_MAP()
};