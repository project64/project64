#pragma once

#include <Common/IniFile.h>
#include <Project64-core/Settings/SettingType/SettingsType-Base.h>

class CSettingTypeRomDatabase :
    public CSettingType
{
public:
    CSettingTypeRomDatabase(const char * Name, const char * DefaultValue, bool DeleteOnDefault = false);
    CSettingTypeRomDatabase(const char * Name, bool DefaultValue, bool DeleteOnDefault = false);
    CSettingTypeRomDatabase(const char * Name, uint32_t DefaultValue, bool DeleteOnDefault = false);
    CSettingTypeRomDatabase(const char * Name, SettingID DefaultSetting, bool DeleteOnDefault = false);

    virtual ~CSettingTypeRomDatabase();

    virtual bool IndexBasedSetting(void) const
    {
        return false;
    }
    virtual SettingType GetSettingType(void) const
    {
        return SettingType_RomDatabase;
    }
    virtual bool IsSettingSet(void) const
    {
        return false;
    }

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

    static void Initialize(void);
    static void CleanUp(void);

protected:
    static void GameChanged(void * /*Data */);
    static void BaseDirChanged(void * /*Data */);

    static bool IsVideoSetting(const char * Name);
    static bool IsAudioSetting(const char * Name);
    static const char * StripNameSection(const char * Name);
    virtual const char * Section(void) const
    {
        return m_SectionIdent->c_str();
    }

    mutable std::string m_KeyName;
    const char * const m_DefaultStr;
    const int32_t m_DefaultValue;
    const SettingID m_DefaultSetting;
    const bool m_DeleteOnDefault;
    bool m_VideoSetting;
    bool m_AudioSetting;

    static std::string * m_SectionIdent;
    static CIniFile * m_SettingsIniFile;
    static CIniFile * m_VideoIniFile;
    static CIniFile * m_AudioIniFile;

private:
    CSettingTypeRomDatabase();
    CSettingTypeRomDatabase(const CSettingTypeRomDatabase &);
    CSettingTypeRomDatabase & operator=(const CSettingTypeRomDatabase &);

    bool Load(uint32_t & Value) const;
};
