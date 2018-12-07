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

class CAdvancedOptionsPage :
	public CSettingsPageImpl<CAdvancedOptionsPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CAdvancedOptionsPage)
		COMMAND_ID_HANDLER_EX(IDC_START_ON_ROM_OPEN,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_ZIP,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_DEBUGGER,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_INTERPRETER, CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_REMEMBER_CHEAT,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_CHECK_RUNNING, CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_UNIQUE_SAVE_DIR, CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_DISPLAY_FRAMERATE, CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_SELECT_IPL_DIR, SelectIplDir)
		COMMAND_HANDLER_EX(IDC_IPL_DIR, EN_UPDATE, IplDirChanged)
		COMMAND_HANDLER_EX(IDC_FRAME_DISPLAY_TYPE,LBN_SELCHANGE,ComboBoxChanged)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_Advanced };

public:
	CAdvancedOptionsPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle ( void ) { return TAB_ADVANCED; }
	void HidePage ( void );
	void ShowPage ( void );
	void ApplySettings ( bool UpdateScreen );
	bool EnableReset ( void );
	void ResetPage ( void );

private:
	void SelectIplDir(UINT Code, int id, HWND ctl);
	void IplDirChanged(UINT Code, int id, HWND ctl);
	void UpdatePageSettings(void);
	void SelectFile(LanguageStringID Title, CModifiedEditBox & EditBox);
	CModifiedEditBox m_IplDir;

	bool m_InUpdateSettings;
};
