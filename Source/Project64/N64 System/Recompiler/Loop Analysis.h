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

class CCodeSection;
class CCodeBlock;

class LoopAnalysis
{
public:
	LoopAnalysis(CCodeBlock * CodeBlock, CCodeSection * Section);
	~LoopAnalysis();

	bool SetupRegisterForLoop ( void );

private:
	LoopAnalysis(void);								// Disable default constructor
	LoopAnalysis(const LoopAnalysis&);				// Disable copy constructor
	LoopAnalysis& operator=(const LoopAnalysis&);	// Disable assignment

	bool SetupEnterSection ( CCodeSection * Section, bool & bChanged, bool & bSkipedSection );
	bool CheckLoopRegisterUsage ( CCodeSection * Section );
	bool SyncRegState ( CRegInfo & RegSet, const CRegInfo& SyncReg );
	void SetJumpRegSet ( CCodeSection * Section, const CRegInfo &Reg );
	void SetContinueRegSet ( CCodeSection * Section, const CRegInfo &Reg );

	/********************** R4300i OpCodes: Special **********************/
	void SPECIAL_SLL     ( void );
	void SPECIAL_SRL     ( void );
	void SPECIAL_SRA     ( void );
	void SPECIAL_SLLV    ( void );
	void SPECIAL_SRLV    ( void );
	void SPECIAL_SRAV    ( void );
	void SPECIAL_JR      ( void );
	void SPECIAL_JALR    ( void );
	void SPECIAL_SYSCALL ( CCodeSection * Section );
	void SPECIAL_BREAK   ( CCodeSection * Section );
	void SPECIAL_MFHI    ( void );
	void SPECIAL_MTHI    ( void );
	void SPECIAL_MFLO    ( void );
	void SPECIAL_MTLO    ( void );
	void SPECIAL_DSLLV   ( void );
	void SPECIAL_DSRLV   ( void );
	void SPECIAL_DSRAV   ( void );
	void SPECIAL_ADD     ( void );
	void SPECIAL_ADDU    ( void );
	void SPECIAL_SUB     ( void );
	void SPECIAL_SUBU    ( void );
	void SPECIAL_AND     ( void );
	void SPECIAL_OR      ( void );
	void SPECIAL_XOR     ( void );
	void SPECIAL_NOR     ( void );
	void SPECIAL_SLT     ( void );
	void SPECIAL_SLTU    ( void );
	void SPECIAL_DADD    ( void );
	void SPECIAL_DADDU   ( void );
	void SPECIAL_DSUB    ( void );
	void SPECIAL_DSUBU   ( void );
	void SPECIAL_DSLL    ( void );
	void SPECIAL_DSRL    ( void );
	void SPECIAL_DSRA    ( void );
	void SPECIAL_DSLL32  ( void );
	void SPECIAL_DSRL32  ( void );
	void SPECIAL_DSRA32  ( void );

	typedef std::map<int,CRegInfo *> RegisterMap;

	RegisterMap    m_EnterRegisters;
	RegisterMap    m_ContinueRegisters;
	RegisterMap    m_JumpRegisters;
	CCodeSection * m_EnterSection;
	CCodeBlock   * m_BlockInfo;
	DWORD          m_PC;
	CRegInfo	   m_Reg;
	STEP_TYPE      m_NextInstruction;
	OPCODE         m_Command;
	DWORD          m_Test;
};
