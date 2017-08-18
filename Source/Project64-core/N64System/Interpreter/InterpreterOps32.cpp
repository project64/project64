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
#include "InterpreterOps32.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/Logging.h>

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

#define ADDRESS_ERROR_EXCEPTION(Address,FromRead) \
    g_Reg->DoAddressError(m_NextInstruction == JUMP,Address,FromRead);\
    m_NextInstruction = JUMP;\
    m_JumpToLocation = (*_PROGRAM_COUNTER);\
    return;

//#define TEST_COP1_USABLE_EXCEPTION
#define TEST_COP1_USABLE_EXCEPTION \
    if ((g_Reg->STATUS_REGISTER & STATUS_CU1) == 0) {\
    g_Reg->DoCopUnusableException(m_NextInstruction == JUMP,1);\
    m_NextInstruction = JUMP;\
    m_JumpToLocation = (*_PROGRAM_COUNTER);\
    return;\
    }

#define TLB_READ_EXCEPTION(Address) \
    g_Reg->DoTLBReadMiss(m_NextInstruction == JUMP,Address);\
    m_NextInstruction = JUMP;\
    m_JumpToLocation = (*_PROGRAM_COUNTER);\
    return;

R4300iOp32::Func * R4300iOp32::BuildInterpreter()
{
    Jump_Opcode[0] = SPECIAL;
    Jump_Opcode[1] = REGIMM;
    Jump_Opcode[2] = R4300iOp::J;
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
    Jump_Opcode[18] = R4300iOp::UnknownOpcode;
    Jump_Opcode[19] = R4300iOp::UnknownOpcode;
    Jump_Opcode[20] = BEQL;
    Jump_Opcode[21] = BNEL;
    Jump_Opcode[22] = BLEZL;
    Jump_Opcode[23] = BGTZL;
    Jump_Opcode[24] = R4300iOp::UnknownOpcode;
    Jump_Opcode[25] = R4300iOp::DADDIU;
    Jump_Opcode[26] = R4300iOp::LDL;
    Jump_Opcode[27] = R4300iOp::LDR;
    Jump_Opcode[28] = R4300iOp::UnknownOpcode;
    Jump_Opcode[29] = R4300iOp::UnknownOpcode;
    Jump_Opcode[30] = R4300iOp::UnknownOpcode;
    Jump_Opcode[31] = R4300iOp::UnknownOpcode;
    Jump_Opcode[32] = LB;
    Jump_Opcode[33] = LH;
    Jump_Opcode[34] = LWL;
    Jump_Opcode[35] = LW;
    Jump_Opcode[36] = LBU;
    Jump_Opcode[37] = LHU;
    Jump_Opcode[38] = LWR;
    Jump_Opcode[39] = LWU;
    Jump_Opcode[40] = R4300iOp::SB;
    Jump_Opcode[41] = R4300iOp::SH;
    Jump_Opcode[42] = R4300iOp::SWL;
    Jump_Opcode[43] = R4300iOp::SW;
    Jump_Opcode[44] = R4300iOp::SDL;
    Jump_Opcode[45] = R4300iOp::SDR;
    Jump_Opcode[46] = R4300iOp::SWR;
    Jump_Opcode[47] = R4300iOp::CACHE;
    Jump_Opcode[48] = R4300iOp::LL;
    Jump_Opcode[49] = R4300iOp::LWC1;
    Jump_Opcode[50] = R4300iOp::UnknownOpcode;
    Jump_Opcode[51] = R4300iOp::UnknownOpcode;
    Jump_Opcode[52] = R4300iOp::UnknownOpcode;
    Jump_Opcode[53] = R4300iOp::LDC1;
    Jump_Opcode[54] = R4300iOp::UnknownOpcode;
    Jump_Opcode[55] = R4300iOp::LD;
    Jump_Opcode[56] = R4300iOp::SC;
    Jump_Opcode[57] = R4300iOp::SWC1;
    Jump_Opcode[58] = R4300iOp::UnknownOpcode;
    Jump_Opcode[59] = R4300iOp::UnknownOpcode;
    Jump_Opcode[60] = R4300iOp::UnknownOpcode;
    Jump_Opcode[61] = R4300iOp::SDC1;
    Jump_Opcode[62] = R4300iOp::UnknownOpcode;
    Jump_Opcode[63] = R4300iOp::SD;

    Jump_Special[0] = SPECIAL_SLL;
    Jump_Special[1] = R4300iOp::UnknownOpcode;
    Jump_Special[2] = SPECIAL_SRL;
    Jump_Special[3] = SPECIAL_SRA;
    Jump_Special[4] = SPECIAL_SLLV;
    Jump_Special[5] = R4300iOp::UnknownOpcode;
    Jump_Special[6] = SPECIAL_SRLV;
    Jump_Special[7] = SPECIAL_SRAV;
    Jump_Special[8] = SPECIAL_JR;
    Jump_Special[9] = SPECIAL_JALR;
    Jump_Special[10] = R4300iOp::UnknownOpcode;
    Jump_Special[11] = R4300iOp::UnknownOpcode;
    Jump_Special[12] = R4300iOp::SPECIAL_SYSCALL;
    Jump_Special[13] = R4300iOp::UnknownOpcode;
    Jump_Special[14] = R4300iOp::UnknownOpcode;
    Jump_Special[15] = R4300iOp::SPECIAL_SYNC;
    Jump_Special[16] = R4300iOp::SPECIAL_MFHI;
    Jump_Special[17] = R4300iOp::SPECIAL_MTHI;
    Jump_Special[18] = R4300iOp::SPECIAL_MFLO;
    Jump_Special[19] = R4300iOp::SPECIAL_MTLO;
    Jump_Special[20] = R4300iOp::SPECIAL_DSLLV;
    Jump_Special[21] = R4300iOp::UnknownOpcode;
    Jump_Special[22] = R4300iOp::SPECIAL_DSRLV;
    Jump_Special[23] = R4300iOp::SPECIAL_DSRAV;
    Jump_Special[24] = R4300iOp::SPECIAL_MULT;
    Jump_Special[25] = R4300iOp::SPECIAL_MULTU;
    Jump_Special[26] = R4300iOp::SPECIAL_DIV;
    Jump_Special[27] = R4300iOp::SPECIAL_DIVU;
    Jump_Special[28] = R4300iOp::SPECIAL_DMULT;
    Jump_Special[29] = R4300iOp::SPECIAL_DMULTU;
    Jump_Special[30] = R4300iOp::SPECIAL_DDIV;
    Jump_Special[31] = R4300iOp::SPECIAL_DDIVU;
    Jump_Special[32] = SPECIAL_ADD;
    Jump_Special[33] = SPECIAL_ADDU;
    Jump_Special[34] = SPECIAL_SUB;
    Jump_Special[35] = SPECIAL_SUBU;
    Jump_Special[36] = SPECIAL_AND;
    Jump_Special[37] = SPECIAL_OR;
    Jump_Special[38] = SPECIAL_XOR;
    Jump_Special[39] = SPECIAL_NOR;
    Jump_Special[40] = R4300iOp::UnknownOpcode;
    Jump_Special[41] = R4300iOp::UnknownOpcode;
    Jump_Special[42] = SPECIAL_SLT;
    Jump_Special[43] = SPECIAL_SLTU;
    Jump_Special[44] = R4300iOp::SPECIAL_DADD;
    Jump_Special[45] = R4300iOp::SPECIAL_DADDU;
    Jump_Special[46] = R4300iOp::SPECIAL_DSUB;
    Jump_Special[47] = R4300iOp::SPECIAL_DSUBU;
    Jump_Special[48] = R4300iOp::UnknownOpcode;
    Jump_Special[49] = R4300iOp::UnknownOpcode;
    Jump_Special[50] = R4300iOp::UnknownOpcode;
    Jump_Special[51] = R4300iOp::UnknownOpcode;
    Jump_Special[52] = R4300iOp::SPECIAL_TEQ;
    Jump_Special[53] = R4300iOp::UnknownOpcode;
    Jump_Special[54] = R4300iOp::UnknownOpcode;
    Jump_Special[55] = R4300iOp::UnknownOpcode;
    Jump_Special[56] = R4300iOp::SPECIAL_DSLL;
    Jump_Special[57] = R4300iOp::UnknownOpcode;
    Jump_Special[58] = R4300iOp::SPECIAL_DSRL;
    Jump_Special[59] = R4300iOp::SPECIAL_DSRA;
    Jump_Special[60] = R4300iOp::SPECIAL_DSLL32;
    Jump_Special[61] = R4300iOp::UnknownOpcode;
    Jump_Special[62] = R4300iOp::SPECIAL_DSRL32;
    Jump_Special[63] = R4300iOp::SPECIAL_DSRA32;

    Jump_Regimm[0] = REGIMM_BLTZ;
    Jump_Regimm[1] = REGIMM_BGEZ;
    Jump_Regimm[2] = REGIMM_BLTZL;
    Jump_Regimm[3] = REGIMM_BGEZL;
    Jump_Regimm[4] = R4300iOp::UnknownOpcode;
    Jump_Regimm[5] = R4300iOp::UnknownOpcode;
    Jump_Regimm[6] = R4300iOp::UnknownOpcode;
    Jump_Regimm[7] = R4300iOp::UnknownOpcode;
    Jump_Regimm[8] = R4300iOp::UnknownOpcode;
    Jump_Regimm[9] = R4300iOp::UnknownOpcode;
    Jump_Regimm[10] = R4300iOp::UnknownOpcode;
    Jump_Regimm[11] = R4300iOp::UnknownOpcode;
    Jump_Regimm[12] = R4300iOp::UnknownOpcode;
    Jump_Regimm[13] = R4300iOp::UnknownOpcode;
    Jump_Regimm[14] = R4300iOp::UnknownOpcode;
    Jump_Regimm[15] = R4300iOp::UnknownOpcode;
    Jump_Regimm[16] = REGIMM_BLTZAL;
    Jump_Regimm[17] = REGIMM_BGEZAL;
    Jump_Regimm[18] = R4300iOp::UnknownOpcode;
    Jump_Regimm[19] = R4300iOp::UnknownOpcode;
    Jump_Regimm[20] = R4300iOp::UnknownOpcode;
    Jump_Regimm[21] = R4300iOp::UnknownOpcode;
    Jump_Regimm[22] = R4300iOp::UnknownOpcode;
    Jump_Regimm[23] = R4300iOp::UnknownOpcode;
    Jump_Regimm[24] = R4300iOp::UnknownOpcode;
    Jump_Regimm[25] = R4300iOp::UnknownOpcode;
    Jump_Regimm[26] = R4300iOp::UnknownOpcode;
    Jump_Regimm[27] = R4300iOp::UnknownOpcode;
    Jump_Regimm[28] = R4300iOp::UnknownOpcode;
    Jump_Regimm[29] = R4300iOp::UnknownOpcode;
    Jump_Regimm[30] = R4300iOp::UnknownOpcode;
    Jump_Regimm[31] = R4300iOp::UnknownOpcode;

    Jump_CoP0[0] = COP0_MF;
    Jump_CoP0[1] = R4300iOp::UnknownOpcode;
    Jump_CoP0[2] = R4300iOp::UnknownOpcode;
    Jump_CoP0[3] = R4300iOp::UnknownOpcode;
    Jump_CoP0[4] = COP0_MT;
    Jump_CoP0[5] = R4300iOp::UnknownOpcode;
    Jump_CoP0[6] = R4300iOp::UnknownOpcode;
    Jump_CoP0[7] = R4300iOp::UnknownOpcode;
    Jump_CoP0[8] = R4300iOp::UnknownOpcode;
    Jump_CoP0[9] = R4300iOp::UnknownOpcode;
    Jump_CoP0[10] = R4300iOp::UnknownOpcode;
    Jump_CoP0[11] = R4300iOp::UnknownOpcode;
    Jump_CoP0[12] = R4300iOp::UnknownOpcode;
    Jump_CoP0[13] = R4300iOp::UnknownOpcode;
    Jump_CoP0[14] = R4300iOp::UnknownOpcode;
    Jump_CoP0[15] = R4300iOp::UnknownOpcode;
    Jump_CoP0[16] = R4300iOp::COP0_CO;
    Jump_CoP0[17] = R4300iOp::COP0_CO;
    Jump_CoP0[18] = R4300iOp::COP0_CO;
    Jump_CoP0[19] = R4300iOp::COP0_CO;
    Jump_CoP0[20] = R4300iOp::COP0_CO;
    Jump_CoP0[21] = R4300iOp::COP0_CO;
    Jump_CoP0[22] = R4300iOp::COP0_CO;
    Jump_CoP0[23] = R4300iOp::COP0_CO;
    Jump_CoP0[24] = R4300iOp::COP0_CO;
    Jump_CoP0[25] = R4300iOp::COP0_CO;
    Jump_CoP0[26] = R4300iOp::COP0_CO;
    Jump_CoP0[27] = R4300iOp::COP0_CO;
    Jump_CoP0[28] = R4300iOp::COP0_CO;
    Jump_CoP0[29] = R4300iOp::COP0_CO;
    Jump_CoP0[30] = R4300iOp::COP0_CO;
    Jump_CoP0[31] = R4300iOp::COP0_CO;

    Jump_CoP0_Function[0] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[1] = R4300iOp::COP0_CO_TLBR;
    Jump_CoP0_Function[2] = R4300iOp::COP0_CO_TLBWI;
    Jump_CoP0_Function[3] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[4] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[5] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[6] = R4300iOp::COP0_CO_TLBWR;
    Jump_CoP0_Function[7] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[8] = R4300iOp::COP0_CO_TLBP;
    Jump_CoP0_Function[9] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[10] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[11] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[12] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[13] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[14] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[15] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[16] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[17] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[18] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[19] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[20] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[21] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[22] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[23] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[24] = R4300iOp::COP0_CO_ERET;
    Jump_CoP0_Function[25] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[26] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[27] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[28] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[29] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[30] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[31] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[32] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[33] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[34] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[35] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[36] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[37] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[38] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[39] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[40] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[41] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[42] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[43] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[44] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[45] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[46] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[47] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[48] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[49] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[50] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[51] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[52] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[53] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[54] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[55] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[56] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[57] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[58] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[59] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[60] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[61] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[62] = R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[63] = R4300iOp::UnknownOpcode;

    Jump_CoP1[0] = COP1_MF;
    Jump_CoP1[1] = R4300iOp::COP1_DMF;
    Jump_CoP1[2] = COP1_CF;
    Jump_CoP1[3] = R4300iOp::UnknownOpcode;
    Jump_CoP1[4] = R4300iOp::COP1_MT;
    Jump_CoP1[5] = COP1_DMT;
    Jump_CoP1[6] = R4300iOp::COP1_CT;
    Jump_CoP1[7] = R4300iOp::UnknownOpcode;
    Jump_CoP1[8] = R4300iOp::COP1_BC;
    Jump_CoP1[9] = R4300iOp::UnknownOpcode;
    Jump_CoP1[10] = R4300iOp::UnknownOpcode;
    Jump_CoP1[11] = R4300iOp::UnknownOpcode;
    Jump_CoP1[12] = R4300iOp::UnknownOpcode;
    Jump_CoP1[13] = R4300iOp::UnknownOpcode;
    Jump_CoP1[14] = R4300iOp::UnknownOpcode;
    Jump_CoP1[15] = R4300iOp::UnknownOpcode;
    Jump_CoP1[16] = R4300iOp::COP1_S;
    Jump_CoP1[17] = R4300iOp::COP1_D;
    Jump_CoP1[18] = R4300iOp::UnknownOpcode;
    Jump_CoP1[19] = R4300iOp::UnknownOpcode;
    Jump_CoP1[20] = R4300iOp::COP1_W;
    Jump_CoP1[21] = R4300iOp::COP1_L;
    Jump_CoP1[22] = R4300iOp::UnknownOpcode;
    Jump_CoP1[23] = R4300iOp::UnknownOpcode;
    Jump_CoP1[24] = R4300iOp::UnknownOpcode;
    Jump_CoP1[25] = R4300iOp::UnknownOpcode;
    Jump_CoP1[26] = R4300iOp::UnknownOpcode;
    Jump_CoP1[27] = R4300iOp::UnknownOpcode;
    Jump_CoP1[28] = R4300iOp::UnknownOpcode;
    Jump_CoP1[29] = R4300iOp::UnknownOpcode;
    Jump_CoP1[30] = R4300iOp::UnknownOpcode;
    Jump_CoP1[31] = R4300iOp::UnknownOpcode;

    Jump_CoP1_BC[0] = R4300iOp::COP1_BCF;
    Jump_CoP1_BC[1] = R4300iOp::COP1_BCT;
    Jump_CoP1_BC[2] = R4300iOp::COP1_BCFL;
    Jump_CoP1_BC[3] = R4300iOp::COP1_BCTL;
    Jump_CoP1_BC[4] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[5] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[6] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[7] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[8] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[9] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[10] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[11] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[12] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[13] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[14] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[15] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[16] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[17] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[18] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[19] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[20] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[21] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[22] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[23] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[24] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[25] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[26] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[27] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[28] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[29] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[30] = R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[31] = R4300iOp::UnknownOpcode;

    Jump_CoP1_S[0] = R4300iOp::COP1_S_ADD;
    Jump_CoP1_S[1] = R4300iOp::COP1_S_SUB;
    Jump_CoP1_S[2] = R4300iOp::COP1_S_MUL;
    Jump_CoP1_S[3] = R4300iOp::COP1_S_DIV;
    Jump_CoP1_S[4] = R4300iOp::COP1_S_SQRT;
    Jump_CoP1_S[5] = R4300iOp::COP1_S_ABS;
    Jump_CoP1_S[6] = R4300iOp::COP1_S_MOV;
    Jump_CoP1_S[7] = R4300iOp::COP1_S_NEG;
	Jump_CoP1_S[8] = R4300iOp::COP1_S_ROUND_L;
    Jump_CoP1_S[9] = R4300iOp::COP1_S_TRUNC_L;
    Jump_CoP1_S[10] = R4300iOp::COP1_S_CEIL_L;		
    Jump_CoP1_S[11] = R4300iOp::COP1_S_FLOOR_L;		
    Jump_CoP1_S[12] = R4300iOp::COP1_S_ROUND_W;
    Jump_CoP1_S[13] = R4300iOp::COP1_S_TRUNC_W;
    Jump_CoP1_S[14] = R4300iOp::COP1_S_CEIL_W;		
    Jump_CoP1_S[15] = R4300iOp::COP1_S_FLOOR_W;
    Jump_CoP1_S[16] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[17] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[18] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[19] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[20] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[21] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[22] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[23] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[24] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[25] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[26] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[27] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[28] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[29] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[30] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[31] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[32] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[33] = R4300iOp::COP1_S_CVT_D;
    Jump_CoP1_S[34] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[35] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[36] = R4300iOp::COP1_S_CVT_W;
    Jump_CoP1_S[37] = R4300iOp::COP1_S_CVT_L;
    Jump_CoP1_S[38] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[39] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[40] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[41] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[42] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[43] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[44] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[45] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[46] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[47] = R4300iOp::UnknownOpcode;
    Jump_CoP1_S[48] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[49] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[50] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[51] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[52] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[53] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[54] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[55] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[56] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[57] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[58] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[59] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[60] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[61] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[62] = R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[63] = R4300iOp::COP1_S_CMP;

    Jump_CoP1_D[0] = R4300iOp::COP1_D_ADD;
    Jump_CoP1_D[1] = R4300iOp::COP1_D_SUB;
    Jump_CoP1_D[2] = R4300iOp::COP1_D_MUL;
    Jump_CoP1_D[3] = R4300iOp::COP1_D_DIV;
    Jump_CoP1_D[4] = R4300iOp::COP1_D_SQRT;
    Jump_CoP1_D[5] = R4300iOp::COP1_D_ABS;
    Jump_CoP1_D[6] = R4300iOp::COP1_D_MOV;
    Jump_CoP1_D[7] = R4300iOp::COP1_D_NEG;
    Jump_CoP1_D[8] = R4300iOp::COP1_D_ROUND_L;
    Jump_CoP1_D[9] = R4300iOp::COP1_D_TRUNC_L;		
    Jump_CoP1_D[10] = R4300iOp::COP1_D_CEIL_L;		
    Jump_CoP1_D[11] = R4300iOp::COP1_D_FLOOR_L;		
    Jump_CoP1_D[12] = R4300iOp::COP1_D_ROUND_W;
    Jump_CoP1_D[13] = R4300iOp::COP1_D_TRUNC_W;
    Jump_CoP1_D[14] = R4300iOp::COP1_D_CEIL_W;		
    Jump_CoP1_D[15] = R4300iOp::COP1_D_FLOOR_W;		
    Jump_CoP1_D[16] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[17] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[18] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[19] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[20] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[21] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[22] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[23] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[24] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[25] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[26] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[27] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[28] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[29] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[30] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[31] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[32] = R4300iOp::COP1_D_CVT_S;
    Jump_CoP1_D[33] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[34] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[35] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[36] = R4300iOp::COP1_D_CVT_W;
    Jump_CoP1_D[37] = R4300iOp::COP1_D_CVT_L;
    Jump_CoP1_D[38] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[39] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[40] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[41] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[42] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[43] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[44] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[45] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[46] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[47] = R4300iOp::UnknownOpcode;
    Jump_CoP1_D[48] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[49] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[50] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[51] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[52] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[53] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[54] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[55] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[56] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[57] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[58] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[59] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[60] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[61] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[62] = R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[63] = R4300iOp::COP1_D_CMP;

    Jump_CoP1_W[0] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[1] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[2] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[3] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[4] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[5] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[6] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[7] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[8] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[9] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[10] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[11] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[12] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[13] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[14] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[15] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[16] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[17] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[18] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[19] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[20] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[21] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[22] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[23] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[24] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[25] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[26] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[27] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[28] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[29] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[30] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[31] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[32] = R4300iOp::COP1_W_CVT_S;
    Jump_CoP1_W[33] = R4300iOp::COP1_W_CVT_D;
    Jump_CoP1_W[34] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[35] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[36] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[37] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[38] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[39] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[40] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[41] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[42] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[43] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[44] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[45] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[46] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[47] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[48] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[49] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[50] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[51] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[52] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[53] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[54] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[55] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[56] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[57] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[58] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[59] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[60] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[61] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[62] = R4300iOp::UnknownOpcode;
    Jump_CoP1_W[63] = R4300iOp::UnknownOpcode;

    Jump_CoP1_L[0] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[1] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[2] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[3] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[4] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[5] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[6] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[7] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[8] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[9] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[10] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[11] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[12] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[13] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[14] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[15] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[16] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[17] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[18] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[19] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[20] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[21] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[22] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[23] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[24] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[25] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[26] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[27] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[28] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[29] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[30] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[31] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[32] = R4300iOp::COP1_L_CVT_S;
    Jump_CoP1_L[33] = R4300iOp::COP1_L_CVT_D;
    Jump_CoP1_L[34] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[35] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[36] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[37] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[38] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[39] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[40] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[41] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[42] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[43] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[44] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[45] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[46] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[47] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[48] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[49] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[50] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[51] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[52] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[53] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[54] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[55] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[56] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[57] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[58] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[59] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[60] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[61] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[62] = R4300iOp::UnknownOpcode;
    Jump_CoP1_L[63] = R4300iOp::UnknownOpcode;

    return Jump_Opcode;
}

