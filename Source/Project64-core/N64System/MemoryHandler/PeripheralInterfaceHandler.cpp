#include "stdafx.h"

#include "PeripheralInterfaceHandler.h"
#include <Common\MemoryManagement.h>
#include <Project64-core\Debugger.h>
#include <Project64-core\N64System\Mips\Disk.h>
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64Disk.h>
#include <Project64-core\N64System\N64Rom.h>
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\SystemGlobals.h>

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

PeripheralInterfaceHandler::PeripheralInterfaceHandler(CN64System & System, CMipsMemoryVM & MMU, CRegisters & Reg, CartridgeDomain2Address2Handler & Domain2Address2Handler) :
    PeripheralInterfaceReg(Reg.m_Peripheral_Interface),
    MIPSInterfaceReg(Reg.m_Mips_Interface),
    m_Domain2Address2Handler(Domain2Address2Handler),
    m_MMU(MMU),
    m_Reg(Reg),
    m_PC(Reg.m_PROGRAM_COUNTER),
    m_DMAUsed(false)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
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
    case 0x04600000: PI_DRAM_ADDR_REG = ((PI_DRAM_ADDR_REG & ~Mask) | (Value & Mask)) & 0x00FFFFFE; break;
    case 0x04600004:
        PI_CART_ADDR_REG = ((PI_CART_ADDR_REG & ~Mask) | (Value & Mask)) & (g_Settings->LoadBool(Game_UnalignedDMA) ? 0xFFFFFFFF : 0xFFFFFFFE);
        if (EnableDisk())
        {
            DiskDMACheck();
        }
        break;
    case 0x04600008:
        PI_RD_LEN_REG = ((PI_RD_LEN_REG & ~Mask) | (Value & Mask)) & 0x00FFFFFF;
        PI_DMA_READ();
        break;
    case 0x0460000C:
        PI_WR_LEN_REG = ((PI_WR_LEN_REG & ~Mask) | (Value & Mask)) & 0x00FFFFFF;
        PI_DMA_WRITE();
        break;
    case 0x04600010:
        //if ((Value & PI_SET_RESET) != 0 )
        //{
        //    g_Notify->DisplayError("reset Controller");
        //}
        if ((Value & PI_CLR_INTR) != 0)
        {
            MI_INTR_REG &= ~MI_INTR_PI;
            PI_STATUS_REG &= ~PI_STATUS_INTERRUPT;
            m_Reg.CheckInterrupts();
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

void PeripheralInterfaceHandler::LoadedGameState(void)
{
    m_DMAUsed = true;
}

void PeripheralInterfaceHandler::SystemReset(void)
{
    PI_RD_LEN_REG = 0x0000007F;
    PI_WR_LEN_REG = 0x0000007F;
    m_DMAUsed = false;
}

void PeripheralInterfaceHandler::OnFirstDMA()
{
    int16_t offset;
    switch (g_Rom->CicChipID())
    {
    case CIC_NUS_6101:
    case CIC_NUS_5167:
    case CIC_NUS_8303:
    case CIC_NUS_DDUS:
    case CIC_NUS_8401:
    case CIC_UNKNOWN:
    case CIC_NUS_6102:
    case CIC_NUS_6103:
    case CIC_NUS_6106:
    case CIC_NUS_5101:
    case CIC_MINI_IPL3:
        offset = 0x0318;
        break;
    case CIC_NUS_6105:
        offset = 0x03F0;
        break;
    default:
        g_Notify->DisplayError(stdstr_f("Unhandled CicChip(%d) in first DMA", g_Rom->CicChipID()).c_str());
        return;
    }
    m_MMU.UpdateMemoryValue32(0x80000000 + offset, m_MMU.RdramSize());
}

void PeripheralInterfaceHandler::PI_DMA_READ()
{
    if (g_Debugger != NULL && HaveDebugger())
    {
        g_Debugger->PIDMAReadStarted();
    }

    // PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    uint32_t PI_RD_LEN = ((PI_RD_LEN_REG)&0x00FFFFFFul) + 1;
    if ((PI_RD_LEN & 1) != 0)
    {
        PI_RD_LEN += 1;
    }

    if (PI_DRAM_ADDR_REG + PI_RD_LEN > m_MMU.RdramSize())
    {
        PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        PI_STATUS_REG |= PI_STATUS_INTERRUPT;
        MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        return;
    }

    // 64DD buffers write
    if (PI_CART_ADDR_REG >= 0x05000000 && PI_CART_ADDR_REG <= 0x050003FF)
    {
        // 64DD C2 sectors (don't care)
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_RD_LEN * 63) / 25, false);
        return;
    }

    if (PI_CART_ADDR_REG >= 0x05000400 && PI_CART_ADDR_REG <= 0x050004FF)
    {
        // 64DD user sector
        uint32_t i;
        uint8_t * RDRAM = m_MMU.Rdram();
        uint8_t * DISK = g_Disk->GetDiskAddressBuffer();
        for (i = 0; i < PI_RD_LEN_REG; i++)
        {
            *(DISK + (i ^ 3)) = *(RDRAM + ((PI_DRAM_ADDR_REG + i) ^ 3));
        }
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_RD_LEN_REG * 63) / 25, false);
        return;
    }

    if (PI_CART_ADDR_REG >= 0x05000580 && PI_CART_ADDR_REG <= 0x050005BF)
    {
        // 64DD MSEQ (don't care)
        PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        PI_STATUS_REG |= PI_STATUS_INTERRUPT;
        MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        return;
    }

    // Write ROM area (for 64DD conversion)
    if (PI_CART_ADDR_REG >= 0x10000000 && PI_CART_ADDR_REG <= 0x1FBFFFFF && g_Settings->LoadBool(Game_AllowROMWrites))
    {
        uint32_t i;
        uint8_t * ROM = g_Rom->GetRomAddress();
        uint8_t * RDRAM = m_MMU.Rdram();

        ProtectMemory(ROM, g_Rom->GetRomSize(), MEM_READWRITE);
        PI_CART_ADDR_REG -= 0x10000000;
        if (PI_CART_ADDR_REG + PI_RD_LEN_REG < g_Rom->GetRomSize())
        {
            for (i = 0; i < PI_RD_LEN_REG; i++)
            {
                *(ROM + ((PI_CART_ADDR_REG + i) ^ 3)) = *(RDRAM + ((PI_DRAM_ADDR_REG + i) ^ 3));
            }
        }
        else
        {
            uint32_t Len;
            Len = g_Rom->GetRomSize() - PI_CART_ADDR_REG;
            for (i = 0; i < Len; i++)
            {
                *(ROM + ((PI_CART_ADDR_REG + i) ^ 3)) = *(RDRAM + ((PI_DRAM_ADDR_REG + i) ^ 3));
            }
        }
        PI_CART_ADDR_REG += 0x10000000;

        if (!m_DMAUsed)
        {
            m_DMAUsed = true;
            OnFirstDMA();
        }
        if (g_Recompiler && g_System->bSMM_PIDMA())
        {
            g_Recompiler->ClearRecompCode_Phys(PI_DRAM_ADDR_REG, PI_WR_LEN_REG, CRecompiler::Remove_DMA);
        }

        ProtectMemory(ROM, g_Rom->GetRomSize(), MEM_READONLY);

        PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        PI_STATUS_REG |= PI_STATUS_INTERRUPT;
        MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        return;
    }

    if (PI_CART_ADDR_REG >= 0x08000000 && PI_CART_ADDR_REG < 0x08088000)
    {
        if (m_Domain2Address2Handler.DMARead())
        {
            return;
        }
    }
    if (g_System->m_SaveUsing == SaveChip_FlashRam)
    {
        g_Notify->DisplayError(stdstr_f("**** FlashRAM DMA read address %08X ****", PI_CART_ADDR_REG).c_str());
        PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        PI_STATUS_REG |= PI_STATUS_INTERRUPT;
        MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        return;
    }
    if (HaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("PI_DMA_READ where are you DMAing to? : %08X", PI_CART_ADDR_REG).c_str());
    }
    PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
    PI_STATUS_REG |= PI_STATUS_INTERRUPT;
    MI_INTR_REG |= MI_INTR_PI;
    m_Reg.CheckInterrupts();
    return;
}

