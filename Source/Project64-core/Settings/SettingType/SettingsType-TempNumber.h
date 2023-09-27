#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-Base.h>

class CSettingTypeTempNumber :
    public CSettingType
{
public:
    CSettingTypeTempNumber(uint32_t initialValue, const char * Name = nullptr);
    ~CSettingTypeTempNumber();

    bool IndexBasedSetting(void) const
    {
        return false;
    }
    SettingType GetSettingType(void) const
    {
        return SettingType_NumberVariable;
    }
    bool IsSettingSet(void) const
    {
        return false;
    }
    const char * GetName(void) const
    {
        return m_Name.c_str();
    }

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
    CSettingTypeTempNumber(void);
    CSettingTypeTempNumber(const CSettingTypeTempNumber &);
    CSettingTypeTempNumber & operator=(const CSettingTypeTempNumber &);

    uint32_t m_Value;
    uint32_t m_InitialValue;
    std::string m_Name;
};
