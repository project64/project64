#include "stdafx.h"

int   CGameSettings::m_RefCount = 0; 

bool CGameSettings::m_bUseTlb;
DWORD CGameSettings::m_CountPerOp = 2;
DWORD CGameSettings::m_ViRefreshRate = 1500;

CGameSettings::CGameSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		_Settings->RegisterChangeCB(Game_UseTlb,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_ViRefreshRate,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_CounterFactor,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CGameSettings::~CGameSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(Game_UseTlb,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_ViRefreshRate,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_CounterFactor,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CGameSettings::RefreshSettings()
{
	m_bUseTlb  = _Settings->LoadBool(Game_UseTlb);
	m_ViRefreshRate = _Settings->LoadDword(Game_ViRefreshRate);
	m_CountPerOp = _Settings->LoadDword(Game_CounterFactor);
}
