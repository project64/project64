/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#include <Project64-core/Plugins/PluginBase.h>
#include <Project64-core/Plugins/NetplayTypes.h>
#include <Project64-core/N64System/CheatClass.h>
#include <Project64-core/RomList/RomList.h>
#include <time.h>
#if defined(ANDROID)
#include <utime.h>
#endif

#define ROM_LIMIT       2048

class CNetplay_Plugin : public CPlugin, public CRomList
{
    typedef struct
    {
        void *      hwnd;
        char *      appName;
        ROM_INFO *  romInfo[ROM_LIMIT];

        int(CALL *chatCallback)(NETPLAY_CHAT Netplay_Chat);
        int(CALL *cheatCallback)(NETPLAY_CHEAT Netplay_Cheat);
        int(CALL *eventCallback)(NETPLAY_EVENT Netplay_Event);
        int(CALL *inputCallback)(NETPLAY_INPUT Netplay_Input);
        int(CALL *settingCallback)(NETPLAY_SETTING Netplay_Setting);
    } NETPLAY_INFO;
public:
    CNetplay_Plugin(void);
    ~CNetplay_Plugin();

    bool Initiate(CN64System * System, RenderWindow * Window);
    bool OpenNetplay(void * hParent);
    bool CloseNetplay(void);
    void SetNetplaySupport(NETPLAY_SUPPORT * Netplay_Support);

    void ForwardCheats(void);
    void ForwardEvents(NETPLAY_EVENT Netplay_Event);
    void ForwardInputs(void);
    void ForwardSettings(void);

    void ProcessCheats(NETPLAY_CHEAT Netplay_Cheats);
    void ProcessEvents(NETPLAY_EVENT Netplay_Events);
    void ProcessInputs(NETPLAY_INPUT Netplay_Inputs);
    void ProcessSettings(NETPLAY_SETTING Netplay_Settings);

    void(CALL *SyncCheats)(NETPLAY_CHEAT * Netplay_Cheats);
    void(CALL *SyncEvents)(NETPLAY_EVENT * Netplay_Events);
    void(CALL *SyncInputs)(NETPLAY_INPUT * Netplay_Inputs);
    void(CALL *SyncSettings)(NETPLAY_SETTING * Netplay_Settings);

    void GetKeys(int32_t Control, BUTTONS * Keys);
    inline CONTROL * PluginControllers(void) { return m_Controls; }
    void ReadController(int32_t Control, uint8_t * Command);

    void InitializeCodes();
    void LoadCode(CCheats::CODES Code);
    void ResetCodes(void);
    inline CCheats::CODES_ARRAY GetCodes(void) { return m_CodesFinal; };

    inline uint32_t GetRandomizerSeed() { return m_RandomizerSeed; }
    inline void SetRandomizerSeed(uint32_t seed) { m_RandomizerSeed = seed; }

    inline int32_t PlayerCount(void) { return m_PlayerCount; }
    inline void SetPlayerCount(int32_t playerCount) { m_PlayerCount = playerCount; }
    inline int32_t PlayerId(void) { return m_PlayerId; }
    inline void SetPlayerId(int32_t playerId) { m_PlayerId = playerId; }
    inline char * PlayerRom(void) { return m_PlayerRom; }
    inline void SetPlayerRom(char * playerRom) { m_PlayerRom = playerRom; }

private:
    CNetplay_Plugin(const CNetplay_Plugin&);			// Disable copy constructor
    CNetplay_Plugin& operator=(const CNetplay_Plugin&);	// Disable assignment

    virtual int32_t GetDefaultSettingStartRange() const { return FirstNetDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstNetSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_NETPLAY; }
    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    // Function used in a thread for opening netplay UI
    static void NetplayThread(CNetplay_Plugin * _this);

    CN64System * m_System;
    void * m_hNetplayThread;
    void * m_hParent;
    int32_t m_PlayerId;
    int32_t m_PlayerCount;
    char * m_PlayerRom;

    BUTTONS m_Buttons[CONTROL_LIMIT];
    int32_t m_Commands[CONTROL_LIMIT];
    CONTROL m_Controls[CONTROL_LIMIT];

    bool m_CheatsLocked = false;
    bool m_ForwardCheats = false;
    CCheats::CODES_ARRAY m_CodesLocal;
    CCheats::CODES_ARRAY m_Codes;
    CCheats::CODES_ARRAY m_CodesFinal;

    void SetNetplayInfo(NETPLAY_INFO * Netplay_Info);
    typedef std::vector<NETPLAY_SETTING> SETTINGS_ARRAY;
    SETTINGS_ARRAY m_SettingsArray;
    uint32_t m_RandomizerSeed = (uint32_t) time(NULL);
};

int CALL NetplayChatCallback(NETPLAY_CHAT Netplay_Chat);
int CALL NetplayCheatCallback(NETPLAY_CHEAT Netplay_Cheat);
int CALL NetplayEventCallback(NETPLAY_EVENT Netplay_Event);
int CALL NetplayInputCallback(NETPLAY_INPUT Netplay_Input);
int CALL NetplaySettingCallback(NETPLAY_SETTING Netplay_Setting);
