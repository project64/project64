#include <stdafx.h>

#include "DebugMMU.h"
#include <Common/MemoryManagement.h>

uint32_t* CDebugMMU::PAddrWordPtr(uint32_t paddr)
{
    if (g_MMU == NULL)
    {
        return NULL;
    }

    paddr = paddr & ~3;
    
    // RDRAM & DMEM/IMEM
    if ((paddr < g_MMU->RdramSize()) || 
        (paddr >= 0x04000000 && paddr <= 0x04001FFF))
    {
        return (uint32_t*)(g_MMU->Rdram() + paddr);
    }

    // 64DD buffer
    if (paddr >= 0x05000000 && paddr <= 0x050004FF) 
    {
        // todo
        return NULL;
    }

    // Cartridge Domain 1 (Address 1) (64DD IPL ROM)
    if (paddr >= 0x06000000 && paddr <= 0x06FFFFFF) 
    {
        uint32_t iplRomOffset = paddr - 0x06000000;

        if (g_DDRom != NULL && iplRomOffset < g_DDRom->GetRomSize())
        {
            return (uint32_t*)(g_MMU->Rdram() + paddr);
        }
        return NULL;
    }

    // Cartridge Domain 2 (Address 2) (SRAM/FlashRAM)
    if (paddr >= 0x08000000 && paddr < 0x08FFFFFF) 
    {
        // stored in a file
        return NULL;
    }

    // Cartridge ROM
    if (paddr >= 0x10000000 && paddr <= 0x15FFFFFF) 
    {
        uint32_t cartRomOffset = paddr - 0x10000000;
        if (g_Rom != NULL && cartRomOffset < g_Rom->GetRomSize())
        {
            return (uint32_t*)(g_Rom->GetRomAddress() + cartRomOffset);
        }
        return false;
    }

    // PIF ROM
    if (paddr >= 0x1FC00000 && paddr <= 0x1FC007BF)
    {
        return NULL;
    }

    // PIF RAM
    if (paddr >= 0x1FC007C0 && paddr <= 0x1FC007FF) 
    {
        uint32_t pifRamOffset = paddr - 0x1FC007C0;
        return (uint32_t*)(g_MMU->PifRam() + pifRamOffset);
    }

    switch (paddr)
    {
    case 0x03F00000: return &g_Reg->RDRAM_CONFIG_REG;
    case 0x03F00004: return &g_Reg->RDRAM_DEVICE_ID_REG;
    case 0x03F00008: return &g_Reg->RDRAM_DELAY_REG;
    case 0x03F0000C: return &g_Reg->RDRAM_MODE_REG;
    case 0x03F00010: return &g_Reg->RDRAM_REF_INTERVAL_REG;
    case 0x03F00014: return &g_Reg->RDRAM_REF_ROW_REG;
    case 0x03F00018: return &g_Reg->RDRAM_RAS_INTERVAL_REG;
    case 0x03F0001C: return &g_Reg->RDRAM_MIN_INTERVAL_REG;
    case 0x03F00020: return &g_Reg->RDRAM_ADDR_SELECT_REG;
    case 0x03F00024: return &g_Reg->RDRAM_DEVICE_MANUF_REG;
    case 0x04040010: return &g_Reg->SP_STATUS_REG;
    case 0x04040014: return &g_Reg->SP_DMA_FULL_REG;
    case 0x04040018: return &g_Reg->SP_DMA_BUSY_REG;
    case 0x0404001C: return &g_Reg->SP_SEMAPHORE_REG;
    case 0x04080000: return &g_Reg->SP_PC_REG;
    case 0x0410000C: return &g_Reg->DPC_STATUS_REG;
    case 0x04100010: return &g_Reg->DPC_CLOCK_REG;
    case 0x04100014: return &g_Reg->DPC_BUFBUSY_REG;
    case 0x04100018: return &g_Reg->DPC_PIPEBUSY_REG;
    case 0x0410001C: return &g_Reg->DPC_TMEM_REG;
    case 0x04300000: return &g_Reg->MI_MODE_REG;
    case 0x04300004: return &g_Reg->MI_VERSION_REG;
    case 0x04300008: return &g_Reg->MI_INTR_REG;
    case 0x0430000C: return &g_Reg->MI_INTR_MASK_REG;
    case 0x04400000: return &g_Reg->VI_STATUS_REG;
    case 0x04400004: return &g_Reg->VI_ORIGIN_REG;
    case 0x04400008: return &g_Reg->VI_WIDTH_REG;
    case 0x0440000C: return &g_Reg->VI_INTR_REG;
    case 0x04400010: return &g_Reg->VI_V_CURRENT_LINE_REG;
    case 0x04400014: return &g_Reg->VI_BURST_REG;
    case 0x04400018: return &g_Reg->VI_V_SYNC_REG;
    case 0x0440001C: return &g_Reg->VI_H_SYNC_REG;
    case 0x04400020: return &g_Reg->VI_LEAP_REG;
    case 0x04400024: return &g_Reg->VI_H_START_REG;
    case 0x04400028: return &g_Reg->VI_V_START_REG;
    case 0x0440002C: return &g_Reg->VI_V_BURST_REG;
    case 0x04400030: return &g_Reg->VI_X_SCALE_REG;
    case 0x04400034: return &g_Reg->VI_Y_SCALE_REG;
    case 0x04600000: return &g_Reg->PI_DRAM_ADDR_REG;
    case 0x04600004: return &g_Reg->PI_CART_ADDR_REG;
    case 0x04600008: return &g_Reg->PI_RD_LEN_REG;
    case 0x0460000C: return &g_Reg->PI_WR_LEN_REG;
    case 0x04600010: return &g_Reg->PI_STATUS_REG;
    case 0x04600014: return &g_Reg->PI_DOMAIN1_REG;
    case 0x04600018: return &g_Reg->PI_BSD_DOM1_PWD_REG;
    case 0x0460001C: return &g_Reg->PI_BSD_DOM1_PGS_REG;
    case 0x04600020: return &g_Reg->PI_BSD_DOM1_RLS_REG;
    case 0x04600024: return &g_Reg->PI_DOMAIN2_REG;
    case 0x04600028: return &g_Reg->PI_BSD_DOM2_PWD_REG;
    case 0x0460002C: return &g_Reg->PI_BSD_DOM2_PGS_REG;
    case 0x04600030: return &g_Reg->PI_BSD_DOM2_RLS_REG;
    case 0x04700000: return &g_Reg->RI_MODE_REG;
    case 0x04700004: return &g_Reg->RI_CONFIG_REG;
    case 0x04700008: return &g_Reg->RI_CURRENT_LOAD_REG;
    case 0x0470000C: return &g_Reg->RI_SELECT_REG;
    case 0x04700010: return &g_Reg->RI_REFRESH_REG;
    case 0x04700014: return &g_Reg->RI_LATENCY_REG;
    case 0x04700018: return &g_Reg->RI_RERROR_REG;
    case 0x0470001C: return &g_Reg->RI_WERROR_REG;
    case 0x04800018: return &g_Reg->SI_STATUS_REG;
    case 0x05000500: return &g_Reg->ASIC_DATA;
    case 0x05000504: return &g_Reg->ASIC_MISC_REG;
    case 0x05000508: return &g_Reg->ASIC_STATUS;
    case 0x0500050C: return &g_Reg->ASIC_CUR_TK;
    case 0x05000510: return &g_Reg->ASIC_BM_STATUS;
    case 0x05000514: return &g_Reg->ASIC_ERR_SECTOR;
    case 0x05000518: return &g_Reg->ASIC_SEQ_STATUS;
    case 0x0500051C: return &g_Reg->ASIC_CUR_SECTOR;
    case 0x05000520: return &g_Reg->ASIC_HARD_RESET;
    case 0x05000524: return &g_Reg->ASIC_C1_S0;
    case 0x05000528: return &g_Reg->ASIC_HOST_SECBYTE;
    case 0x0500052C: return &g_Reg->ASIC_C1_S2;
    case 0x05000530: return &g_Reg->ASIC_SEC_BYTE;
    case 0x05000534: return &g_Reg->ASIC_C1_S4;
    case 0x05000538: return &g_Reg->ASIC_C1_S6;
    case 0x0500053C: return &g_Reg->ASIC_CUR_ADDR;
    case 0x05000540: return &g_Reg->ASIC_ID_REG;
    case 0x05000544: return &g_Reg->ASIC_TEST_REG;
    case 0x05000548: return &g_Reg->ASIC_TEST_PIN_SEL;
    }

    return NULL;
}

