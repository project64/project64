#include "RspTypes.h"

RSPVector::RSPVector()
{
    m_Reg[0] = 0;
    m_Reg[1] = 0;
}

int8_t & RSPVector::s8(uint8_t Index)
{
    return ((int8_t*)&m_Reg)[15 - Index];
}

uint8_t & RSPVector::u8(uint8_t Index)
{
    return ((uint8_t*)&m_Reg)[15 - Index];
}

int16_t & RSPVector::s16(uint8_t Index)
{
    return ((int16_t*)&m_Reg)[7 - Index];
}

uint16_t & RSPVector::u16(uint8_t Index)
{
    return ((uint16_t*)&m_Reg)[7 - Index];
}

int32_t & RSPVector::s32(uint8_t Index)
{
    return ((int32_t*)&m_Reg)[3 - Index];
}

uint64_t & RSPVector::u64(uint8_t Index)
{
    return m_Reg[1 - Index];
}
