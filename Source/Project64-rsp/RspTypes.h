#ifndef __Types_h
#define __Types_h

#include <stdint.h>

// Pointer to RSP operation code functions or "func"
// This is the type of all RSP interpreter and recompiler functions

typedef void (*p_func)(void);

typedef union tagUWORD
{
    int32_t W;
    uint32_t UW;
    int16_t HW[2];
    uint16_t UHW[2];
    int8_t B[4];
    uint8_t UB[4];

    float F;
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

    double D;
    float F[2];
} UDWORD;

typedef union tagVect
{
    int64_t DW[2];
    uint64_t UDW[2];
    int32_t W[4];
    uint32_t UW[4];
    int16_t HW[8];
    uint16_t UHW[8];
    int8_t B[16];
    uint8_t UB[16];

    double FD[2];
    float FS[4];
} VECTOR;

#endif