/************************* Opcode functions *************************/
void R4300iOp32::JAL()
{
    m_NextInstruction = DELAY_SLOT;
    m_JumpToLocation = ((*_PROGRAM_COUNTER) & 0xF0000000) + (m_Opcode.target << 2);
    _GPR[31].UW[0] = (*_PROGRAM_COUNTER) + 8;

    if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
    {
        m_NextInstruction = PERMLOOP_DO_DELAY;
    }
}

void R4300iOp32::BEQ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] == _GPR[m_Opcode.rt].W[0])
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

void R4300iOp32::BNE()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] != _GPR[m_Opcode.rt].W[0])
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

void R4300iOp32::BLEZ() {
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] <= 0)
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

void R4300iOp32::BGTZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] > 0)
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

void R4300iOp32::ADDI()
{
#ifdef Interpreter_StackTest
    if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        StackValue += (int16_t)m_Opcode.immediate;
    }
#endif
    _GPR[m_Opcode.rt].W[0] = (_GPR[m_Opcode.rs].W[0] + ((int16_t)m_Opcode.immediate));
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp32::ADDIU()
{
#ifdef Interpreter_StackTest
    if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        StackValue += (int16_t)m_Opcode.immediate;
    }
#endif
    _GPR[m_Opcode.rt].W[0] = (_GPR[m_Opcode.rs].W[0] + ((int16_t)m_Opcode.immediate));
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp32::SLTI()
{
    if (_GPR[m_Opcode.rs].W[0] < (int64_t)((int16_t)m_Opcode.immediate))
    {
        _GPR[m_Opcode.rt].W[0] = 1;
    }
    else
    {
        _GPR[m_Opcode.rt].W[0] = 0;
    }
}

