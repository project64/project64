#include "stdafx.h"

#include "CartridgeDomain1Address3Handler.h"

CartridgeDomain1Address3Handler::CartridgeDomain1Address3Handler()
{
}

bool CartridgeDomain1Address3Handler::Read32(uint32_t Address, uint32_t & Value)
{
    Value = ((Address & 0xFFFF) << 16) | (Address & 0xFFFF);
    return true;
}

bool CartridgeDomain1Address3Handler::Write32(uint32_t /*Address*/, uint32_t /*Value*/, uint32_t /*Mask*/)
{
    return true;
}
