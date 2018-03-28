/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "SettingsType-TempString.h"

CSettingTypeTempString::CSettingTypeTempString(const char * initialValue) :
    m_value(initialValue)
{
}

CSettingTypeTempString::~CSettingTypeTempString ( void )
{
}

bool CSettingTypeTempString::Load (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeTempString::Load (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeTempString::Load (uint32_t /*Index*/, std::string & Value ) const
{
    Value = m_value;
    return true;
}

//return the default values
void CSettingTypeTempString::LoadDefault (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::LoadDefault (uint32_t /*Index*/, uint32_t & /*Value*/  ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::Save (uint32_t /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::Save (uint32_t /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::Save (uint32_t /*Index*/, const std::string & Value )
{
    m_value = Value;
}

void CSettingTypeTempString::Save (uint32_t /*Index*/, const char * Value )
{
    m_value = Value;
}

void CSettingTypeTempString::Delete(uint32_t /*Index*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}
