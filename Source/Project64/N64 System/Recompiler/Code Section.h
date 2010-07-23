#pragma once 

class CCodeBlock;

class CCodeSection :
	private CRecompilerOps
{
	typedef std::list<CCodeSection *> SECTION_LIST;

public:
	CCodeSection( CCodeBlock * CodeBlock, DWORD EnterPC, DWORD ID);
	~CCodeSection( void );

	void CompileCop1Test           ( void );
	bool CreateSectionLinkage      ( void );
	bool GenerateX86Code           ( DWORD Test );
	void GenerateSectionLinkage    ( void );
	void CompileSystemCheck        ( DWORD TargetPC, const CRegInfo &RegSet );
	void CompileExit               ( DWORD JumpPC, DWORD TargetPC, CRegInfo ExitRegSet, CExitInfo::EXIT_REASON reason, int CompileNow, void (*x86Jmp)(const char * Label, DWORD Value));
	void DetermineLoop             ( DWORD Test, DWORD Test2, DWORD TestID );
	bool FixConstants              ( DWORD Test );
	CCodeSection * ExistingSection ( DWORD Addr, DWORD Test );

	/* Block Connection info */
	SECTION_LIST       m_ParentSection;
	CCodeBlock * const m_BlockInfo;
	const DWORD	       m_EnterPC;
	const DWORD        m_SectionID;
	CCodeSection     * m_ContinueSection;
	CCodeSection     * m_JumpSection;
	bool               m_LinkAllowed;  // are other sections allowed to find block to link to it
	DWORD		       m_Test;
	DWORD		       m_Test2;
	BYTE             * m_CompiledLocation;
	bool               m_DelaySlotSection;
	bool		       m_InLoop;

	/*	
	

	/* Register Info */
	CRegInfo	m_RegEnter;

	/* Jump Info */
	CJumpInfo   m_Jump;
	CJumpInfo   m_Cont;


private:
	void AddParent             ( CCodeSection * Parent );
	void InheritConstants      ( void );
	bool FillSectionInfo       ( STEP_TYPE StartStepType );
	void TestRegConstantStates ( CRegInfo & Base, CRegInfo & Reg );
	void SyncRegState          ( const CRegInfo & SyncTo );
};

