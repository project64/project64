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

class CGfxPlugin  
{
	typedef struct {
		/* Menu */
		/* Items should have an ID between 5101 and 5200 */
		HMENU hGFXMenu;
		void (__cdecl *ProcessMenuItem) ( int ID );

		/* Break Points */
		BOOL UseBPoints;
		char BPPanelName[20];
		void (__cdecl *Add_BPoint)      ( void );
		void (__cdecl *CreateBPPanel)   ( HWND hDlg, RECT_STRUCT rcBox );
		void (__cdecl *HideBPPanel)     ( void );
		void (__cdecl *PaintBPPanel)    ( WINDOWS_PAINTSTRUCT ps );
		void (__cdecl *ShowBPPanel)     ( void );
		void (__cdecl *RefreshBpoints)  ( HWND hList );
		void (__cdecl *RemoveBpoint)    ( HWND hList, int index );
		void (__cdecl *RemoveAllBpoint) ( void );
		
		/* GFX command Window */
		void (__cdecl *Enter_GFX_Commands_Window) ( void );
	} GFXDEBUG_INFO;

	typedef struct {
		void (__cdecl *UpdateBreakPoints)( void );
		void (__cdecl *UpdateMemory)( void );
		void (__cdecl *UpdateR4300iRegisters)( void );
		void (__cdecl *Enter_BPoint_Window)( void );
		void (__cdecl *Enter_R4300i_Commands_Window)( void );
		void (__cdecl *Enter_R4300i_Register_Window)( void );
		void (__cdecl *Enter_RSP_Commands_Window) ( void );
		void (__cdecl *Enter_Memory_Window)( void );
	} DEBUG_INFO;

public:
	CGfxPlugin  ( const char * FileName);
	~CGfxPlugin ( void );

	bool Initiate   ( CN64System * System, CMainGui * RenderWindow );
	bool Initilized ( void ) { return m_Initilized; }
	void Close      ( void );
	void RomOpened  ( void );
	void RomClose   ( void );
	void GameReset  ( void );
	stdstr PluginName ( void ) const { return m_PluginInfo.Name; }

	void (__cdecl *CaptureScreen)      ( const char * );
	void (__cdecl *ChangeWindow)       ( void );
	void (__cdecl *Config)          ( DWORD hParent );
	void (__cdecl *DrawScreen)         ( void );
	void (__cdecl *DrawStatus)         ( const char * lpString, BOOL RightAlign );
	void (__cdecl *MoveScreen)         ( int xpos, int ypos );
	void (__cdecl *ProcessDList)       ( void );
	void (__cdecl *ProcessRDPList)     ( void );
	void (__cdecl *ShowCFB)			   ( void );
	void (__cdecl *UpdateScreen)       ( void );
	void (__cdecl *ViStatusChanged)    ( void );
	void (__cdecl *ViWidthChanged)     ( void );
	void (__cdecl *SoftReset)          ( void );

	//Rom Browser
	HMENU (__cdecl * GetRomBrowserMenu)  ( void ); /* Items should have an ID between 4101 and 4200 */
	void (__cdecl * OnRomBrowserMenuItem) ( int MenuID, HWND hParent, BYTE * HEADER );
	
	HMENU GetDebugMenu (void ) { return m_GFXDebug.hGFXMenu; }
	void ProcessMenuItem (int id );

private:
	CGfxPlugin(void);							// Disable default constructor
	CGfxPlugin(const CGfxPlugin&);				// Disable copy constructor
	CGfxPlugin& operator=(const CGfxPlugin&);	// Disable assignment

	void Init ( const char * FileName );
	void UnloadPlugin         ( void );

	GFXDEBUG_INFO m_GFXDebug;
	void * m_hDll;	
	bool m_Initilized, m_RomOpen;
	PLUGIN_INFO m_PluginInfo;

	void (__cdecl *CloseDLL)         ( void );
	void (__cdecl *RomOpen)          ( void );
	void (__cdecl *RomClosed)        ( void );
	void (__cdecl *GetDebugInfo)     ( GFXDEBUG_INFO * GFXDebugInfo );
	void (__cdecl *InitiateDebugger) ( DEBUG_INFO DebugInfo);
	void (__cdecl *PluginOpened)     ( void );
	void (__cdecl *SetSettingInfo)   ( PLUGIN_SETTINGS * info );
	void (__cdecl *SetSettingInfo2)  ( PLUGIN_SETTINGS2 * info );
	void (__cdecl *SetSettingInfo3)  ( PLUGIN_SETTINGS3 * info );

	static void __cdecl DummyDrawScreen      ( void ) {}
	static void __cdecl DummyMoveScreen      ( int /*xpos*/, int /*ypos*/ ) {}
	static void __cdecl DummyViStatusChanged ( void ) {}
	static void __cdecl DummyViWidthChanged  ( void ) {}
	static void __cdecl DummySoftReset       ( void ) {}
};
