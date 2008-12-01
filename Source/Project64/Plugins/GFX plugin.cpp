#include "..\Plugin.h"
#include <windows.h>

void FixUPXIssue ( BYTE * ProgramLocation );

CGfxPlugin::CGfxPlugin ( const char * FileName) {
	//Make sure all parts of the class are initialized
	m_Initilized = false;
	m_RomOpen    = false;
	hDll         = NULL;
	UnloadPlugin();

	//Try to load the DLL library
	UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
	hDll = LoadLibrary(FileName);
	SetErrorMode(LastErrorMode);
	
	if (hDll == NULL) { 
		UnloadPlugin();
		return;
	}
	FixUPXIssue((BYTE *)hDll);

	//Get DLL information
	void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );
	GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( (HMODULE)hDll, "GetDllInfo" );
	if (GetDllInfo == NULL) { UnloadPlugin(); return; }

	GetDllInfo(&m_PluginInfo);
	if (!ValidPluginVersion(&m_PluginInfo)) { UnloadPlugin(); return; }

	//Find entries for functions in DLL
	BOOL (__cdecl *InitFunc) ( void * Gfx_Info );
	CloseDLL        = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "CloseDLL" );
	ChangeWindow    = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "ChangeWindow" );
	Config          = (void (__cdecl *)(DWORD))   GetProcAddress( (HMODULE)hDll, "DllConfig" );
	DrawScreen      = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "DrawScreen" );
	InitFunc        = (BOOL (__cdecl *)(void *))  GetProcAddress( (HMODULE)hDll, "InitiateGFX" );
	MoveScreen      = (void (__cdecl *)(int, int))GetProcAddress( (HMODULE)hDll, "MoveScreen" );
	ProcessDList    = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "ProcessDList" );
	RomClosed       = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "RomClosed" );
	RomOpen         = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "RomOpen" );
	UpdateScreen    = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "UpdateScreen" );
	ViStatusChanged = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "ViStatusChanged" );
	ViWidthChanged  = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "ViWidthChanged" );
	SoftReset       = (void (__cdecl *)(void))    GetProcAddress( (HMODULE)hDll, "SoftReset" );

	//version 104 functions
	PluginOpened     = (void (__cdecl *)(void))GetProcAddress( (HMODULE)hDll, "PluginLoaded" );
	DrawStatus       = (void (__cdecl *)(const char *, BOOL ))GetProcAddress((HMODULE)hDll, "DrawFullScreenStatus");
	
	// Rom Browser
	GetRomBrowserMenu    = (MENU_HANDLE (__cdecl *)( void ))GetProcAddress( (HMODULE)hDll, "GetRomBrowserMenu" );
	OnRomBrowserMenuItem = (void (__cdecl *) ( int, WND_HANDLE, BYTE * ))GetProcAddress( (HMODULE)hDll, "OnRomBrowserMenuItem" );

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
		ProcessRDPList   = (void (__cdecl *)(void))GetProcAddress( (HMODULE)hDll, "ProcessRDPList" );
		CaptureScreen    = (void (__cdecl *)(const char *))GetProcAddress( (HMODULE)hDll, "CaptureScreen" );
		ShowCFB          = (void (__cdecl *)(void))GetProcAddress( (HMODULE)hDll, "ShowCFB" );
		GetDebugInfo     = (void (__cdecl *)(GFXDEBUG_INFO *))GetProcAddress( (HMODULE)hDll, "GetGfxDebugInfo" );
		InitiateDebugger = (void (__cdecl *)(DEBUG_INFO))GetProcAddress( (HMODULE)hDll, "InitiateGFXDebugger" );

		if (ProcessRDPList == NULL) { UnloadPlugin(); return; }
		if (CaptureScreen == NULL)  { UnloadPlugin(); return; }
		if (ShowCFB == NULL)        { UnloadPlugin(); return; }

		if (GetDebugInfo != NULL) { GetDebugInfo(&m_GFXDebug); }

	}
	
	SetSettingInfo   = (void (__cdecl *)(PLUGIN_SETTINGS *))GetProcAddress( (HMODULE)hDll, "SetSettingInfo" );
	if (SetSettingInfo)
	{
		PLUGIN_SETTINGS info;
		info.dwSize = sizeof(PLUGIN_SETTINGS);
		info.DefaultStartRange = FirstGfxDefaultSet;
		info.SettingStartRange = FirstGfxSettings;
		info.MaximumSettings   = MaxPluginSetting;
		info.NoDefault         = Default_None;
		info.DefaultLocation   = _Settings->LoadDword(Setting_UseFromRegistry) ? SettingType_Registry : SettingType_CfgFile;
		info.handle            = _Settings;
		info.RegisterSetting   = (void (*)(void *,int,int,SettingDataType,SettingType,const char *,const char *, DWORD))CSettings::RegisterSetting;
		info.GetSetting        = (unsigned int (*)( void * handle, int ID ))CSettings::GetSetting;
		info.GetSettingSz      = (const char * (*)( void *, int, char *, int ))CSettings::GetSettingSz;
		info.SetSetting        = (void (*)(void *,int,unsigned int))CSettings::SetSetting;
		info.SetSettingSz      = (void (*)(void *,int,const char *))CSettings::SetSettingSz;
		info.UseUnregisteredSetting = NULL;

		SetSettingInfo(&info);
//		_Settings->UnknownSetting_GFX = info.UseUnregisteredSetting;
	}

	if (m_PluginInfo.Version >= 0x0104)
	{
		if (PluginOpened    == NULL) { UnloadPlugin(); return; }

		PluginOpened();
	}
