#pragma once

#include <stdint.h>

typedef union tagUWORD
{
    int32_t W;
    uint32_t UW;
    int16_t HW[2];
    uint16_t UHW[2];
    int8_t B[4];
    uint8_t UB[4];
} UWORD32;

typedef union tagUDWORD
{
    int64_t DW;
    uint64_t UDW;
    int32_t W[2];
    uint32_t UW[2];
    int16_t HW[4];
    uint16_t UHW[4];
    int8_t B[8];
    uint8_t UB[8];
} UDWORD;

class RSPAccumulator
{
public:
    RSPAccumulator();

    int16_t & Low(uint8_t Index);
    int16_t & Mid(uint8_t Index);
    int16_t & High(uint8_t Index);

    uint16_t & ULow(uint8_t Index);
    uint16_t & UMid(uint8_t Index);
    uint16_t & UHigh(uint8_t Index);

#if defined(__i386__) || defined(_M_IX86)
    int32_t & LowWord(uint8_t Index);
    int32_t & HighWord(uint8_t Index);

    uint32_t & ULowWord(uint8_t Index);
    uint32_t & UHighWord(uint8_t Index);
#endif

    int64_t Get(uint8_t Index);
    void Set(uint8_t Index, int64_t Value);

    uint16_t Saturate(uint8_t el, bool HighValue);

    void Reset(void);

private:
#if defined(__i386__) || defined(_M_IX86)
    UDWORD m_Reg[8];
#endif
#if defined(__amd64__) || defined(_M_X64)
#if defined(_MSC_VER)
    uint64_t m_Reg[6] alignas(16);
#else
    uint64_t m_Reg[6];
#endif
#endif
};

class RSPVector
{
public:
    RSPVector();

    uint16_t & ue(uint8_t Index, uint8_t Element);
    int16_t & se(uint8_t Index, uint8_t Element);

    int8_t & s8(uint8_t Index);
    uint8_t & u8(uint8_t Index);
    int16_t & s16(uint8_t Index);
    uint16_t & u16(uint8_t Index);
    int32_t & s32(uint8_t Index);
    uint64_t & u64(uint8_t Index);

private:
#if defined(_MSC_VER)
    uint64_t m_Reg[2] alignas(16);
#else
    uint64_t m_Reg[2];
#endif
};

class RSPFlag
{
public:
    RSPFlag();

    void Clear(void);
    bool Set(uint8_t Index, bool Value);
    bool Get(uint8_t Index) const;

    uint8_t GetPacked(void) const;
    void SetPacked(uint8_t value);

#if defined(__i386__) || defined(_M_IX86)
    uint8_t & Value();
#endif

private:
    RSPFlag(const RSPFlag &);
    RSPFlag & operator=(const RSPFlag &);

#if defined(__i386__) || defined(_M_IX86)
    uint8_t m_Flag;
#endif
#if defined(__amd64__) || defined(_M_X64)
    alignas(16) uint16_t m_Flags[8];
#endif
};