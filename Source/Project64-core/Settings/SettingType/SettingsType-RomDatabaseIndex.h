#pragma once

class CSettingTypeRomDatabaseIndex :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, const char * DefaultValue );
    CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, bool DefaultValue );
    CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, uint32_t DefaultValue );
    CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, SettingID DefaultSetting );

    virtual ~CSettingTypeRomDatabaseIndex();

    virtual bool IndexBasedSetting ( void ) const { return true; }

    //return the values
    virtual bool Load (uint32_t Index, bool & Value) const;
    virtual bool Load (uint32_t Index, uint32_t & Value) const;
    virtual bool Load (uint32_t Index, std::string & Value) const;

    //return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value) const;

    //Update the settings
    virtual void Save (uint32_t Index, bool Value);
    virtual void Save (uint32_t Index, uint32_t Value);
    virtual void Save (uint32_t Index, const std::string & Value);
    virtual void Save  (uint32_t Index, const char * Value);

    // Delete the setting
    virtual void Delete (uint32_t Index );

private:
    CSettingTypeRomDatabaseIndex(void);
    CSettingTypeRomDatabaseIndex(const CSettingTypeRomDatabaseIndex&);
    CSettingTypeRomDatabaseIndex& operator=(const CSettingTypeRomDatabaseIndex&);

    std::string m_PreIndex, m_PostIndex;
};
