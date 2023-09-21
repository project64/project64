#pragma once

#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/Settings/DebugSettings.h>

class CGameSettings
{
public:
    CGameSettings();
    virtual ~CGameSettings();

    void RefreshGameSettings(void);

    inline static bool RspMultiThreaded(void)
    {
        return m_RspMultiThreaded;
    }
    inline static bool UseHleGfx(void)
    {
        return m_UseHleGfx;
    }
    inline static bool UseHleAudio(void)
    {
        return m_UseHleAudio;
    }
    inline static bool bFPURegCaching(void)
    {
        return m_FPURegCaching;
    }
    inline static bool bRegCaching(void)
    {
        return m_RegCaching;
    }
    inline static bool bLinkBlocks(void)
    {
        return m_bLinkBlocks && !CDebugSettings::HaveWriteBP() && !CDebugSettings::HaveReadBP();
    }
    inline static FUNC_LOOKUP_METHOD LookUpMode(void)
    {
        return (FUNC_LOOKUP_METHOD)m_LookUpMode;
    }
    inline static uint32_t CountPerOp(void)
    {
        return m_CountPerOp;
    }
    inline static uint32_t ViRefreshRate(void)
    {
        return m_ViRefreshRate;
    }
    inline static uint32_t AiCountPerBytes(void)
    {
        return m_AiCountPerBytes;
    }
    inline static bool bDelayDP(void)
    {
        return m_DelayDP;
    }
    inline static bool bDelaySI(void)
    {
        return m_DelaySI;
    }
    inline static bool bRandomizeSIPIInterrupts(void)
    {
        return m_bRandomizeSIPIInterrupts;
    }
    inline static uint32_t RdramSize(void)
    {
        return m_RdramSize;
    }
    inline static bool bFixedAudio(void)
    {
        return m_bFixedAudio;
    }
    inline static bool bSyncToAudio(void)
    {
        return m_bSyncToAudio;
    }
    inline static bool FullSpeed(void)
    {
        return m_FullSpeed;
    }
    inline static bool bFastSP(void)
    {
        return m_bFastSP;
    }
    inline static bool b32BitCore(void)
    {
        return m_b32Bit;
    }
    inline static bool RspAudioSignal(void)
    {
        return m_RspAudioSignal;
    }
    inline static bool bSMM_StoreInstruc(void)
    {
        return m_bSMM_StoreInstruc;
    }
    inline static bool bSMM_Protect(void)
    {
        return m_bSMM_Protect;
    }
    inline static bool bSMM_ValidFunc(void)
    {
        return m_bSMM_ValidFunc;
    }
    inline static bool bSMM_PIDMA(void)
    {
        return m_bSMM_PIDMA;
    }
    inline static bool bSMM_TLB(void)
    {
        return m_bSMM_TLB;
    }
    inline static SYSTEM_TYPE SystemType(void)
    {
        return m_SystemType;
    }
    inline static CPU_TYPE CpuType(void)
    {
        return m_CpuType;
    }
    inline static uint32_t OverClockModifier(void)
    {
        return m_OverClockModifier;
    }
    inline static DISK_SEEK_TYPE DiskSeekTimingType(void)
    {
        return m_DiskSeekTimingType;
    };
    inline static bool EnableDisk(void)
    {
        return m_EnableDisk;
    }
    inline static bool UnalignedDMA(void)
    {
        return m_UnalignedDMA;
    }

    void RefreshSyncToAudio(void);
    static void SetOverClockModifier(bool EnhancmentOverClock, uint32_t EnhancmentOverClockModifier);

protected:
    static void SpeedChanged(int32_t SpeedLimit);
    static void EnableDiskChanged(void);

private:
    CGameSettings(const CGameSettings &);
    CGameSettings & operator=(const CGameSettings &);

    static void EnableDiskChanged(void *);

    static bool m_RspMultiThreaded;
    static bool m_UseHleGfx;
    static bool m_UseHleAudio;
    static bool m_RegCaching;
    static bool m_FPURegCaching;
    static bool m_bLinkBlocks;
    static uint32_t m_LookUpMode; //FUNC_LOOKUP_METHOD
    static uint32_t m_CountPerOp;
    static uint32_t m_ViRefreshRate;
    static uint32_t m_AiCountPerBytes;
    static bool m_DelayDP;
    static bool m_DelaySI;
    static bool m_bRandomizeSIPIInterrupts;
    static uint32_t m_RdramSize;
    static bool m_bFixedAudio;
    static bool m_bSyncToAudio;
    static bool m_FullSpeed;
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
    static DISK_SEEK_TYPE m_DiskSeekTimingType;
    static bool m_EnhancmentOverClock;
    static uint32_t m_EnhancmentOverClockModifier;
    static bool m_EnableDisk;
    static bool m_UnalignedDMA;
    static int32_t m_RefCount;
};
