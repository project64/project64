/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "Dma.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64RomClass.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/N64Class.h>
#include <Windows.h>

CDMA::CDMA(CFlashram & FlashRam, CSram & Sram) :
m_FlashRam(FlashRam),
m_Sram(Sram)
{
}

void CDMA::OnFirstDMA()
{
    int16_t offset;
    const uint32_t base = 0x00000000;
    const uint32_t rt = g_MMU->RdramSize();

    switch (g_Rom->CicChipID())
    {
    case CIC_NUS_6101:  offset = +0x0318; break;
    case CIC_NUS_5167:  offset = +0x0318; break;
    case CIC_UNKNOWN:
    case CIC_NUS_6102:  offset = +0x0318; break;
    case CIC_NUS_6103:  offset = +0x0318; break;
    case CIC_NUS_6105:  offset = +0x03F0; break;
    case CIC_NUS_6106:  offset = +0x0318; break;
    default:
        g_Notify->DisplayError(stdstr_f("Unhandled CicChip(%d) in first DMA", g_Rom->CicChipID()).c_str());
        return;
    }
    g_MMU->SW_PAddr(base + offset, rt);
}

void CDMA::PI_DMA_READ()
{
    //	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    uint32_t PI_RD_LEN_REG = ((g_Reg->PI_RD_LEN_REG) & 0x00FFFFFFul) + 1;

    if ((PI_RD_LEN_REG & 1) != 0)
    {
        PI_RD_LEN_REG += 1;
    }

    if (g_Reg->PI_DRAM_ADDR_REG + PI_RD_LEN_REG > g_MMU->RdramSize())
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("PI_DMA_READ not in Memory: %08X", g_Reg->PI_DRAM_ADDR_REG + PI_RD_LEN_REG).c_str());
        }
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    //Write ROM Area (for 64DD Convert)
    if (g_Reg->PI_CART_ADDR_REG >= 0x10000000 && g_Reg->PI_CART_ADDR_REG <= 0x1FBFFFFF && g_Settings->LoadBool(Game_AllowROMWrites))
    {
        uint32_t i;
        uint8_t * ROM = g_Rom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();

        DWORD OldProtect;
        VirtualProtect(ROM, g_Rom->GetRomSize(), PAGE_READWRITE, &OldProtect);

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

        VirtualProtect(ROM, g_Rom->GetRomSize(), PAGE_READONLY, &OldProtect);

        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    if (g_Reg->PI_CART_ADDR_REG >= 0x08000000 && g_Reg->PI_CART_ADDR_REG <= 0x08010000)
    {
        if (g_System->m_SaveUsing == SaveChip_Auto)
        {
            g_System->m_SaveUsing = SaveChip_Sram;
        }
        if (g_System->m_SaveUsing == SaveChip_Sram)
        {
            m_Sram.DmaToSram(
                g_MMU->Rdram() + g_Reg->PI_DRAM_ADDR_REG,
                g_Reg->PI_CART_ADDR_REG - 0x08000000,
                PI_RD_LEN_REG
                );
            g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->CheckInterrupts();
            return;
        }
        if (g_System->m_SaveUsing == SaveChip_FlashRam)
        {
            m_FlashRam.DmaToFlashram(
                g_MMU->Rdram() + g_Reg->PI_DRAM_ADDR_REG,
                g_Reg->PI_CART_ADDR_REG - 0x08000000,
                PI_RD_LEN_REG
                );
            g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->CheckInterrupts();
            return;
        }
    }
    if (g_System->m_SaveUsing == SaveChip_FlashRam)
    {
        g_Notify->DisplayError(stdstr_f("**** FLashRam DMA Read address %08X *****", g_Reg->PI_CART_ADDR_REG).c_str());
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }
    if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("PI_DMA_READ where are you dmaing to ? : %08X", g_Reg->PI_CART_ADDR_REG).c_str());
    }
    g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
    g_Reg->MI_INTR_REG |= MI_INTR_PI;
    g_Reg->CheckInterrupts();
    return;
}

