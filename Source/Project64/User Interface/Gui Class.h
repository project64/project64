class CGfxPlugin;      //Plugin that controls the rendering
class CAudioPlugin;    //Plugin for audio, need the hwnd
class CControl_Plugin; //Controller needs hwnd to see if it is the focused window
class CBaseMenu;           //Menu for the gui
class CN64System; 
class CNotification;
class CriticalSection;

enum { 
	WM_HIDE_CUROSR   = WM_USER + 10,
	WM_MAKE_FOCUS    = WM_USER + 17,
	WM_INIATE_PLUGIN = WM_USER + 18,
	WM_BORWSER_TOP   = WM_USER + 40,
};

class CMainGui :
	public CRomBrowser,
	private CGuiSettings
{
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CControl_Plugin;

	CBaseMenu     * m_Menu;
		
	enum { StatusBarID = 400 };
	
	WND_HANDLE  m_hMainWindow, m_hStatusWnd;
	bool        m_hacked;
	const bool  m_bMainWindow;
	DWORD       m_InvalidExeMsg;
	CriticalSection m_CS;

	bool        m_SaveMainWindowPos;
	LONG        m_SaveMainWindowTop;
	LONG        m_SaveMainWindowLeft;
	
	bool        m_SaveRomBrowserPos;
	LONG        m_SaveRomBrowserTop;
	LONG        m_SaveRomBrowserLeft;
	
	bool RegisterWinClass ( void );
	void ChangeWinSize    ( long width, long height );
	void Create           ( const char * WindowTitle );
	void CreateStatusBar  ( void );
	void Resize           ( DWORD fwSizeType, WORD nWidth, WORD nHeight ); //responding to WM_SIZE

	friend DWORD CALLBACK AboutBoxProc ( HWND, DWORD, DWORD, DWORD );
	friend DWORD CALLBACK AboutIniBoxProc ( WND_HANDLE, DWORD, DWORD, DWORD );
	static DWORD CALLBACK MainGui_Proc ( WND_HANDLE, DWORD, DWORD, DWORD );

	friend void RomBowserEnabledChanged  (CMainGui * Gui);
	friend void RomBowserColoumnsChanged (CMainGui * Gui);
	friend void RomBrowserRecursiveChanged (CMainGui * Gui);

public:
		 CMainGui ( bool bMainWindow, const char * WindowTitle = "" );
		~CMainGui ( void );
	
	//Message Processing	 
	int  ProcessAllMessages ( void );
	bool ProcessGuiMessages ( void );

	//debugging functions
	void EnterLogOptions ( void );

	//Get Information about the window
	int Height    ( void ); //Get the Height of the window
	int Width     ( void ); //Get the Width of the window

	//Manipulate the state of the window
	void SetPos          ( int X, int Y );    //Move the window to this screen location
	void Show            ( bool ShowWindow ); //Show or Hide the current window
	void MakeWindowOnTop ( bool OnTop );
	void BringToTop      ( void );
	void Caption         ( LPCSTR Caption );  //Set the caption of the window
	void SaveWindowLoc   ( void );

	//Menu Function
	void SetWindowMenu  ( CBaseMenu * Menu );
	void RefreshMenu    ( void );
	CBaseMenu * GetMenuClass ( void ) { return m_Menu; }

	// Status bar
	void SetStatusText  ( int Panel,const char * Text );
	void ShowStatusBar  ( bool ShowBar );

	//About Window
	void AboutIniBox ( void );
	void AboutBox ( void );

	//Plugins
	bool InitiatePlugins ( void );

	//Get Window Handle
	inline WND_HANDLE GetHandle ( void ) const { return m_hMainWindow; }

};
