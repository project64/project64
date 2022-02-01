#include "stdafx.h"
#include "SPRegistersHandler.h"
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\SystemGlobals.h>

SPRegistersReg::SPRegistersReg(uint32_t * SignalProcessorInterface) :
    SP_MEM_ADDR_REG(SignalProcessorInterface[0]),
    SP_DRAM_ADDR_REG(SignalProcessorInterface[1]),
    SP_RD_LEN_REG(SignalProcessorInterface[2]),
    SP_WR_LEN_REG(SignalProcessorInterface[3]),
    SP_STATUS_REG(SignalProcessorInterface[4]),
    SP_DMA_FULL_REG(SignalProcessorInterface[5]),
    SP_DMA_BUSY_REG(SignalProcessorInterface[6]),
    SP_SEMAPHORE_REG(SignalProcessorInterface[7]),
    SP_PC_REG(SignalProcessorInterface[8]),
    SP_IBIST_REG(SignalProcessorInterface[9])
{
}

SPRegistersHandler::SPRegistersHandler(CN64System & System, CMipsMemoryVM & MMU, CRegisters & Reg) :
    SPRegistersReg(Reg.m_SigProcessor_Interface),
    m_System(System),
    m_MMU(MMU),
    m_Reg(Reg),
    m_RspIntrReg(Reg.m_RspIntrReg),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool SPRegistersHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04040010: Value = SP_STATUS_REG; break;
    case 0x04040014: Value = SP_DMA_FULL_REG; break;
    case 0x04040018: Value = SP_DMA_BUSY_REG; break;
    case 0x0404001C:
        Value = SP_SEMAPHORE_REG;
        SP_SEMAPHORE_REG = 1;
        break;
    case 0x04080000: Value = SP_PC_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (LogSPRegisters())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04040000: LogMessage("%08X: read from SP_MEM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04040004: LogMessage("%08X: read from SP_DRAM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04040008: LogMessage("%08X: read from SP_RD_LEN_REG (%08X)", m_PC, Value); break;
        case 0x0404000C: LogMessage("%08X: read from SP_WR_LEN_REG (%08X)", m_PC, Value); break;
        case 0x04040010: LogMessage("%08X: read from SP_STATUS_REG (%08X)", m_PC, Value); break;
        case 0x04040014: LogMessage("%08X: read from SP_DMA_FULL_REG (%08X)", m_PC, Value); break;
        case 0x04040018: LogMessage("%08X: read from SP_DMA_BUSY_REG (%08X)", m_PC, Value); break;
        case 0x0404001C: LogMessage("%08X: read from SP_SEMAPHORE_REG (%08X)", m_PC, Value); break;
        case 0x04080000: LogMessage("%08X: read from SP_PC (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool SPRegistersHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (LogSPRegisters())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04040000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_MEM_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04040004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_DRAM_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04040008: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_RD_LEN_REG", m_PC, Value, Mask); break;
        case 0x0404000C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_WR_LEN_REG", m_PC, Value, Mask); break;
        case 0x04040010: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_STATUS_REG", m_PC, Value, Mask); break;
        case 0x04040014: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_DMA_FULL_REG", m_PC, Value, Mask); break;
        case 0x04040018: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_DMA_BUSY_REG", m_PC, Value, Mask); break;
        case 0x0404001C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_SEMAPHORE_REG", m_PC, Value, Mask); break;
        case 0x04080000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to SP_PC", m_PC, Value, Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    uint32_t MaskedValue = Value & Mask;
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04040000: SP_MEM_ADDR_REG = (SP_MEM_ADDR_REG & ~Mask) | (MaskedValue); break;
    case 0x04040004: SP_DRAM_ADDR_REG = (SP_DRAM_ADDR_REG & ~Mask) | (MaskedValue);  break;
    case 0x04040008:
        SP_RD_LEN_REG = (SP_RD_LEN_REG & ~Mask) | (MaskedValue);
        SP_DMA_READ();
        break;
    case 0x0404000C:
        SP_WR_LEN_REG = (SP_WR_LEN_REG & ~Mask) | (MaskedValue);
        m_MMU.SP_DMA_WRITE();
        break;
    case 0x04040010:
        if ((MaskedValue & SP_CLR_HALT) != 0) { SP_STATUS_REG &= ~SP_STATUS_HALT; }
        if ((MaskedValue & SP_SET_HALT) != 0) { SP_STATUS_REG |= SP_STATUS_HALT; }
        if ((MaskedValue & SP_CLR_BROKE) != 0) { SP_STATUS_REG &= ~SP_STATUS_BROKE; }
        if ((MaskedValue & SP_CLR_INTR) != 0)
        {
            m_Reg.MI_INTR_REG &= ~MI_INTR_SP;
            m_RspIntrReg &= ~MI_INTR_SP;
            m_Reg.CheckInterrupts();
        }
        if ((MaskedValue & SP_SET_INTR) != 0) { if (HaveDebugger()) { g_Notify->DisplayError("SP_SET_INTR"); } }
        if ((MaskedValue & SP_CLR_SSTEP) != 0) { SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
        if ((MaskedValue & SP_SET_SSTEP) != 0) { SP_STATUS_REG |= SP_STATUS_SSTEP; }
        if ((MaskedValue & SP_CLR_INTR_BREAK) != 0) { SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
        if ((MaskedValue & SP_SET_INTR_BREAK) != 0) { SP_STATUS_REG |= SP_STATUS_INTR_BREAK; }
        if ((MaskedValue & SP_CLR_SIG0) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG0; }
        if ((MaskedValue & SP_SET_SIG0) != 0) { SP_STATUS_REG |= SP_STATUS_SIG0; }
        if ((MaskedValue & SP_CLR_SIG1) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG1; }
        if ((MaskedValue & SP_SET_SIG1) != 0) { SP_STATUS_REG |= SP_STATUS_SIG1; }
        if ((MaskedValue & SP_CLR_SIG2) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG2; }
        if ((MaskedValue & SP_SET_SIG2) != 0) { SP_STATUS_REG |= SP_STATUS_SIG2; }
        if ((MaskedValue & SP_CLR_SIG3) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG3; }
        if ((MaskedValue & SP_SET_SIG3) != 0) { SP_STATUS_REG |= SP_STATUS_SIG3; }
        if ((MaskedValue & SP_CLR_SIG4) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG4; }
        if ((MaskedValue & SP_SET_SIG4) != 0) { SP_STATUS_REG |= SP_STATUS_SIG4; }
        if ((MaskedValue & SP_CLR_SIG5) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG5; }
        if ((MaskedValue & SP_SET_SIG5) != 0) { SP_STATUS_REG |= SP_STATUS_SIG5; }
        if ((MaskedValue & SP_CLR_SIG6) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG6; }
        if ((MaskedValue & SP_SET_SIG6) != 0) { SP_STATUS_REG |= SP_STATUS_SIG6; }
        if ((MaskedValue & SP_CLR_SIG7) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG7; }
        if ((MaskedValue & SP_SET_SIG7) != 0) { SP_STATUS_REG |= SP_STATUS_SIG7; }
        if ((MaskedValue & SP_SET_SIG0) != 0 && RspAudioSignal())
        {
            m_Reg.MI_INTR_REG |= MI_INTR_SP;
            m_Reg.CheckInterrupts();
        }
        m_System.RunRSP();
        break;
    case 0x0404001C: SP_SEMAPHORE_REG = 0; break;
    case 0x04080000: SP_PC_REG = MaskedValue & 0xFFC; break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}

void SPRegistersHandler::SP_DMA_READ()
{
    SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

    if (SP_DRAM_ADDR_REG > m_MMU.RdramSize())
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\nSP_DRAM_ADDR_REG not in RDRAM space: % 08X", __FUNCTION__, g_Reg->SP_DRAM_ADDR_REG).c_str());
        }
        SP_DMA_BUSY_REG = 0;
        SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;
        return;
    }

    if (SP_RD_LEN_REG + 1 + (SP_MEM_ADDR_REG & 0xFFF) > 0x1000)
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\nCould not fit copy in memory segment", __FUNCTION__).c_str());
        }
        return;
    }

    if ((SP_MEM_ADDR_REG & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if ((SP_DRAM_ADDR_REG & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (((SP_RD_LEN_REG + 1) & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    memcpy(m_MMU.Dmem() + (SP_MEM_ADDR_REG & 0x1FFF), m_MMU.Rdram() + SP_DRAM_ADDR_REG, SP_RD_LEN_REG + 1);

    SP_DMA_BUSY_REG = 0;
    SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;
}

