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

class CCodeBlock;

class CCodeSection :
	private CRecompilerOps
{
public:
	typedef std::list<CCodeSection *> SECTION_LIST;

	CCodeSection( CCodeBlock * CodeBlock, DWORD EnterPC, DWORD ID, bool LinkAllowed);
	~CCodeSection();

	void SetDelaySlot              ();
	void SetJumpAddress            ( DWORD JumpPC, DWORD TargetPC, bool PermLoop );
	void SetContinueAddress        ( DWORD JumpPC, DWORD TargetPC );
	void CompileCop1Test           ();
	bool GenerateX86Code           ( DWORD Test );
	void GenerateSectionLinkage    ();
	void CompileExit               ( DWORD JumpPC, DWORD TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason, bool CompileNow, void (*x86Jmp)(const char * Label, DWORD Value));
	void DetermineLoop             ( DWORD Test, DWORD Test2, DWORD TestID );
	bool FixConstants              ( DWORD Test );
	CCodeSection * ExistingSection ( DWORD Addr, DWORD Test );
	bool SectionAccessible         ( DWORD SectionId, DWORD Test );
	bool DisplaySectionInformation ( DWORD ID, DWORD Test );
	void DisplaySectionInformation ();
	void AddParent                 ( CCodeSection * Parent );
	void SwitchParent              ( CCodeSection * OldParent, CCodeSection * NewParent );

	/* Block Connection info */
	SECTION_LIST       m_ParentSection;
	CCodeBlock * const m_BlockInfo;
	const DWORD        m_SectionID;
	const DWORD	       m_EnterPC;
	DWORD	           m_EndPC;
	CCodeSection     * m_ContinueSection;
	CCodeSection     * m_JumpSection;
	bool               m_EndSection;   // if this section does not link
	bool               m_LinkAllowed;  // are other sections allowed to find block to link to it
	DWORD              m_Test;
	DWORD              m_Test2;
	BYTE             * m_CompiledLocation;
	bool               m_InLoop;
	bool               m_DelaySlot;

	/* Register Info */
	CRegInfo    m_RegEnter;

	/* Jump Info */
	CJumpInfo   m_Jump;
	CJumpInfo   m_Cont;

private:
	void UnlinkParent           ( CCodeSection * Parent, bool ContinueSection );
	void InheritConstants       ();
	void TestRegConstantStates  ( CRegInfo & Base, CRegInfo & Reg );
	void SyncRegState           ( const CRegInfo & SyncTo );
	bool IsAllParentLoops       ( CCodeSection * Parent, bool IgnoreIfCompiled, DWORD Test ); 
	bool ParentContinue         ();
	bool InheritParentInfo      ();
	bool SetupRegisterForLoop   ();
};

