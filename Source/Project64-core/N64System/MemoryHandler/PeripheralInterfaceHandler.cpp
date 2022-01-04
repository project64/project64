#include "stdafx.h"
#include <Project64-core\N64System\SystemGlobals.h>
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\Mips\Disk.h>
#include "PeripheralInterfaceHandler.h"

PeripheralInterfaceReg::PeripheralInterfaceReg(uint32_t * PeripheralInterface) :
    PI_DRAM_ADDR_REG(PeripheralInterface[0]),
    PI_CART_ADDR_REG(PeripheralInterface[1]),
    PI_RD_LEN_REG(PeripheralInterface[2]),
    PI_WR_LEN_REG(PeripheralInterface[3]),
    PI_STATUS_REG(PeripheralInterface[4]),
    PI_BSD_DOM1_LAT_REG(PeripheralInterface[5]),
    PI_DOMAIN1_REG(PeripheralInterface[5]),
    PI_BSD_DOM1_PWD_REG(PeripheralInterface[6]),
    PI_BSD_DOM1_PGS_REG(PeripheralInterface[7]),
    PI_BSD_DOM1_RLS_REG(PeripheralInterface[8]),
    PI_BSD_DOM2_LAT_REG(PeripheralInterface[9]),
    PI_DOMAIN2_REG(PeripheralInterface[9]),
    PI_BSD_DOM2_PWD_REG(PeripheralInterface[10]),
    PI_BSD_DOM2_PGS_REG(PeripheralInterface[11]),
    PI_BSD_DOM2_RLS_REG(PeripheralInterface[12])
{
}

