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

#define CONTROL_LIMIT   4

enum NETPLAY_ID {
    // netplay types
    Datatype_Chat,
    Datatype_Cheat,
    Datatype_Event,
    Datatype_Input,
    Datatype_Setting,

    // cheat actions
    Cheat_Action_Reset,
    Cheat_Action_Load,
    Cheat_Action_Confirm,
    Cheat_Action_Apply,

    // event types
    Event_Type_Load,
    Event_Type_Open,
    Event_Type_Close,

    // settings
    Setting_First,
    Setting_PlayerCount,
    Setting_RDRamSize,
    Setting_CounterFactor,
    Setting_UseTlb,
    Setting_DelayDP,
    Setting_DelaySI,
    Setting_32Bit,
    Setting_FixedAudio,
    Setting_SyncViaAudio,
    Setting_RspAudioSignal,
    Setting_ViRefreshRate,
    Setting_AiCountPerBytes,
    Setting_OverClockModifier,
    Setting_FullSpeed,
    Setting_Randomizer_Seed,
    Setting_Last,
};

typedef struct
{
    int32_t PlayerId;
    char * Name;
    char * Message;
    time_t Timestamp;
} NETPLAY_CHAT;

typedef struct
{
    int32_t PlayerId;
    int8_t Action;
    uint16_t Index;
    uint32_t Command;
    uint16_t Value;
} NETPLAY_CHEAT;

typedef struct
{
    int32_t PlayerId;
    int8_t Type;
    int32_t EventData;
    char * EventString;
} NETPLAY_EVENT;

typedef struct
{
    int32_t PlayerId;
    CONTROL Control[CONTROL_LIMIT];
    BUTTONS Buttons[CONTROL_LIMIT];
    int32_t Command[CONTROL_LIMIT];
} NETPLAY_INPUT;

typedef struct
{
    int32_t PlayerId;
    int32_t Setting;
    uint32_t Value;
} NETPLAY_SETTING;

typedef struct
{
    int32_t Setting;
    uint32_t Value;
} NETPLAY_SETTING_ITEM;

typedef struct
{
    bool canPause;
    bool canReset;
    bool canSave;
    bool canSaveState;
    bool canCheat;
    bool canDebug;
} NETPLAY_SUPPORT;
