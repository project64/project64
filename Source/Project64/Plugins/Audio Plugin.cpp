#include "stdafx.h"

void FixUPXIssue ( BYTE * ProgramLocation );

CAudioPlugin::CAudioPlugin ( const char * FileName) 
{
	//Make sure all parts of the class are initialized
	m_Initilized = false;
	m_RomOpen    = false;
	hDll         = NULL;
	m_hAudioThread = NULL;
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
	void  (__cdecl *InitFunc)     ( void );
	m_DacrateChanged = (void (__cdecl *)(SystemType))  GetProcAddress( (HMODULE)hDll, "AiDacrateChanged" );
	LenChanged     = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)hDll, "AiLenChanged" );
	Config         = (void (__cdecl *)(DWORD))GetProcAddress( (HMODULE)hDll, "DllConfig" );
	ReadLength     = (DWORD (__cdecl *)(void))GetProcAddress( (HMODULE)hDll, "AiReadLength" );
	InitFunc       = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)hDll, "InitiateAudio" );
	RomOpen        = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)hDll, "RomOpen" );
	RomClosed      = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)hDll, "RomClosed" );
	CloseDLL       = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)hDll, "CloseDLL" );
	ProcessAList   = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)hDll, "ProcessAList" );	

	Update = (void (__cdecl *)(BOOL))GetProcAddress( (HMODULE)hDll, "AiUpdate" );

	//version 102 functions
	PluginOpened     = (void (__cdecl *)(void))GetProcAddress( (HMODULE)hDll, "PluginLoaded" );

	//Make sure dll had all needed functions
	if (m_DacrateChanged == NULL) { UnloadPlugin(); return;  }
	if (LenChanged     == NULL) { UnloadPlugin(); return;  }
	if (ReadLength     == NULL) { UnloadPlugin(); return;  }
	if (InitFunc       == NULL) { UnloadPlugin(); return;  }
	if (RomClosed      == NULL) { UnloadPlugin(); return;  }
	if (ProcessAList   == NULL) { UnloadPlugin(); return;  }

	SetSettingInfo2   = (void (__cdecl *)(PLUGIN_SETTINGS2 *))GetProcAddress( (HMODULE)hDll, "SetSettingInfo2" );
	if (SetSettingInfo2)
	{
		PLUGIN_SETTINGS2 info;
		info.FindSystemSettingId = (unsigned int (*)( void * handle, const char * ))CSettings::FindGameSetting;
		SetSettingInfo2(&info);
	}

	SetSettingInfo   = (void (__cdecl *)(PLUGIN_SETTINGS *))GetProcAddress( (HMODULE)hDll, "SetSettingInfo" );
	if (SetSettingInfo)
	{
		PLUGIN_SETTINGS info;
		info.dwSize = sizeof(PLUGIN_SETTINGS);
		info.DefaultStartRange = FirstAudioDefaultSet;
		info.SettingStartRange = FirstAudioSettings;
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
		//_Settings->UnknownSetting_AUDIO = info.UseUnregisteredSetting;
	}
	
	if (m_PluginInfo.Version >= 0x0102)
	{
		if (PluginOpened    == NULL) { UnloadPlugin(); return; }
		PluginOpened();
	}
}
	
CAudioPlugin::~CAudioPlugin (void) {
	Close();
	UnloadPlugin();
}

