/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

class R4300iOp32 :
	public R4300iOp
{
public:
	/************************* OpCode functions *************************/
	static void  JAL            ( void );
	static void  BEQ            ( void );
	static void  BNE            ( void );
	static void  BLEZ           ( void );
	static void  BGTZ           ( void );
	static void  ADDI           ( void );
	static void  ADDIU          ( void );
	static void  SLTI           ( void );
	static void  SLTIU          ( void );
	static void  ANDI           ( void );
	static void  ORI            ( void );
	static void  XORI           ( void );
	static void  LUI            ( void );
	static void  BEQL           ( void );
	static void  BNEL           ( void );
	static void  BLEZL          ( void );
	static void  BGTZL          ( void );
	static void  LB             ( void );
	static void  LH             ( void );
	static void  LWL            ( void );
	static void  LW             ( void );
	static void  LBU            ( void );
	static void  LHU            ( void );
	static void  LWR            ( void );
	static void  LWU            ( void );
	static void  LL             ( void );

	/********************** R4300i OpCodes: Special **********************/
	static void  SPECIAL_SLL    ( void );
	static void  SPECIAL_SRL    ( void );
	static void  SPECIAL_SRA    ( void );
	static void  SPECIAL_SLLV   ( void );
	static void  SPECIAL_SRLV   ( void );
	static void  SPECIAL_SRAV   ( void );
	static void  SPECIAL_JALR   ( void );
	static void  SPECIAL_ADD    ( void );
	static void  SPECIAL_ADDU   ( void );
	static void  SPECIAL_SUB    ( void );
	static void  SPECIAL_SUBU   ( void );
	static void  SPECIAL_AND    ( void );
	static void  SPECIAL_OR     ( void );
	static void  SPECIAL_NOR    ( void );
	static void  SPECIAL_SLT    ( void );
	static void  SPECIAL_SLTU   ( void );
	static void  SPECIAL_TEQ    ( void );
	static void  SPECIAL_DSRL32 ( void );
	static void  SPECIAL_DSRA32 ( void );

	/********************** R4300i OpCodes: RegImm **********************/
	static void  REGIMM_BLTZ    ( void );
	static void  REGIMM_BGEZ    ( void );
	static void  REGIMM_BLTZL   ( void );
	static void  REGIMM_BGEZL   ( void );
	static void  REGIMM_BLTZAL  ( void );
	static void  REGIMM_BGEZAL  ( void );

	/************************** COP0 functions **************************/
	static void  COP0_MF        ( void );
	static void  COP0_MT        ( void );

	/************************** COP1 functions **************************/
	static void  COP1_MF        ( void );
	static void  COP1_CF        ( void );
	static void  COP1_DMT       ( void );

	static Func * BuildInterpreter (void );
};
