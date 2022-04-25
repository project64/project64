#pragma once
#include "MemoryHandler.h"

class CN64Rom;

class CartridgeDomain1Address1Handler :
    public MemoryHandler
{
public:
    CartridgeDomain1Address1Handler(CN64Rom * DDRom);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    CartridgeDomain1Address1Handler();
    CartridgeDomain1Address1Handler(const CartridgeDomain1Address1Handler &);
    CartridgeDomain1Address1Handler & operator=(const CartridgeDomain1Address1Handler &);

    uint32_t m_DDRomSize;
    uint8_t * m_DDRom;
};
