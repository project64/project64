#include "stdafx.h"

#include "SPRegistersHandler.h"
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64System.h>
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
    RSPRegisterHandler(Reg.m_SigProcessor_Interface, MMU.Rdram(), MMU.RdramSize(), m_IMEM, m_DMEM),
    MIPSInterfaceReg(Reg.m_Mips_Interface),
    m_System(System),
    m_MMU(MMU),
    m_Reg(Reg),
    m_RspIntrReg(Reg.m_RspIntrReg),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
}

bool SPRegistersHandler::Read32(uint32_t Address, uint32_t & Value)
{
    if (Address < 0x04040000)
    {
        if ((Address & 0x1000) == 0)
        {
            Value = *((uint32_t *)&m_DMEM[Address & 0xFFC]);
        }
        else
        {
            Value = *((uint32_t *)&m_IMEM[Address & 0xFFC]);
        }
        return true;
    }

    switch (Address & 0x1FFFFFFF)
    {
    case 0x04040000: Value = SP_MEM_ADDR_REG; break;
    case 0x04040004: Value = SP_DRAM_ADDR_REG; break;
    case 0x04040008: Value = SP_RD_LEN_REG; break;
    case 0x0404000C: Value = SP_WR_LEN_REG; break;
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
        if (BreakOnUnhandledMemory())
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
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool SPRegistersHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    uint32_t MaskedValue = Value & Mask;
    if (Address < 0x04040000)
    {
        if ((Address & 0x1000) == 0)
        {
            *((uint32_t *)&m_DMEM[Address & 0xFFC]) = MaskedValue;
        }
        else
        {
            *((uint32_t *)&m_IMEM[Address & 0xFFC]) = MaskedValue;
        }
        return true;
    }
    if (GenerateLog() && LogSPRegisters())
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
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    switch (Address & 0x1FFFFFFF)
    {
    case 0x04040000: WriteReg(RSPRegister_MEM_ADDR, (SP_MEM_ADDR_REG & ~Mask) | (MaskedValue)); break;
    case 0x04040004: WriteReg(RSPRegister_DRAM_ADDR, (SP_DRAM_ADDR_REG & ~Mask) | (MaskedValue)); break;
    case 0x04040008: WriteReg(RSPRegister_RD_LEN, MaskedValue); break;
    case 0x0404000C: WriteReg(RSPRegister_WR_LEN, MaskedValue); break;
    case 0x04040010:
        WriteReg(RSPRegister_STATUS, MaskedValue);
        if ((MaskedValue & SP_SET_SIG0) != 0 && RspAudioSignal())
        {
            MI_INTR_REG |= MI_INTR_SP;
            m_Reg.CheckInterrupts();
        }
        m_System.GetPlugins()->RSP()->RunRSP();
        break;
    case 0x0404001C: SP_SEMAPHORE_REG = 0; break;
    case 0x04080000: SP_PC_REG = MaskedValue & 0xFFC; break;
    default:
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}

void SPRegistersHandler::ClearSPInterrupt(void)
{
    MI_INTR_REG &= ~MI_INTR_SP;
    m_RspIntrReg &= ~MI_INTR_SP;
    m_Reg.CheckInterrupts();
}

void SPRegistersHandler::SetSPInterrupt(void)
{
    MI_INTR_REG |= MI_INTR_SP;
    m_RspIntrReg |= MI_INTR_SP;
    m_Reg.CheckInterrupts();
}

void SPRegistersHandler::SetHalt(void)
{
}

void SPRegistersHandler::SystemReset(void)
{
    SP_RD_LEN_REG = 0x00000FF8;
    SP_WR_LEN_REG = 0x00000FF8;

    memset(m_IMEM, 0, sizeof(m_IMEM));
    memset(m_DMEM, 0, sizeof(m_DMEM));
}