void PeripheralInterfaceHandler::PI_DMA_WRITE()
{
    if (g_Debugger != nullptr && HaveDebugger())
    {
        g_Debugger->PIDMAWriteStarted();
    }

    if (!m_DMAUsed)
    {
        m_DMAUsed = true;
        OnFirstDMA();
    }

    uint32_t WritePos = PI_DRAM_ADDR_REG & 0x7FFFFE;
    uint32_t ReadPos = PI_CART_ADDR_REG;

    if (ReadPos >= 0x05000580 && ReadPos <= 0x050005BF)
    {
        // 64DD MSEQ (don't care)
        PI_STATUS_REG |= PI_STATUS_INTERRUPT;
        MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
    }
    else if (ReadPos >= 0x08000000 && ReadPos <= 0x08088000)
    {
        m_Domain2Address2Handler.DMAWrite();
    }
    else
    {
        int32_t Length = PI_WR_LEN_REG + 1;
        PI_WR_LEN_REG = Length <= 8 ? 0x7F - (PI_DRAM_ADDR_REG & 7) : 0x7F;

        uint8_t Block[128];
        bool FirstBlock = true;
        uint8_t * Rdram = m_MMU.Rdram();
        uint32_t TransferLen = 0;
        while (Length > 0)
        {
            int32_t BlockAlign = PI_DRAM_ADDR_REG & 6;
            int32_t BlockSize = 128 - BlockAlign;
            int32_t BlockLen = BlockSize;
            if (Length < BlockLen)
            {
                BlockLen = Length;
            }
            Length -= BlockLen;
            if (Length < 0)
            {
                Length = 0;
            }

            int32_t ReadLen = (BlockLen + 1) & ~1;
            ReadBlock(PI_CART_ADDR_REG, Block, ReadLen);
            PI_CART_ADDR_REG += ReadLen;

            if (FirstBlock)
            {
                if (BlockLen == BlockSize - 1)
                {
                    BlockLen += 1;
                }
                BlockLen = BlockLen - BlockAlign;
                if (BlockLen < 0)
                {
                    BlockLen = 0;
                }
            }

            for (int32_t i = 0; i < BlockLen; i++)
            {
                Rdram[(PI_DRAM_ADDR_REG + i) ^ 3] = Block[i];
            }
            PI_DRAM_ADDR_REG = (PI_DRAM_ADDR_REG + BlockLen + 7) & ~7;
            TransferLen += (BlockLen + 7) & ~7;
            FirstBlock = false;
        }

        if (ReadPos >= 0x05000000 && ReadPos <= 0x050004FF)
        {
            // 64DD buffers read and 64DD user sector
            PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
            g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (TransferLen * 63) / 25, false);
        }
        else if (ReadPos >= 0x06000000 && ReadPos <= 0x063FFFFF)
        {
            // 64DD IPL ROM
            PI_STATUS_REG |= PI_STATUS_INTERRUPT;
            MI_INTR_REG |= MI_INTR_PI;
            m_Reg.CheckInterrupts();
        }
        else if (ReadPos >= 0x08000000 && ReadPos <= 0x08088000)
        {
            m_Domain2Address2Handler.DMAWrite();
        }
        else if (ReadPos >= 0x10000000 && ReadPos <= 0x1FFFFFFF)
        {
            if (g_System->bRandomizeSIPIInterrupts())
            {
                //ChangeTimer(PiTimer,(int32_t)(Length * 8.9) + 50);
                //ChangeTimer(PiTimer,(int32_t)(Length * 8.9));
                PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
                g_SystemTimer->SetTimer(g_SystemTimer->PiTimer, TransferLen / 8 + (g_Random->next() % 0x40), false);
            }
            else
            {
                PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
                PI_STATUS_REG |= PI_STATUS_INTERRUPT;
                MI_INTR_REG |= MI_INTR_PI;
                m_Reg.CheckInterrupts();
            }
        }
        else
        {
            PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            MI_INTR_REG |= MI_INTR_PI;
            m_Reg.CheckInterrupts();
        }
        if (g_Recompiler && g_System->bSMM_PIDMA())
        {
            g_Recompiler->ClearRecompCode_Phys(WritePos, Length, CRecompiler::Remove_DMA);
        }
    }
}

