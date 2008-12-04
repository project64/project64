#pragma once

class CGeneralOptionsPage :
	private CSettingsPageImpl<CGeneralOptionsPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGeneralOptionsPage)
		COMMAND_ID_HANDLER_EX(IDC_AUTOSLEEP,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_LOAD_FULLSCREEN,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_SCREEN_SAVER,CheckBoxChanged)
		COMMAND_ID_HANDLER_EX(IDC_BASIC_MODE,CheckBoxChanged)
		COMMAND_HANDLER_EX(IDC_REMEMBER,EN_UPDATE,EditBoxChanged)
		COMMAND_HANDLER_EX(IDC_REMEMBERDIR,EN_UPDATE,EditBoxChanged)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_General };

public:
	CGeneralOptionsPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle     ( void ) { return TAB_OPTIONS; }
	void             HidePage      ( void );
	void             ShowPage      ( void );
	void             ApplySettings ( bool UpdateScreen );
	bool             EnableReset   ( void );
	void             ResetPage     ( void );

private:
};
