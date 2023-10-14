#include "stdafx.h"

#include <Project64-core/Debugger.h>
#include <Project64-core/Logging.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/R4300iInstruction.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Mips/TLB.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <fenv.h>
#include <float.h>
#include <math.h>

bool R4300iOp::m_TestTimer = false;
R4300iOpcode R4300iOp::m_Opcode;

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
R4300iOp::Func R4300iOp::Jump_CoP2[32];

const uint32_t R4300iOp::SWL_MASK[4] = {0x00000000, 0xFF000000, 0xFFFF0000, 0xFFFFFF00};
const uint32_t R4300iOp::SWR_MASK[4] = {0x00FFFFFF, 0x0000FFFF, 0x000000FF, 0x00000000};
const uint32_t R4300iOp::LWL_MASK[4] = {0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF};
const uint32_t R4300iOp::LWR_MASK[4] = {0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0x0000000};

const int32_t R4300iOp::SWL_SHIFT[4] = {0, 8, 16, 24};
const int32_t R4300iOp::SWR_SHIFT[4] = {24, 16, 8, 0};
const int32_t R4300iOp::LWL_SHIFT[4] = {0, 8, 16, 24};
const int32_t R4300iOp::LWR_SHIFT[4] = {24, 16, 8, 0};

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

void R4300iOp::COP2()
{
    Jump_CoP2[m_Opcode.fmt]();
}

void R4300iOp::COP3()
{
    g_Reg->TriggerException(EXC_II);
}

void R4300iOp::COP1_BC()
{
    Jump_CoP1_BC[m_Opcode.ft]();
}

void R4300iOp::COP1_S()
{
    Jump_CoP1_S[m_Opcode.funct]();
}

void R4300iOp::COP1_D()
{
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
    Jump_Opcode[18] = COP2;
    Jump_Opcode[19] = COP3;
    Jump_Opcode[20] = BEQL;
    Jump_Opcode[21] = BNEL;
    Jump_Opcode[22] = BLEZL;
    Jump_Opcode[23] = BGTZL;
    Jump_Opcode[24] = DADDI;
    Jump_Opcode[25] = DADDIU;
    Jump_Opcode[26] = LDL;
    Jump_Opcode[27] = LDR;
    Jump_Opcode[28] = UnknownOpcode;
    Jump_Opcode[29] = UnknownOpcode;
    Jump_Opcode[30] = UnknownOpcode;
    Jump_Opcode[31] = ReservedInstruction;
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
    Jump_Opcode[52] = LLD;
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
    Jump_Special[13] = SPECIAL_BREAK;
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
    Jump_Special[48] = SPECIAL_TGE;
    Jump_Special[49] = SPECIAL_TGEU;
    Jump_Special[50] = SPECIAL_TLT;
    Jump_Special[51] = SPECIAL_TLTU;
    Jump_Special[52] = SPECIAL_TEQ;
    Jump_Special[53] = UnknownOpcode;
    Jump_Special[54] = SPECIAL_TNE;
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
    Jump_Regimm[8] = REGIMM_TGEI;
    Jump_Regimm[9] = REGIMM_TGEIU;
    Jump_Regimm[10] = REGIMM_TLTI;
    Jump_Regimm[11] = REGIMM_TLTIU;
    Jump_Regimm[12] = REGIMM_TEQI;
    Jump_Regimm[13] = UnknownOpcode;
    Jump_Regimm[14] = REGIMM_TNEI;
    Jump_Regimm[15] = UnknownOpcode;
    Jump_Regimm[16] = REGIMM_BLTZAL;
    Jump_Regimm[17] = REGIMM_BGEZAL;
    Jump_Regimm[18] = UnknownOpcode;
    Jump_Regimm[19] = REGIMM_BGEZALL;
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
    Jump_CoP0[1] = COP0_DMF;
    Jump_CoP0[2] = UnknownOpcode;
    Jump_CoP0[3] = UnknownOpcode;
    Jump_CoP0[4] = COP0_MT;
    Jump_CoP0[5] = COP0_DMT;
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
    Jump_CoP1[3] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1[4] = COP1_MT;
    Jump_CoP1[5] = COP1_DMT;
    Jump_CoP1[6] = COP1_CT;
    Jump_CoP1[7] = CPO1_UNIMPLEMENTED_OP;
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
    Jump_CoP1_S[32] = CPO1_UNIMPLEMENTED_OP;
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
    Jump_CoP1_D[33] = CPO1_UNIMPLEMENTED_OP;
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
    Jump_CoP1_W[8] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[9] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[10] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[11] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[12] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[13] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[14] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[15] = CPO1_UNIMPLEMENTED_OP;
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
    Jump_CoP1_W[36] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[37] = CPO1_UNIMPLEMENTED_OP;
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
    Jump_CoP1_L[8] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[9] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[10] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[11] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[12] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[13] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[14] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[15] = CPO1_UNIMPLEMENTED_OP;
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
    Jump_CoP1_L[36] = CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[37] = CPO1_UNIMPLEMENTED_OP;
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

    Jump_CoP2[0] = COP2_MF;
    Jump_CoP2[1] = COP2_DMF;
    Jump_CoP2[2] = COP2_CF;
    Jump_CoP2[3] = CPO2_INVALID_OP;
    Jump_CoP2[4] = COP2_MT;
    Jump_CoP2[5] = COP2_DMT;
    Jump_CoP2[6] = COP2_CT;
    Jump_CoP2[7] = CPO2_INVALID_OP;
    Jump_CoP2[8] = CPO2_INVALID_OP;
    Jump_CoP2[10] = CPO2_INVALID_OP;
    Jump_CoP2[11] = CPO2_INVALID_OP;
    Jump_CoP2[12] = CPO2_INVALID_OP;
    Jump_CoP2[13] = CPO2_INVALID_OP;
    Jump_CoP2[14] = CPO2_INVALID_OP;
    Jump_CoP2[15] = CPO2_INVALID_OP;
    Jump_CoP2[16] = CPO2_INVALID_OP;
    Jump_CoP2[17] = CPO2_INVALID_OP;
    Jump_CoP2[18] = CPO2_INVALID_OP;
    Jump_CoP2[19] = CPO2_INVALID_OP;
    Jump_CoP2[20] = CPO2_INVALID_OP;
    Jump_CoP2[21] = CPO2_INVALID_OP;
    Jump_CoP2[22] = CPO2_INVALID_OP;
    Jump_CoP2[23] = CPO2_INVALID_OP;
    Jump_CoP2[24] = CPO2_INVALID_OP;
    Jump_CoP2[25] = CPO2_INVALID_OP;
    Jump_CoP2[26] = CPO2_INVALID_OP;
    Jump_CoP2[27] = CPO2_INVALID_OP;
    Jump_CoP2[28] = CPO2_INVALID_OP;
    Jump_CoP2[29] = CPO2_INVALID_OP;
    Jump_CoP2[30] = CPO2_INVALID_OP;
    Jump_CoP2[31] = CPO2_INVALID_OP;

    return Jump_Opcode;
}

