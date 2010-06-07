#pragma once 

class CCodeBlock;

class CCodeSection :
	private CRecompilerOps
{
public:
	CCodeSection( CCodeBlock * CodeBlock, DWORD EnterPC, DWORD ID);
	~CCodeSection( void );

	void CompileCop1Test    ( void );
	bool GenerateX86Code    ( DWORD Test );
	void GenerateSectionLinkage ( void );
	void CompileSystemCheck ( DWORD TargetPC, const CRegInfo &RegSet );
	void CompileExit        ( DWORD JumpPC, DWORD TargetPC, CRegInfo ExitRegSet, CExitInfo::EXIT_REASON reason, int CompileNow, void (*x86Jmp)(const char * Label, DWORD Value));

	/* Block Connection info */
	CCodeBlock * const m_BlockInfo;
	const DWORD	       m_EnterPC;
	const DWORD        m_SectionID;
	CCodeSection     * m_ContinueSection;
	CCodeSection     * m_JumpSection;
	bool               m_LinkAllowed;  // are other sections allowed to find block to link to it
	DWORD		       m_Test;
	BYTE             * m_CompiledLocation;
	bool               m_DelaySlotSection;

	/*	SECTION_LIST    ParentSection;
	DWORD		Test2;
	bool		InLoop;
	

	/* Register Info */
	CRegInfo	m_RegEnter;

	/* Jump Info */
	CJumpInfo   m_Jump;
	CJumpInfo   m_Cont;


private:

};

