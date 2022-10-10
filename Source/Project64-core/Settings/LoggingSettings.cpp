#include "stdafx.h"

#include "LoggingSettings.h"

int CLogSettings::m_RefCount = 0;
bool CLogSettings::m_GenerateLog = 0;
bool CLogSettings::m_LogRDRamRegisters = 0;
bool CLogSettings::m_LogSPRegisters = 0;
bool CLogSettings::m_LogDPCRegisters = 0;
bool CLogSettings::m_LogDPSRegisters = 0;
bool CLogSettings::m_LogMIPSInterface = 0;
bool CLogSettings::m_LogVideoInterface = 0;
bool CLogSettings::m_LogAudioInterface = 0;
bool CLogSettings::m_LogPerInterface = 0;
bool CLogSettings::m_LogRDRAMInterface = 0;
bool CLogSettings::m_LogSerialInterface = 0;
bool CLogSettings::m_LogPRDMAOperations = 0;
bool CLogSettings::m_LogPRDirectMemLoads = 0;
bool CLogSettings::m_LogPRDMAMemLoads = 0;
bool CLogSettings::m_LogPRDirectMemStores = 0;
bool CLogSettings::m_LogPRDMAMemStores = 0;
bool CLogSettings::m_LogControllerPak = 0;
bool CLogSettings::m_LogCP0changes = 0;
bool CLogSettings::m_LogCP0reads = 0;
bool CLogSettings::m_LogTLB = 0;
bool CLogSettings::m_LogExceptions = 0;
bool CLogSettings::m_NoInterrupts = 0;
bool CLogSettings::m_LogCache = 0;
bool CLogSettings::m_LogRomHeader = 0;
bool CLogSettings::m_LogUnknown = 0;

CLogSettings::CLogSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(Logging_GenerateLog, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogRDRamRegisters, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogSPRegisters, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogDPCRegisters, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogDPSRegisters, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogMIPSInterface, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogVideoInterface, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogAudioInterface, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogPerInterface, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogRDRAMInterface, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogSerialInterface, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogPRDMAOperations, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogPRDirectMemLoads, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogPRDMAMemLoads, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogPRDirectMemStores, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogPRDMAMemStores, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogControllerPak, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogCP0changes, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogCP0reads, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogTLB, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogExceptions, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_NoInterrupts, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogCache, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogRomHeader, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Logging_LogUnknown, nullptr, RefreshSettings);
        RefreshSettings(nullptr);
    }
}

CLogSettings::~CLogSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(Logging_GenerateLog, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogRDRamRegisters, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogSPRegisters, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogDPCRegisters, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogDPSRegisters, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogMIPSInterface, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogVideoInterface, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogAudioInterface, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogPerInterface, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogRDRAMInterface, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogSerialInterface, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogPRDMAOperations, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogPRDirectMemLoads, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogPRDMAMemLoads, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogPRDirectMemStores, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogPRDMAMemStores, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogControllerPak, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogCP0changes, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogCP0reads, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogTLB, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogExceptions, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_NoInterrupts, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogCache, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogRomHeader, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Logging_LogUnknown, nullptr, RefreshSettings);
    }
}

void CLogSettings::RefreshSettings(void *)
{
    m_GenerateLog = g_Settings->LoadBool(Logging_GenerateLog);
    m_LogRDRamRegisters = g_Settings->LoadBool(Logging_LogRDRamRegisters);
    m_LogSPRegisters = g_Settings->LoadBool(Logging_LogSPRegisters);
    m_LogDPCRegisters = g_Settings->LoadBool(Logging_LogDPCRegisters);
    m_LogDPSRegisters = g_Settings->LoadBool(Logging_LogDPSRegisters);
    m_LogMIPSInterface = g_Settings->LoadBool(Logging_LogMIPSInterface);
    m_LogVideoInterface = g_Settings->LoadBool(Logging_LogVideoInterface);
    m_LogAudioInterface = g_Settings->LoadBool(Logging_LogAudioInterface);
    m_LogPerInterface = g_Settings->LoadBool(Logging_LogPerInterface);
    m_LogRDRAMInterface = g_Settings->LoadBool(Logging_LogRDRAMInterface);
    m_LogSerialInterface = g_Settings->LoadBool(Logging_LogSerialInterface);
    m_LogPRDMAOperations = g_Settings->LoadBool(Logging_LogPRDMAOperations);
    m_LogPRDirectMemLoads = g_Settings->LoadBool(Logging_LogPRDirectMemLoads);
    m_LogPRDMAMemLoads = g_Settings->LoadBool(Logging_LogPRDMAMemLoads);
    m_LogPRDirectMemStores = g_Settings->LoadBool(Logging_LogPRDirectMemStores);
    m_LogPRDMAMemStores = g_Settings->LoadBool(Logging_LogPRDMAMemStores);
    m_LogControllerPak = g_Settings->LoadBool(Logging_LogControllerPak);
    m_LogCP0changes = g_Settings->LoadBool(Logging_LogCP0changes);
    m_LogCP0reads = g_Settings->LoadBool(Logging_LogCP0reads);
    m_LogTLB = g_Settings->LoadBool(Logging_LogTLB);
    m_LogExceptions = g_Settings->LoadBool(Logging_LogExceptions);
    m_NoInterrupts = g_Settings->LoadBool(Logging_NoInterrupts);
    m_LogCache = g_Settings->LoadBool(Logging_LogCache);
    m_LogRomHeader = g_Settings->LoadBool(Logging_LogRomHeader);
    m_LogUnknown = g_Settings->LoadBool(Logging_LogUnknown);
}