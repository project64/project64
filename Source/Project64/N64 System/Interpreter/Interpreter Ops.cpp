/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include "stdafx.h"
#include "../C Core/Logging.h"

void InPermLoop         ( void );
void TestInterpreterJump      ( DWORD PC, DWORD TargetPC, int Reg1, int Reg2 );

BOOL        R4300iOp::m_TestTimer = false;
DWORD       R4300iOp::m_NextInstruction;
OPCODE      R4300iOp::m_Opcode;
DWORD       R4300iOp::m_JumpToLocation;

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

const DWORD R4300iOp::SWL_MASK[4]  = { 0x00000000, 0xFF000000,0xFFFF0000,0xFFFFFF00 };
const DWORD R4300iOp::SWR_MASK[4]  = { 0x00FFFFFF, 0x0000FFFF,0x000000FF,0x00000000 };
const DWORD R4300iOp::LWL_MASK[4]  = { 0x00000000, 0x000000FF,0x0000FFFF,0x00FFFFFF };
const DWORD R4300iOp::LWR_MASK[4]  = { 0xFFFFFF00, 0xFFFF0000,0xFF000000,0x0000000 };

const int   R4300iOp::SWL_SHIFT[4] = { 0, 8, 16, 24 };
const int   R4300iOp::SWR_SHIFT[4] = { 24, 16 , 8, 0  };
const int   R4300iOp::LWL_SHIFT[4] = { 0, 8, 16, 24};
const int   R4300iOp::LWR_SHIFT[4] = { 24, 16 ,8, 0 };

#define ADDRESS_ERROR_EXCEPTION(Address,FromRead) \
	_Reg->DoAddressError(m_NextInstruction == JUMP,Address,FromRead);\
	m_NextInstruction = JUMP;\
	m_JumpToLocation = (*_PROGRAM_COUNTER);\
	return;

//#define TEST_COP1_USABLE_EXCEPTION
#define TEST_COP1_USABLE_EXCEPTION \
	if ((_Reg->STATUS_REGISTER & STATUS_CU1) == 0) {\
		_Reg->DoCopUnusableException(m_NextInstruction == JUMP,1);\
		m_NextInstruction = JUMP;\
		m_JumpToLocation = (*_PROGRAM_COUNTER);\
		return;\
	}

#define TLB_READ_EXCEPTION(Address) \
	_Reg->DoTLBMiss(m_NextInstruction == JUMP,Address);\
	m_NextInstruction = JUMP;\
	m_JumpToLocation = (*_PROGRAM_COUNTER);\
	return;

void R4300iOp::SPECIAL (void) {
	Jump_Special[ m_Opcode.funct ]();
}

void R4300iOp::REGIMM (void) {
	Jump_Regimm[ m_Opcode.rt ]();
}

void R4300iOp::COP0 (void) {
	Jump_CoP0[ m_Opcode.rs ]();
}

void R4300iOp::COP0_CO (void) {
	Jump_CoP0_Function[ m_Opcode.funct ]();
}

void R4300iOp::COP1 (void) {
	Jump_CoP1[ m_Opcode.fmt ]();
}

void R4300iOp::COP1_BC (void) {
	Jump_CoP1_BC[ m_Opcode.ft ]();
}

void R4300iOp::COP1_S (void) {
	_controlfp(*_RoundingModel,_MCW_RC);
	Jump_CoP1_S[ m_Opcode.funct ]();
}

void R4300iOp::COP1_D (void) {
	_controlfp(*_RoundingModel,_MCW_RC);
	Jump_CoP1_D[ m_Opcode.funct ]();
}

void R4300iOp::COP1_W (void) {
	Jump_CoP1_W[ m_Opcode.funct ]();
}

void R4300iOp::COP1_L (void) {
	Jump_CoP1_L[ m_Opcode.funct ]();
}

