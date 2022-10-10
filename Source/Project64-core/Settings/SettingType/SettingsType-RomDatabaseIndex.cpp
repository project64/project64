#include "stdafx.h"

#include "SettingsType-RomDatabase.h"
#include "SettingsType-RomDatabaseIndex.h"

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, const char * DefaultValue) :
    CSettingTypeRomDatabase("", DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, bool DefaultValue) :
    CSettingTypeRomDatabase("", DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, uint32_t DefaultValue) :
    CSettingTypeRomDatabase("", DefaultValue, false),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, SettingID DefaultSetting) :
    CSettingTypeRomDatabase("", DefaultSetting),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::~CSettingTypeRomDatabaseIndex()
{
}

bool CSettingTypeRomDatabaseIndex::Load(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRomDatabaseIndex::Load(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRomDatabaseIndex::Load(uint32_t Index, std::string & Value) const
{
    m_KeyName = stdstr_f("%s%d%s", m_PreIndex.c_str(), Index, m_PostIndex.c_str());
    return CSettingTypeRomDatabase::Load(0, Value);
}

void CSettingTypeRomDatabaseIndex::LoadDefault(uint32_t Index, bool & Value) const
{
    m_KeyName = stdstr_f("%s%d%s", m_PreIndex.c_str(), Index, m_PostIndex.c_str());
    CSettingTypeRomDatabase::LoadDefault(0, Value);
}

void CSettingTypeRomDatabaseIndex::LoadDefault(uint32_t Index, uint32_t & Value) const
{
    m_KeyName = stdstr_f("%s%d%s", m_PreIndex.c_str(), Index, m_PostIndex.c_str());
    CSettingTypeRomDatabase::LoadDefault(0, Value);
}

void CSettingTypeRomDatabaseIndex::LoadDefault(uint32_t Index, std::string & Value) const
{
    m_KeyName = stdstr_f("%s%d%s", m_PreIndex.c_str(), Index, m_PostIndex.c_str());
    CSettingTypeRomDatabase::LoadDefault(0, Value);
}

void CSettingTypeRomDatabaseIndex::Save(uint32_t /*Index*/, bool /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Save(uint32_t /*Index*/, uint32_t /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Save(uint32_t /*Index*/, const std::string & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Save(uint32_t /*Index*/, const char * /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Delete(uint32_t /*Index*/)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), nullptr);
}
