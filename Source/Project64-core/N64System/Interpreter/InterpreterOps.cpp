/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Mips/TLBClass.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/Logging.h>
#include <float.h>
#include <math.h>

#if (defined(_MSC_VER) && (_MSC_VER < 1800))
double trunc(double num)
{
    return (num < 0) ? ceil(num) : floor(num);
}

float truncf(float num)
{
    return (num < 0) ? ceilf(num) : floorf(num);
}
#endif

void InPermLoop();
void TestInterpreterJump(uint32_t PC, uint32_t TargetPC, int32_t Reg1, int32_t Reg2);

bool        R4300iOp::m_TestTimer = false;
uint32_t    R4300iOp::m_NextInstruction;
OPCODE      R4300iOp::m_Opcode;
uint32_t    R4300iOp::m_JumpToLocation;

R4300iOp::Func R4300iOp::Jump_Opcode[64];
R4300iOp::Func R4300iOp::Jump_Special[64];
R4300iOp::Func R4300iOp::Jump_Regimm[32];
R4300iOp::Func R4300iOp::Jump_CoP0[32];
R4300iOp::Func R4300iOp::Jump_CoP0_Function[64];
R4300iOp::Func R4300iOp::Jump_CoP1[32];
R4300iOp::Func R4300iOp::Jump_CoP1_BC[32];
R4300iOp::Func R4300iOp::Jump_CoP1_S[64];
R4300iOp::Func R4300iOp::Jump_CoP1_D[64];
R4300iOp::Func R4300iOp::Jump_CoP1_W[64];
R4300iOp::Func R4300iOp::Jump_CoP1_L[64];

const uint32_t R4300iOp::SWL_MASK[4] = { 0x00000000, 0xFF000000, 0xFFFF0000, 0xFFFFFF00 };
const uint32_t R4300iOp::SWR_MASK[4] = { 0x00FFFFFF, 0x0000FFFF, 0x000000FF, 0x00000000 };
const uint32_t R4300iOp::LWL_MASK[4] = { 0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF };
const uint32_t R4300iOp::LWR_MASK[4] = { 0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0x0000000 };

const int32_t   R4300iOp::SWL_SHIFT[4] = { 0, 8, 16, 24 };
const int32_t   R4300iOp::SWR_SHIFT[4] = { 24, 16, 8, 0 };
const int32_t   R4300iOp::LWL_SHIFT[4] = { 0, 8, 16, 24 };
const int32_t   R4300iOp::LWR_SHIFT[4] = { 24, 16, 8, 0 };

#define ADDRESS_ERROR_EXCEPTION(Address,FromRead) \
    g_Reg->DoAddressError(m_NextInstruction == JUMP,Address,FromRead);\
    m_NextInstruction = JUMP;\
    m_JumpToLocation = (*_PROGRAM_COUNTER);\
    return;

#define TEST_COP1_USABLE_EXCEPTION() \
    if ((g_Reg->STATUS_REGISTER & STATUS_CU1) == 0) {\
    g_Reg->DoCopUnusableException(m_NextInstruction == JUMP,1);\
    m_NextInstruction = JUMP;\
    m_JumpToLocation = (*_PROGRAM_COUNTER);\
    return;}\

#define TLB_READ_EXCEPTION(Address) \
    g_Reg->DoTLBReadMiss(m_NextInstruction == JUMP,Address);\
    m_NextInstruction = JUMP;\
    m_JumpToLocation = (*_PROGRAM_COUNTER);\
    return;

void R4300iOp::SPECIAL()
{
    Jump_Special[m_Opcode.funct]();
}

void R4300iOp::REGIMM()
{
    Jump_Regimm[m_Opcode.rt]();
}

void R4300iOp::COP0()
{
    Jump_CoP0[m_Opcode.rs]();
}

void R4300iOp::COP0_CO()
{
    Jump_CoP0_Function[m_Opcode.funct]();
}

void R4300iOp::COP1()
{
    Jump_CoP1[m_Opcode.fmt]();
}

void R4300iOp::COP1_BC()
{
    Jump_CoP1_BC[m_Opcode.ft]();
}

void R4300iOp::COP1_S()
{
    fesetround(*_RoundingModel);
    Jump_CoP1_S[m_Opcode.funct]();
}

void R4300iOp::COP1_D()
{
    fesetround(*_RoundingModel);
    Jump_CoP1_D[m_Opcode.funct]();
}

void R4300iOp::COP1_W()
{
    Jump_CoP1_W[m_Opcode.funct]();
}

void R4300iOp::COP1_L()
{
    Jump_CoP1_L[m_Opcode.funct]();
}

