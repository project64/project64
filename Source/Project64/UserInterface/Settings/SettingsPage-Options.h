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

class CGeneralOptionsPage :
	public CSettingsPageImpl<CGeneralOptionsPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGeneralOptionsPage)
		COMMAND_ID_HANDLER_EX(IDC_AUTOSLEEP,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_LOAD_FULLSCREEN,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_SCREEN_SAVER,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_BASIC_MODE,OnBasicMode)
		COMMAND_ID_HANDLER_EX(IDC_SELECT_IPL_DIR, SelectIplDir)
		COMMAND_HANDLER_EX(IDC_IPL_DIR, EN_UPDATE, IplDirChanged)
		COMMAND_HANDLER_EX(IDC_REMEMBER,EN_UPDATE,EditBoxChanged)
		COMMAND_HANDLER_EX(IDC_REMEMBERDIR,EN_UPDATE,EditBoxChanged)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_General };

public:
	CGeneralOptionsPage(CSettingConfig * SettingsConfig, HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle     ( void ) { return TAB_OPTIONS; }
	void             HidePage      ( void );
	void             ShowPage      ( void );
	void             ApplySettings ( bool UpdateScreen );
	bool             EnableReset   ( void );
	void             ResetPage     ( void );

private:
	void OnBasicMode ( UINT Code, int id, HWND ctl );
	void SelectIplDir( UINT Code, int id, HWND ctl );
	void IplDirChanged( UINT Code, int id, HWND ctl );
	void UpdatePageSettings( void );
	void SelectFile( LanguageStringID Title, CModifiedEditBox & EditBox );
	CSettingConfig * m_SettingsConfig;
	CModifiedEditBox m_IplDir;

	bool m_InUpdateSettings;
};
