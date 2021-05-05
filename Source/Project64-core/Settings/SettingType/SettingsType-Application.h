#pragma once

#include <Common/IniFile.h>
#include <Project64-core/Settings/SettingType/SettingsType-Base.h>

class CSettingTypeApplication :
    public CSettingType
{
public:
    CSettingTypeApplication(const char * Section, const char * Name, const char * DefaultValue);
    CSettingTypeApplication(const char * Section, const char * Name, bool DefaultValue);
    CSettingTypeApplication(const char * Section, const char * Name, uint32_t DefaultValue);
    CSettingTypeApplication(const char * Section, const char * Name, SettingID DefaultSetting);
    virtual ~CSettingTypeApplication();

    virtual bool IndexBasedSetting(void) const { return false; }
    virtual SettingType GetSettingType(void) const { return SettingType_CfgFile; }
    virtual bool IsSettingSet(void) const;

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

    // Initialize this class to use the INI file or registry
    static void Initialize(void);
    static void CleanUp(void);
    static void Flush(void);
    static void ResetAll(void);

    const char * GetKeyName(void) const { return m_KeyName.c_str(); }

protected:
    const char * m_DefaultStr;
    const uint32_t m_DefaultValue;
    const SettingID m_DefaultSetting;

    std::string FixSectionName(const char * Section);

    static CIniFile * m_SettingsIniFile;
    const std::string m_Section;
    const std::string m_KeyName;
    mutable std::string m_KeyNameIdex;

    virtual const char * SectionName(void) const;

private:
    CSettingTypeApplication(const CSettingTypeApplication&);
    CSettingTypeApplication& operator=(const CSettingTypeApplication&);
};
