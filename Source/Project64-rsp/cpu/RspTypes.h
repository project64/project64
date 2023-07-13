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

class RSPVector
{
public:
    RSPVector();

    int8_t & s8(uint8_t Index);
    uint8_t & u8(uint8_t Index);
    int16_t & s16(uint8_t Index);
    uint16_t & u16(uint8_t Index);
    int32_t & s32(uint8_t Index);
    uint64_t & u64(uint8_t Index);

private:
    uint64_t m_Reg[2] alignas(16);
};
