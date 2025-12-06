#include "RspTypes.h"
#include <Project64-rsp-core/Settings/RspSettings.h>

extern UDWORD EleSpec[16];

RSPAccumulator::RSPAccumulator()
{
    Reset();
}

int16_t & RSPAccumulator::Low(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Reg[Index].HW[1];
#elif defined(__amd64__) || defined(_M_X64)
    return ((int16_t *)&m_Reg[0])[Index];
#endif
}

int16_t & RSPAccumulator::Mid(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Reg[Index].HW[2];
#elif defined(__amd64__) || defined(_M_X64)
    return ((int16_t *)&m_Reg[2])[Index];
#endif
}

int16_t & RSPAccumulator::High(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Reg[Index].HW[3];
#elif defined(__amd64__) || defined(_M_X64)
    return ((int16_t *)&m_Reg[4])[Index];
#endif
}

uint16_t & RSPAccumulator::ULow(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Reg[Index].UHW[1];
#elif defined(__amd64__) || defined(_M_X64)
    return ((uint16_t *)&m_Reg[0])[Index];
#endif
}

uint16_t & RSPAccumulator::UMid(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Reg[Index].UHW[2];
#elif defined(__amd64__) || defined(_M_X64)
    return ((uint16_t *)&m_Reg[2])[Index];
#endif
}

uint16_t & RSPAccumulator::UHigh(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Reg[Index].UHW[3];
#elif defined(__amd64__) || defined(_M_X64)
    return ((uint16_t *)&m_Reg[4])[Index];
#endif
}

#if defined(__i386__) || defined(_M_IX86)
int32_t & RSPAccumulator::LowWord(uint8_t Index)
{
    return m_Reg[Index].W[0];
}

int32_t & RSPAccumulator::HighWord(uint8_t Index)
{
    return m_Reg[Index].W[1];
}

uint32_t & RSPAccumulator::ULowWord(uint8_t Index)
{
    return m_Reg[Index].UW[0];
}

uint32_t & RSPAccumulator::UHighWord(uint8_t Index)
{
    return m_Reg[Index].UW[1];
}
#endif

int64_t RSPAccumulator::Get(uint8_t Index)
{
#if defined(__i386__) || defined(_M_IX86)
    return (((int64_t)m_Reg[Index].HW[3]) << 32) | (((int64_t)m_Reg[Index].UHW[2]) << 16) | m_Reg[Index].UHW[1];
#elif defined(__amd64__) || defined(_M_X64)
    return (((int64_t)((int16_t *)&m_Reg[4])[Index]) << 32) | (((int64_t)((uint16_t)((int16_t *)&m_Reg[2])[Index])) << 16) | ((uint16_t)((int16_t *)&m_Reg[0])[Index]);
#endif
}

void RSPAccumulator::Set(uint8_t Index, int64_t Value)
{
#if defined(__i386__) || defined(_M_IX86)
    m_Reg[Index].HW[3] = (int16_t)(Value >> 32);
    m_Reg[Index].HW[2] = (int16_t)(Value >> 16);
    m_Reg[Index].HW[1] = (int16_t)(Value);
#elif defined(__amd64__) || defined(_M_X64)
    ((int16_t *)&m_Reg[0])[Index] = (int16_t)(Value & 0xFFFF);
    ((int16_t *)&m_Reg[2])[Index] = (int16_t)((Value >> 16) & 0xFFFF);
    ((int16_t *)&m_Reg[4])[Index] = (int16_t)((Value >> 32) & 0xFFFF);
#endif
}

uint16_t RSPAccumulator::Saturate(uint8_t el, bool HighValue)
{
    if (High(el) < 0)
    {
        if (UHigh(el) != 0xFFFF || Mid(el) >= 0)
        {
            return HighValue ? 0x8000 : 0x0000;
        }
        else
        {
            return HighValue ? UMid(el) : ULow(el);
        }
    }
    if (UHigh(el) != 0 || Mid(el) < 0)
    {
        return HighValue ? 0x7fff : 0xffff;
    }
    return HighValue ? UMid(el) : ULow(el);
}

void RSPAccumulator::Reset(void)
{
    for (uint32_t i = 0, n = (sizeof(m_Reg) / sizeof(m_Reg[0])); i < n; i++)
    {
#if defined(__i386__) || defined(_M_IX86)
        m_Reg[i].UDW = 0;
#elif defined(__amd64__) || defined(_M_X64)
        m_Reg[i] = 0;
#endif
    }
}

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

RSPFlag::RSPFlag() 
{
    Clear();
}

void RSPFlag::Clear(void)
{
#if defined(__i386__) || defined(_M_IX86)
    m_Flag = 0;
#endif
#if defined(__amd64__) || defined(_M_X64)
    for (int i = 0; i < 8; i++)
    {
        m_Flags[i] = 0;
    }
#endif
}

bool RSPFlag::Set(uint8_t Index, bool Value)
{
#if defined(__i386__) || defined(_M_IX86)
    if (Value)
    {
        m_Flag |= (1 << (7 - Index));
    }
    else
    {
        m_Flag &= ~(1 << (7 - Index));
    }
#endif
#if defined(__amd64__) || defined(_M_X64)
    m_Flags[Index] = Value ? 1 : 0; 
#endif
    return Value;
}

bool RSPFlag::Get(uint8_t Index) const
{
#if defined(__i386__) || defined(_M_IX86)
    return (m_Flag & (1 << (7 - Index))) != 0;
#endif
#if defined(__amd64__) || defined(_M_X64)
    return m_Flags[Index] != 0;
#endif
}

uint8_t RSPFlag::GetPacked(void) const
{
#if defined(__i386__) || defined(_M_IX86)
    return m_Flag;
#endif
#if defined(__amd64__) || defined(_M_X64)
    uint8_t result = 0;
    for (int i = 0; i < 8; i++)
    {
        if (m_Flags[i])
        {
            result |= (1 << (7 - i));
        }
    }
    return result;
#endif
}

void RSPFlag::SetPacked(uint8_t value)
{
#if defined(__i386__) || defined(_M_IX86)
    m_Flag = value;
#endif
#if defined(__amd64__) || defined(_M_X64)
    for (int i = 0; i < 8; i++)
    {
        m_Flags[i] = (value & (1 << (7 - i))) ? 1 : 0;
    }
#endif
}

#if defined(__i386__) || defined(_M_IX86)
uint8_t & RSPFlag::Value()
{
    return m_Flag;
}
#endif
