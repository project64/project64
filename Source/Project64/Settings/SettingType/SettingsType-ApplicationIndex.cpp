#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-Application.h"
#include "SettingsType-ApplicationIndex.h"


CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, DWORD DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, bool DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeApplication(Section,Name,DefaultSetting)
{
}


bool CSettingTypeApplicationIndex::Load ( int Index, bool & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeApplicationIndex::Load ( int Index, ULONG & Value ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName,Index);
	return CSettingTypeApplication::Load(0,Value);
}

bool CSettingTypeApplicationIndex::Load ( int Index, stdstr & Value ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName,Index);
	return CSettingTypeApplication::Load(0,Value);
}

//Update the settings
void CSettingTypeApplicationIndex::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeApplicationIndex::Save ( int Index, ULONG Value )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName,Index);
	CSettingTypeApplication::Save(0,Value);
}

void CSettingTypeApplicationIndex::Save ( int Index, const stdstr & Value )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName,Index);
	CSettingTypeApplication::Save(0,Value);
}

void CSettingTypeApplicationIndex::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
