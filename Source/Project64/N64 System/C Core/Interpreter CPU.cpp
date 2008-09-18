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
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "main.h"
#include "cpu.h"
#include "debugger.h"
#include "plugin.h"

void * R4300i_Opcode[64];
void * R4300i_Special[64];
void * R4300i_Regimm[32];
void * R4300i_CoP0[32];
void * R4300i_CoP0_Function[64];
void * R4300i_CoP1[32];
void * R4300i_CoP1_BC[32];
void * R4300i_CoP1_S[64];
void * R4300i_CoP1_D[64];
void * R4300i_CoP1_W[64];
void * R4300i_CoP1_L[64];

BOOL TestTimer = FALSE;

void _fastcall R4300i_opcode_SPECIAL (void) {
	((void (_fastcall *)()) R4300i_Special[ Opcode.funct ])();
}

void _fastcall R4300i_opcode_REGIMM (void) {
	((void (_fastcall *)()) R4300i_Regimm[ Opcode.rt ])();
}

void _fastcall R4300i_opcode_COP0 (void) {
	((void (_fastcall *)()) R4300i_CoP0[ Opcode.rs ])();
}

void _fastcall R4300i_opcode_COP0_CO (void) {
	((void (_fastcall *)()) R4300i_CoP0_Function[ Opcode.funct ])();
}

void _fastcall R4300i_opcode_COP1 (void) {
	((void (_fastcall *)()) R4300i_CoP1[ Opcode.fmt ])();
}

void _fastcall R4300i_opcode_COP1_BC (void) {
	((void (_fastcall *)()) R4300i_CoP1_BC[ Opcode.ft ])();
}

void _fastcall R4300i_opcode_COP1_S (void) {
	_controlfp(RoundingModel,_MCW_RC);
	((void (_fastcall *)()) R4300i_CoP1_S[ Opcode.funct ])();
}

void _fastcall R4300i_opcode_COP1_D (void) {
	_controlfp(RoundingModel,_MCW_RC);
	((void (_fastcall *)()) R4300i_CoP1_D[ Opcode.funct ])();
}

void _fastcall R4300i_opcode_COP1_W (void) {
	((void (_fastcall *)()) R4300i_CoP1_W[ Opcode.funct ])();
}

void _fastcall R4300i_opcode_COP1_L (void) {
	((void (_fastcall *)()) R4300i_CoP1_L[ Opcode.funct ])();
}

