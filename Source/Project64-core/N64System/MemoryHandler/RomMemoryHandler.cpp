#include "stdafx.h"
#include "RomMemoryHandler.h"
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64Rom.h>

RomMemoryHandler::RomMemoryHandler(CN64System & System, CRegisters & Reg, CN64Rom & Rom) :
    m_PC(Reg.m_PROGRAM_COUNTER),
    m_Rom(Rom),
    m_RomWrittenTo(false),
    m_RomWroteValue(0)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

bool RomMemoryHandler::Read32(uint32_t Address, uint32_t & Value)
{
    if (m_RomWrittenTo)
    {
        Value = m_RomWroteValue;
        //LogMessage("%X: Read crap from ROM %08X from %08X",PROGRAM_COUNTER,*Value,PAddr);
        m_RomWrittenTo = false;
    }
    else if ((Address & 0xFFFFFFF) < m_Rom.GetRomSize())
    {
        Value = *(uint32_t *)&m_Rom.GetRomAddress()[(Address & 0xFFFFFFF)];
    }
    else
    {
        Value = (Address << 16) | (Address & 0xFFFF);
    }

    if (LogRomHeader() && (Address & 0x1FFFFFFF) >= 0x10000000 && (Address & 0x1FFFFFFF) < 0x10000040)
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x10000004: LogMessage("%08X: read from ROM clock rate (%08X)", m_PC, Value); break;
        case 0x10000008: LogMessage("%08X: read from ROM boot address offset (%08X)", m_PC, Value); break;
        case 0x1000000C: LogMessage("%08X: read from ROM release offset (%08X)", m_PC, Value); break;
        case 0x10000010: LogMessage("%08X: read from ROM CRC1 (%08X)", m_PC, Value); break;
        case 0x10000014: LogMessage("%08X: read from ROM CRC2 (%08X)", m_PC, Value); break;
        default: LogMessage("%08X: read from ROM header 0x%X (%08X)", m_PC, Address & 0xFF, Value);  break;
        }
    }
    return true;
}

bool RomMemoryHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (((Address & 0x1FFFFFFF) - 0x10000000) < m_Rom.GetRomSize())
    {
        m_RomWrittenTo = true;
        m_RomWroteValue = (Value & Mask);
    }
    return true;
}

void RomMemoryHandler::SystemReset(void)
{
    m_RomWrittenTo = false;
    m_RomWroteValue = 0;
}

void RomMemoryHandler::LoadedGameState(void)
{
    m_RomWrittenTo = false;
    m_RomWroteValue = 0;
}
