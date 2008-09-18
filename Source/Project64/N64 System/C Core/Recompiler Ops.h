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
enum BRANCH_TYPE
{
	BranchTypeCop1, BranchTypeRs, BranchTypeRsRt
};

void CompileReadTLBMiss  (CBlockSection * Section, int AddressReg, int LookUpReg );
void CompileWriteTLBMiss (CBlockSection * Section, int AddressReg, int LookUpReg );

/************************** Branch functions  ************************/
void Compile_R4300i_Branch         ( CBlockSection * Section, void (*CompareFunc)(CBlockSection * Section), BRANCH_TYPE BranchType, BOOL Link);
void Compile_R4300i_BranchLikely   ( CBlockSection * Section, void (*CompareFunc)(CBlockSection * Section), BOOL Link);
void BNE_Compare                   ( CBlockSection * Section );
void BEQ_Compare                   ( CBlockSection * Section );
void BGTZ_Compare                  ( CBlockSection * Section );
void BLEZ_Compare                  ( CBlockSection * Section );
void BLTZ_Compare                  ( CBlockSection * Section );
void BGEZ_Compare                  ( CBlockSection * Section );
void COP1_BCF_Compare              ( CBlockSection * Section );
void COP1_BCT_Compare              ( CBlockSection * Section );

/*************************  OpCode functions *************************/
void Compile_R4300i_J              ( CBlockSection * Section );
void Compile_R4300i_JAL            ( CBlockSection * Section );
void Compile_R4300i_ADDI           ( CBlockSection * Section );
void Compile_R4300i_ADDIU          ( CBlockSection * Section );
void Compile_R4300i_SLTI           ( CBlockSection * Section );
void Compile_R4300i_SLTIU          ( CBlockSection * Section );
void Compile_R4300i_ANDI           ( CBlockSection * Section );
void Compile_R4300i_ORI            ( CBlockSection * Section );
void Compile_R4300i_XORI           ( CBlockSection * Section );
void Compile_R4300i_LUI            ( CBlockSection * Section );
void Compile_R4300i_DADDIU         ( CBlockSection * Section );
void Compile_R4300i_LDL            ( CBlockSection * Section );
void Compile_R4300i_LDR            ( CBlockSection * Section );
void Compile_R4300i_LB             ( CBlockSection * Section );
void Compile_R4300i_LH             ( CBlockSection * Section );
void Compile_R4300i_LWL            ( CBlockSection * Section );
void Compile_R4300i_LW             ( CBlockSection * Section );
void Compile_R4300i_LBU            ( CBlockSection * Section );
void Compile_R4300i_LHU            ( CBlockSection * Section );
void Compile_R4300i_LWR            ( CBlockSection * Section );
void Compile_R4300i_LWU            ( CBlockSection * Section );		//added by Witten
void Compile_R4300i_SB             ( CBlockSection * Section );
void Compile_R4300i_SH             ( CBlockSection * Section );
void Compile_R4300i_SWL            ( CBlockSection * Section );
void Compile_R4300i_SW             ( CBlockSection * Section );
void Compile_R4300i_SWR            ( CBlockSection * Section );
void Compile_R4300i_SDL            ( CBlockSection * Section );
void Compile_R4300i_SDR            ( CBlockSection * Section );
void Compile_R4300i_CACHE          ( CBlockSection * Section );
void Compile_R4300i_LL             ( CBlockSection * Section );
void Compile_R4300i_LWC1           ( CBlockSection * Section );
void Compile_R4300i_LDC1           ( CBlockSection * Section );
void Compile_R4300i_LD             ( CBlockSection * Section );
void Compile_R4300i_SC             ( CBlockSection * Section );
void Compile_R4300i_SWC1           ( CBlockSection * Section );
void Compile_R4300i_SDC1           ( CBlockSection * Section );
void Compile_R4300i_SD             ( CBlockSection * Section );

