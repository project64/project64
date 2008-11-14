#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-TempNumber.h"

CSettingTypeTempNumber::CSettingTypeTempNumber(ULONG initialValue) :
	m_value(initialValue)
{
}

bool CSettingTypeTempNumber::Load ( int Index, bool & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return true;
}

bool CSettingTypeTempNumber::Load ( int Index, ULONG & Value ) const
{
	Value = m_value;
	return false;
}

bool CSettingTypeTempNumber::Load ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

//return the default values
void CSettingTypeTempNumber::LoadDefault ( int Index, bool & Value   ) const
{
	Notify().BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeTempNumber::LoadDefault ( int Index, ULONG & Value  ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::LoadDefault ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Save ( int Index, ULONG Value )
{
	m_value = Value;
}

void CSettingTypeTempNumber::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Delete( int Index )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
