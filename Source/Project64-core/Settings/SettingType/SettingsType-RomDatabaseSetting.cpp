#include "stdafx.h"

#include "SettingsType-RomDatabaseSetting.h"

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, uint32_t DefaultValue, bool DeleteOnDefault) :
    CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
    m_SectionIdent(SectionIdent)
{
    m_SectionIdent.Replace("\\", "-");
    if (!m_VideoSetting || !m_AudioSetting)
    {
        m_VideoSetting = IsVideoSetting(m_SectionIdent.c_str());
        m_AudioSetting = IsAudioSetting(m_SectionIdent.c_str());
        m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
    }
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, bool DefaultValue, bool DeleteOnDefault) :
    CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
    m_SectionIdent(SectionIdent)
{
    m_SectionIdent.Replace("\\", "-");
    if (!m_VideoSetting || !m_AudioSetting)
    {
        m_VideoSetting = IsVideoSetting(m_SectionIdent.c_str());
        m_AudioSetting = IsAudioSetting(m_SectionIdent.c_str());
        m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
    }
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, const char * DefaultValue, bool DeleteOnDefault) :
    CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
    m_SectionIdent(SectionIdent)
{
    m_SectionIdent.Replace("\\", "-");
    if (!m_VideoSetting || !m_AudioSetting)
    {
        m_VideoSetting = IsVideoSetting(m_SectionIdent.c_str());
        m_AudioSetting = IsAudioSetting(m_SectionIdent.c_str());
        m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
    }
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, SettingID DefaultSetting, bool DeleteOnDefault) :
    CSettingTypeRomDatabase(Name, DefaultSetting, DeleteOnDefault),
    m_SectionIdent(SectionIdent)
{
    m_SectionIdent.Replace("\\", "-");
    if (!m_VideoSetting || !m_AudioSetting)
    {
        m_VideoSetting = IsVideoSetting(m_SectionIdent.c_str());
        m_AudioSetting = IsAudioSetting(m_SectionIdent.c_str());
        m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
    }
}

CSettingTypeRomDatabaseSetting::~CSettingTypeRomDatabaseSetting()
{
}