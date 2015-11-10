/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

bool CNotificationSettings::m_bInFullScreen = false;

CNotificationSettings::CNotificationSettings()
{
}

CNotificationSettings::~CNotificationSettings()
{
	if (g_Settings)
	{
		g_Settings->UnregisterChangeCB(UserInterface_InFullScreen,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CNotificationSettings::RegisterNotifications()
{
	g_Settings->RegisterChangeCB(UserInterface_InFullScreen, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
	RefreshSettings();
}

void CNotificationSettings::RefreshSettings()
{
	m_bInFullScreen = g_Settings->LoadBool(UserInterface_InFullScreen);
}