R4300iOp::Func * R4300iOp::BuildInterpreter()
{
    Jump_Opcode[0] = SPECIAL;
    Jump_Opcode[1] = REGIMM;
    Jump_Opcode[2] = J;
    Jump_Opcode[3] = JAL;
    Jump_Opcode[4] = BEQ;
    Jump_Opcode[5] = BNE;
    Jump_Opcode[6] = BLEZ;
    Jump_Opcode[7] = BGTZ;
    Jump_Opcode[8] = ADDI;
    Jump_Opcode[9] = ADDIU;
    Jump_Opcode[10] = SLTI;
    Jump_Opcode[11] = SLTIU;
    Jump_Opcode[12] = ANDI;
    Jump_Opcode[13] = ORI;
    Jump_Opcode[14] = XORI;
    Jump_Opcode[15] = LUI;
    Jump_Opcode[16] = COP0;
    Jump_Opcode[17] = COP1;
    Jump_Opcode[18] = UnknownOpcode;
    Jump_Opcode[19] = UnknownOpcode;
    Jump_Opcode[20] = BEQL;
    Jump_Opcode[21] = BNEL;
    Jump_Opcode[22] = BLEZL;
    Jump_Opcode[23] = BGTZL;
    Jump_Opcode[24] = UnknownOpcode;
    Jump_Opcode[25] = DADDIU;
    Jump_Opcode[26] = LDL;
    Jump_Opcode[27] = LDR;
    Jump_Opcode[28] = UnknownOpcode;
    Jump_Opcode[29] = UnknownOpcode;
    Jump_Opcode[30] = UnknownOpcode;
    Jump_Opcode[31] = UnknownOpcode;
    Jump_Opcode[32] = LB;
    Jump_Opcode[33] = LH;
    Jump_Opcode[34] = LWL;
    Jump_Opcode[35] = LW;
    Jump_Opcode[36] = LBU;
    Jump_Opcode[37] = LHU;
    Jump_Opcode[38] = LWR;
    Jump_Opcode[39] = LWU;
    Jump_Opcode[40] = SB;
    Jump_Opcode[41] = SH;
    Jump_Opcode[42] = SWL;
    Jump_Opcode[43] = SW;
    Jump_Opcode[44] = SDL;
    Jump_Opcode[45] = SDR;
    Jump_Opcode[46] = SWR;
    Jump_Opcode[47] = CACHE;
    Jump_Opcode[48] = LL;
    Jump_Opcode[49] = LWC1;
    Jump_Opcode[50] = UnknownOpcode;
    Jump_Opcode[51] = UnknownOpcode;
    Jump_Opcode[52] = UnknownOpcode;
    Jump_Opcode[53] = LDC1;
    Jump_Opcode[54] = UnknownOpcode;
    Jump_Opcode[55] = LD;
    Jump_Opcode[56] = SC;
    Jump_Opcode[57] = SWC1;
    Jump_Opcode[58] = UnknownOpcode;
    Jump_Opcode[59] = UnknownOpcode;
    Jump_Opcode[60] = UnknownOpcode;
    Jump_Opcode[61] = SDC1;
    Jump_Opcode[62] = UnknownOpcode;
    Jump_Opcode[63] = SD;

    Jump_Special[0] = SPECIAL_SLL;
    Jump_Special[1] = UnknownOpcode;
    Jump_Special[2] = SPECIAL_SRL;
    Jump_Special[3] = SPECIAL_SRA;
    Jump_Special[4] = SPECIAL_SLLV;
    Jump_Special[5] = UnknownOpcode;
    Jump_Special[6] = SPECIAL_SRLV;
    Jump_Special[7] = SPECIAL_SRAV;
    Jump_Special[8] = SPECIAL_JR;
    Jump_Special[9] = SPECIAL_JALR;
    Jump_Special[10] = UnknownOpcode;
    Jump_Special[11] = UnknownOpcode;
    Jump_Special[12] = SPECIAL_SYSCALL;
    Jump_Special[13] = UnknownOpcode;
    Jump_Special[14] = UnknownOpcode;
    Jump_Special[15] = SPECIAL_SYNC;
    Jump_Special[16] = SPECIAL_MFHI;
    Jump_Special[17] = SPECIAL_MTHI;
    Jump_Special[18] = SPECIAL_MFLO;
    Jump_Special[19] = SPECIAL_MTLO;
    Jump_Special[20] = SPECIAL_DSLLV;
    Jump_Special[21] = UnknownOpcode;
    Jump_Special[22] = SPECIAL_DSRLV;
    Jump_Special[23] = SPECIAL_DSRAV;
    Jump_Special[24] = SPECIAL_MULT;
    Jump_Special[25] = SPECIAL_MULTU;
    Jump_Special[26] = SPECIAL_DIV;
    Jump_Special[27] = SPECIAL_DIVU;
    Jump_Special[28] = SPECIAL_DMULT;
    Jump_Special[29] = SPECIAL_DMULTU;
    Jump_Special[30] = SPECIAL_DDIV;
    Jump_Special[31] = SPECIAL_DDIVU;
    Jump_Special[32] = SPECIAL_ADD;
    Jump_Special[33] = SPECIAL_ADDU;
    Jump_Special[34] = SPECIAL_SUB;
    Jump_Special[35] = SPECIAL_SUBU;
    Jump_Special[36] = SPECIAL_AND;
    Jump_Special[37] = SPECIAL_OR;
    Jump_Special[38] = SPECIAL_XOR;
    Jump_Special[39] = SPECIAL_NOR;
    Jump_Special[40] = UnknownOpcode;
    Jump_Special[41] = UnknownOpcode;
    Jump_Special[42] = SPECIAL_SLT;
    Jump_Special[43] = SPECIAL_SLTU;
    Jump_Special[44] = SPECIAL_DADD;
    Jump_Special[45] = SPECIAL_DADDU;
    Jump_Special[46] = SPECIAL_DSUB;
    Jump_Special[47] = SPECIAL_DSUBU;
    Jump_Special[48] = UnknownOpcode;
    Jump_Special[49] = UnknownOpcode;
    Jump_Special[50] = UnknownOpcode;
    Jump_Special[51] = UnknownOpcode;
    Jump_Special[52] = SPECIAL_TEQ;
    Jump_Special[53] = UnknownOpcode;
    Jump_Special[54] = UnknownOpcode;
    Jump_Special[55] = UnknownOpcode;
    Jump_Special[56] = SPECIAL_DSLL;
    Jump_Special[57] = UnknownOpcode;
    Jump_Special[58] = SPECIAL_DSRL;
    Jump_Special[59] = SPECIAL_DSRA;
    Jump_Special[60] = SPECIAL_DSLL32;
    Jump_Special[61] = UnknownOpcode;
    Jump_Special[62] = SPECIAL_DSRL32;
    Jump_Special[63] = SPECIAL_DSRA32;

    Jump_Regimm[0] = REGIMM_BLTZ;
    Jump_Regimm[1] = REGIMM_BGEZ;
    Jump_Regimm[2] = REGIMM_BLTZL;
    Jump_Regimm[3] = REGIMM_BGEZL;
    Jump_Regimm[4] = UnknownOpcode;
    Jump_Regimm[5] = UnknownOpcode;
    Jump_Regimm[6] = UnknownOpcode;
    Jump_Regimm[7] = UnknownOpcode;
    Jump_Regimm[8] = UnknownOpcode;
    Jump_Regimm[9] = UnknownOpcode;
    Jump_Regimm[10] = UnknownOpcode;
    Jump_Regimm[11] = UnknownOpcode;
    Jump_Regimm[12] = UnknownOpcode;
    Jump_Regimm[13] = UnknownOpcode;
    Jump_Regimm[14] = UnknownOpcode;
    Jump_Regimm[15] = UnknownOpcode;
    Jump_Regimm[16] = REGIMM_BLTZAL;
    Jump_Regimm[17] = REGIMM_BGEZAL;
    Jump_Regimm[18] = UnknownOpcode;
    Jump_Regimm[19] = UnknownOpcode;
    Jump_Regimm[20] = UnknownOpcode;
    Jump_Regimm[21] = UnknownOpcode;
    Jump_Regimm[22] = UnknownOpcode;
    Jump_Regimm[23] = UnknownOpcode;
    Jump_Regimm[24] = UnknownOpcode;
    Jump_Regimm[25] = UnknownOpcode;
    Jump_Regimm[26] = UnknownOpcode;
    Jump_Regimm[27] = UnknownOpcode;
    Jump_Regimm[28] = UnknownOpcode;
    Jump_Regimm[29] = UnknownOpcode;
    Jump_Regimm[30] = UnknownOpcode;
    Jump_Regimm[31] = UnknownOpcode;

    Jump_CoP0[0] = COP0_MF;
    Jump_CoP0[1] = UnknownOpcode;
    Jump_CoP0[2] = UnknownOpcode;
    Jump_CoP0[3] = UnknownOpcode;
    Jump_CoP0[4] = COP0_MT;
    Jump_CoP0[5] = UnknownOpcode;
    Jump_CoP0[6] = UnknownOpcode;
    Jump_CoP0[7] = UnknownOpcode;
    Jump_CoP0[8] = UnknownOpcode;
    Jump_CoP0[9] = UnknownOpcode;
    Jump_CoP0[10] = UnknownOpcode;
    Jump_CoP0[11] = UnknownOpcode;
    Jump_CoP0[12] = UnknownOpcode;
    Jump_CoP0[13] = UnknownOpcode;
    Jump_CoP0[14] = UnknownOpcode;
    Jump_CoP0[15] = UnknownOpcode;
    Jump_CoP0[16] = COP0_CO;
    Jump_CoP0[17] = COP0_CO;
    Jump_CoP0[18] = COP0_CO;
    Jump_CoP0[19] = COP0_CO;
    Jump_CoP0[20] = COP0_CO;
    Jump_CoP0[21] = COP0_CO;
    Jump_CoP0[22] = COP0_CO;
    Jump_CoP0[23] = COP0_CO;
    Jump_CoP0[24] = COP0_CO;
    Jump_CoP0[25] = COP0_CO;
    Jump_CoP0[26] = COP0_CO;
    Jump_CoP0[27] = COP0_CO;
    Jump_CoP0[28] = COP0_CO;
    Jump_CoP0[29] = COP0_CO;
    Jump_CoP0[30] = COP0_CO;
    Jump_CoP0[31] = COP0_CO;

    Jump_CoP0_Function[0] = UnknownOpcode;
    Jump_CoP0_Function[1] = COP0_CO_TLBR;
    Jump_CoP0_Function[2] = COP0_CO_TLBWI;
    Jump_CoP0_Function[3] = UnknownOpcode;
    Jump_CoP0_Function[4] = UnknownOpcode;
    Jump_CoP0_Function[5] = UnknownOpcode;
    Jump_CoP0_Function[6] = COP0_CO_TLBWR;
    Jump_CoP0_Function[7] = UnknownOpcode;
    Jump_CoP0_Function[8] = COP0_CO_TLBP;
    Jump_CoP0_Function[9] = UnknownOpcode;
    Jump_CoP0_Function[10] = UnknownOpcode;
    Jump_CoP0_Function[11] = UnknownOpcode;
    Jump_CoP0_Function[12] = UnknownOpcode;
    Jump_CoP0_Function[13] = UnknownOpcode;
    Jump_CoP0_Function[14] = UnknownOpcode;
    Jump_CoP0_Function[15] = UnknownOpcode;
    Jump_CoP0_Function[16] = UnknownOpcode;
    Jump_CoP0_Function[17] = UnknownOpcode;
    Jump_CoP0_Function[18] = UnknownOpcode;
    Jump_CoP0_Function[19] = UnknownOpcode;
    Jump_CoP0_Function[20] = UnknownOpcode;
    Jump_CoP0_Function[21] = UnknownOpcode;
    Jump_CoP0_Function[22] = UnknownOpcode;
    Jump_CoP0_Function[23] = UnknownOpcode;
    Jump_CoP0_Function[24] = COP0_CO_ERET;
    Jump_CoP0_Function[25] = UnknownOpcode;
    Jump_CoP0_Function[26] = UnknownOpcode;
    Jump_CoP0_Function[27] = UnknownOpcode;
    Jump_CoP0_Function[28] = UnknownOpcode;
    Jump_CoP0_Function[29] = UnknownOpcode;
    Jump_CoP0_Function[30] = UnknownOpcode;
    Jump_CoP0_Function[31] = UnknownOpcode;
    Jump_CoP0_Function[32] = UnknownOpcode;
    Jump_CoP0_Function[33] = UnknownOpcode;
    Jump_CoP0_Function[34] = UnknownOpcode;
    Jump_CoP0_Function[35] = UnknownOpcode;
    Jump_CoP0_Function[36] = UnknownOpcode;
    Jump_CoP0_Function[37] = UnknownOpcode;
    Jump_CoP0_Function[38] = UnknownOpcode;
    Jump_CoP0_Function[39] = UnknownOpcode;
    Jump_CoP0_Function[40] = UnknownOpcode;
    Jump_CoP0_Function[41] = UnknownOpcode;
    Jump_CoP0_Function[42] = UnknownOpcode;
    Jump_CoP0_Function[43] = UnknownOpcode;
    Jump_CoP0_Function[44] = UnknownOpcode;
    Jump_CoP0_Function[45] = UnknownOpcode;
    Jump_CoP0_Function[46] = UnknownOpcode;
    Jump_CoP0_Function[47] = UnknownOpcode;
    Jump_CoP0_Function[48] = UnknownOpcode;
    Jump_CoP0_Function[49] = UnknownOpcode;
    Jump_CoP0_Function[50] = UnknownOpcode;
    Jump_CoP0_Function[51] = UnknownOpcode;
    Jump_CoP0_Function[52] = UnknownOpcode;
    Jump_CoP0_Function[53] = UnknownOpcode;
    Jump_CoP0_Function[54] = UnknownOpcode;
    Jump_CoP0_Function[55] = UnknownOpcode;
    Jump_CoP0_Function[56] = UnknownOpcode;
    Jump_CoP0_Function[57] = UnknownOpcode;
    Jump_CoP0_Function[58] = UnknownOpcode;
    Jump_CoP0_Function[59] = UnknownOpcode;
    Jump_CoP0_Function[60] = UnknownOpcode;
    Jump_CoP0_Function[61] = UnknownOpcode;
    Jump_CoP0_Function[62] = UnknownOpcode;
    Jump_CoP0_Function[63] = UnknownOpcode;

    Jump_CoP1[0] = COP1_MF;
    Jump_CoP1[1] = COP1_DMF;
    Jump_CoP1[2] = COP1_CF;
    Jump_CoP1[3] = UnknownOpcode;
    Jump_CoP1[4] = COP1_MT;
    Jump_CoP1[5] = COP1_DMT;
    Jump_CoP1[6] = COP1_CT;
    Jump_CoP1[7] = UnknownOpcode;
    Jump_CoP1[8] = COP1_BC;
    Jump_CoP1[9] = UnknownOpcode;
    Jump_CoP1[10] = UnknownOpcode;
    Jump_CoP1[11] = UnknownOpcode;
    Jump_CoP1[12] = UnknownOpcode;
    Jump_CoP1[13] = UnknownOpcode;
    Jump_CoP1[14] = UnknownOpcode;
    Jump_CoP1[15] = UnknownOpcode;
    Jump_CoP1[16] = COP1_S;
    Jump_CoP1[17] = COP1_D;
    Jump_CoP1[18] = UnknownOpcode;
    Jump_CoP1[19] = UnknownOpcode;
    Jump_CoP1[20] = COP1_W;
    Jump_CoP1[21] = COP1_L;
    Jump_CoP1[22] = UnknownOpcode;
    Jump_CoP1[23] = UnknownOpcode;
    Jump_CoP1[24] = UnknownOpcode;
    Jump_CoP1[25] = UnknownOpcode;
    Jump_CoP1[26] = UnknownOpcode;
    Jump_CoP1[27] = UnknownOpcode;
    Jump_CoP1[28] = UnknownOpcode;
    Jump_CoP1[29] = UnknownOpcode;
    Jump_CoP1[30] = UnknownOpcode;
    Jump_CoP1[31] = UnknownOpcode;

    Jump_CoP1_BC[0] = COP1_BCF;
    Jump_CoP1_BC[1] = COP1_BCT;
    Jump_CoP1_BC[2] = COP1_BCFL;
    Jump_CoP1_BC[3] = COP1_BCTL;
    Jump_CoP1_BC[4] = UnknownOpcode;
    Jump_CoP1_BC[5] = UnknownOpcode;
    Jump_CoP1_BC[6] = UnknownOpcode;
    Jump_CoP1_BC[7] = UnknownOpcode;
    Jump_CoP1_BC[8] = UnknownOpcode;
    Jump_CoP1_BC[9] = UnknownOpcode;
    Jump_CoP1_BC[10] = UnknownOpcode;
    Jump_CoP1_BC[11] = UnknownOpcode;
    Jump_CoP1_BC[12] = UnknownOpcode;
    Jump_CoP1_BC[13] = UnknownOpcode;
    Jump_CoP1_BC[14] = UnknownOpcode;
    Jump_CoP1_BC[15] = UnknownOpcode;
    Jump_CoP1_BC[16] = UnknownOpcode;
    Jump_CoP1_BC[17] = UnknownOpcode;
    Jump_CoP1_BC[18] = UnknownOpcode;
    Jump_CoP1_BC[19] = UnknownOpcode;
    Jump_CoP1_BC[20] = UnknownOpcode;
    Jump_CoP1_BC[21] = UnknownOpcode;
    Jump_CoP1_BC[22] = UnknownOpcode;
    Jump_CoP1_BC[23] = UnknownOpcode;
    Jump_CoP1_BC[24] = UnknownOpcode;
    Jump_CoP1_BC[25] = UnknownOpcode;
    Jump_CoP1_BC[26] = UnknownOpcode;
    Jump_CoP1_BC[27] = UnknownOpcode;
    Jump_CoP1_BC[28] = UnknownOpcode;
    Jump_CoP1_BC[29] = UnknownOpcode;
    Jump_CoP1_BC[30] = UnknownOpcode;
    Jump_CoP1_BC[31] = UnknownOpcode;

    Jump_CoP1_S[0] = COP1_S_ADD;
    Jump_CoP1_S[1] = COP1_S_SUB;
    Jump_CoP1_S[2] = COP1_S_MUL;
    Jump_CoP1_S[3] = COP1_S_DIV;
    Jump_CoP1_S[4] = COP1_S_SQRT;
    Jump_CoP1_S[5] = COP1_S_ABS;
    Jump_CoP1_S[6] = COP1_S_MOV;
    Jump_CoP1_S[7] = COP1_S_NEG;
    Jump_CoP1_S[8] = COP1_S_ROUND_L;
    Jump_CoP1_S[9] = COP1_S_TRUNC_L;
    Jump_CoP1_S[10] = COP1_S_CEIL_L;
    Jump_CoP1_S[11] = COP1_S_FLOOR_L;
    Jump_CoP1_S[12] = COP1_S_ROUND_W;
    Jump_CoP1_S[13] = COP1_S_TRUNC_W;
    Jump_CoP1_S[14] = COP1_S_CEIL_W;
    Jump_CoP1_S[15] = COP1_S_FLOOR_W;
    Jump_CoP1_S[16] = UnknownOpcode;
    Jump_CoP1_S[17] = UnknownOpcode;
    Jump_CoP1_S[18] = UnknownOpcode;
    Jump_CoP1_S[19] = UnknownOpcode;
    Jump_CoP1_S[20] = UnknownOpcode;
    Jump_CoP1_S[21] = UnknownOpcode;
    Jump_CoP1_S[22] = UnknownOpcode;
    Jump_CoP1_S[23] = UnknownOpcode;
    Jump_CoP1_S[24] = UnknownOpcode;
    Jump_CoP1_S[25] = UnknownOpcode;
    Jump_CoP1_S[26] = UnknownOpcode;
    Jump_CoP1_S[27] = UnknownOpcode;
    Jump_CoP1_S[28] = UnknownOpcode;
    Jump_CoP1_S[29] = UnknownOpcode;
    Jump_CoP1_S[30] = UnknownOpcode;
    Jump_CoP1_S[31] = UnknownOpcode;
    Jump_CoP1_S[32] = UnknownOpcode;
    Jump_CoP1_S[33] = COP1_S_CVT_D;
    Jump_CoP1_S[34] = UnknownOpcode;
    Jump_CoP1_S[35] = UnknownOpcode;
    Jump_CoP1_S[36] = COP1_S_CVT_W;
    Jump_CoP1_S[37] = COP1_S_CVT_L;
    Jump_CoP1_S[38] = UnknownOpcode;
    Jump_CoP1_S[39] = UnknownOpcode;
    Jump_CoP1_S[40] = UnknownOpcode;
    Jump_CoP1_S[41] = UnknownOpcode;
    Jump_CoP1_S[42] = UnknownOpcode;
    Jump_CoP1_S[43] = UnknownOpcode;
    Jump_CoP1_S[44] = UnknownOpcode;
    Jump_CoP1_S[45] = UnknownOpcode;
    Jump_CoP1_S[46] = UnknownOpcode;
    Jump_CoP1_S[47] = UnknownOpcode;
    Jump_CoP1_S[48] = COP1_S_CMP;
    Jump_CoP1_S[49] = COP1_S_CMP;
    Jump_CoP1_S[50] = COP1_S_CMP;
    Jump_CoP1_S[51] = COP1_S_CMP;
    Jump_CoP1_S[52] = COP1_S_CMP;
    Jump_CoP1_S[53] = COP1_S_CMP;
    Jump_CoP1_S[54] = COP1_S_CMP;
    Jump_CoP1_S[55] = COP1_S_CMP;
    Jump_CoP1_S[56] = COP1_S_CMP;
    Jump_CoP1_S[57] = COP1_S_CMP;
    Jump_CoP1_S[58] = COP1_S_CMP;
    Jump_CoP1_S[59] = COP1_S_CMP;
    Jump_CoP1_S[60] = COP1_S_CMP;
    Jump_CoP1_S[61] = COP1_S_CMP;
    Jump_CoP1_S[62] = COP1_S_CMP;
    Jump_CoP1_S[63] = COP1_S_CMP;

    Jump_CoP1_D[0] = COP1_D_ADD;
    Jump_CoP1_D[1] = COP1_D_SUB;
    Jump_CoP1_D[2] = COP1_D_MUL;
    Jump_CoP1_D[3] = COP1_D_DIV;
    Jump_CoP1_D[4] = COP1_D_SQRT;
    Jump_CoP1_D[5] = COP1_D_ABS;
    Jump_CoP1_D[6] = COP1_D_MOV;
    Jump_CoP1_D[7] = COP1_D_NEG;
    Jump_CoP1_D[8] = COP1_D_ROUND_L;
    Jump_CoP1_D[9] = COP1_D_TRUNC_L;
    Jump_CoP1_D[10] = COP1_D_CEIL_L;
    Jump_CoP1_D[11] = COP1_D_FLOOR_L;
    Jump_CoP1_D[12] = COP1_D_ROUND_W;
    Jump_CoP1_D[13] = COP1_D_TRUNC_W;
    Jump_CoP1_D[14] = COP1_D_CEIL_W;
    Jump_CoP1_D[15] = COP1_D_FLOOR_W;
    Jump_CoP1_D[16] = UnknownOpcode;
    Jump_CoP1_D[17] = UnknownOpcode;
    Jump_CoP1_D[18] = UnknownOpcode;
    Jump_CoP1_D[19] = UnknownOpcode;
    Jump_CoP1_D[20] = UnknownOpcode;
    Jump_CoP1_D[21] = UnknownOpcode;
    Jump_CoP1_D[22] = UnknownOpcode;
    Jump_CoP1_D[23] = UnknownOpcode;
    Jump_CoP1_D[24] = UnknownOpcode;
    Jump_CoP1_D[25] = UnknownOpcode;
    Jump_CoP1_D[26] = UnknownOpcode;
    Jump_CoP1_D[27] = UnknownOpcode;
    Jump_CoP1_D[28] = UnknownOpcode;
    Jump_CoP1_D[29] = UnknownOpcode;
    Jump_CoP1_D[30] = UnknownOpcode;
    Jump_CoP1_D[31] = UnknownOpcode;
    Jump_CoP1_D[32] = COP1_D_CVT_S;
    Jump_CoP1_D[33] = UnknownOpcode;
    Jump_CoP1_D[34] = UnknownOpcode;
    Jump_CoP1_D[35] = UnknownOpcode;
    Jump_CoP1_D[36] = COP1_D_CVT_W;
    Jump_CoP1_D[37] = COP1_D_CVT_L;
    Jump_CoP1_D[38] = UnknownOpcode;
    Jump_CoP1_D[39] = UnknownOpcode;
    Jump_CoP1_D[40] = UnknownOpcode;
    Jump_CoP1_D[41] = UnknownOpcode;
    Jump_CoP1_D[42] = UnknownOpcode;
    Jump_CoP1_D[43] = UnknownOpcode;
    Jump_CoP1_D[44] = UnknownOpcode;
    Jump_CoP1_D[45] = UnknownOpcode;
    Jump_CoP1_D[46] = UnknownOpcode;
    Jump_CoP1_D[47] = UnknownOpcode;
    Jump_CoP1_D[48] = COP1_D_CMP;
    Jump_CoP1_D[49] = COP1_D_CMP;
    Jump_CoP1_D[50] = COP1_D_CMP;
    Jump_CoP1_D[51] = COP1_D_CMP;
    Jump_CoP1_D[52] = COP1_D_CMP;
    Jump_CoP1_D[53] = COP1_D_CMP;
    Jump_CoP1_D[54] = COP1_D_CMP;
    Jump_CoP1_D[55] = COP1_D_CMP;
    Jump_CoP1_D[56] = COP1_D_CMP;
    Jump_CoP1_D[57] = COP1_D_CMP;
    Jump_CoP1_D[58] = COP1_D_CMP;
    Jump_CoP1_D[59] = COP1_D_CMP;
    Jump_CoP1_D[60] = COP1_D_CMP;
    Jump_CoP1_D[61] = COP1_D_CMP;
    Jump_CoP1_D[62] = COP1_D_CMP;
    Jump_CoP1_D[63] = COP1_D_CMP;

    Jump_CoP1_W[0] = UnknownOpcode;
    Jump_CoP1_W[1] = UnknownOpcode;
    Jump_CoP1_W[2] = UnknownOpcode;
    Jump_CoP1_W[3] = UnknownOpcode;
    Jump_CoP1_W[4] = UnknownOpcode;
    Jump_CoP1_W[5] = UnknownOpcode;
    Jump_CoP1_W[6] = UnknownOpcode;
    Jump_CoP1_W[7] = UnknownOpcode;
    Jump_CoP1_W[8] = UnknownOpcode;
    Jump_CoP1_W[9] = UnknownOpcode;
    Jump_CoP1_W[10] = UnknownOpcode;
    Jump_CoP1_W[11] = UnknownOpcode;
    Jump_CoP1_W[12] = UnknownOpcode;
    Jump_CoP1_W[13] = UnknownOpcode;
    Jump_CoP1_W[14] = UnknownOpcode;
    Jump_CoP1_W[15] = UnknownOpcode;
    Jump_CoP1_W[16] = UnknownOpcode;
    Jump_CoP1_W[17] = UnknownOpcode;
    Jump_CoP1_W[18] = UnknownOpcode;
    Jump_CoP1_W[19] = UnknownOpcode;
    Jump_CoP1_W[20] = UnknownOpcode;
    Jump_CoP1_W[21] = UnknownOpcode;
    Jump_CoP1_W[22] = UnknownOpcode;
    Jump_CoP1_W[23] = UnknownOpcode;
    Jump_CoP1_W[24] = UnknownOpcode;
    Jump_CoP1_W[25] = UnknownOpcode;
    Jump_CoP1_W[26] = UnknownOpcode;
    Jump_CoP1_W[27] = UnknownOpcode;
    Jump_CoP1_W[28] = UnknownOpcode;
    Jump_CoP1_W[29] = UnknownOpcode;
    Jump_CoP1_W[30] = UnknownOpcode;
    Jump_CoP1_W[31] = UnknownOpcode;
    Jump_CoP1_W[32] = COP1_W_CVT_S;
    Jump_CoP1_W[33] = COP1_W_CVT_D;
    Jump_CoP1_W[34] = UnknownOpcode;
    Jump_CoP1_W[35] = UnknownOpcode;
    Jump_CoP1_W[36] = UnknownOpcode;
    Jump_CoP1_W[37] = UnknownOpcode;
    Jump_CoP1_W[38] = UnknownOpcode;
    Jump_CoP1_W[39] = UnknownOpcode;
    Jump_CoP1_W[40] = UnknownOpcode;
    Jump_CoP1_W[41] = UnknownOpcode;
    Jump_CoP1_W[42] = UnknownOpcode;
    Jump_CoP1_W[43] = UnknownOpcode;
    Jump_CoP1_W[44] = UnknownOpcode;
    Jump_CoP1_W[45] = UnknownOpcode;
    Jump_CoP1_W[46] = UnknownOpcode;
    Jump_CoP1_W[47] = UnknownOpcode;
    Jump_CoP1_W[48] = UnknownOpcode;
    Jump_CoP1_W[49] = UnknownOpcode;
    Jump_CoP1_W[50] = UnknownOpcode;
    Jump_CoP1_W[51] = UnknownOpcode;
    Jump_CoP1_W[52] = UnknownOpcode;
    Jump_CoP1_W[53] = UnknownOpcode;
    Jump_CoP1_W[54] = UnknownOpcode;
    Jump_CoP1_W[55] = UnknownOpcode;
    Jump_CoP1_W[56] = UnknownOpcode;
    Jump_CoP1_W[57] = UnknownOpcode;
    Jump_CoP1_W[58] = UnknownOpcode;
    Jump_CoP1_W[59] = UnknownOpcode;
    Jump_CoP1_W[60] = UnknownOpcode;
    Jump_CoP1_W[61] = UnknownOpcode;
    Jump_CoP1_W[62] = UnknownOpcode;
    Jump_CoP1_W[63] = UnknownOpcode;

    Jump_CoP1_L[0] = UnknownOpcode;
    Jump_CoP1_L[1] = UnknownOpcode;
    Jump_CoP1_L[2] = UnknownOpcode;
    Jump_CoP1_L[3] = UnknownOpcode;
    Jump_CoP1_L[4] = UnknownOpcode;
    Jump_CoP1_L[5] = UnknownOpcode;
    Jump_CoP1_L[6] = UnknownOpcode;
    Jump_CoP1_L[7] = UnknownOpcode;
    Jump_CoP1_L[8] = UnknownOpcode;
    Jump_CoP1_L[9] = UnknownOpcode;
    Jump_CoP1_L[10] = UnknownOpcode;
    Jump_CoP1_L[11] = UnknownOpcode;
    Jump_CoP1_L[12] = UnknownOpcode;
    Jump_CoP1_L[13] = UnknownOpcode;
    Jump_CoP1_L[14] = UnknownOpcode;
    Jump_CoP1_L[15] = UnknownOpcode;
    Jump_CoP1_L[16] = UnknownOpcode;
    Jump_CoP1_L[17] = UnknownOpcode;
    Jump_CoP1_L[18] = UnknownOpcode;
    Jump_CoP1_L[19] = UnknownOpcode;
    Jump_CoP1_L[20] = UnknownOpcode;
    Jump_CoP1_L[21] = UnknownOpcode;
    Jump_CoP1_L[22] = UnknownOpcode;
    Jump_CoP1_L[23] = UnknownOpcode;
    Jump_CoP1_L[24] = UnknownOpcode;
    Jump_CoP1_L[25] = UnknownOpcode;
    Jump_CoP1_L[26] = UnknownOpcode;
    Jump_CoP1_L[27] = UnknownOpcode;
    Jump_CoP1_L[28] = UnknownOpcode;
    Jump_CoP1_L[29] = UnknownOpcode;
    Jump_CoP1_L[30] = UnknownOpcode;
    Jump_CoP1_L[31] = UnknownOpcode;
    Jump_CoP1_L[32] = COP1_L_CVT_S;
    Jump_CoP1_L[33] = COP1_L_CVT_D;
    Jump_CoP1_L[34] = UnknownOpcode;
    Jump_CoP1_L[35] = UnknownOpcode;
    Jump_CoP1_L[36] = UnknownOpcode;
    Jump_CoP1_L[37] = UnknownOpcode;
    Jump_CoP1_L[38] = UnknownOpcode;
    Jump_CoP1_L[39] = UnknownOpcode;
    Jump_CoP1_L[40] = UnknownOpcode;
    Jump_CoP1_L[41] = UnknownOpcode;
    Jump_CoP1_L[42] = UnknownOpcode;
    Jump_CoP1_L[43] = UnknownOpcode;
    Jump_CoP1_L[44] = UnknownOpcode;
    Jump_CoP1_L[45] = UnknownOpcode;
    Jump_CoP1_L[46] = UnknownOpcode;
    Jump_CoP1_L[47] = UnknownOpcode;
    Jump_CoP1_L[48] = UnknownOpcode;
    Jump_CoP1_L[49] = UnknownOpcode;
    Jump_CoP1_L[50] = UnknownOpcode;
    Jump_CoP1_L[51] = UnknownOpcode;
    Jump_CoP1_L[52] = UnknownOpcode;
    Jump_CoP1_L[53] = UnknownOpcode;
    Jump_CoP1_L[54] = UnknownOpcode;
    Jump_CoP1_L[55] = UnknownOpcode;
    Jump_CoP1_L[56] = UnknownOpcode;
    Jump_CoP1_L[57] = UnknownOpcode;
    Jump_CoP1_L[58] = UnknownOpcode;
    Jump_CoP1_L[59] = UnknownOpcode;
    Jump_CoP1_L[60] = UnknownOpcode;
    Jump_CoP1_L[61] = UnknownOpcode;
    Jump_CoP1_L[62] = UnknownOpcode;
    Jump_CoP1_L[63] = UnknownOpcode;

    return Jump_Opcode;
}

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