void R4300iOp32::SLTIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rs].UW[0] < (uint64_t)imm64 ? 1 : 0;
}

void R4300iOp32::ANDI()
{
    _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rs].W[0] & m_Opcode.immediate;
}

void R4300iOp32::ORI()
{
    _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rs].W[0] | m_Opcode.immediate;
}

void R4300iOp32::XORI()
{
    _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rs].W[0] ^ m_Opcode.immediate;
}

void R4300iOp32::LUI()
{
    _GPR[m_Opcode.rt].W[0] = (int32_t)((int16_t)m_Opcode.offset << 16);
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29)
    {
        StackValue = _GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp32::BEQL()
{
    if (_GPR[m_Opcode.rs].W[0] == _GPR[m_Opcode.rt].W[0])
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

void R4300iOp32::BNEL()
{
    if (_GPR[m_Opcode.rs].W[0] != _GPR[m_Opcode.rt].W[0])
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

void R4300iOp32::BLEZL()
{
    if (_GPR[m_Opcode.rs].W[0] <= 0)
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

void R4300iOp32::BGTZL()
{
    if (_GPR[m_Opcode.rs].W[0] > 0)
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

void R4300iOp32::LB()
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
        _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rt].B[0];
    }
}

void R4300iOp32::LH()
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
        _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rt].HW[0];
    }
}

