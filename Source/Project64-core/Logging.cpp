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

    if ((VAddr >= 0xA0000000 && VAddr < (0xA0000000 + g_MMU->RdramSize())) ||
        (VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024) ||
        (VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC) ||
        (VAddr == 0xA4080000) || 
        (VAddr >= 0xA4100000 && VAddr <= 0xA410001C) ||
        (VAddr >= 0xA4300000 && VAddr <= 0xA430000C) ||
        (VAddr >= 0xA4400000 && VAddr <= 0xA4400034) ||
        (VAddr >= 0xA4500000 && VAddr <= 0xA4500014) ||
        (VAddr == 0xA4800000 && VAddr <= 0xA4800018) ||
        (VAddr >= 0xB0000000 && ((VAddr - 0xB0000000) < g_Rom->GetRomSize())) ||
        (VAddr >= 0xBFC00000 && VAddr <= 0xBFC007FC))
    {
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

    if ((VAddr >= 0xA0000000 && VAddr < (0xA0000000 + g_MMU->RdramSize())) ||
        (VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024) ||
        (VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC) || 
        (VAddr >= 0xA4040000 && VAddr <= 0xA404001C) ||
        (VAddr == 0xA4080000) || 
        (VAddr >= 0xA4100000 && VAddr <= 0xA410001C) ||
        (VAddr >= 0xA4300000 && VAddr <= 0xA430000C) ||
        (VAddr >= 0xA4400000 && VAddr <= 0xA4400034) ||
        (VAddr >= 0xA4500000 && VAddr <= 0xA4500014) ||
        (VAddr >= 0xA4800000 && VAddr <= 0xA4800018) ||
        (VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC))
    {
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
