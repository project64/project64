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
#include "SettingsType-TempNumber.h"

CSettingTypeTempNumber::CSettingTypeTempNumber(uint32_t initialValue) :
    m_value(initialValue),
    m_initialValue(initialValue)
{
}

CSettingTypeTempNumber::~CSettingTypeTempNumber ( void )
{
}

bool CSettingTypeTempNumber::Load ( int /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return true;
}

bool CSettingTypeTempNumber::Load ( int /*Index*/, uint32_t & Value ) const
{
    Value = m_value;
    return false;
}

bool CSettingTypeTempNumber::Load ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//return the default values
void CSettingTypeTempNumber::LoadDefault ( int /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::LoadDefault ( int /*Index*/, uint32_t & Value ) const
{
   Value = m_initialValue;
}

void CSettingTypeTempNumber::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Save ( int /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Save ( int /*Index*/, uint32_t Value )
{
    m_value = Value;
}

void CSettingTypeTempNumber::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Save ( int /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempNumber::Delete( int /*Index*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}
