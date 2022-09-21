#include "stdafx.h"

#include "DebugMMU.h"
#include <Common/MemoryManagement.h>
#include <Project64-core/N64System/N64Disk.h>

#define PJMEM_CARTROM 1

uint8_t * CDebugMMU::GetPhysicalPtr(uint32_t paddr, WORD * flags)
{
    if (g_MMU == nullptr)
    {
        return nullptr;
    }

    uint8_t * ptr = nullptr;
    int nbyte = paddr & 3;
    paddr = paddr & ~3;

    bool bBigEndian = false;
    bool bCartRom = false;

    if (paddr < g_MMU->RdramSize())
    {
        ptr = (uint8_t *)(g_MMU->Rdram() + paddr);
    }
    else if (paddr >= 0x04000000 && paddr <= 0x04000FFF)
    {
        ptr = (uint8_t *)(g_MMU->Dmem() + paddr - 0x04000000);
    }
    else if (paddr >= 0x04001000 && paddr <= 0x04001FFF)
    {
        ptr = (uint8_t *)(g_MMU->Imem() + paddr - 0x04001000);
    }
    else if (paddr >= 0x05000000 && paddr <= 0x050004FF) // 64DD buffer
    {
        // todo
    }
    else if (paddr >= 0x06000000 && paddr <= 0x06FFFFFF) // Cartridge domain 1 (address 1) (64DD IPL ROM)
    {
        uint32_t iplRomOffset = paddr - 0x06000000;

        if (g_DDRom != nullptr && iplRomOffset < g_DDRom->GetRomSize())
        {
            ptr = (uint8_t *)(g_MMU->Rdram() + paddr);
        }
    }
    else if (paddr >= 0x10000000 && paddr <= 0x1FBFFFFF) // Cartridge ROM
    {
        uint32_t cartRomOffset = paddr - 0x10000000;
        if (g_Rom != nullptr && cartRomOffset < g_Rom->GetRomSize())
        {
            ptr = (uint8_t *)(g_Rom->GetRomAddress() + cartRomOffset);
            bCartRom = true;
        }
    }
    else if (paddr >= 0x1FC007C0 && paddr <= 0x1FC007FF) // PIF RAM
    {
        uint32_t pifRamOffset = paddr - 0x1FC007C0;
        ptr = (uint8_t *)(g_MMU->PifRam() + pifRamOffset);
        bBigEndian = true;
    }
    else
    {
        // Note: write-only registers are excluded
        switch (paddr)
        {
        case 0x03F00000: ptr = (uint8_t *)&g_Reg->RDRAM_CONFIG_REG; break;
        case 0x03F00004: ptr = (uint8_t *)&g_Reg->RDRAM_DEVICE_ID_REG; break;
        case 0x03F00008: ptr = (uint8_t *)&g_Reg->RDRAM_DELAY_REG; break;
        case 0x03F0000C: ptr = (uint8_t *)&g_Reg->RDRAM_MODE_REG; break;
        case 0x03F00010: ptr = (uint8_t *)&g_Reg->RDRAM_REF_INTERVAL_REG; break;
        case 0x03F00014: ptr = (uint8_t *)&g_Reg->RDRAM_REF_ROW_REG; break;
        case 0x03F00018: ptr = (uint8_t *)&g_Reg->RDRAM_RAS_INTERVAL_REG; break;
        case 0x03F0001C: ptr = (uint8_t *)&g_Reg->RDRAM_MIN_INTERVAL_REG; break;
        case 0x03F00020: ptr = (uint8_t *)&g_Reg->RDRAM_ADDR_SELECT_REG; break;
        case 0x03F00024: ptr = (uint8_t *)&g_Reg->RDRAM_DEVICE_MANUF_REG; break;
        case 0x04040010: ptr = (uint8_t *)&g_Reg->SP_STATUS_REG; break;
        case 0x04040014: ptr = (uint8_t *)&g_Reg->SP_DMA_FULL_REG; break;
        case 0x04040018: ptr = (uint8_t *)&g_Reg->SP_DMA_BUSY_REG; break;
        case 0x0404001C: ptr = (uint8_t *)&g_Reg->SP_SEMAPHORE_REG; break;
        case 0x04080000: ptr = (uint8_t *)&g_Reg->SP_PC_REG; break;
        case 0x0410000C: ptr = (uint8_t *)&g_Reg->DPC_STATUS_REG; break;
        case 0x04100010: ptr = (uint8_t *)&g_Reg->DPC_CLOCK_REG; break;
        case 0x04100014: ptr = (uint8_t *)&g_Reg->DPC_BUFBUSY_REG; break;
        case 0x04100018: ptr = (uint8_t *)&g_Reg->DPC_PIPEBUSY_REG; break;
        case 0x0410001C: ptr = (uint8_t *)&g_Reg->DPC_TMEM_REG; break;
        case 0x04300000: ptr = (uint8_t *)&g_Reg->MI_MODE_REG; break;
        case 0x04300004: ptr = (uint8_t *)&g_Reg->MI_VERSION_REG; break;
        case 0x04300008: ptr = (uint8_t *)&g_Reg->MI_INTR_REG; break;
        case 0x0430000C: ptr = (uint8_t *)&g_Reg->MI_INTR_MASK_REG; break;
        case 0x04400000: ptr = (uint8_t *)&g_Reg->VI_STATUS_REG; break;
        case 0x04400004: ptr = (uint8_t *)&g_Reg->VI_ORIGIN_REG; break;
        case 0x04400008: ptr = (uint8_t *)&g_Reg->VI_WIDTH_REG; break;
        case 0x0440000C: ptr = (uint8_t *)&g_Reg->VI_INTR_REG; break;
        case 0x04400010: ptr = (uint8_t *)&g_Reg->VI_V_CURRENT_LINE_REG; break;
        case 0x04400014: ptr = (uint8_t *)&g_Reg->VI_BURST_REG; break;
        case 0x04400018: ptr = (uint8_t *)&g_Reg->VI_V_SYNC_REG; break;
        case 0x0440001C: ptr = (uint8_t *)&g_Reg->VI_H_SYNC_REG; break;
        case 0x04400020: ptr = (uint8_t *)&g_Reg->VI_LEAP_REG; break;
        case 0x04400024: ptr = (uint8_t *)&g_Reg->VI_H_START_REG; break;
        case 0x04400028: ptr = (uint8_t *)&g_Reg->VI_V_START_REG; break;
        case 0x0440002C: ptr = (uint8_t *)&g_Reg->VI_V_BURST_REG; break;
        case 0x04400030: ptr = (uint8_t *)&g_Reg->VI_X_SCALE_REG; break;
        case 0x04400034: ptr = (uint8_t *)&g_Reg->VI_Y_SCALE_REG; break;
        case 0x04600000: ptr = (uint8_t *)&g_Reg->PI_DRAM_ADDR_REG; break;
        case 0x04600004: ptr = (uint8_t *)&g_Reg->PI_CART_ADDR_REG; break;
        case 0x04600008: ptr = (uint8_t *)&g_Reg->PI_RD_LEN_REG; break;
        case 0x0460000C: ptr = (uint8_t *)&g_Reg->PI_WR_LEN_REG; break;
        case 0x04600010: ptr = (uint8_t *)&g_Reg->PI_STATUS_REG; break;
        case 0x04600014: ptr = (uint8_t *)&g_Reg->PI_DOMAIN1_REG; break;
        case 0x04600018: ptr = (uint8_t *)&g_Reg->PI_BSD_DOM1_PWD_REG; break;
        case 0x0460001C: ptr = (uint8_t *)&g_Reg->PI_BSD_DOM1_PGS_REG; break;
        case 0x04600020: ptr = (uint8_t *)&g_Reg->PI_BSD_DOM1_RLS_REG; break;
        case 0x04600024: ptr = (uint8_t *)&g_Reg->PI_DOMAIN2_REG; break;
        case 0x04600028: ptr = (uint8_t *)&g_Reg->PI_BSD_DOM2_PWD_REG; break;
        case 0x0460002C: ptr = (uint8_t *)&g_Reg->PI_BSD_DOM2_PGS_REG; break;
        case 0x04600030: ptr = (uint8_t *)&g_Reg->PI_BSD_DOM2_RLS_REG; break;
        case 0x04700000: ptr = (uint8_t *)&g_Reg->RI_MODE_REG; break;
        case 0x04700004: ptr = (uint8_t *)&g_Reg->RI_CONFIG_REG; break;
        case 0x04700008: ptr = (uint8_t *)&g_Reg->RI_CURRENT_LOAD_REG; break;
        case 0x0470000C: ptr = (uint8_t *)&g_Reg->RI_SELECT_REG; break;
        case 0x04700010: ptr = (uint8_t *)&g_Reg->RI_REFRESH_REG; break;
        case 0x04700014: ptr = (uint8_t *)&g_Reg->RI_LATENCY_REG; break;
        case 0x04700018: ptr = (uint8_t *)&g_Reg->RI_RERROR_REG; break;
        case 0x0470001C: ptr = (uint8_t *)&g_Reg->RI_WERROR_REG; break;
        case 0x04800018: ptr = (uint8_t *)&g_Reg->SI_STATUS_REG; break;
        case 0x05000500: ptr = (uint8_t *)&g_Reg->ASIC_DATA; break;
        case 0x05000504: ptr = (uint8_t *)&g_Reg->ASIC_MISC_REG; break;
        case 0x05000508: ptr = (uint8_t *)&g_Reg->ASIC_STATUS; break;
        case 0x0500050C: ptr = (uint8_t *)&g_Reg->ASIC_CUR_TK; break;
        case 0x05000510: ptr = (uint8_t *)&g_Reg->ASIC_BM_STATUS; break;
        case 0x05000514: ptr = (uint8_t *)&g_Reg->ASIC_ERR_SECTOR; break;
        case 0x05000518: ptr = (uint8_t *)&g_Reg->ASIC_SEQ_STATUS; break;
        case 0x0500051C: ptr = (uint8_t *)&g_Reg->ASIC_CUR_SECTOR; break;
        case 0x05000520: ptr = (uint8_t *)&g_Reg->ASIC_HARD_RESET; break;
        case 0x05000524: ptr = (uint8_t *)&g_Reg->ASIC_C1_S0; break;
        case 0x05000528: ptr = (uint8_t *)&g_Reg->ASIC_HOST_SECBYTE; break;
        case 0x0500052C: ptr = (uint8_t *)&g_Reg->ASIC_C1_S2; break;
        case 0x05000530: ptr = (uint8_t *)&g_Reg->ASIC_SEC_BYTE; break;
        case 0x05000534: ptr = (uint8_t *)&g_Reg->ASIC_C1_S4; break;
        case 0x05000538: ptr = (uint8_t *)&g_Reg->ASIC_C1_S6; break;
        case 0x0500053C: ptr = (uint8_t *)&g_Reg->ASIC_CUR_ADDR; break;
        case 0x05000540: ptr = (uint8_t *)&g_Reg->ASIC_ID_REG; break;
        case 0x05000544: ptr = (uint8_t *)&g_Reg->ASIC_TEST_REG; break;
        case 0x05000548: ptr = (uint8_t *)&g_Reg->ASIC_TEST_PIN_SEL; break;
        }
    }

    if (ptr == nullptr)
    {
        return nullptr;
    }

    if (flags != nullptr)
    {
        *flags = (bCartRom ? PJMEM_CARTROM : 0);
    }

    if (bBigEndian)
    {
        return &ptr[nbyte];
    }
    else
    {
        return &ptr[nbyte ^ 3];
    }
}

