#include "stdafx.h"

int  CN64SystemSettings::m_RefCount = 0; 

bool CN64SystemSettings::m_bShowCPUPer;  
bool CN64SystemSettings::m_bProfiling;   
bool CN64SystemSettings::m_bBasicMode;   
bool CN64SystemSettings::m_bLimitFPS;    
bool CN64SystemSettings::m_bShowDListAListCount;
bool CN64SystemSettings::m_bFixedAudio;  
bool CN64SystemSettings::m_bSyncToAudio; 
bool CN64SystemSettings::m_bDisplayFrameRate;
bool CN64SystemSettings::m_bFastSP;
bool CN64SystemSettings::m_b32Bit;
DWORD CN64SystemSettings::m_ViRefreshRate;


CN64SystemSettings::CN64SystemSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		_Settings->RegisterChangeCB(UserInterface_BasicMode,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(UserInterface_ShowCPUPer,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(UserInterface_DisplayFrameRate,NULL,RefreshSettings);

		_Settings->RegisterChangeCB(Debugger_ProfileCode,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Debugger_ShowDListAListCount,NULL,RefreshSettings);

		_Settings->RegisterChangeCB(GameRunning_LimitFPS,NULL,RefreshSettings);

		_Settings->RegisterChangeCB(Game_FixedAudio,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Game_SyncViaAudio,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Game_32Bit,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Game_FastSP,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Game_ViRefreshRate,NULL,RefreshSettings);
		RefreshSettings(NULL);
	}
}

CN64SystemSettings::~CN64SystemSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(UserInterface_BasicMode,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(UserInterface_DisplayFrameRate,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(UserInterface_ShowCPUPer,NULL,RefreshSettings);

		_Settings->UnregisterChangeCB(Debugger_ProfileCode,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Debugger_ShowDListAListCount,NULL,RefreshSettings);

		_Settings->UnregisterChangeCB(GameRunning_LimitFPS,NULL,RefreshSettings);

		_Settings->UnregisterChangeCB(Game_FixedAudio,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Game_SyncViaAudio,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Game_32Bit,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Game_FastSP,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Game_ViRefreshRate,NULL,RefreshSettings);
	}
}

void CN64SystemSettings::RefreshSettings(void *)
{
	m_bBasicMode           = _Settings->LoadBool(UserInterface_BasicMode);
	m_bDisplayFrameRate    = _Settings->LoadBool(UserInterface_DisplayFrameRate);

	m_bShowCPUPer          = _Settings->LoadBool(UserInterface_ShowCPUPer);
	m_bProfiling           = _Settings->LoadBool(Debugger_ProfileCode);
	m_bShowDListAListCount = _Settings->LoadBool(Debugger_ShowDListAListCount);
	m_bLimitFPS            = _Settings->LoadBool(GameRunning_LimitFPS);

	m_bFixedAudio          = _Settings->LoadBool(Game_FixedAudio);
	m_bSyncToAudio         = m_bFixedAudio ? _Settings->LoadBool(Game_SyncViaAudio) : false;
	m_b32Bit               = _Settings->LoadBool(Game_32Bit);
	m_bFastSP              = _Settings->LoadBool(Game_FastSP);
	m_ViRefreshRate        = _Settings->LoadDword(Game_ViRefreshRate);
}
