#include "stdafx.h"

#include "SettingsType-Application.h"
#include "SettingsType-GameSetting.h"

bool CSettingTypeGame::m_RdbEditor = false;
bool CSettingTypeGame::m_EraseDefaults = true;
std::string * CSettingTypeGame::m_SectionIdent = nullptr;

CSettingTypeGame::CSettingTypeGame(const char * Name, bool DefaultValue) :
    CSettingTypeApplication("", Name, DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(const char * Name, const char * DefaultValue) :
    CSettingTypeApplication("", Name, DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(const char * Name, uint32_t DefaultValue) :
    CSettingTypeApplication("", Name, DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(const char * Name, SettingID DefaultSetting) :
    CSettingTypeApplication("", Name, DefaultSetting)
{
}

CSettingTypeGame::~CSettingTypeGame()
{
}

void CSettingTypeGame::Initialize(void)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");
    UpdateSettings(nullptr);
    g_Settings->RegisterChangeCB(Game_IniKey, nullptr, UpdateSettings);
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeGame::CleanUp(void)
{
    g_Settings->UnregisterChangeCB(Game_IniKey, nullptr, UpdateSettings);
    if (m_SectionIdent)
    {
        delete m_SectionIdent;
        m_SectionIdent = nullptr;
    }
}

const char * CSettingTypeGame::SectionName(void) const
{
    return m_SectionIdent ? m_SectionIdent->c_str() : "";
}

void CSettingTypeGame::UpdateSettings(void * /*Data */)
{
    m_RdbEditor = g_Settings->LoadBool(Setting_RdbEditor);
    m_EraseDefaults = g_Settings->LoadBool(Setting_EraseGameDefaults);
    stdstr SectionIdent = g_Settings->LoadStringVal(Game_IniKey);

    if (m_SectionIdent == nullptr)
    {
        m_SectionIdent = new stdstr;
    }
    if (SectionIdent != *m_SectionIdent)
    {
        *m_SectionIdent = SectionIdent;
        g_Settings->SettingTypeChanged(SettingType_GameSetting);
        g_Settings->SettingTypeChanged(SettingType_RomDatabase);
    }
}

bool CSettingTypeGame::Load(uint32_t Index, bool & Value) const
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            return g_Settings->LoadBoolIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            return g_Settings->LoadBool(m_DefaultSetting, Value);
        }
    }
    return CSettingTypeApplication::Load(Index, Value);
}

bool CSettingTypeGame::Load(uint32_t Index, uint32_t & Value) const
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            return g_Settings->LoadDwordIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            return g_Settings->LoadDword(m_DefaultSetting, Value);
        }
    }
    return CSettingTypeApplication::Load(Index, Value);
}

bool CSettingTypeGame::Load(uint32_t Index, std::string & Value) const
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            return g_Settings->LoadStringIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            return g_Settings->LoadStringVal(m_DefaultSetting, Value);
        }
    }
    return CSettingTypeApplication::Load(Index, Value);
}

// Return the default values
void CSettingTypeGame::LoadDefault(uint32_t Index, bool & Value) const
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->LoadDefaultBoolIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->LoadDefaultBool(m_DefaultSetting, Value);
        }
    }
    else
    {
        CSettingTypeApplication::LoadDefault(Index, Value);
    }
}

void CSettingTypeGame::LoadDefault(uint32_t Index, uint32_t & Value) const
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->LoadDefaultDwordIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->LoadDefaultDword(m_DefaultSetting, Value);
        }
    }
    else
    {
        CSettingTypeApplication::LoadDefault(Index, Value);
    }
}

void CSettingTypeGame::LoadDefault(uint32_t Index, std::string & Value) const
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->LoadDefaultStringIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->LoadDefaultString(m_DefaultSetting, Value);
        }
    }
    else
    {
        CSettingTypeApplication::LoadDefault(Index, Value);
    }
}

// Update the settings
void CSettingTypeGame::Save(uint32_t Index, bool Value)
{
    if (m_EraseDefaults)
    {
        bool bDefault;
        LoadDefault(Index, bDefault);
        if (bDefault == Value)
        {
            Delete(Index);
            return;
        }
    }
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->SaveBoolIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->SaveBool(m_DefaultSetting, Value);
        }
    }
    else
    {
        CSettingTypeApplication::Save(Index, Value);
    }
}

void CSettingTypeGame::Save(uint32_t Index, uint32_t Value)
{
    if (m_EraseDefaults)
    {
        uint32_t ulDefault;
        CSettingTypeGame::LoadDefault(Index, ulDefault);
        if (ulDefault == Value)
        {
            Delete(Index);
            return;
        }
    }
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->SaveDwordIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->SaveDword(m_DefaultSetting, Value);
        }
    }
    else
    {
        CSettingTypeApplication::Save(Index, Value);
    }
}

void CSettingTypeGame::Save(uint32_t Index, const std::string & Value)
{
    Save(Index, Value.c_str());
}

void CSettingTypeGame::Save(uint32_t Index, const char * Value)
{
    if (m_EraseDefaults && m_DefaultSetting != Rdb_GoodName)
    {
        stdstr szDefault;
        CSettingTypeGame::LoadDefault(Index, szDefault);
        if (_stricmp(szDefault.c_str(), Value) == 0)
        {
            Delete(Index);
            return;
        }
    }
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->SaveStringIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->SaveString(m_DefaultSetting, Value);
        }
    }
    else
    {
        CSettingTypeApplication::Save(Index, Value);
    }
}

void CSettingTypeGame::Delete(uint32_t Index)
{
    if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
    {
        if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->DeleteSettingIndex(m_DefaultSetting, Index);
        }
        else
        {
            g_Settings->DeleteSetting(m_DefaultSetting);
        }
    }
    else
    {
        CSettingTypeApplication::Delete(Index);
    }
}