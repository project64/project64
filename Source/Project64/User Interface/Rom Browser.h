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

#include <vector>

class CMainGui;
class CNotification;
class CPlugins;

class ROMBROWSER_FIELDS {
	stdstr m_Name;
	size_t m_Pos, m_DefaultPos;
	int    m_ID;
	ULONG  m_ColWidth;
	LanguageStringID  m_LangID;
	bool   m_PosChanged;

public:
	ROMBROWSER_FIELDS (const char * Name, int Pos, int ID, int ColWidth, LanguageStringID LangID, bool UseDefault) :
		m_Name(Name),
		m_Pos(Pos),
		m_DefaultPos(Pos),
		m_ID(ID),
		m_ColWidth(ColWidth),
		m_LangID(LangID),
		m_PosChanged(false)
		
	{
		if (!UseDefault)
		{
			m_PosChanged = g_Settings->LoadDwordIndex(RomBrowser_PosIndex,m_ID,(ULONG &)m_Pos );
			g_Settings->LoadDwordIndex(RomBrowser_WidthIndex,m_ID,m_ColWidth);
		}
	}
	inline LPCSTR Name ( void ) const { return m_Name.c_str(); }
	inline size_t Pos  ( void ) const { return m_Pos; }
	inline bool   PosChanged ( void ) const { return m_PosChanged; }
	inline int    ID  ( void ) const { return m_ID; }
	inline int    ColWidth  ( void ) const { return m_ColWidth; }
	inline LanguageStringID  LangID  ( void ) const { return m_LangID; }
	
	void SetColWidth  ( int ColWidth ) 
	{
		m_ColWidth = ColWidth;
		g_Settings->SaveDwordIndex(RomBrowser_WidthIndex,m_ID,m_ColWidth);
	}
	void SetColPos  ( int Pos)
	{
		m_Pos = Pos;
		g_Settings->SaveDwordIndex(RomBrowser_PosIndex,m_ID,m_Pos);
		m_PosChanged = true;
	}
	void ResetPos  ( void )
	{
		m_Pos = m_DefaultPos;
		g_Settings->DeleteSettingIndex(RomBrowser_PosIndex,m_ID);
		m_PosChanged = false;
	}
};

typedef std::vector<ROMBROWSER_FIELDS>   ROMBROWSER_FIELDS_LIST;
typedef std::vector<int>                 FIELD_TYPE_LIST;

class CRomBrowser;
typedef struct {
	CRomBrowser * _this;
	int           Key;
	bool          KeyAscend;
} SORT_FIELD;

class C7zip;
class CRomBrowser 
{
	enum { IDC_ROMLIST = 223 };
	enum { RB_FileName      = 0,  RB_InternalName = 1,  RB_GoodName    = 2,
           RB_Status        = 3,  RB_RomSize      = 4,  RB_CoreNotes   = 5,
		   RB_PluginNotes   = 6,  RB_UserNotes    = 7,  RB_CartridgeID = 8,
		   RB_Manufacturer  = 9,  RB_Country      = 10, RB_Developer   = 11,
		   RB_Crc1          = 12, RB_Crc2         = 13, RB_CICChip     = 14,
		   RB_ReleaseDate   = 15, RB_Genre        = 16, RB_Players     = 17,
		   RB_ForceFeedback = 18, RB_FileFormat   = 19 };

	enum FILE_FORMAT {
		Format_Uncompressed,
		Format_Zip,
		Format_7zip,
	} ;

	enum 
	{
		NoOfSortKeys = 3
	};

	typedef struct {
		char     szFullFileName[300];
		FILE_FORMAT FileFormat;
		char     Status[60];
		char     FileName[200];
		char     InternalName[22];
		char     GoodName[200];
		char     CartID[3];
		char     PluginNotes[250];
		char     CoreNotes[250];
		char     UserNotes[250];
		char     Developer[30];
		char     ReleaseDate[30];
		char     Genre[15];
		int		 Players;
		DWORD    TextColor;
		int      SelColor;
		DWORD    SelTextColor;
		DWORD    SelColorBrush;
		int      RomSize;
		BYTE     Manufacturer;
		BYTE     Country;
		DWORD    CRC1;
		DWORD    CRC2;
		int      CicChip;
		char     ForceFeedback[15];
	} ROM_INFO;

	typedef std::vector<ROM_INFO>   ROMINFO_LIST;

