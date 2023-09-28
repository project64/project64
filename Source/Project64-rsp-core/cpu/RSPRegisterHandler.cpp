#include "RSPRegisterHandler.h"
#include "RSPRegisters.h"
#include <Project64-plugin-spec/Rsp.h>
#include <Settings/Settings.h>
#include <string.h>

RSPRegisterHandler::RSPRegisterHandler(uint32_t * SignalProcessorInterface, uint8_t *& Rdram, const uint32_t & RdramSize, uint8_t * IMEM, uint8_t * DMEM) :
    SP_MEM_ADDR_REG(SignalProcessorInterface[0]),
    SP_DRAM_ADDR_REG(SignalProcessorInterface[1]),
    SP_RD_LEN_REG(SignalProcessorInterface[2]),
    SP_WR_LEN_REG(SignalProcessorInterface[3]),
    SP_STATUS_REG(SignalProcessorInterface[4]),
    SP_DMA_FULL_REG(SignalProcessorInterface[5]),
    SP_DMA_BUSY_REG(SignalProcessorInterface[6]),
    SP_SEMAPHORE_REG(SignalProcessorInterface[7]),
    SP_PC_REG(SignalProcessorInterface[8]),
    m_Rdram(Rdram),
    m_RdramSize(RdramSize),
    m_IMEM(IMEM),
    m_DMEM(DMEM),
    m_PendingSPMemAddr(0),
    m_PendingSPDramAddr(0)
{
}

RSPRegisterHandler::RSPRegisterHandler(_RSP_INFO & RSPInfo, const uint32_t & RdramSize) :
    SP_MEM_ADDR_REG(*RSPInfo.SP_MEM_ADDR_REG),
    SP_DRAM_ADDR_REG(*RSPInfo.SP_DRAM_ADDR_REG),
    SP_RD_LEN_REG(*RSPInfo.SP_RD_LEN_REG),
    SP_WR_LEN_REG(*RSPInfo.SP_WR_LEN_REG),
    SP_STATUS_REG(*RSPInfo.SP_STATUS_REG),
    SP_DMA_FULL_REG(*RSPInfo.SP_DMA_FULL_REG),
    SP_DMA_BUSY_REG(*RSPInfo.SP_DMA_BUSY_REG),
    SP_SEMAPHORE_REG(*RSPInfo.SP_SEMAPHORE_REG),
    SP_PC_REG(*RSPInfo.SP_PC_REG),
    m_Rdram(RSPInfo.RDRAM),
    m_RdramSize(RdramSize),
    m_IMEM(RSPInfo.IMEM),
    m_DMEM(RSPInfo.DMEM),
    m_PendingSPMemAddr(0),
    m_PendingSPDramAddr(0)
{
}

