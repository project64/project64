#include "RspTypes.h"
#include <Project64-rsp-core/Settings/RspSettings.h>

extern UDWORD EleSpec[16];

RSPVector::RSPVector()
{
    m_Reg[0] = 0;
    m_Reg[1] = 0;
}

uint16_t & RSPVector::ue(uint8_t Index, uint8_t Element)
{
    Index = EleSpec[Element].B[Index];
    return ((uint16_t *)&m_Reg)[Index];
}

int16_t & RSPVector::se(uint8_t Index, uint8_t Element)
{
    Index = EleSpec[Element].B[Index];
    return ((int16_t *)&m_Reg)[Index];
}

int8_t & RSPVector::s8(uint8_t Index)
{
    return ((int8_t *)&m_Reg)[Index];
}

uint8_t & RSPVector::u8(uint8_t Index)
{
    return ((uint8_t *)&m_Reg)[Index];
}

int16_t & RSPVector::s16(uint8_t Index)
{
    return ((int16_t *)&m_Reg)[Index];
}

uint16_t & RSPVector::u16(uint8_t Index)
{
    return ((uint16_t *)&m_Reg)[Index];
}

int32_t & RSPVector::s32(uint8_t Index)
{
    return ((int32_t *)&m_Reg)[Index];
}

uint64_t & RSPVector::u64(uint8_t Index)
{
    return m_Reg[Index];
}

RSPFlag::RSPFlag(uint8_t & Flag) :
    m_Flag(Flag)
{
}

void RSPFlag::Clear(void)
{
    m_Flag = 0;
}

bool RSPFlag::Set(uint8_t Index, bool Value)
{
    if (Value)
    {
        m_Flag |= (1 << (7 - Index));
    }
    else
    {
        m_Flag &= ~(1 << (7 - Index));
    }
    return Value;
}

bool RSPFlag::Get(uint8_t Index) const
{
    return (m_Flag & (1 << (7 - Index))) != 0;
}
