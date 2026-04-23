#include "stdafx.h"

#include <Project64-core/Settings/LoggingSettings.h>

LogSettings g_LogSettings = {};

void RefreshLogSettings(void)
{
    if (g_Settings == nullptr)
    {
        return;
    }
    g_LogSettings.generateLog = g_Settings->LoadBool(Logging_GenerateLog);
    g_LogSettings.logRdRamRegisters = g_Settings->LoadBool(Logging_LogRDRamRegisters);
    g_LogSettings.logSpRegisters = g_Settings->LoadBool(Logging_LogSPRegisters);
    g_LogSettings.logDpcRegisters = g_Settings->LoadBool(Logging_LogDPCRegisters);
    g_LogSettings.logDpsRegisters = g_Settings->LoadBool(Logging_LogDPSRegisters);
    g_LogSettings.logMipsInterface = g_Settings->LoadBool(Logging_LogMIPSInterface);
    g_LogSettings.logVideoInterface = g_Settings->LoadBool(Logging_LogVideoInterface);
    g_LogSettings.logAudioInterface = g_Settings->LoadBool(Logging_LogAudioInterface);
    g_LogSettings.logPerInterface = g_Settings->LoadBool(Logging_LogPerInterface);
    g_LogSettings.logRdramInterface = g_Settings->LoadBool(Logging_LogRDRAMInterface);
    g_LogSettings.logSerialInterface = g_Settings->LoadBool(Logging_LogSerialInterface);
    g_LogSettings.logPrDmaOperations = g_Settings->LoadBool(Logging_LogPRDMAOperations);
    g_LogSettings.logPrDirectMemLoads = g_Settings->LoadBool(Logging_LogPRDirectMemLoads);
    g_LogSettings.logPrDmaMemLoads = g_Settings->LoadBool(Logging_LogPRDMAMemLoads);
    g_LogSettings.logPrDirectMemStores = g_Settings->LoadBool(Logging_LogPRDirectMemStores);
    g_LogSettings.logPrDmaMemStores = g_Settings->LoadBool(Logging_LogPRDMAMemStores);
    g_LogSettings.logControllerPak = g_Settings->LoadBool(Logging_LogControllerPak);
    g_LogSettings.logCp0changes = g_Settings->LoadBool(Logging_LogCP0changes);
    g_LogSettings.logCp0reads = g_Settings->LoadBool(Logging_LogCP0reads);
    g_LogSettings.logTlb = g_Settings->LoadBool(Logging_LogTLB);
    g_LogSettings.logExceptions = g_Settings->LoadBool(Logging_LogExceptions);
    g_LogSettings.logNoInterrupts = g_Settings->LoadBool(Logging_NoInterrupts);
    g_LogSettings.logCache = g_Settings->LoadBool(Logging_LogCache);
    g_LogSettings.logRomHeader = g_Settings->LoadBool(Logging_LogRomHeader);
    g_LogSettings.logUnknown = g_Settings->LoadBool(Logging_LogUnknown);
}

static bool s_LogSettingsRegistered = false;

static void LogSettingsChanged(void * /*Data*/)
{
    RefreshLogSettings();
}

void SetupLogSettings(void)
{
    if (g_Settings == nullptr || s_LogSettingsRegistered)
    {
        return;
    }

    static const SettingID kWatch[] = {
        Logging_GenerateLog,
        Logging_LogRDRamRegisters,
        Logging_LogSPRegisters,
        Logging_LogDPCRegisters,
        Logging_LogDPSRegisters,
        Logging_LogMIPSInterface,
        Logging_LogVideoInterface,
        Logging_LogAudioInterface,
        Logging_LogPerInterface,
        Logging_LogRDRAMInterface,
        Logging_LogSerialInterface,
        Logging_LogPRDMAOperations,
        Logging_LogPRDirectMemLoads,
        Logging_LogPRDMAMemLoads,
        Logging_LogPRDirectMemStores,
        Logging_LogPRDMAMemStores,
        Logging_LogControllerPak,
        Logging_LogCP0changes,
        Logging_LogCP0reads,
        Logging_LogTLB,
        Logging_LogExceptions,
        Logging_NoInterrupts,
        Logging_LogCache,
        Logging_LogRomHeader,
        Logging_LogUnknown,
    };

    for (SettingID id : kWatch)
    {
        g_Settings->RegisterChangeCB(id, nullptr, LogSettingsChanged);
    }

    RefreshLogSettings();
    s_LogSettingsRegistered = true;
}

void ShutdownLogSettings(void)
{
    if (g_Settings == nullptr || !s_LogSettingsRegistered)
    {
        return;
    }

    static const SettingID kWatch[] = {
        Logging_GenerateLog,
        Logging_LogRDRamRegisters,
        Logging_LogSPRegisters,
        Logging_LogDPCRegisters,
        Logging_LogDPSRegisters,
        Logging_LogMIPSInterface,
        Logging_LogVideoInterface,
        Logging_LogAudioInterface,
        Logging_LogPerInterface,
        Logging_LogRDRAMInterface,
        Logging_LogSerialInterface,
        Logging_LogPRDMAOperations,
        Logging_LogPRDirectMemLoads,
        Logging_LogPRDMAMemLoads,
        Logging_LogPRDirectMemStores,
        Logging_LogPRDMAMemStores,
        Logging_LogControllerPak,
        Logging_LogCP0changes,
        Logging_LogCP0reads,
        Logging_LogTLB,
        Logging_LogExceptions,
        Logging_NoInterrupts,
        Logging_LogCache,
        Logging_LogRomHeader,
        Logging_LogUnknown,
    };

    for (SettingID id : kWatch)
    {
        g_Settings->UnregisterChangeCB(id, nullptr, LogSettingsChanged);
    }

    s_LogSettingsRegistered = false;
}
