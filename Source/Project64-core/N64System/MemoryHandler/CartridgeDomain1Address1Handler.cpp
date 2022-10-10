#include "stdafx.h"

#include "CartridgeDomain1Address1Handler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64Rom.h>

CartridgeDomain1Address1Handler::CartridgeDomain1Address1Handler(CRegisters & Reg, CN64Rom * DDRom) :
    m_Reg(Reg),
    m_DDRom(DDRom != nullptr ? DDRom->GetRomAddress() : nullptr),
    m_DDRomSize(DDRom != nullptr ? DDRom->GetRomSize() : 0)
{
}

bool CartridgeDomain1Address1Handler::Read32(uint32_t Address, uint32_t & Value)
{
    if (m_DDRom != nullptr && (Address & 0xFFFFFF) < m_DDRomSize)
    {
        Value = *(uint32_t *)&m_DDRom[Address & 0xFFFFFF];
    }
    else
    {
        m_Reg.PI_CART_ADDR_REG = (Address + 4) & ~1;
        Value = ((Address & 0xFFFF) << 16) | (Address & 0xFFFF);
    }
    return true;
}

bool CartridgeDomain1Address1Handler::Write32(uint32_t /*Address*/, uint32_t /*Value*/, uint32_t /*Mask*/)
{
    return true;
}
