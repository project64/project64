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

CControl_Plugin::CControl_Plugin ( const char * FileName) :
	m_hDll(NULL),
	m_Initilized(false),
	m_RomOpen(false),
	m_AllocatedControllers(false),
	Config(NULL),
	WM_KeyDown(NULL),
	WM_KeyUp(NULL),
	RumbleCommand(NULL),
	GetKeys(NULL),
	ReadController(NULL),
	ControllerCommand(NULL),
	CloseDLL(NULL),
	RomOpen(NULL),
	RomClosed(NULL),
	PluginOpened(NULL),
	SetSettingInfo(NULL),
	SetSettingInfo2(NULL)
{
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));
	memset(&m_PluginControllers,0,sizeof(m_PluginControllers));
	memset(&m_Controllers,0,sizeof(m_Controllers));

	Init(FileName);
}

void CControl_Plugin::Init ( const char * FileName )
{
	//Make sure all parts of the class are initialized
	UnloadPlugin();

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
	void  (__cdecl *InitFunc)     ( void );
	Config            = (void (__cdecl *)(DWORD))GetProcAddress( (HMODULE)m_hDll, "DllConfig" );
	ControllerCommand = (void (__cdecl *)(int,BYTE *))GetProcAddress( (HMODULE)m_hDll, "ControllerCommand" );
	GetKeys           = (void (__cdecl *)(int,BUTTONS*)) GetProcAddress( (HMODULE)m_hDll, "GetKeys" );
	ReadController    = (void (__cdecl *)(int,BYTE *))GetProcAddress( (HMODULE)m_hDll, "ReadController" );
	WM_KeyDown        = (void (__cdecl *)(DWORD,DWORD))GetProcAddress( (HMODULE)m_hDll, "WM_KeyDown" );
	WM_KeyUp          = (void (__cdecl *)(DWORD,DWORD))GetProcAddress( (HMODULE)m_hDll, "WM_KeyUp" );
	InitFunc          = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "InitiateControllers" );
	RomOpen           = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "RomOpen" );
	RomClosed         = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "RomClosed" );
	CloseDLL          = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "CloseDLL" );
	RumbleCommand     = (void (__cdecl *)(int, BOOL))GetProcAddress( (HMODULE)m_hDll, "RumbleCommand" );

	//version 101 functions
	PluginOpened     = (void (__cdecl *)(void))GetProcAddress( (HMODULE)m_hDll, "PluginLoaded" );

	//Make sure dll had all needed functions
	if (InitFunc       == NULL) { UnloadPlugin(); return;  }
	if (CloseDLL       == NULL) { UnloadPlugin(); return;  }

	SetSettingInfo2   = (void (__cdecl *)(PLUGIN_SETTINGS2 *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo2" );
	if (SetSettingInfo2)
	{
		PLUGIN_SETTINGS2 info;
		info.FindSystemSettingId = (unsigned int (*)( void * handle, const char * ))CSettings::FindGameSetting;
		SetSettingInfo2(&info);
	}

	SetSettingInfo   = (void (__cdecl *)(PLUGIN_SETTINGS *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo" );
	if (SetSettingInfo)
	{
		PLUGIN_SETTINGS info;
		info.dwSize = sizeof(PLUGIN_SETTINGS);
		info.DefaultStartRange = FirstCtrlDefaultSet;
		info.SettingStartRange = FirstCtrlSettings;
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
//		g_Settings->UnknownSetting_CTRL = info.UseUnregisteredSetting;
	}
	
	if (m_PluginInfo.Version >= 0x0102)
	{
		if (PluginOpened    == NULL) { UnloadPlugin(); return; }

		PluginOpened();
	}

	//Allocate our own controller
	m_AllocatedControllers = true;
	m_Controllers[0] = new CCONTROL(m_PluginControllers[0].Present,m_PluginControllers[0].RawData,m_PluginControllers[0].Plugin);
	m_Controllers[1] = new CCONTROL(m_PluginControllers[1].Present,m_PluginControllers[1].RawData,m_PluginControllers[1].Plugin);
	m_Controllers[2] = new CCONTROL(m_PluginControllers[2].Present,m_PluginControllers[2].RawData,m_PluginControllers[2].Plugin);
	m_Controllers[3] = new CCONTROL(m_PluginControllers[3].Present,m_PluginControllers[3].RawData,m_PluginControllers[3].Plugin);
}

CControl_Plugin::~CControl_Plugin (void) {
	Close();
	UnloadPlugin();
}

bool CControl_Plugin::Initiate ( CN64System * System, CMainGui * RenderWindow ) {
	m_PluginControllers[0].Present = FALSE;
	m_PluginControllers[0].RawData = FALSE;
	m_PluginControllers[0].Plugin  = PLUGIN_NONE;

	m_PluginControllers[1].Present = FALSE;
	m_PluginControllers[1].RawData = FALSE;
	m_PluginControllers[1].Plugin  = PLUGIN_NONE;

	m_PluginControllers[2].Present = FALSE;
	m_PluginControllers[2].RawData = FALSE;
	m_PluginControllers[2].Plugin  = PLUGIN_NONE;

	m_PluginControllers[3].Present = FALSE;
	m_PluginControllers[3].RawData = FALSE;
	m_PluginControllers[3].Plugin  = PLUGIN_NONE;

	//Get DLL information
	void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );
	GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( (HMODULE)m_hDll, "GetDllInfo" );
	if (GetDllInfo == NULL) { return false; }

	PLUGIN_INFO PluginInfo;
	GetDllInfo(&PluginInfo);

	//Test Plugin version
	if (PluginInfo.Version == 0x0100) {
		//Get Function from DLL
		void (__cdecl *InitiateControllers_1_0)( HWND hMainWindow, CONTROL Controls[4] );
		InitiateControllers_1_0 = (void (__cdecl *)(HWND, CONTROL *))GetProcAddress( (HMODULE)m_hDll, "InitiateControllers" );
		if (InitiateControllers_1_0 == NULL) { return false; }
		InitiateControllers_1_0((HWND)RenderWindow->m_hMainWindow,m_PluginControllers);
		m_Initilized = true;
	}
	if (PluginInfo.Version >= 0x0101) {
		typedef struct {
			HWND hMainWindow;
			HINSTANCE hinst;

			BOOL MemoryBswaped;		// If this is set to TRUE, then the memory has been pre
									//   bswap on a dword (32 bits) boundry, only effects header. 
									//	eg. the first 8 bytes are stored like this:
									//        4 3 2 1   8 7 6 5
			BYTE * HEADER;			// This is the rom header (first 40h bytes of the rom)
			CONTROL *Controls;		// A pointer to an array of 4 controllers .. eg:
									// CONTROL Controls[4];
		} CONTROL_INFO;

		//Get Function from DLL		
		void (__cdecl *InitiateControllers_1_1)( CONTROL_INFO * ControlInfo );
		InitiateControllers_1_1 = (void (__cdecl *)(CONTROL_INFO *))GetProcAddress( (HMODULE)m_hDll, "InitiateControllers" );
		if (InitiateControllers_1_1 == NULL) { return false; }
		
		CONTROL_INFO ControlInfo;
		if (System == NULL) {
			BYTE Buffer[100];
			
			ControlInfo.Controls      = m_PluginControllers;
			ControlInfo.HEADER        = Buffer;
			ControlInfo.hinst         = GetModuleHandle(NULL);
			ControlInfo.hMainWindow   = (HWND)RenderWindow->m_hMainWindow;
			ControlInfo.MemoryBswaped = TRUE;
			InitiateControllers_1_1(&ControlInfo);
			m_Initilized = true;
		} else {
			ControlInfo.Controls      = m_PluginControllers;
			ControlInfo.HEADER        = g_Rom->GetRomAddress();
			ControlInfo.hinst         = GetModuleHandle(NULL);
			ControlInfo.hMainWindow   = (HWND)RenderWindow->m_hMainWindow;
			ControlInfo.MemoryBswaped = TRUE;
			InitiateControllers_1_1(&ControlInfo);
			m_Initilized = true;
		}
	}

	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
	CloseHandle(hthread);

	
	return m_Initilized;
}

void CControl_Plugin::RomOpened ( void )
{
	//Real system ... then make the file as open
	if (!m_RomOpen) 
	{
		RomOpen();
		m_RomOpen = true;
	}
}

void CControl_Plugin::RomClose  ( void )
{
	if (m_RomOpen)
	{
		RomClosed();
		m_RomOpen = false;
	}
}

void CControl_Plugin::Close(void) {
	if (m_RomOpen) {
		RomClosed();
		m_RomOpen = false;
	}
	if (m_Initilized) {
		CloseDLL();
		m_Initilized = false;
	}
}

void CControl_Plugin::GameReset(void)
{
	if (m_RomOpen) 
	{
		RomClosed();
		RomOpen();
	}
}

void CControl_Plugin::UnloadPlugin(void) {
	if (m_AllocatedControllers) {
		for (int count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++) {
			delete m_Controllers[count];
			m_Controllers[count] = NULL;
		}
	}
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));
	if (m_hDll != NULL ) {
		FreeLibrary((HMODULE)m_hDll);
		m_hDll = NULL;
	}
	m_AllocatedControllers = false;
	m_Controllers[0]       = NULL;
	m_Controllers[1]       = NULL;
	m_Controllers[2]       = NULL;
	m_Controllers[3]       = NULL;
	Config                 = NULL;
	ControllerCommand      = NULL;
	GetKeys                = NULL;
	ReadController         = NULL;
	WM_KeyDown             = NULL;
	WM_KeyUp               = NULL;
	CloseDLL               = NULL;
	RomOpen                = NULL;
	RomClosed              = NULL;

}

void CControl_Plugin::UpdateKeys (void) {
	if (!m_AllocatedControllers) { return; }
	for (int cont = 0; cont < sizeof(m_Controllers) / sizeof(m_Controllers[0]); cont++) {
		if (!m_Controllers[cont]->m_Present) { continue; }
		if (!m_Controllers[cont]->m_RawData) { 
			GetKeys(cont,&m_Controllers[cont]->m_Buttons);
		} else {
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		}
	}
	if (ReadController) { ReadController(-1,NULL); }
}

void CControl_Plugin::SetControl(CControl_Plugin const * const Plugin)
{
	if (m_AllocatedControllers) {
		for (int count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++) {
			delete m_Controllers[count];
			m_Controllers[count] = NULL;
		}
	}
	m_AllocatedControllers = false;
	for (int count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++) {
		m_Controllers[count] = Plugin->m_Controllers[count];
	}

}

CCONTROL::CCONTROL(DWORD &Present,DWORD &RawData, int &PlugType) :
	m_Present(Present),m_RawData(RawData),m_PlugType(PlugType)
{
	m_Buttons.Value = 0;
}