bool CDebugMMU::DebugLW_PAddr(uint32_t paddr, uint32_t& value)
{
    if (g_MMU == NULL)
    {
        return false;
    }

    uint32_t* ptr = PAddrWordPtr(paddr);

    if (ptr != NULL)
    {
        value = *ptr;
        return true;
    }

    if (paddr >= 0x08000000 && paddr < 0x08FFFFFF) // Cartridge Domain 2 (Address 2)
    {
        uint32_t saveOffset = paddr & 0x000FFFFF;
    
        if (g_System->m_SaveUsing == SaveChip_Sram && saveOffset <= 0x7FFF) // sram
        {
            uint8_t tmp[4] = "";
            CSram *sram = g_MMU->GetSram();
            sram->DmaFromSram(tmp, paddr - 0x08000000, 4);
            value = tmp[3] << 24 | tmp[2] << 16 | tmp[1] << 8 | tmp[0];
            return true;
        }
        else if (g_System->m_SaveUsing == SaveChip_FlashRam && saveOffset == 0) // flash ram status
        {
            CFlashram* flashRam = g_MMU->GetFlashram();
            value = flashRam->ReadFromFlashStatus(0x08000000);
            return true;
        }
    }

    if (paddr == 0x04500004)
    {
        if (g_System->bFixedAudio())
        {
            value = g_Audio->GetLength();
        }
        else
        {
            CAudioPlugin* audioPlg = g_Plugins->Audio();
            value = (audioPlg->AiReadLength != NULL) ? audioPlg->AiReadLength() : 0;
        }
        return true;
    }

    if (paddr == 0x0450000C)
    {
        value = g_System->bFixedAudio() ? g_Audio->GetStatus() : g_Reg->AI_STATUS_REG;
        return true;
    }

    return false;
}

