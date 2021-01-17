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
#include <Project64\WTLApp.h>

class CEditEnhancement;

class CEnhancementUI :
    public CDialogImpl<CEnhancementUI>
{
    enum TV_CHECK_STATE 
    { 
        TV_STATE_UNKNOWN, 
        TV_STATE_CLEAR, 
        TV_STATE_CHECKED, 
        TV_STATE_INDETERMINATE 
    };

public:
    BEGIN_MSG_MAP_EX(CEnhancementUI)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        COMMAND_ID_HANDLER(ID_POPUP_ADDENHANCEMENT, OnAddEnhancement)
        NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_RCLICK, OnEnhancementListRClicked)
        NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_DBLCLK, OnEnhancementListDClicked)
    END_MSG_MAP()

    enum { IDD = IDD_Enhancement_Config };

    CEnhancementUI();

    void Display(HWND hParent, bool BlockExecution);

private:
    friend CEditEnhancement;

    CEnhancementUI(const CEnhancementUI&);
    CEnhancementUI& operator=(const CEnhancementUI&);

    LRESULT	OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnAddEnhancement(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnEnhancementListRClicked(NMHDR* pNMHDR);
    LRESULT OnEnhancementListDClicked(NMHDR* pNMHDR);

    void AddCodeLayers(LPARAM ListID, const std::wstring &Name, HTREEITEM hParent, bool Active);
    void RefreshList(void);
    TV_CHECK_STATE TV_GetCheckState(HTREEITEM hItem);
    bool TV_SetCheckState(HTREEITEM hItem, TV_CHECK_STATE state);

    CEnhancementList m_Enhancements;
    CTreeViewCtrl m_TreeList;
    HTREEITEM m_hSelectedItem;
};
