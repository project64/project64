#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

    // Get Plugin Settings, take a setting id
    unsigned int GetSetting(short SettingID);
    const char * GetSettingSz(short SettingID, char * Buffer, int BufferLen);

    // Get System Settings, take a setting returned by FindSystemSettingId
    unsigned int GetSystemSetting(short SettingID);
    const char * GetSystemSettingSz(short SettingID, char * Buffer, int BufferLen);

    // Set a settings value
    void SetSetting(short SettingID, unsigned int Value);
    void SetSettingSz(short SettingID, const char * Value);

    // enum's
    enum SETTING_DATA_TYPE
    {
        Data_DWORD_General = 0, // A unsigned int setting used anywhere
        Data_String_General = 1, // A string setting used anywhere
        Data_DWORD_Game = 2, // A unsigned int associated with the current game
        Data_String_Game = 3, // A string associated with the current game
        Data_DWORD_RDB = 4, // A unsigned int associated with the current game in the rom database
        Data_String_RDB = 5, // A string associated with the current game in the rom database
        Data_DWORD_RDB_Setting = 6, // A unsigned int read from the rom database, with config file
        Data_String_RDB_Setting = 7, // A string read from the rom database, with config file
    };

    // set other information about different settings
    int SettingsInitilized(void);
    void SetModuleName(const char * Name);
    void RegisterSetting(short SettingID, SETTING_DATA_TYPE Type, const char * Name, const char * Category,
        unsigned int DefaultDW, const char * DefaultStr);
    void RegisterSetting2(short SettingID, SETTING_DATA_TYPE Type, const char * Name, const char * Category, short DefaultSettingID);
    short FindSystemSettingId(const char * Name);
    void FlushSettings(void);

    // this must be implemented to be notified when a setting is used but has not been set up
    void UseUnregisteredSetting(int SettingID);

    typedef void(*SettingChangedFunc)(void *);
    void SettingsRegisterChange(bool SystemSetting, int Type, void * Data, SettingChangedFunc Func);
    void SettingsUnregisterChange(bool SystemSetting, int Type, void * Data, SettingChangedFunc Func);

#if defined(__cplusplus)
}
#endif

class CNotification
{
public:
    static void DisplayError(const char * Message);
    static void FatalError(const char * Message);
    static void DisplayMessage(int DisplayTime, const char * Message);
    static void DisplayMessage2(const char * Message);
    static void BreakPoint(const char * FileName, int LineNumber);
};

extern CNotification * g_Notify;
