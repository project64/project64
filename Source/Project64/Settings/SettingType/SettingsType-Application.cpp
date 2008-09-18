#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-Application.h"

bool       CSettingTypeApplication::m_UseRegistry     = false;
CIniFile * CSettingTypeApplication::m_SettingsIniFile = NULL;

CSettingTypeApplication::CSettingTypeApplication(LPCSTR Section, LPCSTR Name, DWORD DefaultValue ) :
	m_Section(FixSectionName(Section)),
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(DefaultValue),
	m_DefaultSetting(No_Default),
	m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::CSettingTypeApplication(LPCSTR Section, LPCSTR Name, bool DefaultValue ) :
	m_Section(FixSectionName(Section)),
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(DefaultValue),
	m_DefaultSetting(No_Default),
	m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::CSettingTypeApplication(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue ) :
	m_Section(FixSectionName(Section)),
	m_KeyName(Name),
	m_DefaultStr(DefaultValue),
	m_DefaultValue(0),
	m_DefaultSetting(No_Default),
	m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::CSettingTypeApplication(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting ) :
	m_Section(FixSectionName(Section)),
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(0),
	m_DefaultSetting(DefaultSetting),
	m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::~CSettingTypeApplication()
{
}


void CSettingTypeApplication::Initilize( const char * AppName )
{
	m_SettingsIniFile = new CIniFile(_Settings->LoadString(SettingsIniName).c_str());
	m_UseRegistry = _Settings->LoadBool(UseSettingFromRegistry);
}


void CSettingTypeApplication::CleanUp()
{
	if (m_SettingsIniFile)
	{
		delete m_SettingsIniFile;
		m_SettingsIniFile = NULL;
	}
}

bool CSettingTypeApplication::Load ( int Index, bool & Value ) const
{
	if (!m_UseRegistry)
	{
		DWORD dwValue;
		bool bRes = m_SettingsIniFile->GetNumber(m_Section.c_str(),m_KeyNameIdex.c_str(),m_DefaultValue,dwValue);
		Value = dwValue != 0;
		return bRes;
	}
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeApplication::Load ( int Index, ULONG & Value ) const
{
	bool bRes;
	if (!m_UseRegistry)
	{
		bRes = m_SettingsIniFile->GetNumber(m_Section.c_str(),m_KeyNameIdex.c_str(),m_DefaultValue,Value);
	} else {
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
	if (!bRes && m_DefaultSetting != No_Default)
	{
		bRes = _Settings->LoadDword(m_DefaultSetting,Value);
	}
	return bRes;
}

bool CSettingTypeApplication::Load ( int Index, stdstr & Value ) const
{
	bool bRes;
	if (!m_UseRegistry)
	{
		bRes = m_SettingsIniFile->GetString(m_Section.c_str(),m_KeyNameIdex.c_str(),m_DefaultStr,Value);
	} else {
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
	if (!bRes && m_DefaultSetting != No_Default)
	{
		bRes = _Settings->LoadString(m_DefaultSetting,Value);
	}
	return bRes;
}


//Update the settings
void CSettingTypeApplication::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeApplication::Save ( int Index, ULONG Value )
{
	if (!m_UseRegistry)
	{
		m_SettingsIniFile->SaveNumber(m_Section.c_str(),m_KeyNameIdex.c_str(),Value);
	} else {
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
}

void CSettingTypeApplication::Save ( int Index, const stdstr & Value )
{
	if (!m_UseRegistry)
	{
		m_SettingsIniFile->SaveString(m_Section.c_str(),m_KeyNameIdex.c_str(),Value.c_str());
	} else {
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
}

void CSettingTypeApplication::Save ( int Index, const char * Value )
{
	if (!m_UseRegistry)
	{
		m_SettingsIniFile->SaveString(m_Section.c_str(),m_KeyNameIdex.c_str(),Value);
	} else {
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
}

stdstr CSettingTypeApplication::FixSectionName(LPCSTR Section)
{
	stdstr SectionName(Section);

	if (!m_UseRegistry)
	{
		if (SectionName.empty()) 
		{ 
			SectionName = "default";
		}
		SectionName.replace("\\","-");
	}
	return SectionName;
}
