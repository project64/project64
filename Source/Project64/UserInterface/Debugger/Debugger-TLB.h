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

class CDebugTlb :
    public CDebugDialog < CDebugTlb >
{
    BEGIN_MSG_MAP_EX(CDebugTlb)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
    END_MSG_MAP()

    LRESULT				OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT				OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);

public:
    enum { IDD = IDD_Debugger_TLB };

    CDebugTlb(CDebuggerUI * debugger);
    virtual ~CDebugTlb(void);

    void RefreshTLBWindow(void);
};