void TestInterpreterJump(uint32_t PC, uint32_t TargetPC, int32_t Reg1, int32_t Reg2)
{
    if (PC != TargetPC)
    {
        return;
    }
    if (DelaySlotEffectsCompare(PC, Reg1, Reg2))
    {
        return;
    }
    R4300iOp::m_NextInstruction = PERMLOOP_DO_DELAY;
    R4300iOp::m_TestTimer = true;
}

/************************* Opcode functions *************************/
void R4300iOp::J()
{
    m_NextInstruction = DELAY_SLOT;
    m_JumpToLocation = ((*_PROGRAM_COUNTER) & 0xF0000000) + (m_Opcode.target << 2);
    if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
    {
        m_NextInstruction = PERMLOOP_DO_DELAY;
    }
}

void R4300iOp::JAL()
{
    m_NextInstruction = DELAY_SLOT;
    m_JumpToLocation = ((*_PROGRAM_COUNTER) & 0xF0000000) + (m_Opcode.target << 2);
    _GPR[31].DW = (int32_t)((*_PROGRAM_COUNTER) + 8);

    if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
    {
        m_NextInstruction = PERMLOOP_DO_DELAY;
    }
}

void R4300iOp::BEQ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, m_Opcode.rt))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BNE()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, m_Opcode.rt))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BLEZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW <= 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, 0))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BGTZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW > 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, 0))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::ADDI()
{
#ifdef Interpreter_StackTest
    if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        StackValue += (int16_t)m_Opcode.immediate;
    }
