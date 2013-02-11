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
void DummyFunc1 ( BOOL /*a*/) {}

CRSP_Plugin::CRSP_Plugin ( const char * FileName) :
	Config(NULL),
	DoRspCycles(NULL),
	EnableDebugging(NULL),
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
	m_RomOpen(false),
	m_CycleCount(0)
{
	memset(&m_RSPDebug,0,sizeof(m_RSPDebug));
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));
	Init(FileName);
}

void CRSP_Plugin::Init ( const char * FileName )
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
	void (__cdecl *InitFunc)( void );
	DoRspCycles      = (DWORD (__cdecl *)(DWORD))GetProcAddress( (HMODULE)m_hDll, "DoRspCycles" );
	InitFunc         = (void (__cdecl *)(void))  GetProcAddress( (HMODULE)m_hDll, "InitiateRSP" );
	RomClosed        = (void (__cdecl *)(void))  GetProcAddress( (HMODULE)m_hDll, "RomClosed" );
	RomOpen          = (void (__cdecl *)(void))  GetProcAddress( (HMODULE)m_hDll, "RomOpen" );
	CloseDLL         = (void (__cdecl *)(void))  GetProcAddress( (HMODULE)m_hDll, "CloseDLL" );
	Config           = (void (__cdecl *)(DWORD)) GetProcAddress( (HMODULE)m_hDll, "DllConfig" );
	GetDebugInfo     = (void (__cdecl *)(RSPDEBUG_INFO *))GetProcAddress( (HMODULE)m_hDll, "GetRspDebugInfo" );
	InitiateDebugger = (void (__cdecl *)(DEBUG_INFO))GetProcAddress( (HMODULE)m_hDll, "InitiateRSPDebugger" );
	EnableDebugging  = (void (__cdecl *)(BOOL))GetProcAddress( (HMODULE)m_hDll, "EnableDebugging" );
	if (EnableDebugging == NULL) { EnableDebugging = DummyFunc1; }

	//version 102 functions
	PluginOpened     = (void (__cdecl *)(void))GetProcAddress( (HMODULE)m_hDll, "PluginLoaded" );

	//Make sure dll had all needed functions
	if (DoRspCycles == NULL) { UnloadPlugin(); return; }
	if (InitFunc    == NULL) { UnloadPlugin(); return; }
	if (RomClosed   == NULL) { UnloadPlugin(); return; }
	if (CloseDLL    == NULL) { UnloadPlugin(); return; }

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
		info.DefaultStartRange = FirstRSPDefaultSet;
		info.SettingStartRange = FirstRSPSettings;
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
		//g_Settings->UnknownSetting_RSP = info.UseUnregisteredSetting;
	}

	if (m_PluginInfo.Version >= 0x0102)
	{
		if (PluginOpened    == NULL) { UnloadPlugin(); return; }

		PluginOpened();
	}

	//Get debug info if able
	if (GetDebugInfo != NULL) { GetDebugInfo(&m_RSPDebug); }

}

CRSP_Plugin::~CRSP_Plugin (void) {
	Close();
	UnloadPlugin();
}

