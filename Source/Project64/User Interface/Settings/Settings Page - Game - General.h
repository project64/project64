#pragma once

class CGameGeneralPage :
	private CDialogImpl<CGameGeneralPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGameGeneralPage)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_GameGeneral };

public:
	CGameGeneralPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle ( void ) { return TAB_ROMSETTINGS; }
	void             HidePage  ( void );
	void             ShowPage  ( void );
};
