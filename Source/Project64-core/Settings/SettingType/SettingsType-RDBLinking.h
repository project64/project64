#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-RomDatabase.h>
#include <cstdint>
#include <string>

class CSettingTypeRDBLinking :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRDBLinking(const char * Name, uint32_t DefaultValue);
    ~CSettingTypeRDBLinking();

    virtual bool Load(uint32_t Index, bool & Value) const;
    virtual bool Load(uint32_t Index, uint32_t & Value) const;
    virtual bool Load(uint32_t Index, std::string & Value) const;

    virtual void LoadDefault(uint32_t Index, bool & Value) const;
    virtual void LoadDefault(uint32_t Index, uint32_t & Value) const;
    virtual void LoadDefault(uint32_t Index, std::string & Value) const;

    virtual void Save(uint32_t Index, bool Value);
    virtual void Save(uint32_t Index, uint32_t Value);
    virtual void Save(uint32_t Index, const std::string & Value);
    virtual void Save(uint32_t Index, const char * Value);

    virtual void Delete(uint32_t Index);

private:
    CSettingTypeRDBLinking(void);
    CSettingTypeRDBLinking(const CSettingTypeRDBLinking &);
    CSettingTypeRDBLinking & operator=(const CSettingTypeRDBLinking &);
};
