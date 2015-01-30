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

void FixUPXIssue(BYTE * ProgramLocation);

CPlugin::CPlugin() :
	m_hDll(NULL),
	m_Initilized(false),
	m_RomOpen(false),
	RomOpen(NULL),
	RomClosed(NULL),
	CloseDLL(NULL),
	PluginLoaded(NULL),
	DllAbout(NULL),
	DllConfig(NULL),
	SetSettingInfo(NULL),
	SetSettingInfo2(NULL),
	SetSettingInfo3(NULL)
{
	memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
}

// Load & initialise arbitrary plugin
CPlugin * CPlugin::InitPlugin(const char * FileName)
{
	/*
	This is done in two steps.
	First we need to load the plugin to determine what type of plugin it is (GFX, audio, RSP, controller)...
	and then we need to create an appropriate plugin instance, and load it properly according to the plugin.

	This way plugin behaviour is consistent.
	*/
	CPlugin * plugin = NULL;
	HMODULE hModule = LoadPlugin(FileName);
	if (hModule == NULL)
		return plugin;

	// Ensure we can pull information about the plugin.
	void(__cdecl *GetDllInfo)(PLUGIN_INFO *) = (void(__cdecl *)(PLUGIN_INFO *))GetProcAddress(hModule, "GetDllInfo");
	if (GetDllInfo == NULL)
	{
		FreeLibrary(hModule);
		return plugin;
	}

	// Attempt to retrieve plugin information.
	PLUGIN_INFO PluginInfo;
	GetDllInfo(&PluginInfo);

	// We no longer need this DLL instance for anything (we'll load it again properly, if all is well).
	FreeLibrary(hModule);

	// Verify that the plugin version is indeed correct (no point continuing, otherwise)
	if (!CPluginList::ValidPluginVersion(PluginInfo))
		return plugin;

	// Now initialise the plugin appropriately, for consistency.
	switch (PluginInfo.Type)
	{
	case PLUGIN_TYPE_RSP:
		plugin = new CRSP_Plugin(FileName);
		break;

	case PLUGIN_TYPE_GFX:
		plugin = new CGfxPlugin(FileName);
		break;

	case PLUGIN_TYPE_AUDIO:
		plugin = new CAudioPlugin(FileName);
		break;

	case PLUGIN_TYPE_CONTROLLER:
		plugin = new CControl_Plugin(FileName);
		break;
	}

	// We should now have an initialised plugin instance...
	// or NULL.
	return plugin;
}

HMODULE CPlugin::LoadPlugin(const char * FileName)
{
	HMODULE hModule = NULL;
	UINT LastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	hModule = LoadLibrary(FileName);
	SetErrorMode(LastErrorMode);
	if (hModule == NULL)
		return NULL;

	FixUPXIssue((BYTE *)hModule);
	return hModule;
}

bool CPlugin::Init(const char * FileName)
{
	// Already loaded, so unload first.
	if (m_hDll != NULL)
		UnloadPlugin();

	// Try to load the plugin DLL
	m_hDll = LoadPlugin(FileName);
	if (m_hDll == NULL)
		return false;

	// Get DLL information
	void(__cdecl *GetDllInfo) (PLUGIN_INFO * PluginInfo);
	LoadFunction(GetDllInfo);
	if (GetDllInfo == NULL)
		return false;

	GetDllInfo(&m_PluginInfo);
	if (!CPluginList::ValidPluginVersion(m_PluginInfo))
		return false;

	LoadFunction(CloseDLL);
	LoadFunction(RomOpen);
	LoadFunction(RomClosed);
	LoadFunction(DllAbout);
	LoadFunction(DllConfig);

	if (RomClosed == NULL)
		return false;

	// GFX: version 0x104, audio: version 0x102
	LoadFunction(PluginLoaded);

	LoadFunction(SetSettingInfo3);
	LoadFunction(SetSettingInfo2);
	LoadFunction(SetSettingInfo);

	if (SetSettingInfo3)
	{
		PLUGIN_SETTINGS3 info;
		info.FlushSettings = (void(*)(void *))&CSettings::FlushSettings;
		SetSettingInfo3(&info);
	}

	if (SetSettingInfo2)
	{
		PLUGIN_SETTINGS2 info;
		info.FindSystemSettingId = (unsigned int(*)(void *, const char *))&CSettings::FindSetting;
		SetSettingInfo2(&info);
	}

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
		//		g_Settings->UnknownSetting_GFX = info.UseUnregisteredSetting;
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
	if (!m_RomOpen)
		return;

	RomClosed();
	RomOpen();
}

void CPlugin::Close()
{
	RomClose();

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
		FreeLibrary((HMODULE)m_hDll);
		m_hDll = NULL;
	}

	//	DllAbout		= NULL;
	CloseDLL = NULL;
	RomOpen = NULL;
	RomClosed = NULL;
	PluginLoaded = NULL;
	DllConfig = NULL;
	SetSettingInfo = NULL;
	SetSettingInfo2 = NULL;
	SetSettingInfo3 = NULL;
}

CPlugin::~CPlugin()
{
	// NOTE: Unless the cleanup code is changed, we need to reimplement this in each derived plugin class
	// as the object most likely will have changed at this point, and refer to CPlugin's vtable rather than
	// the derived plugin class' vtable.
#if 0
	Close();
	UnloadPlugin();
#endif
}