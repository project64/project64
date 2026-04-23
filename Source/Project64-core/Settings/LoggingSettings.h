#pragma once

struct LogSettings
{
    bool generateLog;
    bool logRdRamRegisters;
    bool logSpRegisters;
    bool logDpcRegisters;
    bool logDpsRegisters;
    bool logMipsInterface;
    bool logVideoInterface;
    bool logAudioInterface;
    bool logPerInterface;
    bool logRdramInterface;
    bool logSerialInterface;
    bool logPrDmaOperations;
    bool logPrDirectMemLoads;
    bool logPrDmaMemLoads;
    bool logPrDirectMemStores;
    bool logPrDmaMemStores;
    bool logControllerPak;
    bool logCp0changes;
    bool logCp0reads;
    bool logTlb;
    bool logExceptions;
    bool logNoInterrupts;
    bool logCache;
    bool logRomHeader;
    bool logUnknown;
};

extern LogSettings g_LogSettings;

void SetupLogSettings(void);
void ShutdownLogSettings(void);
void RefreshLogSettings(void);