bool CDebugMMU::GetPhysicalByte(uint32_t paddr, uint8_t * value)
{
    uint8_t * ptr = GetPhysicalPtr(paddr, nullptr);

    if (ptr != nullptr)
    {
        *value = *ptr;
        return true;
    }

    int nByte = paddr & 3;

    if (paddr >= 0x08000000 && paddr <= 0x08FFFFFF) // Cartridge domain 2 (address 2)
    {
        uint32_t saveOffset = paddr & 0x000FFFFF;

        if (g_System->m_SaveUsing == SaveChip_Sram && saveOffset < 0x88000 && (saveOffset & 0x3FFFF) < 0x8000) // SRAM
        {
            uint32_t wordpaddr = paddr & ~3;
            uint8_t data[4];

            CSram & sram = g_MMU->GetSram();
            sram.DmaFromSram(data, wordpaddr - 0x08000000, 4);
            *value = data[nByte ^ 3];
            return true;
        }
        else if (g_System->m_SaveUsing == SaveChip_FlashRam && saveOffset <= 3) // FlashRAM status
        {
            CFlashRam & FlashRam = g_MMU->GetFlashRam();
            uint32_t flashStatus = FlashRam.ReadFromFlashStatus(0x08000000);
            *value = (flashStatus >> (24 - nByte * 8)) & 0xFF;
            return true;
        }
    }

    if (paddr >= 0x04500004 && paddr <= 0x04500007)
    {
        uint32_t audioLength;

        if (g_System->bFixedAudio())
        {
            audioLength = g_MMU->AudioInterface().GetLength();
        }
        else
        {
            CAudioPlugin * audioPlg = g_Plugins->Audio();
            audioLength = audioPlg->AiReadLength != nullptr ? audioPlg->AiReadLength() : 0;
        }

        *value = (audioLength >> (24 - nByte * 8)) & 0xFF;
        return true;
    }

    if (paddr >= 0x0450000C && paddr <= 0x0450000F)
    {
        uint32_t audioStatus = g_System->bFixedAudio() ? g_MMU->AudioInterface().GetStatus() : g_Reg->AI_STATUS_REG;
        *value = (audioStatus >> (24 - nByte * 8)) & 0xFF;
        return true;
    }

    return false;
}