void CDMA::PI_DMA_WRITE()
{
    uint32_t PI_WR_LEN_REG = ((g_Reg->PI_WR_LEN_REG) & 0x00FFFFFFul) + 1;

    if ((PI_WR_LEN_REG & 1) != 0)
    {
        PI_WR_LEN_REG += 1; /* fixes AI Shougi 3, Doraemon 3, etc. */
    }

    g_Reg->PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
    if (g_Reg->PI_DRAM_ADDR_REG + PI_WR_LEN_REG > g_MMU->RdramSize())
    {
        if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { g_Notify->DisplayError(stdstr_f("PI_DMA_WRITE not in Memory: %08X", g_Reg->PI_DRAM_ADDR_REG + PI_WR_LEN_REG).c_str()); }
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        return;
    }

    if (g_Reg->PI_CART_ADDR_REG >= 0x08000000 && g_Reg->PI_CART_ADDR_REG <= 0x08088000)
    {
        if (g_System->m_SaveUsing == SaveChip_Auto)
        {
            g_System->m_SaveUsing = SaveChip_Sram;
        }
        if (g_System->m_SaveUsing == SaveChip_Sram)
        {
            m_Sram.DmaFromSram(
                g_MMU->Rdram() + g_Reg->PI_DRAM_ADDR_REG,
                g_Reg->PI_CART_ADDR_REG - 0x08000000,
                PI_WR_LEN_REG
                );
            g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->CheckInterrupts();
            return;
        }
        if (g_System->m_SaveUsing == SaveChip_FlashRam)
        {
            m_FlashRam.DmaFromFlashram(
                g_MMU->Rdram() + g_Reg->PI_DRAM_ADDR_REG,
                g_Reg->PI_CART_ADDR_REG - 0x08000000,
                PI_WR_LEN_REG
                );
            g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->CheckInterrupts();
        }
        return;
    }

    if (g_Reg->PI_CART_ADDR_REG >= 0x10000000 && g_Reg->PI_CART_ADDR_REG <= 0x1FFFFFFF)
    {
        uint32_t i;

#ifdef legacycode
#ifdef ROM_IN_MAPSPACE
        if (WrittenToRom)
        {
            uint32_t OldProtect;
            VirtualProtect(ROM,m_RomFileSize,PAGE_READONLY, &OldProtect);
        }
#endif
#endif

        uint8_t * ROM = g_Rom->GetRomAddress();
        uint8_t * RDRAM = g_MMU->Rdram();
        g_Reg->PI_CART_ADDR_REG -= 0x10000000;
        if (g_Reg->PI_CART_ADDR_REG + PI_WR_LEN_REG < g_Rom->GetRomSize())
        {
            for (i = 0; i < PI_WR_LEN_REG; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((g_Reg->PI_CART_ADDR_REG + i) ^ 3));
            }
        }
        else if (g_Reg->PI_CART_ADDR_REG >= g_Rom->GetRomSize())
        {
            uint32_t cart = g_Reg->PI_CART_ADDR_REG - g_Rom->GetRomSize();
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
            Len = g_Rom->GetRomSize() - g_Reg->PI_CART_ADDR_REG;
            for (i = 0; i < Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = *(ROM + ((g_Reg->PI_CART_ADDR_REG + i) ^ 3));
            }
            for (i = Len; i < PI_WR_LEN_REG - Len; i++)
            {
                *(RDRAM + ((g_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) = 0;
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
        g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        g_Reg->MI_INTR_REG |= MI_INTR_PI;
        g_Reg->CheckInterrupts();
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN_REG * 8.9) + 50);
        //ChangeTimer(PiTimer,(int32_t)(PI_WR_LEN_REG * 8.9));
        return;
    }

    if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
    {
        g_Notify->DisplayError(stdstr_f("PI_DMA_WRITE not in ROM: %08X", g_Reg->PI_CART_ADDR_REG).c_str());
    }
    g_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
    g_Reg->MI_INTR_REG |= MI_INTR_PI;
    g_Reg->CheckInterrupts();
}

void CDMA::SP_DMA_READ()
{
    g_Reg->SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

    if (g_Reg->SP_DRAM_ADDR_REG > g_MMU->RdramSize())
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f(__FUNCTION__ "\nSP_DRAM_ADDR_REG not in RDRam space : % 08X", g_Reg->SP_DRAM_ADDR_REG).c_str());
        }
        g_Reg->SP_DMA_BUSY_REG = 0;
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;
        return;
    }

    if (g_Reg->SP_RD_LEN_REG + 1 + (g_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000)
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(__FUNCTION__ "\nCould not fit copy in memory segment");
        }
        return;
    }

    if ((g_Reg->SP_MEM_ADDR_REG & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if ((g_Reg->SP_DRAM_ADDR_REG & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (((g_Reg->SP_RD_LEN_REG + 1) & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    memcpy(g_MMU->Dmem() + (g_Reg->SP_MEM_ADDR_REG & 0x1FFF), g_MMU->Rdram() + g_Reg->SP_DRAM_ADDR_REG,
        g_Reg->SP_RD_LEN_REG + 1);

    g_Reg->SP_DMA_BUSY_REG = 0;
    g_Reg->SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;
}

void CDMA::SP_DMA_WRITE()
{
    if (g_Reg->SP_DRAM_ADDR_REG > g_MMU->RdramSize())
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f(__FUNCTION__ "\nSP_DRAM_ADDR_REG not in RDRam space : % 08X", g_Reg->SP_DRAM_ADDR_REG).c_str());
        }
        return;
    }

    if (g_Reg->SP_WR_LEN_REG + 1 + (g_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000)
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError("SP DMA WRITE\ncould not fit copy in memory segement");
        }
        return;
    }

    if ((g_Reg->SP_MEM_ADDR_REG & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if ((g_Reg->SP_DRAM_ADDR_REG & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (((g_Reg->SP_WR_LEN_REG + 1) & 3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    memcpy(g_MMU->Rdram() + g_Reg->SP_DRAM_ADDR_REG, g_MMU->Dmem() + (g_Reg->SP_MEM_ADDR_REG & 0x1FFF),
        g_Reg->SP_WR_LEN_REG + 1);

    g_Reg->SP_DMA_BUSY_REG = 0;
    g_Reg->SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;
}