void PeripheralInterfaceHandler::ReadBlock(uint32_t Address, uint8_t * Block, uint32_t BlockLen)
{
    if (Address >= 0x05000000 && Address <= 0x050003FF)
    {
        // 64DD buffers read - C2 sectors (just read 0)
        for (uint32_t i = 0, n = (BlockLen + 1) & ~1; i < n; i++)
        {
            Block[i] = 0;
        }
    }
    else if (Address >= 0x05000400 && Address <= 0x050004FF)
    {
        // 64DD user sector
        uint32_t ReadPos = Address - 0x05000400;
        uint8_t * DISK = g_Disk->GetDiskAddressBuffer();
        for (uint32_t i = 0, n = (BlockLen + 1) & ~1; i < n; i++)
        {
            Block[i] = DISK[((ReadPos + i) ^ 3)];
        }
    }
    else if (Address >= 0x06000000 && Address <= 0x063FFFFF)
    {
        uint32_t ReadPos = Address - 0x06000000;
        uint8_t * ROM = g_DDRom->GetRomAddress();
        uint32_t RomSize = g_DDRom->GetRomSize();
        for (uint32_t i = 0, n = (BlockLen + 1) & ~1; i < n; i++)
        {
            uint32_t Pos = ((ReadPos + i) ^ 3);
            Block[i] = Pos < RomSize ? ROM[Pos] : 0;
        }
    }
    else if (Address >= 0x10000000 && Address + BlockLen <= 0x1FFFFFFF)
    {
        uint32_t ReadPos = Address - 0x10000000;
        uint8_t * ROM = g_Rom->GetRomAddress();
        uint32_t RomSize = g_Rom->GetRomSize();
        for (uint32_t i = 0, n = (BlockLen + 1) & ~1; i < n; i++)
        {
            uint32_t Pos = ((ReadPos + i) ^ 3);
            Block[i] = Pos < RomSize ? ROM[Pos] : 0;
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}