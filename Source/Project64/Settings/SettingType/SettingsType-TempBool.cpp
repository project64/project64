#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-TempBool.h"

CSettingTypeTempBool::CSettingTypeTempBool(bool initialValue) :
	m_value(initialValue)
{
}

bool CSettingTypeTempBool::Load ( int Index, bool & Value ) const
{
	Value = m_value;
	return true;
}

bool CSettingTypeTempBool::Load ( int Index, ULONG & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeTempBool::Load ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

void CSettingTypeTempBool::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempBool::Save ( int Index, ULONG Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempBool::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeTempBool::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
