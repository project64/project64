#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-Application.h"
#include "SettingsType-GameSetting.h"

CSettingTypeGame::CSettingTypeGame(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue )	:
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(LPCSTR Section, LPCSTR Name, DWORD DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeApplication(Section,Name,DefaultSetting)
{
}

bool CSettingTypeGame::Load ( bool & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeGame::Load ( ULONG & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeGame::Load ( stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}


//Update the settings
void CSettingTypeGame::Save ( bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeGame::Save ( ULONG Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeGame::Save ( const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeGame::Save ( const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
