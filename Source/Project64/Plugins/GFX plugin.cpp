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

CGfxPlugin::CGfxPlugin() : 
	CaptureScreen(NULL),
	ChangeWindow(NULL),
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
	GetDebugInfo(NULL),
	InitiateDebugger(NULL)
{
	memset(&m_GFXDebug, 0, sizeof(m_GFXDebug));
}

CGfxPlugin::~CGfxPlugin()
{
	Close();
	UnloadPlugin();
}

bool CGfxPlugin::LoadFunctions ( void )
{
	// Find entries for functions in DLL
	BOOL (__cdecl *InitiateGFX) ( void * Gfx_Info );
	LoadFunction(InitiateGFX);
	LoadFunction(ChangeWindow);
	LoadFunction(DrawScreen);
	LoadFunction(MoveScreen);
	LoadFunction(ProcessDList);
	LoadFunction(UpdateScreen);
	LoadFunction(ViStatusChanged);
	LoadFunction(ViWidthChanged);
	LoadFunction(SoftReset);

	// version 0x104 functions
	_LoadFunction("DrawFullScreenStatus", DrawStatus);

	// Rom Browser
	LoadFunction(GetRomBrowserMenu);
	LoadFunction(OnRomBrowserMenuItem);

	//Make sure dll had all needed functions
	if (ChangeWindow == NULL)    { UnloadPlugin(); return false; }
	if (DrawScreen == NULL)      { DrawScreen = DummyDrawScreen; }
	if (InitiateGFX == NULL)        { UnloadPlugin(); return false; }
	if (MoveScreen == NULL)      { MoveScreen = DummyMoveScreen; }
	if (ProcessDList == NULL)    { UnloadPlugin(); return false; }
	if (UpdateScreen == NULL)    { UnloadPlugin(); return false; }
	if (ViStatusChanged == NULL) { ViStatusChanged = DummyViStatusChanged; }
	if (ViWidthChanged == NULL)  { ViWidthChanged = DummyViWidthChanged; }
	if (SoftReset == NULL)       { SoftReset = DummySoftReset; }

	if (m_PluginInfo.Version >= 0x0103)
	{
		LoadFunction(ProcessRDPList);
		LoadFunction(CaptureScreen);
		LoadFunction(ShowCFB);
		LoadFunction(GetDebugInfo);
		_LoadFunction("InitiateGFXDebugger", InitiateDebugger);

		if (ProcessRDPList == NULL) { UnloadPlugin(); return false; }
		if (CaptureScreen == NULL)  { UnloadPlugin(); return false; }
		if (ShowCFB == NULL)        { UnloadPlugin(); return false; }
	}

	if (m_PluginInfo.Version >= 0x0104)
	{
		if (PluginOpened    == NULL) { UnloadPlugin(); return false; }
	}

	if (GetDebugInfo != NULL)
		GetDebugInfo(&m_GFXDebug);

	return true;
}