bool CRSP_Plugin::Initiate ( CPlugins * Plugins, CN64System * System ) 
{
	//Get DLL information
	void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );
	GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( (HMODULE)m_hDll, "GetDllInfo" );
	if (GetDllInfo == NULL) { return false; }

	PLUGIN_INFO PluginInfo;
	GetDllInfo(&PluginInfo);

	if (PluginInfo.Version == 1 || PluginInfo.Version  == 0x100) {
		return false;
	//	return Initiate_1_0(System,RenderWindow);
	}

	typedef struct {
		HINSTANCE hInst;
		BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
								  bswap on a dword (32 bits) boundry */
		BYTE * RDRAM;
		BYTE * DMEM;
		BYTE * IMEM;

		DWORD * MI__INTR_REG;

		DWORD * SP__MEM_ADDR_REG;
		DWORD * SP__DRAM_ADDR_REG;
		DWORD * SP__RD_LEN_REG;
		DWORD * SP__WR_LEN_REG;
		DWORD * SP__STATUS_REG;
		DWORD * SP__DMA_FULL_REG;
		DWORD * SP__DMA_BUSY_REG;
		DWORD * SP__PC_REG;
		DWORD * SP__SEMAPHORE_REG;

		DWORD * DPC__START_REG;
		DWORD * DPC__END_REG;
		DWORD * DPC__CURRENT_REG;
		DWORD * DPC__STATUS_REG;
		DWORD * DPC__CLOCK_REG;
		DWORD * DPC__BUFBUSY_REG;
		DWORD * DPC__PIPEBUSY_REG;
		DWORD * DPC__TMEM_REG;

		void ( __cdecl *CheckInterrupts)( void );
		void (__cdecl *ProcessDlist)( void );
		void (__cdecl *ProcessAlist)( void );
		void (__cdecl *ProcessRdpList)( void );
		void (__cdecl *ShowCFB)( void );
	} RSP_INFO_1_1;

	//Get Function from DLL
	void (__cdecl *InitiateRSP)    ( RSP_INFO_1_1 Audio_Info,DWORD * Cycles );
	InitiateRSP = (void (__cdecl *)(RSP_INFO_1_1,DWORD *))GetProcAddress( (HMODULE)m_hDll, "InitiateRSP" );
	if (InitiateRSP == NULL) { return false; }

	RSP_INFO_1_1 Info;
	memset(&Info,0,sizeof(Info));
	//We are initilizing the plugin before any rom is loaded so we do not have any correct
	//paramaters here .. just needed to we can config the DLL

	if (System == NULL) 
	{
		BYTE Buffer[100];
		DWORD Value = 0;

		Info.CheckInterrupts   = DummyCheckInterrupts;
		Info.ProcessDlist      = DummyCheckInterrupts;
		Info.ProcessRdpList    = DummyCheckInterrupts;
		Info.ShowCFB           = DummyCheckInterrupts;
		Info.ProcessAlist      = DummyCheckInterrupts;

		Info.hInst             = GetModuleHandle(NULL);;
		Info.RDRAM             = Buffer;
		Info.DMEM              = Buffer;
		Info.IMEM              = Buffer;
		Info.MemoryBswaped     = TRUE;
		
		Info.MI__INTR_REG      = &Value;
			
		Info.SP__MEM_ADDR_REG  = &Value;
		Info.SP__DRAM_ADDR_REG = &Value;
		Info.SP__RD_LEN_REG    = &Value;
		Info.SP__WR_LEN_REG    = &Value;
		Info.SP__STATUS_REG    = &Value;
		Info.SP__DMA_FULL_REG  = &Value;
		Info.SP__DMA_BUSY_REG  = &Value;
		Info.SP__PC_REG        = &Value;
		Info.SP__SEMAPHORE_REG = &Value;
			
		Info.DPC__START_REG    = &Value;
		Info.DPC__END_REG      = &Value;
		Info.DPC__CURRENT_REG  = &Value;
		Info.DPC__STATUS_REG   = &Value;
		Info.DPC__CLOCK_REG    = &Value;
		Info.DPC__BUFBUSY_REG  = &Value;
		Info.DPC__PIPEBUSY_REG = &Value;
		Info.DPC__TMEM_REG     = &Value;

		InitiateRSP(Info,&m_CycleCount);
		m_Initilized = TRUE;
		//jabo had a bug so I call CreateThread so his dllmain gets called again
		DWORD ThreadID;
		HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
		CloseHandle(hthread);
		Sleep(100);
		return m_Initilized;
	}

	//Send Initilization information to the DLL
	Info.CheckInterrupts   = DummyCheckInterrupts;
	Info.ProcessDlist      = Plugins->Gfx()->ProcessDList;
	Info.ProcessRdpList    = Plugins->Gfx()->ProcessRDPList;
	Info.ShowCFB           = Plugins->Gfx()->ShowCFB;
	Info.ProcessAlist      = Plugins->Audio()->ProcessAList;

	Info.hInst             = GetModuleHandle(NULL);;
	Info.RDRAM             = g_MMU->Rdram();
	Info.DMEM              = g_MMU->Dmem();
	Info.IMEM              = g_MMU->Imem();
	Info.MemoryBswaped     = FALSE;

	Info.MI__INTR_REG      = &g_Reg->m_RspIntrReg;
		
	Info.SP__MEM_ADDR_REG  = &g_Reg->SP_MEM_ADDR_REG;
	Info.SP__DRAM_ADDR_REG = &g_Reg->SP_DRAM_ADDR_REG;
	Info.SP__RD_LEN_REG    = &g_Reg->SP_RD_LEN_REG;
	Info.SP__WR_LEN_REG    = &g_Reg->SP_WR_LEN_REG;
	Info.SP__STATUS_REG    = &g_Reg->SP_STATUS_REG;
	Info.SP__DMA_FULL_REG  = &g_Reg->SP_DMA_FULL_REG;
	Info.SP__DMA_BUSY_REG  = &g_Reg->SP_DMA_BUSY_REG;
	Info.SP__PC_REG        = &g_Reg->SP_PC_REG;
	Info.SP__SEMAPHORE_REG = &g_Reg->SP_SEMAPHORE_REG;
		
	Info.DPC__START_REG    = &g_Reg->DPC_START_REG;
	Info.DPC__END_REG      = &g_Reg->DPC_END_REG;
	Info.DPC__CURRENT_REG  = &g_Reg->DPC_CURRENT_REG;
	Info.DPC__STATUS_REG   = &g_Reg->DPC_STATUS_REG;
	Info.DPC__CLOCK_REG    = &g_Reg->DPC_CLOCK_REG;
	Info.DPC__BUFBUSY_REG  = &g_Reg->DPC_BUFBUSY_REG;
	Info.DPC__PIPEBUSY_REG = &g_Reg->DPC_PIPEBUSY_REG;
	Info.DPC__TMEM_REG     = &g_Reg->DPC_TMEM_REG;

	InitiateRSP(Info,&m_CycleCount);
	m_Initilized = true;

	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
	CloseHandle(hthread);
	return m_Initilized;
}