// Opcode functions

void R4300iOp::J()
{
    g_System->DelayedJump((*_PROGRAM_COUNTER & 0xF0000000) + (m_Opcode.target << 2));
}

void R4300iOp::JAL()
{
    g_System->DelayedJump((*_PROGRAM_COUNTER & 0xF0000000) + (m_Opcode.target << 2));
    _GPR[31].DW = (int32_t)(g_System->m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? g_System->m_JumpToLocation + 4 : *_PROGRAM_COUNTER + 8);
}

void R4300iOp::BEQ()
{
    if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::BNE()
{
    if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::BLEZ()
{
    if (_GPR[m_Opcode.rs].DW <= 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::BGTZ()
{
    if (_GPR[m_Opcode.rs].DW > 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
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
    int32_t rs = _GPR[m_Opcode.rs].W[0];
    int32_t imm = (int16_t)m_Opcode.immediate;
    int32_t sum = rs + imm;
    if ((~(rs ^ imm) & (rs ^ sum)) & 0x80000000)
    {
        g_Reg->TriggerException(EXC_OV);
        return;
    }
    _GPR[m_Opcode.rt].DW = sum;
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
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BNEL()
{
    if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BLEZL()
{
    if (_GPR[m_Opcode.rs].DW <= 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BGTZL()
{
    if (_GPR[m_Opcode.rs].DW > 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::DADDI()
{
    int64_t rs = _GPR[m_Opcode.rs].DW;
    int64_t imm = (int64_t)((int16_t)m_Opcode.immediate);
    int64_t sum = rs + imm;
    if ((~(rs ^ imm) & (rs ^ sum)) & 0x8000000000000000)
    {
        g_Reg->TriggerException(EXC_OV);
        return;
    }
    _GPR[m_Opcode.rt].DW = sum;
}

void R4300iOp::DADDIU()
{
    _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW + (int64_t)((int16_t)m_Opcode.immediate);
}

uint64_t LDL_MASK[8] = {0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFF, 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF};
int32_t LDL_SHIFT[8] = {0, 8, 16, 24, 32, 40, 48, 56};

void R4300iOp::LDL()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (g_MMU->LD_Memory((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].DW & LDL_MASK[Offset];
        _GPR[m_Opcode.rt].DW += MemoryValue << LDL_SHIFT[Offset];
    }
}

uint64_t LDR_MASK[8] = {0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000,
                        0xFFFFFFFFFF000000, 0xFFFFFFFF00000000,
                        0xFFFFFF0000000000, 0xFFFF000000000000,
                        0xFF00000000000000, 0};
int32_t LDR_SHIFT[8] = {56, 48, 40, 32, 24, 16, 8, 0};

void R4300iOp::LDR()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (g_MMU->LD_Memory((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        _GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].DW & LDR_MASK[Offset];
        _GPR[m_Opcode.rt].DW += MemoryValue >> LDR_SHIFT[Offset];
    }
}

void R4300iOp::LB()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint8_t MemoryValue;
    if (g_MMU->LB_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].DW = (int8_t)MemoryValue;
    }
}

void R4300iOp::LH()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint16_t MemoryValue;
    if (g_MMU->LH_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].DW = (int16_t)MemoryValue;
    }
}

void R4300iOp::LWL()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->LW_Memory((Address & ~3), MemoryValue))
    {
        uint32_t Offset = Address & 3;
        _GPR[m_Opcode.rt].DW = (int32_t)(_GPR[m_Opcode.rt].W[0] & LWL_MASK[Offset]);
        _GPR[m_Opcode.rt].DW += (int32_t)(MemoryValue << LWL_SHIFT[Offset]);
    }
}

void R4300iOp::LW()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->LW_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].DW = (int32_t)MemoryValue;
    }
}

void R4300iOp::LBU()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint8_t MemoryValue;
    if (g_MMU->LB_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].UDW = MemoryValue;
    }
}

void R4300iOp::LHU()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint16_t MemoryValue;
    if (g_MMU->LH_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].UDW = MemoryValue;
    }
}

void R4300iOp::LWR()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->LW_Memory((Address & ~3), MemoryValue))
    {
        uint32_t Offset = Address & 3;
        _GPR[m_Opcode.rt].DW = (int32_t)(_GPR[m_Opcode.rt].W[0] & LWR_MASK[Offset]);
        _GPR[m_Opcode.rt].DW += (int32_t)(MemoryValue >> LWR_SHIFT[Offset]);
    }
}

void R4300iOp::LWU()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->LW_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].UDW = MemoryValue;
    }
}

void R4300iOp::SB()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->SB_Memory(Address, _GPR[m_Opcode.rt].UW[0]);
}

void R4300iOp::SH()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->SH_Memory(Address, _GPR[m_Opcode.rt].UW[0]);
}