bool CGfxPlugin::Initiate(CN64System * System, CMainGui * RenderWindow)
{
	if (m_Initialized)
		Close();

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

	GFX_INFO Info = { 0 };

	Info.MemoryBswaped = TRUE;
	Info.hWnd = (HWND)RenderWindow->m_hMainWindow;
	Info.hStatusBar = (HWND)RenderWindow->m_hStatusWnd;
	Info.CheckInterrupts = DummyCheckInterrupts;

	// We are initializing the plugin before any rom is loaded so we do not have any correct
	// parameters here.. it's just needed so we can config the DLL.
	if (System == NULL)
	{
		BYTE Buffer[100];
		DWORD Value = 0;

		Info.HEADER = Buffer;
		Info.RDRAM = Buffer;
		Info.DMEM = Buffer;
		Info.IMEM = Buffer;
		Info.MI__INTR_REG = &Value;
		Info.VI__STATUS_REG = &Value;
		Info.VI__ORIGIN_REG = &Value;
		Info.VI__WIDTH_REG = &Value;
		Info.VI__INTR_REG = &Value;
		Info.VI__V_CURRENT_LINE_REG = &Value;
		Info.VI__TIMING_REG = &Value;
		Info.VI__V_SYNC_REG = &Value;
		Info.VI__H_SYNC_REG = &Value;
		Info.VI__LEAP_REG = &Value;
		Info.VI__H_START_REG = &Value;
		Info.VI__V_START_REG = &Value;
		Info.VI__V_BURST_REG = &Value;
		Info.VI__X_SCALE_REG = &Value;
		Info.VI__Y_SCALE_REG = &Value;
	}
	// Send initialization information to the DLL
	else
	{
		Info.HEADER = g_Rom->GetRomAddress();
		Info.RDRAM = g_MMU->Rdram();
		Info.DMEM = g_MMU->Dmem();
		Info.IMEM = g_MMU->Imem();
		Info.MI__INTR_REG = &g_Reg->m_GfxIntrReg;
		Info.DPC__START_REG = &g_Reg->DPC_START_REG;
		Info.DPC__END_REG = &g_Reg->DPC_END_REG;
		Info.DPC__CURRENT_REG = &g_Reg->DPC_CURRENT_REG;
		Info.DPC__STATUS_REG = &g_Reg->DPC_STATUS_REG;
		Info.DPC__CLOCK_REG = &g_Reg->DPC_CLOCK_REG;
		Info.DPC__BUFBUSY_REG = &g_Reg->DPC_BUFBUSY_REG;
		Info.DPC__PIPEBUSY_REG = &g_Reg->DPC_PIPEBUSY_REG;
		Info.DPC__TMEM_REG = &g_Reg->DPC_TMEM_REG;
		Info.VI__STATUS_REG = &g_Reg->VI_STATUS_REG;
		Info.VI__ORIGIN_REG = &g_Reg->VI_ORIGIN_REG;
		Info.VI__WIDTH_REG = &g_Reg->VI_WIDTH_REG;
		Info.VI__INTR_REG = &g_Reg->VI_INTR_REG;
		Info.VI__V_CURRENT_LINE_REG = &g_Reg->VI_CURRENT_REG;
		Info.VI__TIMING_REG = &g_Reg->VI_TIMING_REG;
		Info.VI__V_SYNC_REG = &g_Reg->VI_V_SYNC_REG;
		Info.VI__H_SYNC_REG = &g_Reg->VI_H_SYNC_REG;
		Info.VI__LEAP_REG = &g_Reg->VI_LEAP_REG;
		Info.VI__H_START_REG = &g_Reg->VI_H_START_REG;
		Info.VI__V_START_REG = &g_Reg->VI_V_START_REG;
		Info.VI__V_BURST_REG = &g_Reg->VI_V_BURST_REG;
		Info.VI__X_SCALE_REG = &g_Reg->VI_X_SCALE_REG;
		Info.VI__Y_SCALE_REG = &g_Reg->VI_Y_SCALE_REG;
	}

	m_Initialized = InitiateGFX(Info) != 0;

	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyFunction, NULL, 0, &ThreadID);
	CloseHandle(hthread);

	return m_Initialized;
}

void CGfxPlugin::UnloadPluginDetails(void)
{
	if (m_hDll != NULL ) {
		FreeLibrary((HMODULE)m_hDll);
		m_hDll = NULL;
	}
	memset(&m_GFXDebug, 0, sizeof(m_GFXDebug));

	//	CaptureScreen        = NULL;
	ChangeWindow = NULL;
	GetDebugInfo = NULL;
	DrawScreen = NULL;
	DrawStatus = NULL;
	//	FrameBufferRead      = NULL;
	//	FrameBufferWrite     = NULL;
	InitiateDebugger = NULL;
	MoveScreen = NULL;
	ProcessDList = NULL;
	ProcessRDPList = NULL;
	ShowCFB = NULL;
	UpdateScreen = NULL;
	ViStatusChanged = NULL;
	ViWidthChanged = NULL;
	GetRomBrowserMenu = NULL;
	OnRomBrowserMenuItem = NULL;
}

void CGfxPlugin::ProcessMenuItem(int id)
{
	if (m_GFXDebug.ProcessMenuItem)
	{
		m_GFXDebug.ProcessMenuItem(id);
	}
}
