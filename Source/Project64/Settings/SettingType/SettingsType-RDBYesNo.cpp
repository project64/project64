#include "stdafx.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBYesNo.h"

CSettingTypeRDBYesNo::CSettingTypeRDBYesNo(LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBYesNo::CSettingTypeRDBYesNo(LPCSTR Name, int DefaultValue ) :
	CSettingTypeRomDatabase(Name,DefaultValue)
{
}

	CSettingTypeRDBYesNo::~CSettingTypeRDBYesNo()
{
}

bool CSettingTypeRDBYesNo::Load ( int Index, bool & Value ) const
{
	stdstr strValue;
	bool bRes = m_SettingsIniFile->GetString(m_SectionIdent.c_str(),m_KeyName.c_str(),m_DefaultStr,strValue);
	if (!bRes)
	{
		LoadDefault(Index,Value);
		return false;
	}
	LPCSTR String = strValue.c_str();

	if (_stricmp(String,"Yes") == 0)    { Value = true; } 
	else if (_stricmp(String,"No") == 0)  { Value = false; } 
	else if (_stricmp(String,"default") == 0)  
	{ 
		LoadDefault(Index,Value);
		return false;
	}  else { 
		WriteTraceF(TraceError,"Invalid Yes/No setting value (Section: %s Key: %s Value: %s)",m_SectionIdent.c_str(),String,m_KeyName.c_str(),strValue.c_str());
		LoadDefault(Index,Value);
		return false;
	}
	
	return true;
}

bool CSettingTypeRDBYesNo::Load ( int Index, ULONG & Value ) const
{
	_Notify->BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeRDBYesNo::Load ( int Index, stdstr & Value ) const
{
	_Notify->BreakPoint(__FILE__,__LINE__); 
	return false;
}

//return the default values
void CSettingTypeRDBYesNo::LoadDefault ( int Index, bool & Value   ) const
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

void CSettingTypeRDBYesNo::LoadDefault ( int Index, ULONG & Value  ) const
{
	_Notify->BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBYesNo::LoadDefault ( int Index, stdstr & Value ) const
{
	_Notify->BreakPoint(__FILE__,__LINE__); 
}


//Update the settings
void CSettingTypeRDBYesNo::Save ( int Index, bool Value )
{
	m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),Value? "Yes" : "No");
}

void CSettingTypeRDBYesNo::Save ( int Index, ULONG Value )
{
	m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),Value? "Yes" : "No");
}

void CSettingTypeRDBYesNo::Save ( int Index, const stdstr & Value )
{
	_Notify->BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBYesNo::Save ( int Index, const char * Value )
{
	_Notify->BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBYesNo::Delete( int Index )
{
	m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),NULL);
}
