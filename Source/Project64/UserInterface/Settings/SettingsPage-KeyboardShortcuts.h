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

class COptionsShortCutsPage :
	public CSettingsPageImpl<COptionsShortCutsPage>,
	public CSettingsPage
{
	typedef CMenuShortCutKey::RUNNING_STATE RUNNING_STATE;
	typedef CShortCutItem::SHORTCUT_KEY_LIST SHORTCUT_KEY_LIST;

	BEGIN_MSG_MAP_EX(COptionsShortCutsPage)
		COMMAND_HANDLER_EX(IDC_C_CPU_STATE,LBN_SELCHANGE,OnCpuStateChanged);
		NOTIFY_HANDLER_EX(IDC_MENU_ITEMS,TVN_SELCHANGED,OnMenuItemChanged);
		COMMAND_HANDLER_EX(IDC_REMOVE,BN_CLICKED,OnRemoveClicked)
		COMMAND_HANDLER_EX(IDC_KEY_PROMPT,BN_CLICKED,OnDetectKeyClicked)
		COMMAND_HANDLER_EX(IDC_ASSIGN,BN_CLICKED,OnAssignClicked)
		COMMAND_HANDLER_EX(IDC_CTL,BN_CLICKED,OnShortCutChanged)
		COMMAND_HANDLER_EX(IDC_ALT,BN_CLICKED,OnShortCutChanged)
		COMMAND_HANDLER_EX(IDC_SHIFT,BN_CLICKED,OnShortCutChanged)
		COMMAND_HANDLER_EX(IDC_VIRTUALKEY,LBN_SELCHANGE,OnShortCutChanged)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_Accelerator };

public:
	COptionsShortCutsPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle     ( void ) { return TAB_SHORTCUTS; }
	void             HidePage      ( void );
	void             ShowPage      ( void );
	void             ApplySettings ( bool UpdateScreen );
	bool             EnableReset   ( void );
	void             ResetPage     ( void );

private:
	void    OnCpuStateChanged      ( UINT Code, int id, HWND ctl );
	void    OnRemoveClicked        ( UINT Code, int id, HWND ctl );
	void    OnDetectKeyClicked     ( UINT Code, int id, HWND ctl );
	void    OnAssignClicked        ( UINT Code, int id, HWND ctl );
	void    OnShortCutChanged      ( UINT Code, int id, HWND ctl );
	LRESULT OnMenuItemChanged      ( LPNMHDR lpnmh );

	void    RefreshShortCutOptions ( HTREEITEM hItem );
	void    InputGetKeys           ( void );
	void    CheckResetEnable       ( void );

	static void stInputGetKeys (COptionsShortCutsPage * _this )
	{
		_this->InputGetKeys();
	}

	CPartialGroupBox m_CreateNewShortCut;
	CComboBox        m_CpuState, m_VirtualKeyList;
	CShortCuts       m_ShortCuts;
	CTreeViewCtrl    m_MenuItems;
	CListBox         m_CurrentKeys;
	bool             m_EnableReset;
};
