#include "RspLog.h"
#include "RSPRegisters.h"
#include <Common/File.h>
#include <Common/Log.h>
#include <Common/StdString.h>
#include <Common/path.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>

CRDPLog RDPLog(RSPSystem);
CLog * CPULog = nullptr;

void StartCPULog(void)
{
    if (CPULog != nullptr)
    {
        return;
    }
    char LogDir[260];
    CPath LogFilePath(GetSystemSettingSz(Set_DirectoryLog, LogDir, sizeof(LogDir)), "RSP_x86Log.txt");
    CPULog = new CLog;
    if (CPULog != nullptr)
    {
        if (CPULog->Open(LogFilePath))
        {
            CPULog->SetMaxFileSize(300 * CLog::MB);
        }
        else
        {
            StopCPULog();
        }
    }
}

void StopCPULog(void)
{
    if (CPULog != NULL)
    {
        delete CPULog;
        CPULog = NULL;
    }
}

void CPU_Message(const char * Message, ...)
{
    if (CPULog == NULL)
    {
        return;
    }

    stdstr Msg;

    va_list args;
    va_start(args, Message);
    Msg.ArgFormat(Message, args);
    va_end(args);

    Msg += "\r\n";

    CPULog->Log(Msg.c_str());
}

CRDPLog::CRDPLog(CRSPSystem & System) :
    m_System(System),
    m_Log(nullptr),
    m_DPC_START_REG(System.m_DPC_START_REG),
    m_DPC_END_REG(System.m_DPC_END_REG),
    m_DPC_CURRENT_REG(System.m_DPC_CURRENT_REG),
    m_DPC_STATUS_REG(System.m_DPC_STATUS_REG),
    m_DPC_CLOCK_REG(System.m_DPC_CLOCK_REG),
    m_RDRAM(System.m_RDRAM),
    m_DMEM(System.m_DMEM)
{
}

void CRDPLog::StartLog(void)
{
    if (m_Log == nullptr && Set_DirectoryLog != 0)
    {
        char LogDir[260];
        CPath LogFilePath(GetSystemSettingSz(Set_DirectoryLog, LogDir, sizeof(LogDir)), "RDP_Log.txt");
        m_Log = new CLog;
        m_Log->Open(LogFilePath);
        m_Log->SetMaxFileSize(400 * 1024 * 1024);
        //		RDPLog->SetFlush(true);
    }
}

void CRDPLog::StopLog(void)
{
    if (m_Log != nullptr)
    {
        delete m_Log;
        m_Log = nullptr;
    }
}

void CRDPLog::Message(const char * Message, ...)
{
    if (m_Log == nullptr)
    {
        return;
    }

    stdstr Msg;

    va_list args;
    va_start(args, Message);
    Msg.ArgFormat(Message, args);
    va_end(args);

    Msg += "\r\n";

    m_Log->Log(Msg.c_str());
}

void CRDPLog::LogMT0(uint32_t PC, int Reg, uint32_t Value)
{
    if (m_Log == nullptr)
    {
        return;
    }
    switch (Reg)
    {
    case 0: Message("%03X: Stored 0x%08X into SP_MEM_ADDR_REG", PC, Value); break;
    case 1: Message("%03X: Stored 0x%08X into SP_DRAM_ADDR_REG", PC, Value); break;
    case 2: Message("%03X: Stored 0x%08X into SP_RD_LEN_REG", PC, Value); break;
    case 3: Message("%03X: Stored 0x%08X into SP_WR_LEN_REG", PC, Value); break;
    case 4: Message("%03X: Stored 0x%08X into SP_STATUS_REG", PC, Value); break;
    case 5: Message("%03X: Stored 0x%08X into Reg 5 ???", PC, Value); break;
    case 6: Message("%03X: Stored 0x%08X into Reg 6 ???", PC, Value); break;
    case 7: Message("%03X: Stored 0x%08X into SP_SEMAPHORE_REG", PC, Value); break;
    case 8: Message("%03X: Stored 0x%08X into DPC_START_REG", PC, Value); break;
    case 9: Message("%03X: Stored 0x%08X into DPC_END_REG", PC, Value); break;
    case 10: Message("%03X: Stored 0x%08X into DPC_CURRENT_REG", PC, Value); break;
    case 11: Message("%03X: Stored 0x%08X into DPC_STATUS_REG", PC, Value); break;
    case 12: Message("%03X: Stored 0x%08X into DPC_CLOCK_REG", PC, Value); break;
    }
}

void CRDPLog::LogMF0(uint32_t PC, int Reg)
{
    switch (Reg)
    {
    case 8: Message("%03X: Read 0x%08X from DPC_START_REG", PC, *m_DPC_START_REG); break;
    case 9: Message("%03X: Read 0x%08X from DPC_END_REG", PC, *m_DPC_END_REG); break;
    case 10: Message("%03X: Read 0x%08X from DPC_CURRENT_REG", PC, *m_DPC_CURRENT_REG); break;
    case 11: Message("%03X: Read 0x%08X from DPC_STATUS_REG", PC, *m_DPC_STATUS_REG); break;
    case 12: Message("%03X: Read 0x%08X from DPC_CLOCK_REG", PC, *m_DPC_CLOCK_REG); break;
    }
}

void CRDPLog::LogDlist(void)
{
    if (m_Log == nullptr)
    {
        return;
    }
    uint32_t Length = *m_DPC_END_REG - *m_DPC_CURRENT_REG;
    Message("    Dlist length = %d bytes", Length);

    uint32_t Pos = *m_DPC_CURRENT_REG;
    while (Pos < *m_DPC_END_REG)
    {
        char Hex[100], Ascii[30];
        uint32_t count;

        memset(&Hex, 0, sizeof(Hex));
        memset(&Ascii, 0, sizeof(Ascii));

        uint8_t * Mem = m_DMEM;
        if ((*m_DPC_STATUS_REG & DPC_STATUS_XBUS_DMEM_DMA) == 0)
        {
            Mem = m_RDRAM;
        }

        for (count = 0; count < 0x10; count++, Pos++)
        {
            char tmp[3];
            if ((count % 4) != 0 || count == 0)
            {
                sprintf(tmp, "%02X", Mem[Pos]);
                strcat(Hex, " ");
                strcat(Hex, tmp);
            }
            else
            {
                sprintf(tmp, "%02X", Mem[Pos]);
                strcat(Hex, " - ");
                strcat(Hex, tmp);
            }

            if (Mem[Pos] < 30 || Mem[Pos] > 127)
            {
                strcat(Ascii, ".");
            }
            else
            {
                sprintf(tmp, "%c", Mem[Pos]);
                strcat(Ascii, tmp);
            }
        }
        Message("   %s %s", Hex, Ascii);
    }
}
