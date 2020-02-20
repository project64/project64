/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#pragma once

#include "DebuggerUI.h"

// todo maybe add char* ownerName and use a TreeView

class CDebugSymbols :
    public CDebugDialog<CDebugSymbols>,
    public CDialogResize<CDebugSymbols>
{
private:
    enum {
        SymbolsListView_Col_Address,
        SymbolsListView_Col_Type,
        SymbolsListView_Col_Name,
        SymbolsListView_Col_Value,
        SymbolsListView_Col_Description,
        SymbolsListView_Num_Columns
    };

    static const CSetValueDlg::ComboItem ModalChangeTypeItems[];

    CListViewCtrl m_SymbolsListView;
    CSetValueDlg  m_SetValueDlg;
    CAddSymbolDlg m_AddSymbolDlg;

public:
    enum { IDD = IDD_Debugger_Symbols };
    enum { TIMER_ID_AUTO_REFRESH };

    CDebugSymbols(CDebuggerUI * debugger);
    //virtual ~CDebugScripts(void);

    void Refresh();
    void RefreshValues();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT	OnListDblClicked(NMHDR* pNMHDR);
    LRESULT OnDestroy(void);
    void OnExitSizeMove(void);
    void OnTimer(UINT_PTR nIDEvent);

    BEGIN_MSG_MAP_EX(CDebugSymbols)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MSG_WM_DESTROY(OnDestroy)
        NOTIFY_HANDLER_EX(IDC_SYMBOLS_LIST, NM_DBLCLK, OnListDblClicked)
        MSG_WM_TIMER(OnTimer)
        CHAIN_MSG_MAP(CDialogResize<CDebugSymbols>)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
        END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugSymbols)
        DLGRESIZE_CONTROL(IDC_FILTER_EDIT, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FILTER_STATIC, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_REMOVESYMBOL_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_ADDSYMBOL_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_SYMBOLS_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()
};
