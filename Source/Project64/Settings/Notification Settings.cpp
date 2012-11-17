#include "stdafx.h"

bool CNotificationSettings::m_bInFullScreen = false;

CNotificationSettings::CNotificationSettings()
{
	g_Settings->RegisterChangeCB(UserInterface_InFullScreen,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	RefreshSettings();
}

CNotificationSettings::~CNotificationSettings()
{
	if (g_Settings)
	{
		g_Settings->UnregisterChangeCB(UserInterface_InFullScreen,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CNotificationSettings::RefreshSettings()
{
	m_bInFullScreen = g_Settings->LoadBool(UserInterface_InFullScreen);
}
