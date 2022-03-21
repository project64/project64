#include "stdafx.h"
#include <Project64-core/Logging.h>

#include <stdio.h>
#include <stdarg.h>
#include <Common/path.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/TranslateVaddr.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/N64Rom.h>

CFile * CLogging::m_hLogFile = nullptr;

void CLogging::Log_LW(uint32_t PC, uint32_t VAddr)
{
    if (!GenerateLog())
    {
        return;
    }

    if (VAddr < 0xA0000000 || VAddr >= 0xC0000000)
    {
        uint32_t PAddr;
        if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
        {
            if (LogUnknown())
            {
                LogMessage("%08X: read from unknown ??? (%08X)", PC, VAddr);
            }
            return;
        }
        VAddr = PAddr + 0xA0000000;
    }

    uint32_t Value;
    if (VAddr >= 0xA0000000 && VAddr < (0xA0000000 + g_MMU->RdramSize()))
    {
        return;
    }
    if (VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024)
    {
        return;
    }
    if (VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC)
    {
        return;
    }
    if (VAddr == 0xA4080000)
    {
        return;
    }
    if (VAddr >= 0xA4100000 && VAddr <= 0xA410001C)
    {
        return;
    }
    if (VAddr >= 0xA4300000 && VAddr <= 0xA430000C)
    {
        return;
    }
    if (VAddr >= 0xA4400000 && VAddr <= 0xA4400034)
    {
        return;
    }
    if (VAddr >= 0xA4500000 && VAddr <= 0xA4500014)
    {
        return;
    }
    if (VAddr == 0xA4800000)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_DRAM_ADDR_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr == 0xA4800004)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_PIF_ADDR_RD64B_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr == 0xA4800010)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_PIF_ADDR_WR64B_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr == 0xA4800018)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_STATUS_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr >= 0xBFC00000 && VAddr <= 0xBFC007C0)
    {
        return;
    }
    if (VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC)
    {
        if (!LogPRDirectMemLoads())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read word from PIF RAM at 0x%X (%08X)", PC, VAddr - 0xBFC007C0, Value);
        return;
    }
    if (VAddr >= 0xB0000040 && ((VAddr - 0xB0000000) < g_Rom->GetRomSize()))
    {
        return;
    }
    if (VAddr >= 0xB0000000 && VAddr < 0xB0000040)
    {
        if (!LogRomHeader())
        {
            return;
        }

        g_MMU->LW_VAddr(VAddr, Value);
        switch (VAddr)
        {
        case 0xB0000004: LogMessage("%08X: read from ROM clock rate (%08X)", PC, Value); break;
        case 0xB0000008: LogMessage("%08X: read from ROM boot address offset (%08X)", PC, Value); break;
        case 0xB000000C: LogMessage("%08X: read from ROM release offset (%08X)", PC, Value); break;
        case 0xB0000010: LogMessage("%08X: read from ROM CRC1 (%08X)", PC, Value); break;
        case 0xB0000014: LogMessage("%08X: read from ROM CRC2 (%08X)", PC, Value); break;
        default: LogMessage("%08X: read from ROM header 0x%X (%08X)", PC, VAddr & 0xFF, Value);  break;
        }
        return;
    }
    if (!LogUnknown())
    {
        return;
    }
    LogMessage("%08X: read from unknown ??? (%08X)", PC, VAddr);
}

void CLogging::Log_SW(uint32_t PC, uint32_t VAddr, uint32_t Value)
{
    if (!GenerateLog())
    {
        return;
    }

    if (VAddr < 0xA0000000 || VAddr >= 0xC0000000)
    {
        uint32_t PAddr;
        if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
        {
            if (LogUnknown())
            {
                LogMessage("%08X: Writing 0x%08X to %08X", PC, Value, VAddr);
            }
            return;
        }
        VAddr = PAddr + 0xA0000000;
    }

    if (VAddr >= 0xA0000000 && VAddr < (0xA0000000 + g_MMU->RdramSize()))
    {
        return;
    }
    if (VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024)
    {
    }
    if (VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC)
    {
        return;
    }

    if (VAddr >= 0xA4040000 && VAddr <= 0xA404001C)
    {
        return;
    }
    if (VAddr == 0xA4080000)
    {
        return;
    }

    if (VAddr >= 0xA4100000 && VAddr <= 0xA410001C)
    {
        return;
    }

    if (VAddr >= 0xA4200000 && VAddr <= 0xA420000C)
    {
        if (!LogDPSRegisters())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4200000: LogMessage("%08X: Writing 0x%08X to DPS_TBIST_REG", PC, Value); return;
        case 0xA4200004: LogMessage("%08X: Writing 0x%08X to DPS_TEST_MODE_REG", PC, Value); return;
        case 0xA4200008: LogMessage("%08X: Writing 0x%08X to DPS_BUFTEST_ADDR_REG", PC, Value); return;
        case 0xA420000C: LogMessage("%08X: Writing 0x%08X to DPS_BUFTEST_DATA_REG", PC, Value); return;
        }
    }

    if ((VAddr >= 0xA4300000 && VAddr <= 0xA430000C) ||
        (VAddr >= 0xA4400000 && VAddr <= 0xA4400034) ||
        (VAddr >= 0xA4500000 && VAddr <= 0xA4500014))
    {
        return;
    }

    if (VAddr == 0xA4800000)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_DRAM_ADDR_REG", PC, Value); return;
    }
    if (VAddr == 0xA4800004)
    {
        if (LogPRDMAOperations())
        {
            LogMessage("%08X: A DMA transfer from the PIF RAM has occurred", PC);
        }
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_PIF_ADDR_RD64B_REG", PC, Value); return;
    }
    if (VAddr == 0xA4800010)
    {
        if (LogPRDMAOperations())
        {
            LogMessage("%08X: A DMA transfer to the PIF RAM has occurred", PC);
        }
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_PIF_ADDR_WR64B_REG", PC, Value); return;
    }
    if (VAddr == 0xA4800018)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_STATUS_REG", PC, Value); return;
    }

    if (VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC)
    {
        if (!LogPRDirectMemStores())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to PIF RAM at 0x%X", PC, Value, VAddr - 0xBFC007C0);
        return;
    }
    if (!LogUnknown())
    {
        return;
    }
    LogMessage("%08X: Writing 0x%08X to %08X ????", PC, Value, VAddr);
}

void CLogging::LogMessage(const char * Message, ...)
{
    char Msg[400];
    va_list ap;

    if (!g_Settings->LoadBool(Debugger_Enabled))
    {
        return;
    }
    if (m_hLogFile == nullptr)
    {
        return;
    }

    va_start(ap, Message);
    vsprintf(Msg, Message, ap);
    va_end(ap);

    strcat(Msg, "\r\n");

    m_hLogFile->Write(Msg, strlen(Msg));
}

void CLogging::StartLog(void)
{
    if (!GenerateLog())
    {
        StopLog();
        return;
    }
    if (m_hLogFile != nullptr)
    {
        return;
    }

    CPath LogFile(g_Settings->LoadStringVal(Directory_Log).c_str(), "cpudebug.log");
    m_hLogFile = new CFile(LogFile, CFileBase::modeCreate | CFileBase::modeWrite);
}

void CLogging::StopLog(void)
{
    if (m_hLogFile)
    {
        delete m_hLogFile;
        m_hLogFile = nullptr;
    }
}
