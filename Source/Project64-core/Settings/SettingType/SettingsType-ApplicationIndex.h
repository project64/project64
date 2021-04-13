#pragma once

class CSettingTypeApplicationIndex :
    public CSettingTypeApplication
{
public:
    CSettingTypeApplicationIndex(const char * Section, const char * Name, const char * DefaultValue);
    CSettingTypeApplicationIndex(const char * Section, const char * Name, bool DefaultValue);
    CSettingTypeApplicationIndex(const char * Section, const char * Name, uint32_t DefaultValue);
    CSettingTypeApplicationIndex(const char * Section, const char * Name, SettingID DefaultSetting);
    ~CSettingTypeApplicationIndex();

    virtual bool IndexBasedSetting(void) const { return true; }

    //return the values
    virtual bool Load(uint32_t Index, bool & Value) const;
    virtual bool Load(uint32_t Index, uint32_t & Value) const;
    virtual bool Load(uint32_t Index, std::string & Value) const;

    //return the default values
    virtual void LoadDefault(uint32_t Index, bool & Value) const;
    virtual void LoadDefault(uint32_t Index, uint32_t & Value) const;
    virtual void LoadDefault(uint32_t Index, std::string & Value) const;

    //Update the settings
    virtual void Save(uint32_t Index, bool Value);
    virtual void Save(uint32_t Index, uint32_t Value);
    virtual void Save(uint32_t Index, const std::string & Value);
    virtual void Save(uint32_t Index, const char * Value);

    // Delete the setting
    virtual void Delete(uint32_t Index);

private:
    CSettingTypeApplicationIndex(void);
    CSettingTypeApplicationIndex(const CSettingTypeApplicationIndex&);
    CSettingTypeApplicationIndex& operator=(const CSettingTypeApplicationIndex&);
};