R4300iOp::Func * R4300iOp::BuildInterpreter (void )
{ 
	Jump_Opcode[ 0] = SPECIAL;
	Jump_Opcode[ 1] = REGIMM;
	Jump_Opcode[ 2] = J;
	Jump_Opcode[ 3] = JAL;
	Jump_Opcode[ 4] = BEQ;
	Jump_Opcode[ 5] = BNE;
	Jump_Opcode[ 6] = BLEZ;
	Jump_Opcode[ 7] = BGTZ;
	Jump_Opcode[ 8] = ADDI;
	Jump_Opcode[ 9] = ADDIU;
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

	Jump_Special[ 0] = SPECIAL_SLL;
	Jump_Special[ 1] = UnknownOpcode;
	Jump_Special[ 2] = SPECIAL_SRL;
	Jump_Special[ 3] = SPECIAL_SRA;
	Jump_Special[ 4] = SPECIAL_SLLV;
	Jump_Special[ 5] = UnknownOpcode;
	Jump_Special[ 6] = SPECIAL_SRLV;
	Jump_Special[ 7] = SPECIAL_SRAV;
	Jump_Special[ 8] = SPECIAL_JR;
	Jump_Special[ 9] = SPECIAL_JALR;
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

	Jump_Regimm[ 0] = REGIMM_BLTZ;
	Jump_Regimm[ 1] = REGIMM_BGEZ;
	Jump_Regimm[ 2] = REGIMM_BLTZL;
	Jump_Regimm[ 3] = REGIMM_BGEZL;
	Jump_Regimm[ 4] = UnknownOpcode;
	Jump_Regimm[ 5] = UnknownOpcode;
	Jump_Regimm[ 6] = UnknownOpcode;
	Jump_Regimm[ 7] = UnknownOpcode;
	Jump_Regimm[ 8] = UnknownOpcode;
	Jump_Regimm[ 9] = UnknownOpcode;
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
	
	Jump_CoP0[ 0] = COP0_MF;
	Jump_CoP0[ 1] = UnknownOpcode;
	Jump_CoP0[ 2] = UnknownOpcode;
	Jump_CoP0[ 3] = UnknownOpcode;
	Jump_CoP0[ 4] = COP0_MT;
	Jump_CoP0[ 5] = UnknownOpcode;
	Jump_CoP0[ 6] = UnknownOpcode;
	Jump_CoP0[ 7] = UnknownOpcode;
	Jump_CoP0[ 8] = UnknownOpcode;
	Jump_CoP0[ 9] = UnknownOpcode;
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

	Jump_CoP0_Function[ 0] = UnknownOpcode;
	Jump_CoP0_Function[ 1] = COP0_CO_TLBR;
	Jump_CoP0_Function[ 2] = COP0_CO_TLBWI;
	Jump_CoP0_Function[ 3] = UnknownOpcode;
	Jump_CoP0_Function[ 4] = UnknownOpcode;
	Jump_CoP0_Function[ 5] = UnknownOpcode;
	Jump_CoP0_Function[ 6] = COP0_CO_TLBWR;
	Jump_CoP0_Function[ 7] = UnknownOpcode;
	Jump_CoP0_Function[ 8] = COP0_CO_TLBP;
	Jump_CoP0_Function[ 9] = UnknownOpcode;
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
	
	Jump_CoP1[ 0] = COP1_MF;
	Jump_CoP1[ 1] = COP1_DMF;
	Jump_CoP1[ 2] = COP1_CF;
	Jump_CoP1[ 3] = UnknownOpcode;
	Jump_CoP1[ 4] = COP1_MT;
	Jump_CoP1[ 5] = COP1_DMT;
	Jump_CoP1[ 6] = COP1_CT;
	Jump_CoP1[ 7] = UnknownOpcode;
	Jump_CoP1[ 8] = COP1_BC;
	Jump_CoP1[ 9] = UnknownOpcode;
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

	Jump_CoP1_BC[ 0] = COP1_BCF;
	Jump_CoP1_BC[ 1] = COP1_BCT;
	Jump_CoP1_BC[ 2] = COP1_BCFL;
	Jump_CoP1_BC[ 3] = COP1_BCTL;
	Jump_CoP1_BC[ 4] = UnknownOpcode;
	Jump_CoP1_BC[ 5] = UnknownOpcode;
	Jump_CoP1_BC[ 6] = UnknownOpcode;
	Jump_CoP1_BC[ 7] = UnknownOpcode;
	Jump_CoP1_BC[ 8] = UnknownOpcode;
	Jump_CoP1_BC[ 9] = UnknownOpcode;
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

	Jump_CoP1_S[ 0] = COP1_S_ADD;
	Jump_CoP1_S[ 1] = COP1_S_SUB;
	Jump_CoP1_S[ 2] = COP1_S_MUL;
	Jump_CoP1_S[ 3] = COP1_S_DIV;
	Jump_CoP1_S[ 4] = COP1_S_SQRT;
	Jump_CoP1_S[ 5] = COP1_S_ABS;
	Jump_CoP1_S[ 6] = COP1_S_MOV;
	Jump_CoP1_S[ 7] = COP1_S_NEG;
	Jump_CoP1_S[ 8] = UnknownOpcode;
	Jump_CoP1_S[ 9] = COP1_S_TRUNC_L;
	Jump_CoP1_S[10] = COP1_S_CEIL_L;		//added by Witten
	Jump_CoP1_S[11] = COP1_S_FLOOR_L;		//added by Witten
	Jump_CoP1_S[12] = COP1_S_ROUND_W;
	Jump_CoP1_S[13] = COP1_S_TRUNC_W;
	Jump_CoP1_S[14] = COP1_S_CEIL_W;		//added by Witten
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

	Jump_CoP1_D[ 0] = COP1_D_ADD;
	Jump_CoP1_D[ 1] = COP1_D_SUB;
	Jump_CoP1_D[ 2] = COP1_D_MUL;
	Jump_CoP1_D[ 3] = COP1_D_DIV;
	Jump_CoP1_D[ 4] = COP1_D_SQRT;
	Jump_CoP1_D[ 5] = COP1_D_ABS;
	Jump_CoP1_D[ 6] = COP1_D_MOV;
	Jump_CoP1_D[ 7] = COP1_D_NEG;
	Jump_CoP1_D[ 8] = UnknownOpcode;
	Jump_CoP1_D[ 9] = COP1_D_TRUNC_L;		//added by Witten
	Jump_CoP1_D[10] = COP1_D_CEIL_L;		//added by Witten
	Jump_CoP1_D[11] = COP1_D_FLOOR_L;		//added by Witten
	Jump_CoP1_D[12] = COP1_D_ROUND_W;
	Jump_CoP1_D[13] = COP1_D_TRUNC_W;
	Jump_CoP1_D[14] = COP1_D_CEIL_W;		//added by Witten
	Jump_CoP1_D[15] = COP1_D_FLOOR_W;		//added by Witten
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

	Jump_CoP1_W[ 0] = UnknownOpcode;
	Jump_CoP1_W[ 1] = UnknownOpcode;
	Jump_CoP1_W[ 2] = UnknownOpcode;
	Jump_CoP1_W[ 3] = UnknownOpcode;
	Jump_CoP1_W[ 4] = UnknownOpcode;
	Jump_CoP1_W[ 5] = UnknownOpcode;
	Jump_CoP1_W[ 6] = UnknownOpcode;
	Jump_CoP1_W[ 7] = UnknownOpcode;
	Jump_CoP1_W[ 8] = UnknownOpcode;
	Jump_CoP1_W[ 9] = UnknownOpcode;
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

	Jump_CoP1_L[ 0] = UnknownOpcode;
	Jump_CoP1_L[ 1] = UnknownOpcode;
	Jump_CoP1_L[ 2] = UnknownOpcode;
	Jump_CoP1_L[ 3] = UnknownOpcode;
	Jump_CoP1_L[ 4] = UnknownOpcode;
	Jump_CoP1_L[ 5] = UnknownOpcode;
	Jump_CoP1_L[ 6] = UnknownOpcode;
	Jump_CoP1_L[ 7] = UnknownOpcode;
	Jump_CoP1_L[ 8] = UnknownOpcode;
	Jump_CoP1_L[ 9] = UnknownOpcode;
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

int DelaySlotEffectsCompare (DWORD PC, DWORD Reg1, DWORD Reg2);

void TestInterpreterJump (DWORD PC, DWORD TargetPC, int Reg1, int Reg2) 
{
	if (PC != TargetPC) { return; }
	if (DelaySlotEffectsCompare(PC,Reg1,Reg2)) { return; }
	R4300iOp::m_NextInstruction = PERMLOOP_DO_DELAY;
	R4300iOp::m_TestTimer = TRUE;
}

/************************* Opcode functions *************************/
void R4300iOp::J (void) {
	m_NextInstruction = DELAY_SLOT;
	m_JumpToLocation = ((*_PROGRAM_COUNTER) & 0xF0000000) + (m_Opcode.target << 2);
	if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
	{
		m_NextInstruction = PERMLOOP_DO_DELAY;
	}
}

void R4300iOp::JAL (void) {
	m_NextInstruction = DELAY_SLOT;
	m_JumpToLocation = ((*_PROGRAM_COUNTER) & 0xF0000000) + (m_Opcode.target << 2);
	_GPR[31].DW= (long)((*_PROGRAM_COUNTER) + 8);

	if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
	{
		m_NextInstruction = PERMLOOP_DO_DELAY;
	}
}

void R4300iOp::BEQ (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,m_Opcode.rt))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::BNE (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,m_Opcode.rt))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::BLEZ (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW <= 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,0))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::BGTZ (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW > 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,0))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::ADDI (void) {
#ifdef Interpreter_StackTest
	if (m_Opcode.rs == 29 && m_Opcode.rt == 29) {
		StackValue += (short)m_Opcode.immediate;
	}
#endif
	if (m_Opcode.rt == 0) { return; }
	_GPR[m_Opcode.rt].DW = (_GPR[m_Opcode.rs].W[0] + ((short)m_Opcode.immediate));
#ifdef Interpreter_StackTest
	if (m_Opcode.rt == 29 && m_Opcode.rs != 29) {
		StackValue = _GPR[m_Opcode.rt].W[0];		
	}
#endif
}