#endif
    _GPR[m_Opcode.rt].DW = (_GPR[m_Opcode.rs].W[0] + ((int16_t)m_Opcode.immediate));
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::ADDIU()
{
#ifdef Interpreter_StackTest
    if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        StackValue += (int16_t)m_Opcode.immediate;
    }
#endif
    _GPR[m_Opcode.rt].DW = (_GPR[m_Opcode.rs].W[0] + ((int16_t)m_Opcode.immediate));
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::SLTI()
{
    if (_GPR[m_Opcode.rs].DW < (int64_t)((int16_t)m_Opcode.immediate))
    {
        _GPR[m_Opcode.rt].DW = 1;
    }
    else
    {
        _GPR[m_Opcode.rt].DW = 0;
    }
}

void R4300iOp::SLTIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].UDW < (uint64_t)imm64 ? 1 : 0;
}

void R4300iOp::ANDI()
{
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW & m_Opcode.immediate;
}

void R4300iOp::ORI()
{
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW | m_Opcode.immediate;
}

void R4300iOp::XORI()
{
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW ^ m_Opcode.immediate;
}

void R4300iOp::LUI()
{
    _GPR[m_Opcode.rt].DW = (int32_t)((int16_t)m_Opcode.offset << 16);
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::BEQL()
{
    if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, m_Opcode.rt))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BNEL()
{
    if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, m_Opcode.rt))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BLEZL()
{
    if (_GPR[m_Opcode.rs].DW <= 0)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, 0))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BGTZL()
{
    if (_GPR[m_Opcode.rs].DW > 0)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER, m_Opcode.rs, 0))
            {
                m_NextInstruction = PERMLOOP_DO_DELAY;
            }
        }
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::DADDIU()
{
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW + (int64_t)((int16_t)m_Opcode.immediate);
}

