#include "cpu/RspTypes.h"

// RSP registers
UWORD32 RSP_GPR[32], RSP_Flags[4];
UDWORD RSP_ACCUM[8];
RSPVector RSP_Vect[32];

char * GPR_Strings[32] = {
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