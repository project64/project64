/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/Settings/GameSettings.h>

bool CGameSettings::m_bSMM_StoreInstruc;
bool CGameSettings::m_bSMM_Protect;
bool CGameSettings::m_bSMM_ValidFunc;
bool CGameSettings::m_bSMM_PIDMA;
bool CGameSettings::m_bSMM_TLB;
bool CGameSettings::m_bUseTlb;
uint32_t CGameSettings::m_CountPerOp = 2;
uint32_t CGameSettings::m_ViRefreshRate = 1500;
uint32_t CGameSettings::m_AiCountPerBytes = 500;
bool CGameSettings::m_DelayDP = false;
bool CGameSettings::m_DelaySI = false;
uint32_t CGameSettings::m_RdramSize = 0;
bool CGameSettings::m_bFixedAudio = true;
bool CGameSettings::m_bSyncingToAudio = true;
bool CGameSettings::m_bSyncToAudio = true;
bool CGameSettings::m_bFastSP = true;
bool CGameSettings::m_b32Bit = true;
bool CGameSettings::m_RspAudioSignal;
bool CGameSettings::m_bRomInMemory;
bool CGameSettings::m_RegCaching;
bool CGameSettings::m_bLinkBlocks;
uint32_t CGameSettings::m_LookUpMode; //FUNC_LOOKUP_METHOD
SYSTEM_TYPE CGameSettings::m_SystemType = SYSTEM_NTSC;
CPU_TYPE CGameSettings::m_CpuType = CPU_Recompiler;
uint32_t CGameSettings::m_OverClockModifier = 1;

void CGameSettings::RefreshGameSettings()
{
    WriteTrace(TraceN64System, TraceDebug, "start");
    m_bSMM_StoreInstruc = false /*g_Settings->LoadBool(Game_SMM_StoreInstruc)*/;
    m_bSMM_Protect = g_Settings->LoadBool(Game_SMM_Protect);
    m_bSMM_ValidFunc = g_Settings->LoadBool(Game_SMM_ValidFunc);
    m_bSMM_PIDMA = g_Settings->LoadBool(Game_SMM_PIDMA);
    m_bSMM_TLB = g_Settings->LoadBool(Game_SMM_TLB);
    m_bUseTlb = g_Settings->LoadBool(Game_UseTlb);
    m_ViRefreshRate = g_Settings->LoadDword(Game_ViRefreshRate);
    m_AiCountPerBytes = g_Settings->LoadDword(Game_AiCountPerBytes);
    m_CountPerOp = g_Settings->LoadDword(Game_CounterFactor);
    m_RdramSize = g_Settings->LoadDword(Game_RDRamSize);
    m_DelaySI = g_Settings->LoadBool(Game_DelaySI);
    m_DelayDP = g_Settings->LoadBool(Game_DelayDP);
    m_bFixedAudio = g_Settings->LoadBool(Game_FixedAudio);
    m_bSyncToAudio = m_bFixedAudio ? g_Settings->LoadBool(Game_SyncViaAudio) : false;
    m_b32Bit = g_Settings->LoadBool(Game_32Bit);
#ifdef ANDROID
    m_bFastSP = false;
#else
    m_bFastSP = g_Settings->LoadBool(Game_FastSP);
#endif
    m_RspAudioSignal = g_Settings->LoadBool(Game_RspAudioSignal);
    m_bRomInMemory = g_Settings->LoadBool(Game_LoadRomToMemory);
    m_RegCaching = g_Settings->LoadBool(Game_RegCache);
    m_bLinkBlocks = g_Settings->LoadBool(Game_BlockLinking);
    m_LookUpMode = g_Settings->LoadDword(Game_FuncLookupMode);
    m_SystemType = (SYSTEM_TYPE)g_Settings->LoadDword(Game_SystemType);
    m_CpuType = (CPU_TYPE)g_Settings->LoadDword(Game_CpuType);
    m_OverClockModifier = g_Settings->LoadDword(Game_OverClockModifier);
    m_bSyncingToAudio = m_bSyncToAudio;
    if (m_CountPerOp == 0)
    {
        m_CountPerOp = 2;
    }
    if (m_OverClockModifier < 1) { m_OverClockModifier = 1; }
    if (m_OverClockModifier > 20) { m_OverClockModifier = 20; }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CGameSettings::SpeedChanged(int SpeedLimit)
{
    int FullSpeed = g_System->m_SystemType == SYSTEM_PAL ? 50 : 60;
    m_bSyncingToAudio = SpeedLimit == FullSpeed ? m_bSyncToAudio : false;
}