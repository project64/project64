#include "jniBridgeSettings.h"
#include <Project64-core/Settings.h>

int  CJniBridegSettings::m_RefCount = 0; 
bool CJniBridegSettings::m_bCPURunning; 

CJniBridegSettings::CJniBridegSettings()
{	
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		g_Settings->RegisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
		RefreshSettings(NULL);
	}
}

CJniBridegSettings::~CJniBridegSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		g_Settings->UnregisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
	}
}

void CJniBridegSettings::RefreshSettings(void *)
{
	m_bCPURunning  = g_Settings->LoadBool(GameRunning_CPU_Running);
}