void R4300iOp::SWL()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->MemoryValue32(Address & ~3, MemoryValue))
    {
        uint32_t Offset = Address & 3;
        MemoryValue &= SWL_MASK[Offset];
        MemoryValue += _GPR[m_Opcode.rt].UW[0] >> SWL_SHIFT[Offset];
        g_MMU->SW_Memory(Address & ~3, MemoryValue);
    }
    else
    {
        g_Reg->TriggerAddressException(Address, EXC_WMISS);
    }
}

void R4300iOp::SW()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->SW_Memory(Address, _GPR[m_Opcode.rt].UW[0]);
}

uint64_t SDL_MASK[8] = {0, 0xFF00000000000000,
                        0xFFFF000000000000,
                        0xFFFFFF0000000000,
                        0xFFFFFFFF00000000,
                        0xFFFFFFFFFF000000,
                        0xFFFFFFFFFFFF0000,
                        0xFFFFFFFFFFFFFF00};

int32_t SDL_SHIFT[8] = {0, 8, 16, 24, 32, 40, 48, 56};

void R4300iOp::SDL()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (g_MMU->MemoryValue64((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        MemoryValue &= SDL_MASK[Offset];
        MemoryValue += _GPR[m_Opcode.rt].UDW >> SDL_SHIFT[Offset];
        g_MMU->SD_Memory((Address & ~7), MemoryValue);
    }
    else
    {
        g_Reg->TriggerAddressException(Address, EXC_WMISS);
    }
}

uint64_t SDR_MASK[8] = {0x00FFFFFFFFFFFFFF,
                        0x0000FFFFFFFFFFFF,
                        0x000000FFFFFFFFFF,
                        0x00000000FFFFFFFF,
                        0x0000000000FFFFFF,
                        0x000000000000FFFF,
                        0x00000000000000FF,
                        0x0000000000000000};

int32_t SDR_SHIFT[8] = {56, 48, 40, 32, 24, 16, 8, 0};

void R4300iOp::SDR()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (g_MMU->MemoryValue64((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        MemoryValue &= SDR_MASK[Offset];
        MemoryValue += _GPR[m_Opcode.rt].UDW << SDR_SHIFT[Offset];
        g_MMU->SD_Memory((Address & ~7), MemoryValue);
    }
    else
    {
        g_Reg->TriggerAddressException(Address, EXC_WMISS);
    }
}

void R4300iOp::SWR()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->MemoryValue32((Address & ~3), MemoryValue))
    {
        uint32_t Offset = Address & 3;
        MemoryValue &= SWR_MASK[Offset];
        MemoryValue += _GPR[m_Opcode.rt].UW[0] << SWR_SHIFT[Offset];
        g_MMU->SW_Memory((Address & ~0x03), MemoryValue);
    }
    else
    {
        g_Reg->TriggerAddressException(Address, EXC_WMISS);
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
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (g_MMU->LW_Memory(Address, MemoryValue))
    {
        _GPR[m_Opcode.rt].DW = (int32_t)MemoryValue;
        (*_LLBit) = 1;
        uint32_t PhysicalAddr;
        bool MemoryUsed;
        g_TLB->VAddrToPAddr(Address, PhysicalAddr, MemoryUsed);
        _CP0[17] = PhysicalAddr >> 4;
    }
}

void R4300iOp::LWC1()
{
    if (TestCop1UsableException())
    {
        return;
    }
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->LW_Memory(Address, *(uint32_t *)_FPR_S[m_Opcode.ft]);
}

void R4300iOp::SC()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    if ((*_LLBit) != 1 || g_MMU->SW_Memory(Address, _GPR[m_Opcode.rt].UW[0]))
    {
        _GPR[m_Opcode.rt].UW[0] = (*_LLBit);
    }
}

void R4300iOp::LD()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    if (g_MMU->LD_Memory(Address, _GPR[m_Opcode.rt].UDW))
    {
#ifdef Interpreter_StackTest
        if (m_Opcode.rt == 29)
        {
            StackValue = _GPR[m_Opcode.rt].W[0];
        }
#endif
    }
}

void R4300iOp::LLD()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    if (g_MMU->LD_Memory(Address, _GPR[m_Opcode.rt].UDW))
    {
        (*_LLBit) = 1;
        uint32_t PhysicalAddr;
        bool MemoryUsed;
        g_TLB->VAddrToPAddr(Address, PhysicalAddr, MemoryUsed);
        _CP0[17] = PhysicalAddr >> 4;
    }
}

void R4300iOp::LDC1()
{
    if (TestCop1UsableException())
    {
        return;
    }
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->LD_Memory(Address, *(uint64_t *)_FPR_D[m_Opcode.ft]);
}

void R4300iOp::SWC1()
{
    if (TestCop1UsableException())
    {
        return;
    }

    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->SW_Memory(Address, *(uint32_t *)_FPR_S[m_Opcode.ft]);
}

void R4300iOp::SDC1()
{
    if (TestCop1UsableException())
    {
        return;
    }
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->SD_Memory(Address, *((uint64_t *)_FPR_D[m_Opcode.ft]));
}

void R4300iOp::SD()
{
    uint64_t Address = _GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    g_MMU->SD_Memory(Address, _GPR[m_Opcode.rt].UDW);
}

