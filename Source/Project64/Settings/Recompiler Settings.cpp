#include "stdafx.h"

int   CRecompilerSettings::m_RefCount = 0; 

bool  CRecompilerSettings::m_bShowRecompMemSize;
bool  CRecompilerSettings::m_bProfiling;  

CRecompilerSettings::CRecompilerSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		g_Settings->RegisterChangeCB(Debugger_ShowRecompMemSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CRecompilerSettings::~CRecompilerSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		g_Settings->UnregisterChangeCB(Debugger_ShowRecompMemSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CRecompilerSettings::RefreshSettings()
{
	m_bShowRecompMemSize = g_Settings->LoadBool(Debugger_ShowRecompMemSize);
	m_bProfiling         = g_Settings->LoadBool(Debugger_ProfileCode);
}
