#include "stdafx.h"

bool CNotificationSettings::m_bInFullScreen = false;

CNotificationSettings::CNotificationSettings()
{
	_Settings->RegisterChangeCB(UserInterface_InFullScreen,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	RefreshSettings();
}

CNotificationSettings::~CNotificationSettings()
{
	if (_Settings)
	{
		_Settings->UnregisterChangeCB(UserInterface_InFullScreen,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CNotificationSettings::RefreshSettings()
{
	m_bInFullScreen = _Settings->LoadBool(UserInterface_InFullScreen);
}
