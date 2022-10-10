#include "stdafx.h"

#include "SettingsType-SelectedDirectory.h"

CSettingTypeSelectedDirectory::CSettingTypeSelectedDirectory(const char * Name, SettingID InitialDir, SettingID SelectedDir, SettingID UseSelected, SettingID NotifyChangeId) :
    m_Name(Name),
    m_InitialDir(InitialDir),
    m_SelectedDir(SelectedDir),
    m_UseSelected(UseSelected),
    m_NotifyChangeId(NotifyChangeId)
{
    g_Settings->RegisterChangeCB(m_InitialDir, this, (CSettings::SettingChangedFunc)DirectoryChanged);
    g_Settings->RegisterChangeCB(m_SelectedDir, this, (CSettings::SettingChangedFunc)DirectoryChanged);
    g_Settings->RegisterChangeCB(m_UseSelected, this, (CSettings::SettingChangedFunc)DirectoryChanged);
}

CSettingTypeSelectedDirectory::~CSettingTypeSelectedDirectory()
{
    g_Settings->UnregisterChangeCB(m_InitialDir, this, (CSettings::SettingChangedFunc)DirectoryChanged);
    g_Settings->UnregisterChangeCB(m_SelectedDir, this, (CSettings::SettingChangedFunc)DirectoryChanged);
    g_Settings->UnregisterChangeCB(m_UseSelected, this, (CSettings::SettingChangedFunc)DirectoryChanged);
}

bool CSettingTypeSelectedDirectory::IsSettingSet(void) const
{
    SettingID DirSettingId = g_Settings->LoadBool(m_UseSelected) ? m_SelectedDir : m_InitialDir;
    return g_Settings->IsSettingSet(DirSettingId);
}

bool CSettingTypeSelectedDirectory::Load(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeSelectedDirectory::Load(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeSelectedDirectory::Load(uint32_t /*Index*/, std::string & Value) const
{
    SettingID DirSettingId = g_Settings->LoadBool(m_UseSelected) ? m_SelectedDir : m_InitialDir;
    return g_Settings->LoadStringVal(DirSettingId, Value);
}

// Return the default values
void CSettingTypeSelectedDirectory::LoadDefault(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeSelectedDirectory::LoadDefault(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeSelectedDirectory::LoadDefault(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// Update the settings
void CSettingTypeSelectedDirectory::Save(uint32_t /*Index*/, bool /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeSelectedDirectory::Save(uint32_t /*Index*/, uint32_t /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeSelectedDirectory::Save(uint32_t Index, const std::string & Value)
{
    Save(Index, Value.c_str());
}

void CSettingTypeSelectedDirectory::Save(uint32_t /*Index*/, const char * Value)
{
    g_Settings->SaveBool(m_UseSelected, true);
    g_Settings->SaveString(m_SelectedDir, Value);
}

void CSettingTypeSelectedDirectory::Delete(uint32_t /*Index*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeSelectedDirectory::DirectoryChanged(CSettingTypeSelectedDirectory * _this)
{
    g_Settings->NotifyCallBacks(_this->m_NotifyChangeId);
}
