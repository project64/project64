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
    MIPSInterfaceReg(Reg.m_Mips_Interface),
    m_SPMemAddrRegRead(0),
    m_SPDramAddrRegRead(0),
    m_ExecutedDMARead(false),
    m_System(System),
    m_MMU(MMU),
    m_Reg(Reg),
    m_RspIntrReg(Reg.m_RspIntrReg),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);    
}

bool SPRegistersHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04040000: Value = m_ExecutedDMARead ? m_SPMemAddrRegRead : SP_MEM_ADDR_REG; break;
    case 0x04040004: Value = m_ExecutedDMARead ? m_SPDramAddrRegRead : SP_DRAM_ADDR_REG; break;
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
        SP_RD_LEN_REG = MaskedValue;
        SP_DMA_READ();
        break;
    case 0x0404000C:
        SP_WR_LEN_REG = (SP_WR_LEN_REG & ~Mask) | (MaskedValue);
        SP_DMA_WRITE();
        break;
    case 0x04040010:
        if ((MaskedValue & SP_CLR_HALT) != 0) { SP_STATUS_REG &= ~SP_STATUS_HALT; }
        if ((MaskedValue & SP_SET_HALT) != 0) { SP_STATUS_REG |= SP_STATUS_HALT; }
        if ((MaskedValue & SP_CLR_BROKE) != 0) { SP_STATUS_REG &= ~SP_STATUS_BROKE; }
        if ((MaskedValue & SP_CLR_INTR) != 0)
        {
            MI_INTR_REG &= ~MI_INTR_SP;
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
            MI_INTR_REG |= MI_INTR_SP;
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
    uint8_t * Dest = ((SP_MEM_ADDR_REG & 0x1000) != 0 ? m_MMU.Imem() : m_MMU.Dmem());
    uint8_t * Source = m_MMU.Rdram();
    uint32_t ReadPos = SP_DRAM_ADDR_REG & 0x00FFFFF8;
    int32_t Length = ((SP_RD_LEN_REG & 0xFFF) | 7) + 1;
    int32_t Count = ((SP_RD_LEN_REG >> 12) & 0xFF) + 1;
    int32_t Skip = (SP_RD_LEN_REG >> 20) & 0xF8;
    int32_t Pos = (SP_MEM_ADDR_REG & 0x0FF8);

    for (int32_t i = 0; i < Count; i++)
    {
        int32_t CopyLength = Length;
        if ((Pos + Length) > 0x1000)
        {
            CopyLength = 0x1000 - Pos;
            if (CopyLength <= 0)
            {
                break;
            }
        }

        uint32_t NullLen = 0;
        if ((ReadPos + Length) > m_MMU.RdramSize())
        {
            if ((m_MMU.RdramSize() - ReadPos) < (uint32_t)CopyLength)
            {
                CopyLength = (int32_t)m_MMU.RdramSize() - (int32_t)ReadPos;
            }
            else
            {
                NullLen = CopyLength;
            }
        }

        if (NullLen != 0)
        {
            memset(&Dest[Pos], 0, NullLen);
        }
        else
        {
            memcpy(&Dest[Pos], &Source[ReadPos], CopyLength);
        }
        if (CopyLength != Length)
        {
            ReadPos = (ReadPos + CopyLength) & 0x00FFFFFF;
            CopyLength = Length - CopyLength;
            Pos = 0;
            if ((CopyLength + ReadPos) > m_MMU.RdramSize())
            {
                int32_t CopyAmount = m_MMU.RdramSize() - ReadPos;
                if (CopyAmount < 0) { CopyAmount = 0; }
                NullLen = CopyLength - CopyAmount;

                if (CopyAmount > 0)
                {
                    memcpy(&Dest[Pos], &Source[ReadPos], CopyLength);
                }
                if (NullLen > 0)
                {
                    memset(&Dest[Pos], 0, NullLen);
                }
            }
            else
            {
                memcpy(&Dest[Pos], &Source[ReadPos], CopyLength);
            }
        }
        ReadPos += CopyLength + Skip;
        Pos += CopyLength;
    }

    if (Count > 1)
    {
        ReadPos -= Skip;
    }

    SP_DMA_BUSY_REG = 0;
    SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;

    m_ExecutedDMARead = true;
    m_SPMemAddrRegRead = Pos | (SP_MEM_ADDR_REG & 0x1000);
    m_SPDramAddrRegRead = ReadPos;
    SP_RD_LEN_REG = (SP_RD_LEN_REG & 0xFF800000) | 0x00000FF8;
    SP_WR_LEN_REG = (SP_RD_LEN_REG & 0xFF800000) | 0x00000FF8;
}

void SPRegistersHandler::SP_DMA_WRITE()
{
    uint8_t * Source = ((SP_MEM_ADDR_REG & 0x1000) != 0 ? m_MMU.Imem() : m_MMU.Dmem());
    uint8_t * Dest = m_MMU.Rdram();
    uint32_t WritePos = SP_DRAM_ADDR_REG & 0x00FFFFF8;
    int32_t Length = ((SP_WR_LEN_REG & 0xFFF) | 7) + 1;
    int32_t Count = ((SP_WR_LEN_REG >> 12) & 0xFF) + 1;
    int32_t Skip = (SP_WR_LEN_REG >> 20) & 0xF8;
    int32_t Pos = (SP_MEM_ADDR_REG & 0x0FF8);

    for (int32_t i = 0; i < Count; i++)
    {
        int32_t CopyLength = Length;
        if (Pos + Length > 0x1000)
        {
            CopyLength = 0x1000 - Pos;
            if (CopyLength <= 0)
            {
                break;
            }
        }

        if (WritePos < m_MMU.RdramSize())
        {
            int32_t CopyAmount = (int32_t)m_MMU.RdramSize() - (int32_t)WritePos;
            if (CopyLength < CopyAmount)
            {
                CopyAmount = CopyLength;
            }
            if (CopyAmount > 0)
            {
                memcpy(&Dest[WritePos], &Source[Pos], CopyAmount);
            }
        }

        if (CopyLength != Length)
        {
            WritePos = (WritePos + CopyLength) & 0x00FFFFFF;
            CopyLength = Length - CopyLength;
            Pos = 0;

            int32_t CopyAmount = (int32_t)m_MMU.RdramSize() - (int32_t)WritePos;
            if (CopyLength < CopyAmount)
            {
                CopyAmount = CopyLength;
            }
            if (CopyAmount > 0)
            {
                memcpy(&Dest[WritePos], &Source[Pos], CopyAmount);
            }
        }
        WritePos += CopyLength + Skip;
        Pos += CopyLength;
    }

    if (Count > 1)
    {
        WritePos -= Skip;
    }

    SP_DMA_BUSY_REG = 0;
    SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;

    m_ExecutedDMARead = true;
    m_SPMemAddrRegRead = Pos | (SP_MEM_ADDR_REG & 0x1000);
    m_SPDramAddrRegRead = WritePos;
    SP_RD_LEN_REG = (SP_WR_LEN_REG & 0xFF800000) | 0x00000FF8;
    SP_WR_LEN_REG = (SP_WR_LEN_REG & 0xFF800000) | 0x00000FF8;
}

void SPRegistersHandler::SystemReset(void)
{
    SP_RD_LEN_REG = 0x00000FF8;
    SP_WR_LEN_REG = 0x00000FF8;
}

void SPRegistersHandler::LoadedGameState(void)
{
    m_SPMemAddrRegRead = 0;
    m_ExecutedDMARead = false;
}