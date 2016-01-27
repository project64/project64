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

bool CSettingTypeTempString::Load ( int /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeTempString::Load ( int /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeTempString::Load ( int /*Index*/, stdstr & Value ) const
{
    Value = m_value;
    return true;
}

//return the default values
void CSettingTypeTempString::LoadDefault ( int /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::LoadDefault ( int /*Index*/, uint32_t & /*Value*/  ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::Save ( int /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::Save ( int /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempString::Save ( int /*Index*/, const stdstr & Value )
{
    m_value = Value;
}

void CSettingTypeTempString::Save ( int /*Index*/, const char * Value )
{
    m_value = Value;
}

void CSettingTypeTempString::Delete( int /*Index*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}
