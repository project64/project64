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

CIniFile * CSettingTypeRomDatabase::m_SettingsIniFile = NULL;
CIniFile * CSettingTypeRomDatabase::m_Project64IniFile = NULL;
stdstr   * CSettingTypeRomDatabase::m_SectionIdent = NULL;

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, int DefaultValue, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault),
    m_GlideSetting(IsGlideSetting(Name))
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, bool DefaultValue, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault),
    m_GlideSetting(IsGlideSetting(Name))
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, const char * DefaultValue, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(DefaultValue),
    m_DefaultValue(0),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault),
    m_GlideSetting(IsGlideSetting(Name))
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, SettingID DefaultSetting, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(""),
    m_DefaultValue(0),
    m_DefaultSetting(DefaultSetting),
    m_DeleteOnDefault(DeleteOnDefault),
    m_GlideSetting(IsGlideSetting(Name))
{
}

CSettingTypeRomDatabase::~CSettingTypeRomDatabase()
{
}

void CSettingTypeRomDatabase::Initialize(void)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");

    m_SettingsIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
    m_Project64IniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Project64VideoRDB).c_str());

    g_Settings->RegisterChangeCB(Game_IniKey, NULL, GameChanged);
    g_Settings->RegisterChangeCB(Cmd_BaseDirectory, NULL, BaseDirChanged);

    m_SectionIdent = new stdstr(g_Settings->LoadStringVal(Game_IniKey));
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeRomDatabase::CleanUp(void)
{
    g_Settings->UnregisterChangeCB(Cmd_BaseDirectory, NULL, BaseDirChanged);
    g_Settings->UnregisterChangeCB(Game_IniKey, NULL, GameChanged);
    if (m_SettingsIniFile)
    {
        delete m_SettingsIniFile;
        m_SettingsIniFile = NULL;
    }
    if (m_Project64IniFile)
    {
        delete m_Project64IniFile;
        m_Project64IniFile = NULL;
    }
    if (m_SectionIdent)
    {
        delete m_SectionIdent;
        m_SectionIdent = NULL;
    }
}

void CSettingTypeRomDatabase::BaseDirChanged(void * /*Data */)
{
    if (m_SettingsIniFile)
    {
        delete m_SettingsIniFile;
        m_SettingsIniFile = NULL;
    }
    if (m_Project64IniFile)
    {
        delete m_Project64IniFile;
        m_Project64IniFile = NULL;
    }
    m_SettingsIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
    m_Project64IniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Project64VideoRDB).c_str());
}

void CSettingTypeRomDatabase::GameChanged(void * /*Data */)
{
    if (m_SectionIdent)
    {
        *m_SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    }
}

bool CSettingTypeRomDatabase::Load(int Index, bool & Value) const
{
    uint32_t temp_value = Value;
    bool bRes = Load(Index, temp_value);
    Value = temp_value != 0;
    return bRes;
}

bool CSettingTypeRomDatabase::Load(int Index, uint32_t & Value) const
{
    bool bRes = false;
    if (m_GlideSetting)
    {
        bRes = m_Project64IniFile->GetNumber(Section(), m_KeyName.c_str(), Value, Value);
    }
    else
    {
        bRes = m_SettingsIniFile->GetNumber(Section(), m_KeyName.c_str(), Value, Value);
    }
    if (!bRes)
    {
        LoadDefault(Index, Value);
    }
    return bRes;
}

bool CSettingTypeRomDatabase::Load(int Index, stdstr & Value) const
{
    stdstr temp_value;
    bool bRes = false;
    if (m_GlideSetting)
    {
        bRes = m_Project64IniFile->GetString(Section(), m_KeyName.c_str(), m_DefaultStr, temp_value);
    }
    else
    {
        bRes = m_SettingsIniFile->GetString(Section(), m_KeyName.c_str(), m_DefaultStr, temp_value);
    }
    if (bRes)
    {
        Value = temp_value;
    }
    else
    {
        LoadDefault(Index, Value);
    }
    return bRes;
}

//return the default values
void CSettingTypeRomDatabase::LoadDefault(int /*Index*/, bool & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else {
            g_Settings->LoadBool(m_DefaultSetting, Value);
        }
    }
}

void CSettingTypeRomDatabase::LoadDefault(int /*Index*/, uint32_t & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue;
        }
        else {
            g_Settings->LoadDword(m_DefaultSetting, Value);
        }
    }
}

void CSettingTypeRomDatabase::LoadDefault(int /*Index*/, stdstr & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultStr;
        }
        else {
            g_Settings->LoadStringVal(m_DefaultSetting, Value);
        }
    }
}

//Update the settings
void CSettingTypeRomDatabase::Save(int /*Index*/, bool Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_DeleteOnDefault)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (m_GlideSetting)
    {
        m_Project64IniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
    else
    {
        m_SettingsIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
}

void CSettingTypeRomDatabase::Save(int Index, uint32_t Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_DeleteOnDefault)
    {
        uint32_t defaultValue = 0;
        LoadDefault(Index, defaultValue);
        if (defaultValue == Value)
        {
            Delete(Index);
            return;
        }
    }
    if (m_GlideSetting)
    {
        m_Project64IniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
    else
    {
        m_SettingsIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
}

void CSettingTypeRomDatabase::Save(int /*Index*/, const stdstr & Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_GlideSetting)
    {
        m_Project64IniFile->SaveString(Section(), m_KeyName.c_str(), Value.c_str());
    }
    else
    {
        m_SettingsIniFile->SaveString(Section(), m_KeyName.c_str(), Value.c_str());
    }
}

void CSettingTypeRomDatabase::Save(int /*Index*/, const char * Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_GlideSetting)
    {
        m_Project64IniFile->SaveString(Section(), m_KeyName.c_str(), Value);
    }
    else
    {
        m_SettingsIniFile->SaveString(Section(), m_KeyName.c_str(), Value);
    }
}

void CSettingTypeRomDatabase::Delete(int /*Index*/)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_GlideSetting)
    {
        m_Project64IniFile->SaveString(Section(), m_KeyName.c_str(), NULL);
    }
    else
    {
        m_SettingsIniFile->SaveString(Section(), m_KeyName.c_str(), NULL);
    }
}

bool CSettingTypeRomDatabase::IsGlideSetting(const char * Name)
{
    if (_strnicmp(Name, "Glide64-", 8) == 0)
    {
        return true;
    }
    return false;
}

const char * CSettingTypeRomDatabase::StripNameSection(const char * Name)
{
    if (_strnicmp(Name, "Glide64-", 8) == 0)
    {
        return &Name[8];
    }
    return Name;
}