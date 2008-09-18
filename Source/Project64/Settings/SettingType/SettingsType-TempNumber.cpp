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

void CSettingTypeTempNumber::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Save ( int Index, ULONG Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempNumber::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
