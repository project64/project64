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

class CDebugStackView :
    public CDebugDialog<CDebugStackView>,
    public CDialogResize<CDebugStackView>
{
public:
    enum { IDD = IDD_Debugger_Stack };

    CDebugStackView(CDebuggerUI * debugger);
    virtual ~CDebugStackView(void);

    void Refresh();

private:
    CListViewCtrl m_StackList;
    CStatic m_SPStatic;

    LRESULT	OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(void);
    LRESULT	OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BEGIN_MSG_MAP_EX(CDebugStackView)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        CHAIN_MSG_MAP(CDialogResize<CDebugStackView>)
        END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugStackView)
        DLGRESIZE_CONTROL(IDC_STACK_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()
};