#pragma once
#include "MemoryHandler.h"

class CartridgeDomain1Address3Handler :
    public MemoryHandler
{
public:
    CartridgeDomain1Address3Handler();

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    CartridgeDomain1Address3Handler(const CartridgeDomain1Address3Handler &);
    CartridgeDomain1Address3Handler & operator=(const CartridgeDomain1Address3Handler &);
};
