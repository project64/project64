/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

class CCheats {
	typedef struct {
		DWORD Command;
		WORD  Value;
	} GAMESHARK_CODE;

	typedef std::vector<GAMESHARK_CODE> CODES;
	typedef std::vector<CODES>          CODES_ARRAY;

	enum { MaxCheats = 50000 };
	
	CN64Rom       * const _Rom;

	static int CALLBACK CheatAddProc        ( WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK CheatListProc       ( WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK ManageCheatsProc    ( WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK CheatsCodeExProc    ( WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK CheatsCodeQuantProc ( WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	
	//information about the gui for selecting cheats
	WND_HANDLE    m_Window, m_hSelectCheat, m_AddCheat, m_hCheatTree, m_hSelectedItem;
	void          * const m_rcList, * const m_rcAdd;
	int           m_MinSizeDlg, m_MaxSizeDlg;
	int           m_EditCheat;
	bool          m_DeleteingEntries;
	CODES_ARRAY   m_Codes;
	bool          m_CheatSelectionChanged;
	
	//Information about the current cheat we are editing
	stdstr        m_EditName;
	stdstr        m_EditCode;
	stdstr        m_EditOptions;
	stdstr        m_EditNotes;

	enum Dialog_State   { CONTRACTED, EXPANDED } m_DialogState;
	enum TV_CHECK_STATE { TV_STATE_UNKNOWN, TV_STATE_CLEAR, TV_STATE_CHECKED, TV_STATE_INDETERMINATE };
	enum { MaxGSEntries = 100, IDC_MYTREE = 0x500 };

	void LoadPermCheats (void);
	bool LoadCode ( int CheatNo, LPCSTR CheatString );
	void AddCodeLayers           ( int CheatNumber, const stdstr &CheatName, WND_HANDLE hParent, bool CheatActive ); 
	//Reload the cheats from the ini file to the select gui
	void RefreshCheatManager      ( void );
	void ChangeChildrenStatus     ( WND_HANDLE hParent, bool Checked );
	void CheckParentStatus        ( WND_HANDLE hParent );
	static stdstr ReadCodeString   ( WND_HANDLE hDlg, bool &validcodes, bool &validoption, bool &nooptions, int &codeformat );
	static stdstr ReadOptionsString( WND_HANDLE hDlg, bool &validcodes, bool &validoptions, bool &nooptions, int &codeformat );
	int ApplyCheatEntry (CMipsMemory * MMU,const CODES & CodeEntry, int CurrentEntry, BOOL Execute );
	void RecordCheatValues ( WND_HANDLE hDlg );
	bool CheatChanged ( WND_HANDLE hDlg );
	bool IsValid16BitCode ( LPCSTR CheatString ) const;
	void DeleteCheat(int Index);

	//Get Information about the Cheat
	stdstr GetCheatName            ( int CheatNo, bool AddExtension ) const;
	static bool   CheatUsesCodeExtensions ( const stdstr &LineEntry );

	//Working with treeview 
	static bool  TV_SetCheckState(WND_HANDLE hwndTreeView, WND_HANDLE hItem, TV_CHECK_STATE state);
	static int   TV_GetCheckState(WND_HANDLE hwndTreeView, WND_HANDLE hItem);
	static DWORD AsciiToHex            ( const char * HexValue );
	static void  MenuSetText           ( MENU_HANDLE hMenu, int MenuPos, const char * Title, char * ShotCut );


	//UI Functions
	static stdstr GetDlgItemStr (WND_HANDLE hDlg, int nIDDlgItem);

public:
	CCheats (CN64Rom * const Rom = NULL);
	~CCheats ( void );

	bool IsCheatMessage ( MSG * msg );
	void ApplyCheats    ( CMipsMemory * MMU );
	void ApplyGSButton  ( CMipsMemory * MMU );
	void LoadCheats     ( bool DisableSelected );
	void SelectCheats   ( WND_HANDLE hParent, bool BlockExecution );
	inline bool CheatsSlectionChanged ( void ) const { return m_CheatSelectionChanged; }
};