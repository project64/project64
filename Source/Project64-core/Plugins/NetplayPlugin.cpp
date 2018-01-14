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
#include "stdafx.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64RomClass.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/CheatClass.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/Plugins/NetplayPlugin.h>
#include <Project64-core/RomList/RomList.h>
#ifdef _WIN32
#include <Windows.h>
#endif

CNetplay_Plugin::CNetplay_Plugin() :
SyncCheats(NULL),
SyncEvents(NULL),
SyncInputs(NULL),
SyncSettings(NULL)
{
}

CNetplay_Plugin::~CNetplay_Plugin()
{
    Close(NULL);
    UnloadPlugin();
}

bool CNetplay_Plugin::LoadFunctions(void)
{
    //Find entries for functions in DLL
    void(CALL *InitiateNetplay)(void);
    LoadFunction(InitiateNetplay);

    LoadFunction(SyncCheats);
    LoadFunction(SyncEvents);
    LoadFunction(SyncInputs);
    LoadFunction(SyncSettings);

    //Make sure dll has all needed functions
    if (InitiateNetplay == NULL) { UnloadPlugin(); return false; }
    if (SyncInputs == NULL) { UnloadPlugin(); return false; }

    if (m_PluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == NULL) { UnloadPlugin(); return false; }
    }

    return true;
}

bool CNetplay_Plugin::Initiate(CN64System * System, RenderWindow * Window)
{
    //Get Function from DLL
    int32_t(CALL *InitiateNetplay)(NETPLAY_INFO Netplay_Info, NETPLAY_SUPPORT * Netplay_Support);
    LoadFunction(InitiateNetplay);
    if (InitiateNetplay == NULL)
    {
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Failed to find InitiateNetplay");
        return false;
    }

    NETPLAY_SUPPORT Support = { 0 };
    NETPLAY_INFO Info = { 0 };
    SetNetplayInfo(&Info);

#ifdef _WIN32
    Info.hwnd = Window ? Window->GetWindowHandle() : NULL;
#else
    Info.hwnd = NULL;
#endif
    m_Initialized = InitiateNetplay(Info, &Support) != 0;
    SetNetplaySupport(&Support);
    m_System = System;

    memset(&m_Buttons, 0, sizeof(m_Buttons));
    memset(&m_Commands, 0, sizeof(m_Commands));
    memset(&m_Controls, 0, sizeof(m_Controls));

    return m_Initialized;
}

bool CNetplay_Plugin::OpenNetplay(void* hParent)
{
    //Get Function from DLL
    void(CALL *OpenNetplay)(void);
    LoadFunction(OpenNetplay);
    void(CALL *CloseNetplay)(void);
    LoadFunction(CloseNetplay);

    if (OpenNetplay == NULL) { UnloadPlugin(); return false; }
    if (CloseNetplay == NULL) { UnloadPlugin(); return false; }

    m_hParent = hParent;

#ifdef _WIN32
    if (m_hParent != NULL)
    {
        if (m_hNetplayThread)
        {
            WriteTrace(TraceNetplayPlugin, TraceDebug, "Terminate Netplay Thread");
            TerminateThread(m_hNetplayThread, 0);
        }
        DWORD ThreadID;
        m_hNetplayThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) NetplayThread, (LPVOID) this, 0, &ThreadID);
    }
#endif

    if (m_hNetplayThread == NULL) { return false; }
    return true;
}

bool CNetplay_Plugin::CloseNetplay(void)
{
    //Get Function from DLL
    int32_t(CALL *CloseNetplay)(void);
    LoadFunction(CloseNetplay);
    if (CloseNetplay == NULL)
    {
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Failed to find CloseNetplay");
        return false;
    }

    bool Opened = CloseNetplay();
    g_Settings->SaveBool(Plugin_NET_Loaded, !Opened);

#ifdef _WIN32
    if (m_hNetplayThread)
    {
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Terminate Netplay Thread");
        TerminateThread(m_hNetplayThread, 0);
        m_hNetplayThread = NULL;
    }
#endif

    return true;
}

