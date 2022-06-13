#include "stdafx.h"
#include <Common/MemoryManagement.h>

#include <Project64-core/N64System/Mips/Dma.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/N64System/N64Disk.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/Debugger.h>

CDMA::CDMA(CartridgeDomain2Address2Handler & Domain2Address2Handler) :
    m_Domain2Address2Handler(Domain2Address2Handler)
{
}

void CDMA::OnFirstDMA()
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

void CDMA::PI_DMA_READ()
{
    if (g_Debugger != NULL && HaveDebugger())
    {
        g_Debugger->PIDMAReadStarted();
    }

    // PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    uint32_t PI_RD_LEN_REG = ((g_Reg->PI_RD_LEN_REG) & 0x00FFFFFFul) + 1;

    if ((PI_RD_LEN_REG & 1) != 0)
    {
        PI_RD_LEN_REG += 1;
    }

    if (g_Reg->PI_DRAM_ADDR_REG + PI_RD_LEN_REG > g_MMU->RdramSize())
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("PI_DMA_READ not in Memory: %08X", g_Reg->PI_DRAM_ADDR_REG + PI_RD_LEN_REG).c_str());
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
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_RD_LEN_REG * 63) / 25, false);
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

void CDMA::PI_DMA_WRITE()
{
    if (g_Debugger != NULL && HaveDebugger())
    {
        g_Debugger->PIDMAWriteStarted();
    }

    // Rounding PI_WR_LEN_REG up to the nearest even number fixes AI Shougi 3, Doraemon 3, etc.
    uint32_t PI_WR_LEN_REG = ((g_Reg->PI_WR_LEN_REG) & 0x00FFFFFEul) + 2;
    uint32_t PI_CART_ADDR_REG = !g_Settings->LoadBool(Game_UnalignedDMA) ? g_Reg->PI_CART_ADDR_REG & ~1 : g_Reg->PI_CART_ADDR_REG;

    g_Reg->PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    if (g_Reg->PI_DRAM_ADDR_REG + PI_WR_LEN_REG > g_MMU->RdramSize())
    {
        if (ShowUnhandledMemory()) { g_Notify->DisplayError(stdstr_f("PI_DMA_WRITE not in memory: %08X", g_Reg->PI_DRAM_ADDR_REG + PI_WR_LEN_REG).c_str()); }
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    // 64DD buffers read
    if (PI_CART_ADDR_REG >= 0x05000000 && PI_CART_ADDR_REG <= 0x050003FF)
    {
        // 64DD C2 sectors (just read 0)
        uint32_t i;
        uint8_t * RDRAM = g_MMU->Rdram();
        for (i = 0; i < PI_WR_LEN_REG; i++)
        {
            *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
        }

        // Timer is needed for track read
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_WR_LEN_REG * 63) / 25, false);
        return;
    }

    if (PI_CART_ADDR_REG >= 0x05000400 && PI_CART_ADDR_REG <= 0x050004FF)
    {
        // 64DD user sector
        uint32_t i;
        uint8_t * RDRAM = g_MMU->Rdram();
        uint8_t * DISK = g_Disk->GetDiskAddressBuffer();
        for (i = 0; i < PI_WR_LEN_REG; i++)
        {
            *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(DISK + (i ^ 3));
        }

        // Timer is needed for track read
        g_SystemTimer->SetTimer(g_SystemTimer->DDPiTimer, (PI_WR_LEN_REG * 63) / 25, false);
        return;
    }

    if (PI_CART_ADDR_REG >= 0x05000580 && PI_CART_ADDR_REG <= 0x050005BF)
    {
        // 64DD MSEQ (don't care)
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    // 64DD IPL ROM
    if (PI_CART_ADDR_REG >= 0x06000000 && PI_CART_ADDR_REG <= 0x063FFFFF)
    {
        uint32_t i;

        uint8_t * ROM = g_DDRom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();
        PI_CART_ADDR_REG -= 0x06000000;
        if (PI_CART_ADDR_REG + PI_WR_LEN_REG < g_DDRom->GetRomSize())
        {
            for (i = 0; i < PI_WR_LEN_REG; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((PI_CART_ADDR_REG + i) ^ 3));
            }
        }
        else if (PI_CART_ADDR_REG >= g_DDRom->GetRomSize())
        {
            uint32_t cart = PI_CART_ADDR_REG - g_DDRom->GetRomSize();
            while (cart >= g_DDRom->GetRomSize())
            {
                cart -= g_DDRom->GetRomSize();
            }
            for (i = 0; i < PI_WR_LEN_REG; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((cart + i) ^ 3));
            }
        }
        else
        {
            uint32_t Len;
            Len = g_DDRom->GetRomSize() - PI_CART_ADDR_REG;
            for (i = 0; i < Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((PI_CART_ADDR_REG + i) ^ 3));
            }
            for (i = Len; i < PI_WR_LEN_REG - Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
            }
        }
        PI_CART_ADDR_REG += 0x06000000;

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
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN_REG * 8.9) + 50);
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN_REG * 8.9));
        return;
    }

    if (PI_CART_ADDR_REG >= 0x08000000 && PI_CART_ADDR_REG <= 0x08088000)
    {
        m_Domain2Address2Handler.DMAWrite();
        return;
    }

    if (PI_CART_ADDR_REG >= 0x10000000 && PI_CART_ADDR_REG <= 0x1FFFFFFF)
    {
        if (g_Recompiler && g_System->bSMM_PIDMA())
        {
            g_Recompiler->ClearRecompCode_Phys(g_Reg->PI_DRAM_ADDR_REG, g_Reg->PI_WR_LEN_REG, CRecompiler::Remove_DMA);
        }

        uint32_t i;

        uint8_t * ROM = g_Rom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();
        PI_CART_ADDR_REG -= 0x10000000;
        if (PI_CART_ADDR_REG + PI_WR_LEN_REG < g_Rom->GetRomSize())
        {
            size_t alignment;
            RDRAM += g_Reg->PI_DRAM_ADDR_REG;
            ROM += PI_CART_ADDR_REG;
            alignment = PI_WR_LEN_REG | (size_t)RDRAM | (size_t)ROM;
            if ((alignment & 0x3) == 0)
            {
                for (i = 0; i < PI_WR_LEN_REG; i += 4)
                {
                    *(uint32_t *)(RDRAM + i) = *(uint32_t *)(ROM + i);
                }
            }
            else if ((alignment & 1) == 0)
            {
                if ((PI_WR_LEN_REG & 2) == 0)
                {
                    if (((size_t)RDRAM & 2) == 0)
                    {
                        for (i = 0; i < PI_WR_LEN_REG; i += 4)
                        {
                            *(uint16_t *)(((size_t)RDRAM + i) + 2) = *(uint16_t *)(((size_t)ROM + i) - 2);
                            *(uint16_t *)(((size_t)RDRAM + i) + 0) = *(uint16_t *)(((size_t)ROM + i) + 4);
                        }
                    }
                    else
                    {
                        if (((size_t)ROM & 2) == 0)
                        {
                            for (i = 0; i < PI_WR_LEN_REG; i += 4)
                            {
                                *(uint16_t *)(((size_t)RDRAM + i) - 2) = *(uint16_t *)(((size_t)ROM + i) + 2);
                                *(uint16_t *)(((size_t)RDRAM + i) + 4) = *(uint16_t *)(((size_t)ROM + i) + 0);
                            }
                        }
                        else
                        {
                            for (i = 0; i < PI_WR_LEN_REG; i += 4)
                            {
                                *(uint16_t *)(((size_t)RDRAM + i) - 2) = *(uint16_t *)(((size_t)ROM + i) - 2);
                                *(uint16_t *)(((size_t)RDRAM + i) + 4) = *(uint16_t *)(((size_t)ROM + i) + 4);
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < PI_WR_LEN_REG; i += 2)
                    {
                        *(uint16_t *)(((size_t)RDRAM + i) ^ 2) = *(uint16_t *)(((size_t)ROM + i) ^ 2);
                    }
                }
            }
            else
            {
                for (i = 0; i < PI_WR_LEN_REG; i++)
                {
                    *(uint8_t *)(((size_t)RDRAM + i) ^ 3) = *(uint8_t *)(((size_t)ROM + i) ^ 3);
                }
            }
        }
        else if (PI_CART_ADDR_REG >= g_Rom->GetRomSize())
        {
            uint32_t cart = PI_CART_ADDR_REG - g_Rom->GetRomSize();
            while (cart >= g_Rom->GetRomSize())
            {
                cart -= g_Rom->GetRomSize();
            }
            for (i = 0; i < PI_WR_LEN_REG; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((cart + i) ^ 3));
            }
        }
        else
        {
            uint32_t Len;
            Len = g_Rom->GetRomSize() - PI_CART_ADDR_REG;
            for (i = 0; i < Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((PI_CART_ADDR_REG + i) ^ 3));
            }
            for (i = Len; i < PI_WR_LEN_REG - Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
            }
        }
        PI_CART_ADDR_REG += 0x10000000;

        if (!g_System->DmaUsed())
        {
            g_System->SetDmaUsed(true);
            OnFirstDMA();
        }

        if(g_System->bRandomizeSIPIInterrupts())
        {
            g_SystemTimer->SetTimer(g_SystemTimer->PiTimer, PI_WR_LEN_REG / 8 + (g_Random->next() % 0x40), false);
        }
        else
        {
            g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->CheckInterrupts();
        }
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN_REG * 8.9) + 50);
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN_REG * 8.9));
        return;
    }

    if (ShowUnhandledMemory())
    {
        g_Notify->DisplayError(stdstr_f("PI_DMA_WRITE not in ROM: %08X", PI_CART_ADDR_REG).c_str());
    }
    g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
    g_Reg->MI_INTR_REG |= MI_INTR_PI;
    g_Reg->CheckInterrupts();
}