void CRSP_Plugin::Close(void) {
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

void CRSP_Plugin::RomOpened  ( void )
{
	//Real system ... then make the file as open
	if (!m_RomOpen)
	{
		if (RomOpen)
		{
			RomOpen();
		}
		m_RomOpen = true;
	}
}

void CRSP_Plugin::RomClose  ( void )
{
	if (m_RomOpen)
	{
		RomClosed();
		m_RomOpen = false;
	}
}

void CRSP_Plugin::GameReset(void)
{
	if (m_RomOpen) 
	{
		RomClose();
		RomOpened();
	}
}

void CRSP_Plugin::UnloadPlugin(void) {
	if (m_hDll != NULL ) {
		FreeLibrary((HMODULE)m_hDll);
		m_hDll = NULL;
	}
	memset(&m_RSPDebug,0,sizeof(m_RSPDebug));
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));
	DoRspCycles      = NULL;
	RomClosed        = NULL;
	Config           = NULL;
	CloseDLL         = NULL;
	SetSettingInfo   = NULL;
	EnableDebugging  = NULL;
	GetDebugInfo     = NULL;
	PluginOpened     = NULL;
	InitiateDebugger = NULL;
}

void CRSP_Plugin::ProcessMenuItem (int id )
{
	if (m_RSPDebug.ProcessMenuItem)
	{
		m_RSPDebug.ProcessMenuItem(id); 
	}
}
