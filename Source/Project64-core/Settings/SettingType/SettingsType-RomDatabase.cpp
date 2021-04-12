#include "stdafx.h"
#include "SettingsType-RomDatabase.h"

CIniFile * CSettingTypeRomDatabase::m_SettingsIniFile = nullptr;
CIniFile * CSettingTypeRomDatabase::m_VideoIniFile = nullptr;
CIniFile * CSettingTypeRomDatabase::m_AudioIniFile = nullptr;
std::string * CSettingTypeRomDatabase::m_SectionIdent = nullptr;

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, uint32_t DefaultValue, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault),
    m_VideoSetting(IsVideoSetting(Name)),
    m_AudioSetting(IsAudioSetting(Name))
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, bool DefaultValue, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault),
    m_VideoSetting(IsVideoSetting(Name)),
    m_AudioSetting(IsAudioSetting(Name))
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, const char * DefaultValue, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(DefaultValue),
    m_DefaultValue(0),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault),
    m_VideoSetting(IsVideoSetting(Name)),
    m_AudioSetting(IsAudioSetting(Name))
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(const char * Name, SettingID DefaultSetting, bool DeleteOnDefault) :
    m_KeyName(StripNameSection(Name)),
    m_DefaultStr(""),
    m_DefaultValue(0),
    m_DefaultSetting(DefaultSetting),
    m_DeleteOnDefault(DeleteOnDefault),
    m_VideoSetting(IsVideoSetting(Name)),
    m_AudioSetting(IsAudioSetting(Name))
{
}

CSettingTypeRomDatabase::~CSettingTypeRomDatabase()
{
}

void CSettingTypeRomDatabase::Initialize(void)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");

    m_SettingsIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
    m_VideoIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_VideoRDB).c_str());
    m_AudioIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_AudioRDB).c_str());

    g_Settings->RegisterChangeCB(Game_IniKey, nullptr, GameChanged);
    g_Settings->RegisterChangeCB(Cmd_BaseDirectory, nullptr, BaseDirChanged);

    m_SectionIdent = new stdstr(g_Settings->LoadStringVal(Game_IniKey));
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeRomDatabase::CleanUp(void)
{
    g_Settings->UnregisterChangeCB(Cmd_BaseDirectory, nullptr, BaseDirChanged);
    g_Settings->UnregisterChangeCB(Game_IniKey, nullptr, GameChanged);
    if (m_SettingsIniFile)
    {
        delete m_SettingsIniFile;
        m_SettingsIniFile = nullptr;
    }
    if (m_VideoIniFile)
    {
        delete m_VideoIniFile;
        m_VideoIniFile = nullptr;
    }
    if (m_AudioIniFile)
    {
        delete m_AudioIniFile;
        m_AudioIniFile = nullptr;
    }
    if (m_SectionIdent)
    {
        delete m_SectionIdent;
        m_SectionIdent = nullptr;
    }
}

void CSettingTypeRomDatabase::BaseDirChanged(void * /*Data */)
{
    if (m_SettingsIniFile)
    {
        delete m_SettingsIniFile;
        m_SettingsIniFile = nullptr;
    }
    if (m_VideoIniFile)
    {
        delete m_VideoIniFile;
        m_VideoIniFile = nullptr;
    }
    if (m_AudioIniFile)
    {
        delete m_AudioIniFile;
        m_AudioIniFile = nullptr;
    }
    m_SettingsIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
    m_VideoIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_VideoRDB).c_str());
    m_AudioIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_AudioRDB).c_str());
}

void CSettingTypeRomDatabase::GameChanged(void * /*Data */)
{
    if (m_SectionIdent)
    {
        *m_SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    }
}

bool CSettingTypeRomDatabase::Load(uint32_t & Value) const
{
	bool bRes = false;
	if (m_VideoSetting)
	{
		bRes = m_VideoIniFile->GetNumber(Section(), m_KeyName.c_str(), Value, Value);
	}
	else if (m_AudioSetting)
	{
		bRes = m_AudioIniFile->GetNumber(Section(), m_KeyName.c_str(), Value, Value);
	}
	else
	{
		bRes = m_SettingsIniFile->GetNumber(Section(), m_KeyName.c_str(), Value, Value);
	}
	return bRes;
}

