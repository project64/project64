#include "..\support.h"
#include "..\Settings.h"

bool CGuiSettings::bCPURunning; 
bool CGuiSettings::bAutoSleep; 

CGuiSettings::CGuiSettings()
{
	RefreshSettings();
	_Settings->RegisterChangeCB(CPU_Running,this,(CSettings::SettingChangedFunc)CPURunningChanged);
	_Settings->RegisterChangeCB(AutoSleep,this,(CSettings::SettingChangedFunc)AutoSleepChanged);
}

CGuiSettings::~CGuiSettings()
{
	_Settings->UnregisterChangeCB(CPU_Running,this,(CSettings::SettingChangedFunc)CPURunningChanged);
	_Settings->UnregisterChangeCB(AutoSleep,this,(CSettings::SettingChangedFunc)AutoSleepChanged);
}

void CGuiSettings::RefreshSettings()
{
	bCPURunning  = _Settings->LoadBool(CPU_Running);
	bAutoSleep   = _Settings->LoadBool(AutoSleep);
}

void CGuiSettings::CPURunningChanged (CGuiSettings * _this)
{
	_this->bCPURunning  = _Settings->LoadBool(CPU_Running);
}

void CGuiSettings::AutoSleepChanged (CGuiSettings * _this)
{
	_this->bAutoSleep  = _Settings->LoadBool(AutoSleep);
}
