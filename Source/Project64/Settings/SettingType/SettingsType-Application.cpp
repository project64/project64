/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "SettingsType-Application.h"
#include <Common/path.h>

bool       CSettingTypeApplication::m_UseRegistry     = false;
CIniFile * CSettingTypeApplication::m_SettingsIniFile = NULL;

CSettingTypeApplication::CSettingTypeApplication(const char * Section, const char * Name, uint32_t DefaultValue ) :
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::CSettingTypeApplication(const char * Section, const char * Name, bool DefaultValue ) :
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::CSettingTypeApplication(const char * Section, const char * Name, const char * DefaultValue ) :
    m_DefaultStr(DefaultValue),
    m_DefaultValue(0),
    m_DefaultSetting(Default_Constant),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::CSettingTypeApplication(const char * Section, const char * Name, SettingID DefaultSetting ) :
    m_DefaultStr(""),
    m_DefaultValue(0),
    m_DefaultSetting(DefaultSetting),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeApplication::~CSettingTypeApplication()
{
}

void CSettingTypeApplication::Initialize( const char * /*AppName*/ )
{
    stdstr SettingsFile, OrigSettingsFile;

    for (int i = 0; i < 100; i++)
    {
        OrigSettingsFile = SettingsFile;
        if (!g_Settings->LoadStringVal(SupportFile_Settings,SettingsFile) && i > 0)
        {
            break;
        }
        if (SettingsFile == OrigSettingsFile)
        {
            break;
        }
        if (m_SettingsIniFile)
        {
            delete m_SettingsIniFile;
        }
        CPath SettingsDir(CPath(SettingsFile).GetDriveDirectory(),"");
        if (!SettingsDir.DirectoryExists())
        {
			SettingsDir.DirectoryCreate();
        }

        m_SettingsIniFile = new CIniFile(SettingsFile.c_str());
    }

    m_SettingsIniFile->SetAutoFlush(false);
    m_UseRegistry = g_Settings->LoadBool(Setting_UseFromRegistry);
}

void CSettingTypeApplication::Flush()
{
    if (m_SettingsIniFile)
    {
        m_SettingsIniFile->FlushChanges();
    }
}

void CSettingTypeApplication::CleanUp()
{
    if (m_SettingsIniFile)
    {
        m_SettingsIniFile->SetAutoFlush(true);
        delete m_SettingsIniFile;
        m_SettingsIniFile = NULL;
    }
}

bool CSettingTypeApplication::Load ( int /*Index*/, bool & Value ) const
{
    bool bRes = false;

    if (!m_UseRegistry)
    {
        uint32_t dwValue;
        bRes = m_SettingsIniFile->GetNumber(SectionName(),m_KeyNameIdex.c_str(),Value,dwValue);
        if (bRes)
        {
            Value = dwValue != 0;
        }
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }

    if (!bRes && m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        } else {
            g_Settings->LoadBool(m_DefaultSetting,Value);
        }
    }
    return bRes;
}

bool CSettingTypeApplication::Load ( int /*Index*/, uint32_t & Value ) const
{
    bool bRes = false;
    if (!m_UseRegistry)
    {
        bRes = m_SettingsIniFile->GetNumber(SectionName(),m_KeyNameIdex.c_str(),Value,Value);
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
    if (!bRes && m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue;
        } else {
            g_Settings->LoadDword(m_DefaultSetting,Value);
        }
    }
    return bRes;
}

const char * CSettingTypeApplication::SectionName ( void ) const
{
    return m_Section.c_str();
}

bool CSettingTypeApplication::Load ( int Index, stdstr & Value ) const
{
    bool bRes = false;
    if (!m_UseRegistry)
    {
        bRes = m_SettingsIniFile ? m_SettingsIniFile->GetString(SectionName(),m_KeyNameIdex.c_str(),m_DefaultStr,Value) : false;
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
    if (!bRes)
    {
        CSettingTypeApplication::LoadDefault(Index,Value);
    }
    return bRes;
}

//return the default values
void CSettingTypeApplication::LoadDefault ( int /*Index*/, bool & Value   ) const
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

void CSettingTypeApplication::LoadDefault ( int /*Index*/, uint32_t & Value  ) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue;
        } else {
            g_Settings->LoadDword(m_DefaultSetting,Value);
        }
    }
}

void CSettingTypeApplication::LoadDefault ( int /*Index*/, stdstr & Value ) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultStr;
        } else {
            g_Settings->LoadStringVal(m_DefaultSetting,Value);
        }
    }
}

//Update the settings
void CSettingTypeApplication::Save ( int /*Index*/, bool Value )
{
    if (!m_UseRegistry)
    {
        m_SettingsIniFile->SaveNumber(SectionName(),m_KeyNameIdex.c_str(),Value);
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
}

void CSettingTypeApplication::Save ( int /*Index*/, uint32_t Value )
{
    if (!m_UseRegistry)
    {
        m_SettingsIniFile->SaveNumber(SectionName(),m_KeyNameIdex.c_str(),Value);
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
}

void CSettingTypeApplication::Save ( int /*Index*/, const stdstr & Value )
{
    if (!m_UseRegistry)
    {
        m_SettingsIniFile->SaveString(SectionName(),m_KeyNameIdex.c_str(),Value.c_str());
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
}

void CSettingTypeApplication::Save ( int /*Index*/, const char * Value )
{
    if (!m_UseRegistry)
    {
        m_SettingsIniFile->SaveString(SectionName(),m_KeyNameIdex.c_str(),Value);
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
}

stdstr CSettingTypeApplication::FixSectionName(const char * Section)
{
    stdstr SectionName(Section);

    if (!m_UseRegistry)
    {
        if (SectionName.empty())
        {
            SectionName = "default";
        }
        SectionName.Replace("\\","-");
    }
    return SectionName;
}

void CSettingTypeApplication::Delete( int /*Index*/ )
{
    if (!m_UseRegistry)
    {
        m_SettingsIniFile->SaveString(SectionName(),m_KeyNameIdex.c_str(),NULL);
    } else {
        g_Notify->BreakPoint(__FILEW__,__LINE__);
    }
}
