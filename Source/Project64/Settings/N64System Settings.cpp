#include "..\support.h"
#include "..\Settings.h"

bool CN64SystemSettings::m_bShowCPUPer;  
bool CN64SystemSettings::m_bProfiling;   
bool CN64SystemSettings::m_bBasicMode;   
bool CN64SystemSettings::m_bLimitFPS;    
bool CN64SystemSettings::m_bShowDListAListCount;
bool CN64SystemSettings::m_bFixedAudio;  
bool CN64SystemSettings::m_bSyncToAudio; 
bool CN64SystemSettings::m_bDisplayFrameRate;
bool CN64SystemSettings::m_SPHack;


CN64SystemSettings::CN64SystemSettings()
{
	_Settings->RegisterChangeCB(UserInterface_BasicMode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->RegisterChangeCB(UserInterface_ShowCPUPer,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->RegisterChangeCB(UserInterface_DisplayFrameRate,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

	_Settings->RegisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->RegisterChangeCB(Debugger_ShowDListAListCount,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

	_Settings->RegisterChangeCB(GameRunning_LimitFPS,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

	_Settings->RegisterChangeCB(Game_FixedAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->RegisterChangeCB(Game_SyncViaAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->RegisterChangeCB(Game_SPHack,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	RefreshSettings();
}

CN64SystemSettings::~CN64SystemSettings()
{
	_Settings->UnregisterChangeCB(UserInterface_BasicMode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->UnregisterChangeCB(UserInterface_DisplayFrameRate,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->UnregisterChangeCB(UserInterface_ShowCPUPer,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

	_Settings->UnregisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->UnregisterChangeCB(Debugger_ShowDListAListCount,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

	_Settings->UnregisterChangeCB(GameRunning_LimitFPS,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

	_Settings->UnregisterChangeCB(Game_FixedAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->UnregisterChangeCB(Game_SyncViaAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->UnregisterChangeCB(Game_SPHack,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
}

void CN64SystemSettings::RefreshSettings()
{
	m_bBasicMode           = _Settings->LoadBool(UserInterface_BasicMode);
	m_bDisplayFrameRate    = _Settings->LoadBool(UserInterface_DisplayFrameRate);

	m_bShowCPUPer          = _Settings->LoadBool(UserInterface_ShowCPUPer);
	m_bProfiling           = _Settings->LoadBool(Debugger_ProfileCode);
	m_bShowDListAListCount = _Settings->LoadBool(Debugger_ShowDListAListCount);
	m_bLimitFPS            = _Settings->LoadBool(GameRunning_LimitFPS);

	m_bFixedAudio          = _Settings->LoadBool(Game_FixedAudio);
	m_bSyncToAudio         = m_bFixedAudio ? _Settings->LoadBool(Game_SyncViaAudio) : false;
	m_SPHack               = _Settings->LoadBool(Game_SPHack);
}
