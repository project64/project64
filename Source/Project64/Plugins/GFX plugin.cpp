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

CGfxPlugin::CGfxPlugin(const char * FileName) : CPlugin(),
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
	Init(FileName);
}

bool CGfxPlugin::Init(const char * FileName)
{
	if (!CPlugin::Init(FileName))
	{
		UnloadPlugin();
		return false;
	}

	// Find entries for functions in DLL
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

	if (DrawScreen == NULL)      { DrawScreen = DummyDrawScreen; }
	if (MoveScreen == NULL)      { MoveScreen = DummyMoveScreen; }
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
	}

	// Required functions for plugins
	if (InitiateGFX == NULL
		|| ChangeWindow == NULL
		|| ProcessDList == NULL
		|| UpdateScreen == NULL
		// Required functions for plugins version 0x103+
		|| (m_PluginInfo.Version >= 0x0103
		&& (ProcessRDPList == NULL
		|| CaptureScreen == NULL
		|| ShowCFB == NULL))
		// Required functions for plugins version 0x104+
		|| (m_PluginInfo.Version >= 0x0104
		&& PluginLoaded == NULL))
	{
		UnloadPlugin();
		return false;
	}

	if (GetDebugInfo != NULL)
		GetDebugInfo(&m_GFXDebug);

	if (PluginLoaded != NULL)
		PluginLoaded();

	return true;
}

bool CGfxPlugin::Initiate(CN64System * System, CMainGui * RenderWindow)
{
	if (m_Initilized)
		Close();

	if (InitiateGFX == NULL)
		return false;

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

	// NOTE: Sleep(100) call removed after InitiateGFX() in the System == NULL condition.
	m_Initilized = InitiateGFX(Info) != 0;

	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyFunction, NULL, 0, &ThreadID);
	CloseHandle(hthread);

	return m_Initilized;
}

#if 0
void CGfxPlugin::RomOpened(void)
{
	//Real system ... then make the file as open
	if (!m_RomOpen)
	{
		WriteTrace(TraceGfxPlugin, __FUNCTION__ ": Before RomOpen");
		RomOpen();
		WriteTrace(TraceGfxPlugin, __FUNCTION__ ": After RomOpen");
		m_RomOpen = true;
	}
}

void CGfxPlugin::RomClose(void)
{
	if (m_RomOpen)
	{
		WriteTrace(TraceGfxPlugin, __FUNCTION__ ": Before RomClosed");
		RomClosed();
		WriteTrace(TraceGfxPlugin, __FUNCTION__ ": After RomClosed");
		m_RomOpen = false;
	}
}
#endif

void CGfxPlugin::UnloadPlugin()
{
	CPlugin::UnloadPlugin();

	memset(&m_GFXDebug, 0, sizeof(m_GFXDebug));

	InitiateGFX = NULL;
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

CGfxPlugin::~CGfxPlugin()
{
	Close();
	UnloadPlugin();
}