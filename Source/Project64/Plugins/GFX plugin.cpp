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
#include "stdafx.h"

void FixUPXIssue ( BYTE * ProgramLocation );

CGfxPlugin::CGfxPlugin ( const char * FileName) :
	CaptureScreen(NULL),
	ChangeWindow(NULL),
	Config(NULL),
	DrawScreen(NULL),
	DrawStatus(NULL),
	MoveScreen(NULL),
	ProcessDList(NULL),
	ProcessRDPList(NULL),
	ShowCFB(NULL),
	UpdateScreen(NULL),
	ViStatusChanged(NULL),
	ViWidthChanged(NULL),
	SoftReset(NULL),
	GetRomBrowserMenu(NULL),
	OnRomBrowserMenuItem(NULL),
	CloseDLL(NULL),
	RomOpen(NULL),
	RomClosed(NULL),
	GetDebugInfo(NULL),
	InitiateDebugger(NULL),
	PluginOpened(NULL),
	SetSettingInfo(NULL),
	SetSettingInfo2(NULL),
	m_hDll(NULL),	
	m_Initilized(false), 
	m_RomOpen(false)
{
	memset(&m_GFXDebug,0,sizeof(m_GFXDebug));
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));
	Init(FileName);
}

void CGfxPlugin::Init ( const char * FileName )
{
	//Try to load the DLL library
	UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
	m_hDll = LoadLibrary(FileName);
	SetErrorMode(LastErrorMode);
	
	if (m_hDll == NULL) { 
		UnloadPlugin();
		return;
	}
	FixUPXIssue((BYTE *)m_hDll);

	//Get DLL information
	void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );
	GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( (HMODULE)m_hDll, "GetDllInfo" );
	if (GetDllInfo == NULL) { UnloadPlugin(); return; }

	GetDllInfo(&m_PluginInfo);
	if (!CPluginList::ValidPluginVersion(m_PluginInfo)) { UnloadPlugin(); return; }

	//Find entries for functions in DLL
	BOOL (__cdecl *InitFunc) ( void * Gfx_Info );
	CloseDLL        = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "CloseDLL" );
	ChangeWindow    = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "ChangeWindow" );
	Config          = (void (__cdecl *)(DWORD))   GetProcAddress( (HMODULE)m_hDll, "DllConfig" );
	DrawScreen      = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "DrawScreen" );
	InitFunc        = (BOOL (__cdecl *)(void *))  GetProcAddress( (HMODULE)m_hDll, "InitiateGFX" );
	MoveScreen      = (void (__cdecl *)(int, int))GetProcAddress( (HMODULE)m_hDll, "MoveScreen" );
	ProcessDList    = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "ProcessDList" );
	RomClosed       = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "RomClosed" );
	RomOpen         = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "RomOpen" );
	UpdateScreen    = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "UpdateScreen" );
	ViStatusChanged = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "ViStatusChanged" );
	ViWidthChanged  = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "ViWidthChanged" );
	SoftReset       = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)m_hDll, "SoftReset" );

	//version 104 functions
	PluginOpened     = (void (__cdecl *)(void))GetProcAddress( (HMODULE)m_hDll, "PluginLoaded" );
	DrawStatus       = (void (__cdecl *)(const char *, BOOL ))GetProcAddress((HMODULE)m_hDll, "DrawFullScreenStatus");
	
	// Rom Browser
	GetRomBrowserMenu    = (HMENU (__cdecl *)( void ))GetProcAddress( (HMODULE)m_hDll, "GetRomBrowserMenu" );
	OnRomBrowserMenuItem = (void (__cdecl *) ( int, HWND, BYTE * ))GetProcAddress( (HMODULE)m_hDll, "OnRomBrowserMenuItem" );

	//Make sure dll had all needed functions
	if (ChangeWindow == NULL)    { UnloadPlugin(); return; }
	if (DrawScreen == NULL)      { DrawScreen = DummyDrawScreen; }
	if (InitFunc == NULL)        { UnloadPlugin(); return; }
	if (MoveScreen == NULL)      { MoveScreen = DummyMoveScreen; }
	if (ProcessDList == NULL)    { UnloadPlugin(); return; }
	if (RomClosed == NULL)       { UnloadPlugin(); return; }
	if (RomOpen == NULL)         { UnloadPlugin(); return; }
	if (UpdateScreen == NULL)    { UnloadPlugin(); return; }
	if (ViStatusChanged == NULL) { ViStatusChanged = DummyViStatusChanged; }
	if (ViWidthChanged == NULL)  { ViWidthChanged = DummyViWidthChanged; }	
	if (CloseDLL == NULL)        { UnloadPlugin(); return; }
	if (SoftReset == NULL)       { SoftReset = DummySoftReset; }
	
	if (m_PluginInfo.Version >= 0x0103 ){
		ProcessRDPList   = (void (__cdecl *)(void))GetProcAddress( (HMODULE)m_hDll, "ProcessRDPList" );
		CaptureScreen    = (void (__cdecl *)(const char *))GetProcAddress( (HMODULE)m_hDll, "CaptureScreen" );
		ShowCFB          = (void (__cdecl *)(void))GetProcAddress( (HMODULE)m_hDll, "ShowCFB" );
		GetDebugInfo     = (void (__cdecl *)(GFXDEBUG_INFO *))GetProcAddress( (HMODULE)m_hDll, "GetGfxDebugInfo" );
		InitiateDebugger = (void (__cdecl *)(DEBUG_INFO))GetProcAddress( (HMODULE)m_hDll, "InitiateGFXDebugger" );

		if (ProcessRDPList == NULL) { UnloadPlugin(); return; }
		if (CaptureScreen == NULL)  { UnloadPlugin(); return; }
		if (ShowCFB == NULL)        { UnloadPlugin(); return; }

		if (GetDebugInfo != NULL) { GetDebugInfo(&m_GFXDebug); }

	}
	
	SetSettingInfo3 = (void (__cdecl *)(PLUGIN_SETTINGS3 *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo3" );
	if (SetSettingInfo3)
	{
		PLUGIN_SETTINGS3 info;
		info.FlushSettings = (void (*)( void * handle))CSettings::FlushSettings;
		SetSettingInfo3(&info);
	}

	SetSettingInfo2 = (void (__cdecl *)(PLUGIN_SETTINGS2 *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo2" );
	if (SetSettingInfo2)
	{
		PLUGIN_SETTINGS2 info;
		info.FindSystemSettingId = (unsigned int (*)( void * handle, const char * ))CSettings::FindSetting;
		SetSettingInfo2(&info);
	}

	SetSettingInfo   = (void (__cdecl *)(PLUGIN_SETTINGS *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo" );
	if (SetSettingInfo)
	{
		PLUGIN_SETTINGS info;
		info.dwSize = sizeof(PLUGIN_SETTINGS);
		info.DefaultStartRange = FirstGfxDefaultSet;
		info.SettingStartRange = FirstGfxSettings;
		info.MaximumSettings   = MaxPluginSetting;
		info.NoDefault         = Default_None;
		info.DefaultLocation   = g_Settings->LoadDword(Setting_UseFromRegistry) ? SettingType_Registry : SettingType_CfgFile;
		info.handle            = g_Settings;
		info.RegisterSetting   = (void (*)(void *,int,int,SettingDataType,SettingType,const char *,const char *, DWORD))CSettings::RegisterSetting;
		info.GetSetting        = (unsigned int (*)( void * handle, int ID ))CSettings::GetSetting;
		info.GetSettingSz      = (const char * (*)( void *, int, char *, int ))CSettings::GetSettingSz;
		info.SetSetting        = (void (*)(void *,int,unsigned int))CSettings::SetSetting;
		info.SetSettingSz      = (void (*)(void *,int,const char *))CSettings::SetSettingSz;
		info.UseUnregisteredSetting = NULL;

		SetSettingInfo(&info);
//		g_Settings->UnknownSetting_GFX = info.UseUnregisteredSetting;
	}

	if (m_PluginInfo.Version >= 0x0104)
	{
		if (PluginOpened    == NULL) { UnloadPlugin(); return; }

		PluginOpened();
	}
//	FrameBufferRead = (void (__cdecl *)(DWORD))GetProcAddress( (HMODULE)m_hDll, "FBRead" );
//	FrameBufferWrite = (void (__cdecl *)(DWORD, DWORD))GetProcAddress( (HMODULE)m_hDll, "FBWrite" );

}

CGfxPlugin::~CGfxPlugin (void) {
	Close();
	UnloadPlugin();
}

bool CGfxPlugin::Initiate ( CN64System * System, CMainGui * RenderWindow ) 
{
	if (m_Initilized)
	{
		Close();
	}

	typedef struct {
		HWND hWnd;			/* Render window */
		HWND hStatusBar;    /* if render window does not have a status bar then this is NULL */

		BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
							   //   bswap on a dword (32 bits) boundry 
							   //	eg. the first 8 bytes are stored like this:
							   //        4 3 2 1   8 7 6 5

		BYTE * HEADER;	// This is the rom header (first 40h bytes of the rom
						// This will be in the same memory format as the rest of the memory.
		BYTE * RDRAM;
		BYTE * DMEM;
		BYTE * IMEM;

		DWORD * MI__INTR_REG;

		DWORD * DPC__START_REG;
		DWORD * DPC__END_REG;
		DWORD * DPC__CURRENT_REG;
		DWORD * DPC__STATUS_REG;
		DWORD * DPC__CLOCK_REG;
		DWORD * DPC__BUFBUSY_REG;
		DWORD * DPC__PIPEBUSY_REG;
		DWORD * DPC__TMEM_REG;

		DWORD * VI__STATUS_REG;
		DWORD * VI__ORIGIN_REG;
		DWORD * VI__WIDTH_REG;
		DWORD * VI__INTR_REG;
		DWORD * VI__V_CURRENT_LINE_REG;
		DWORD * VI__TIMING_REG;
		DWORD * VI__V_SYNC_REG;
		DWORD * VI__H_SYNC_REG;
		DWORD * VI__LEAP_REG;
		DWORD * VI__H_START_REG;
		DWORD * VI__V_START_REG;
		DWORD * VI__V_BURST_REG;
		DWORD * VI__X_SCALE_REG;
		DWORD * VI__Y_SCALE_REG;

		void (__cdecl *CheckInterrupts)( void );
	} GFX_INFO;

	//Get Function from DLL
	BOOL (__cdecl *InitiateGFX)( GFX_INFO Gfx_Info );
	InitiateGFX = (BOOL (__cdecl *)(GFX_INFO))GetProcAddress( (HMODULE)m_hDll, "InitiateGFX" );
	if (InitiateGFX == NULL) { return false; }

	GFX_INFO Info;
	memset(&Info,0,sizeof(Info));
	//We are initilizing the plugin before any rom is loaded so we do not have any correct
	//paramaters here .. just needed to we can config the DLL
	if (System == NULL) {
		BYTE Buffer[100];
		DWORD Value = 0;

		Info.MemoryBswaped          = TRUE;
		Info.hWnd                   = (HWND)RenderWindow->m_hMainWindow;
		Info.hStatusBar             = (HWND)RenderWindow->m_hStatusWnd;
		Info.CheckInterrupts        = DummyCheckInterrupts;
		Info.HEADER                 = Buffer;
		Info.RDRAM                  = Buffer;
		Info.DMEM                   = Buffer;
		Info.IMEM                   = Buffer;
		Info.MI__INTR_REG           = &Value;	
		Info.VI__STATUS_REG         = &Value;
		Info.VI__ORIGIN_REG         = &Value;
		Info.VI__WIDTH_REG          = &Value;
		Info.VI__INTR_REG           = &Value;
		Info.VI__V_CURRENT_LINE_REG = &Value;
		Info.VI__TIMING_REG         = &Value;
		Info.VI__V_SYNC_REG         = &Value;
		Info.VI__H_SYNC_REG         = &Value;
		Info.VI__LEAP_REG           = &Value;
		Info.VI__H_START_REG        = &Value;
		Info.VI__V_START_REG        = &Value;
		Info.VI__V_BURST_REG        = &Value;
		Info.VI__X_SCALE_REG        = &Value;
		Info.VI__Y_SCALE_REG        = &Value;
		m_Initilized = InitiateGFX(Info) != 0;
		//jabo had a bug so I call CreateThread so his dllmain gets called again
		DWORD ThreadID;
		HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
		CloseHandle(hthread);
		Sleep(100);
		return m_Initilized;
	}
	
	//Send Initilization information to the DLL
	Info.MemoryBswaped          = TRUE;
	Info.CheckInterrupts        = DummyCheckInterrupts;
	Info.hWnd                   = (HWND)RenderWindow->m_hMainWindow;
	Info.hStatusBar             = (HWND)RenderWindow->m_hStatusWnd;
	Info.HEADER                 = g_Rom->GetRomAddress();
	Info.RDRAM                  = g_MMU->Rdram();
	Info.DMEM                   = g_MMU->Dmem();
	Info.IMEM                   = g_MMU->Imem();
	Info.MI__INTR_REG           = &g_Reg->m_GfxIntrReg;	
	Info.DPC__START_REG         = &g_Reg->DPC_START_REG;
	Info.DPC__END_REG           = &g_Reg->DPC_END_REG;
	Info.DPC__CURRENT_REG       = &g_Reg->DPC_CURRENT_REG;
	Info.DPC__STATUS_REG        = &g_Reg->DPC_STATUS_REG;
	Info.DPC__CLOCK_REG         = &g_Reg->DPC_CLOCK_REG;
	Info.DPC__BUFBUSY_REG       = &g_Reg->DPC_BUFBUSY_REG;
	Info.DPC__PIPEBUSY_REG      = &g_Reg->DPC_PIPEBUSY_REG;
	Info.DPC__TMEM_REG          = &g_Reg->DPC_TMEM_REG;
	Info.VI__STATUS_REG         = &g_Reg->VI_STATUS_REG;
	Info.VI__ORIGIN_REG         = &g_Reg->VI_ORIGIN_REG;
	Info.VI__WIDTH_REG          = &g_Reg->VI_WIDTH_REG;
	Info.VI__INTR_REG           = &g_Reg->VI_INTR_REG;
	Info.VI__V_CURRENT_LINE_REG = &g_Reg->VI_CURRENT_REG;
	Info.VI__TIMING_REG         = &g_Reg->VI_TIMING_REG;
	Info.VI__V_SYNC_REG         = &g_Reg->VI_V_SYNC_REG;
	Info.VI__H_SYNC_REG         = &g_Reg->VI_H_SYNC_REG;
	Info.VI__LEAP_REG           = &g_Reg->VI_LEAP_REG;
	Info.VI__H_START_REG        = &g_Reg->VI_H_START_REG;
	Info.VI__V_START_REG        = &g_Reg->VI_V_START_REG;
	Info.VI__V_BURST_REG        = &g_Reg->VI_V_BURST_REG;
	Info.VI__X_SCALE_REG        = &g_Reg->VI_X_SCALE_REG;
	Info.VI__Y_SCALE_REG        = &g_Reg->VI_Y_SCALE_REG;
	
	m_Initilized = InitiateGFX(Info) != 0;
	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
	CloseHandle(hthread);
	
	return m_Initilized;
}

void CGfxPlugin::Close(void) {
	if (m_RomOpen) 
	{
		RomClose();
	}
	if (m_Initilized) 
	{
		CloseDLL();
		m_Initilized = false;
	}
}

void CGfxPlugin::RomOpened  ( void )
{
	//Real system ... then make the file as open
	if (!m_RomOpen)
	{
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Before RomOpen");
		RomOpen();
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": After RomOpen");
		m_RomOpen = true;
	}
}

void CGfxPlugin::RomClose  ( void )
{
	if (m_RomOpen)
	{
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Before RomClosed");
		RomClosed();
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": After RomClosed");
		m_RomOpen = false;
	}
}

void CGfxPlugin::GameReset(void)
{
	if (m_RomOpen) 
	{
		RomClose();
		RomOpened();
	}
}

void CGfxPlugin::UnloadPlugin(void) {
	if (m_hDll != NULL ) {
		FreeLibrary((HMODULE)m_hDll);
		m_hDll = NULL;
	}
	memset(&m_GFXDebug,0,sizeof(m_GFXDebug));
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));

//	CaptureScreen        = NULL;
	ChangeWindow         = NULL;
	GetDebugInfo         = NULL;
	CloseDLL             = NULL;
//	DllAbout             = NULL;
	Config               = NULL;
	RomClosed            = NULL;
	RomOpen              = NULL;
	DrawScreen           = NULL;
	DrawStatus           = NULL;
//	FrameBufferRead      = NULL;
//	FrameBufferWrite     = NULL;
	InitiateDebugger     = NULL;
	MoveScreen           = NULL;
	ProcessDList         = NULL;
	ProcessRDPList       = NULL;
	ShowCFB              = NULL;
	UpdateScreen         = NULL;
	ViStatusChanged      = NULL;
	ViWidthChanged       = NULL;
	GetRomBrowserMenu    = NULL;
	OnRomBrowserMenuItem = NULL;
}

void CGfxPlugin::ProcessMenuItem (int id )
{
	if (m_GFXDebug.ProcessMenuItem)
	{
		m_GFXDebug.ProcessMenuItem(id); 
	}
}