uint64_t LDL_MASK[8] = { 0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFF, 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF };
int32_t LDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void R4300iOp::LDL()
{
    uint32_t Offset, Address;
    uint64_t Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 7;

    if (!g_MMU->LD_VAddr((Address & ~7), Value))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].DW & LDL_MASK[Offset];
    _GPR[m_Opcode.rt].DW += Value << LDL_SHIFT[Offset];
}

uint64_t LDR_MASK[8] = { 0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000,
    0xFFFFFFFFFF000000, 0xFFFFFFFF00000000,
    0xFFFFFF0000000000, 0xFFFF000000000000,
    0xFF00000000000000, 0 };
int32_t LDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

void R4300iOp::LDR()
{
    uint32_t Offset, Address;
    uint64_t Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 7;

    if (!g_MMU->LD_VAddr((Address & ~7), Value))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].DW & LDR_MASK[Offset];
    _GPR[m_Opcode.rt].DW += Value >> LDR_SHIFT[Offset];
}

void R4300iOp::LB()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if (!g_MMU->LB_VAddr(Address, _GPR[m_Opcode.rt].UB[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].B[0];
    }
}

void R4300iOp::LH()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 1) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }
    if (!g_MMU->LH_VAddr(Address, _GPR[m_Opcode.rt].UHW[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].HW[0];
    }
}

void R4300iOp::LWL()
{
    uint32_t Offset, Address, Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 3;

    if (!g_MMU->LW_VAddr((Address & ~3), Value))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    _GPR[m_Opcode.rt].DW = (int32_t)(_GPR[m_Opcode.rt].W[0] & LWL_MASK[Offset]);
    _GPR[m_Opcode.rt].DW += (int32_t)(Value << LWL_SHIFT[Offset]);
}

void R4300iOp::LW()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }

    if (GenerateLog())
    {
        Log_LW((*_PROGRAM_COUNTER), Address);
    }

    if (!g_MMU->LW_VAddr(Address, _GPR[m_Opcode.rt].UW[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].W[0];
    }
}

void R4300iOp::LBU()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if (!g_MMU->LB_VAddr(Address, _GPR[m_Opcode.rt].UB[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].UDW = _GPR[m_Opcode.rt].UB[0];
    }
}

void R4300iOp::LHU()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 1) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }
    if (!g_MMU->LH_VAddr(Address, _GPR[m_Opcode.rt].UHW[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].UDW = _GPR[m_Opcode.rt].UHW[0];
    }
}

void R4300iOp::LWR()
{
    uint32_t Offset, Address, Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 3;

    if (!g_MMU->LW_VAddr((Address & ~3), Value))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    _GPR[m_Opcode.rt].DW = (int32_t)(_GPR[m_Opcode.rt].W[0] & LWR_MASK[Offset]);
    _GPR[m_Opcode.rt].DW += (int32_t)(Value >> LWR_SHIFT[Offset]);
}

void R4300iOp::LWU()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }

    if (!g_MMU->LW_VAddr(Address, _GPR[m_Opcode.rt].UW[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].UDW = _GPR[m_Opcode.rt].UW[0];
    }
}

void R4300iOp::SB()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if (!g_MMU->SB_VAddr(Address, _GPR[m_Opcode.rt].UB[0]))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SH()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 1) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, false);
    }
    if (!g_MMU->SH_VAddr(Address, _GPR[m_Opcode.rt].UHW[0]))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SWL()
{
    uint32_t Offset, Address, Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 3;

    if (!g_MMU->LW_VAddr((Address & ~3), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    Value &= SWL_MASK[Offset];
    Value += _GPR[m_Opcode.rt].UW[0] >> SWL_SHIFT[Offset];

    if (!g_MMU->SW_VAddr((Address & ~0x03), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SW()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, false);
    }
    if (GenerateLog())
    {
        Log_SW((*_PROGRAM_COUNTER), Address, _GPR[m_Opcode.rt].UW[0]);
    }
    if (!g_MMU->SW_VAddr(Address, _GPR[m_Opcode.rt].UW[0]))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

uint64_t SDL_MASK[8] = { 0, 0xFF00000000000000,
    0xFFFF000000000000,
    0xFFFFFF0000000000,
    0xFFFFFFFF00000000,
    0xFFFFFFFFFF000000,
    0xFFFFFFFFFFFF0000,
    0xFFFFFFFFFFFFFF00 };

int32_t SDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void R4300iOp::SDL()
{
    uint32_t Offset, Address;
    uint64_t Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 7;

    if (!g_MMU->LD_VAddr((Address & ~7), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    Value &= SDL_MASK[Offset];
    Value += _GPR[m_Opcode.rt].UDW >> SDL_SHIFT[Offset];

    if (!g_MMU->SD_VAddr((Address & ~7), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

uint64_t SDR_MASK[8] = { 0x00FFFFFFFFFFFFFF,
    0x0000FFFFFFFFFFFF,
    0x000000FFFFFFFFFF,
    0x00000000FFFFFFFF,
    0x0000000000FFFFFF,
    0x000000000000FFFF,
    0x00000000000000FF,
    0x0000000000000000 };

int32_t SDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

void R4300iOp::SDR()
{
    uint32_t Offset, Address;
    uint64_t Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 7;

    if (!g_MMU->LD_VAddr((Address & ~7), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    Value &= SDR_MASK[Offset];
    Value += _GPR[m_Opcode.rt].UDW << SDR_SHIFT[Offset];

    if (!g_MMU->SD_VAddr((Address & ~7), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SWR()
{
    uint32_t Offset, Address, Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 3;

    if (!g_MMU->LW_VAddr((Address & ~3), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }

    Value &= SWR_MASK[Offset];
    Value += _GPR[m_Opcode.rt].UW[0] << SWR_SHIFT[Offset];

    if (!g_MMU->SW_VAddr((Address & ~0x03), Value))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::CACHE()
{
    if (!LogCache())
    {
        return;
    }
    LogMessage("%08X: Cache operation %d, 0x%08X", (*_PROGRAM_COUNTER), m_Opcode.rt, _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset);
}

void R4300iOp::LL()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }

    if (!g_MMU->LW_VAddr(Address, _GPR[m_Opcode.rt].UW[0]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].W[0];
        (*_LLBit) = 1;
    }
}

void R4300iOp::LWC1()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (uint32_t)((int16_t)m_Opcode.offset);
    TEST_COP1_USABLE_EXCEPTION();
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }
    if (!g_MMU->LW_VAddr(Address, *(uint32_t *)_FPR_S[m_Opcode.ft]))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
    }
}

void R4300iOp::SC()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, false);
    }
    Log_SW((*_PROGRAM_COUNTER), Address, _GPR[m_Opcode.rt].UW[0]);
    if ((*_LLBit) == 1)
    {
        if (!g_MMU->SW_VAddr(Address, _GPR[m_Opcode.rt].UW[0]))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            if (bShowTLBMisses())
            {
                g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
            }
        }
    }
    _GPR[m_Opcode.rt].UW[0] = (*_LLBit);
}

void R4300iOp::LD()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 7) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }
    if (!g_MMU->LD_VAddr(Address, _GPR[m_Opcode.rt].UDW))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        return;
    }
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::LDC1()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;

    TEST_COP1_USABLE_EXCEPTION();
    if ((Address & 7) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, true);
    }
    if (!g_MMU->LD_VAddr(Address, *(uint64_t *)_FPR_D[m_Opcode.ft]))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SWC1()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    TEST_COP1_USABLE_EXCEPTION();
    if ((Address & 3) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, false);
    }

    if (!g_MMU->SW_VAddr(Address, *(uint32_t *)_FPR_S[m_Opcode.ft]))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SDC1()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;

    TEST_COP1_USABLE_EXCEPTION();
    if ((Address & 7) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, false);
    }
    if (!g_MMU->SD_VAddr(Address, *(int64_t *)_FPR_D[m_Opcode.ft]))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}

