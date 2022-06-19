#include "stdafx.h"
#include <Project64-core\N64System\SystemGlobals.h>
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\Mips\Disk.h>
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\N64Rom.h>
#include <Project64-core\N64System\N64Disk.h>
#include <Project64-core\Debugger.h>
#include "PeripheralInterfaceHandler.h"
#include <Common\MemoryManagement.h>

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

PeripheralInterfaceHandler::PeripheralInterfaceHandler(CMipsMemoryVM & MMU, CRegisters & Reg, CartridgeDomain2Address2Handler & Domain2Address2Handler) :
    PeripheralInterfaceReg(Reg.m_Peripheral_Interface),
    m_Domain2Address2Handler(Domain2Address2Handler),
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
        PI_DMA_READ();
        break;
    case 0x0460000C:
        PI_WR_LEN_REG = (PI_WR_LEN_REG & ~Mask) | (Value & Mask);
        PI_DMA_WRITE();
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

void PeripheralInterfaceHandler::OnFirstDMA()
{
    int16_t offset;
    const uint32_t rt = g_MMU->RdramSize();

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
        offset = 0x0318;
        break;
    case CIC_NUS_6105:
        offset = 0x03F0;
        break;
    default:
        g_Notify->DisplayError(stdstr_f("Unhandled CicChip(%d) in first DMA", g_Rom->CicChipID()).c_str());
        return;
    }
    g_MMU->UpdateMemoryValue32(0x80000000 + offset, rt);
}

void PeripheralInterfaceHandler::PI_DMA_READ()
{
    if (g_Debugger != NULL && HaveDebugger())
    {
        g_Debugger->PIDMAReadStarted();
    }

    // PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    uint32_t PI_RD_LEN = ((g_Reg->PI_RD_LEN_REG) & 0x00FFFFFFul) + 1;

    if ((PI_RD_LEN & 1) != 0)
    {
        PI_RD_LEN += 1;
    }

    if (g_Reg->PI_DRAM_ADDR_REG + PI_RD_LEN > g_MMU->RdramSize())
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("PI_DMA_READ not in Memory: %08X", g_Reg->PI_DRAM_ADDR_REG + PI_RD_LEN).c_str());
        }
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    // 64DD buffers write
    if (g_Reg->PI_CART_ADDR_REG >= 0x05000000 && g_Reg->PI_CART_ADDR_REG <= 0x050003FF)
    {
        // 64DD C2 sectors (don't care)
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_RD_LEN * 63) / 25, false);
        return;
    }

    if (g_Reg->PI_CART_ADDR_REG >= 0x05000400 && g_Reg->PI_CART_ADDR_REG <= 0x050004FF)
    {
        // 64DD user sector
        uint32_t i;
        uint8_t * RDRAM = g_MMU->Rdram();
        uint8_t * DISK = g_Disk->GetDiskAddressBuffer();
        for (i = 0; i < PI_RD_LEN_REG; i++)
        {
            *(DISK + (i ^ 3)) = *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3));
        }
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_RD_LEN_REG * 63) / 25, false);
        return;
    }

    if (g_Reg->PI_CART_ADDR_REG >= 0x05000580 && g_Reg->PI_CART_ADDR_REG <= 0x050005BF)
    {
        // 64DD MSEQ (don't care)
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    // Write ROM area (for 64DD conversion)
    if (g_Reg->PI_CART_ADDR_REG >= 0x10000000 && g_Reg->PI_CART_ADDR_REG <= 0x1FBFFFFF && g_Settings->LoadBool(Game_AllowROMWrites))
    {
        uint32_t i;
        uint8_t * ROM = g_Rom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();

        ProtectMemory(ROM, g_Rom->GetRomSize(), MEM_READWRITE);
        g_Reg->PI_CART_ADDR_REG -= 0x10000000;
        if (g_Reg->PI_CART_ADDR_REG + PI_RD_LEN_REG < g_Rom->GetRomSize())
        {
            for (i = 0; i < PI_RD_LEN_REG; i++)
            {
                *(ROM + ((g_Reg->PI_CART_ADDR_REG + i) ^ 3)) = *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3));
            }
        }
        else
        {
            uint32_t Len;
            Len = g_Rom->GetRomSize() - g_Reg->PI_CART_ADDR_REG;
            for (i = 0; i < Len; i++)
            {
                *(ROM + ((g_Reg->PI_CART_ADDR_REG + i) ^ 3)) = *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3));
            }
        }
        g_Reg->PI_CART_ADDR_REG += 0x10000000;

        if (!g_System->DmaUsed())
        {
            g_System->SetDmaUsed(true);
            OnFirstDMA();
        }
        if (g_Recompiler && g_System->bSMM_PIDMA())
        {
            g_Recompiler->ClearRecompCode_Phys(g_Reg->PI_DRAM_ADDR_REG, g_Reg->PI_WR_LEN_REG, CRecompiler::Remove_DMA);
        }

        ProtectMemory(ROM, g_Rom->GetRomSize(), MEM_READONLY);

        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    if (g_Reg->PI_CART_ADDR_REG >= 0x08000000 && g_Reg->PI_CART_ADDR_REG < 0x08088000)
    {
        if (m_Domain2Address2Handler.DMARead())
        {
            return;
        }
    }
    if (g_System->m_SaveUsing == SaveChip_FlashRam)
    {
        g_Notify->DisplayError(stdstr_f("**** FlashRAM DMA read address %08X ****", g_Reg->PI_CART_ADDR_REG).c_str());
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }
    if (HaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("PI_DMA_READ where are you DMAing to? : %08X", g_Reg->PI_CART_ADDR_REG).c_str());
    }
    g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
    g_Reg->MI_INTR_REG |= MI_INTR_PI;
    g_Reg->CheckInterrupts();
    return;
}