void RSPRegisterHandler::SP_DMA_READ()
{
    SP_STATUS_REG |= SP_STATUS_DMA_BUSY;

    uint8_t * Dest = ((m_PendingSPMemAddr & 0x1000) != 0 ? m_IMEM : m_DMEM);
    uint8_t * Source = m_Rdram;
    uint32_t ReadPos = m_PendingSPDramAddr & 0x00FFFFF8;
    int32_t Length = ((SP_RD_LEN_REG & 0xFFF) | 7) + 1;
    int32_t Count = ((SP_RD_LEN_REG >> 12) & 0xFF) + 1;
    int32_t Skip = (SP_RD_LEN_REG >> 20) & 0xF8;
    int32_t Pos = (m_PendingSPMemAddr & 0x0FF8);

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
        if ((ReadPos + Length) > m_RdramSize)
        {
            if ((m_RdramSize - ReadPos) < (uint32_t)CopyLength)
            {
                CopyLength = (int32_t)m_RdramSize - (int32_t)ReadPos;
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
            if ((CopyLength + ReadPos) > m_RdramSize)
            {
                int32_t CopyAmount = m_RdramSize - ReadPos;
                if (CopyAmount < 0)
                {
                    CopyAmount = 0;
                }
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

    SP_MEM_ADDR_REG = (Pos & 0xFFF) | (m_PendingSPMemAddr & 0x1000);
    SP_DRAM_ADDR_REG = ReadPos;
    SP_RD_LEN_REG = (SP_RD_LEN_REG & 0xFF800000) | 0x00000FF8;
    SP_WR_LEN_REG = (SP_RD_LEN_REG & 0xFF800000) | 0x00000FF8;
    DmaReadDone(Pos);
}

void RSPRegisterHandler::SP_DMA_WRITE()
{
    SP_STATUS_REG |= SP_STATUS_DMA_BUSY;

    uint8_t * Source = ((m_PendingSPMemAddr & 0x1000) != 0 ? m_IMEM : m_DMEM);
    uint8_t * Dest = m_Rdram;
    uint32_t WritePos = m_PendingSPDramAddr & 0x00FFFFF8;
    int32_t Length = ((SP_WR_LEN_REG & 0xFFF) | 7) + 1;
    int32_t Count = ((SP_WR_LEN_REG >> 12) & 0xFF) + 1;
    int32_t Skip = (SP_WR_LEN_REG >> 20) & 0xF8;
    int32_t Pos = (m_PendingSPMemAddr & 0x0FF8);

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

        if (WritePos < m_RdramSize)
        {
            int32_t CopyAmount = (int32_t)m_RdramSize - (int32_t)WritePos;
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

            int32_t CopyAmount = (int32_t)m_RdramSize - (int32_t)WritePos;
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

    SP_MEM_ADDR_REG = (Pos & 0xFFF) | (m_PendingSPMemAddr & 0x1000);
    SP_DRAM_ADDR_REG = WritePos;
    SP_RD_LEN_REG = (SP_WR_LEN_REG & 0xFF800000) | 0x00000FF8;
    SP_WR_LEN_REG = (SP_WR_LEN_REG & 0xFF800000) | 0x00000FF8;
}

uint32_t RSPRegisterHandler::ReadReg(RSPRegister Reg)
{
    switch (Reg)
    {
    case RSPRegister_MEM_ADDR: return SP_MEM_ADDR_REG;
    case RSPRegister_DRAM_ADDR: return SP_DRAM_ADDR_REG;
    case RSPRegister_RD_LEN: return SP_RD_LEN_REG;
    case RSPRegister_WR_LEN: return SP_WR_LEN_REG;
    case RSPRegister_STATUS: return SP_STATUS_REG;
    }
    return 0;
}

void RSPRegisterHandler::WriteReg(RSPRegister Reg, uint32_t Value)
{
    switch (Reg)
    {
    case RSPRegister_MEM_ADDR: m_PendingSPMemAddr = Value; break;
    case RSPRegister_DRAM_ADDR: m_PendingSPDramAddr = Value; break;
    case RSPRegister_RD_LEN:
        SP_RD_LEN_REG = Value;
        SP_DMA_READ();
        break;
    case RSPRegister_WR_LEN:
        SP_WR_LEN_REG = Value;
        SP_DMA_WRITE();
        break;
    case RSPRegister_STATUS:
        if ((Value & SP_CLR_HALT) != 0 && (Value & SP_SET_HALT) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_HALT;
        }
        if ((Value & SP_SET_HALT) != 0 && (Value & SP_CLR_HALT) == 0 && (SP_STATUS_REG & SP_STATUS_HALT) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_HALT;
            SetHalt();
        }
        if ((Value & SP_CLR_BROKE) != 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_BROKE;
        }
        if ((Value & SP_CLR_INTR) != 0 && (Value & SP_SET_INTR) == 0)
        {
            ClearSPInterrupt();
        }
        if ((Value & SP_SET_INTR) != 0 && (Value & SP_CLR_INTR) == 0)
        {
            SetSPInterrupt();
        }
        if ((Value & SP_CLR_SSTEP) != 0 && (Value & SP_SET_SSTEP) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SSTEP;
        }
        if ((Value & SP_SET_SSTEP) != 0 && (Value & SP_CLR_SSTEP) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SSTEP;
        }
        if ((Value & SP_CLR_INTR_BREAK) != 0 && (Value & SP_SET_INTR_BREAK) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK;
        }
        if ((Value & SP_SET_INTR_BREAK) != 0 && (Value & SP_CLR_INTR_BREAK) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_INTR_BREAK;
        }
        if ((Value & SP_CLR_SIG0) != 0 && (Value & SP_SET_SIG0) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG0;
        }
        if ((Value & SP_SET_SIG0) != 0 && (Value & SP_CLR_SIG0) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG0;
        }
        if ((Value & SP_CLR_SIG1) != 0 && (Value & SP_SET_SIG1) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG1;
        }
        if ((Value & SP_SET_SIG1) != 0 && (Value & SP_CLR_SIG1) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG1;
        }
        if ((Value & SP_CLR_SIG2) != 0 && (Value & SP_SET_SIG2) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG2;
        }
        if ((Value & SP_SET_SIG2) != 0 && (Value & SP_CLR_SIG2) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG2;
        }
        if ((Value & SP_CLR_SIG3) != 0 && (Value & SP_SET_SIG3) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG3;
        }
        if ((Value & SP_SET_SIG3) != 0 && (Value & SP_CLR_SIG3) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG3;
        }
        if ((Value & SP_CLR_SIG4) != 0 && (Value & SP_SET_SIG4) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG4;
        }
        if ((Value & SP_SET_SIG4) != 0 && (Value & SP_CLR_SIG4) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG4;
        }
        if ((Value & SP_CLR_SIG5) != 0 && (Value & SP_SET_SIG5) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG5;
        }
        if ((Value & SP_SET_SIG5) != 0 && (Value & SP_CLR_SIG5) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG5;
        }
        if ((Value & SP_CLR_SIG6) != 0 && (Value & SP_SET_SIG6) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG6;
        }
        if ((Value & SP_SET_SIG6) != 0 && (Value & SP_CLR_SIG6) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG6;
        }
        if ((Value & SP_CLR_SIG7) != 0 && (Value & SP_SET_SIG7) == 0)
        {
            SP_STATUS_REG &= ~SP_STATUS_SIG7;
        }
        if ((Value & SP_SET_SIG7) != 0 && (Value & SP_CLR_SIG7) == 0)
        {
            SP_STATUS_REG |= SP_STATUS_SIG7;
        }
        break;
    default:
        __debugbreak();
    }
}