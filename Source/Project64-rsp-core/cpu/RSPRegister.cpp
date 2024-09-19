#include "RspTypes.h"
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <string.h>

const char * GPR_Strings[32] = {
    "R0",
    "AT",
    "V0",
    "V1",
    "A0",
    "A1",
    "A2",
    "A3",
    "T0",
    "T1",
    "T2",
    "T3",
    "T4",
    "T5",
    "T6",
    "T7",
    "S0",
    "S1",
    "S2",
    "S3",
    "S4",
    "S5",
    "S6",
    "S7",
    "T8",
    "T9",
    "K0",
    "K1",
    "GP",
    "SP",
    "S8",
    "RA",
};

CGPRRegisters::CGPRRegisters(UWORD32 (&m_GPR)[32]) :
    GPR_R0(m_GPR[0].UW),
    GPR_AT(m_GPR[1].UW),
    GPR_V0(m_GPR[2].UW),
    GPR_V1(m_GPR[3].UW),
    GPR_A0(m_GPR[4].UW),
    GPR_A1(m_GPR[5].UW),
    GPR_A2(m_GPR[6].UW),
    GPR_A3(m_GPR[7].UW),
    GPR_T0(m_GPR[8].UW),
    GPR_T1(m_GPR[9].UW),
    GPR_T2(m_GPR[10].UW),
    GPR_T3(m_GPR[11].UW),
    GPR_T4(m_GPR[12].UW),
    GPR_T5(m_GPR[13].UW),
    GPR_T6(m_GPR[14].UW),
    GPR_T7(m_GPR[15].UW),
    GPR_S0(m_GPR[16].UW),
    GPR_S1(m_GPR[17].UW),
    GPR_S2(m_GPR[18].UW),
    GPR_S3(m_GPR[19].UW),
    GPR_S4(m_GPR[20].UW),
    GPR_S5(m_GPR[21].UW),
    GPR_S6(m_GPR[22].UW),
    GPR_S7(m_GPR[23].UW),
    GPR_T8(m_GPR[24].UW),
    GPR_T9(m_GPR[25].UW),
    GPR_K0(m_GPR[26].UW),
    GPR_K1(m_GPR[27].UW),
    GPR_GP(m_GPR[28].UW),
    GPR_SP(m_GPR[29].UW),
    GPR_S8(m_GPR[30].UW),
    GPR_RA(m_GPR[31].UW)
{
}

CRSPRegisters::CRSPRegisters() :
    VCOL(m_Flags[0].UB[0]),
    VCOH(m_Flags[0].UB[1]),
    VCCL(m_Flags[1].UB[0]),
    VCCH(m_Flags[1].UB[1]),
    VCE(m_Flags[2].UB[0])
{
    Reset();
}

void CRSPRegisters::Reset(void)
{
    memset(m_GPR, 0, sizeof(m_GPR));
    memset(m_Flags, 0, sizeof(m_Flags));
    memset(m_ACCUM, 0, sizeof(m_ACCUM));
    for (size_t i = 0, n = sizeof(m_Vect) / sizeof(m_Vect[0]); i < n; i++)
    {
        m_Vect[i] = RSPVector();
    }
    m_Reciprocals[0] = 0xFFFF;
    for (uint16_t i = 1; i < 512; i++)
    {
        m_Reciprocals[i] = uint16_t((((1ull << 34) / (uint64_t)(i + 512)) + 1) >> 8);
    }

    for (uint16_t i = 0; i < 512; i++)
    {
        uint64_t a = (i + 512) >> ((i % 2 == 1) ? 1 : 0);
        uint64_t b = 1 << 17;
        while (a * (b + 1) * (b + 1) < (uint64_t(1) << 44))
        {
            b++;
        }
        m_InverseSquareRoots[i] = uint16_t(b >> 1);
    }
    m_Result = 0;
    m_In = 0;
    m_High = false;
}

int64_t CRSPRegisters::AccumulatorGet(uint8_t el)
{
    return (((int64_t)m_ACCUM[el].HW[3]) << 32) | (((int64_t)m_ACCUM[el].UHW[2]) << 16) | m_ACCUM[el].UHW[1];
}

void CRSPRegisters::AccumulatorSet(uint8_t el, int64_t Accumulator)
{
    m_ACCUM[el].HW[3] = (int16_t)(Accumulator >> 32);
    m_ACCUM[el].HW[2] = (int16_t)(Accumulator >> 16);
    m_ACCUM[el].HW[1] = (int16_t)(Accumulator);
}

uint16_t CRSPRegisters::AccumulatorSaturate(uint8_t el, bool High)
{
    if (m_ACCUM[el].HW[3] < 0)
    {
        if (m_ACCUM[el].UHW[3] != 0xFFFF || m_ACCUM[el].HW[2] >= 0)
        {
            return High ? 0x8000 : 0x0000;
        }
        else
        {
            return m_ACCUM[el].UHW[High ? 2 : 1];
        }
    }
    if (m_ACCUM[el].UHW[3] != 0 || m_ACCUM[el].HW[2] < 0)
    {
        return High ? 0x7fff : 0xffff;
    }
    return m_ACCUM[el].UHW[High ? 2 : 1];
}
