#include "stdafx.h"
#include "LoggingSettings.h"

int  CLogSettings::m_RefCount = 0;
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
		g_Settings->RegisterChangeCB(Logging_GenerateLog, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogRDRamRegisters, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogSPRegisters, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogDPCRegisters, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogDPSRegisters, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogMIPSInterface, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogVideoInterface, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogAudioInterface, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogPerInterface, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogRDRAMInterface, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogSerialInterface, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogPRDMAOperations, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogPRDirectMemLoads, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogPRDMAMemLoads, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogPRDirectMemStores, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogPRDMAMemStores, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogControllerPak, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogCP0changes, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogCP0reads, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogTLB, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogExceptions, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_NoInterrupts, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogCache, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogRomHeader, NULL, RefreshSettings);
		g_Settings->RegisterChangeCB(Logging_LogUnknown, NULL, RefreshSettings);
		RefreshSettings(NULL);
	}
}

CLogSettings::~CLogSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		g_Settings->UnregisterChangeCB(Logging_GenerateLog, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogRDRamRegisters, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogSPRegisters, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogDPCRegisters, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogDPSRegisters, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogMIPSInterface, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogVideoInterface, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogAudioInterface, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogPerInterface, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogRDRAMInterface, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogSerialInterface, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogPRDMAOperations, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogPRDirectMemLoads, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogPRDMAMemLoads, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogPRDirectMemStores, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogPRDMAMemStores, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogControllerPak, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogCP0changes, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogCP0reads, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogTLB, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogExceptions, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_NoInterrupts, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogCache, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogRomHeader, NULL, RefreshSettings);
		g_Settings->UnregisterChangeCB(Logging_LogUnknown, NULL, RefreshSettings);
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