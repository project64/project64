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

class CCodeBlock :
	private CRecompilerOps
{
public:
	CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos );
	~CCodeBlock();

	bool Compile();

	DWORD    VAddrEnter() const { return m_VAddrEnter; }
	DWORD    VAddrFirst() const { return m_VAddrFirst; }
	DWORD    VAddrLast()  const { return m_VAddrLast; }
	BYTE *   CompiledLocation() const { return m_CompiledLocation; }
	int      NoOfSections() const { return m_Sections.size(); }
	const CCodeSection & EnterSection() const { return *m_EnterSection; }
	const MD5Digest & Hash() const { return m_Hash; }

	void SetVAddrFirst(DWORD VAddr) { m_VAddrFirst = VAddr; }
	void SetVAddrLast(DWORD VAddr) { m_VAddrLast = VAddr; }

	CCodeSection * ExistingSection(DWORD Addr) { return m_EnterSection->ExistingSection(Addr, NextTest()); }
	bool SectionAccessible(DWORD m_SectionID) { return m_EnterSection->SectionAccessible(m_SectionID, NextTest()); }

	QWORD   MemContents(int i) const { return m_MemContents[i]; }
	QWORD * MemLocation(int i) const { return m_MemLocation[i]; }

	DWORD NextTest();

	EXIT_LIST       m_ExitInfo;

private:
	CCodeBlock();                             // Disable default constructor
	CCodeBlock(const CCodeBlock&);            // Disable copy constructor
	CCodeBlock& operator=(const CCodeBlock&); // Disable assignment

	bool AnalyseBlock();
	void CompileExitCode();

	bool CreateBlockLinkage ( CCodeSection * EnterSection );
	void DetermineLoops     ();
	void LogSectionInfo     ();
	bool SetSection         ( CCodeSection * & Section, CCodeSection * CurrentSection, DWORD TargetPC, bool LinkAllowed, DWORD CurrentPC );
	bool SetJumpInfo        ( CCodeSection * & Section, DWORD TargetPC, DWORD CurrentPC );
	bool AnalyzeInstruction ( DWORD PC, DWORD & TargetPC, DWORD & ContinuePC, bool & LikelyBranch, bool & IncludeDelaySlot, 
		bool & EndBlock, bool & PermLoop );

	DWORD           m_VAddrEnter;
	DWORD           m_VAddrFirst;       // the address of the first opcode in the block
	DWORD           m_VAddrLast;        // the address of the first opcode in the block
	BYTE*           m_CompiledLocation; // What address is this compiled at
	
	typedef std::map<DWORD,CCodeSection *> SectionMap;
	typedef std::list<CCodeSection *>      SectionList;

	SectionMap      m_SectionMap;
	SectionList     m_Sections;
	CCodeSection  * m_EnterSection;
	long            m_Test;
	MD5Digest       m_Hash;
	QWORD           m_MemContents[2];
	QWORD *         m_MemLocation[2];
};