void R4300iOp32::LWL()
{
    uint32_t Offset, Address, Value;

    Address = _GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset;
    Offset = Address & 3;

    if (!g_MMU->LW_VAddr((Address & ~3), Value))
    {
        if (bShowTLBMisses())
        {
            g_Notify->DisplayError(stdstr_f("%s TLB: %X", __FUNCTION__, Address).c_str());
        }
        TLB_READ_EXCEPTION(Address);
        return;
    }

    _GPR[m_Opcode.rt].W[0] = (int32_t)(_GPR[m_Opcode.rt].W[0] & LWL_MASK[Offset]);
    _GPR[m_Opcode.rt].W[0] += (int32_t)(Value << LWL_SHIFT[Offset]);
}

void R4300iOp32::LW()
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
        _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rt].W[0];
    }
}

void R4300iOp32::LBU()
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
        _GPR[m_Opcode.rt].UW[0] = _GPR[m_Opcode.rt].UB[0];
    }
}

void R4300iOp32::LHU()
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
        _GPR[m_Opcode.rt].UW[0] = _GPR[m_Opcode.rt].UHW[0];
    }
}

void R4300iOp32::LWR()
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

    _GPR[m_Opcode.rt].W[0] = (int32_t)(_GPR[m_Opcode.rt].W[0] & LWR_MASK[Offset]);
    _GPR[m_Opcode.rt].W[0] += (int32_t)(Value >> LWR_SHIFT[Offset]);
}