bool CDebugMMU::SetPhysicalByte(uint32_t paddr, uint8_t value)
{
    WORD flags;
    uint8_t * ptr = GetPhysicalPtr(paddr, &flags);
    bool bCartRom = flags & PJMEM_CARTROM;

    if (ptr != nullptr)
    {
        if (!bCartRom)
        {
            *ptr = value;
        }
        else
        {
            ProtectMemory(g_Rom->GetRomAddress(), g_Rom->GetRomSize(), MEM_READWRITE);
            *ptr = value;
            ProtectMemory(g_Rom->GetRomAddress(), g_Rom->GetRomSize(), MEM_READONLY);
        }
        return true;
    }

    int nByte = paddr & 3;

    if (paddr >= 0x08000000 && paddr <= 0x08FFFFFF) // Cartridge domain 2 (address 2)
    {
        uint32_t saveOffset = paddr & 0x000FFFFF;

        if (g_System->m_SaveUsing == SaveChip_Sram && saveOffset < 0x88000 && (saveOffset & 0x3FFFF) < 0x8000)
        {
            uint32_t wordpaddr = paddr & ~3;
            uint8_t data[4];

            CSram & sram = g_MMU->GetSram();
            sram.DmaFromSram(data, wordpaddr - 0x08000000, sizeof(data));
            data[nByte ^ 3] = value;
            sram.DmaToSram(data, wordpaddr - 0x08000000, sizeof(data));
            return true;
        }
    }

    return false;
}

