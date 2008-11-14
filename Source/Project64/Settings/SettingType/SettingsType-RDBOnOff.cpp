#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBOnOff.h"

CSettingTypeRDBOnOff::CSettingTypeRDBOnOff(LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBOnOff::CSettingTypeRDBOnOff(LPCSTR Name, int DefaultValue ) :
	CSettingTypeRomDatabase(Name,DefaultValue)
{
}

	CSettingTypeRDBOnOff::~CSettingTypeRDBOnOff()
{
}

bool CSettingTypeRDBOnOff::Load ( int Index, bool & Value ) const
{
	stdstr strValue;
	bool bRes = m_SettingsIniFile->GetString(m_SectionIdent.c_str(),m_KeyName.c_str(),m_DefaultStr,strValue);
	if (!bRes)
	{
		LoadDefault(Index,Value);
		return false;
	}
	LPCSTR String = strValue.c_str();

	if (strcmp(String,"On") == 0)    { Value = true; } 
	else if (strcmp(String,"Off") == 0)  { Value = false; } 
	else if (strcmp(String,"Global") == 0)  
	{
		LoadDefault(Index,Value);
		return false;
	} 
	else { Notify().BreakPoint(__FILE__,__LINE__); }
	
	return true;
}

bool CSettingTypeRDBOnOff::Load ( int Index, ULONG & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeRDBOnOff::Load ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

//return the default values
void CSettingTypeRDBOnOff::LoadDefault ( int Index, bool & Value   ) const
{
	if (m_DefaultSetting != Default_None)
	{
		if (m_DefaultSetting == Default_Constant)
		{
			Value = m_DefaultValue != 0;
		} else {
			_Settings->LoadBool(m_DefaultSetting,Value);
		}
	}
}

void CSettingTypeRDBOnOff::LoadDefault ( int Index, ULONG & Value  ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBOnOff::LoadDefault ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}


//Update the settings
void CSettingTypeRDBOnOff::Save ( int Index, bool Value )
{
	m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),Value? "On" : "Off");
}

void CSettingTypeRDBOnOff::Save ( int Index, ULONG Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBOnOff::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBOnOff::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