PeripheralInterfaceHandler::PeripheralInterfaceHandler(CMipsMemoryVM & MMU, CRegisters & Reg) :
    PeripheralInterfaceReg(Reg.m_Peripheral_Interface),
    m_MMU(MMU),
    m_Reg(Reg),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool PeripheralInterfaceHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04600000: Value = PI_DRAM_ADDR_REG; break;
    case 0x04600004: Value = PI_CART_ADDR_REG; break;
    case 0x04600008: Value = PI_RD_LEN_REG; break;
    case 0x0460000C: Value = PI_WR_LEN_REG; break;
    case 0x04600010: Value = PI_STATUS_REG; break;
    case 0x04600014: Value = PI_DOMAIN1_REG; break;
    case 0x04600018: Value = PI_BSD_DOM1_PWD_REG; break;
    case 0x0460001C: Value = PI_BSD_DOM1_PGS_REG; break;
    case 0x04600020: Value = PI_BSD_DOM1_RLS_REG; break;
    case 0x04600024: Value = PI_DOMAIN2_REG; break;
    case 0x04600028: Value = PI_BSD_DOM2_PWD_REG; break;
    case 0x0460002C: Value = PI_BSD_DOM2_PGS_REG; break;
    case 0x04600030: Value = PI_BSD_DOM2_RLS_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (GenerateLog() && LogPerInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04600000: LogMessage("%08X: read from PI_DRAM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04600004: LogMessage("%08X: read from PI_CART_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04600008: LogMessage("%08X: read from PI_RD_LEN_REG (%08X)", m_PC, Value); break;
        case 0x0460000C: LogMessage("%08X: read from PI_WR_LEN_REG (%08X)", m_PC, Value); break;
        case 0x04600010: LogMessage("%08X: read from PI_STATUS_REG (%08X)", m_PC, Value); break;
        case 0x04600014: LogMessage("%08X: read from PI_BSD_DOM1_LAT_REG/PI_DOMAIN1_REG (%08X)", m_PC, Value); break;
        case 0x04600018: LogMessage("%08X: read from PI_BSD_DOM1_PWD_REG (%08X)", m_PC, Value); break;
        case 0x0460001C: LogMessage("%08X: read from PI_BSD_DOM1_PGS_REG (%08X)", m_PC, Value); break;
        case 0x04600020: LogMessage("%08X: read from PI_BSD_DOM1_RLS_REG (%08X)", m_PC, Value); break;
        case 0x04600024: LogMessage("%08X: read from PI_BSD_DOM2_LAT_REG/PI_DOMAIN2_REG (%08X)", m_PC, Value); break;
        case 0x04600028: LogMessage("%08X: read from PI_BSD_DOM2_PWD_REG (%08X)", m_PC, Value); break;
        case 0x0460002C: LogMessage("%08X: read from PI_BSD_DOM2_PGS_REG (%08X)", m_PC, Value); break;
        case 0x04600030: LogMessage("%08X: read from PI_BSD_DOM2_RLS_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool PeripheralInterfaceHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (GenerateLog() && LogPerInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04600000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_DRAM_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04600004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_CART_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04600008: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_RD_LEN_REG", m_PC, Value, Mask); break;
        case 0x0460000C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_WR_LEN_REG", m_PC, Value, Mask); break;
        case 0x04600010: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_STATUS_REG", m_PC, Value, Mask); break;
        case 0x04600014: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM1_LAT_REG/PI_DOMAIN1_REG", m_PC, Value, Mask); break;
        case 0x04600018: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM1_PWD_REG", m_PC, Value, Mask); break;
        case 0x0460001C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM1_PGS_REG", m_PC, Value, Mask); break;
        case 0x04600020: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM1_RLS_REG", m_PC, Value, Mask); break;
        case 0x04600024: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM2_LAT_REG/PI_DOMAIN2_REG", m_PC, Value, Mask); break;
        case 0x04600028: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM2_PWD_REG", m_PC, Value, Mask); break;
        case 0x0460002C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM2_PGS_REG", m_PC, Value, Mask); break;
        case 0x04600030: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to PI_BSD_DOM2_RLS_REG", m_PC, Value, Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    switch (Address & 0x1FFFFFFF)
    {
    case 0x04600000: PI_DRAM_ADDR_REG = (PI_DRAM_ADDR_REG & ~Mask) | (Value & Mask); break;
    case 0x04600004:
        PI_CART_ADDR_REG = (PI_CART_ADDR_REG & ~Mask) | (Value & Mask);
        if (EnableDisk())
        {
            DiskDMACheck();
        }
        break;
    case 0x04600008:
        PI_RD_LEN_REG = (PI_RD_LEN_REG & ~Mask) | (Value & Mask);
        m_MMU.PI_DMA_READ();
        break;
    case 0x0460000C:
        PI_WR_LEN_REG = (PI_WR_LEN_REG & ~Mask) | (Value & Mask);
        m_MMU.PI_DMA_WRITE();
        break;
    case 0x04600010:
        //if ((Value & PI_SET_RESET) != 0 )
        //{
        //    g_Notify->DisplayError("reset Controller");
        //}
        if ((Value & PI_CLR_INTR) != 0)
        {
            g_Reg->MI_INTR_REG &= ~MI_INTR_PI;
            g_Reg->CheckInterrupts();
        }
        break;
    case 0x04600014: PI_DOMAIN1_REG = ((PI_DOMAIN1_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x04600018: PI_BSD_DOM1_PWD_REG = ((PI_BSD_DOM1_PWD_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x0460001C: PI_BSD_DOM1_PGS_REG = ((PI_BSD_DOM1_PGS_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x04600020: PI_BSD_DOM1_RLS_REG = ((PI_BSD_DOM1_RLS_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x04600024: PI_DOMAIN2_REG = ((PI_DOMAIN2_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x04600028: PI_BSD_DOM2_PWD_REG = ((PI_BSD_DOM2_PWD_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x0460002C: PI_BSD_DOM2_PGS_REG = ((PI_BSD_DOM2_PGS_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    case 0x04600030: PI_BSD_DOM2_RLS_REG = ((PI_BSD_DOM2_RLS_REG & ~Mask) | (Value & Mask)) & 0xFF; break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}