void R4300iOp::ADDIU (void) {
#ifdef Interpreter_StackTest
	if (m_Opcode.rs == 29 && m_Opcode.rt == 29) {
		StackValue += (short)m_Opcode.immediate;
	}
#endif
	_GPR[m_Opcode.rt].DW = (_GPR[m_Opcode.rs].W[0] + ((short)m_Opcode.immediate));
#ifdef Interpreter_StackTest
	if (m_Opcode.rt == 29 && m_Opcode.rs != 29) {
		StackValue = _GPR[m_Opcode.rt].W[0];		
	}
#endif
}

void R4300iOp::SLTI (void) {
	if (_GPR[m_Opcode.rs].DW < (_int64)((short)m_Opcode.immediate)) {
		_GPR[m_Opcode.rt].DW = 1;
	} else {
		_GPR[m_Opcode.rt].DW = 0;
	}
}

void R4300iOp::SLTIU (void) {
	int imm32 = (short)m_Opcode.immediate;
	__int64 imm64;

	imm64 = imm32;
	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].UDW < (unsigned __int64)imm64?1:0;
}

void R4300iOp::ANDI (void) {
	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW & m_Opcode.immediate;
}

void R4300iOp::ORI (void) {
	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW | m_Opcode.immediate;
}

void R4300iOp::XORI (void) {
	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW ^ m_Opcode.immediate;
}

void R4300iOp::LUI (void) {
	if (m_Opcode.rt == 0) { return; }
	_GPR[m_Opcode.rt].DW = (long)((short)m_Opcode.offset << 16);
#ifdef Interpreter_StackTest
	if (m_Opcode.rt == 29) {
		StackValue = _GPR[m_Opcode.rt].W[0];
	}
#endif
}

void R4300iOp::BEQL (void) {
	if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,m_Opcode.rt))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::BNEL (void) {
	if (_GPR[m_Opcode.rs].DW != _GPR[m_Opcode.rt].DW) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,m_Opcode.rt))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::BLEZL (void) {
	if (_GPR[m_Opcode.rs].DW <= 0) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,0))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::BGTZL (void) {
	if (_GPR[m_Opcode.rs].DW > 0) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{
			if (!DelaySlotEffectsCompare(*_PROGRAM_COUNTER,m_Opcode.rs,0))
			{
				m_NextInstruction = PERMLOOP_DO_DELAY;
			}
		}
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::DADDIU (void) {
	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rs].DW + (_int64)((short)m_Opcode.immediate);
}

QWORD LDL_MASK[8] = { 0,0xFF,0xFFFF,0xFFFFFF,0xFFFFFFFF,0xFFFFFFFFFF,
					 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF };
int LDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void R4300iOp::LDL (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 7;

	if (!_MMU->LD_VAddr((Address & ~7),Value)) 
	{
		if (g_ShowTLBMisses) 
		{
			DisplayError("LDL TLB: %X",Address);
		}
		return;
	}
	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].DW & LDL_MASK[Offset];
	_GPR[m_Opcode.rt].DW += Value << LDL_SHIFT[Offset];
}

QWORD LDR_MASK[8] = { 0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000,
                      0xFFFFFFFFFF000000, 0xFFFFFFFF00000000,
                      0xFFFFFF0000000000, 0xFFFF000000000000,
                      0xFF00000000000000, 0 };
int LDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

void R4300iOp::LDR (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 7;

	if (!_MMU->LD_VAddr((Address & ~7),Value)) 
	{
		if (g_ShowTLBMisses) 
		{
			DisplayError("LDR TLB: %X",Address);
		}
		return;
	}

	_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].DW & LDR_MASK[Offset];
	_GPR[m_Opcode.rt].DW += Value >> LDR_SHIFT[Offset];

}

void R4300iOp::LB (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if (m_Opcode.rt == 0) { return; }
	if (!_MMU->LB_VAddr(Address,_GPR[m_Opcode.rt].UB[0])) {
		if (g_ShowTLBMisses) {
#ifndef EXTERNAL_RELEASE
			DisplayError("LB TLB: %X",Address);
#endif
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].B[0];
	}
}

