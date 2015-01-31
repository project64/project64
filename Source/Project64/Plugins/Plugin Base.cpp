/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

CPlugin::CPlugin() :
	m_hDll(NULL),
	m_Initilized(false),
	m_RomOpen(false),
	RomOpen(NULL),
	RomClosed(NULL),
	CloseDLL(NULL),
	PluginOpened(NULL),
	DllAbout(NULL),
	DllConfig(NULL),
	SetSettingInfo(NULL),
	SetSettingInfo2(NULL),
	SetSettingInfo3(NULL)
{
	memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
}

CPlugin::~CPlugin()
{
	UnloadPlugin();
}

bool CPlugin::Load (const char * FileName)
{
	// Already loaded, so unload first.
	if (m_hDll != NULL)
	{
		UnloadPlugin();
	}

	// Try to load the plugin DLL
	//Try to load the DLL library
	UINT LastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	m_hDll = LoadLibrary(FileName);
	SetErrorMode(LastErrorMode);
	
	if (m_hDll == NULL) 
	{ 
		return false;
	}

	// Get DLL information
	void(__cdecl *GetDllInfo) (PLUGIN_INFO * PluginInfo);
	LoadFunction(GetDllInfo);
	if (GetDllInfo == NULL) { return false; }

	GetDllInfo(&m_PluginInfo);
	if (!CPluginList::ValidPluginVersion(m_PluginInfo)) { return false; }
	if (m_PluginInfo.Type != type()) { return false; }

	CloseDLL       = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "CloseDLL" );
	RomOpen        = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "RomOpen" );
	RomClosed      = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "RomClosed" );
	PluginOpened   = (void (__cdecl *)(void)) GetProcAddress( (HMODULE)m_hDll, "PluginLoaded" );
	DllConfig      = (void (__cdecl *)(DWORD)) GetProcAddress( (HMODULE)m_hDll, "DllConfig" );
	DllAbout       = (void (__cdecl *)(HWND)) GetProcAddress( (HMODULE)m_hDll, "DllAbout" );

	SetSettingInfo3 = (void (__cdecl *)(PLUGIN_SETTINGS3 *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo3" );
	if (SetSettingInfo3)
	{
		PLUGIN_SETTINGS3 info;
		info.FlushSettings = (void (*)( void * handle))CSettings::FlushSettings;
		SetSettingInfo3(&info);
	}

	SetSettingInfo2 = (void (__cdecl *)(PLUGIN_SETTINGS2 *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo2" );
	if (SetSettingInfo2)
	{
		PLUGIN_SETTINGS2 info;
		info.FindSystemSettingId = (unsigned int (*)( void * handle, const char * ))CSettings::FindSetting;
		SetSettingInfo2(&info);
	}

	SetSettingInfo   = (void (__cdecl *)(PLUGIN_SETTINGS *))GetProcAddress( (HMODULE)m_hDll, "SetSettingInfo" );
	if (SetSettingInfo)
	{
		PLUGIN_SETTINGS info;
		info.dwSize = sizeof(PLUGIN_SETTINGS);
		info.DefaultStartRange = GetDefaultSettingStartRange();
		info.SettingStartRange = GetSettingStartRange();
		info.MaximumSettings = MaxPluginSetting;
		info.NoDefault = Default_None;
		info.DefaultLocation = g_Settings->LoadDword(Setting_UseFromRegistry) ? SettingType_Registry : SettingType_CfgFile;
		info.handle = g_Settings;
		info.RegisterSetting = (void(*)(void *, int, int, SettingDataType, SettingType, const char *, const char *, DWORD))&CSettings::RegisterSetting;
		info.GetSetting = (unsigned int(*)(void *, int))&CSettings::GetSetting;
		info.GetSettingSz = (const char * (*)(void *, int, char *, int))&CSettings::GetSettingSz;
		info.SetSetting = (void(*)(void *, int, unsigned int))&CSettings::SetSetting;
		info.SetSettingSz = (void(*)(void *, int, const char *))&CSettings::SetSettingSz;
		info.UseUnregisteredSetting = NULL;

		SetSettingInfo(&info);
	}

	if (RomClosed == NULL)
		return false;

	if (!LoadFunctions())
	{
		return false;
	}

	if (PluginOpened)
	{
		PluginOpened();
	}
	return true;
}

void CPlugin::RomOpened()
{
	if (m_RomOpen || RomOpen == NULL)
		return;

	RomOpen();
	m_RomOpen = true;
}

void CPlugin::RomClose()
{
	if (!m_RomOpen)
		return;

	RomClosed();
	m_RomOpen = false;
}

void CPlugin::GameReset()
{
	if (m_RomOpen) 
	{
		RomClosed();
		if (RomOpen)
		{
			RomOpen();
		}
	}
}

void CPlugin::Close()
{
	if (m_RomOpen) {
		RomClosed();
		m_RomOpen = false;
	}
	if (m_Initilized)
	{
		CloseDLL();
		m_Initilized = false;
	}
}

void CPlugin::UnloadPlugin()
{
	memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
	if (m_hDll != NULL)
	{
		UnloadPluginDetails();
		FreeLibrary((HMODULE)m_hDll);
		m_hDll = NULL;
	}

	DllAbout = NULL;
	CloseDLL = NULL;
	RomOpen = NULL;
	RomClosed = NULL;
	PluginOpened = NULL;
	DllConfig = NULL;
	SetSettingInfo = NULL;
	SetSettingInfo2 = NULL;
	SetSettingInfo3 = NULL;
}