void CNetplay_Plugin::SetNetplayInfo(NETPLAY_INFO * Netplay_Info)
{
    WriteTrace(TraceNetplayPlugin, TraceDebug, "SetNetplayInfo");

    LoadRomList();
    int index;
    int RomLimit = m_RomInfo.size() > ROM_LIMIT ? ROM_LIMIT : (int) m_RomInfo.size();

    for (index = 0; index < RomLimit; index++)
        Netplay_Info->romInfo[index] = &m_RomInfo[index];

    Netplay_Info->appName = VER_FILE_VERSION_STR;
    Netplay_Info->chatCallback = NetplayChatCallback;
    Netplay_Info->cheatCallback = NetplayCheatCallback;
    Netplay_Info->eventCallback = NetplayEventCallback;
    Netplay_Info->inputCallback = NetplayInputCallback;
    Netplay_Info->settingCallback = NetplaySettingCallback;
}

void CNetplay_Plugin::ForwardCheats(void)
{
    WriteTrace(TraceNetplayPlugin, TraceDebug, "Forwarding Cheat Codes");

    if (m_CheatsLocked)
    {
        m_ForwardCheats = true;
        return;
    }
    else
    {
        m_CheatsLocked = true;
        m_ForwardCheats = false;
    }

    NETPLAY_CHEAT Netplay_Reset_Cheat = { 0 };
    Netplay_Reset_Cheat.PlayerId = m_PlayerId;
    Netplay_Reset_Cheat.Action = Cheat_Action_Reset;
    if (SyncCheats) { SyncCheats(&Netplay_Reset_Cheat); }

    for (size_t CurrentCheat = 0; CurrentCheat < m_CodesLocal.size(); CurrentCheat++)
    {
        const CCheats::CODES & CodeEntry = m_CodesLocal[CurrentCheat];
        for (size_t CurrentEntry = 0; CurrentEntry < CodeEntry.size(); CurrentEntry++)
        {
            NETPLAY_CHEAT Netplay_Cheat = { 0 };
            Netplay_Cheat.PlayerId = m_PlayerId;

            // cheats with the same index will be appended to the CodeEntry
            Netplay_Cheat.Action = Cheat_Action_Load;
            Netplay_Cheat.Index = CurrentCheat;
            Netplay_Cheat.Command = CodeEntry[CurrentEntry].Command;
            Netplay_Cheat.Value = CodeEntry[CurrentEntry].Value;
            if (SyncCheats) { SyncCheats(&Netplay_Cheat); }
        }
    }

    NETPLAY_CHEAT Netplay_Apply_Cheat = { 0 };
    Netplay_Apply_Cheat.PlayerId = m_PlayerId;
    Netplay_Apply_Cheat.Action = Cheat_Action_Apply;
    if (SyncCheats) { SyncCheats(&Netplay_Apply_Cheat); }

    m_CheatsLocked = false;
    if (m_ForwardCheats) { ForwardCheats(); }
}

void CNetplay_Plugin::ForwardEvents(NETPLAY_EVENT Netplay_Event)
{
    SyncEvents(&Netplay_Event);
}

void CNetplay_Plugin::ForwardInputs(void)
{
    NETPLAY_INPUT Netplay_Input = { 0 };
    Netplay_Input.PlayerId = m_PlayerId;

    BUTTONS Keys;
    uint8_t PifRam[8];
    int32_t Command;
    CONTROL * Controllers;
    memset(&Keys, 0, sizeof(Keys));

    if (g_Plugins->Control()->PluginControllers())
        Controllers = g_Plugins->Control()->PluginControllers();

    for (int Control = 0; Control < CONTROL_LIMIT; Control++)
    {
        memset(&Keys, 0, sizeof(Keys));
        memset(&PifRam, 0, sizeof(PifRam));

        if (g_Plugins->Control()->GetKeys)
            g_Plugins->Control()->GetKeys(Control, &Keys);

        //TODO: Clean this up
        try
        {
            memset(&PifRam, 0xFF, sizeof(PifRam));
            PifRam[1] = 0x01; PifRam[2] = 0x04; PifRam[3] = 0x01;

            if (Controllers[Control].Present && Controllers[Control].RawData) {
                if (g_Plugins->Control()->ReadController)
                    g_Plugins->Control()->ReadController(Control, &PifRam[1]);
            }

            Command = (Command << 8) + PifRam[7];
            Command = (Command << 8) + PifRam[6];
            Command = (Command << 8) + PifRam[5];
            Command = (Command << 8) + PifRam[4];
        }
        catch (...)
        {
            WriteTrace(TraceNetplayPlugin, TraceDebug, "Unable to read controller data");
        }

        Netplay_Input.Buttons[Control].Value = Keys.Value;
        Netplay_Input.Command[Control] = Command;
        Netplay_Input.Control[Control] = Controllers[Control];
    }

    if (g_Plugins->Control()->ReadController)
        g_Plugins->Control()->ReadController(-1, NULL);

    if (SyncInputs)
        SyncInputs(&Netplay_Input);
}

