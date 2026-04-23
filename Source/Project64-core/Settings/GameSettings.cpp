#include "stdafx.h"

#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Settings/GameSettings.h>

GameSettings g_GameSettings = {};

void RefreshSyncToAudio(void)
{
    if (g_Settings == nullptr)
    {
        return;
    }
    g_GameSettings.syncToAudio = g_Settings->LoadBool(Game_SyncViaAudio) && g_Settings->LoadBool(Setting_SyncViaAudioEnabled) &&
                                 g_Settings->LoadBool(Plugin_EnableAudio);
}

void RefreshGameSettings(void)
{
    if (g_Settings == nullptr)
    {
        return;
    }
    WriteTrace(TraceN64System, TraceDebug, "start");
    g_GameSettings.rspMultiThreaded = g_Settings->LoadBool(Game_RspMultiThreaded);
    g_GameSettings.useHleGfx = g_Settings->LoadBool(Game_UseHleGfx);
    g_GameSettings.useHleAudio = g_Settings->LoadBool(Game_UseHleAudio);
    g_GameSettings.smmStoreInstruc = g_Settings->LoadBool(Game_SMM_StoreInstruc);
    g_GameSettings.smmValidFunc = g_Settings->LoadBool(Game_SMM_ValidFunc);
    g_GameSettings.smmPidma = g_Settings->LoadBool(Game_SMM_PIDMA);
    g_GameSettings.smmTlb = g_Settings->LoadBool(Game_SMM_TLB);
    g_GameSettings.viRefreshRate = g_Settings->LoadDword(Game_ViRefreshRate);
    g_GameSettings.aiCountPerBytes = g_Settings->LoadDword(Game_AiCountPerBytes);
    g_GameSettings.countPerOp = g_Settings->LoadDword(Game_CounterFactor);
    g_GameSettings.rdramSize = g_Settings->LoadDword(Game_RDRamSize);
    g_GameSettings.delaySI = g_Settings->LoadDword(Game_DelaySI);
    g_GameSettings.randomizeSipiInterrupts = g_Settings->LoadBool(Game_RandomizeSIPIInterrupts);
    g_GameSettings.delayDP = g_Settings->LoadBool(Game_DelayDP);
    g_GameSettings.fixedAudio = g_Settings->LoadBool(Game_FixedAudio);
    g_GameSettings.fullSpeed = g_Settings->LoadBool(Game_FullSpeed);
    g_GameSettings.core32Bit = g_Settings->LoadBool(Game_32Bit);
#ifdef ANDROID
    g_GameSettings.fastSP = false;
#else
    g_GameSettings.fastSP = g_Settings->LoadBool(Game_FastSP);
#endif
    g_GameSettings.rspAudioSignal = g_Settings->LoadBool(Game_RspAudioSignal);
    g_GameSettings.regCaching = g_Settings->LoadBool(Game_RegCache);
    g_GameSettings.fpuRegCaching = g_Settings->LoadBool(Game_FPURegCache);
    g_GameSettings.blockLinkingMode = (BLOCK_LINKING_MODE)g_Settings->LoadDword(Game_BlockLinkingMode);
    g_GameSettings.lookUpMode = (FUNC_LOOKUP_METHOD)g_Settings->LoadDword(Game_FuncLookupMode);
    g_GameSettings.systemType = (SYSTEM_TYPE)g_Settings->LoadDword(Game_SystemType);
    g_GameSettings.cpuType = (CPU_TYPE)g_Settings->LoadDword(Game_CpuType);
    g_GameSettings.overClockModifier = g_Settings->LoadDword(Game_OverClockModifier);
    if (g_GameSettings.countPerOp == 0)
    {
        g_GameSettings.countPerOp = 2;
    }
    if (g_GameSettings.overClockModifier < 1)
    {
        g_GameSettings.overClockModifier = 1;
    }
    if (g_GameSettings.overClockModifier > 100)
    {
        g_GameSettings.overClockModifier = 100;
    }
    g_GameSettings.diskSeekTimingType = (DISK_SEEK_TYPE)g_Settings->LoadDword(Game_DiskSeekTiming);
    g_GameSettings.unalignedDMA = g_Settings->LoadBool(Game_UnalignedDMA);
    g_GameSettings.enableDisk = g_Settings->LoadBool(Setting_EnableDisk);
    RefreshSyncToAudio();
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void NotifyGameSpeedChanged(int32_t speedLimit)
{
    if (g_Settings == nullptr)
    {
        return;
    }
    g_GameSettings.fullSpeed = (g_GameSettings.systemType == SYSTEM_PAL ? 50 : 60) == speedLimit;
    g_Settings->SaveBool(Game_FullSpeed, g_GameSettings.fullSpeed);
}

void SetGameOverClockModifier(bool enhancementOverClock, uint32_t enhancementOverClockModifier)
{
    if (g_Settings == nullptr)
    {
        return;
    }
    g_GameSettings.enhancementOverClock = enhancementOverClock;
    g_GameSettings.enhancementOverClockModifier = enhancementOverClockModifier;

    if (g_GameSettings.enhancementOverClock)
    {
        g_GameSettings.overClockModifier = g_GameSettings.enhancementOverClockModifier;
    }
    else
    {
        g_GameSettings.overClockModifier = g_Settings->LoadDword(Game_OverClockModifier);
    }
    if (g_GameSettings.overClockModifier < 1)
    {
        g_GameSettings.overClockModifier = 1;
    }
    if (g_GameSettings.overClockModifier > 100)
    {
        g_GameSettings.overClockModifier = 100;
    }
}

static bool s_GameSettingsRegistered = false;

static void GameSettingsChanged(void * /*Data*/)
{
    RefreshGameSettings();
}

void SetupGameSettings(void)
{
    if (g_Settings == nullptr || s_GameSettingsRegistered)
    {
        return;
    }

    static const SettingID kWatch[] = {
        Game_RspMultiThreaded,
        Game_UseHleGfx,
        Game_UseHleAudio,
        Game_SMM_StoreInstruc,
        Game_SMM_ValidFunc,
        Game_SMM_PIDMA,
        Game_SMM_TLB,
        Game_ViRefreshRate,
        Game_AiCountPerBytes,
        Game_CounterFactor,
        Game_RDRamSize,
        Game_DelaySI,
        Game_RandomizeSIPIInterrupts,
        Game_DelayDP,
        Game_FixedAudio,
        Game_FullSpeed,
        Game_32Bit,
        Game_FastSP,
        Game_RspAudioSignal,
        Game_RegCache,
        Game_FPURegCache,
        Game_BlockLinkingMode,
        Game_FuncLookupMode,
        Game_SystemType,
        Game_CpuType,
        Game_OverClockModifier,
        Game_DiskSeekTiming,
        Game_UnalignedDMA,
        Setting_EnableDisk,
        Game_SyncViaAudio,
        Setting_SyncViaAudioEnabled,
        Plugin_EnableAudio,
        Game_IniKey,
    };

    for (SettingID id : kWatch)
    {
        g_Settings->RegisterChangeCB(id, nullptr, GameSettingsChanged);
    }

    RefreshGameSettings();
    s_GameSettingsRegistered = true;
}

void ShutdownGameSettings(void)
{
    if (g_Settings == nullptr || !s_GameSettingsRegistered)
    {
        return;
    }

    static const SettingID kWatch[] = {
        Game_RspMultiThreaded,
        Game_UseHleGfx,
        Game_UseHleAudio,
        Game_SMM_StoreInstruc,
        Game_SMM_ValidFunc,
        Game_SMM_PIDMA,
        Game_SMM_TLB,
        Game_ViRefreshRate,
        Game_AiCountPerBytes,
        Game_CounterFactor,
        Game_RDRamSize,
        Game_DelaySI,
        Game_RandomizeSIPIInterrupts,
        Game_DelayDP,
        Game_FixedAudio,
        Game_FullSpeed,
        Game_32Bit,
        Game_FastSP,
        Game_RspAudioSignal,
        Game_RegCache,
        Game_FPURegCache,
        Game_BlockLinkingMode,
        Game_FuncLookupMode,
        Game_SystemType,
        Game_CpuType,
        Game_OverClockModifier,
        Game_DiskSeekTiming,
        Game_UnalignedDMA,
        Setting_EnableDisk,
        Game_SyncViaAudio,
        Setting_SyncViaAudioEnabled,
        Plugin_EnableAudio,
        Game_IniKey,
    };

    for (SettingID id : kWatch)
    {
        g_Settings->UnregisterChangeCB(id, nullptr, GameSettingsChanged);
    }

    s_GameSettingsRegistered = false;
}
