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

#ifdef __cplusplus
extern "C" {
#endif

/************************* OpCode functions *************************/
void _fastcall r4300i_J              ( void );
void _fastcall r4300i_JAL            ( void );
void _fastcall r4300i_BNE            ( void );
void _fastcall r4300i_BEQ            ( void );
void _fastcall r4300i_BLEZ           ( void );
void _fastcall r4300i_BGTZ           ( void );
void _fastcall r4300i_ADDI           ( void );
void _fastcall r4300i_ADDIU          ( void );
void _fastcall r4300i_SLTI           ( void );
void _fastcall r4300i_SLTIU          ( void );
void _fastcall r4300i_ANDI           ( void );
void _fastcall r4300i_ORI            ( void );
void _fastcall r4300i_XORI           ( void );
void _fastcall r4300i_LUI            ( void );
void _fastcall r4300i_BEQL           ( void );
void _fastcall r4300i_BNEL           ( void );
void _fastcall r4300i_BLEZL          ( void );
void _fastcall r4300i_BGTZL          ( void );
void _fastcall r4300i_DADDIU         ( void );
void _fastcall r4300i_LDL            ( void );
void _fastcall r4300i_LDR            ( void );
void _fastcall r4300i_LB             ( void );
void _fastcall r4300i_LH             ( void );
void _fastcall r4300i_LWL            ( void );
void _fastcall r4300i_LW             ( void );
void _fastcall r4300i_LBU            ( void );
void _fastcall r4300i_LHU            ( void );
void _fastcall r4300i_LWR            ( void );
void _fastcall r4300i_LWU            ( void );
void _fastcall r4300i_SB             ( void );
void _fastcall r4300i_SH             ( void );
void _fastcall r4300i_SWL            ( void );
void _fastcall r4300i_SW             ( void );
void _fastcall r4300i_SDL            ( void );
void _fastcall r4300i_SDR            ( void );
void _fastcall r4300i_SWR            ( void );
void _fastcall r4300i_CACHE          ( void );
void _fastcall r4300i_LL             ( void );
void _fastcall r4300i_LWC1           ( void );
void _fastcall r4300i_LDC1           ( void );
void _fastcall r4300i_LD             ( void );
void _fastcall r4300i_SC             ( void );
void _fastcall r4300i_SWC1           ( void );
void _fastcall r4300i_SDC1           ( void );
void _fastcall r4300i_SD             ( void );

/********************** R4300i OpCodes: Special **********************/
void _fastcall r4300i_SPECIAL_SLL    ( void );
void _fastcall r4300i_SPECIAL_SRL    ( void );
void _fastcall r4300i_SPECIAL_SRA    ( void );
void _fastcall r4300i_SPECIAL_SLLV   ( void );
void _fastcall r4300i_SPECIAL_SRLV   ( void );
void _fastcall r4300i_SPECIAL_SRAV   ( void );
void _fastcall r4300i_SPECIAL_JR     ( void );
void _fastcall r4300i_SPECIAL_JALR   ( void );
void _fastcall r4300i_SPECIAL_SYSCALL ( void );
void _fastcall r4300i_SPECIAL_BREAK   ( void );
void _fastcall r4300i_SPECIAL_SYNC    ( void );
void _fastcall r4300i_SPECIAL_MFHI    ( void );
void _fastcall r4300i_SPECIAL_MTHI    ( void );
void _fastcall r4300i_SPECIAL_MFLO   ( void );
void _fastcall r4300i_SPECIAL_MTLO   ( void );
void _fastcall r4300i_SPECIAL_DSLLV  ( void );
void _fastcall r4300i_SPECIAL_DSRLV  ( void );
void _fastcall r4300i_SPECIAL_DSRAV  ( void );
void _fastcall r4300i_SPECIAL_MULT   ( void );
void _fastcall r4300i_SPECIAL_MULTU  ( void );
void _fastcall r4300i_SPECIAL_DIV    ( void );
void _fastcall r4300i_SPECIAL_DIVU   ( void );
void _fastcall r4300i_SPECIAL_DMULT  ( void );
void _fastcall r4300i_SPECIAL_DMULTU ( void );
void _fastcall r4300i_SPECIAL_DDIV   ( void );
void _fastcall r4300i_SPECIAL_DDIVU  ( void );
void _fastcall r4300i_SPECIAL_ADD    ( void );
void _fastcall r4300i_SPECIAL_ADDU   ( void );
void _fastcall r4300i_SPECIAL_SUB    ( void );
void _fastcall r4300i_SPECIAL_SUBU   ( void );
void _fastcall r4300i_SPECIAL_AND    ( void );
void _fastcall r4300i_SPECIAL_OR     ( void );
void _fastcall r4300i_SPECIAL_XOR    ( void );
void _fastcall r4300i_SPECIAL_NOR    ( void );
void _fastcall r4300i_SPECIAL_SLT    ( void );
void _fastcall r4300i_SPECIAL_SLTU   ( void );
void _fastcall r4300i_SPECIAL_DADD   ( void );
void _fastcall r4300i_SPECIAL_DADDU  ( void );
void _fastcall r4300i_SPECIAL_DSUB   ( void );
void _fastcall r4300i_SPECIAL_DSUBU  ( void );
void _fastcall r4300i_SPECIAL_TEQ    ( void );
void _fastcall r4300i_SPECIAL_DSLL   ( void );
void _fastcall r4300i_SPECIAL_DSRL   ( void );
void _fastcall r4300i_SPECIAL_DSRA   ( void );
void _fastcall r4300i_SPECIAL_DSLL32 ( void );
void _fastcall r4300i_SPECIAL_DSRL32 ( void );
void _fastcall r4300i_SPECIAL_DSRA32 ( void );

/********************** R4300i OpCodes: RegImm **********************/
void _fastcall r4300i_REGIMM_BLTZ    ( void );
void _fastcall r4300i_REGIMM_BGEZ    ( void );
void _fastcall r4300i_REGIMM_BLTZL   ( void );
void _fastcall r4300i_REGIMM_BGEZL   ( void );
void _fastcall r4300i_REGIMM_BLTZAL  ( void );
void _fastcall r4300i_REGIMM_BGEZAL  ( void );

/************************** COP0 functions **************************/
void _fastcall r4300i_COP0_MF        ( void );
void _fastcall r4300i_COP0_MT        ( void );

/************************** COP0 CO functions ***********************/
void _fastcall r4300i_COP0_CO_TLBR   ( void );
void _fastcall r4300i_COP0_CO_TLBWI  ( void );
void _fastcall r4300i_COP0_CO_TLBWR  ( void );
void _fastcall r4300i_COP0_CO_TLBP   ( void );
void _fastcall r4300i_COP0_CO_ERET   ( void );

/************************** COP1 functions **************************/
void _fastcall r4300i_COP1_MF        ( void );
void _fastcall r4300i_COP1_DMF       ( void );
void _fastcall r4300i_COP1_CF        ( void );
void _fastcall r4300i_COP1_MT        ( void );
void _fastcall r4300i_COP1_DMT       ( void );
void _fastcall r4300i_COP1_CT        ( void );

/************************* COP1: BC1 functions ***********************/
void _fastcall r4300i_COP1_BCF       ( void );
void _fastcall r4300i_COP1_BCT       ( void );
void _fastcall r4300i_COP1_BCFL      ( void );
void _fastcall r4300i_COP1_BCTL      ( void );

/************************** COP1: S functions ************************/
void _fastcall r4300i_COP1_S_ADD     ( void );
void _fastcall r4300i_COP1_S_SUB     ( void );
void _fastcall r4300i_COP1_S_MUL     ( void );
void _fastcall r4300i_COP1_S_DIV     ( void );
void _fastcall r4300i_COP1_S_SQRT    ( void );
void _fastcall r4300i_COP1_S_ABS     ( void );
void _fastcall r4300i_COP1_S_MOV     ( void );
void _fastcall r4300i_COP1_S_NEG     ( void );
void _fastcall r4300i_COP1_S_TRUNC_L ( void );
void _fastcall r4300i_COP1_S_CEIL_L  ( void );	//added by Witten
void _fastcall r4300i_COP1_S_FLOOR_L ( void );	//added by Witten
void _fastcall r4300i_COP1_S_ROUND_W ( void );
void _fastcall r4300i_COP1_S_TRUNC_W ( void );
void _fastcall r4300i_COP1_S_CEIL_W  ( void );	//added by Witten
void _fastcall r4300i_COP1_S_FLOOR_W ( void );
void _fastcall r4300i_COP1_S_CVT_D   ( void );
void _fastcall r4300i_COP1_S_CVT_W   ( void );
void _fastcall r4300i_COP1_S_CVT_L   ( void );
void _fastcall r4300i_COP1_S_CMP     ( void );

/************************** COP1: D functions ************************/
void _fastcall r4300i_COP1_D_ADD     ( void );
void _fastcall r4300i_COP1_D_SUB     ( void );
void _fastcall r4300i_COP1_D_MUL     ( void );
void _fastcall r4300i_COP1_D_DIV     ( void );
void _fastcall r4300i_COP1_D_SQRT    ( void );
void _fastcall r4300i_COP1_D_ABS     ( void );
void _fastcall r4300i_COP1_D_MOV     ( void );
void _fastcall r4300i_COP1_D_NEG     ( void );
void _fastcall r4300i_COP1_D_TRUNC_L ( void );	//added by Witten	
void _fastcall r4300i_COP1_D_CEIL_L  ( void );	//added by Witten
void _fastcall r4300i_COP1_D_FLOOR_L ( void );	//added by Witten
void _fastcall r4300i_COP1_D_ROUND_W ( void );
void _fastcall r4300i_COP1_D_TRUNC_W ( void );
void _fastcall r4300i_COP1_D_CEIL_W  ( void );	//added by Witten
void _fastcall r4300i_COP1_D_FLOOR_W ( void );	//added by Witten
void _fastcall r4300i_COP1_D_CVT_S   ( void );
void _fastcall r4300i_COP1_D_CVT_W   ( void );
void _fastcall r4300i_COP1_D_CVT_L   ( void );
void _fastcall r4300i_COP1_D_CMP     ( void );

/************************** COP1: W functions ************************/
void _fastcall r4300i_COP1_W_CVT_S   ( void );
void _fastcall r4300i_COP1_W_CVT_D   ( void );

/************************** COP1: L functions ************************/
void _fastcall r4300i_COP1_L_CVT_S   ( void );
void _fastcall r4300i_COP1_L_CVT_D   ( void );

/************************** Other functions **************************/	
void _fastcall  R4300i_UnknownOpcode ( void );

extern DWORD SWL_MASK[4], SWR_MASK[4], LWL_MASK[4], LWR_MASK[4];
extern int SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];
extern int RoundingModel;


#ifdef __cplusplus
}
#endif
