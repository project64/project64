#pragma once

#include "SettingsType-RomDatabase.h"

class CSettingTypeRomDatabaseSetting :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, const char * DefaultValue, bool DeleteOnDefault = false );
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, bool DefaultValue, bool DeleteOnDefault = false );
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, uint32_t DefaultValue, bool DeleteOnDefault = false );
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, SettingID DefaultSetting, bool DeleteOnDefault = false );

    virtual ~CSettingTypeRomDatabaseSetting();

    virtual SettingType GetSettingType    ( void ) const { return SettingType_RdbSetting; }

private:
    virtual const char * Section ( void ) const { return m_SectionIdent.c_str(); }

    stdstr m_SectionIdent;

private:
    CSettingTypeRomDatabaseSetting(void);                                               // Disable default constructor
    CSettingTypeRomDatabaseSetting(const CSettingTypeRomDatabaseSetting&);              // Disable copy constructor
    CSettingTypeRomDatabaseSetting& operator=(const CSettingTypeRomDatabaseSetting&);   // Disable assignment
};
