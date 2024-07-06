#include "RspLog.h"
#include "RSPRegisters.h"
#include <Common/File.h>
#include <Common/Log.h>
#include <Common/StdString.h>
#include <Common/path.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Settings/Settings.h>

CLog * RDPLog = nullptr;
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

void StartRDPLog(void)
{
    if (RDPLog == nullptr && Set_DirectoryLog != 0)
    {
        char LogDir[260];
        CPath LogFilePath(GetSystemSettingSz(Set_DirectoryLog, LogDir, sizeof(LogDir)), "RDP_Log.txt");
        RDPLog = new CLog;
        RDPLog->Open(LogFilePath);
        RDPLog->SetMaxFileSize(400 * 1024 * 1024);
        //		RDPLog->SetFlush(true);
    }
}

void StopRDPLog(void)
{
    if (RDPLog != NULL)
    {
        delete RDPLog;
        RDPLog = NULL;
    }
}

void RDP_Message(const char * Message, ...)
{
    if (RDPLog == NULL)
    {
        return;
    }

    stdstr Msg;

    va_list args;
    va_start(args, Message);
    Msg.ArgFormat(Message, args);
    va_end(args);

    Msg += "\r\n";

    RDPLog->Log(Msg.c_str());
}

void RDP_LogMT0(uint32_t PC, int Reg, uint32_t Value)
{
    if (RDPLog == NULL)
    {
        return;
    }
    switch (Reg)
    {
    case 0: RDP_Message("%03X: Stored 0x%08X into SP_MEM_ADDR_REG", PC, Value); break;
    case 1: RDP_Message("%03X: Stored 0x%08X into SP_DRAM_ADDR_REG", PC, Value); break;
    case 2: RDP_Message("%03X: Stored 0x%08X into SP_RD_LEN_REG", PC, Value); break;
    case 3: RDP_Message("%03X: Stored 0x%08X into SP_WR_LEN_REG", PC, Value); break;
    case 4: RDP_Message("%03X: Stored 0x%08X into SP_STATUS_REG", PC, Value); break;
    case 5: RDP_Message("%03X: Stored 0x%08X into Reg 5 ???", PC, Value); break;
    case 6: RDP_Message("%03X: Stored 0x%08X into Reg 6 ???", PC, Value); break;
    case 7: RDP_Message("%03X: Stored 0x%08X into SP_SEMAPHORE_REG", PC, Value); break;
    case 8: RDP_Message("%03X: Stored 0x%08X into DPC_START_REG", PC, Value); break;
    case 9: RDP_Message("%03X: Stored 0x%08X into DPC_END_REG", PC, Value); break;
    case 10: RDP_Message("%03X: Stored 0x%08X into DPC_CURRENT_REG", PC, Value); break;
    case 11: RDP_Message("%03X: Stored 0x%08X into DPC_STATUS_REG", PC, Value); break;
    case 12: RDP_Message("%03X: Stored 0x%08X into DPC_CLOCK_REG", PC, Value); break;
    }
}

void RDP_LogMF0(uint32_t PC, int Reg)
{
    switch (Reg)
    {
    case 8: RDP_Message("%03X: Read 0x%08X from DPC_START_REG", PC, *RSPInfo.DPC_START_REG); break;
    case 9: RDP_Message("%03X: Read 0x%08X from DPC_END_REG", PC, *RSPInfo.DPC_END_REG); break;
    case 10: RDP_Message("%03X: Read 0x%08X from DPC_CURRENT_REG", PC, *RSPInfo.DPC_CURRENT_REG); break;
    case 11: RDP_Message("%03X: Read 0x%08X from DPC_STATUS_REG", PC, *RSPInfo.DPC_STATUS_REG); break;
    case 12: RDP_Message("%03X: Read 0x%08X from DPC_CLOCK_REG", PC, *RSPInfo.DPC_CLOCK_REG); break;
    }
}

void RDP_LogDlist(void)
{
    if (RDPLog == NULL)
    {
        return;
    }
    uint32_t Length = *RSPInfo.DPC_END_REG - *RSPInfo.DPC_CURRENT_REG;
    RDP_Message("    Dlist length = %d bytes", Length);

    uint32_t Pos = *RSPInfo.DPC_CURRENT_REG;
    while (Pos < *RSPInfo.DPC_END_REG)
    {
        char Hex[100], Ascii[30];
        uint32_t count;

        memset(&Hex, 0, sizeof(Hex));
        memset(&Ascii, 0, sizeof(Ascii));

        uint8_t * Mem = RSPInfo.DMEM;
        if ((*RSPInfo.DPC_STATUS_REG & DPC_STATUS_XBUS_DMEM_DMA) == 0)
        {
            Mem = RSPInfo.RDRAM;
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
        RDP_Message("   %s %s", Hex, Ascii);
    }
}

void RDP_LogLoc(uint32_t /*PC*/)
{
    //	RDP_Message("%03X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X",PC, RSP_GPR[26].UW, *(uint32_t *)&RSPInfo.IMEM[0xDBC],
    //		RSP_Flags[0].UW, RSP_Vect[0].UW[0],RSP_Vect[0].UW[1],RSP_Vect[0].UW[2],RSP_Vect[0].UW[3],
    //		RSP_Vect[28].UW[0],RSP_Vect[28].UW[1],RSP_Vect[28].UW[2],RSP_Vect[28].UW[3],RSP_Vect[31].UW[0]);
}
