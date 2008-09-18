#include "..\Settings.h"
#include "Notification Settings.h"

bool CNotificationSettings::bInFullScreen; //= _Settings->Load(InFullScreen) != 0;

CNotificationSettings::CNotificationSettings()
{
	bInFullScreen = _Settings->LoadBool(InFullScreen);
	_Settings->RegisterChangeCB(InFullScreen,this,(CSettings::SettingChangedFunc)InFullScreenChanged);
}

CNotificationSettings::~CNotificationSettings()
{
	if (_Settings)
	{
		_Settings->UnregisterChangeCB(InFullScreen,this,(CSettings::SettingChangedFunc)InFullScreenChanged);
	}
}

void CNotificationSettings::InFullScreenChanged (CNotificationSettings * _this)
{
	bInFullScreen = _Settings->LoadBool(InFullScreen);
}
