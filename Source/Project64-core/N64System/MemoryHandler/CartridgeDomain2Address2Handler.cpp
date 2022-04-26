#include "stdafx.h"
#include "CartridgeDomain2Address2Handler.h"
#include <Project64-core\N64System\N64System.h>

CartridgeDomain2Address2Handler::CartridgeDomain2Address2Handler(CN64System & System, CRegisters & Reg, CMipsMemoryVM & MMU, bool SavesReadOnly) :
    m_System(System),
    m_Reg(Reg),
    m_MMU(MMU),
    m_Sram(SavesReadOnly),
    m_FlashRam(SavesReadOnly)
{
}

bool CartridgeDomain2Address2Handler::Read32(uint32_t Address, uint32_t & Value)
{
    if (m_System.m_SaveUsing == SaveChip_Auto)
    {
        m_System.m_SaveUsing = SaveChip_FlashRam;
    }

    uint32_t offset = (Address & 0x1FFFFFFF) - 0x08000000;
    if (offset > 0x88000)
    {
        Value = ((offset & 0xFFFF) << 16) | (offset & 0xFFFF);
        return true;
    }
    if (m_System.m_SaveUsing == SaveChip_Sram)
    {
        // Load SRAM
        uint8_t tmp[4] = "";
        m_Sram.DmaFromSram(tmp, offset, 4);
        Value = tmp[3] << 24 | tmp[2] << 16 | tmp[1] << 8 | tmp[0];
    }
    else if (m_System.m_SaveUsing == SaveChip_FlashRam)
    {
        Value = m_FlashRam.ReadFromFlashStatus(Address & 0x1FFFFFFF);
    }
    else
    {
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        Value = ((Address & 0xFFFF) << 16) | (Address & 0xFFFF);
    }
    return true;
}

bool CartridgeDomain2Address2Handler::Write32(uint32_t Address, uint32_t Value, uint32_t /*Mask*/)
{
    if (m_System.m_SaveUsing == SaveChip_Auto)
    {
        m_System.m_SaveUsing = SaveChip_FlashRam;
    }

    uint32_t offset = (Address & 0x1FFFFFFF) - 0x08000000;
    if (offset > 0x10000)
    {
        return true;
    }
    if (m_System.m_SaveUsing == SaveChip_Sram && offset < 0x88000)
    {
        // Store SRAM
        uint8_t tmp[4] = "";
        tmp[0] = 0xFF & (Value);
        tmp[1] = 0xFF & (Value >> 8);
        tmp[2] = 0xFF & (Value >> 16);
        tmp[3] = 0xFF & (Value >> 24);
        m_Sram.DmaToSram(tmp, (Address & 0x1FFFFFFF) - 0x08000000, 4);
        return true;
    }
    else if (m_System.m_SaveUsing == SaveChip_FlashRam)
    {
        m_FlashRam.WriteToFlashCommand(Value);
    }
    return true;
}

bool CartridgeDomain2Address2Handler::DMARead()
{
    uint32_t PI_RD_LEN_REG = ((m_Reg.PI_RD_LEN_REG) & 0x00FFFFFFul) + 1;

    if ((PI_RD_LEN_REG & 1) != 0)
    {
        PI_RD_LEN_REG += 1;
    }

    if (m_System.m_SaveUsing == SaveChip_Auto)
    {
        m_System.m_SaveUsing = SaveChip_Sram;
    }
    if (m_System.m_SaveUsing == SaveChip_Sram)
    {
        m_Sram.DmaToSram(m_MMU.Rdram() + m_Reg.PI_DRAM_ADDR_REG,m_Reg.PI_CART_ADDR_REG - 0x08000000, PI_RD_LEN_REG);
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        m_Reg.MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        return true;
    }
    else if (m_System.m_SaveUsing == SaveChip_FlashRam)
    {
        m_FlashRam.DmaToFlashram(m_MMU.Rdram() + m_Reg.PI_DRAM_ADDR_REG, m_Reg.PI_CART_ADDR_REG - 0x08000000, PI_RD_LEN_REG);
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        m_Reg.MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        return true;
    }
    return false;
}

void CartridgeDomain2Address2Handler::DMAWrite()
{
    uint32_t PI_WR_LEN_REG = ((m_Reg.PI_WR_LEN_REG) & 0x00FFFFFEul) + 2;

    if (m_System.m_SaveUsing == SaveChip_Auto)
    {
        m_System.m_SaveUsing = SaveChip_Sram;
    }
    if (m_System.m_SaveUsing == SaveChip_Sram)
    {
        m_Sram.DmaFromSram(m_MMU.Rdram() + m_Reg.PI_DRAM_ADDR_REG, m_Reg.PI_CART_ADDR_REG - 0x08000000, PI_WR_LEN_REG);
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        m_Reg.MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
    }
    else if (m_System.m_SaveUsing == SaveChip_FlashRam)
    {
        m_FlashRam.DmaFromFlashram(m_MMU.Rdram() + m_Reg.PI_DRAM_ADDR_REG, m_Reg.PI_CART_ADDR_REG - 0x08000000, PI_WR_LEN_REG);
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        m_Reg.MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
    }
}