void R4300iOp::SD()
{
    uint32_t Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    if ((Address & 7) != 0)
    {
        ADDRESS_ERROR_EXCEPTION(Address, false);
    }
    if (!g_MMU->SD_VAddr(Address, _GPR[m_Opcode.rt].UDW))
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
    }
}
/********************** R4300i OpCodes: Special **********************/
void R4300iOp::SPECIAL_SLL()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] << m_Opcode.sa);
}

void R4300iOp::SPECIAL_SRL()
{
    _GPR[m_Opcode.rd].DW = (int32_t)(_GPR[m_Opcode.rt].UW[0] >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_SRA()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_SLLV()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] << (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_SRLV()
{
    _GPR[m_Opcode.rd].DW = (int32_t)(_GPR[m_Opcode.rt].UW[0] >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_SRAV()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_JR()
{
    m_NextInstruction = DELAY_SLOT;
    m_JumpToLocation = _GPR[m_Opcode.rs].UW[0];
    m_TestTimer = true;
}

void R4300iOp::SPECIAL_JALR()
{
    m_NextInstruction = DELAY_SLOT;
    m_JumpToLocation = _GPR[m_Opcode.rs].UW[0];
    _GPR[m_Opcode.rd].DW = (int32_t)((*_PROGRAM_COUNTER) + 8);
    m_TestTimer = true;
}

void R4300iOp::SPECIAL_SYSCALL()
{
    g_Reg->DoSysCallException(m_NextInstruction == JUMP);
    m_NextInstruction = JUMP;
    m_JumpToLocation = (*_PROGRAM_COUNTER);
}

void R4300iOp::SPECIAL_BREAK()
{
    g_Reg->DoBreakException(m_NextInstruction == JUMP);
    m_NextInstruction = JUMP;
    m_JumpToLocation = (*_PROGRAM_COUNTER);
}

void R4300iOp::SPECIAL_SYNC()
{
}

void R4300iOp::SPECIAL_MFHI()
{
    _GPR[m_Opcode.rd].DW = _RegHI->DW;
}

void R4300iOp::SPECIAL_MTHI()
{
    _RegHI->DW = _GPR[m_Opcode.rs].DW;
}

void R4300iOp::SPECIAL_MFLO()
{
    _GPR[m_Opcode.rd].DW = _RegLO->DW;
}

void R4300iOp::SPECIAL_MTLO()
{
    _RegLO->DW = _GPR[m_Opcode.rs].DW;
}

void R4300iOp::SPECIAL_DSLLV()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rt].DW << (_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_DSRLV()
{
    _GPR[m_Opcode.rd].UDW = _GPR[m_Opcode.rt].UDW >> (_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_DSRAV()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rt].DW >> (_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_MULT()
{
    _RegHI->DW = (int64_t)(_GPR[m_Opcode.rs].W[0]) * (int64_t)(_GPR[m_Opcode.rt].W[0]);
    _RegLO->DW = _RegHI->W[0];
    _RegHI->DW = _RegHI->W[1];
}

void R4300iOp::SPECIAL_MULTU()
{
    _RegHI->DW = (uint64_t)(_GPR[m_Opcode.rs].UW[0]) * (uint64_t)(_GPR[m_Opcode.rt].UW[0]);
    _RegLO->DW = _RegHI->W[0];
    _RegHI->DW = _RegHI->W[1];
}

void R4300iOp::SPECIAL_DIV()
{
    if (_GPR[m_Opcode.rt].W[0] != 0)
    {
        _RegLO->DW = _GPR[m_Opcode.rs].W[0] / _GPR[m_Opcode.rt].W[0];
        _RegHI->DW = _GPR[m_Opcode.rs].W[0] % _GPR[m_Opcode.rt].W[0];
    }
    else
    {
        if (bShowDivByZero())
        {
            g_Notify->DisplayError("DIV by 0 ???");
        }
        _RegLO->DW = 0;
        _RegHI->DW = 0;
    }
}

void R4300iOp::SPECIAL_DIVU()
{
    if (_GPR[m_Opcode.rt].UW[0] != 0)
    {
        _RegLO->DW = (int32_t)(_GPR[m_Opcode.rs].UW[0] / _GPR[m_Opcode.rt].UW[0]);
        _RegHI->DW = (int32_t)(_GPR[m_Opcode.rs].UW[0] % _GPR[m_Opcode.rt].UW[0]);
    }
    else
    {
        if (bShowDivByZero())
        {
            g_Notify->DisplayError("DIVU by 0 ???");
        }
        _RegLO->DW = 0;
        _RegHI->DW = 0;
    }
}

void R4300iOp::SPECIAL_DMULT()
{
    MIPS_DWORD Tmp[3];

    _RegLO->UDW = (uint64_t)_GPR[m_Opcode.rs].UW[0] * (uint64_t)_GPR[m_Opcode.rt].UW[0];
    Tmp[0].UDW = (int64_t)_GPR[m_Opcode.rs].W[1] * (int64_t)(uint64_t)_GPR[m_Opcode.rt].UW[0];
    Tmp[1].UDW = (int64_t)(uint64_t)_GPR[m_Opcode.rs].UW[0] * (int64_t)_GPR[m_Opcode.rt].W[1];
    _RegHI->UDW = (int64_t)_GPR[m_Opcode.rs].W[1] * (int64_t)_GPR[m_Opcode.rt].W[1];

    Tmp[2].UDW = (uint64_t)_RegLO->UW[1] + (uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0];
    _RegLO->UDW += ((uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0]) << 32;
    _RegHI->UDW += (uint64_t)Tmp[0].W[1] + (uint64_t)Tmp[1].W[1] + Tmp[2].UW[1];
}

void R4300iOp::SPECIAL_DMULTU()
{
    MIPS_DWORD Tmp[3];

    _RegLO->UDW = (uint64_t)_GPR[m_Opcode.rs].UW[0] * (uint64_t)_GPR[m_Opcode.rt].UW[0];
    Tmp[0].UDW = (uint64_t)_GPR[m_Opcode.rs].UW[1] * (uint64_t)_GPR[m_Opcode.rt].UW[0];
    Tmp[1].UDW = (uint64_t)_GPR[m_Opcode.rs].UW[0] * (uint64_t)_GPR[m_Opcode.rt].UW[1];
    _RegHI->UDW = (uint64_t)_GPR[m_Opcode.rs].UW[1] * (uint64_t)_GPR[m_Opcode.rt].UW[1];

    Tmp[2].UDW = (uint64_t)_RegLO->UW[1] + (uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0];
    _RegLO->UDW += ((uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0]) << 32;
    _RegHI->UDW += (uint64_t)Tmp[0].UW[1] + (uint64_t)Tmp[1].UW[1] + Tmp[2].UW[1];
}

void R4300iOp::SPECIAL_DDIV()
{
    if (_GPR[m_Opcode.rt].UDW != 0)
    {
        _RegLO->DW = _GPR[m_Opcode.rs].DW / _GPR[m_Opcode.rt].DW;
        _RegHI->DW = _GPR[m_Opcode.rs].DW % _GPR[m_Opcode.rt].DW;
    }
    else
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError("DDIV by 0 ???");
        }
    }
}

void R4300iOp::SPECIAL_DDIVU()
{
    if (_GPR[m_Opcode.rt].UDW != 0)
    {
        _RegLO->UDW = _GPR[m_Opcode.rs].UDW / _GPR[m_Opcode.rt].UDW;
        _RegHI->UDW = _GPR[m_Opcode.rs].UDW % _GPR[m_Opcode.rt].UDW;
    }
    else
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError("DDIVU by 0 ???");
        }
    }
}

void R4300iOp::SPECIAL_ADD()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_ADDU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_SUB()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] - _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_SUBU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] - _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_AND()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW & _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_OR()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW | _GPR[m_Opcode.rt].DW;
#ifdef Interpreter_StackTest
    if (m_Opcode.rd == 29)
    {
        StackValue = _GPR[m_Opcode.rd].W[0];
    }
#endif
}

void R4300iOp::SPECIAL_XOR()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW ^ _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_NOR()
{
    _GPR[m_Opcode.rd].DW = ~(_GPR[m_Opcode.rs].DW | _GPR[m_Opcode.rt].DW);
}

void R4300iOp::SPECIAL_SLT()
{
    if (_GPR[m_Opcode.rs].DW < _GPR[m_Opcode.rt].DW)
    {
        _GPR[m_Opcode.rd].DW = 1;
    }
    else
    {
        _GPR[m_Opcode.rd].DW = 0;
    }
}

void R4300iOp::SPECIAL_SLTU()
{
    if (_GPR[m_Opcode.rs].UDW < _GPR[m_Opcode.rt].UDW)
    {
        _GPR[m_Opcode.rd].DW = 1;
    }
    else
    {
        _GPR[m_Opcode.rd].DW = 0;
    }
}

void R4300iOp::SPECIAL_DADD()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW + _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DADDU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW + _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DSUB()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW - _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DSUBU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW - _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_TEQ()
{
    if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW && bHaveDebugger())
    {
        g_Notify->DisplayError("Should trap this ???");
    }
}

void R4300iOp::SPECIAL_DSLL()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW << m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSRL()
{
    _GPR[m_Opcode.rd].UDW = (_GPR[m_Opcode.rt].UDW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSRA()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSLL32()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW << (m_Opcode.sa + 32));
}

void R4300iOp::SPECIAL_DSRL32()
{
    _GPR[m_Opcode.rd].UDW = (_GPR[m_Opcode.rt].UDW >> (m_Opcode.sa + 32));
}

void R4300iOp::SPECIAL_DSRA32()
{
    _GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW >> (m_Opcode.sa + 32));
}

