#pragma once

class CGameStatusPage :
	private CDialogImpl<CGameStatusPage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGameStatusPage)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_GameStatus };

public:
	CGameStatusPage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle ( void ) { return TAB_ROMNOTES; }
	void             HidePage  ( void );
	void             ShowPage  ( void );
	
};
