#include "stdafx.h"

#include "SerialInterfaceHandler.h"
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\SystemGlobals.h>

SerialInterfaceReg::SerialInterfaceReg(uint32_t * Interface) :
    SI_DRAM_ADDR_REG(Interface[0]),
    SI_PIF_ADDR_RD64B_REG(Interface[1]),
    SI_PIF_ADDR_WR64B_REG(Interface[2]),
    SI_STATUS_REG(Interface[3])
{
}

SerialInterfaceHandler::SerialInterfaceHandler(CMipsMemoryVM & MMU, CRegisters & Reg) :
    SerialInterfaceReg(Reg.m_SerialInterface),
    MIPSInterfaceReg(Reg.m_Mips_Interface),
    m_MMU(MMU),
    m_Reg(Reg),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool SerialInterfaceHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04800018: Value = SI_STATUS_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (LogSerialInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04800000: LogMessage("%08X: read from SI_DRAM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04800004: LogMessage("%08X: read from SI_PIF_ADDR_RD64B_REG (%08X)", m_PC, Value); break;
        case 0xA4800010: LogMessage("%08X: read from SI_PIF_ADDR_WR64B_REG (%08X)", m_PC, Value); break;
        case 0x04800018: LogMessage("%08X: read from SI_STATUS_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool SerialInterfaceHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (GenerateLog())
    {
        if (LogSerialInterface())
        {
            switch (Address & 0x1FFFFFFF)
            {
            case 0x04800000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SI_DRAM_ADDR_REG", m_PC, Value, Mask); break;
            case 0x04800004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SI_PIF_ADDR_RD64B_REG", m_PC, Value, Mask); break;
            case 0x04800010: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SI_PIF_ADDR_WR64B_REG", m_PC, Value, Mask); break;
            case 0x04800018: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SI_STATUS_REG", m_PC, Value, Mask); break;
            default:
                if (HaveDebugger())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        if (LogPRDMAOperations())
        {
            switch (Address & 0x1FFFFFFF)
            {
            case 0x04800004: LogMessage("%08X: A DMA transfer from the PIF RAM has occurred", m_PC); break;
            case 0x04800010: LogMessage("%08X: A DMA transfer to the PIF RAM has occurred", m_PC); break;
            }
        }
    }

    uint32_t MaskedValue = Value & Mask;
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04800000: SI_DRAM_ADDR_REG = (SI_DRAM_ADDR_REG & ~Mask) | (MaskedValue); break;
    case 0x04800004:
        SI_PIF_ADDR_RD64B_REG = (SI_PIF_ADDR_RD64B_REG & ~Mask) | (MaskedValue);
        m_MMU.SI_DMA_READ();
        break;
    case 0x04800010:
        SI_PIF_ADDR_WR64B_REG = (SI_PIF_ADDR_WR64B_REG & ~Mask) | (MaskedValue);
        m_MMU.SI_DMA_WRITE();
        break;
    case 0x04800018:
        MI_INTR_REG &= ~MI_INTR_SI;
        m_Reg.SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
        m_Reg.CheckInterrupts();
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}
