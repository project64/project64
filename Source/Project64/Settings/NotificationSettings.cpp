#include "stdafx.h"

bool CNotificationSettings::m_bInFullScreen = false;

CNotificationSettings::CNotificationSettings()
{
}

CNotificationSettings::~CNotificationSettings()
{
    if (g_Settings)
    {
        g_Settings->UnregisterChangeCB((SettingID)UserInterface_InFullScreen, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
    }
}

void CNotificationSettings::RegisterNotifications()
{
    g_Settings->RegisterChangeCB((SettingID)UserInterface_InFullScreen, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
    RefreshSettings();
}

void CNotificationSettings::RefreshSettings()
{
    m_bInFullScreen = UISettingsLoadBool(UserInterface_InFullScreen);
}
