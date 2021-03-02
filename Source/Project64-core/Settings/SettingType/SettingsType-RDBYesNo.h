#pragma once

class CSettingTypeRDBYesNo :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRDBYesNo(const char * Name, SettingID DefaultSetting );
    CSettingTypeRDBYesNo(const char * Name, uint32_t DefaultValue );
    ~CSettingTypeRDBYesNo();

    //return the values
    virtual bool Load (uint32_t Index, bool & Value   ) const;
    virtual bool Load (uint32_t Index, uint32_t & Value  ) const;
    virtual bool Load (uint32_t Index, std::string & Value ) const;

    //return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value   ) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value  ) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value ) const;

    //Update the settings
    virtual void Save (uint32_t Index, bool Value );
    virtual void Save (uint32_t Index, uint32_t Value );
    virtual void Save (uint32_t Index, const std::string & Value );
    virtual void Save (uint32_t Index, const char * Value );

    // Delete the setting
    virtual void Delete (uint32_t Index );

private:
    CSettingTypeRDBYesNo(void);                                     // Disable default constructor
    CSettingTypeRDBYesNo(const CSettingTypeRDBYesNo&);              // Disable copy constructor
    CSettingTypeRDBYesNo& operator=(const CSettingTypeRDBYesNo&);   // Disable assignment
};
