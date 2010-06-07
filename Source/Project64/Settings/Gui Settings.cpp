#include "stdafx.h"

int  CGuiSettings::m_RefCount = 0; 
bool CGuiSettings::m_bCPURunning; 
bool CGuiSettings::m_bAutoSleep; 

CGuiSettings::CGuiSettings()
{	
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		_Settings->RegisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
		_Settings->RegisterChangeCB(Setting_AutoSleep,NULL,RefreshSettings);
		RefreshSettings(NULL);
	}
}

CGuiSettings::~CGuiSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
		_Settings->UnregisterChangeCB(Setting_AutoSleep,NULL,RefreshSettings);
	}
}

void CGuiSettings::RefreshSettings(void *)
{
	m_bCPURunning  = _Settings->LoadBool(GameRunning_CPU_Running);
	m_bAutoSleep   = _Settings->LoadBool(Setting_AutoSleep);
}

