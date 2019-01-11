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

class CEnhancementConfig :
    public CDialogImpl < CEnhancementConfig >
{
public:
    BEGIN_MSG_MAP_EX(CEnhancementConfig)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(ID_POPUP_DELETE, OnDeleteItem)
		COMMAND_ID_HANDLER(ID_POPUP_ADDENHANCEMENT, OnAddEnhancement)
		NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_CLICK, OnEnhancementListClicked)
		NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_RCLICK, OnEnhancementListRClicked)
		NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, NM_DBLCLK, OnEnhancementListDblClicked)
		NOTIFY_HANDLER_EX(IDC_ENHANCEMENTLIST, TVN_SELCHANGED, OnEnhancementListSelChanged)
	END_MSG_MAP()

    enum { IDD = IDD_Enhancement_Config };

	CEnhancementConfig(void);
    ~CEnhancementConfig(void);

    void Display(void * ParentWindow);

	LRESULT	OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddEnhancement(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnhancementListClicked(NMHDR* pNMHDR);
	LRESULT OnEnhancementListRClicked(NMHDR* pNMHDR);
	LRESULT OnEnhancementListDblClicked(NMHDR* pNMHDR);
	LRESULT OnEnhancementListSelChanged(NMHDR* pNMHDR);
	LRESULT OnDeleteItem(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CEnhancementConfig(const CEnhancementConfig&);             // Disable copy constructor
	CEnhancementConfig& operator=(const CEnhancementConfig&);  // Disable assignment

	enum TV_CHECK_STATE { TV_STATE_UNKNOWN, TV_STATE_CLEAR, TV_STATE_CHECKED, TV_STATE_INDETERMINATE };

	void RefreshList(void);
	void AddCodeLayers(int index, const std::string & Name, HTREEITEM hParent, bool Active);
	void ChangeChildrenStatus(HTREEITEM hParent, bool Checked);
	void CheckParentStatus(HTREEITEM hParent);
	bool TV_SetCheckState(HTREEITEM hItem, TV_CHECK_STATE state);
	int TV_GetCheckState(HTREEITEM hItem);

	CTreeViewCtrl m_TreeList;
	HTREEITEM m_hSelectedItem;
};
