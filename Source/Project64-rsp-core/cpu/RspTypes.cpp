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
    return ((uint16_t *)&m_Reg)[AccurateEmulation ? 7 - Index : Index];
}

int16_t & RSPVector::se(uint8_t Index, uint8_t Element)
{
    Index = EleSpec[Element].B[Index];
    return ((int16_t *)&m_Reg)[AccurateEmulation ? 7 - Index : Index];
}

int8_t & RSPVector::s8(uint8_t Index)
{
    return ((int8_t *)&m_Reg)[AccurateEmulation ? 15 - Index : Index];
}

uint8_t & RSPVector::u8(uint8_t Index)
{
    return ((uint8_t *)&m_Reg)[AccurateEmulation ? 15 - Index : Index];
}

int16_t & RSPVector::s16(uint8_t Index)
{
    return ((int16_t *)&m_Reg)[AccurateEmulation ? 7 - Index : Index];
}

uint16_t & RSPVector::u16(uint8_t Index)
{
    return ((uint16_t *)&m_Reg)[AccurateEmulation ? 7 - Index : Index];
}

int32_t & RSPVector::s32(uint8_t Index)
{
    return ((int32_t *)&m_Reg)[AccurateEmulation ? 3 - Index : Index];
}

uint64_t & RSPVector::u64(uint8_t Index)
{
    return m_Reg[AccurateEmulation ? 1 - Index : Index];
}