void CNetplay_Plugin::ProcessInputs(NETPLAY_INPUT Netplay_Inputs) {
    for (int Control = 0; Control < CONTROL_LIMIT; Control++)
    {
        m_Buttons[Control] = Netplay_Inputs.Buttons[Control];
        m_Commands[Control] = Netplay_Inputs.Command[Control];
        m_Controls[Control] = Netplay_Inputs.Control[Control];
    }
}

void CNetplay_Plugin::ForwardSettings(void)
{
    std::vector<NETPLAY_SETTING_ITEM> Settings_Array;

    Settings_Array.push_back({ Setting_First, Setting_First });
    Settings_Array.push_back({ Setting_PlayerCount, (uint32_t)m_PlayerCount });
    Settings_Array.push_back({ Setting_RDRamSize, g_Settings->LoadDword(Game_RDRamSize) });
    Settings_Array.push_back({ Setting_CounterFactor, g_Settings->LoadDword(Game_CounterFactor) });
    Settings_Array.push_back({ Setting_UseTlb, g_Settings->LoadBool(Game_UseTlb) });
    Settings_Array.push_back({ Setting_DelayDP, g_Settings->LoadBool(Game_DelayDP) });
    Settings_Array.push_back({ Setting_DelaySI, g_Settings->LoadBool(Game_DelaySI) });
    Settings_Array.push_back({ Setting_32Bit, g_Settings->LoadBool(Game_32Bit) });
    Settings_Array.push_back({ Setting_FixedAudio, g_Settings->LoadBool(Game_FixedAudio) });
    Settings_Array.push_back({ Setting_SyncViaAudio, g_Settings->LoadBool(Game_SyncViaAudio) });
    Settings_Array.push_back({ Setting_RspAudioSignal, g_Settings->LoadBool(Game_RspAudioSignal) });
    Settings_Array.push_back({ Setting_ViRefreshRate, g_Settings->LoadDword(Game_ViRefreshRate) });
    Settings_Array.push_back({ Setting_AiCountPerBytes, g_Settings->LoadDword(Game_AiCountPerBytes) });
    Settings_Array.push_back({ Setting_OverClockModifier, g_Settings->LoadDword(Game_OverClockModifier) });
    Settings_Array.push_back({ Setting_FullSpeed, g_Settings->LoadBool(Game_FullSpeed) });
    Settings_Array.push_back({ Setting_Randomizer_Seed, (uint32_t)time(NULL) });
    Settings_Array.push_back({ Setting_Last, Setting_Last });

    for (size_t CurrentSetting = 0; CurrentSetting < Settings_Array.size(); CurrentSetting++)
    {
        NETPLAY_SETTING Netplay_Setting = { 0 };
        Netplay_Setting.PlayerId = m_PlayerId;
        Netplay_Setting.Setting = Settings_Array[CurrentSetting].Setting;
        Netplay_Setting.Value = Settings_Array[CurrentSetting].Value;
        if (SyncSettings) { SyncSettings(&Netplay_Setting); }
    }
}

void CNetplay_Plugin::GetKeys(int32_t Control, BUTTONS * Keys)
{
    *Keys = m_Buttons[Control];
}

void CNetplay_Plugin::ReadController(int32_t Control, uint8_t * Command)
{
    //TODO: Clean this up
    int x = 6;
    for (int i = 3; i >= 0; i--)
    {
        uint8_t byte = (m_Commands[Control] >> 8 * i) & 0xFF;
        Command[x] = byte;
        x--;
    }
}

void CNetplay_Plugin::InitializeCodes()
{
    m_CheatsLocked = false;
    m_ForwardCheats = false;

    m_CodesLocal.clear();
    m_Codes.clear();
    m_CodesFinal.clear();
}

void CNetplay_Plugin::LoadCode(CCheats::CODES Code)
{
    m_CodesLocal.push_back(Code);
}

void CNetplay_Plugin::ResetCodes()
{
    m_CodesLocal.clear();
}

