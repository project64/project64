#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-Base.h>
#include <Project64-core/Settings/SettingsID.h>
#include <map>

enum SettingDataType
{
    Data_DWORD = 0,
    Data_String = 1,
    Data_CPUTYPE = 2,
    Data_SelfMod = 3,
    Data_OnOff = 4,
    Data_YesNo = 5,
    Data_SaveChip = 6
};

class CSettings
{
public:
    typedef void(*SettingChangedFunc)(void *);

    CSettings(void);
    ~CSettings(void);

    bool Initialize(const char * BaseDirectory, const char * AppName);

    //return the values
    bool LoadBool(SettingID Type);
    bool LoadBool(SettingID Type, bool & Value);
    bool LoadBoolIndex(SettingID Type, uint32_t index);
    bool LoadBoolIndex(SettingID Type, uint32_t index, bool & Value);
    uint32_t LoadDword(SettingID Type);
    bool LoadDword(SettingID Type, uint32_t & Value);
    uint32_t LoadDwordIndex(SettingID Type, uint32_t index);
    bool LoadDwordIndex(SettingID Type, uint32_t index, uint32_t & Value);
    std::string LoadStringVal(SettingID Type);
    bool LoadStringVal(SettingID Type, std::string & Value);
    bool LoadStringVal(SettingID Type, char * Buffer, uint32_t BufferSize);
    std::string LoadStringIndex(SettingID Type, uint32_t index);
    bool LoadStringIndex(SettingID Type, uint32_t index, std::string & Value);
    bool LoadStringIndex(SettingID Type, uint32_t index, char * Buffer, uint32_t BufferSize);

    //Load the default value for the setting
    bool LoadDefaultBool(SettingID Type);
    void LoadDefaultBool(SettingID Type, bool & Value);
    bool LoadDefaultBoolIndex(SettingID Type, uint32_t index);
    void LoadDefaultBoolIndex(SettingID Type, uint32_t index, bool & Value);
    uint32_t LoadDefaultDword(SettingID Type);
    void LoadDefaultDword(SettingID Type, uint32_t & Value);
    uint32_t LoadDefaultDwordIndex(SettingID Type, uint32_t index);
    void LoadDefaultDwordIndex(SettingID Type, uint32_t index, uint32_t & Value);
    std::string LoadDefaultString(SettingID Type);
    void LoadDefaultString(SettingID Type, std::string & Value);
    void LoadDefaultString(SettingID Type, char * Buffer, uint32_t BufferSize);
    std::string LoadDefaultStringIndex(SettingID Type, uint32_t index);
    void LoadDefaultStringIndex(SettingID Type, uint32_t index, std::string & Value);
    void LoadDefaultStringIndex(SettingID Type, uint32_t index, char * Buffer, uint32_t BufferSize);

    //Update the settings
    void SaveBool(SettingID Type, bool Value);
    void SaveBoolIndex(SettingID Type, uint32_t index, bool Value);
    void SaveDword(SettingID Type, uint32_t Value);
    void SaveDwordIndex(SettingID Type, uint32_t index, uint32_t Value);
    void SaveString(SettingID Type, const std::string & Value);
    void SaveStringIndex(SettingID Type, uint32_t index, const std::string & Value);
    void SaveString(SettingID Type, const char * Buffer);
    void SaveStringIndex(SettingID Type, uint32_t index, const char * Buffer);

    // Delete a setting
    void DeleteSetting(SettingID Type);
    void DeleteSettingIndex(SettingID Type, uint32_t index);

    //Register Notification of change
    void RegisterChangeCB(SettingID Type, void * Data, SettingChangedFunc Func);
    void UnregisterChangeCB(SettingID Type, void * Data, SettingChangedFunc Func);

    // information about setting
    SettingType GetSettingType(SettingID Type);
    bool IndexBasedSetting(SettingID Type);
    void SettingTypeChanged(SettingType Type);
    bool IsSettingSet(SettingID Type);

    // static functions for plugins
    static uint32_t  GetSetting(CSettings * _this, SettingID Type);
    static const char * GetSettingSz(CSettings * _this, SettingID Type, char * Buffer, uint32_t BufferSize);
    static void SetSetting(CSettings * _this, SettingID ID, uint32_t Value);
    static void SetSettingSz(CSettings * _this, SettingID ID, const char * Value);
    static void RegisterSetting(CSettings * _this, SettingID ID, SettingID DefaultID, SettingDataType DataType,
        SettingType Type, const char * Category, const char * DefaultStr, uint32_t Value);
    static uint32_t FindSetting(CSettings * _this, const char * Name);
    static void FlushSettings(CSettings * _this);
    static void sRegisterChangeCB(CSettings * _this, SettingID Type, void * Data, SettingChangedFunc Func);
    static void sUnregisterChangeCB(CSettings * _this, SettingID Type, void * Data, SettingChangedFunc Func);

    //Notification
    void NotifyCallBacks(SettingID Type);
    void AddHandler(SettingID TypeID, CSettingType * Handler);

private:
    void AddHowToHandleSetting(const char * BaseDirectory);
    void UnknownSetting(SettingID Type);

    struct SETTING_CHANGED_CB
    {
        void * Data;
        SettingChangedFunc Func;
        SETTING_CHANGED_CB * Next;
    };

    typedef std::map<SettingID, SETTING_CHANGED_CB *> SETTING_CALLBACK;
    typedef std::map<SettingID, CSettingType *> SETTING_MAP;
    typedef SETTING_MAP::iterator SETTING_HANDLER;

    SETTING_MAP m_SettingInfo;
    SETTING_CALLBACK m_Callback;
    uint32_t m_NextAutoSettingId;
};

extern CSettings * g_Settings;
