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
#include "SettingsType-Application.h"
#include <Common/path.h>

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
	CPath BaseDir(g_Settings->LoadStringVal(Cmd_BaseDirectory).c_str(),"");
	if (!BaseDir.DirectoryExists())
	{
		printf("BaseDir does not exists, doing nothing");
		return;
	}

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
#ifdef _WIN32
        CPath SettingsDir(CPath(SettingsFile).GetDriveDirectory(),"");
#else
		CPath SettingsDir(CPath(SettingsFile).GetDirectory(), "");
#endif
        if (!SettingsDir.DirectoryExists())
        {
			SettingsDir.DirectoryCreate();
        }

        m_SettingsIniFile = new CIniFile(SettingsFile.c_str());
    }

    m_SettingsIniFile->SetAutoFlush(false);
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

    uint32_t dwValue;
    bRes = m_SettingsIniFile->GetNumber(SectionName(),m_KeyNameIdex.c_str(),Value,dwValue);
    if (bRes)
    {
        Value = dwValue != 0;
    }

    if (!bRes && m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else 
		{
            g_Settings->LoadBool(m_DefaultSetting,Value);
        }
    }
    return bRes;
}

bool CSettingTypeApplication::Load ( int /*Index*/, uint32_t & Value ) const
{
    bool bRes = m_SettingsIniFile->GetNumber(SectionName(),m_KeyNameIdex.c_str(),Value,Value);
    if (!bRes && m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue;
        }
        else 
		{
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
    bool bRes = m_SettingsIniFile ? m_SettingsIniFile->GetString(SectionName(),m_KeyNameIdex.c_str(),m_DefaultStr,Value) : false;
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
        }
        else 
		{
            g_Settings->LoadBool(m_DefaultSetting,Value);
        }
    }
}

void CSettingTypeApplication::LoadDefault(int /*Index*/, uint32_t & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue;
        }
        else 
		{
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
        }
        else 
		{
            g_Settings->LoadStringVal(m_DefaultSetting,Value);
        }
    }
}

//Update the settings
void CSettingTypeApplication::Save ( int /*Index*/, bool Value )
{
    m_SettingsIniFile->SaveNumber(SectionName(),m_KeyNameIdex.c_str(),Value);
}

void CSettingTypeApplication::Save ( int /*Index*/, uint32_t Value )
{
    m_SettingsIniFile->SaveNumber(SectionName(),m_KeyNameIdex.c_str(),Value);
}

void CSettingTypeApplication::Save ( int /*Index*/, const stdstr & Value )
{
    m_SettingsIniFile->SaveString(SectionName(),m_KeyNameIdex.c_str(),Value.c_str());
}

void CSettingTypeApplication::Save ( int /*Index*/, const char * Value )
{
    m_SettingsIniFile->SaveString(SectionName(),m_KeyNameIdex.c_str(),Value);
}

stdstr CSettingTypeApplication::FixSectionName(const char * Section)
{
    stdstr SectionName(Section);

    if (SectionName.empty())
    {
        SectionName = "default";
    }
    SectionName.Replace("\\","-");
    return SectionName;
}

void CSettingTypeApplication::Delete( int /*Index*/ )
{
    m_SettingsIniFile->SaveString(SectionName(),m_KeyNameIdex.c_str(),NULL);
}
