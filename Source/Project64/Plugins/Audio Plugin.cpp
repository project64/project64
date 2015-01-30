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

CAudioPlugin::CAudioPlugin(const char * FileName) : CPlugin(),
	m_hAudioThread(NULL),
	InitiateAudio(NULL),
	AiDacrateChanged(NULL),
	AiLenChanged(NULL),
	AiReadLength(NULL),
	ProcessAList(NULL),
	AiUpdate(NULL)
{
	Init(FileName);
}

bool CAudioPlugin::Init(const char * FileName)
{
	if (!CPlugin::Init(FileName))
	{
		UnloadPlugin();
		return false;
	}

	// Find entries for functions in DLL
	LoadFunction(InitiateAudio);
	LoadFunction(AiDacrateChanged);
	LoadFunction(AiLenChanged);
	LoadFunction(AiReadLength);
	LoadFunction(AiUpdate);
	LoadFunction(ProcessAList);

	// Make sure dll has all needed functions
	if (InitiateAudio == NULL
		|| AiDacrateChanged == NULL
		|| AiLenChanged == NULL
		|| AiReadLength == NULL
		|| ProcessAList == NULL
		|| (m_PluginInfo.Version >= 0x0102
		&& PluginLoaded == NULL))
	{
		UnloadPlugin();
		return false;
	}

	if (PluginLoaded != NULL)
		PluginLoaded();

	return true;
}

bool CAudioPlugin::Initiate(CN64System * System, CMainGui * RenderWindow)
{
	if (InitiateAudio == NULL)
		return false;

	AUDIO_INFO Info = { 0 };

	Info.hwnd = (HWND)RenderWindow->m_hMainWindow;;
	Info.hinst = GetModuleHandle(NULL);
	Info.MemoryBswaped = TRUE;
	Info.CheckInterrupts = DummyCheckInterrupts;

	// We are initializing the plugin before any rom is loaded so we do not have any correct
	// parameters here.. just needed to we can config the DLL.
	if (System == NULL)
	{
		BYTE Buffer[100];
		DWORD Value = 0;

		Info.HEADER = Buffer;
		Info.RDRAM = Buffer;
		Info.DMEM = Buffer;
		Info.IMEM = Buffer;
		Info.MI__INTR_REG = &Value;
		Info.AI__DRAM_ADDR_REG = &Value;
		Info.AI__LEN_REG = &Value;
		Info.AI__CONTROL_REG = &Value;
		Info.AI__STATUS_REG = &Value;
		Info.AI__DACRATE_REG = &Value;
		Info.AI__BITRATE_REG = &Value;
	}
	// Send initialization information to the DLL
	else
	{
		Info.HEADER = g_Rom->GetRomAddress();
		Info.RDRAM = g_MMU->Rdram();
		Info.DMEM = g_MMU->Dmem();
		Info.IMEM = g_MMU->Imem();
		Info.MI__INTR_REG = &g_Reg->m_AudioIntrReg;
		Info.AI__DRAM_ADDR_REG = &g_Reg->AI_DRAM_ADDR_REG;
		Info.AI__LEN_REG = &g_Reg->AI_LEN_REG;
		Info.AI__CONTROL_REG = &g_Reg->AI_CONTROL_REG;
		Info.AI__STATUS_REG = &g_Reg->AI_STATUS_REG;
		Info.AI__DACRATE_REG = &g_Reg->AI_DACRATE_REG;
		Info.AI__BITRATE_REG = &g_Reg->AI_BITRATE_REG;
	}

	// NOTE: Sleep(100) call removed after InitiateAudio() in the System == NULL condition.
	m_Initilized = InitiateAudio(Info) != 0;

	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyFunction, NULL, 0, &ThreadID);
	CloseHandle(hthread);

	if (System != NULL)
	{
		if (AiUpdate)
			m_hAudioThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AudioThread, (LPVOID)this, 0, &ThreadID);

		if (g_Reg->AI_DACRATE_REG != 0)
			DacrateChanged(System->SystemType());
	}

	return m_Initilized;
}

void CAudioPlugin::UnloadPlugin(void)
{
	if (m_hAudioThread)
	{
		WriteTraceF(TraceAudio, __FUNCTION__ ": Terminate Audio Thread");
		TerminateThread(m_hAudioThread, 0);
		m_hAudioThread = NULL;
	}

	CPlugin::UnloadPlugin();

	AiDacrateChanged = NULL;
	AiLenChanged = NULL;
	AiReadLength = NULL;
	AiUpdate = NULL;
	ProcessAList = NULL;
}

void CAudioPlugin::DacrateChanged(SYSTEM_TYPE Type)
{
	if (!Initilized()) { return; }
	WriteTraceF(TraceAudio, __FUNCTION__ ": SystemType: %s", Type == SYSTEM_NTSC ? "SYSTEM_NTSC" : "SYSTEM_PAL");

	//DWORD Frequency = g_Reg->AI_DACRATE_REG * 30;
	//DWORD CountsPerSecond = (g_Reg->VI_V_SYNC_REG != 0 ? (g_Reg->VI_V_SYNC_REG + 1) * g_Settings->LoadDword(Game_ViRefreshRate) : 500000) * 60;
	AiDacrateChanged(Type);
}

void CAudioPlugin::AudioThread(CAudioPlugin * _this) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	for (;;)
	{
		_this->AiUpdate(true);
	}
}

CAudioPlugin::~CAudioPlugin()
{
	Close();
	UnloadPlugin();
}