bool CAudioPlugin::Initiate ( CN64System * System, CMainGui * RenderWindow ) {
	typedef struct {
		HWND hwnd;
		HINSTANCE hinst;

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

		DWORD * AI__DRAM_ADDR_REG;
		DWORD * AI__LEN_REG;
		DWORD * AI__CONTROL_REG;
		DWORD * AI__STATUS_REG;
		DWORD * AI__DACRATE_REG;
		DWORD * AI__BITRATE_REG;

		void (__cdecl *CheckInterrupts)( void );
	} AUDIO_INFO;

	//Get Function from DLL
	BOOL (__cdecl *InitiateAudio)    ( AUDIO_INFO Audio_Info );
	InitiateAudio = (BOOL (__cdecl *)(AUDIO_INFO))GetProcAddress( (HMODULE)hDll, "InitiateAudio" );
	if (InitiateAudio == NULL) { return false; }

	AUDIO_INFO Info;
	memset(&Info,0,sizeof(Info));
	//We are initilizing the plugin before any rom is loaded so we do not have any correct
	//paramaters here .. just needed to we can config the DLL
	if (System == NULL) {
		BYTE Buffer[100];
		DWORD Value = 0;

		Info.hwnd              = (HWND)RenderWindow->m_hMainWindow;;
		Info.hinst             = GetModuleHandle(NULL);
		Info.MemoryBswaped     = TRUE;
		Info.HEADER            = Buffer;
		Info.RDRAM             = Buffer;
		Info.DMEM              = Buffer;
		Info.IMEM              = Buffer;
		Info.MI__INTR_REG      = &Value;	
		Info.AI__DRAM_ADDR_REG = &Value;	
		Info.AI__LEN_REG       = &Value;	
		Info.AI__CONTROL_REG   = &Value;	
		Info.AI__STATUS_REG    = &Value;	
		Info.AI__DACRATE_REG   = &Value;	
		Info.AI__BITRATE_REG   = &Value;	
		Info.CheckInterrupts   = DummyCheckInterrupts;

		m_Initilized = InitiateAudio(Info) != 0;
		//jabo had a bug so I call CreateThread so his dllmain gets called again
		DWORD ThreadID;
		HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
		CloseHandle(hthread);
		Sleep(100);
		return m_Initilized;
	}
	m_StatusReg = 0;
	
	//Send Initilization information to the DLL
	Info.hwnd              = (HWND)RenderWindow->m_hMainWindow;
	Info.hinst             = GetModuleHandle(NULL);
	Info.MemoryBswaped     = TRUE;
	Info.HEADER            = _Rom->GetRomAddress();
	Info.RDRAM             = _MMU->Rdram();
	Info.DMEM              = _MMU->Dmem();
	Info.IMEM              = _MMU->Imem();
	Info.MI__INTR_REG      = &_Reg->m_AudioIntrReg;	
	Info.AI__DRAM_ADDR_REG = &_Reg->AI_DRAM_ADDR_REG;	
	Info.AI__LEN_REG       = &_Reg->AI_LEN_REG;	
	Info.AI__CONTROL_REG   = &_Reg->AI_CONTROL_REG;	
	Info.AI__STATUS_REG    = &m_StatusReg;	
	Info.AI__DACRATE_REG   = &_Reg->AI_DACRATE_REG;	
	Info.AI__BITRATE_REG   = &_Reg->AI_BITRATE_REG;	
	Info.CheckInterrupts   = DummyCheckInterrupts;

	m_Initilized = InitiateAudio(Info) != 0;
	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)DummyFunction,NULL,0, &ThreadID);	
	CloseHandle(hthread);

	if (Update) { 
		m_hAudioThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AudioThread, (LPVOID)this,0, &ThreadID);			
	}
	
	if (_Reg->AI_DACRATE_REG != 0) {
		DacrateChanged(SYSTEM_NTSC);
	}
	return m_Initilized;
}

void CAudioPlugin::RomOpened  ( void )
{
	//Real system ... then make the file as open
	if (!m_RomOpen && RomOpen)
	{
		RomOpen();
		m_RomOpen = true;
	}
}

void CAudioPlugin::Close(void) {
	if (m_RomOpen) {
		RomClosed();
		m_RomOpen = false;
	}
	if (m_Initilized) {
		CloseDLL();
		m_Initilized = false;
	}
}

void CAudioPlugin::GameReset(void)
{
	if (m_RomOpen) 
	{
		RomClosed();
		if (RomOpen)
		{
			RomOpen();
		}
	}
}

bool CAudioPlugin::ValidPluginVersion(PLUGIN_INFO * PluginInfo) {
	switch (PluginInfo->Type) {
	case PLUGIN_TYPE_AUDIO:
		if (PluginInfo->MemoryBswaped == FALSE) { return FALSE; }
		if (PluginInfo->Version == 0x0101) { return TRUE; }
		if (PluginInfo->Version == 0x0102) { return TRUE; }
		break;
	}
	return FALSE;
}


void CAudioPlugin::UnloadPlugin(void) {
	if (m_hAudioThread) 
	{
		WriteTraceF(TraceAudio,__FUNCTION__ ": Terminate Audio Thread");
		TerminateThread(m_hAudioThread,0);
		m_hAudioThread = NULL;
	}
	memset(&m_PluginInfo,0,sizeof(m_PluginInfo));
	if (hDll != NULL ) {
		FreeLibrary((HMODULE)hDll);
		hDll = NULL;
	}
	m_DacrateChanged = NULL;
	LenChanged     = NULL;
	Config         = NULL;
	ReadLength     = NULL;
	Update         = NULL;
	ProcessAList   = NULL;
	RomClosed      = NULL;
	CloseDLL       = NULL;
}

void CAudioPlugin::DacrateChanged  (SystemType Type) 
{
	if (!Initilized()) { return; }
	WriteTraceF(TraceAudio,__FUNCTION__ ": SystemType: %s", Type == SYSTEM_NTSC ? "SYSTEM_NTSC" : "SYSTEM_PAL");

	DWORD Frequency = _Reg->AI_DACRATE_REG * 30;
	DWORD CountsPerSecond = (_Reg->VI_V_SYNC_REG != 0 ? (_Reg->VI_V_SYNC_REG + 1) * _Settings->LoadDword(Game_ViRefreshRate) : 500000) * 60;
	m_DacrateChanged(Type);
}

void CAudioPlugin::AudioThread   (CAudioPlugin * _this) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
	for (;;) 
	{ 
		_this->Update(true); 
	}
}
