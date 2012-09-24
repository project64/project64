#include "stdafx.h"

int   CGameSettings::m_RefCount = 0; 

bool  CGameSettings::m_bUseTlb;

CGameSettings::CGameSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		_Settings->RegisterChangeCB(Game_UseTlb,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CGameSettings::~CGameSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(Game_UseTlb,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CGameSettings::RefreshSettings()
{
	m_bUseTlb  = _Settings->LoadBool(Game_UseTlb);
}
