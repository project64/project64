#pragma once

class CSettingTypeRDBSaveChip :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRDBSaveChip(const char * Name, SettingID DefaultSetting );
    CSettingTypeRDBSaveChip(const char * Name, uint32_t DefaultValue );
    ~CSettingTypeRDBSaveChip();

    //return the values
    virtual bool Load (uint32_t Index, bool & Value ) const;
    virtual bool Load (uint32_t Index, uint32_t & Value ) const;
    virtual bool Load (uint32_t Index, std::string & Value ) const;

    //return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value ) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value ) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value ) const;

    //Update the settings
    virtual void Save (uint32_t Index, bool Value );
    virtual void Save (uint32_t Index, uint32_t Value );
    virtual void Save (uint32_t Index, const std::string & Value );
    virtual void Save (uint32_t Index, const char * Value );

    // Delete the setting
    virtual void Delete (uint32_t Index );

private:
    CSettingTypeRDBSaveChip(void);                                        // Disable default constructor
    CSettingTypeRDBSaveChip(const CSettingTypeRDBSaveChip&);              // Disable copy constructor
    CSettingTypeRDBSaveChip& operator=(const CSettingTypeRDBSaveChip&);   // Disable assignment
};
