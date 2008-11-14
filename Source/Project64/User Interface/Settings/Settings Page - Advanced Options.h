#pragma once

class CAdvancedOptionsPage :
	public CSettingsPageImpl<CAdvancedOptionsPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CAdvancedOptionsPage)
		COMMAND_ID_HANDLER_EX(IDC_START_ON_ROM_OPEN,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_ZIP,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_DEBUGGER,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_REMEMBER_CHEAT,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_DISPLAY_FRAMERATE,CheckBoxChanged)
		COMMAND_HANDLER_EX(IDC_FRAME_DISPLAY_TYPE,LBN_SELCHANGE,ComboBoxChanged)

	END_MSG_MAP()

	enum { IDD = IDD_Settings_Advanced };

public:
	CAdvancedOptionsPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle     ( void ) { return TAB_ADVANCED; }
	void             HidePage      ( void );
	void             ShowPage      ( void );
	void             ApplySettings ( bool UpdateScreen );
	bool             EnableReset   ( void );
	void             ResetPage     ( void );

private:
};
