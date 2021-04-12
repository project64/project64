#pragma once

#include <Project64-core/Settings.h>
#include <Common/StdString.h>
#include <stdint.h>

enum SettingType
{
    SettingType_Unknown = -1,
    SettingType_ConstString = 0,
    SettingType_ConstValue = 1,
    SettingType_CfgFile = 2,
    SettingType_Registry = 3,
    SettingType_RelativePath = 4,
    TemporarySetting = 5,
    SettingType_RomDatabase = 6,
    SettingType_CheatSetting = 7,
    SettingType_GameSetting = 8,
    SettingType_BoolVariable = 9,
    SettingType_NumberVariable = 10,
    SettingType_StringVariable = 11,
    SettingType_SelectedDirectory = 12,
	SettingType_RdbSetting = 13,
	SettingType_Enhancement = 14,
};

class CSettingType
{
public:
    virtual ~CSettingType() {};

    virtual SettingType GetSettingType(void) const = 0;
    virtual bool IndexBasedSetting(void) const = 0;
    virtual bool IsSettingSet(void) const = 0;

    //return the values
    virtual bool Load(uint32_t Index, bool & Value) const = 0;
    virtual bool Load(uint32_t Index, uint32_t & Value) const = 0;
    virtual bool Load(uint32_t Index, std::string & Value) const = 0;

    //return the default values
    virtual void LoadDefault(uint32_t Index, bool & Value) const = 0;
    virtual void LoadDefault(uint32_t Index, uint32_t & Value) const = 0;
    virtual void LoadDefault(uint32_t Index, std::string & Value) const = 0;

    //Update the settings
    virtual void Save(uint32_t Index, bool Value) = 0;
    virtual void Save(uint32_t Index, uint32_t Value) = 0;
    virtual void Save(uint32_t Index, const std::string & Value) = 0;
    virtual void Save(uint32_t Index, const char * Value) = 0;

    // Delete the setting
    virtual void Delete(uint32_t Index) = 0;
};
