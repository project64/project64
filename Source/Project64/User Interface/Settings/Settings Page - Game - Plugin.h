#pragma once

class CGamePluginPage :
	private CDialogImpl<CGamePluginPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGamePluginPage)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_GamePlugin };

public:
	CGamePluginPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle ( void ) { return TAB_PLUGIN; }
	void             HidePage  ( void );
	void             ShowPage  ( void );
	
};
