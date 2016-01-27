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
#include "SettingsType-TempBool.h"

CSettingTypeTempBool::CSettingTypeTempBool(bool initialValue) :
    m_value(initialValue)
{
}

CSettingTypeTempBool::~CSettingTypeTempBool ( void )
{
}

bool CSettingTypeTempBool::Load ( int /*Index*/, bool & Value ) const
{
    Value = m_value;
    return true;
}

bool CSettingTypeTempBool::Load ( int /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeTempBool::Load ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//return the default values
void CSettingTypeTempBool::LoadDefault ( int /*Index*/, bool & /*Value*/   ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempBool::LoadDefault ( int /*Index*/, uint32_t & /*Value*/  ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempBool::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempBool::Save ( int /*Index*/, bool Value )
{
    m_value = Value;
}

void CSettingTypeTempBool::Save ( int /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempBool::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempBool::Save ( int /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeTempBool::Delete( int /*Index*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}
