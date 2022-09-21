#pragma once
#include "SettingsPage.h"

class CGameStatusPage :
	public CSettingsPageImpl<CGameStatusPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGameStatusPage)
		COMMAND_HANDLER_EX(IDC_STATUS_TYPE,LBN_SELCHANGE,ComboBoxChanged)
		COMMAND_HANDLER_EX(IDC_NOTES_CORE,EN_UPDATE,EditBoxChanged)
		COMMAND_HANDLER_EX(IDC_NOTES_PLUGIN,EN_UPDATE,EditBoxChanged)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_GameStatus };

public:
	CGameStatusPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle     ( void ) { return TAB_ROMNOTES; }
	void             HidePage      ( void );
	void             ShowPage      ( void );
	void             ApplySettings ( bool UpdateScreen );
	bool             EnableReset   ( void );
	void             ResetPage     ( void );
	
};
