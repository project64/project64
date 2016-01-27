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
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBOnOff.h"

CSettingTypeRDBOnOff::CSettingTypeRDBOnOff(const char * Name, SettingID DefaultSetting ) :
    CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBOnOff::CSettingTypeRDBOnOff(const char * Name, int DefaultValue ) :
    CSettingTypeRomDatabase(Name,DefaultValue)
{
}

CSettingTypeRDBOnOff::~CSettingTypeRDBOnOff()
{
}

bool CSettingTypeRDBOnOff::Load ( int Index, bool & Value ) const
{
    stdstr strValue;
    bool bRes = m_SettingsIniFile->GetString(m_SectionIdent->c_str(),m_KeyName.c_str(),m_DefaultStr,strValue);
    if (!bRes)
    {
        LoadDefault(Index,Value);
        return false;
    }
    const char * String = strValue.c_str();

    if (_stricmp(String,"On") == 0)    { Value = true; }
    else if (_stricmp(String,"Off") == 0)  { Value = false; }
    else if (_stricmp(String,"Global") == 0 || _stricmp(String,"default"))
    {
        LoadDefault(Index,Value);
        return false;
    }
    else { g_Notify->BreakPoint(__FILE__, __LINE__); }

    return true;
}

bool CSettingTypeRDBOnOff::Load ( int /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDBOnOff::Load ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//return the default values
void CSettingTypeRDBOnOff::LoadDefault ( int /*Index*/, bool & Value ) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        } else {
            g_Settings->LoadBool(m_DefaultSetting,Value);
        }
    }
}

void CSettingTypeRDBOnOff::LoadDefault ( int /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBOnOff::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

//Update the settings
void CSettingTypeRDBOnOff::Save ( int /*Index*/, bool Value )
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),Value? "On" : "Off");
}

void CSettingTypeRDBOnOff::Save ( int /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBOnOff::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBOnOff::Save ( int /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBOnOff::Delete( int /*Index*/ )
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),NULL);
}