bool CSettingTypeRomDatabase::Load(uint32_t Index, bool & Value) const
{
    uint32_t temp_value = Value;
	if (Load(temp_value))
	{
		Value = temp_value != 0;
		return true;
	}
	LoadDefault(Index, Value);
	return false;
}

bool CSettingTypeRomDatabase::Load(uint32_t Index, uint32_t & Value) const
{
    if (!Load(Value))
    {
        LoadDefault(Index, Value);
		return false;
	}
    return true;
}

bool CSettingTypeRomDatabase::Load(uint32_t Index, std::string & Value) const
{
    stdstr temp_value;
    bool bRes = false;
    if (m_VideoSetting)
    {
        bRes = m_VideoIniFile->GetString(Section(), m_KeyName.c_str(), m_DefaultStr, temp_value);
    }
    else if (m_AudioSetting)
    {
        bRes = m_AudioIniFile->GetString(Section(), m_KeyName.c_str(), m_DefaultStr, temp_value);
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
void CSettingTypeRomDatabase::LoadDefault(uint32_t /*Index*/, bool & Value) const
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

void CSettingTypeRomDatabase::LoadDefault(uint32_t /*Index*/, uint32_t & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        Value = m_DefaultSetting == Default_Constant ? m_DefaultValue : g_Settings->LoadDword(m_DefaultSetting);
    }
}

void CSettingTypeRomDatabase::LoadDefault(uint32_t /*Index*/, std::string & Value) const
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
void CSettingTypeRomDatabase::Save(uint32_t /*Index*/, bool Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_DeleteOnDefault)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (m_VideoSetting)
    {
        m_VideoIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
    else if (m_AudioSetting)
    {
        m_AudioIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
    else
    {
        m_SettingsIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
}

void CSettingTypeRomDatabase::Save(uint32_t Index, uint32_t Value)
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
    if (m_VideoSetting)
    {
        m_VideoIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
    else if (m_AudioSetting)
    {
        m_AudioIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
    else
    {
        m_SettingsIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
    }
}

void CSettingTypeRomDatabase::Save(uint32_t /*Index*/, const std::string & Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_VideoSetting)
    {
        m_VideoIniFile->SaveString(Section(), m_KeyName.c_str(), Value.c_str());
    }
    else if (m_AudioSetting)
    {
        m_AudioIniFile->SaveString(Section(), m_KeyName.c_str(), Value.c_str());
    }
    else
    {
        m_SettingsIniFile->SaveString(Section(), m_KeyName.c_str(), Value.c_str());
    }
}

void CSettingTypeRomDatabase::Save(uint32_t /*Index*/, const char * Value)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_VideoSetting)
    {
        m_VideoIniFile->SaveString(Section(), m_KeyName.c_str(), Value);
    }
    else if (m_AudioSetting)
    {
        m_AudioIniFile->SaveString(Section(), m_KeyName.c_str(), Value);
    }
    else
    {
        m_SettingsIniFile->SaveString(Section(), m_KeyName.c_str(), Value);
    }
}

void CSettingTypeRomDatabase::Delete(uint32_t /*Index*/)
{
    if (!g_Settings->LoadBool(Setting_RdbEditor))
    {
        return;
    }
    if (m_VideoSetting)
    {
        m_VideoIniFile->SaveString(Section(), m_KeyName.c_str(), nullptr);
    }
    else if (m_AudioSetting)
    {
        m_AudioIniFile->SaveString(Section(), m_KeyName.c_str(), nullptr);
    }
    else
    {
        m_SettingsIniFile->SaveString(Section(), m_KeyName.c_str(), nullptr);
    }
}

bool CSettingTypeRomDatabase::IsVideoSetting(const char * Name)
{
    if (_strnicmp(Name, "Video-", 6) == 0)
    {
        return true;
    }
    return false;
}

bool CSettingTypeRomDatabase::IsAudioSetting(const char * Name)
{
    if (_strnicmp(Name, "Audio-", 6) == 0)
    {
        return true;
    }
    return false;
}

const char * CSettingTypeRomDatabase::StripNameSection(const char * Name)
{
    if (_strnicmp(Name, "Video-", 6) == 0)
    {
        return &Name[6];
    }
    if (_strnicmp(Name, "Audio-", 6) == 0)
    {
        return &Name[6];
    }
    return Name;
}