void BuildInterpreter (void ) { 
	R4300i_Opcode[ 0] = R4300i_opcode_SPECIAL;
	R4300i_Opcode[ 1] = R4300i_opcode_REGIMM;
	R4300i_Opcode[ 2] = r4300i_J;
	R4300i_Opcode[ 3] = r4300i_JAL;
	R4300i_Opcode[ 4] = r4300i_BEQ;
	R4300i_Opcode[ 5] = r4300i_BNE;
	R4300i_Opcode[ 6] = r4300i_BLEZ;
	R4300i_Opcode[ 7] = r4300i_BGTZ;
	R4300i_Opcode[ 8] = r4300i_ADDI;
	R4300i_Opcode[ 9] = r4300i_ADDIU;
	R4300i_Opcode[10] = r4300i_SLTI;
	R4300i_Opcode[11] = r4300i_SLTIU;
	R4300i_Opcode[12] = r4300i_ANDI;
	R4300i_Opcode[13] = r4300i_ORI;
	R4300i_Opcode[14] = r4300i_XORI;
	R4300i_Opcode[15] = r4300i_LUI;
	R4300i_Opcode[16] = R4300i_opcode_COP0;
	R4300i_Opcode[17] = R4300i_opcode_COP1;
	R4300i_Opcode[18] = R4300i_UnknownOpcode;
	R4300i_Opcode[19] = R4300i_UnknownOpcode;
	R4300i_Opcode[20] = r4300i_BEQL;
	R4300i_Opcode[21] = r4300i_BNEL;
	R4300i_Opcode[22] = r4300i_BLEZL;
	R4300i_Opcode[23] = r4300i_BGTZL;
	R4300i_Opcode[24] = R4300i_UnknownOpcode;
	R4300i_Opcode[25] = r4300i_DADDIU;
	R4300i_Opcode[26] = r4300i_LDL;
	R4300i_Opcode[27] = r4300i_LDR;
	R4300i_Opcode[28] = R4300i_UnknownOpcode;
	R4300i_Opcode[29] = R4300i_UnknownOpcode;
	R4300i_Opcode[30] = R4300i_UnknownOpcode;
	R4300i_Opcode[31] = R4300i_UnknownOpcode;
	R4300i_Opcode[32] = r4300i_LB;
	R4300i_Opcode[33] = r4300i_LH;
	R4300i_Opcode[34] = r4300i_LWL;
	R4300i_Opcode[35] = r4300i_LW;
	R4300i_Opcode[36] = r4300i_LBU;
	R4300i_Opcode[37] = r4300i_LHU;
	R4300i_Opcode[38] = r4300i_LWR;
	R4300i_Opcode[39] = r4300i_LWU;
	R4300i_Opcode[40] = r4300i_SB;
	R4300i_Opcode[41] = r4300i_SH;
	R4300i_Opcode[42] = r4300i_SWL;
	R4300i_Opcode[43] = r4300i_SW;
	R4300i_Opcode[44] = r4300i_SDL;
	R4300i_Opcode[45] = r4300i_SDR;
	R4300i_Opcode[46] = r4300i_SWR;
	R4300i_Opcode[47] = r4300i_CACHE;
	R4300i_Opcode[48] = r4300i_LL;
	R4300i_Opcode[49] = r4300i_LWC1;
	R4300i_Opcode[50] = R4300i_UnknownOpcode;
	R4300i_Opcode[51] = R4300i_UnknownOpcode;
	R4300i_Opcode[52] = R4300i_UnknownOpcode;
	R4300i_Opcode[53] = r4300i_LDC1;
	R4300i_Opcode[54] = R4300i_UnknownOpcode;
	R4300i_Opcode[55] = r4300i_LD;
	R4300i_Opcode[56] = r4300i_SC;
	R4300i_Opcode[57] = r4300i_SWC1;
	R4300i_Opcode[58] = R4300i_UnknownOpcode;
	R4300i_Opcode[59] = R4300i_UnknownOpcode;
	R4300i_Opcode[60] = R4300i_UnknownOpcode;
	R4300i_Opcode[61] = r4300i_SDC1;
	R4300i_Opcode[62] = R4300i_UnknownOpcode;
	R4300i_Opcode[63] = r4300i_SD;

	R4300i_Special[ 0] = r4300i_SPECIAL_SLL;
	R4300i_Special[ 1] = R4300i_UnknownOpcode;
	R4300i_Special[ 2] = r4300i_SPECIAL_SRL;
	R4300i_Special[ 3] = r4300i_SPECIAL_SRA;
	R4300i_Special[ 4] = r4300i_SPECIAL_SLLV;
	R4300i_Special[ 5] = R4300i_UnknownOpcode;
	R4300i_Special[ 6] = r4300i_SPECIAL_SRLV;
	R4300i_Special[ 7] = r4300i_SPECIAL_SRAV;
	R4300i_Special[ 8] = r4300i_SPECIAL_JR;
	R4300i_Special[ 9] = r4300i_SPECIAL_JALR;
	R4300i_Special[10] = R4300i_UnknownOpcode;
	R4300i_Special[11] = R4300i_UnknownOpcode;
	R4300i_Special[12] = r4300i_SPECIAL_SYSCALL;
	R4300i_Special[13] = R4300i_UnknownOpcode;
	R4300i_Special[14] = R4300i_UnknownOpcode;
	R4300i_Special[15] = r4300i_SPECIAL_SYNC;
	R4300i_Special[16] = r4300i_SPECIAL_MFHI;
	R4300i_Special[17] = r4300i_SPECIAL_MTHI;
	R4300i_Special[18] = r4300i_SPECIAL_MFLO;
	R4300i_Special[19] = r4300i_SPECIAL_MTLO;
	R4300i_Special[20] = r4300i_SPECIAL_DSLLV;
	R4300i_Special[21] = R4300i_UnknownOpcode;
	R4300i_Special[22] = r4300i_SPECIAL_DSRLV;
	R4300i_Special[23] = r4300i_SPECIAL_DSRAV;
	R4300i_Special[24] = r4300i_SPECIAL_MULT;
	R4300i_Special[25] = r4300i_SPECIAL_MULTU;
	R4300i_Special[26] = r4300i_SPECIAL_DIV;
	R4300i_Special[27] = r4300i_SPECIAL_DIVU;
	R4300i_Special[28] = r4300i_SPECIAL_DMULT;
	R4300i_Special[29] = r4300i_SPECIAL_DMULTU;
	R4300i_Special[30] = r4300i_SPECIAL_DDIV;
	R4300i_Special[31] = r4300i_SPECIAL_DDIVU;
	R4300i_Special[32] = r4300i_SPECIAL_ADD;
	R4300i_Special[33] = r4300i_SPECIAL_ADDU;
	R4300i_Special[34] = r4300i_SPECIAL_SUB;
	R4300i_Special[35] = r4300i_SPECIAL_SUBU;
	R4300i_Special[36] = r4300i_SPECIAL_AND;
	R4300i_Special[37] = r4300i_SPECIAL_OR;
	R4300i_Special[38] = r4300i_SPECIAL_XOR;
	R4300i_Special[39] = r4300i_SPECIAL_NOR;
	R4300i_Special[40] = R4300i_UnknownOpcode;
	R4300i_Special[41] = R4300i_UnknownOpcode;
	R4300i_Special[42] = r4300i_SPECIAL_SLT;
	R4300i_Special[43] = r4300i_SPECIAL_SLTU;
	R4300i_Special[44] = r4300i_SPECIAL_DADD;
	R4300i_Special[45] = r4300i_SPECIAL_DADDU;
	R4300i_Special[46] = r4300i_SPECIAL_DSUB;
	R4300i_Special[47] = r4300i_SPECIAL_DSUBU;
	R4300i_Special[48] = R4300i_UnknownOpcode;
	R4300i_Special[49] = R4300i_UnknownOpcode;
	R4300i_Special[50] = R4300i_UnknownOpcode;
	R4300i_Special[51] = R4300i_UnknownOpcode;
	R4300i_Special[52] = r4300i_SPECIAL_TEQ;
	R4300i_Special[53] = R4300i_UnknownOpcode;
	R4300i_Special[54] = R4300i_UnknownOpcode;
	R4300i_Special[55] = R4300i_UnknownOpcode;
	R4300i_Special[56] = r4300i_SPECIAL_DSLL;
	R4300i_Special[57] = R4300i_UnknownOpcode;
	R4300i_Special[58] = r4300i_SPECIAL_DSRL;
	R4300i_Special[59] = r4300i_SPECIAL_DSRA;
	R4300i_Special[60] = r4300i_SPECIAL_DSLL32;
	R4300i_Special[61] = R4300i_UnknownOpcode;
	R4300i_Special[62] = r4300i_SPECIAL_DSRL32;
	R4300i_Special[63] = r4300i_SPECIAL_DSRA32;

	R4300i_Regimm[ 0] = r4300i_REGIMM_BLTZ;
	R4300i_Regimm[ 1] = r4300i_REGIMM_BGEZ;
	R4300i_Regimm[ 2] = r4300i_REGIMM_BLTZL;
	R4300i_Regimm[ 3] = r4300i_REGIMM_BGEZL;
	R4300i_Regimm[ 4] = R4300i_UnknownOpcode;
	R4300i_Regimm[ 5] = R4300i_UnknownOpcode;
	R4300i_Regimm[ 6] = R4300i_UnknownOpcode;
	R4300i_Regimm[ 7] = R4300i_UnknownOpcode;
	R4300i_Regimm[ 8] = R4300i_UnknownOpcode;
	R4300i_Regimm[ 9] = R4300i_UnknownOpcode;
	R4300i_Regimm[10] = R4300i_UnknownOpcode;
	R4300i_Regimm[11] = R4300i_UnknownOpcode;
	R4300i_Regimm[12] = R4300i_UnknownOpcode;
	R4300i_Regimm[13] = R4300i_UnknownOpcode;
	R4300i_Regimm[14] = R4300i_UnknownOpcode;
	R4300i_Regimm[15] = R4300i_UnknownOpcode;
	R4300i_Regimm[16] = r4300i_REGIMM_BLTZAL;
	R4300i_Regimm[17] = r4300i_REGIMM_BGEZAL;
	R4300i_Regimm[18] = R4300i_UnknownOpcode;
	R4300i_Regimm[19] = R4300i_UnknownOpcode;
	R4300i_Regimm[20] = R4300i_UnknownOpcode;
	R4300i_Regimm[21] = R4300i_UnknownOpcode;
	R4300i_Regimm[22] = R4300i_UnknownOpcode;
	R4300i_Regimm[23] = R4300i_UnknownOpcode;
	R4300i_Regimm[24] = R4300i_UnknownOpcode;
	R4300i_Regimm[25] = R4300i_UnknownOpcode;
	R4300i_Regimm[26] = R4300i_UnknownOpcode;
	R4300i_Regimm[27] = R4300i_UnknownOpcode;
	R4300i_Regimm[28] = R4300i_UnknownOpcode;
	R4300i_Regimm[29] = R4300i_UnknownOpcode;
	R4300i_Regimm[30] = R4300i_UnknownOpcode;
	R4300i_Regimm[31] = R4300i_UnknownOpcode;
	
	R4300i_CoP0[ 0] = r4300i_COP0_MF;
	R4300i_CoP0[ 1] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 2] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 3] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 4] = r4300i_COP0_MT;
	R4300i_CoP0[ 5] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 6] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 7] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 8] = R4300i_UnknownOpcode;
	R4300i_CoP0[ 9] = R4300i_UnknownOpcode;
	R4300i_CoP0[10] = R4300i_UnknownOpcode;
	R4300i_CoP0[11] = R4300i_UnknownOpcode;
	R4300i_CoP0[12] = R4300i_UnknownOpcode;
	R4300i_CoP0[13] = R4300i_UnknownOpcode;
	R4300i_CoP0[14] = R4300i_UnknownOpcode;
	R4300i_CoP0[15] = R4300i_UnknownOpcode;
	R4300i_CoP0[16] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[17] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[18] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[19] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[20] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[21] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[22] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[23] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[24] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[25] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[26] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[27] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[28] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[29] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[30] = R4300i_opcode_COP0_CO;
	R4300i_CoP0[31] = R4300i_opcode_COP0_CO;

	R4300i_CoP0_Function[ 0] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 1] = r4300i_COP0_CO_TLBR;
	R4300i_CoP0_Function[ 2] = r4300i_COP0_CO_TLBWI;
	R4300i_CoP0_Function[ 3] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 4] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 5] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 6] = r4300i_COP0_CO_TLBWR;
	R4300i_CoP0_Function[ 7] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 8] = r4300i_COP0_CO_TLBP;
	R4300i_CoP0_Function[ 9] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[10] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[11] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[12] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[13] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[14] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[15] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[16] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[17] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[18] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[19] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[20] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[21] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[22] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[23] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[24] = r4300i_COP0_CO_ERET;
	R4300i_CoP0_Function[25] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[26] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[27] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[28] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[29] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[30] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[31] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[32] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[33] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[34] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[35] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[36] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[37] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[38] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[39] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[40] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[41] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[42] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[43] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[44] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[45] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[46] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[47] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[48] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[49] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[50] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[51] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[52] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[53] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[54] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[55] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[56] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[57] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[58] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[59] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[60] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[61] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[62] = R4300i_UnknownOpcode;
	R4300i_CoP0_Function[63] = R4300i_UnknownOpcode;
	
	R4300i_CoP1[ 0] = r4300i_COP1_MF;
	R4300i_CoP1[ 1] = r4300i_COP1_DMF;
	R4300i_CoP1[ 2] = r4300i_COP1_CF;
	R4300i_CoP1[ 3] = R4300i_UnknownOpcode;
	R4300i_CoP1[ 4] = r4300i_COP1_MT;
	R4300i_CoP1[ 5] = r4300i_COP1_DMT;
	R4300i_CoP1[ 6] = r4300i_COP1_CT;
	R4300i_CoP1[ 7] = R4300i_UnknownOpcode;
	R4300i_CoP1[ 8] = R4300i_opcode_COP1_BC;
	R4300i_CoP1[ 9] = R4300i_UnknownOpcode;
	R4300i_CoP1[10] = R4300i_UnknownOpcode;
	R4300i_CoP1[11] = R4300i_UnknownOpcode;
	R4300i_CoP1[12] = R4300i_UnknownOpcode;
	R4300i_CoP1[13] = R4300i_UnknownOpcode;
	R4300i_CoP1[14] = R4300i_UnknownOpcode;
	R4300i_CoP1[15] = R4300i_UnknownOpcode;
	R4300i_CoP1[16] = R4300i_opcode_COP1_S;
	R4300i_CoP1[17] = R4300i_opcode_COP1_D;
	R4300i_CoP1[18] = R4300i_UnknownOpcode;
	R4300i_CoP1[19] = R4300i_UnknownOpcode;
	R4300i_CoP1[20] = R4300i_opcode_COP1_W;
	R4300i_CoP1[21] = R4300i_opcode_COP1_L;
	R4300i_CoP1[22] = R4300i_UnknownOpcode;
	R4300i_CoP1[23] = R4300i_UnknownOpcode;
	R4300i_CoP1[24] = R4300i_UnknownOpcode;
	R4300i_CoP1[25] = R4300i_UnknownOpcode;
	R4300i_CoP1[26] = R4300i_UnknownOpcode;
	R4300i_CoP1[27] = R4300i_UnknownOpcode;
	R4300i_CoP1[28] = R4300i_UnknownOpcode;
	R4300i_CoP1[29] = R4300i_UnknownOpcode;
	R4300i_CoP1[30] = R4300i_UnknownOpcode;
	R4300i_CoP1[31] = R4300i_UnknownOpcode;

	R4300i_CoP1_BC[ 0] = r4300i_COP1_BCF;
	R4300i_CoP1_BC[ 1] = r4300i_COP1_BCT;
	R4300i_CoP1_BC[ 2] = r4300i_COP1_BCFL;
	R4300i_CoP1_BC[ 3] = r4300i_COP1_BCTL;
	R4300i_CoP1_BC[ 4] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 5] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 6] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 7] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 8] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 9] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[10] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[11] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[12] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[13] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[14] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[15] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[16] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[17] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[18] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[19] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[20] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[21] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[22] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[23] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[24] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[25] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[26] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[27] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[28] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[29] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[30] = R4300i_UnknownOpcode;
	R4300i_CoP1_BC[31] = R4300i_UnknownOpcode;

	R4300i_CoP1_S[ 0] = r4300i_COP1_S_ADD;
	R4300i_CoP1_S[ 1] = r4300i_COP1_S_SUB;
	R4300i_CoP1_S[ 2] = r4300i_COP1_S_MUL;
	R4300i_CoP1_S[ 3] = r4300i_COP1_S_DIV;
	R4300i_CoP1_S[ 4] = r4300i_COP1_S_SQRT;
	R4300i_CoP1_S[ 5] = r4300i_COP1_S_ABS;
	R4300i_CoP1_S[ 6] = r4300i_COP1_S_MOV;
	R4300i_CoP1_S[ 7] = r4300i_COP1_S_NEG;
	R4300i_CoP1_S[ 8] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[ 9] = r4300i_COP1_S_TRUNC_L;
	R4300i_CoP1_S[10] = r4300i_COP1_S_CEIL_L;		//added by Witten
	R4300i_CoP1_S[11] = r4300i_COP1_S_FLOOR_L;		//added by Witten
	R4300i_CoP1_S[12] = r4300i_COP1_S_ROUND_W;
	R4300i_CoP1_S[13] = r4300i_COP1_S_TRUNC_W;
	R4300i_CoP1_S[14] = r4300i_COP1_S_CEIL_W;		//added by Witten
	R4300i_CoP1_S[15] = r4300i_COP1_S_FLOOR_W;
	R4300i_CoP1_S[16] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[17] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[18] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[19] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[20] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[21] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[22] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[23] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[24] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[25] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[26] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[27] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[28] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[29] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[30] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[31] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[32] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[33] = r4300i_COP1_S_CVT_D;
	R4300i_CoP1_S[34] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[35] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[36] = r4300i_COP1_S_CVT_W;
	R4300i_CoP1_S[37] = r4300i_COP1_S_CVT_L;
	R4300i_CoP1_S[38] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[39] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[40] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[41] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[42] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[43] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[44] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[45] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[46] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[47] = R4300i_UnknownOpcode;
	R4300i_CoP1_S[48] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[49] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[50] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[51] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[52] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[53] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[54] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[55] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[56] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[57] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[58] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[59] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[60] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[61] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[62] = r4300i_COP1_S_CMP;
	R4300i_CoP1_S[63] = r4300i_COP1_S_CMP;

	R4300i_CoP1_D[ 0] = r4300i_COP1_D_ADD;
	R4300i_CoP1_D[ 1] = r4300i_COP1_D_SUB;
	R4300i_CoP1_D[ 2] = r4300i_COP1_D_MUL;
	R4300i_CoP1_D[ 3] = r4300i_COP1_D_DIV;
	R4300i_CoP1_D[ 4] = r4300i_COP1_D_SQRT;
	R4300i_CoP1_D[ 5] = r4300i_COP1_D_ABS;
	R4300i_CoP1_D[ 6] = r4300i_COP1_D_MOV;
	R4300i_CoP1_D[ 7] = r4300i_COP1_D_NEG;
	R4300i_CoP1_D[ 8] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[ 9] = r4300i_COP1_D_TRUNC_L;		//added by Witten
	R4300i_CoP1_D[10] = r4300i_COP1_D_CEIL_L;		//added by Witten
	R4300i_CoP1_D[11] = r4300i_COP1_D_FLOOR_L;		//added by Witten
	R4300i_CoP1_D[12] = r4300i_COP1_D_ROUND_W;
	R4300i_CoP1_D[13] = r4300i_COP1_D_TRUNC_W;
	R4300i_CoP1_D[14] = r4300i_COP1_D_CEIL_W;		//added by Witten
	R4300i_CoP1_D[15] = r4300i_COP1_D_FLOOR_W;		//added by Witten
	R4300i_CoP1_D[16] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[17] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[18] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[19] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[20] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[21] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[22] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[23] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[24] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[25] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[26] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[27] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[28] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[29] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[30] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[31] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[32] = r4300i_COP1_D_CVT_S;
	R4300i_CoP1_D[33] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[34] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[35] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[36] = r4300i_COP1_D_CVT_W;
	R4300i_CoP1_D[37] = r4300i_COP1_D_CVT_L;
	R4300i_CoP1_D[38] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[39] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[40] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[41] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[42] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[43] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[44] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[45] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[46] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[47] = R4300i_UnknownOpcode;
	R4300i_CoP1_D[48] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[49] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[50] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[51] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[52] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[53] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[54] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[55] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[56] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[57] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[58] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[59] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[60] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[61] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[62] = r4300i_COP1_D_CMP;
	R4300i_CoP1_D[63] = r4300i_COP1_D_CMP;

	R4300i_CoP1_W[ 0] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 1] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 2] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 3] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 4] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 5] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 6] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 7] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 8] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 9] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[10] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[11] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[12] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[13] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[14] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[15] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[16] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[17] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[18] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[19] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[20] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[21] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[22] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[23] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[24] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[25] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[26] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[27] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[28] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[29] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[30] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[31] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[32] = r4300i_COP1_W_CVT_S;
	R4300i_CoP1_W[33] = r4300i_COP1_W_CVT_D;
	R4300i_CoP1_W[34] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[35] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[36] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[37] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[38] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[39] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[40] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[41] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[42] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[43] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[44] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[45] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[46] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[47] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[48] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[49] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[50] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[51] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[52] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[53] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[54] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[55] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[56] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[57] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[58] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[59] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[60] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[61] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[62] = R4300i_UnknownOpcode;
	R4300i_CoP1_W[63] = R4300i_UnknownOpcode;	

	R4300i_CoP1_L[ 0] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 1] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 2] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 3] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 4] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 5] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 6] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 7] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 8] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 9] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[10] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[11] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[12] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[13] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[14] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[15] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[16] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[17] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[18] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[19] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[20] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[21] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[22] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[23] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[24] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[25] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[26] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[27] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[28] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[29] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[30] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[31] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[32] = r4300i_COP1_L_CVT_S;
	R4300i_CoP1_L[33] = r4300i_COP1_L_CVT_D;
	R4300i_CoP1_L[34] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[35] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[36] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[37] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[38] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[39] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[40] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[41] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[42] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[43] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[44] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[45] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[46] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[47] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[48] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[49] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[50] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[51] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[52] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[53] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[54] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[55] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[56] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[57] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[58] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[59] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[60] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[61] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[62] = R4300i_UnknownOpcode;
	R4300i_CoP1_L[63] = R4300i_UnknownOpcode;	
}