void R4300iOp::LH (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!_MMU->LH_VAddr(Address,_GPR[m_Opcode.rt].UHW[0])) {
		if (g_ShowTLBMisses) {
			DisplayError("LH TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].HW[0];
	}
}

void R4300iOp::LWL (void) {
	DWORD Offset, Address, Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 3;

	if (!_MMU->LW_VAddr((Address & ~3),Value)) 
	{
		if (g_ShowTLBMisses) 
		{
			DisplayError("LWL TLB: %X",Address);
		}
		return;
	}
	
	_GPR[m_Opcode.rt].DW = (int)(_GPR[m_Opcode.rt].W[0] & LWL_MASK[Offset]);
	_GPR[m_Opcode.rt].DW += (int)(Value << LWL_SHIFT[Offset]);
}

void R4300iOp::LW (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }

#ifndef EXTERNAL_RELEASE
	if (LogOptions.GenerateLog)
	{ 
		Log_LW((*_PROGRAM_COUNTER),Address);
	}
#endif

	if (m_Opcode.rt == 0) { return; }

	if (!_MMU->LW_VAddr(Address,_GPR[m_Opcode.rt].UW[0])) {
		if (g_ShowTLBMisses) {
			DisplayError("LW TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].W[0];
	}
}

void R4300iOp::LBU (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if (!_MMU->LB_VAddr(Address,_GPR[m_Opcode.rt].UB[0])) {
		if (g_ShowTLBMisses) {
			DisplayError("LBU TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].UDW = _GPR[m_Opcode.rt].UB[0];
	}
}

void R4300iOp::LHU (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!_MMU->LH_VAddr(Address,_GPR[m_Opcode.rt].UHW[0])) {
		if (g_ShowTLBMisses) {
			DisplayError("LHU TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].UDW = _GPR[m_Opcode.rt].UHW[0];
	}
}

void R4300iOp::LWR (void) {
	DWORD Offset, Address, Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 3;

	if (!_MMU->LW_VAddr((Address & ~3),Value)) 
	{
		if (g_ShowTLBMisses) 
		{
			DisplayError("LWR TLB: %X",Address);
		}
		return;
	}
	
	_GPR[m_Opcode.rt].DW = (int)(_GPR[m_Opcode.rt].W[0] & LWR_MASK[Offset]);
	_GPR[m_Opcode.rt].DW += (int)(Value >> LWR_SHIFT[Offset]);
}

void R4300iOp::LWU (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (m_Opcode.rt == 0) { return; }

	if (!_MMU->LW_VAddr(Address,_GPR[m_Opcode.rt].UW[0])) {
		if (g_ShowTLBMisses) {
			DisplayError("LWU TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].UDW = _GPR[m_Opcode.rt].UW[0];
	}
}

void R4300iOp::SB (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if (!_MMU->SB_VAddr(Address,_GPR[m_Opcode.rt].UB[0])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SB TLB: %X",Address);
#endif
	}
}

void R4300iOp::SH (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
	if (!_MMU->SH_VAddr(Address,_GPR[m_Opcode.rt].UHW[0])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SH TLB: %X",Address);
#endif
	}
}

void R4300iOp::SWL (void) {
	DWORD Offset, Address, Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 3;

	if (!_MMU->LW_VAddr((Address & ~3),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SWL_MASK[Offset];
	Value += _GPR[m_Opcode.rt].UW[0] >> SWL_SHIFT[Offset];

	if (!_MMU->SW_VAddr((Address & ~0x03),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
	}
}


void R4300iOp::SW (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.GenerateLog) 
	{ 
		Log_SW((*_PROGRAM_COUNTER),Address,_GPR[m_Opcode.rt].UW[0]);
	}
#endif
	if (!_MMU->SW_VAddr(Address,_GPR[m_Opcode.rt].UW[0])) 
	{
		if (g_ShowTLBMisses) 
		{
			DisplayError("SW TLB: %X",Address);
		}
	}
}

QWORD SDL_MASK[8] = { 0,0xFF00000000000000,
						0xFFFF000000000000,
						0xFFFFFF0000000000,
						0xFFFFFFFF00000000,
					    0xFFFFFFFFFF000000,
						0xFFFFFFFFFFFF0000,
						0xFFFFFFFFFFFFFF00 
					};
int SDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void R4300iOp::SDL (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 7;

	if (!_MMU->LD_VAddr((Address & ~7),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SDL_MASK[Offset];
	Value += _GPR[m_Opcode.rt].UDW >> SDL_SHIFT[Offset];

	if (!_MMU->SD_VAddr((Address & ~7),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
	}
}

QWORD SDR_MASK[8] = { 0x00FFFFFFFFFFFFFF,
					  0x0000FFFFFFFFFFFF,
					  0x000000FFFFFFFFFF,
					  0x00000000FFFFFFFF,
					  0x0000000000FFFFFF,
					  0x000000000000FFFF,
					  0x00000000000000FF, 
					  0x0000000000000000 
					};
int SDR_SHIFT[8] = { 56,48,40,32,24,16,8,0 };

void R4300iOp::SDR (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 7;

	if (!_MMU->LD_VAddr((Address & ~7),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SDR_MASK[Offset];
	Value += _GPR[m_Opcode.rt].UDW << SDR_SHIFT[Offset];

	if (!_MMU->SD_VAddr((Address & ~7),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
	}
}

void R4300iOp::SWR (void) {
	DWORD Offset, Address, Value;
	
	Address = _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	Offset  = Address & 3;

	if (!_MMU->LW_VAddr((Address & ~3),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SWR_MASK[Offset];
	Value += _GPR[m_Opcode.rt].UW[0] << SWR_SHIFT[Offset];

	if (!_MMU->SW_VAddr((Address & ~0x03),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
	}
}

void R4300iOp::CACHE (void) {
#if (!defined(EXTERNAL_RELEASE))
	if (!LogOptions.LogCache) { return; }
	LogMessage("%08X: Cache operation %d, 0x%08X", (*_PROGRAM_COUNTER), m_Opcode.rt,
		_GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset );
#endif
}

void R4300iOp::LL (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }

	if (m_Opcode.rt == 0) { return; }

	if (!_MMU->LW_VAddr(Address,_GPR[m_Opcode.rt].UW[0])) {
		if (g_ShowTLBMisses) {
			DisplayError("LL TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		_GPR[m_Opcode.rt].DW = _GPR[m_Opcode.rt].W[0];
		(*_LLBit) = 1;
	}
}

void R4300iOp::LWC1 (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (DWORD)((short)m_Opcode.offset);
	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!_MMU->LW_VAddr(Address,*(DWORD *)_FPR_S[m_Opcode.ft])) {
		if (g_ShowTLBMisses) {
			DisplayError("LWC1 TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	}
}

void R4300iOp::SC (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
#if (!defined(EXTERNAL_RELEASE))
	Log_SW((*_PROGRAM_COUNTER),Address,_GPR[m_Opcode.rt].UW[0]);
#endif
	if ((*_LLBit) == 1) {
		if (!_MMU->SW_VAddr(Address,_GPR[m_Opcode.rt].UW[0])) 
		{
			if (g_ShowTLBMisses) 
			{
				DisplayError("SC TLB: %X",Address);
			}
		}
	}
	_GPR[m_Opcode.rt].UW[0] = (*_LLBit);
}

void R4300iOp::LD (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!_MMU->LD_VAddr(Address,_GPR[m_Opcode.rt].UDW)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LD TLB: %X",Address);
#endif
	}
#ifdef Interpreter_StackTest
	if (m_Opcode.rt == 29) {
		StackValue = _GPR[m_Opcode.rt].W[0];
	}
#endif
}


void R4300iOp::LDC1 (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;

	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!_MMU->LD_VAddr(Address,*(unsigned __int64 *)_FPR_D[m_Opcode.ft])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LD TLB: %X",Address);
#endif
	}
}

void R4300iOp::SWC1 (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;
	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }

	if (!_MMU->SW_VAddr(Address,*(DWORD *)_FPR_S[m_Opcode.ft])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWC1 TLB: %X",Address);
#endif
	}
}

void R4300iOp::SDC1 (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;

	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
	if (!_MMU->SD_VAddr(Address,*(__int64 *)_FPR_D[m_Opcode.ft])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDC1 TLB: %X",Address);
#endif
	}
}

void R4300iOp::SD (void) {
	DWORD Address =  _GPR[m_Opcode.base].UW[0] + (short)m_Opcode.offset;	
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
	if (!_MMU->SD_VAddr(Address,_GPR[m_Opcode.rt].UDW)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SD TLB: %X",Address);
#endif
	}
}
/********************** R4300i OpCodes: Special **********************/
void R4300iOp::SPECIAL_SLL (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] << m_Opcode.sa);
}

void R4300iOp::SPECIAL_SRL (void) {
	_GPR[m_Opcode.rd].DW = (int)(_GPR[m_Opcode.rt].UW[0] >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_SRA (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_SLLV (void) {
	if (m_Opcode.rd == 0) { return; }
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] << (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_SRLV (void) {
	_GPR[m_Opcode.rd].DW = (int)(_GPR[m_Opcode.rt].UW[0] >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_SRAV (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].W[0] >> (_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_JR (void) {
	m_NextInstruction = DELAY_SLOT;
	m_JumpToLocation = _GPR[m_Opcode.rs].UW[0];
	m_TestTimer = TRUE;
}

void R4300iOp::SPECIAL_JALR (void) {
	m_NextInstruction = DELAY_SLOT;
	m_JumpToLocation = _GPR[m_Opcode.rs].UW[0];
	_GPR[m_Opcode.rd].DW = (long)((*_PROGRAM_COUNTER) + 8);
	m_TestTimer = TRUE;
}

void R4300iOp::SPECIAL_SYSCALL (void) {
	_Reg->DoSysCallException(m_NextInstruction == JUMP);
	m_NextInstruction = JUMP;
	m_JumpToLocation = (*_PROGRAM_COUNTER);
}

void R4300iOp::SPECIAL_BREAK (void) {
	_Reg->DoBreakException(m_NextInstruction == JUMP);
	m_NextInstruction = JUMP;
	m_JumpToLocation = (*_PROGRAM_COUNTER);
}

void R4300iOp::SPECIAL_SYNC (void) {
}

void R4300iOp::SPECIAL_MFHI (void) {
	_GPR[m_Opcode.rd].DW = _RegHI->DW;
}

void R4300iOp::SPECIAL_MTHI (void) {
	_RegHI->DW = _GPR[m_Opcode.rs].DW;
}

void R4300iOp::SPECIAL_MFLO (void) {
	_GPR[m_Opcode.rd].DW = _RegLO->DW;
}

void R4300iOp::SPECIAL_MTLO (void) {
	_RegLO->DW = _GPR[m_Opcode.rs].DW;
}

void R4300iOp::SPECIAL_DSLLV (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rt].DW << (_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_DSRLV (void) {
	_GPR[m_Opcode.rd].UDW = _GPR[m_Opcode.rt].UDW >> (_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_DSRAV (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rt].DW >> (_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_MULT (void) {
	_RegHI->DW = (_int64)(_GPR[m_Opcode.rs].W[0]) * (_int64)(_GPR[m_Opcode.rt].W[0]);
	_RegLO->DW = _RegHI->W[0];
	_RegHI->DW = _RegHI->W[1];
}

void R4300iOp::SPECIAL_MULTU (void) {
	_RegHI->DW = (unsigned _int64)(_GPR[m_Opcode.rs].UW[0]) * (unsigned _int64)(_GPR[m_Opcode.rt].UW[0]);
	_RegLO->DW = _RegHI->W[0];
	_RegHI->DW = _RegHI->W[1];
}

void R4300iOp::SPECIAL_DIV (void) {
	if ( _GPR[m_Opcode.rt].UDW != 0 ) {
		_RegLO->DW = _GPR[m_Opcode.rs].W[0] / _GPR[m_Opcode.rt].W[0];
		_RegHI->DW = _GPR[m_Opcode.rs].W[0] % _GPR[m_Opcode.rt].W[0];
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DIV by 0 ???");
#endif
	}
}

void R4300iOp::SPECIAL_DIVU (void) {
	if ( _GPR[m_Opcode.rt].UDW != 0 ) {
		_RegLO->DW = (int)(_GPR[m_Opcode.rs].UW[0] / _GPR[m_Opcode.rt].UW[0]);
		_RegHI->DW = (int)(_GPR[m_Opcode.rs].UW[0] % _GPR[m_Opcode.rt].UW[0]);
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DIVU by 0 ???");
#endif
	}
}

void R4300iOp::SPECIAL_DMULT (void) {
	MIPS_DWORD Tmp[3];
	
	_RegLO->UDW = (QWORD)_GPR[m_Opcode.rs].UW[0] * (QWORD)_GPR[m_Opcode.rt].UW[0];
	Tmp[0].UDW = (_int64)_GPR[m_Opcode.rs].W[1] * (_int64)(QWORD)_GPR[m_Opcode.rt].UW[0];
	Tmp[1].UDW = (_int64)(QWORD)_GPR[m_Opcode.rs].UW[0] * (_int64)_GPR[m_Opcode.rt].W[1];
	_RegHI->UDW = (_int64)_GPR[m_Opcode.rs].W[1] * (_int64)_GPR[m_Opcode.rt].W[1];
	
	Tmp[2].UDW = (QWORD)_RegLO->UW[1] + (QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0];
	_RegLO->UDW += ((QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0]) << 32;
	_RegHI->UDW += (QWORD)Tmp[0].W[1] + (QWORD)Tmp[1].W[1] + Tmp[2].UW[1];
}

void R4300iOp::SPECIAL_DMULTU (void) 
{
	MIPS_DWORD Tmp[3];
	
	_RegLO->UDW = (QWORD)_GPR[m_Opcode.rs].UW[0] * (QWORD)_GPR[m_Opcode.rt].UW[0];
	Tmp[0].UDW = (QWORD)_GPR[m_Opcode.rs].UW[1] * (QWORD)_GPR[m_Opcode.rt].UW[0];
	Tmp[1].UDW = (QWORD)_GPR[m_Opcode.rs].UW[0] * (QWORD)_GPR[m_Opcode.rt].UW[1];
	_RegHI->UDW = (QWORD)_GPR[m_Opcode.rs].UW[1] * (QWORD)_GPR[m_Opcode.rt].UW[1];
	
	Tmp[2].UDW = (QWORD)_RegLO->UW[1] + (QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0];
	_RegLO->UDW += ((QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0]) << 32;
	_RegHI->UDW += (QWORD)Tmp[0].UW[1] + (QWORD)Tmp[1].UW[1] + Tmp[2].UW[1];
}

void R4300iOp::SPECIAL_DDIV (void) {
	if ( _GPR[m_Opcode.rt].UDW != 0 ) {
		_RegLO->DW = _GPR[m_Opcode.rs].DW / _GPR[m_Opcode.rt].DW;
		_RegHI->DW = _GPR[m_Opcode.rs].DW % _GPR[m_Opcode.rt].DW;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DDIV by 0 ???");
#endif
	}
}

void R4300iOp::SPECIAL_DDIVU (void) {
	if ( _GPR[m_Opcode.rt].UDW != 0 ) {
		_RegLO->UDW = _GPR[m_Opcode.rs].UDW / _GPR[m_Opcode.rt].UDW;
		_RegHI->UDW = _GPR[m_Opcode.rs].UDW % _GPR[m_Opcode.rt].UDW;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DDIVU by 0 ???");
#endif
	}
}

void R4300iOp::SPECIAL_ADD (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_ADDU (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] + _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_SUB (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] - _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_SUBU (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].W[0] - _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_AND (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW & _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_OR (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW | _GPR[m_Opcode.rt].DW;
#ifdef Interpreter_StackTest
	if (m_Opcode.rd == 29) {
		StackValue = _GPR[m_Opcode.rd].W[0];
	}
#endif
}

void R4300iOp::SPECIAL_XOR (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW ^ _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_NOR (void) {
	_GPR[m_Opcode.rd].DW = ~(_GPR[m_Opcode.rs].DW | _GPR[m_Opcode.rt].DW);
}

void R4300iOp::SPECIAL_SLT (void) {
	if (_GPR[m_Opcode.rs].DW < _GPR[m_Opcode.rt].DW) {
		_GPR[m_Opcode.rd].DW = 1;
	} else {
		_GPR[m_Opcode.rd].DW = 0;
	}
}

void R4300iOp::SPECIAL_SLTU (void) {
	if (_GPR[m_Opcode.rs].UDW < _GPR[m_Opcode.rt].UDW) {
		_GPR[m_Opcode.rd].DW = 1;
	} else {
		_GPR[m_Opcode.rd].DW = 0;
	}
}

void R4300iOp::SPECIAL_DADD (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW + _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DADDU (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW + _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DSUB (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW - _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DSUBU (void) {
	_GPR[m_Opcode.rd].DW = _GPR[m_Opcode.rs].DW - _GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_TEQ (void) {
	if (_GPR[m_Opcode.rs].DW == _GPR[m_Opcode.rt].DW) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Should trap this ???");
#endif
	}
}

void R4300iOp::SPECIAL_DSLL (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW << m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSRL (void) {
	_GPR[m_Opcode.rd].UDW = (_GPR[m_Opcode.rt].UDW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSRA (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSLL32 (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW << (m_Opcode.sa + 32));
}

void R4300iOp::SPECIAL_DSRL32 (void) {
   _GPR[m_Opcode.rd].UDW = (_GPR[m_Opcode.rt].UDW >> (m_Opcode.sa + 32));
}

void R4300iOp::SPECIAL_DSRA32 (void) {
	_GPR[m_Opcode.rd].DW = (_GPR[m_Opcode.rt].DW >> (m_Opcode.sa + 32));
}

/********************** R4300i OpCodes: RegImm **********************/
void R4300iOp::REGIMM_BLTZ (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW < 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{			
			if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER),m_Opcode.rs,0)) 
			{
				InPermLoop();
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::REGIMM_BGEZ (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW >= 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{			
			if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER),m_Opcode.rs,0)) 
			{
				InPermLoop();
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::REGIMM_BLTZL (void) {
	if (_GPR[m_Opcode.rs].DW < 0) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{			
			if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER),m_Opcode.rs,0)) 
			{
				InPermLoop();
			}
		}
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::REGIMM_BGEZL (void) {
	if (_GPR[m_Opcode.rs].DW >= 0) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{			
			if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER),m_Opcode.rs,0)) 
			{
				InPermLoop();
			}
		}
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::REGIMM_BLTZAL (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW < 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{			
			if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER),m_Opcode.rs,0)) 
			{
				InPermLoop();
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
	_GPR[31].DW= (long)((*_PROGRAM_COUNTER) + 8);
}

void R4300iOp::REGIMM_BGEZAL (void) {
	m_NextInstruction = DELAY_SLOT;
	if (_GPR[m_Opcode.rs].DW >= 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
		if ((*_PROGRAM_COUNTER) == m_JumpToLocation)
		{			
			if (!DelaySlotEffectsCompare((*_PROGRAM_COUNTER),m_Opcode.rs,0)) 
			{
				InPermLoop();
			}
		}
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
	_GPR[31].DW = (long)((*_PROGRAM_COUNTER) + 8);
}
/************************** COP0 functions **************************/
void R4300iOp::COP0_MF (void) {
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.LogCP0reads) {
		LogMessage("%08X: R4300i Read from %s (0x%08X)", (*_PROGRAM_COUNTER),
			CRegName::Cop0[m_Opcode.rd], _CP0[m_Opcode.rd]);
	}
#endif
	if (m_Opcode.rd == 9)
	{
		_SystemTimer->UpdateTimers();
	}
	_GPR[m_Opcode.rt].DW = (int)_CP0[m_Opcode.rd];
}

void R4300iOp::COP0_MT (void) {
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.LogCP0changes) {
		LogMessage("%08X: Writing 0x%X to %s register (Originally: 0x%08X)",(*_PROGRAM_COUNTER),
			_GPR[m_Opcode.rt].UW[0],CRegName::Cop0[m_Opcode.rd], _CP0[m_Opcode.rd]);
		if (m_Opcode.rd == 11) { //Compare
			LogMessage("%08X: Cause register changed from %08X to %08X",(*_PROGRAM_COUNTER),
				_Reg->CAUSE_REGISTER, (_Reg->CAUSE_REGISTER & ~CAUSE_IP7));
		}
	}
#endif
	switch (m_Opcode.rd) {	
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
		_SystemTimer->UpdateTimers();
		_CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
		break;
	case 4: //Context
		_CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0] & 0xFF800000;
		break;
	case 9: //Count
		_SystemTimer->UpdateTimers();
		_CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
		_SystemTimer->UpdateCompareTimer();
		break;		
	case 11: //Compare
		_SystemTimer->UpdateTimers();
		_CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
		_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP7;
		_SystemTimer->UpdateCompareTimer();
		break;		
	case 12: //Status
		if ((_CP0[m_Opcode.rd] ^ _GPR[m_Opcode.rt].UW[0]) != 0) {
			_CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
			SetFpuLocations();
		} else {
			_CP0[m_Opcode.rd] = _GPR[m_Opcode.rt].UW[0];
		}
		if ((_CP0[m_Opcode.rd] & 0x18) != 0) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("Left kernel mode ??");
#endif
		}
		_Reg->CheckInterrupts();
		break;		
	case 13: //cause
		_CP0[m_Opcode.rd] &= 0xFFFFCFF;
#ifndef EXTERNAL_RELEASE
		if ((_GPR[m_Opcode.rt].UW[0] & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		break;
	default:
		UnknownOpcode();
	}
}

/************************** COP0 CO functions ***********************/
void R4300iOp::COP0_CO_TLBR (void) {
	if (!bUseTlb()) { return; }
	_TLB->ReadEntry();
}

void R4300iOp::COP0_CO_TLBWI (void) {
	if (!bUseTlb()) { return; }
	_TLB->WriteEntry(_Reg->INDEX_REGISTER & 0x1F,FALSE);
}

void R4300iOp::COP0_CO_TLBWR (void) {
	if (!bUseTlb()) { return; }
	_TLB->WriteEntry(_Reg->RANDOM_REGISTER & 0x1F,true);
}

void R4300iOp::COP0_CO_TLBP (void) {
	if (!bUseTlb()) { return; }
	_TLB->Probe();
}

void R4300iOp::COP0_CO_ERET (void) {
	m_NextInstruction = JUMP;
	if ((_Reg->STATUS_REGISTER & STATUS_ERL) != 0) {
		m_JumpToLocation = _Reg->ERROREPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		m_JumpToLocation = _Reg->EPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_EXL;
	}
	(*_LLBit) = 0;
	_Reg->CheckInterrupts();
	m_TestTimer = TRUE;
}

/************************** COP1 functions **************************/
void R4300iOp::COP1_MF (void) {
	TEST_COP1_USABLE_EXCEPTION	
	_GPR[m_Opcode.rt].DW = *(int *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_DMF (void) {
	TEST_COP1_USABLE_EXCEPTION
	_GPR[m_Opcode.rt].DW = *(__int64 *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_CF (void) {
	TEST_COP1_USABLE_EXCEPTION
	if (m_Opcode.fs != 31 && m_Opcode.fs != 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("CFC1 what register are you writing to ?");
#endif
		return;
	}
	_GPR[m_Opcode.rt].DW = (int)_FPCR[m_Opcode.fs];
}

void R4300iOp::COP1_MT (void) {
	TEST_COP1_USABLE_EXCEPTION
	*(int *)_FPR_S[m_Opcode.fs] = _GPR[m_Opcode.rt].W[0];
}

void R4300iOp::COP1_DMT (void) {
	TEST_COP1_USABLE_EXCEPTION
	*(__int64 *)_FPR_D[m_Opcode.fs] = _GPR[m_Opcode.rt].DW;
}

void R4300iOp::COP1_CT (void) {
	TEST_COP1_USABLE_EXCEPTION
	if (m_Opcode.fs == 31) {
		_FPCR[m_Opcode.fs] = _GPR[m_Opcode.rt].W[0];
		switch((_FPCR[m_Opcode.fs] & 3)) {
		case 0: *_RoundingModel = ROUND_NEAR; break;
		case 1: *_RoundingModel = ROUND_CHOP; break;
		case 2: *_RoundingModel = ROUND_UP; break;
		case 3: *_RoundingModel = ROUND_DOWN; break;
		}
		return;
	}
#ifndef EXTERNAL_RELEASE
	DisplayError("CTC1 what register are you writing to ?");
#endif
}

/************************* COP1: BC1 functions ***********************/
void R4300iOp::COP1_BCF (void) {
	TEST_COP1_USABLE_EXCEPTION
	m_NextInstruction = DELAY_SLOT;
	if ((_FPCR[31] & FPCSR_C) == 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::COP1_BCT (void) {
	TEST_COP1_USABLE_EXCEPTION
	m_NextInstruction = DELAY_SLOT;
	if ((_FPCR[31] & FPCSR_C) != 0) {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
	} else {
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::COP1_BCFL (void) {
	TEST_COP1_USABLE_EXCEPTION
	if ((_FPCR[31] & FPCSR_C) == 0) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}

void R4300iOp::COP1_BCTL (void) {
	TEST_COP1_USABLE_EXCEPTION
	if ((_FPCR[31] & FPCSR_C) != 0) {
		m_NextInstruction = DELAY_SLOT;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + ((short)m_Opcode.offset << 2) + 4;
	} else {
		m_NextInstruction = JUMP;
		m_JumpToLocation = (*_PROGRAM_COUNTER) + 8;
	}
}
/************************** COP1: S functions ************************/
__inline void Float_RoundToInteger32( int * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void Float_RoundToInteger64( __int64 * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp qword ptr [edi]
	}
}

void R4300iOp::COP1_S_ADD (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] + *(float *)_FPR_S[m_Opcode.ft]); 
}

void R4300iOp::COP1_S_SUB (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] - *(float *)_FPR_S[m_Opcode.ft]); 
}

void R4300iOp::COP1_S_MUL (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] * *(float *)_FPR_S[m_Opcode.ft]); 
}

void R4300iOp::COP1_S_DIV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] / *(float *)_FPR_S[m_Opcode.ft]); 
}

void R4300iOp::COP1_S_SQRT (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (float)sqrt(*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_ABS (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (float)fabs(*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_MOV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = *(float *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_S_NEG (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (*(float *)_FPR_S[m_Opcode.fs] * -1.0f);
}

void R4300iOp::COP1_S_TRUNC_L (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)_FPR_D[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CEIL_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_UP,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)_FPR_D[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_FLOOR_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)_FPR_D[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_ROUND_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_NEAR,_MCW_RC);
	Float_RoundToInteger32(&*(int *)_FPR_S[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_TRUNC_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger32(&*(int *)_FPR_S[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CEIL_W (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_UP,_MCW_RC);
	Float_RoundToInteger32(&*(int *)_FPR_S[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_FLOOR_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Float_RoundToInteger32(&*(int *)_FPR_S[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CVT_D (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = (double)(*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CVT_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	Float_RoundToInteger32(&*(int *)_FPR_S[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CVT_L (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)_FPR_D[m_Opcode.fd],&*(float *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_S_CMP (void) {
	int less, equal, unorded, condition;
	float Temp0, Temp1;

	TEST_COP1_USABLE_EXCEPTION

	Temp0 = *(float *)_FPR_S[m_Opcode.fs];
	Temp1 = *(float *)_FPR_S[m_Opcode.ft];

	if (_isnan(Temp0) || _isnan(Temp1)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Nan ?");
#endif
		less = FALSE;
		equal = FALSE;
		unorded = TRUE;
		if ((m_Opcode.funct & 8) != 0) {
#ifndef EXTERNAL_RELEASE
			DisplayError("Signal InvalidOperationException\nin r4300i_COP1_S_CMP\n%X  %ff\n%X  %ff",
				Temp0,Temp0,Temp1,Temp1);
#endif
		}
	} else {
		less = Temp0 < Temp1;
		equal = Temp0 == Temp1;
		unorded = FALSE;
	}
	
	condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) | 
		((m_Opcode.funct & 1) && unorded);

	if (condition) {
		_FPCR[31] |= FPCSR_C;
	} else {
		_FPCR[31] &= ~FPCSR_C;
	}
	
}

/************************** COP1: D functions ************************/
__inline void Double_RoundToInteger32( DWORD * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void Double_RoundToInteger64( unsigned __int64 * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp qword ptr [edi]
	}
}

void R4300iOp::COP1_D_ADD (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] + *(double *)_FPR_D[m_Opcode.ft]; 
}

void R4300iOp::COP1_D_SUB (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] - *(double *)_FPR_D[m_Opcode.ft]; 
}

void R4300iOp::COP1_D_MUL (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] * *(double *)_FPR_D[m_Opcode.ft]; 
}

void R4300iOp::COP1_D_DIV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = *(double *)_FPR_D[m_Opcode.fs] / *(double *)_FPR_D[m_Opcode.ft]; 
}

void R4300iOp::COP1_D_SQRT (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = (double)sqrt(*(double *)_FPR_D[m_Opcode.fs]); 
}

void R4300iOp::COP1_D_ABS (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = fabs(*(double *)_FPR_D[m_Opcode.fs]);
}

void R4300iOp::COP1_D_MOV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(__int64 *)_FPR_D[m_Opcode.fd] = *(__int64 *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_D_NEG (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = (*(double *)_FPR_D[m_Opcode.fs] * -1.0);
}

void R4300iOp::COP1_D_TRUNC_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_CHOP,_MCW_RC);
	Double_RoundToInteger64(&*(QWORD *)_FPR_S[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs] );
}

void R4300iOp::COP1_D_CEIL_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_UP,_MCW_RC);
	Double_RoundToInteger64(&*(QWORD *)_FPR_S[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs] );
}

void R4300iOp::COP1_D_FLOOR_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Double_RoundToInteger64(&*(QWORD *)_FPR_D[m_Opcode.fd],&*(double *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_D_ROUND_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_NEAR,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)_FPR_S[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs] );
}

void R4300iOp::COP1_D_TRUNC_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_CHOP,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)_FPR_S[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs] );
}

void R4300iOp::COP1_D_CEIL_W (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_UP,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)_FPR_S[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs] );
}

void R4300iOp::COP1_D_FLOOR_W (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)_FPR_D[m_Opcode.fd],&*(double *)_FPR_S[m_Opcode.fs]);
}

void R4300iOp::COP1_D_CVT_S (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (float)*(double *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_D_CVT_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)_FPR_S[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs] );
}

void R4300iOp::COP1_D_CVT_L (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	Double_RoundToInteger64(&*(unsigned __int64 *)_FPR_D[m_Opcode.fd],&*(double *)_FPR_D[m_Opcode.fs]);
}

void R4300iOp::COP1_D_CMP (void) {
	int less, equal, unorded, condition;
	MIPS_DWORD Temp0, Temp1;

	TEST_COP1_USABLE_EXCEPTION

	Temp0.DW = *(__int64 *)_FPR_D[m_Opcode.fs];
	Temp1.DW = *(__int64 *)_FPR_D[m_Opcode.ft];

	if (_isnan(Temp0.D) || _isnan(Temp1.D)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Nan ?");
#endif
		less = FALSE;
		equal = FALSE;
		unorded = TRUE;
		if ((m_Opcode.funct & 8) != 0) {
#ifndef EXTERNAL_RELEASE
			DisplayError("Signal InvalidOperationException\nin r4300i_COP1_D_CMP");
#endif
		}
	} else {
		less = Temp0.D < Temp1.D;
		equal = Temp0.D == Temp1.D;
		unorded = FALSE;
	}
	
	condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) | 
		((m_Opcode.funct & 1) && unorded);

	if (condition) {
		_FPCR[31] |= FPCSR_C;
	} else {
		_FPCR[31] &= ~FPCSR_C;
	}	
}

/************************** COP1: W functions ************************/
void R4300iOp::COP1_W_CVT_S (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (float)*(int *)_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_W_CVT_D (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = (double)*(int *)_FPR_S[m_Opcode.fs];
}

/************************** COP1: L functions ************************/
void R4300iOp::COP1_L_CVT_S (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(float *)_FPR_S[m_Opcode.fd] = (float)*(__int64 *)_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_L_CVT_D (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(*_RoundingModel,_MCW_RC);
	*(double *)_FPR_D[m_Opcode.fd] = (double)*(__int64 *)_FPR_D[m_Opcode.fs];
}

/************************** Other functions **************************/
void R4300iOp::UnknownOpcode (void) 
{
	DisplayError("%s: %08X\n%s\n\nStopping Emulation !", GS(MSG_UNHANDLED_OP), (*_PROGRAM_COUNTER),
		R4300iOpcodeName(m_Opcode.Hex,(*_PROGRAM_COUNTER)));
	_System->m_EndEmulation = true;
	
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (HaveDebugger && !inFullScreen) {
		int response;

		strcat(Message,"\n\nDo you wish to enter the debugger ?");
	
		response = MessageBox(NULL,Message,GS(MSG_MSGBOX_TITLE), MB_YESNO | MB_ICONERROR );
		if (response == IDYES) {
			Enter_R4300i_Commands_Window ();
		}
		ExitThread(0);
	} else {
		DisplayError(Message);
		ExitThread(0);
	}
#endif
}
