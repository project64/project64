#include "stdafx.h"

#include "RDRAMRegistersHandler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\SystemGlobals.h>

RDRAMRegistersReg::RDRAMRegistersReg(uint32_t * RdramInterface) :
    RDRAM_CONFIG_REG(RdramInterface[0]),
    RDRAM_DEVICE_TYPE_REG(RdramInterface[0]),
    RDRAM_DEVICE_ID_REG(RdramInterface[1]),
    RDRAM_DELAY_REG(RdramInterface[2]),
    RDRAM_MODE_REG(RdramInterface[3]),
    RDRAM_REF_INTERVAL_REG(RdramInterface[4]),
    RDRAM_REF_ROW_REG(RdramInterface[5]),
    RDRAM_RAS_INTERVAL_REG(RdramInterface[6]),
    RDRAM_MIN_INTERVAL_REG(RdramInterface[7]),
    RDRAM_ADDR_SELECT_REG(RdramInterface[8]),
    RDRAM_DEVICE_MANUF_REG(RdramInterface[9])
{
}

RDRAMRegistersHandler::RDRAMRegistersHandler(CRegisters & Reg) :
    RDRAMRegistersReg(Reg.m_RDRAM_Registers),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool RDRAMRegistersHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x03F00000: Value = RDRAM_CONFIG_REG; break;
    case 0x03F00004: Value = RDRAM_DEVICE_ID_REG; break;
    case 0x03F00008: Value = RDRAM_DELAY_REG; break;
    case 0x03F0000C: Value = RDRAM_MODE_REG; break;
    case 0x03F00010: Value = RDRAM_REF_INTERVAL_REG; break;
    case 0x03F00014: Value = RDRAM_REF_ROW_REG; break;
    case 0x03F00018: Value = RDRAM_RAS_INTERVAL_REG; break;
    case 0x03F0001C: Value = RDRAM_MIN_INTERVAL_REG; break;
    case 0x03F00020: Value = RDRAM_ADDR_SELECT_REG; break;
    case 0x03F00024: Value = RDRAM_DEVICE_MANUF_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (LogRDRamRegisters())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x03F00000: LogMessage("%016llX: read from RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG (%08X)", m_PC, Value); break;
        case 0x03F00004: LogMessage("%016llX: read from RDRAM_DEVICE_ID_REG (%08X)", m_PC, Value); break;
        case 0x03F00008: LogMessage("%016llX: read from RDRAM_DELAY_REG (%08X)", m_PC, Value); break;
        case 0x03F0000C: LogMessage("%016llX: read from RDRAM_MODE_REG (%08X)", m_PC, Value); break;
        case 0x03F00010: LogMessage("%016llX: read from RDRAM_REF_INTERVAL_REG (%08X)", m_PC, Value); break;
        case 0x03F00014: LogMessage("%016llX: read from RDRAM_REF_ROW_REG (%08X)", m_PC, Value); break;
        case 0x03F00018: LogMessage("%016llX: read from RDRAM_RAS_INTERVAL_REG (%08X)", m_PC, Value); break;
        case 0x03F0001C: LogMessage("%016llX: read from RDRAM_MIN_INTERVAL_REG (%08X)", m_PC, Value); break;
        case 0x03F00020: LogMessage("%016llX: read from RDRAM_ADDR_SELECT_REG (%08X)", m_PC, Value); break;
        case 0x03F00024: LogMessage("%016llX: read from RDRAM_DEVICE_MANUF_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool RDRAMRegistersHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (LogRDRamRegisters())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x03F00000: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG", m_PC, Value, Mask); break;
        case 0x03F00004: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_DEVICE_ID_REG", m_PC, Value, Mask); break;
        case 0x03F00008: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_DELAY_REG", m_PC, Value, Mask); break;
        case 0x03F0000C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_MODE_REG", m_PC, Value, Mask); break;
        case 0x03F00010: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_REF_INTERVAL_REG", m_PC, Value, Mask); break;
        case 0x03F00014: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_REF_ROW_REG", m_PC, Value, Mask); break;
        case 0x03F00018: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_RAS_INTERVAL_REG", m_PC, Value, Mask); break;
        case 0x03F0001C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_MIN_INTERVAL_REG", m_PC, Value, Mask); break;
        case 0x03F00020: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_ADDR_SELECT_REG", m_PC, Value, Mask); break;
        case 0x03F00024: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to RDRAM_DEVICE_MANUF_REG", m_PC, Value, Mask); break;
        case 0x03F04004: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to 0x%08X - Ignored", m_PC, Value, Mask, Address); break;
        case 0x03F08004: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to 0x%08X - Ignored", m_PC, Value, Mask, Address); break;
        case 0x03F80004: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to 0x%08X - Ignored", m_PC, Value, Mask, Address); break;
        case 0x03F80008: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to 0x%08X - Ignored", m_PC, Value, Mask, Address); break;
        case 0x03F8000C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to 0x%08X - Ignored", m_PC, Value, Mask, Address); break;
        case 0x03F80014: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to 0x%08X - Ignored", m_PC, Value, Mask, Address); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    switch ((Address & 0x1FFFFFFF))
    {
    case 0x03F00000: RDRAM_CONFIG_REG = (RDRAM_CONFIG_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00004: RDRAM_DEVICE_ID_REG = (RDRAM_DEVICE_ID_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00008: RDRAM_DELAY_REG = (RDRAM_DELAY_REG & ~Mask) | (Value & Mask); break;
    case 0x03F0000C: RDRAM_MODE_REG = (RDRAM_MODE_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00010: RDRAM_REF_INTERVAL_REG = (RDRAM_REF_INTERVAL_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00014: RDRAM_REF_ROW_REG = (RDRAM_REF_ROW_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00018: RDRAM_RAS_INTERVAL_REG = (RDRAM_RAS_INTERVAL_REG & ~Mask) | (Value & Mask); break;
    case 0x03F0001C: RDRAM_MIN_INTERVAL_REG = (RDRAM_MIN_INTERVAL_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00020: RDRAM_ADDR_SELECT_REG = (RDRAM_ADDR_SELECT_REG & ~Mask) | (Value & Mask); break;
    case 0x03F00024: RDRAM_DEVICE_MANUF_REG = (RDRAM_DEVICE_MANUF_REG & ~Mask) | (Value & Mask); break;
    case 0x03F04004: break;
    case 0x03F08004: break;
    case 0x03F80004: break;
    case 0x03F80008: break;
    case 0x03F8000C: break;
    case 0x03F80014: break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}
