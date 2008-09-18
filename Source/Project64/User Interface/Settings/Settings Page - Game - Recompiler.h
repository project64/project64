#pragma once

class CGameRecompilePage :
	private CDialogImpl<CGameRecompilePage>,
	public CSettingsPage
{

	BEGIN_MSG_MAP_EX(CGameRecompilePage)
	END_MSG_MAP()

	enum { IDD = IDD_Settings_GameRecompiler };

public:
	CGameRecompilePage(HWND hParent, const RECT & rcDispay );

	LanguageStringID PageTitle ( void ) { return TAB_RECOMPILER; }
	void             HidePage  ( void );
	void             ShowPage  ( void );
	
};