void R4300iOp32::LWU()
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
        _GPR[m_Opcode.rt].UW[0] = _GPR[m_Opcode.rt].UW[0];
    }
}

void R4300iOp32::LL()
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
        _GPR[m_Opcode.rt].W[0] = _GPR[m_Opcode.rt].W[0];
        (*_LLBit) = 1;
    }
}

/********************** R4300i OpCodes: Special **********************/
void R4300iOp32::SPECIAL_SLL()
{
    _GPR[m_Opcode.rd].W[0] = (_GPR[m_Opcode.rt].W[0] << m_Opcode.sa);
}

void R4300iOp32::SPECIAL_SRL()
{
    _GPR[m_Opcode.rd].W[0] = (int32_t)(_GPR[m_Opcode.rt].UW[0] >> m_Opcode.sa);
}

void R4300iOp32::SPECIAL_SRA()
{
    _GPR[m_Opcode.rd].W[0] = (_GPR[m_Opcode.rt].W[0] >> m_Opcode.sa);
}

void R4300iOp32::SPECIAL_SLLV()
{
    _GPR[m_Opcode.rd].W[0] = (_GPR[m_Opcode.rt].W[0] << (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp32::SPECIAL_SRLV()
{
    _GPR[m_Opcode.rd].W[0] = (int32_t)(_GPR[m_Opcode.rt].UW[0] >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp32::SPECIAL_SRAV()
{
    _GPR[m_Opcode.rd].W[0] = (_GPR[m_Opcode.rt].W[0] >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp32::SPECIAL_JALR()
{
    m_NextInstruction = DELAY_SLOT;
    m_JumpToLocation = _GPR[m_Opcode.rs].UW[0];
    _GPR[m_Opcode.rd].W[0] = (int32_t)((*_PROGRAM_COUNTER) + 8);
    m_TestTimer = true;
}

void R4300iOp32::SPECIAL_ADD()
{
    _GPR[m_Opcode.rd].W[0] = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp32::SPECIAL_ADDU()
{
    _GPR[m_Opcode.rd].W[0] = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp32::SPECIAL_SUB()
{
    _GPR[m_Opcode.rd].W[0] = _GPR[m_Opcode.rs].W[0] - _GPR[m_Opcode.rt].W[0];
}

void R4300iOp32::SPECIAL_SUBU()
{
    _GPR[m_Opcode.rd].W[0] = _GPR[m_Opcode.rs].W[0] - _GPR[m_Opcode.rt].W[0];
}

void R4300iOp32::SPECIAL_AND()
{
    _GPR[m_Opcode.rd].W[0] = _GPR[m_Opcode.rs].W[0] & _GPR[m_Opcode.rt].W[0];
}

void R4300iOp32::SPECIAL_OR()
{
    _GPR[m_Opcode.rd].W[0] = _GPR[m_Opcode.rs].W[0] | _GPR[m_Opcode.rt].W[0];
#ifdef Interpreter_StackTest
    if (m_Opcode.rd == 29)
    {
        StackValue = _GPR[m_Opcode.rd].W[0];
    }
#endif
}

void R4300iOp32::SPECIAL_NOR()
{
    _GPR[m_Opcode.rd].W[0] = ~(_GPR[m_Opcode.rs].W[0] | _GPR[m_Opcode.rt].W[0]);
}

void R4300iOp32::SPECIAL_SLT()
{
    if (_GPR[m_Opcode.rs].W[0] < _GPR[m_Opcode.rt].W[0])
    {
        _GPR[m_Opcode.rd].W[0] = 1;
    }
    else
    {
        _GPR[m_Opcode.rd].W[0] = 0;
    }
}

void R4300iOp32::SPECIAL_SLTU()
{
    if (_GPR[m_Opcode.rs].UW[0] < _GPR[m_Opcode.rt].UW[0])
    {
        _GPR[m_Opcode.rd].W[0] = 1;
    }
    else
    {
        _GPR[m_Opcode.rd].W[0] = 0;
    }
}

void R4300iOp32::SPECIAL_TEQ()
{
    if (_GPR[m_Opcode.rs].W[0] == _GPR[m_Opcode.rt].W[0] && g_Settings->LoadBool(Debugger_Enabled))
    {
        g_Notify->DisplayError("Should trap this ???");
    }
}

/********************** R4300i OpCodes: RegImm **********************/
void R4300iOp32::REGIMM_BLTZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] < 0)
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

void R4300iOp32::REGIMM_BGEZ()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] >= 0)
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

void R4300iOp32::REGIMM_BLTZL()
{
    if (_GPR[m_Opcode.rs].W[0] < 0)
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

void R4300iOp32::REGIMM_BGEZL()
{
    if (_GPR[m_Opcode.rs].W[0] >= 0)
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

void R4300iOp32::REGIMM_BLTZAL()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] < 0)
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
    _GPR[31].W[0] = (int32_t)((*_PROGRAM_COUNTER) + 8);
}

void R4300iOp32::REGIMM_BGEZAL()
{
    m_NextInstruction = DELAY_SLOT;
    if (_GPR[m_Opcode.rs].W[0] >= 0)
    {
        m_JumpToLocation = (*_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
        if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
        {
			if (g_Settings->LoadBool(Debugger_Enabled))
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
    _GPR[31].W[0] = (int32_t)((*_PROGRAM_COUNTER) + 8);
}

/************************** COP0 functions **************************/
void R4300iOp32::COP0_MF()
{
    if (LogCP0reads())
    {
        LogMessage("%08X: R4300i Read from %s (0x%08X)", (*_PROGRAM_COUNTER), CRegName::Cop0[m_Opcode.rd], _CP0[m_Opcode.rd]);
    }

    if (m_Opcode.rd == 9)
    {
        g_SystemTimer->UpdateTimers();
    }
    _GPR[m_Opcode.rt].W[0] = (int32_t)_CP0[m_Opcode.rd];
}

void R4300iOp32::COP0_MT()
{
    if (LogCP0changes())
    {
        LogMessage("%08X: Writing 0x%X to %s register (Originally: 0x%08X)", (*_PROGRAM_COUNTER), _GPR[m_Opcode.rt].UW[0], CRegName::Cop0[m_Opcode.rd], _CP0[m_Opcode.rd]);
        if (m_Opcode.rd == 11) //Compare
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
        if ((_CP0[m_Opcode.rd] & 0x18) != 0 && g_Settings->LoadBool(Debugger_Enabled))
        {
            g_Notify->DisplayError("Left kernel mode ??");
        }
        g_Reg->CheckInterrupts();
        break;
    case 13: //cause
        _CP0[m_Opcode.rd] &= 0xFFFFCFF;
        if ((_GPR[m_Opcode.rt].UW[0] & 0x300) != 0 && g_Settings->LoadBool(Debugger_Enabled))
        {
            g_Notify->DisplayError("Set IP0 or IP1");
        }
        break;
    default:
        UnknownOpcode();
    }
}

/************************** COP1 functions **************************/
void R4300iOp32::COP1_MF()
{
    TEST_COP1_USABLE_EXCEPTION
        _GPR[m_Opcode.rt].W[0] = *(int32_t *)_FPR_S[m_Opcode.fs];
}

void R4300iOp32::COP1_CF()
{
    TEST_COP1_USABLE_EXCEPTION
        if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
        {
            if (g_Settings->LoadBool(Debugger_Enabled)) { g_Notify->DisplayError("CFC1 what register are you writing to ?"); }
            return;
        }
    _GPR[m_Opcode.rt].W[0] = (int32_t)_FPCR[m_Opcode.fs];
}

void R4300iOp32::COP1_DMT()
{
    TEST_COP1_USABLE_EXCEPTION
        *(int64_t *)_FPR_D[m_Opcode.fs] = _GPR[m_Opcode.rt].W[0];
}