/********************** R4300i OpCodes: RegImm **********************/
void R4300iOp::REGIMM_BLTZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW < 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER), m_Opcode.rs, 0))
            {
                CInterpreterCPU::InPermLoop();
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BGEZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER), m_Opcode.rs, 0))
            {
                CInterpreterCPU::InPermLoop();
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BLTZL()
{
    if (_GPR[m_Opcode.rs].DW < 0)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER), m_Opcode.rs, 0))
            {
                CInterpreterCPU::InPermLoop();
            }
        }
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BGEZL()
{
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER), m_Opcode.rs, 0))
            {
                CInterpreterCPU::InPermLoop();
            }
        }
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BLTZAL()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW < 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER), m_Opcode.rs, 0))
            {
                CInterpreterCPU::InPermLoop();
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
    _GPR[31].DW = (int32_t)((*_PROGRAM_COUNTER) + 8);
}

void R4300iOp::REGIMM_BGEZAL()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
            if (CDebugSettings::bHaveDebugger())
            {
                if (g_Reg->m_PROGRAM_COUNTER < 0x80000400)
                {
                    // Break out of possible checksum halt
                    g_Notify->DisplayMessage(5, "Broke out of permanent loop! Invalid checksum?");
                    m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
                    _GPR[31].DW = (int32_t)((*_PROGRAM_COUNTER) + 8);
                    R4300iOp::m_NextInstruction = DELAY_SLOT;
                    return;
                }
            }
            if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER), m_Opcode.rs, 0))
            {
                CInterpreterCPU::InPermLoop();
            }
        }
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
    _GPR[31].DW = (int32_t)((*_PROGRAM_COUNTER) + 8);
}
/************************** COP0 functions **************************/
void R4300iOp::COP0_MF()
{
    if (LogCP0reads())
    {
        LogMessage("%08X: R4300i Read from %s (0x%08X)", (*_PROGRAM_COUNTER), CRegName::Cop0[m_Opcode.rd], _CP0[m_Opcode.rd]);
    }

    if (m_Opcode.rd == 9)
    {
        g_SystemTimer->UpdateTimers();
    }
    _GPR[m_Opcode.rt].DW = (int32_t)_CP0[m_Opcode.rd];
}

void R4300iOp::COP0_MT()
{
    if (LogCP0changes())
    {
        LogMessage("%08X: Writing 0x%X to %s register (Originally: 0x%08X)", (*_PROGRAM_COUNTER), _GPR[m_Opcode.rt].UW[0], CRegName::Cop0[m_Opcode.rd], _CP0[m_Opcode.rd]);
        if (m_Opcode.rd == 11)  //Compare
        {
            LogMessage("%08X: Cause register changed from %08X to %08X", (*_PROGRAM_COUNTER), g_Reg->CAUSE_REGISTER, (g_Reg->CAUSE_REGISTER & ~CAUSE_IP7));
        }
    }

    switch (m_Opcode.rd)
    {
    case 0: //Index
    case 2: //EntryLo0
    case 3: //EntryLo1
    case 5: //PageMask
    case 10: //Entry Hi
    case 14: //EPC
    case 16: //Config
    case 18: //WatchLo
    case 19: //WatchHi
    case 28: //Tag lo
    case 29: //Tag Hi
    case 30: //ErrEPC
        _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
        break;
    case 6: //Wired
        g_SystemTimer->UpdateTimers();
        _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
        break;
    case 4: //Context
        _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0] & 0xFF800000;
        break;
    case 9: //Count
        g_SystemTimer->UpdateTimers();
        _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
        g_SystemTimer->UpdateCompareTimer();
        break;
    case 11: //Compare
        g_SystemTimer->UpdateTimers();
        _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
        g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP7;
        g_SystemTimer->UpdateCompareTimer();
        break;
    case 12: //Status
        if ((_CP0[m_Opcode.rd] & STATUS_FR) != (_GPR[m_Opcode.rt].UW[0] & STATUS_FR))
        {
            _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
            g_Reg->FixFpuLocations();
        }
        else
        {
            _CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
        }
        if ((_CP0[m_Opcode.rd] & 0x18) != 0 && bHaveDebugger())
        {
            g_Notify->DisplayError("Left kernel mode ??");
        }
        g_Reg->CheckInterrupts();
        break;
    case 13: //cause
        _CP0[m_Opcode.rd] &= 0xFFFFCFF;
        if ((_GPR[m_Opcode.rt].UW[0] & 0x300) != 0 && bHaveDebugger())
        {
            g_Notify->DisplayError("Set IP0 or IP1");
        }
        break;
    default:
        UnknownOpcode();
    }
}

/************************** COP0 CO functions ***********************/
void R4300iOp::COP0_CO_TLBR()
{
    if (!g_System->bUseTlb())
    {
        return;
    }
    g_TLB->ReadEntry();
}

void R4300iOp::COP0_CO_TLBWI()
{
    if (!g_System->bUseTlb())
    {
        return;
    }
    g_TLB->WriteEntry(g_Reg->INDEX_REGISTER & 0x1F, false);
}

void R4300iOp::COP0_CO_TLBWR()
{
    if (!g_System->bUseTlb())
    {
        return;
    }
    g_TLB->WriteEntry(g_Reg->RANDOM_REGISTER & 0x1F, true);
}

void R4300iOp::COP0_CO_TLBP()
{
    if (!g_System->bUseTlb())
    {
        return;
    }
    g_TLB->Probe();
}

void R4300iOp::COP0_CO_ERET()
{
    m_NextInstruction = JUMP;
    if ((g_Reg->STATUS_REGISTER & STATUS_ERL) != 0)
    {
        m_JumpToLocation = g_Reg->ERROREPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_ERL;
    }
    else
    {
        m_JumpToLocation = g_Reg->EPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_EXL;
    }
    (*_LLBit) = 0;
    g_Reg->CheckInterrupts();
    m_TestTimer = true;
}

/************************** COP1 functions **************************/
void R4300iOp::COP1_MF()
{
    TEST_COP1_USABLE_EXCEPTION();
    _GPR[m_Opcode.rt].DW = *(int32_t *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_DMF()
{
    TEST_COP1_USABLE_EXCEPTION();
    _GPR[m_Opcode.rt].DW = *(int64_t *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_CF()
{
    TEST_COP1_USABLE_EXCEPTION();
    if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError("CFC1 what register are you writing to ?");
        }
        return;
    }
    _GPR[m_Opcode.rt].DW = (int32_t)_FPCR[m_Opcode.fs];
}

void R4300iOp::COP1_MT()
{
    TEST_COP1_USABLE_EXCEPTION();
    *(int32_t *)_FPR_S[m_Opcode.fs] = _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::COP1_DMT()
{
    TEST_COP1_USABLE_EXCEPTION();
    *(int64_t *)_FPR_D[m_Opcode.fs] = _GPR[m_Opcode.rt].DW;
}

void R4300iOp::COP1_CT()
{
    TEST_COP1_USABLE_EXCEPTION();
    if (m_Opcode.fs == 31)
    {
        _FPCR[m_Opcode.fs] = _GPR[m_Opcode.rt].W[0];
        switch ((_FPCR[m_Opcode.fs] & 3))
        {
        case 0: *_RoundingModel = FE_TONEAREST; break;
        case 1: *_RoundingModel = FE_TOWARDZERO; break;
        case 2: *_RoundingModel = FE_UPWARD; break;
        case 3: *_RoundingModel = FE_DOWNWARD; break;
        }
        return;
    }
    if (bHaveDebugger())
    {
        g_Notify->DisplayError("CTC1 what register are you writing to ?");
    }
}

/************************* COP1: BC1 functions ***********************/
void R4300iOp::COP1_BCF()
{
    TEST_COP1_USABLE_EXCEPTION();
    m_NextInstruction = DELAY_SLOT;
    if ((_FPCR[31] & FPCSR_C) == 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCT()
{
    TEST_COP1_USABLE_EXCEPTION();
    m_NextInstruction = DELAY_SLOT;
    if ((_FPCR[31] & FPCSR_C) != 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCFL()
{
    TEST_COP1_USABLE_EXCEPTION();
    if ((_FPCR[31] & FPCSR_C) == 0)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCTL()
{
    TEST_COP1_USABLE_EXCEPTION();
    if ((_FPCR[31] & FPCSR_C) != 0)
    {
        m_NextInstruction = DELAY_SLOT;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_NextInstruction = JUMP;
        m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}
/************************** COP1: S functions ************************/
__inline void Float_RoundToInteger32(int32_t * Dest, const float * Source, int RoundType)
{
#pragma warning(push)
#pragma warning(disable:4244) //warning C4244: disabe conversion from 'float' to 'int32_t', possible loss of data

    if (RoundType == FE_TONEAREST)
    {
        float reminder = *Source - floorf(*Source);
        if (reminder == 0.5)
        {
            //make any decimal point in even to go to odd and any decimal point in odd stay as odd
            if (*Source < 0)
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? floorf(*Source) : ceilf(*Source);
            }
            else
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? ceilf(*Source) : floorf(*Source);
            }
        }
        else
        {
            *Dest = roundf(*Source);
        }
    }
    else if (RoundType == FE_TOWARDZERO) { *Dest = truncf(*Source); }
    else if (RoundType == FE_UPWARD) { *Dest = ceilf(*Source); }
    else if (RoundType == FE_DOWNWARD) { *Dest = floorf(*Source); }

#pragma warning(pop)
}

__inline void Float_RoundToInteger64(int64_t * Dest, const float * Source, int RoundType)
{
#pragma warning(push)
#pragma warning(disable:4244) //warning C4244: disabe conversion from 'float' to 'int64_t', possible loss of data

    if (RoundType == FE_TONEAREST)
    {
        float reminder = *Source - floorf(*Source);
        if (reminder == 0.5)
        {
            //make any decimal point in even to go to odd and any decimal point in odd stay as odd
            if (*Source < 0)
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? floorf(*Source) : ceilf(*Source);
            }
            else
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? ceilf(*Source) : floorf(*Source);
            }
        }
        else
        {
            *Dest = roundf(*Source);
        }
    }
    else if (RoundType == FE_TOWARDZERO) { *Dest = truncf(*Source); }
    else if (RoundType == FE_UPWARD) { *Dest = ceilf(*Source); }
    else if (RoundType == FE_DOWNWARD) { *Dest = floorf(*Source); }

#pragma warning(pop)
}

void R4300iOp::COP1_S_ADD()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] + *(float *)_FPR_S[m_Opcode.ft]);
}

void R4300iOp::COP1_S_SUB()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] - *(float *)_FPR_S[m_Opcode.ft]);
}

