#pragma once

#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/Settings/DebugSettings.h>

struct GameSettings
{
    bool rspMultiThreaded;
    bool useHleGfx;
    bool useHleAudio;
    bool smmStoreInstruc;
    bool smmValidFunc;
    bool smmPidma;
    bool smmTlb;
    uint32_t viRefreshRate;
    uint32_t aiCountPerBytes;
    uint32_t countPerOp;
    uint32_t rdramSize;
    uint32_t delaySI;
    bool randomizeSipiInterrupts;
    bool delayDP;
    bool fixedAudio;
    bool syncToAudio;
    bool fullSpeed;
    bool fastSP;
    bool core32Bit;
    bool rspAudioSignal;
    bool regCaching;
    bool fpuRegCaching;
    BLOCK_LINKING_MODE blockLinkingMode;
    FUNC_LOOKUP_METHOD lookUpMode;
    SYSTEM_TYPE systemType;
    CPU_TYPE cpuType;
    uint32_t overClockModifier;
    DISK_SEEK_TYPE diskSeekTimingType;
    bool enhancementOverClock;
    uint32_t enhancementOverClockModifier;
    bool enableDisk;
    bool unalignedDMA;
};

extern GameSettings g_GameSettings;

inline bool GameLinkBlocks(void)
{
    return g_GameSettings.blockLinkingMode == BlockLinking_Eager && !g_DebugSettings.haveWriteBP && !g_DebugSettings.haveReadBP;
}

void SetupGameSettings(void);
void ShutdownGameSettings(void);
void RefreshGameSettings(void);
void RefreshSyncToAudio(void);
void SetGameOverClockModifier(bool enhancementOverClock, uint32_t enhancementOverClockModifier);
void NotifyGameSpeedChanged(int32_t speedLimit);