// R4300i opcodes: Special

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
    _GPR[m_Opcode.rd].DW = (int32_t)(_GPR[m_Opcode.rt].DW >> m_Opcode.sa);
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
    _GPR[m_Opcode.rd].DW = (int32_t)(_GPR[m_Opcode.rt].DW >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_JR()
{
    g_System->DelayedJump(_GPR[m_Opcode.rs].UW[0]);
    m_TestTimer = true;
}

void R4300iOp::SPECIAL_JALR()
{
    g_System->DelayedJump(_GPR[m_Opcode.rs].UW[0]);
    _GPR[m_Opcode.rd].DW = (int32_t)(g_System->m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? g_System->m_JumpToLocation + 4 : *_PROGRAM_COUNTER + 8);
    m_TestTimer = true;
}

void R4300iOp::SPECIAL_SYSCALL()
{
    g_Reg->TriggerException(EXC_SYSCALL);
}

void R4300iOp::SPECIAL_BREAK()
{
    if (StepOnBreakOpCode())
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
        g_Debugger->WaitForStep();
    }
    else
    {
        g_Reg->TriggerException(EXC_BREAK);
    }
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
        if (_GPR[m_Opcode.rs].W[0] != 0x80000000 || _GPR[m_Opcode.rt].W[0] != -1)
        {
            _RegLO->DW = _GPR[m_Opcode.rs].W[0] / _GPR[m_Opcode.rt].W[0];
            _RegHI->DW = _GPR[m_Opcode.rs].W[0] % _GPR[m_Opcode.rt].W[0];
        }
        else
        {
            _RegLO->DW = 0xFFFFFFFF80000000;
            _RegHI->DW = 0x0000000000000000;
        }
    }
    else
    {
        _RegLO->DW = _GPR[m_Opcode.rs].W[0] < 0 ? 0x0000000000000001 : 0xFFFFFFFFFFFFFFFF;
        _RegHI->DW = _GPR[m_Opcode.rs].W[0];
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
        _RegLO->DW = 0xFFFFFFFFFFFFFFFF;
        _RegHI->DW = _GPR[m_Opcode.rs].W[0];
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
        _RegLO->DW = _GPR[m_Opcode.rs].DW < 0 ? 0x0000000000000001 : 0xFFFFFFFFFFFFFFFF;
        _RegHI->DW = _GPR[m_Opcode.rs].DW;
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
        _RegLO->DW = 0xFFFFFFFFFFFFFFFF;
        _RegHI->DW = _GPR[m_Opcode.rs].DW;
    }
}

void R4300iOp::SPECIAL_ADD()
{
    int32_t rs = _GPR[m_Opcode.rs].W[0];
    int32_t rt = _GPR[m_Opcode.rt].W[0];
    int32_t sum = rs + rt;
    if ((~(rs ^ rt) & (rs ^ sum)) & 0x80000000)
    {
        g_Reg->TriggerException(EXC_OV);
        return;
    }
    _GPR[m_Opcode.rd].DW = sum;
}

void R4300iOp::SPECIAL_ADDU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_SUB()
{
    int32_t rs = _GPR[m_Opcode.rs].W[0];
    int32_t rt = _GPR[m_Opcode.rt].W[0];
    int32_t sub = rs - rt;

    if (((rs ^ rt) & (rs ^ sub)) & 0x80000000)
    {
        g_Reg->TriggerException(EXC_OV);
        return;
    }
    _GPR[m_Opcode.rd].DW = sub;
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
    int64_t rs = _GPR[m_Opcode.rs].DW;
    int64_t rt = _GPR[m_Opcode.rt].DW;
    int64_t sum = rs + rt;
    if ((~(rs ^ rt) & (rs ^ sum)) & 0x8000000000000000)
    {
        g_Reg->TriggerException(EXC_OV);
        return;
    }
    _GPR[m_Opcode.rd].DW = sum;
}

void R4300iOp::SPECIAL_DADDU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW + _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DSUB()
{
    int64_t rs = _GPR[m_Opcode.rs].DW;
    int64_t rt = _GPR[m_Opcode.rt].DW;
    int64_t sub = rs - rt;

    if (((rs ^ rt) & (rs ^ sub)) & 0x8000000000000000)
    {
        g_Reg->TriggerException(EXC_OV);
        return;
    }
    _GPR[m_Opcode.rd].DW = sub;
}

