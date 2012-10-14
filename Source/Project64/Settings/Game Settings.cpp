#include "stdafx.h"

int CGameSettings::m_RefCount = 0; 
bool CGameSettings::m_Registered = false; 

bool  CGameSettings::m_bUseTlb;
DWORD CGameSettings::m_CountPerOp = 2;
DWORD CGameSettings::m_ViRefreshRate = 1500;
bool  CGameSettings::m_DelayDP = false;
bool  CGameSettings::m_DelaySI = false;
DWORD CGameSettings::m_RdramSize = 0;
bool  CGameSettings::m_bFixedAudio;  
bool  CGameSettings::m_bSyncToAudio; 
bool  CGameSettings::m_bFastSP;
bool  CGameSettings::m_b32Bit;

CGameSettings::CGameSettings()
{
	m_RefCount += 1;
	if (_Settings && !m_Registered)
	{
		m_Registered = true;
		_Settings->RegisterChangeCB(Game_UseTlb,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_ViRefreshRate,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_CounterFactor,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_RDRamSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_DelaySI,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_DelayDP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_FixedAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_SyncViaAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_32Bit,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_FastSP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CGameSettings::~CGameSettings()
{
	m_RefCount -= 1;
	if (m_Registered && m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(Game_UseTlb,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_ViRefreshRate,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_CounterFactor,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_RDRamSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_DelaySI,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_DelayDP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_FixedAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_SyncViaAudio,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_32Bit,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_FastSP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);

		m_Registered = false;
	}
}

void CGameSettings::RefreshSettings()
{
	m_bUseTlb       = _Settings->LoadBool(Game_UseTlb);
	m_ViRefreshRate = _Settings->LoadDword(Game_ViRefreshRate);
	m_CountPerOp    = _Settings->LoadDword(Game_CounterFactor);
	m_RdramSize     = _Settings->LoadDword(Game_RDRamSize);
	m_DelaySI       = _Settings->LoadBool(Game_DelaySI);
	m_DelayDP       = _Settings->LoadBool(Game_DelayDP);
	m_bFixedAudio   = _Settings->LoadBool(Game_FixedAudio);
	m_bSyncToAudio  = m_bFixedAudio ? _Settings->LoadBool(Game_SyncViaAudio) : false;
	m_b32Bit        = _Settings->LoadBool(Game_32Bit);
	m_bFastSP       = _Settings->LoadBool(Game_FastSP);
}