size_t CDebugMMU::ReadPhysical(uint32_t paddr, size_t length, uint8_t * buffer)
{
    size_t nByte;
    for (nByte = 0; nByte < length; nByte++)
    {
        if (!GetPhysicalByte(paddr + nByte, &buffer[nByte]))
        {
            return nByte;
        }
    }
    return nByte;
}

size_t CDebugMMU::ReadVirtual(uint32_t vaddr, size_t length, uint8_t * buffer)
{
    size_t nByte;
    for (nByte = 0; nByte < length; nByte++)
    {
        uint32_t paddr;
        if (!g_MMU || !g_MMU->VAddrToPAddr(vaddr + nByte, paddr))
        {
            return nByte;
        }
        if (!GetPhysicalByte(paddr, &buffer[nByte]))
        {
            return nByte;
        }
    }
    return nByte;
}

size_t CDebugMMU::WritePhysical(uint32_t paddr, size_t length, uint8_t * buffer)
{
    size_t nByte;
    for (nByte = 0; nByte < length; nByte++)
    {
        if (!SetPhysicalByte(paddr + nByte, buffer[nByte]))
        {
            return nByte;
        }
    }
    return nByte;
}

size_t CDebugMMU::WriteVirtual(uint32_t vaddr, size_t length, uint8_t * buffer)
{
    size_t nByte;
    for (nByte = 0; nByte < length; nByte++)
    {
        uint32_t paddr;
        if (!g_MMU || !g_MMU->VAddrToPAddr(vaddr + nByte, paddr))
        {
            return nByte;
        }
        if (!SetPhysicalByte(paddr, buffer[nByte]))
        {
            return nByte;
        }
    }
    return nByte;
}