bool CDebugMMU::DebugLW_VAddr(uint32_t vaddr, uint32_t& value)
{
    if (vaddr <= 0x7FFFFFFF || vaddr >= 0xC0000000) // KUSEG, KSEG2 (TLB)
    {
        if (g_MMU == NULL)
        {
            return false;
        }

        return g_MMU->LW_VAddr(vaddr, value);
    }

    uint32_t paddr = vaddr & 0x1FFFFFFF;
    return DebugLW_PAddr(paddr, value);
}

bool CDebugMMU::DebugLB_PAddr(uint32_t vaddr, uint8_t& value)
{
    uint32_t word;
    if (!DebugLW_PAddr(vaddr & ~3, word))
    {
        return false;
    }
    value = (word >> (24 - (vaddr & 3) * 8)) & 0xFF;
    return true;
}

bool CDebugMMU::DebugLB_VAddr(uint32_t vaddr, uint8_t& value)
{
    uint32_t word;
    if (!DebugLW_VAddr(vaddr & ~3, word))
    {
        return false;
    }
    value = (word >> (24 - (vaddr & 3) * 8)) & 0xFF;
    return true;
}

bool CDebugMMU::DebugSB_PAddr(uint32_t paddr, uint8_t value)
{
    bool bWriteToRom = false;

    if (paddr >= 0x10000000 && paddr <= 0x1FBFFFFF)
    {
        uint32_t romOffset = paddr - 0x10000000;
        if (romOffset > g_Rom->GetRomSize())
        {
            return false;
        }
        bWriteToRom = true;
    }

    int nbyte = 3 - (paddr & 3);
    uint8_t* ptr = (uint8_t*)PAddrWordPtr(paddr & ~3);

    if (ptr == NULL)
    {
        return false;
    }

    if (bWriteToRom)
    {
        ProtectMemory(g_Rom->GetRomAddress(), g_Rom->GetRomSize(), MEM_READWRITE);
    }

    ptr[nbyte] = value;

    if (bWriteToRom)
    {
        ProtectMemory(g_Rom->GetRomAddress(), g_Rom->GetRomSize(), MEM_READONLY);
    }
    return true;
}

bool CDebugMMU::DebugSB_VAddr(uint32_t vaddr, uint8_t value)
{
    if (vaddr <= 0x7FFFFFFF || vaddr >= 0xC0000000) // KUSEG, KSEG2 (TLB)
    {
        if (g_MMU == NULL)
        {
            return false;
        }

        return g_MMU->SB_VAddr(vaddr, value);
    }

    uint32_t paddr = vaddr & 0x1FFFFFFF;
    return DebugSB_PAddr(paddr, value);
}
