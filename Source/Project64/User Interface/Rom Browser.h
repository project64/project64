#include <vector>

class CMainGui;
class CNotification;
class CPlugins;

class ROMBROWSER_FIELDS {
public:
	ROMBROWSER_FIELDS (char * Name, int Pos, int ID, int ColWidth, LanguageStringID LangID) {
		strncpy(this->Name,Name,sizeof(this->Name));
		this->Name[sizeof(this->Name) - 1] = 0;
		this->Pos      = Pos;
		this->ID       = ID;
		this->ColWidth = ColWidth;
		this->LangID   = LangID;
	}
	char Name[50];
	int  Pos;
	int  ID;
	int  ColWidth;
	LanguageStringID  LangID;
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
class CRomBrowser {
	CNotification * _Notify;

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

	CN64System           * _System;
	WND_HANDLE           & m_MainWindow;
	WND_HANDLE           & m_StatusWindow;
	WND_HANDLE             m_hRomList;
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
	CPlugins             * m_Plugins;
	bool                   m_AllowSelectionLastRom;

	void  AddFileNameToList       ( strlist & FileList, stdstr & Directory, CPath & File );
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
	bool  FillRomInfo2            ( ROM_INFO * pRomInfo, BYTE * RomData, DWORD RomDataSize );
	void  FillRomList             ( strlist & FileList, CPath & BaseDirectory, stdstr & Directory, const char * lpLastRom );
	void  FixRomListWindow        ( void );
	static int   GetCicChipID     ( BYTE * RomData );
	static bool  IsValidRomImage  ( BYTE Test[4] );
	bool  LoadDataFromRomFile     ( char * FileName, BYTE * Data,int DataLen, int * RomSize, FILE_FORMAT & FileFormat );
	void  LoadRomList             ( void );
	void  MenuSetText             ( MENU_HANDLE hMenu, int MenuPos, const char * Title, char * ShotCut);
	void  SaveRomList             ( strlist & FileList );
	void  RomList_ColoumnSortList ( DWORD pnmh );
	void  RomList_GetDispInfo     ( DWORD pnmh );
	void  RomList_OpenRom         ( DWORD pnmh );
	void  RomList_PopupMenu       ( DWORD pnmh );
	void  RomList_SortList        ( void );
	void  GetRomFileNames         ( strlist & FileList, CPath & BaseDirectory, stdstr & Directory, bool InWatchThread );
	MD5   RomListHash             ( strlist & FileList );

	static void  __stdcall NotificationCB ( LPCSTR Status, CRomBrowser * _this );

	//Watch Directory Changed function
	HANDLE m_WatchThread, m_WatchStopEvent;
	DWORD  m_WatchThreadID;
	stdstr m_WatchRomDir;
	void WatchThreadStart (void);
	void WatchThreadStop (void);
	bool RomDirNeedsRefresh ( void ); // Called from watch thread
	static void WatchRomDirChanged ( CRomBrowser * _this );
	static void RefreshRomBrowserStatic ( CRomBrowser * _this );

	//Callback
	static int CALLBACK SelectRomDirCallBack ( WND_HANDLE hwnd,DWORD uMsg,DWORD lp, DWORD lpData );
	static int CALLBACK RomList_CompareItems ( DWORD lParam1, DWORD lParam2, DWORD lParamSort );

	//needs to access internal information for configuration
	friend int CALLBACK RomBrowserConfigProc ( DWORD, DWORD, DWORD, DWORD );
public:
	      CRomBrowser             ( WND_HANDLE & hMainWindow, WND_HANDLE & StatusWindow, CNotification * Notify, CN64System * System );
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
	void  SelectRomDir            ( CNotification * Notify );
	void  ShowRomList             ( void );
	bool  ShowingRomBrowser       ( void ) { return m_ShowingRomBrowser; } 
	LPCSTR CurrentedSelectedRom   ( void ) { return m_SelectedRom.c_str(); }
	void  SetPluginList           ( CPlugins * Plugins);
	static void Store7ZipInfo     ( CSettings * Settings, C7zip & ZipFile, int FileNo );
};