//	FrameBufferRead = (void (__cdecl *)(DWORD))GetProcAddress( (HMODULE)hDll, "FBRead" );
//	FrameBufferWrite = (void (__cdecl *)(DWORD, DWORD))GetProcAddress( (HMODULE)hDll, "FBWrite" );

}

CGfxPlugin::~CGfxPlugin (void) {
	Close();
	UnloadPlugin();
}

bool CGfxPlugin::Initiate ( CN64System * System, CMainGui * RenderWindow ) {
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
	InitiateGFX = (BOOL (__cdecl *)(GFX_INFO))GetProcAddress( (HMODULE)hDll, "InitiateGFX" );
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
	CRegisters * _Reg = System->_MMU->_Reg;	
	Info.MemoryBswaped          = TRUE;
	Info.CheckInterrupts        = DummyCheckInterrupts;
	Info.hWnd                   = (HWND)RenderWindow->m_hMainWindow;
	Info.hStatusBar             = (HWND)RenderWindow->m_hStatusWnd;
	Info.HEADER                 = System->_MMU->ROM;
	Info.RDRAM                  = System->_MMU->RDRAM;
	Info.DMEM                   = System->_MMU->DMEM;
	Info.IMEM                   = System->_MMU->IMEM;
	Info.MI__INTR_REG           = &_Reg->MI_INTR_REG;	
	Info.DPC__START_REG         = &_Reg->DPC_START_REG;
	Info.DPC__END_REG           = &_Reg->DPC_END_REG;
	Info.DPC__CURRENT_REG       = &_Reg->DPC_CURRENT_REG;
	Info.DPC__STATUS_REG        = &_Reg->DPC_STATUS_REG;
	Info.DPC__CLOCK_REG         = &_Reg->DPC_CLOCK_REG;
	Info.DPC__BUFBUSY_REG       = &_Reg->DPC_BUFBUSY_REG;
	Info.DPC__PIPEBUSY_REG      = &_Reg->DPC_PIPEBUSY_REG;
	Info.DPC__TMEM_REG          = &_Reg->DPC_TMEM_REG;
	Info.VI__STATUS_REG         = &_Reg->VI_STATUS_REG;
	Info.VI__ORIGIN_REG         = &_Reg->VI_ORIGIN_REG;
	Info.VI__WIDTH_REG          = &_Reg->VI_WIDTH_REG;
	Info.VI__INTR_REG           = &_Reg->VI_INTR_REG;
	Info.VI__V_CURRENT_LINE_REG = &_Reg->VI_CURRENT_REG;
	Info.VI__TIMING_REG         = &_Reg->VI_TIMING_REG;
	Info.VI__V_SYNC_REG         = &_Reg->VI_V_SYNC_REG;
	Info.VI__H_SYNC_REG         = &_Reg->VI_H_SYNC_REG;
	Info.VI__LEAP_REG           = &_Reg->VI_LEAP_REG;
	Info.VI__H_START_REG        = &_Reg->VI_H_START_REG;
	Info.VI__V_START_REG        = &_Reg->VI_V_START_REG;
	Info.VI__V_BURST_REG        = &_Reg->VI_V_BURST_REG;
	Info.VI__X_SCALE_REG        = &_Reg->VI_X_SCALE_REG;
	Info.VI__Y_SCALE_REG        = &_Reg->VI_Y_SCALE_REG;
	
	m_Initilized = InitiateGFX(Info) != 0;
	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
	CloseHandle(hthread);
	
	return m_Initilized;
}

void CGfxPlugin::Close(void) {
	if (m_RomOpen) {
		RomClosed();
		m_RomOpen = false;
	}
	if (m_Initilized) {
		CloseDLL();
		m_Initilized = false;
	}
}

void CGfxPlugin::RomOpened  ( void )
{
	//Real system ... then make the file as open
	if (!m_RomOpen)
	{
		RomOpen();
		m_RomOpen = true;
	}
}

void CGfxPlugin::GameReset(void)
{
	if (m_RomOpen) 
	{
		WriteTrace(TraceGfxPlugin,"RomClosed: Starting");
		RomClosed();
		WriteTrace(TraceGfxPlugin,"RomClosed: Done");
		WriteTrace(TraceGfxPlugin,"RomOpen: Starting");
		RomOpen();
		WriteTrace(TraceGfxPlugin,"RomOpen: Done");
	}
}

bool CGfxPlugin::ValidPluginVersion(PLUGIN_INFO * PluginInfo) {
	switch (PluginInfo->Type) {
	case PLUGIN_TYPE_GFX:
		if (PluginInfo->MemoryBswaped == FALSE) { return FALSE; }
		if (PluginInfo->Version == 0x0102) { return TRUE; }
		if (PluginInfo->Version == 0x0103) { return TRUE; }
		if (PluginInfo->Version == 0x0104) { return TRUE; }
		break;
	}
	return FALSE;
}

void CGfxPlugin::UnloadPlugin(void) {
	if (hDll != NULL ) {
		FreeLibrary((HMODULE)hDll);
		hDll = NULL;
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