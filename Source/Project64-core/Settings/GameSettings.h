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
#pragma once

#include <Project64-core/N64System/N64Types.h>

class CGameSettings
{
public:
    void RefreshGameSettings(void);

    inline static bool UseHleGfx(void) { return m_UseHleGfx; }
    inline static bool bRomInMemory(void) { return m_bRomInMemory; }
    inline static bool bRegCaching(void) { return m_RegCaching; }
    inline static bool bLinkBlocks(void) { return m_bLinkBlocks; }
    inline static FUNC_LOOKUP_METHOD LookUpMode(void) { return (FUNC_LOOKUP_METHOD)m_LookUpMode; }
    inline static bool bUseTlb(void) { return m_bUseTlb; }
    inline static uint32_t CountPerOp(void) { return m_CountPerOp; }
    inline static uint32_t ViRefreshRate(void) { return m_ViRefreshRate; }
    inline static uint32_t AiCountPerBytes(void) { return m_AiCountPerBytes; }
    inline static bool bDelayDP(void) { return m_DelayDP; }
    inline static bool bDelaySI(void) { return m_DelaySI; }
    inline static uint32_t RdramSize(void) { return m_RdramSize; }
    inline static bool bFixedAudio(void) { return m_bFixedAudio; }
    inline static bool bSyncToAudio(void) { return m_bSyncingToAudio; }
    inline static bool bFastSP(void) { return m_bFastSP; }
    inline static bool b32BitCore(void) { return m_b32Bit; }
    inline static bool RspAudioSignal(void) { return m_RspAudioSignal; }
    inline static bool bSMM_StoreInstruc(void) { return m_bSMM_StoreInstruc; }
    inline static bool bSMM_Protect(void) { return m_bSMM_Protect; }
    inline static bool bSMM_ValidFunc(void) { return m_bSMM_ValidFunc; }
    inline static bool bSMM_PIDMA(void) { return m_bSMM_PIDMA; }
    inline static bool bSMM_TLB(void) { return m_bSMM_TLB; }
    inline static SYSTEM_TYPE SystemType(void) { return m_SystemType; }
    inline static CPU_TYPE CpuType(void) { return m_CpuType; }
    inline static uint32_t OverClockModifier(void) { return m_OverClockModifier; }

protected:
    static void SpeedChanged(int32_t SpeedLimit);

private:
    //Settings that can be changed on the fly
    static bool m_UseHleGfx;
    static bool m_bRomInMemory;
    static bool m_RegCaching;
    static bool m_bLinkBlocks;
    static uint32_t m_LookUpMode; //FUNC_LOOKUP_METHOD
    static bool m_bUseTlb;
    static uint32_t m_CountPerOp;
    static uint32_t m_ViRefreshRate;
    static uint32_t m_AiCountPerBytes;
    static bool m_DelayDP;
    static bool m_DelaySI;
    static uint32_t m_RdramSize;
    static bool m_bFixedAudio;
    static bool m_bSyncingToAudio;
    static bool m_bSyncToAudio;
    static bool m_bFastSP;
    static bool m_b32Bit;
    static bool m_RspAudioSignal;
    static bool m_bSMM_StoreInstruc;
    static bool m_bSMM_Protect;
    static bool m_bSMM_ValidFunc;
    static bool m_bSMM_PIDMA;
    static bool m_bSMM_TLB;
    static SYSTEM_TYPE m_SystemType;
    static CPU_TYPE m_CpuType;
    static uint32_t m_OverClockModifier;
};