void CNetplay_Plugin::ProcessCheats(NETPLAY_CHEAT Netplay_Cheat) {
    CCheats::GAMESHARK_CODE CodeEntry;
    CCheats::CODES Code;

    switch(Netplay_Cheat.Action)
    {
    case Cheat_Action_Reset:
    {
        m_Codes.clear();
        break;
    }

    case Cheat_Action_Load:
    {
        size_t CodesSize = m_Codes.size();

        CodeEntry.Command = Netplay_Cheat.Command;
        CodeEntry.Value = Netplay_Cheat.Value;

        if (CodesSize == 0 || Netplay_Cheat.Index + 1 > CodesSize)
        {
            Code.push_back(CodeEntry);
            m_Codes.push_back(Code);
        }
        else
        {
            m_Codes[Netplay_Cheat.Index].push_back(CodeEntry);
        }
        break;
    }

    case Cheat_Action_Confirm:
    {
        //TODO: Add confirmation that all the cheats have been distributed
        break;
    }

    case Cheat_Action_Apply:
    {
        m_CodesFinal.clear();
        m_CodesFinal = m_Codes;
        break;
    }

    default:
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Unknown cheat action: %i", Netplay_Cheat.Action);
        break;
    }
}

void CNetplay_Plugin::SetNetplaySupport(NETPLAY_SUPPORT * Netplay_Support)
{
    WriteTrace(TraceNetplayPlugin, TraceDebug, "SetNetplaySupport");
    g_Settings->SaveBool(Plugin_NET_CanPause, Netplay_Support->canPause);
    g_Settings->SaveBool(Plugin_NET_CanReset, Netplay_Support->canReset);
    g_Settings->SaveBool(Plugin_NET_CanSave, Netplay_Support->canSave);
    g_Settings->SaveBool(Plugin_NET_CanSaveState, Netplay_Support->canSaveState);
    g_Settings->SaveBool(Plugin_NET_CanCheat, Netplay_Support->canCheat);
    g_Settings->SaveBool(Plugin_NET_CanDebug, Netplay_Support->canDebug);
}

void CNetplay_Plugin::UnloadPluginDetails(void)
{
    SyncCheats = NULL;
    SyncEvents = NULL;
    SyncInputs = NULL;
    SyncSettings = NULL;
}

#ifdef _WIN32
void CNetplay_Plugin::NetplayThread(CNetplay_Plugin * _this)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    //Get Function from DLL
    int32_t(CALL *OpenNetplay)(NETPLAY_INFO Netplay_Info);
    _this->LoadFunction(OpenNetplay);
    if (OpenNetplay == NULL)
    {
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Failed to find OpenNetplay");
        return;
    }

    NETPLAY_INFO Info = { 0 };
    _this->SetNetplayInfo(&Info);
    Info.hwnd = _this->m_hParent;

    g_Settings->SaveBool(Plugin_NET_Loaded, true);
    OpenNetplay(Info);
    g_Settings->SaveBool(Plugin_NET_Loaded, false);
}
#endif

int CALL NetplayChatCallback(NETPLAY_CHAT Netplay_Chat)
{
    WriteTrace(TraceNetplayPlugin, TraceDebug, "<%s>: %s", Netplay_Chat.Name, Netplay_Chat.Message);
    g_Notify->DisplayMessage(0, stdstr_f("<%s>: %s", Netplay_Chat.Name, Netplay_Chat.Message).c_str());
    return 0;
}

int CALL NetplayCheatCallback(NETPLAY_CHEAT Netplay_Cheat)
{
    g_Plugins->Netplay()->ProcessCheats(Netplay_Cheat);
    return 0;
}

