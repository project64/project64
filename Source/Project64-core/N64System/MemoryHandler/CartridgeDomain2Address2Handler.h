#pragma once
#include <Project64-core\N64System\SaveType\Sram.h>
#include <Project64-core\N64System\SaveType\FlashRam.h>
#include <Project64-core\Settings\DebugSettings.h>
#include "MemoryHandler.h"

class CN64System;
class CMipsMemoryVM;
class CRegisters;

class CartridgeDomain2Address2Handler :
    public MemoryHandler,
    private CDebugSettings
{
public:
    CartridgeDomain2Address2Handler(CN64System & System, CRegisters & Reg, CMipsMemoryVM & MMU, bool SavesReadOnly);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

    bool DMARead();
    void DMAWrite();

    CSram & Sram(void) { return m_Sram; }
    CFlashRam & FlashRam (void) { return m_FlashRam; }

private:
    CartridgeDomain2Address2Handler(void);
    CartridgeDomain2Address2Handler(const CartridgeDomain2Address2Handler &);
    CartridgeDomain2Address2Handler & operator=(const CartridgeDomain2Address2Handler &);

    CN64System & m_System;
    CRegisters & m_Reg;
    CMipsMemoryVM & m_MMU;
    CSram m_Sram;
    CFlashRam m_FlashRam;
};