/********************** R4300i OpCodes: Special **********************/
void Compile_R4300i_SPECIAL_SLL    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SRL    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SRA    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SLLV   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SRLV   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SRAV   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_JR     ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_JALR   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SYSCALL( CBlockSection * Section );
void Compile_R4300i_SPECIAL_MFLO   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_MTLO   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_MFHI   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_MTHI   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSLLV  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSRLV  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSRAV  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_MULT   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_MULTU  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DIV    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DIVU   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DMULT  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DMULTU ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DDIV   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DDIVU  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_ADD    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_ADDU   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SUB    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SUBU   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_AND    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_OR     ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_XOR    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_NOR    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SLT    ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_SLTU   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DADD   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DADDU  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSUB   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSUBU  ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSLL   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSRL   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSRA   ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSLL32 ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSRL32 ( CBlockSection * Section );
void Compile_R4300i_SPECIAL_DSRA32 ( CBlockSection * Section );

/************************** COP0 functions **************************/
void Compile_R4300i_COP0_MF        ( CBlockSection * Section );
void Compile_R4300i_COP0_MT        ( CBlockSection * Section );

/************************** COP0 CO functions ***********************/
void Compile_R4300i_COP0_CO_TLBR   ( CBlockSection * Section );
void Compile_R4300i_COP0_CO_TLBWI  ( CBlockSection * Section );
void Compile_R4300i_COP0_CO_TLBWR  ( CBlockSection * Section );
void Compile_R4300i_COP0_CO_TLBP   ( CBlockSection * Section );
void Compile_R4300i_COP0_CO_ERET   ( CBlockSection * Section );

/************************** COP1 functions **************************/
void Compile_R4300i_COP1_MF        ( CBlockSection * Section );
void Compile_R4300i_COP1_DMF       ( CBlockSection * Section );
void Compile_R4300i_COP1_CF        ( CBlockSection * Section );
void Compile_R4300i_COP1_MT        ( CBlockSection * Section );
void Compile_R4300i_COP1_DMT       ( CBlockSection * Section );
void Compile_R4300i_COP1_CT        ( CBlockSection * Section );

/************************** COP1: S functions ************************/
void Compile_R4300i_COP1_S_ADD     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_SUB     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_MUL     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_DIV     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_ABS     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_NEG     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_SQRT    ( CBlockSection * Section );
void Compile_R4300i_COP1_S_MOV     ( CBlockSection * Section );
void Compile_R4300i_COP1_S_TRUNC_L ( CBlockSection * Section );
void Compile_R4300i_COP1_S_CEIL_L  ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_S_FLOOR_L ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_S_ROUND_W ( CBlockSection * Section );
void Compile_R4300i_COP1_S_TRUNC_W ( CBlockSection * Section );
void Compile_R4300i_COP1_S_CEIL_W  ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_S_FLOOR_W ( CBlockSection * Section );
void Compile_R4300i_COP1_S_CVT_D   ( CBlockSection * Section );
void Compile_R4300i_COP1_S_CVT_W   ( CBlockSection * Section );
void Compile_R4300i_COP1_S_CVT_L   ( CBlockSection * Section );
void Compile_R4300i_COP1_S_CMP     ( CBlockSection * Section );

/************************** COP1: D functions ************************/
void Compile_R4300i_COP1_D_ADD     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_SUB     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_MUL     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_DIV     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_ABS     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_NEG     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_SQRT    ( CBlockSection * Section );
void Compile_R4300i_COP1_D_MOV     ( CBlockSection * Section );
void Compile_R4300i_COP1_D_TRUNC_L ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_D_CEIL_L  ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_D_FLOOR_L ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_D_ROUND_W ( CBlockSection * Section );
void Compile_R4300i_COP1_D_TRUNC_W ( CBlockSection * Section );
void Compile_R4300i_COP1_D_CEIL_W  ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_D_FLOOR_W ( CBlockSection * Section );			//added by Witten
void Compile_R4300i_COP1_D_CVT_S   ( CBlockSection * Section );
void Compile_R4300i_COP1_D_CVT_W   ( CBlockSection * Section );
void Compile_R4300i_COP1_D_CVT_L   ( CBlockSection * Section );
void Compile_R4300i_COP1_D_CMP     ( CBlockSection * Section );

/************************** COP1: W functions ************************/
void Compile_R4300i_COP1_W_CVT_S   ( CBlockSection * Section );
void Compile_R4300i_COP1_W_CVT_D   ( CBlockSection * Section );

/************************** COP1: L functions ************************/
void Compile_R4300i_COP1_L_CVT_S   ( CBlockSection * Section );
void Compile_R4300i_COP1_L_CVT_D   ( CBlockSection * Section );

/************************** Other functions **************************/
void Compile_R4300i_UnknownOpcode  ( CBlockSection * Section );