int CALL NetplayEventCallback(NETPLAY_EVENT Netplay_Event)
{
    bool AutoStart = g_Settings->LoadBool(Setting_AutoStart);
    NETPLAY_SUPPORT Support = { 0 };

    switch (Netplay_Event.Type)
    {
    case Event_Type_Load:
        g_Plugins->Netplay()->SetPlayerId(Netplay_Event.PlayerId);
        g_Plugins->Netplay()->SetPlayerCount(Netplay_Event.EventData);
        g_Plugins->Netplay()->SetPlayerRom(Netplay_Event.EventString);

        WriteTrace(TraceNetplayPlugin, TraceDebug, "[Load] Player %i of %i", Netplay_Event.PlayerId + 1, Netplay_Event.EventData);
        if (! g_BaseSystem->LoadFileImage(Netplay_Event.EventString))
            return -1;

        g_Plugins->Netplay()->InitializeCodes();
        g_Plugins->Netplay()->ForwardSettings();
        break;

    case Event_Type_Open:
        g_Plugins->Netplay()->SetPlayerId(Netplay_Event.PlayerId);
        g_Plugins->Netplay()->SetPlayerRom(Netplay_Event.EventString);

        WriteTrace(TraceNetplayPlugin, TraceDebug, "[Open] Player %i of %i", Netplay_Event.PlayerId + 1, g_Plugins->Netplay()->PlayerCount());
        g_Settings->SaveBool(Plugin_NET_Running, true);
        g_BaseSystem->RunFileImage(Netplay_Event.EventString, g_Plugins->Netplay()->GetRandomizerSeed(), true);
        break;

    case Event_Type_Close:
        g_Plugins->Netplay()->SetPlayerId(NULL);
        g_Plugins->Netplay()->SetPlayerCount(NULL);
        g_Plugins->Netplay()->SetPlayerRom(NULL);

        g_Settings->SaveBool(Plugin_NET_Running, false);
        if (g_BaseSystem)
            g_BaseSystem->CloseCpu();
        break;

    default:
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Unknown event type: %i", Netplay_Event.Type);
        break;
    }

    return 0;
}

int CALL NetplayInputCallback(NETPLAY_INPUT Netplay_Input)
{
    g_Plugins->Netplay()->ProcessInputs(Netplay_Input);
    return 0;
}

int CALL NetplaySettingCallback(NETPLAY_SETTING Netplay_Setting)
{
    switch (Netplay_Setting.Setting)
    {
    case Setting_First:
        break;

    case Setting_PlayerCount: g_Plugins->Netplay()->SetPlayerCount(Netplay_Setting.Value); break;
    case Setting_RDRamSize: g_Settings->SaveDword(Netplay_RDRamSize, Netplay_Setting.Value); break;
    case Setting_CounterFactor: g_Settings->SaveDword(Netplay_CounterFactor, Netplay_Setting.Value); break;
    case Setting_UseTlb: g_Settings->SaveBool(Netplay_UseTlb, (bool)Netplay_Setting.Value); break;
    case Setting_DelayDP: g_Settings->SaveBool(Netplay_DelayDP, (bool)Netplay_Setting.Value); break;
    case Setting_DelaySI: g_Settings->SaveBool(Netplay_DelaySI, (bool)Netplay_Setting.Value); break;
    case Setting_32Bit: g_Settings->SaveBool(Netplay_32Bit, (bool)Netplay_Setting.Value); break;
    case Setting_FixedAudio: g_Settings->SaveBool(Netplay_FixedAudio, (bool)Netplay_Setting.Value); break;
    case Setting_SyncViaAudio: g_Settings->SaveBool(Netplay_SyncViaAudio, (bool)Netplay_Setting.Value); break;
    case Setting_RspAudioSignal: g_Settings->SaveBool(Netplay_RspAudioSignal, (bool)Netplay_Setting.Value); break;
    case Setting_ViRefreshRate: g_Settings->SaveDword(Netplay_ViRefreshRate, Netplay_Setting.Value); break;
    case Setting_AiCountPerBytes: g_Settings->SaveDword(Netplay_AiCountPerBytes, Netplay_Setting.Value); break;
    case Setting_OverClockModifier: g_Settings->SaveDword(Netplay_OverClockModifier, Netplay_Setting.Value); break;
    case Setting_FullSpeed: g_Settings->SaveBool(Netplay_FullSpeed, (bool)Netplay_Setting.Value); break;
    case Setting_Randomizer_Seed: g_Plugins->Netplay()->SetRandomizerSeed(Netplay_Setting.Value); break;

    case Setting_Last:
    {
        NETPLAY_EVENT Netplay_Event = { 0 };
        Netplay_Event.PlayerId = g_Plugins->Netplay()->PlayerId();
        Netplay_Event.Type = Event_Type_Open;
        Netplay_Event.EventData = g_Plugins->Netplay()->PlayerCount();
        Netplay_Event.EventString = g_Plugins->Netplay()->PlayerRom();
        g_Plugins->Netplay()->ForwardEvents(Netplay_Event);
        break;
    }

    default:
    {
        WriteTrace(TraceNetplayPlugin, TraceDebug, "Unknown setting: %i", Netplay_Setting.Setting);
        break;
    }

    }
    return 0;
}
