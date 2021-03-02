#include "stdafx.h"
#include "SettingsType-TempNumber.h"

CSettingTypeTempNumber::CSettingTypeTempNumber(uint32_t initialValue) :
    m_value(initialValue),
    m_initialValue(initialValue)
{
}

CSettingTypeTempNumber::~CSettingTypeTempNumber ( void )
{
}

bool CSettingTypeTempNumber::Load (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return true;
}

bool CSettingTypeTempNumber::Load (uint32_t /*Index*/, uint32_t & Value ) const
{
    Value = m_value;
    return false;
}

bool CSettingTypeTempNumber::Load (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//return the default values
void CSettingTypeTempNumber::LoadDefault (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::LoadDefault (uint32_t /*Index*/, uint32_t & Value ) const
{
   Value = m_initialValue;
}

void CSettingTypeTempNumber::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Save (uint32_t /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Save (uint32_t /*Index*/, uint32_t Value )
{
    m_value = Value;
}

void CSettingTypeTempNumber::Save (uint32_t /*Index*/, const std::string & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Save (uint32_t /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Delete(uint32_t /*Index*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}