void R4300iOp::COP1_S_MUL()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] * *(float *)_FPR_S[m_Opcode.ft]);
}

void R4300iOp::COP1_S_DIV()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] / *(float *)_FPR_S[m_Opcode.ft]);
}

void R4300iOp::COP1_S_SQRT()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);

    *(float *)(_FPR_S[m_Opcode.fd]) = sqrtf(*(float *)(_FPR_S[m_Opcode.fs]));
}

void R4300iOp::COP1_S_ABS()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (float)fabs(*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_MOV()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = *(float *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_S_NEG()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] * -1.0f);
}

void R4300iOp::COP1_S_ROUND_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_TONEAREST);
}

void R4300iOp::COP1_S_TRUNC_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_TOWARDZERO);
}

void R4300iOp::COP1_S_CEIL_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_UPWARD);
}

void R4300iOp::COP1_S_FLOOR_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_DOWNWARD);
}

void R4300iOp::COP1_S_ROUND_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_TONEAREST);
}

void R4300iOp::COP1_S_TRUNC_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_TOWARDZERO);
}

void R4300iOp::COP1_S_CEIL_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_UPWARD);
}

void R4300iOp::COP1_S_FLOOR_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], FE_DOWNWARD);
}

void R4300iOp::COP1_S_CVT_D()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = (double)(*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CVT_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], *_RoundingModel);
}

void R4300iOp::COP1_S_CVT_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Float_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(float *)_FPR_S[m_Opcode.fs], *_RoundingModel);
}

void R4300iOp::COP1_S_CMP()
{
    bool less, equal, unorded;
    int32_t condition;

    TEST_COP1_USABLE_EXCEPTION();

    float Temp0 = *(float *)_FPR_S[m_Opcode.fs];
    float Temp1 = *(float *)_FPR_S[m_Opcode.ft];

    if (_isnan(Temp0) || _isnan(Temp1))
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s: Nan ?", __FUNCTION__).c_str());
        }
        less = false;
        equal = false;
        unorded = true;
        if ((m_Opcode.funct & 8) != 0)
        {
            if (bHaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("Signal InvalidOperationException\nin r4300i_COP1_S_CMP\n%X  %ff\n%X  %ff", Temp0, Temp0, Temp1, Temp1).c_str());
            }
        }
    }
    else
    {
        less = Temp0 < Temp1;
        equal = Temp0 == Temp1;
        unorded = false;
    }

    condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) |
        ((m_Opcode.funct & 1) && unorded);

    if (condition)
    {
        _FPCR[31] |= FPCSR_C;
    }
    else
    {
        _FPCR[31] &= ~FPCSR_C;
    }
}

/************************** COP1: D functions ************************/
__inline void Double_RoundToInteger32(int32_t * Dest, const double * Source, int RoundType)
{
#pragma warning(push)
#pragma warning(disable:4244) //warning C4244: disabe conversion from 'double' to 'uint32_t', possible loss of data

    if (RoundType == FE_TONEAREST)
    {
        double reminder = *Source - floor(*Source);
        if (reminder == 0.5)
        {
            //make any decimal point in even to go to odd and any decimal point in odd stay as odd
            if (*Source < 0)
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? floor(*Source) : ceil(*Source);
            }
            else
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? ceil(*Source) : floor(*Source);
            }
        }
        else
        {
            *Dest = round(*Source);
        }
    }
    else if (RoundType == FE_TOWARDZERO) { *Dest = trunc(*Source); }
    else if (RoundType == FE_UPWARD) { *Dest = ceil(*Source); }
    else if (RoundType == FE_DOWNWARD) { *Dest = floor(*Source); }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

#pragma warning(pop)
}

__inline void Double_RoundToInteger64(int64_t * Dest, const double * Source, int RoundType)
{
#pragma warning(push)
#pragma warning(disable:4244) //warning C4244: disabe conversion from 'double' to 'uint64_t', possible loss of data

    if (RoundType == FE_TONEAREST)
    {
        double reminder = *Source - floor(*Source);
        if (reminder == 0.5)
        {
            //make any decimal point in even to go to odd and any decimal point in odd stay as odd
            if (*Source < 0)
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? floor(*Source) : ceil(*Source);
            }
            else
            {
                *Dest = (int)truncf(*Source) % 2 != 0 ? ceil(*Source) : floor(*Source);
            }
        }
        else
        {
            *Dest = round(*Source);
        }
    }
    else if (RoundType == FE_TOWARDZERO) { *Dest = trunc(*Source); }
    else if (RoundType == FE_UPWARD) { *Dest = ceil(*Source); }
    else if (RoundType == FE_DOWNWARD) { *Dest = floor(*Source); }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

#pragma warning(pop)
}

void R4300iOp::COP1_D_ADD()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] + *(double *)_FPR_D[m_Opcode.ft];
}

void R4300iOp::COP1_D_SUB()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] - *(double *)_FPR_D[m_Opcode.ft];
}

void R4300iOp::COP1_D_MUL()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] * *(double *)_FPR_D[m_Opcode.ft];
}

void R4300iOp::COP1_D_DIV()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] / *(double *)_FPR_D[m_Opcode.ft];
}

void R4300iOp::COP1_D_SQRT()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = (double)sqrt(*(double *)_FPR_D[m_Opcode.fs]);
}

void R4300iOp::COP1_D_ABS()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = fabs(*(double *)_FPR_D[m_Opcode.fs]);
}

void R4300iOp::COP1_D_MOV()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(int64_t *)_FPR_D[m_Opcode.fd] = *(int64_t *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_D_NEG()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = (*(double *)_FPR_D[m_Opcode.fs] * -1.0);
}

void R4300iOp::COP1_D_ROUND_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], FE_TONEAREST);
}

void R4300iOp::COP1_D_TRUNC_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], FE_TOWARDZERO);
}

void R4300iOp::COP1_D_CEIL_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], FE_UPWARD);
}

void R4300iOp::COP1_D_FLOOR_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(double *)_FPR_S[m_Opcode.fs], FE_DOWNWARD);
}

void R4300iOp::COP1_D_ROUND_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], FE_TONEAREST);
}

void R4300iOp::COP1_D_TRUNC_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], FE_TOWARDZERO);
}

void R4300iOp::COP1_D_CEIL_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], FE_UPWARD);
}

void R4300iOp::COP1_D_FLOOR_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger32(&*(int32_t *)_FPR_D[m_Opcode.fd], &*(double *)_FPR_S[m_Opcode.fs], FE_DOWNWARD);
}

void R4300iOp::COP1_D_CVT_S()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (float)*(double *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_D_CVT_W()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger32(&*(int32_t *)_FPR_S[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], *_RoundingModel);
}

void R4300iOp::COP1_D_CVT_L()
{
    TEST_COP1_USABLE_EXCEPTION();
    Double_RoundToInteger64(&*(int64_t *)_FPR_D[m_Opcode.fd], &*(double *)_FPR_D[m_Opcode.fs], *_RoundingModel);
}

void R4300iOp::COP1_D_CMP()
{
    bool less, equal, unorded;
    int32_t condition;
    MIPS_DWORD Temp0, Temp1;

    TEST_COP1_USABLE_EXCEPTION();

    Temp0.DW = *(int64_t *)_FPR_D[m_Opcode.fs];
    Temp1.DW = *(int64_t *)_FPR_D[m_Opcode.ft];

    if (_isnan(Temp0.D) || _isnan(Temp1.D))
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s: Nan ?", __FUNCTION__).c_str());
        }
        less = false;
        equal = false;
        unorded = true;
        if ((m_Opcode.funct & 8) != 0)
        {
            if (bHaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("Signal InvalidOperationException\nin %s", __FUNCTION__).c_str());
            }
        }
    }
    else
    {
        less = Temp0.D < Temp1.D;
        equal = Temp0.D == Temp1.D;
        unorded = false;
    }

    condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) |
        ((m_Opcode.funct & 1) && unorded);

    if (condition)
    {
        _FPCR[31] |= FPCSR_C;
    }
    else
    {
        _FPCR[31] &= ~FPCSR_C;
    }
}

/************************** COP1: W functions ************************/
void R4300iOp::COP1_W_CVT_S()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (float)*(int32_t *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_W_CVT_D()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = (double)*(int32_t *)_FPR_S[m_Opcode.fs];
}

/************************** COP1: L functions ************************/
void R4300iOp::COP1_L_CVT_S()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(float *)_FPR_S[m_Opcode.fd] = (float)*(int64_t *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_L_CVT_D()
{
    TEST_COP1_USABLE_EXCEPTION();
    fesetround(*_RoundingModel);
    *(double *)_FPR_D[m_Opcode.fd] = (double)*(int64_t *)_FPR_D[m_Opcode.fs];
}

/************************** Other functions **************************/
void R4300iOp::UnknownOpcode()
{
    g_Notify->DisplayError(stdstr_f("%s: %08X\n%s\n\nStopping Emulation !", GS(MSG_UNHANDLED_OP), (*_PROGRAM_COUNTER),
        R4300iOpcodeName(m_Opcode.Hex, (*_PROGRAM_COUNTER))).c_str());
    g_System->m_EndEmulation = true;

    g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
    if (HaveDebugger && !inFullScreen)
    {
        int32_t response;

        strcat(Message, "\n\nDo you wish to enter the debugger ?");

        response = MessageBox(NULL, Message, GS(MSG_MSGBOX_TITLE), MB_YESNO | MB_ICONERROR);
        if (response == IDYES)
        {
            Enter_R4300i_Commands_Window();
        }
        ExitThread(0);
    }
#endif
}