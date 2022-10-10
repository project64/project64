#pragma once

class CLogSettings
{
public:
    CLogSettings();
    virtual ~CLogSettings();

    inline static bool GenerateLog(void)
    {
        return m_GenerateLog;
    }

    // Registers log
    inline static bool LogRDRamRegisters(void)
    {
        return m_LogRDRamRegisters;
    }
    inline static bool LogSPRegisters(void)
    {
        return m_LogSPRegisters;
    }
    inline static bool LogDPCRegisters(void)
    {
        return m_LogDPCRegisters;
    }
    inline static bool LogDPSRegisters(void)
    {
        return m_LogDPSRegisters;
    }
    inline static bool LogMIPSInterface(void)
    {
        return m_LogMIPSInterface;
    }
    inline static bool LogVideoInterface(void)
    {
        return m_LogVideoInterface;
    }
    inline static bool LogAudioInterface(void)
    {
        return m_LogAudioInterface;
    }
    inline static bool LogPerInterface(void)
    {
        return m_LogPerInterface;
    }
    inline static bool LogRDRAMInterface(void)
    {
        return m_LogRDRAMInterface;
    }
    inline static bool LogSerialInterface(void)
    {
        return m_LogSerialInterface;
    }

    // PIF RAM log
    inline static bool LogPRDMAOperations(void)
    {
        return m_LogPRDMAOperations;
    }
    inline static bool LogPRDirectMemLoads(void)
    {
        return m_LogPRDirectMemLoads;
    }
    inline static bool LogPRDMAMemLoads(void)
    {
        return m_LogPRDMAMemLoads;
    }
    inline static bool LogPRDirectMemStores(void)
    {
        return m_LogPRDirectMemStores;
    }
    inline static bool LogPRDMAMemStores(void)
    {
        return m_LogPRDMAMemStores;
    }
    inline static bool LogControllerPak(void)
    {
        return m_LogControllerPak;
    }

    // Special log
    inline static bool LogCP0changes(void)
    {
        return m_LogCP0changes;
    }
    inline static bool LogCP0reads(void)
    {
        return m_LogCP0reads;
    }
    inline static bool LogTLB(void)
    {
        return m_LogTLB;
    }
    inline static bool LogExceptions(void)
    {
        return m_LogExceptions;
    }
    inline static bool LogNoInterrupts(void)
    {
        return m_NoInterrupts;
    }
    inline static bool LogCache(void)
    {
        return m_LogCache;
    }
    inline static bool LogRomHeader(void)
    {
        return m_LogRomHeader;
    }
    inline static bool LogUnknown(void)
    {
        return m_LogUnknown;
    }

private:
    static void RefreshSettings(void *);

    static bool m_GenerateLog;

    // Registers log
    static bool m_LogRDRamRegisters;
    static bool m_LogSPRegisters;
    static bool m_LogDPCRegisters;
    static bool m_LogDPSRegisters;
    static bool m_LogMIPSInterface;
    static bool m_LogVideoInterface;
    static bool m_LogAudioInterface;
    static bool m_LogPerInterface;
    static bool m_LogRDRAMInterface;
    static bool m_LogSerialInterface;

    // PIF RAM log
    static bool m_LogPRDMAOperations;
    static bool m_LogPRDirectMemLoads;
    static bool m_LogPRDMAMemLoads;
    static bool m_LogPRDirectMemStores;
    static bool m_LogPRDMAMemStores;
    static bool m_LogControllerPak;

    // Special log
    static bool m_LogCP0changes;
    static bool m_LogCP0reads;
    static bool m_LogTLB;
    static bool m_LogExceptions;
    static bool m_NoInterrupts;
    static bool m_LogCache;
    static bool m_LogRomHeader;
    static bool m_LogUnknown;

    static int32_t m_RefCount;
};
