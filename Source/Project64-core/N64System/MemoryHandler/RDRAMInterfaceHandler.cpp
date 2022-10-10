#include "stdafx.h"

#include "RDRAMInterfaceHandler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\SystemGlobals.h>

RDRAMInterfaceReg::RDRAMInterfaceReg(uint32_t * RdramInterface) :
    RI_MODE_REG(RdramInterface[0]),
    RI_CONFIG_REG(RdramInterface[1]),
    RI_CURRENT_LOAD_REG(RdramInterface[2]),
    RI_SELECT_REG(RdramInterface[3]),
    RI_COUNT_REG(RdramInterface[4]),
    RI_REFRESH_REG(RdramInterface[4]),
    RI_LATENCY_REG(RdramInterface[5]),
    RI_RERROR_REG(RdramInterface[6]),
    RI_WERROR_REG(RdramInterface[7])
{
}

RDRAMInterfaceHandler::RDRAMInterfaceHandler(CRegisters & Reg) :
    RDRAMInterfaceReg(Reg.m_RDRAM_Interface),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool RDRAMInterfaceHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04700000: Value = RI_MODE_REG; break;
    case 0x04700004: Value = RI_CONFIG_REG; break;
    case 0x04700008: Value = RI_CURRENT_LOAD_REG; break;
    case 0x0470000C: Value = RI_SELECT_REG; break;
    case 0x04700010: Value = RI_REFRESH_REG; break;
    case 0x04700014: Value = RI_LATENCY_REG; break;
    case 0x04700018: Value = RI_RERROR_REG; break;
    case 0x0470001C: Value = RI_WERROR_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (GenerateLog() && LogRDRAMInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04700000: LogMessage("%08X: read from RI_MODE_REG (%08X)", m_PC, Value); break;
        case 0x04700004: LogMessage("%08X: read from RI_CONFIG_REG (%08X)", m_PC, Value); break;
        case 0x04700008: LogMessage("%08X: read from RI_CURRENT_LOAD_REG (%08X)", m_PC, Value); break;
        case 0x0470000C: LogMessage("%08X: read from RI_SELECT_REG (%08X)", m_PC, Value); break;
        case 0x04700010: LogMessage("%08X: read from RI_REFRESH_REG/RI_COUNT_REG (%08X)", m_PC, Value); break;
        case 0x04700014: LogMessage("%08X: read from RI_LATENCY_REG (%08X)", m_PC, Value); break;
        case 0x04700018: LogMessage("%08X: read from RI_RERROR_REG (%08X)", m_PC, Value); break;
        case 0x0470001C: LogMessage("%08X: read from RI_WERROR_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool RDRAMInterfaceHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (GenerateLog() && LogRDRAMInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04700000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_MODE_REG", m_PC, Value, Mask); break;
        case 0x04700004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_CONFIG_REG", m_PC, Value, Mask); break;
        case 0x04700008: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_CURRENT_LOAD_REG", m_PC, Value, Mask); break;
        case 0x0470000C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_SELECT_REG", m_PC, Value, Mask); break;
        case 0x04700010: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_REFRESH_REG/RI_COUNT_REG", m_PC, Value, Mask); break;
        case 0x04700014: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_LATENCY_REG", m_PC, Value, Mask); break;
        case 0x04700018: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_RERROR_REG", m_PC, Value, Mask); break;
        case 0x0470001C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to RI_WERROR_REG", m_PC, Value, Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    switch (Address & 0x1FFFFFFF)
    {
    case 0x04700000: RI_MODE_REG = (RI_MODE_REG & ~Mask) | (Value & Mask); break;
    case 0x04700004: RI_CONFIG_REG = (RI_CONFIG_REG & ~Mask) | (Value & Mask); break;
    case 0x04700008: RI_CURRENT_LOAD_REG = (RI_CURRENT_LOAD_REG & ~Mask) | (Value & Mask); break;
    case 0x0470000C: RI_SELECT_REG = (RI_SELECT_REG & ~Mask) | (Value & Mask); break;
    case 0x04700010: RI_REFRESH_REG = (RI_REFRESH_REG & ~Mask) | (Value & Mask); break;
    case 0x04700014: RI_LATENCY_REG = (RI_LATENCY_REG & ~Mask) | (Value & Mask); break;
    case 0x04700018: RI_RERROR_REG = (RI_RERROR_REG & ~Mask) | (Value & Mask); break;
    case 0x0470001C: RI_WERROR_REG = (RI_WERROR_REG & ~Mask) | (Value & Mask); break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}