void R4300iOp::SPECIAL_DSUBU()
{
    _GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW - _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_TEQ()
{
    if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TGE()
{
    if (_GPR[m_Opcode.rs].DW >= _GPR[m_Opcode.rt].DW)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TGEU()
{
    if (_GPR[m_Opcode.rs].UDW >= _GPR[m_Opcode.rt].UDW)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TLT()
{
    if (_GPR[m_Opcode.rs].DW < _GPR[m_Opcode.rt].DW)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TLTU()
{
    if (_GPR[m_Opcode.rs].UDW < _GPR[m_Opcode.rt].UDW)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TNE()
{
    if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW)
    {
        g_Reg->TriggerException(EXC_TRAP);
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

// R4300i opcodes: RegImm

void R4300iOp::REGIMM_BLTZ()
{
    if (_GPR[m_Opcode.rs].DW < 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::REGIMM_BGEZ()
{
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::REGIMM_BLTZL()
{
    if (_GPR[m_Opcode.rs].DW < 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BGEZL()
{
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BLTZAL()
{
    if (_GPR[m_Opcode.rs].DW < 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
    _GPR[31].DW = (int32_t)(g_System->m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? g_System->m_JumpToLocation + 4 : *_PROGRAM_COUNTER + 8);
}

void R4300iOp::REGIMM_BGEZAL()
{
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->DelayedJump(*_PROGRAM_COUNTER + 8);
    }
    _GPR[31].DW = (int32_t)(g_System->m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? g_System->m_JumpToLocation + 4 : *_PROGRAM_COUNTER + 8);
}

void R4300iOp::REGIMM_BGEZALL()
{
    if (_GPR[m_Opcode.rs].DW >= 0)
    {
        g_System->DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
    _GPR[31].DW = (int32_t)(g_System->m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? g_System->m_JumpToLocation + 4 : *_PROGRAM_COUNTER + 8);
}

void R4300iOp::REGIMM_TEQI()
{
    if (_GPR[m_Opcode.rs].DW == (int64_t)((int16_t)m_Opcode.immediate))
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TGEI()
{
    if (_GPR[m_Opcode.rs].DW >= (int64_t)((int16_t)m_Opcode.immediate))
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TGEIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    if (_GPR[m_Opcode.rs].UDW >= (uint64_t)imm64)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TLTI()
{
    if (_GPR[m_Opcode.rs].DW < (int64_t)((int16_t)m_Opcode.immediate))
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TLTIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    if (_GPR[m_Opcode.rs].UDW < (uint64_t)imm64)
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TNEI()
{
    if (_GPR[m_Opcode.rs].DW != (int64_t)((int16_t)m_Opcode.immediate))
    {
        g_Reg->TriggerException(EXC_TRAP);
    }
}

// COP0 functions

void R4300iOp::COP0_MF()
{
    _GPR[m_Opcode.rt].DW = (int32_t)g_Reg->Cop0_MF((CRegisters::COP0Reg)m_Opcode.rd);
}

void R4300iOp::COP0_DMF()
{
    _GPR[m_Opcode.rt].DW = g_Reg->Cop0_MF((CRegisters::COP0Reg)m_Opcode.rd);
}

void R4300iOp::COP0_MT()
{
    g_Reg->Cop0_MT((CRegisters::COP0Reg)m_Opcode.rd, (int64_t)_GPR[m_Opcode.rt].W[0]);
}

void R4300iOp::COP0_DMT()
{
    g_Reg->Cop0_MT((CRegisters::COP0Reg)m_Opcode.rd, _GPR[m_Opcode.rt].UDW);
}
// COP0 CO functions

void R4300iOp::COP0_CO_TLBR()
{
    g_TLB->ReadEntry();
}

void R4300iOp::COP0_CO_TLBWI()
{
    g_TLB->WriteEntry(g_Reg->INDEX_REGISTER & 0x1F, false);
}

void R4300iOp::COP0_CO_TLBWR()
{
    g_TLB->WriteEntry(g_Reg->RANDOM_REGISTER & 0x1F, true);
}

void R4300iOp::COP0_CO_TLBP()
{
    g_TLB->Probe();
}

void R4300iOp::COP0_CO_ERET()
{
    g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
    if ((g_Reg->STATUS_REGISTER.ErrorLevel) != 0)
    {
        g_System->m_JumpToLocation = (uint32_t)g_Reg->ERROREPC_REGISTER;
        g_Reg->STATUS_REGISTER.ErrorLevel = 0;
    }
    else
    {
        g_System->m_JumpToLocation = (uint32_t)g_Reg->EPC_REGISTER;
        g_Reg->STATUS_REGISTER.ExceptionLevel = 0;
    }
    (*_LLBit) = 0;
    g_Reg->CheckInterrupts();
    m_TestTimer = true;
}

// COP1 functions
void R4300iOp::CPO1_UNIMPLEMENTED_OP()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
    StatusReg.Cause.UnimplementedOperation = 1;
    g_Reg->TriggerException(EXC_FPE);
}

void R4300iOp::COP1_MF()
{
    if (TestCop1UsableException())
    {
        return;
    }
    _GPR[m_Opcode.rt].DW = *(int32_t *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_DMF()
{
    if (TestCop1UsableException())
    {
        return;
    }
    _GPR[m_Opcode.rt].DW = *(int64_t *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_CF()
{
    if (TestCop1UsableException())
    {
        return;
    }
    _GPR[m_Opcode.rt].DW = (int32_t)_FPCR[m_Opcode.fs];
}

void R4300iOp::COP1_MT()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *(int32_t *)_FPR_S[m_Opcode.fs] = _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::COP1_DMT()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *(int64_t *)_FPR_D[m_Opcode.fs] = _GPR[m_Opcode.rt].DW;
}

void R4300iOp::COP1_CT()
{
    if (TestCop1UsableException())
    {
        return;
    }
    g_Reg->Cop1_CT(m_Opcode.fs, _GPR[m_Opcode.rt].W[0]);
}

// COP1: BC1 functions

void R4300iOp::COP1_BCF()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    g_System->m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
    if ((_FPCR[31] & FPCSR_C) == 0)
    {
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCT()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    g_System->m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
    if ((_FPCR[31] & FPCSR_C) != 0)
    {
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCFL()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if ((_FPCR[31] & FPCSR_C) == 0)
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCTL()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if ((_FPCR[31] & FPCSR_C) != 0)
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        g_System->m_PipelineStage = PIPELINE_STAGE_JUMP;
        g_System->m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
    }
}

// COP1: S functions
void R4300iOp::COP1_S_ADD()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)_FPR_S_L[m_Opcode.fs] + *(float *)_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_SUB()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)_FPR_S_L[m_Opcode.fs] - *(float *)_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_MUL()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)_FPR_S_L[m_Opcode.fs] * *(float *)_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_DIV()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)_FPR_S_L[m_Opcode.fs] / *(float *)_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_SQRT()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    float Result = sqrtf(*(float *)(_FPR_S_L[m_Opcode.fs]));
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_ABS()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    float Result = (float)fabs(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_MOV()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = (*(uint64_t *)_FPR_D[m_Opcode.fs] & 0xFFFFFFFF00000000ll) | *(uint32_t *)_FPR_S_L[m_Opcode.fs];
}

void R4300iOp::COP1_S_NEG()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    float Result = (*(float *)_FPR_S_L[m_Opcode.fs] * -1.0f);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_ROUND_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }

    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_TRUNC_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_CEIL_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_FLOOR_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_ROUND_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_TRUNC_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_CEIL_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_FLOOR_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_CVT_D()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    double Result = (double)(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_CVT_W()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_CVT_L()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_CMP()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    float Temp0 = *(float *)_FPR_S_L[m_Opcode.fs];
    float Temp1 = *(float *)_FPR_UW[m_Opcode.ft];

    bool less, equal, unorded;
    if (_isnan(Temp0) || _isnan(Temp1))
    {
        less = false;
        equal = false;
        unorded = true;

        bool QuietNan = false;
        if ((*(uint32_t *)_FPR_S_L[m_Opcode.fs] >= 0x7FC00000 && *(uint32_t *)_FPR_S_L[m_Opcode.fs] <= 0x7FFFFFFF) ||
            (*(uint32_t *)_FPR_S_L[m_Opcode.fs] >= 0xFFC00000 && *(uint32_t *)_FPR_S_L[m_Opcode.fs] <= 0xFFFFFFFF))
        {
            QuietNan = true;
        }
        else if ((*(uint32_t *)_FPR_UW[m_Opcode.ft] >= 0x7FC00000 && *(uint32_t *)_FPR_UW[m_Opcode.ft] <= 0x7FFFFFFF) ||
                 (*(uint32_t *)_FPR_UW[m_Opcode.ft] >= 0xFFC00000 && *(uint32_t *)_FPR_UW[m_Opcode.ft] <= 0xFFFFFFFF))
        {
            QuietNan = true;
        }
        if ((m_Opcode.funct & 8) != 0 || QuietNan)
        {
            FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                g_Reg->TriggerException(EXC_FPE);
                return;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    else
    {
        less = Temp0 < Temp1;
        equal = Temp0 == Temp1;
        unorded = false;
    }

    int32_t condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) |
                        ((m_Opcode.funct & 1) && unorded);

    if (condition != 0)
    {
        _FPCR[31] |= FPCSR_C;
    }
    else
    {
        _FPCR[31] &= ~FPCSR_C;
    }
}

// COP1: D functions
void R4300iOp::COP1_D_ADD()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)_FPR_D[m_Opcode.fs] + *(double *)_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_SUB()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)_FPR_D[m_Opcode.fs] - *(double *)_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_MUL()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)_FPR_D[m_Opcode.fs] * *(double *)_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_DIV()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)_FPR_D[m_Opcode.fs] / *(double *)_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

#if defined(_MSC_VER) && (defined(__i386__) || defined(_M_IX86))
static double correct_sqrt(double a)
{
    __asm
    {
        fld QWORD PTR [a]
        fsqrt
    }
}
#endif

void R4300iOp::COP1_D_SQRT()
{
#if defined(_MSC_VER) && (defined(__i386__) || defined(_M_IX86))
    _controlfp(_PC_53, _MCW_PC);
#endif
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
#if defined(_MSC_VER) && (defined(__i386__) || defined(_M_IX86))
    double Result = (double)correct_sqrt(*(double *)_FPR_D[m_Opcode.fs]);
#else
    double Result = (double)sqrt(*(double *)_FPR_D[m_Opcode.fs]);
#endif
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_ABS()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    double Result = fabs(*(double *)_FPR_D[m_Opcode.fs]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_MOV()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(int64_t *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_D_NEG()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    double Result = (*(double *)_FPR_D[m_Opcode.fs] * -1.0);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_ROUND_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }
    const double & fs = *(double *)_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_TRUNC_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    const double & fs = *(double *)_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_CEIL_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    const double & fs = *(double *)_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_FLOOR_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    const double & fs = *(double *)_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_ROUND_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_TRUNC_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CEIL_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_FLOOR_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CVT_S()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput64(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    float Result = (float)*(double *)_FPR_D[m_Opcode.fs];
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CVT_W()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CVT_L()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    const double & fs = *(double *)_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_CMP()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }

    MIPS_DWORD Temp0, Temp1;
    Temp0.DW = *(int64_t *)_FPR_D[m_Opcode.fs];
    Temp1.DW = *(int64_t *)_FPR_UDW[m_Opcode.ft];

    bool less, equal, unorded;
    if (_isnan(Temp0.D) || _isnan(Temp1.D))
    {
        less = false;
        equal = false;
        unorded = true;

        bool QuietNan = false;
        if ((*(uint64_t *)_FPR_D[m_Opcode.fs] >= 0x7FF8000000000000 && *(uint64_t *)_FPR_D[m_Opcode.fs] <= 0x7FFFFFFFFFFFFFFF) ||
            (*(uint64_t *)_FPR_D[m_Opcode.fs] >= 0xFFF8000000000000 && *(uint64_t *)_FPR_D[m_Opcode.fs] <= 0xFFFFFFFFFFFFFFFF))
        {
            QuietNan = true;
        }
        else if ((*(uint64_t *)_FPR_UDW[m_Opcode.ft] >= 0x7FF8000000000000 && *(uint64_t *)_FPR_UDW[m_Opcode.ft] <= 0x7FFFFFFFFFFFFFFF) ||
                 (*(uint64_t *)_FPR_UDW[m_Opcode.ft] >= 0xFFF8000000000000 && *(uint64_t *)_FPR_UDW[m_Opcode.ft] <= 0xFFFFFFFFFFFFFFFF))
        {
            QuietNan = true;
        }

        if ((m_Opcode.funct & 8) != 0 || QuietNan)
        {
            FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                g_Reg->TriggerException(EXC_FPE);
                return;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    else
    {
        less = Temp0.D < Temp1.D;
        equal = Temp0.D == Temp1.D;
        unorded = false;
    }

    int32_t condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) |
                        ((m_Opcode.funct & 1) && unorded);

    if (condition != 0)
    {
        _FPCR[31] |= FPCSR_C;
    }
    else
    {
        _FPCR[31] &= ~FPCSR_C;
    }
}

// COP1: W functions
void R4300iOp::COP1_W_CVT_S()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    float Result = (float)*(int32_t *)_FPR_S_L[m_Opcode.fs];
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_W_CVT_D()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    double Result = (double)*(int32_t *)_FPR_S_L[m_Opcode.fs];
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

// COP1: L functions

void R4300iOp::COP1_L_CVT_S()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    int64_t fs = *(int64_t *)_FPR_D[m_Opcode.fs];
    if (fs >= (int64_t)0x0080000000000000ull || fs < (int64_t)0xff80000000000000ull)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        g_Reg->TriggerException(EXC_FPE);
        return;
    }
    float Result = (float)fs;
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_L_CVT_D()
{
    if (InitFpuOperation(((FPStatusReg &)_FPCR[31]).RoundingMode))
    {
        return;
    }
    int64_t fs = *(int64_t *)_FPR_D[m_Opcode.fs];
    if (fs >= (int64_t)0x0080000000000000ull || fs < (int64_t)0xff80000000000000ull)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        g_Reg->TriggerException(EXC_FPE);
        return;
    }
    double Result = (double)fs;
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

// COP2 functions
void R4300iOp::CPO2_INVALID_OP(void)
{
    g_Reg->TriggerException(g_Reg->STATUS_REGISTER.CU2 == 0 ? EXC_CPU : EXC_II, 2);
}

void R4300iOp::COP2_MF()
{
    if (g_Reg->STATUS_REGISTER.CU2 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 2);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = (int32_t)g_Reg->Cop2_MF(m_Opcode.rd);
    }
}

void R4300iOp::COP2_DMF()
{
    if (g_Reg->STATUS_REGISTER.CU2 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 2);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = g_Reg->Cop2_MF(m_Opcode.rd);
    }
}

void R4300iOp::COP2_CF()
{
    if (g_Reg->STATUS_REGISTER.CU2 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 2);
    }
    else
    {
        _GPR[m_Opcode.rt].DW = (int32_t)g_Reg->Cop2_MF(m_Opcode.rd);
    }
}

void R4300iOp::COP2_MT()
{
    if (g_Reg->STATUS_REGISTER.CU2 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 2);
    }
    else
    {
        g_Reg->Cop2_MT((CRegisters::COP0Reg)m_Opcode.rd, _GPR[m_Opcode.rt].DW);
    }
}

void R4300iOp::COP2_DMT()
{
    if (g_Reg->STATUS_REGISTER.CU2 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 2);
    }
    else
    {
        g_Reg->Cop2_MT((CRegisters::COP0Reg)m_Opcode.rd, _GPR[m_Opcode.rt].DW);
    }
}

void R4300iOp::COP2_CT()
{
    if (g_Reg->STATUS_REGISTER.CU2 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 2);
    }
    else
    {
        g_Reg->Cop2_MT((CRegisters::COP0Reg)m_Opcode.rd, _GPR[m_Opcode.rt].DW);
    }
}

// Other functions
void R4300iOp::ReservedInstruction()
{
    g_Reg->TriggerException(EXC_II);
}

void R4300iOp::UnknownOpcode()
{
    if (HaveDebugger())
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
        g_Debugger->WaitForStep();
    }
    else
    {
        R4300iInstruction Opcode(*_PROGRAM_COUNTER, m_Opcode.Value);
        g_Notify->DisplayError(stdstr_f("%s: %08X\n%s %s\n\nStopping emulation", GS(MSG_UNHANDLED_OP), (*_PROGRAM_COUNTER), Opcode.Name(), Opcode.Param()).c_str());
        g_System->m_EndEmulation = true;

        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

bool R4300iOp::MemoryBreakpoint()
{
    if (g_Settings->LoadBool(Debugger_SteppingOps))
    {
        return false;
    }
    g_Settings->SaveBool(Debugger_SteppingOps, true);
    g_Debugger->WaitForStep();
    if (SkipOp())
    {
        // Skip command if instructed by the debugger
        g_Settings->SaveBool(Debugger_SkipOp, false);
        return true;
    }
    return false;
}

bool R4300iOp::TestCop1UsableException(void)
{
    if (g_Reg->STATUS_REGISTER.CU1 == 0)
    {
        g_Reg->TriggerException(EXC_CPU, 1);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput32(const float & Value)
{
    bool Exception = false;
    if ((*((uint32_t *)&Value) & 0x7F800000) == 0x00000000 && (*((uint32_t *)&Value) & 0x007FFFFF) != 0x00000000) // Sub Normal
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        Exception = true;
    }
    else if ((*((uint32_t *)&Value) & 0x7F800000) == 0x7F800000 && (*((uint32_t *)&Value) & 0x007FFFFF) != 0x00000000) // Nan
    {
        uint32_t Value32 = *(uint32_t *)&Value;
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        if ((Value32 >= 0x7F800001 && Value32 < 0x7FC00000) ||
            (Value32 >= 0xFF800001 && Value32 < 0xFFC00000))
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            Exception = true;
        }
        else
        {
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                Exception = true;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    if (Exception)
    {
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput32Conv(const float & Value)
{
    uint32_t InvalidValueMax = 0x5a000000, InvalidMinValue = 0xda000000;

    int Type = fpclassify(Value);
    if (Type == FP_SUBNORMAL || Type == FP_INFINITE || Type == FP_NAN ||
        Value >= *(float *)&InvalidValueMax || Value <= *(float *)&InvalidMinValue)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput64(const double & Value)
{
    int Type = fpclassify(Value);
    bool Exception = false;
    if (Type == FP_SUBNORMAL)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        Exception = true;
    }
    else if (Type == FP_NAN)
    {
        uint64_t Value64 = *(uint64_t *)&Value;
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        if ((Value64 >= 0x7FF0000000000001 && Value64 <= 0x7FF7FFFFFFFFFFFF) ||
            (Value64 >= 0xFFF0000000000001 && Value64 <= 0xFFF7FFFFFFFFFFFF))
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            Exception = true;
        }
        else
        {
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                Exception = true;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    if (Exception)
    {
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput64Conv(const double & Value)
{
    uint64_t InvalidValueMax = 0x4340000000000000, InvalidMinValue = 0xc340000000000000;

    int Type = fpclassify(Value);
    if (Type == FP_SUBNORMAL || Type == FP_INFINITE || Type == FP_NAN ||
        Value >= *(double *)&InvalidValueMax || Value <= *(double *)&InvalidMinValue)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUResult32(float & Result)
{
    int Except = fetestexcept(FE_ALL_EXCEPT);
    bool DoException = false;

    if ((*((uint32_t *)&Result) & 0x7F800000) == 0x7F800000 && (*((uint32_t *)&Result) & 0x007FFFFF) != 0x00000000) // Nan
    {
        if (Except == 0 || !SetFPUException())
        {
            *((uint32_t *)&Result) = 0x7fbfffff;
        }
        else
        {
            DoException = true;
        }
    }
    else if ((*((uint32_t *)&Result) & 0x7F800000) == 0x00000000 && (*((uint32_t *)&Result) & 0x007FFFFF) != 0x00000000) // Sub Normal
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        if (!StatusReg.FlushSubnormals || StatusReg.Enable.Underflow || StatusReg.Enable.Inexact)
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            DoException = true;
        }
        else if (Except == 0 || !SetFPUException())
        {
            StatusReg.Cause.Underflow = 1;
            StatusReg.Flags.Underflow = 1;

            StatusReg.Cause.Inexact = 1;
            StatusReg.Flags.Inexact = 1;

            switch (StatusReg.RoundingMode)
            {
            case FPRoundingMode_RoundToNearest:
            case FPRoundingMode_RoundTowardZero:
                Result = Result >= 0.0f ? 0.0f : -0.0f;
                break;
            case FPRoundingMode_RoundTowardPlusInfinity:
                Result = Result >= 0.0f ? 1.175494351e-38F : -0.0f;
                break;
            case FPRoundingMode_RoundTowardMinusInfinity:
                Result = Result >= 0.0f ? 0.0f : -1.175494351e-38F;
                break;
            }
        }
        else
        {
            DoException = true;
        }
    }
    else if (Except != 0 && SetFPUException())
    {
        DoException = true;
    }
    if (DoException)
    {
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUResult64(double & Result)
{
    int Except = fetestexcept(FE_ALL_EXCEPT);
    bool DoException = false;
    int fptype = fpclassify(Result);
    if (fptype == FP_NAN)
    {
        if (Except == 0 || !SetFPUException())
        {
            *((uint64_t *)&Result) = 0x7FF7FFFFFFFFFFFF;
        }
        else
        {
            DoException = true;
        }
    }
    else if (fptype == FP_SUBNORMAL)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        if (!StatusReg.FlushSubnormals || StatusReg.Enable.Underflow || StatusReg.Enable.Inexact)
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            DoException = true;
        }
        else if (Except == 0 || !SetFPUException())
        {
            StatusReg.Cause.Underflow = 1;
            StatusReg.Flags.Underflow = 1;

            StatusReg.Cause.Inexact = 1;
            StatusReg.Flags.Inexact = 1;

            switch (StatusReg.RoundingMode)
            {
            case FPRoundingMode_RoundToNearest:
            case FPRoundingMode_RoundTowardZero:
                Result = Result >= 0.0 ? 0.0 : -0.0;
                break;
            case FPRoundingMode_RoundTowardPlusInfinity:
                Result = Result >= 0.0 ? 2.2250738585072014e-308 : -0.0;
                break;
            case FPRoundingMode_RoundTowardMinusInfinity:
                Result = Result >= 0.0 ? 0.0 : -2.2250738585072014e-308;
                break;
            }
        }
        else
        {
            DoException = true;
        }
    }
    else if (Except != 0 && SetFPUException())
    {
        DoException = true;
    }
    if (DoException)
    {
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInvalidException(void)
{
    int Except = fetestexcept(FE_ALL_EXCEPT);
    if (Except == 0)
    {
        return false;
    }

    FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
    bool Res = false;
    if ((Except & FE_INVALID) != 0)
    {
        StatusReg.Cause.UnimplementedOperation = 1;
        Res = true;
    }
    else if ((Except & FE_INEXACT) != 0)
    {
        StatusReg.Cause.Inexact = 1;
        if (StatusReg.Enable.Inexact)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Inexact = 1;
        }
    }

    if (Res)
    {
        g_Reg->TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::InitFpuOperation(FPRoundingMode RoundingModel)
{
    if (TestCop1UsableException())
    {
        return true;
    }
    _FPCR[31] &= ~0x0003F000;
    switch (RoundingModel)
    {
    case FPRoundingMode_RoundToNearest: fesetround(FE_TONEAREST); break;
    case FPRoundingMode_RoundTowardZero: fesetround(FE_TOWARDZERO); break;
    case FPRoundingMode_RoundTowardPlusInfinity: fesetround(FE_UPWARD); break;
    case FPRoundingMode_RoundTowardMinusInfinity: fesetround(FE_DOWNWARD); break;
    }
    feclearexcept(FE_ALL_EXCEPT);
    return false;
}

bool R4300iOp::SetFPUException(void)
{
    FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
    int Except = fetestexcept(FE_ALL_EXCEPT);
    if ((Except & FE_UNDERFLOW) != 0)
    {
        if (StatusReg.FlushSubnormals == 0 || StatusReg.Enable.Underflow || StatusReg.Enable.Inexact)
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            return true;
        }
    }

    bool Res = false;
    if ((Except & FE_INEXACT) != 0)
    {
        StatusReg.Cause.Inexact = 1;
        if (StatusReg.Enable.Inexact)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Inexact = 1;
        }
    }
    if ((Except & FE_UNDERFLOW) != 0)
    {
        StatusReg.Cause.Underflow = 1;
        if (StatusReg.Enable.Underflow)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Underflow = 1;
        }
    }
    if ((Except & FE_OVERFLOW) != 0)
    {
        StatusReg.Cause.Overflow = 1;
        if (StatusReg.Enable.Overflow)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Overflow = 1;
        }
    }
    if ((Except & FE_DIVBYZERO) != 0)
    {
        StatusReg.Cause.DivisionByZero = 1;
        if (StatusReg.Enable.DivisionByZero)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.DivisionByZero = 1;
        }
    }
    if ((Except & FE_INVALID) != 0)
    {
        StatusReg.Cause.InvalidOperation = 1;
        if (StatusReg.Enable.InvalidOperation)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.InvalidOperation = 1;
        }
    }
    return Res;
}
