#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-Base.h>

class CSettingTypeTempBool :
    public CSettingType
{
public:
    CSettingTypeTempBool(bool initialValue, const char * name = nullptr);
    ~CSettingTypeTempBool();

    bool IndexBasedSetting(void) const { return false; }
    SettingType GetSettingType(void) const { return SettingType_BoolVariable; }
    bool IsSettingSet(void) const { return m_changed; }

    const char * GetName(void) const { return m_Name.c_str(); }

    // Return the values
    bool Load(uint32_t Index, bool & Value) const;
    bool Load(uint32_t Index, uint32_t & Value) const;
    bool Load(uint32_t Index, std::string & Value) const;

    // Return the default values
    void LoadDefault(uint32_t Index, bool & Value) const;
    void LoadDefault(uint32_t Index, uint32_t & Value) const;
    void LoadDefault(uint32_t Index, std::string & Value) const;

    // Update the settings
    void Save(uint32_t Index, bool Value);
    void Save(uint32_t Index, uint32_t Value);
    void Save(uint32_t Index, const std::string & Value);
    void Save(uint32_t Index, const char * Value);

    // Delete the setting
    void Delete(uint32_t Index);

private:
    CSettingTypeTempBool(void);
    CSettingTypeTempBool(const CSettingTypeTempBool&);
    CSettingTypeTempBool& operator=(const CSettingTypeTempBool&);

    bool m_value;
    bool m_changed;
    std::string m_Name;
};
