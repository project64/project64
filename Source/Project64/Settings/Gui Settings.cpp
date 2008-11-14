#include "..\support.h"
#include "..\Settings.h"

bool CGuiSettings::m_bCPURunning; 
bool CGuiSettings::m_bAutoSleep; 

CGuiSettings::CGuiSettings()
{
	RefreshSettings();
	_Settings->RegisterChangeCB(GameRunning_CPU_Running,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->RegisterChangeCB(Setting_AutoSleep,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
}

CGuiSettings::~CGuiSettings()
{
	_Settings->UnregisterChangeCB(GameRunning_CPU_Running,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	_Settings->UnregisterChangeCB(Setting_AutoSleep,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
}

void CGuiSettings::RefreshSettings()
{
	m_bCPURunning  = _Settings->LoadBool(GameRunning_CPU_Running);
	m_bAutoSleep   = _Settings->LoadBool(Setting_AutoSleep);
}