void PeripheralInterfaceHandler::PI_DMA_WRITE()
{
    if (g_Debugger != NULL && HaveDebugger())
    {
        g_Debugger->PIDMAWriteStarted();
    }

    // Rounding PI_WR_LEN up to the nearest even number fixes AI Shougi 3, Doraemon 3, etc.
    uint32_t PI_WR_LEN = ((g_Reg->PI_WR_LEN_REG) & 0x00FFFFFEul) + 2;
    uint32_t PI_CART_ADDR = !g_Settings->LoadBool(Game_UnalignedDMA) ? g_Reg->PI_CART_ADDR_REG & ~1 : g_Reg->PI_CART_ADDR_REG;

    g_Reg->PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    if (g_Reg->PI_DRAM_ADDR_REG + PI_WR_LEN > g_MMU->RdramSize())
    {
        if (ShowUnhandledMemory()) { g_Notify->DisplayError(stdstr_f("PI_DMA_WRITE not in memory: %08X", g_Reg->PI_DRAM_ADDR_REG + PI_WR_LEN).c_str()); }
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    // 64DD buffers read
    if (PI_CART_ADDR >= 0x05000000 && PI_CART_ADDR <= 0x050003FF)
    {
        // 64DD C2 sectors (just read 0)
        uint32_t i;
        uint8_t * RDRAM = g_MMU->Rdram();
        for (i = 0; i < PI_WR_LEN; i++)
        {
            *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
        }

        // Timer is needed for track read
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_WR_LEN * 63) / 25, false);
        return;
    }

    if (PI_CART_ADDR >= 0x05000400 && PI_CART_ADDR <= 0x050004FF)
    {
        // 64DD user sector
        uint32_t i;
        uint8_t * RDRAM = g_MMU->Rdram();
        uint8_t * DISK = g_Disk->GetDiskAddressBuffer();
        for (i = 0; i < PI_WR_LEN; i++)
        {
            *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(DISK + (i ^ 3));
        }

        // Timer is needed for track read
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_WR_LEN * 63) / 25, false);
        return;
    }

    if (PI_CART_ADDR >= 0x05000580 && PI_CART_ADDR <= 0x050005BF)
    {
        // 64DD MSEQ (don't care)
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    // 64DD IPL ROM
    if (PI_CART_ADDR >= 0x06000000 && PI_CART_ADDR <= 0x063FFFFF)
    {
        uint32_t i;

        uint8_t * ROM = g_DDRom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();
        PI_CART_ADDR -= 0x06000000;
        if (PI_CART_ADDR + PI_WR_LEN < g_DDRom->GetRomSize())
        {
            for (i = 0; i < PI_WR_LEN; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((PI_CART_ADDR + i) ^ 3));
            }
        }
        else if (PI_CART_ADDR >= g_DDRom->GetRomSize())
        {
            uint32_t cart = PI_CART_ADDR - g_DDRom->GetRomSize();
            while (cart >= g_DDRom->GetRomSize())
            {
                cart -= g_DDRom->GetRomSize();
            }
            for (i = 0; i < PI_WR_LEN; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((cart + i) ^ 3));
            }
        }
        else
        {
            uint32_t Len;
            Len = g_DDRom->GetRomSize() - PI_CART_ADDR;
            for (i = 0; i < Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((PI_CART_ADDR + i) ^ 3));
            }
            for (i = Len; i < PI_WR_LEN - Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
            }
        }
        PI_CART_ADDR += 0x06000000;

        if (!g_System->DmaUsed())
        {
            g_System->SetDmaUsed(true);
            OnFirstDMA();
        }
        if (g_Recompiler && g_System->bSMM_PIDMA())
        {
            g_Recompiler->ClearRecompCode_Phys(g_Reg->PI_DRAM_ADDR_REG, g_Reg->PI_WR_LEN_REG, CRecompiler::Remove_DMA);
        }
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN * 8.9) + 50);
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN * 8.9));
        return;
    }

    if (PI_CART_ADDR >= 0x08000000 && PI_CART_ADDR <= 0x08088000)
    {
        m_Domain2Address2Handler.DMAWrite();
        return;
    }

    if (PI_CART_ADDR >= 0x10000000 && PI_CART_ADDR <= 0x1FFFFFFF)
    {
        if (g_Recompiler && g_System->bSMM_PIDMA())
        {
            g_Recompiler->ClearRecompCode_Phys(g_Reg->PI_DRAM_ADDR_REG, g_Reg->PI_WR_LEN_REG, CRecompiler::Remove_DMA);
        }

        uint32_t i;

        uint8_t * ROM = g_Rom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();
        PI_CART_ADDR -= 0x10000000;
        if (PI_CART_ADDR + PI_WR_LEN < g_Rom->GetRomSize())
        {
            size_t alignment;
            RDRAM += g_Reg->PI_DRAM_ADDR_REG;
            ROM += PI_CART_ADDR;
            alignment = PI_WR_LEN | (size_t)RDRAM | (size_t)ROM;
            if ((alignment & 0x3) == 0)
            {
                for (i = 0; i < PI_WR_LEN; i += 4)
                {
                    *(uint32_t *)(RDRAM + i) = *(uint32_t *)(ROM + i);
                }
            }
            else if ((alignment & 1) == 0)
            {
                if ((PI_WR_LEN & 2) == 0)
                {
                    if (((size_t)RDRAM & 2) == 0)
                    {
                        for (i = 0; i < PI_WR_LEN; i += 4)
                        {
                            *(uint16_t *)(((size_t)RDRAM + i) + 2) = *(uint16_t *)(((size_t)ROM + i) - 2);
                            *(uint16_t *)(((size_t)RDRAM + i) + 0) = *(uint16_t *)(((size_t)ROM + i) + 4);
                        }
                    }
                    else
                    {
                        if (((size_t)ROM & 2) == 0)
                        {
                            for (i = 0; i < PI_WR_LEN; i += 4)
                            {
                                *(uint16_t *)(((size_t)RDRAM + i) - 2) = *(uint16_t *)(((size_t)ROM + i) + 2);
                                *(uint16_t *)(((size_t)RDRAM + i) + 4) = *(uint16_t *)(((size_t)ROM + i) + 0);
                            }
                        }
                        else
                        {
                            for (i = 0; i < PI_WR_LEN; i += 4)
                            {
                                *(uint16_t *)(((size_t)RDRAM + i) - 2) = *(uint16_t *)(((size_t)ROM + i) - 2);
                                *(uint16_t *)(((size_t)RDRAM + i) + 4) = *(uint16_t *)(((size_t)ROM + i) + 4);
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < PI_WR_LEN; i += 2)
                    {
                        *(uint16_t *)(((size_t)RDRAM + i) ^ 2) = *(uint16_t *)(((size_t)ROM + i) ^ 2);
                    }
                }
            }
            else
            {
                for (i = 0; i < PI_WR_LEN; i++)
                {
                    *(uint8_t *)(((size_t)RDRAM + i) ^ 3) = *(uint8_t *)(((size_t)ROM + i) ^ 3);
                }
            }
        }
        else if (PI_CART_ADDR >= g_Rom->GetRomSize())
        {
            uint32_t cart = PI_CART_ADDR - g_Rom->GetRomSize();
            while (cart >= g_Rom->GetRomSize())
            {
                cart -= g_Rom->GetRomSize();
            }
            for (i = 0; i < PI_WR_LEN; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((cart + i) ^ 3));
            }
        }
        else
        {
            uint32_t Len;
            Len = g_Rom->GetRomSize() - PI_CART_ADDR;
            for (i = 0; i < Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((PI_CART_ADDR + i) ^ 3));
            }
            for (i = Len; i < PI_WR_LEN - Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
            }
        }
        PI_CART_ADDR += 0x10000000;

        if (!g_System->DmaUsed())
        {
            g_System->SetDmaUsed(true);
            OnFirstDMA();
        }

        if (g_System->bRandomizeSIPIInterrupts())
        {
            g_SystemTimer->SetTimer(g_SystemTimer->PiTimer, PI_WR_LEN / 8 + (g_Random->next() % 0x40), false);
        }
        else
        {
            g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->CheckInterrupts();
        }
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN * 8.9) + 50);
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN * 8.9));
        return;
    }

    if (ShowUnhandledMemory())
    {
        g_Notify->DisplayError(stdstr_f("PI_DMA_WRITE not in ROM: %08X", PI_CART_ADDR).c_str());
    }
    g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
    g_Reg->MI_INTR_REG |= MI_INTR_PI;
    g_Reg->CheckInterrupts();
}