	HWND           & m_MainWindow;
	HWND           & m_StatusWindow;
	HWND             m_hRomList;
	ROMBROWSER_FIELDS_LIST m_Fields;
	FIELD_TYPE_LIST        m_FieldType;
	ROMINFO_LIST           m_RomInfo;
	stdstr                 m_SelectedRom;
	bool                   m_Visible;
	bool                   m_ShowingRomBrowser;
	HANDLE                 m_RefreshThread;
	bool                   m_StopRefresh;
	CIniFile             * m_RomIniFile;
	CIniFile             * m_NotesIniFile;
	CIniFile             * m_ExtIniFile;
	CIniFile             * m_ZipIniFile;
	bool                   m_AllowSelectionLastRom;

	void  AddFileNameToList       ( strlist & FileList, const stdstr & Directory, CPath & File );
	void  AddRomToList            ( const char * RomLocation, const char * lpLastRom );
	void  AddRomInfoToList        ( ROM_INFO &RomInfo, const char * lpLastRom );
	void  AllocateBrushs          ( void );
	DWORD AsciiToHex              ( char * HexValue );
	static void  ByteSwapRomData         ( BYTE * Data, int DataLen );
	int   CalcSortPosition        ( DWORD lParam ); 
	void  CreateRomListControl    ( void );
	void  DeallocateBrushs        ( void );
	void  FillRomExtensionInfo    ( ROM_INFO * pRomInfo );
	bool  FillRomInfo             ( ROM_INFO * pRomInfo );
	void  FillRomList             ( strlist & FileList, const CPath & BaseDirectory, const stdstr & Directory, const char * lpLastRom );
	void  FixRomListWindow        ( void );
	static int   GetCicChipID     ( BYTE * RomData );
	bool  LoadDataFromRomFile     ( char * FileName, BYTE * Data,int DataLen, int * RomSize, FILE_FORMAT & FileFormat );
	void  LoadRomList             ( void );
	void  MenuSetText             ( HMENU hMenu, int MenuPos, const wchar_t * Title, char * ShotCut);
	void  SaveRomList             ( strlist & FileList );
	void  RomList_ColoumnSortList ( DWORD pnmh );
	void  RomList_GetDispInfo     ( DWORD pnmh );
	void  RomList_OpenRom         ( DWORD pnmh );
	void  RomList_PopupMenu       ( DWORD pnmh );
	void  RomList_SortList        ( void );
	bool  GetRomFileNames         ( strlist & FileList, const CPath & BaseDirectory, const stdstr & Directory, bool InWatchThread );
	MD5   RomListHash             ( strlist & FileList );

	static void  __stdcall NotificationCB ( LPCWSTR Status, CRomBrowser * _this );

	//Watch Directory Changed function
	HANDLE m_WatchThread, m_WatchStopEvent;
	DWORD  m_WatchThreadID;
	stdstr m_WatchRomDir;
	void WatchThreadStart (void);
	void WatchThreadStop (void);
	bool RomDirNeedsRefresh ( void ); // Called from watch thread
	static void WatchRomDirChanged ( CRomBrowser * _this );
	static void RefreshRomBrowserStatic ( CRomBrowser * _this );
	static void AddField (ROMBROWSER_FIELDS_LIST & Fields, LPCSTR Name, int Pos,int ID,int Width,LanguageStringID LangID, bool UseDefault);

	//Callback
	static int CALLBACK SelectRomDirCallBack ( HWND hwnd,DWORD uMsg,DWORD lp, DWORD lpData );
	static int CALLBACK RomList_CompareItems ( DWORD lParam1, DWORD lParam2, DWORD lParamSort );

public:
	      CRomBrowser             ( HWND & hMainWindow, HWND & StatusWindow );
	     ~CRomBrowser             ( void );
	void  HighLightLastRom        ( void );
	void  HideRomList             ( void );
	void  RefreshRomBrowser       ( void );
	void  ResetRomBrowserColomuns ( void );
	void  ResizeRomList           ( WORD nWidth, WORD nHeight );
	void  RomBrowserToTop         ( void );
	void  RomBrowserMaximize      ( bool Mazimize ) ;
	bool  RomBrowserVisible       ( void );
	bool  RomListDrawItem         ( int idCtrl, DWORD lParam );
	bool  RomListNotify           ( int idCtrl, DWORD pnmh );
	void  SaveRomListColoumnInfo  ( void );
	void  SelectRomDir            ( void );
	void  ShowRomList             ( void );
	bool  ShowingRomBrowser       ( void ) { return m_ShowingRomBrowser; } 
	LPCSTR CurrentedSelectedRom   ( void ) { return m_SelectedRom.c_str(); }
	void  Store7ZipInfo           ( C7zip & ZipFile, int FileNo );

	static void GetFieldInfo      ( ROMBROWSER_FIELDS_LIST & Fields, bool UseDefault = false );
};
