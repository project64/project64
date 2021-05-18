#pragma once

class CSettingTypeRomDatabase :
    public CSettingType
{
public:
    CSettingTypeRomDatabase(const char * Name, const char * DefaultValue);
    CSettingTypeRomDatabase(const char * Name, bool DefaultValue);
    CSettingTypeRomDatabase(const char * Name, int32_t DefaultValue);
    CSettingTypeRomDatabase(const char * Name, SettingID DefaultSetting);
    ~CSettingTypeRomDatabase();

    virtual SettingLocation GetSettingsLocation ( void ) const { return SettingLocation_RomDatabase; }

    // Return the values
    virtual bool Load (int32_t Index, bool & Value) const;
    virtual bool Load (int32_t Index, uint32_t & Value) const;
    virtual bool Load (int32_t Index, stdstr & Value) const;

    // Update the settings
    virtual void Save (int32_t Index, bool Value);
    virtual void Save (int32_t Index, uint32_t Value);
    virtual void Save (int32_t Index, const stdstr & Value);
    virtual void Save (int32_t Index, const char * Value);

    static void Initilize ( void );

private:
    CSettingTypeRomDatabase(void);
    CSettingTypeRomDatabase(const CSettingTypeRomDatabase&);
    CSettingTypeRomDatabase& operator=(const CSettingTypeRomDatabase&);

    const const char * m_KeyName;
    const const char * m_DefaultStr;
    const int32_t m_DefaultValue;
    const SettingID m_DefaultSetting;

    static CIniFile * m_SettingsIniFile;
    stdstr m_SectionIdent;
};