BOOL ExecuteInterpreterOpCode (void) {
	if (!r4300i_LW_VAddr(PROGRAM_COUNTER, &Opcode.Hex)) { 
		DoTLBMiss(NextInstruction == JUMP,PROGRAM_COUNTER);
		NextInstruction = NORMAL;
		return FALSE;
	} 

	COUNT_REGISTER += CountPerOp;
	*g_Timer -= CountPerOp;

	RANDOM_REGISTER -= 1;
	if ((int)RANDOM_REGISTER < (int)WIRED_REGISTER) {
		RANDOM_REGISTER = 31;
	}

	((void (_fastcall *)()) R4300i_Opcode[ Opcode.op ])();

	if (GPR[0].DW != 0) {
#if (!defined(EXTERNAL_RELEASE))
		DisplayError("GPR[0].DW has been written to");
#endif
		GPR[0].DW = 0;
	}
#ifdef Interpreter_StackTest
	if (StackValue != GPR[29].UW[0]) {
		DisplayError("Stack has Been changed");
	} 
#endif

	switch (NextInstruction) {
	case NORMAL: 
		PROGRAM_COUNTER += 4; 
		break;
	case DELAY_SLOT:
		NextInstruction = JUMP;
		PROGRAM_COUNTER += 4; 
		break;
	case JUMP:
		{
			BOOL CheckTimer = (JumpToLocation < PROGRAM_COUNTER || TestTimer); 
			PROGRAM_COUNTER  = JumpToLocation;
			NextInstruction = NORMAL;
			if (CheckTimer)
			{
				TestTimer = FALSE;
				if (*g_Timer < 0) 
				{ 
					TimerDone();
				}
				if (CPU_Action.DoSomething) { DoSomething(); }
			}
		}
		if (CPU_Type != CPU_SyncCores) {
			if (Profiling) {
				if (IndvidualBlock) {
					StartTimer(PROGRAM_COUNTER);
				} else {
					StartTimer(Timer_R4300);
				}
			}
		}
	}		
	return TRUE;
}
	
