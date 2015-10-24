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

	bool SetupRegisterForLoop();

private:
	LoopAnalysis();                               // Disable default constructor
	LoopAnalysis(const LoopAnalysis&);            // Disable copy constructor
	LoopAnalysis& operator=(const LoopAnalysis&); // Disable assignment

	bool SetupEnterSection ( CCodeSection * Section, bool & bChanged, bool & bSkipedSection );
	bool CheckLoopRegisterUsage ( CCodeSection * Section );
	bool SyncRegState ( CRegInfo & RegSet, const CRegInfo& SyncReg );
	void SetJumpRegSet ( CCodeSection * Section, const CRegInfo &Reg );
	void SetContinueRegSet(CCodeSection * Section, const CRegInfo &Reg);

	/********************** R4300i OpCodes: Special **********************/
	void SPECIAL_SLL();
	void SPECIAL_SRL();
	void SPECIAL_SRA();
	void SPECIAL_SLLV();
	void SPECIAL_SRLV();
	void SPECIAL_SRAV();
	void SPECIAL_JR();
	void SPECIAL_JALR();
	void SPECIAL_SYSCALL(CCodeSection * Section);
	void SPECIAL_BREAK(CCodeSection * Section);
	void SPECIAL_MFHI();
	void SPECIAL_MTHI();
	void SPECIAL_MFLO();
	void SPECIAL_MTLO();
	void SPECIAL_DSLLV();
	void SPECIAL_DSRLV();
	void SPECIAL_DSRAV();
	void SPECIAL_ADD();
	void SPECIAL_ADDU();
	void SPECIAL_SUB();
	void SPECIAL_SUBU();
	void SPECIAL_AND();
	void SPECIAL_OR();
	void SPECIAL_XOR();
	void SPECIAL_NOR();
	void SPECIAL_SLT();
	void SPECIAL_SLTU();
	void SPECIAL_DADD();
	void SPECIAL_DADDU();
	void SPECIAL_DSUB();
	void SPECIAL_DSUBU();
	void SPECIAL_DSLL();
	void SPECIAL_DSRL();
	void SPECIAL_DSRA();
	void SPECIAL_DSLL32();
	void SPECIAL_DSRL32();
	void SPECIAL_DSRA32();

	typedef std::map<int,CRegInfo *> RegisterMap;

	RegisterMap    m_EnterRegisters;
	RegisterMap    m_ContinueRegisters;
	RegisterMap    m_JumpRegisters;
	CCodeSection * m_EnterSection;
	CCodeBlock   * m_BlockInfo;
	DWORD          m_PC;
	CRegInfo       m_Reg;
	STEP_TYPE      m_NextInstruction;
	OPCODE         m_Command;
	DWORD          m_Test;
};
