#include "stdafx.h"
#include "RomMemoryHandler.h"
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64Rom.h>
#include <Project64-core\N64System\SystemGlobals.h>

RomMemoryHandler::RomMemoryHandler(CN64System & System, CRegisters & Reg, CN64Rom & Rom) :
    m_PC(Reg.m_PROGRAM_COUNTER),
    m_Reg(Reg),
    m_Rom(Rom),
    m_RomWrittenTo(false),
    m_RomWroteValue(0)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

bool RomMemoryHandler::Read32(uint32_t Address, uint32_t & Value)
{
    uint32_t ReadAddr = (Address + 2) & ~0x3;

    if (g_DDRom == nullptr)
    {
        m_Reg.PI_CART_ADDR_REG = (Address + 4) & ~1;
    }
    if (m_RomWrittenTo)
    {
        Value = m_RomWroteValue;
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_IO_BUSY;
        m_RomWrittenTo = false;
    }
    else if ((Address & 0xFFFFFFF) < m_Rom.GetRomSize())
    {
        Value = *(uint32_t *)&m_Rom.GetRomAddress()[(ReadAddr & 0xFFFFFFF)];
        if (LogRomHeader() && (ReadAddr & 0x1FFFFFFF) >= 0x10000000 && (ReadAddr & 0x1FFFFFFF) < 0x10000040)
        {
            switch (ReadAddr & 0x1FFFFFFF)
            {
            case 0x10000004: LogMessage("%08X: read from ROM clock rate (%08X)", m_PC, Value); break;
            case 0x10000008: LogMessage("%08X: read from ROM boot address offset (%08X)", m_PC, Value); break;
            case 0x1000000C: LogMessage("%08X: read from ROM release offset (%08X)", m_PC, Value); break;
            case 0x10000010: LogMessage("%08X: read from ROM CRC1 (%08X)", m_PC, Value); break;
            case 0x10000014: LogMessage("%08X: read from ROM CRC2 (%08X)", m_PC, Value); break;
            default: LogMessage("%08X: read from ROM header 0x%X (%08X)", m_PC, ReadAddr & 0xFF, Value);  break;
            }
        }
    }
    else
    {
        Value = (Address << 16) | (Address & 0xFFFF);
    }
    return true;
}

bool RomMemoryHandler::Write32(uint32_t /*Address*/, uint32_t Value, uint32_t Mask)
{
    if (!m_RomWrittenTo)
    {
        m_RomWrittenTo = true;
        m_RomWroteValue = (Value & Mask);
        m_Reg.PI_STATUS_REG |= PI_STATUS_IO_BUSY;
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