void StartInterpreterCPU (void ) { 
	//DWORD Value, Value2, Addr = 0x80031000;

	CoInitialize(NULL);
	TestTimer = FALSE;
	NextInstruction = NORMAL;
	//Add_R4300iBPoint(0x802000C8,FALSE);
	ExecuteInterpreterOps(-1);
}

void ExecuteInterpreterOps (DWORD Cycles)
{
	DWORD CyclesLeft = Cycles;
	__try {
		while(!EndEmulation()) {
#if (!defined(EXTERNAL_RELEASE))
			if (NoOfBpoints != 0) {
				if (CheckForR4300iBPoint(PROGRAM_COUNTER)) {
					UpdateCurrentR4300iRegisterPanel();
					Refresh_Memory();
					if (InR4300iCommandsWindow) {
						Enter_R4300i_Commands_Window();
						SetR4300iCommandViewto( PROGRAM_COUNTER );
						if (CPU_Action.Stepping) {
							DisplayError ( "Encounted a R4300i Breakpoint" );
						} else {
							DisplayError ( "Encounted a R4300i Breakpoint\n\nNow Stepping" );
							SetR4300iCommandToStepping();
						}
					} else {
						DisplayError ( "Encounted a R4300i Breakpoint\n\nEntering Command Window" );
						Enter_R4300i_Commands_Window();
					}					
				}
			}

			//r4300i_LW_VAddr(Addr,&Value);
			//if (Value2 != Value) {
			//	DisplayError("%X changed",Addr);
			//}
			//Value2 = Value;
			if (CPU_Action.Stepping) {
				do {
					SetR4300iCommandViewto (PROGRAM_COUNTER);
					UpdateCurrentR4300iRegisterPanel();
					Refresh_Memory();
					WaitForSingleObject( CPU_Action.hStepping, INFINITE );
					if (CPU_Action.Stepping) { ExecuteInterpreterOpCode(); }
				} while (CPU_Action.Stepping);
			}
#endif
			//if ((Profiling || ShowCPUPer) && ProfilingLabel[0] == 0) { StartTimer(Timer_R4300); };
			if (Cycles != -1) {
				if (CyclesLeft <= 0) { 
					return; 
				}
				if (ExecuteInterpreterOpCode())
				{
					CyclesLeft -= CountPerOp;
				}
			} else {
				ExecuteInterpreterOpCode();
			}
		}
	} __except( r4300i_CPU_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
		ExitThread(0);
	}
}

void TestInterpreterJump (DWORD PC, DWORD TargetPC, int Reg1, int Reg2) {
	if (PC != TargetPC) { return; }
	if (DelaySlotEffectsCompare(PC,Reg1,Reg2)) { return; }
	InPermLoop();
	NextInstruction = DELAY_SLOT;
	TestTimer = TRUE;
}