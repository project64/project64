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

class CCheats 
{
public:
	CCheats ( const CN64Rom * Rom = NULL );
	~CCheats ( void );

	bool IsCheatMessage ( MSG * msg );
	void ApplyCheats    ( CMipsMemory * MMU );
	void ApplyGSButton  ( CMipsMemory * MMU );
	void LoadCheats     ( bool DisableSelected );
	void SelectCheats   ( HWND hParent, bool BlockExecution );
	void LoadPermCheats ( CPlugins * Plugins );
	inline bool CheatsSlectionChanged ( void ) const { return m_CheatSelectionChanged; }

private:
	typedef struct {
		DWORD Command;
		WORD  Value;
	} GAMESHARK_CODE;

	typedef std::vector<GAMESHARK_CODE> CODES;
	typedef std::vector<CODES>          CODES_ARRAY;

	enum { MaxCheats = 50000 };
	

	static int CALLBACK CheatAddProc        ( HWND hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK CheatListProc       ( HWND hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK ManageCheatsProc    ( HWND hDlg,DWORD uMsg,DWORD wParam, DWORD lParam );
	static int CALLBACK CheatsCodeExProc    ( HWND hDlg, DWORD uMsg, DWORD wParam, DWORD lParam);
	static int CALLBACK CheatsCodeQuantProc ( HWND hDlg, DWORD uMsg, DWORD wParam, DWORD lParam);
	
	//information about the gui for selecting cheats
	HWND    m_Window, m_hSelectCheat, m_AddCheat, m_hCheatTree, m_hSelectedItem;
	const CN64Rom * m_Rom;
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

	bool LoadCode ( int CheatNo, LPCSTR CheatString );
	void AddCodeLayers           ( int CheatNumber, const stdstr &CheatName, HWND hParent, bool CheatActive ); 
	//Reload the cheats from the ini file to the select gui
	void RefreshCheatManager      ( void );
	void ChangeChildrenStatus     ( HWND hParent, bool Checked );
	void CheckParentStatus        ( HWND hParent );
	static stdstr ReadCodeString   ( HWND hDlg, bool &validcodes, bool &validoption, bool &nooptions, int &codeformat );
	static stdstr ReadOptionsString( HWND hDlg, bool &validcodes, bool &validoptions, bool &nooptions, int &codeformat );
	int ApplyCheatEntry (CMipsMemory * MMU,const CODES & CodeEntry, int CurrentEntry, BOOL Execute );
	void RecordCheatValues ( HWND hDlg );
	bool CheatChanged ( HWND hDlg );
	bool IsValid16BitCode ( LPCSTR CheatString ) const;
	void DeleteCheat(int Index);

	//Get Information about the Cheat
	stdstr GetCheatName            ( int CheatNo, bool AddExtension ) const;
	static bool   CheatUsesCodeExtensions ( const stdstr &LineEntry );

	//Working with treeview 
	static bool  TV_SetCheckState(HWND hwndTreeView, HWND hItem, TV_CHECK_STATE state);
	static int   TV_GetCheckState(HWND hwndTreeView, HWND hItem);
	static DWORD AsciiToHex            ( const char * HexValue );
	static void  MenuSetText           ( HMENU hMenu, int MenuPos, const char * Title, char * ShotCut );


	//UI Functions
	static stdstr GetDlgItemStr (HWND hDlg, int nIDDlgItem);
};
