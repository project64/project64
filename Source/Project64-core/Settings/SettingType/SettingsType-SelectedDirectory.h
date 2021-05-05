#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-Base.h>

class CSettingTypeSelectedDirectory :
    public CSettingType
{
public:
    CSettingTypeSelectedDirectory(const char * Name, SettingID InitialDir, SettingID SelectedDir, SettingID UseSelected, SettingID NotifyChangeId);
    ~CSettingTypeSelectedDirectory();

    virtual bool IndexBasedSetting(void) const { return false; }
    virtual SettingType GetSettingType(void) const { return SettingType_SelectedDirectory; }
    virtual bool IsSettingSet(void) const;

    const char * GetName(void) const { return m_Name.c_str(); }

    // Return the values
    virtual bool Load(uint32_t Index, bool & Value) const;
    virtual bool Load(uint32_t Index, uint32_t & Value) const;
    virtual bool Load(uint32_t Index, std::string & Value) const;

    // Return the default values
    virtual void LoadDefault(uint32_t Index, bool & Value) const;
    virtual void LoadDefault(uint32_t Index, uint32_t & Value) const;
    virtual void LoadDefault(uint32_t Index, std::string & Value) const;

    // Update the settings
    virtual void Save(uint32_t Index, bool Value);
    virtual void Save(uint32_t Index, uint32_t Value);
    virtual void Save(uint32_t Index, const std::string & Value);
    virtual void Save(uint32_t Index, const char * Value);

    // Delete the setting
    virtual void Delete(uint32_t Index);

private:
    CSettingTypeSelectedDirectory(void);
    CSettingTypeSelectedDirectory(const CSettingTypeSelectedDirectory&);
    CSettingTypeSelectedDirectory& operator=(const CSettingTypeSelectedDirectory&);

    static void DirectoryChanged(CSettingTypeSelectedDirectory * _this);

    std::string m_Name;
    SettingID m_InitialDir;
    SettingID m_SelectedDir;
    SettingID m_UseSelected;
    SettingID m_NotifyChangeId;
};
