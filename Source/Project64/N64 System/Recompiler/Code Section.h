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

	/*	SECTION_LIST    ParentSection;

	DWORD		Test2;
	bool		InLoop;
	bool        DelaySlotSection;
	

	/* Register Info */
	CRegInfo	m_RegEnter;

	/* Jump Info */
	CJumpInfo   m_Jump;
	CJumpInfo   m_Cont;

	//Information about the opcode current being compiled
/*	DWORD		m_CompilePC;
	OPCODE      m_CompileOpcode;

	void AddParent          ( CCodeSection * Parent );
	void UnlinkParent       ( CCodeSection * Parent, bool AllowDelete, bool ContinueLink );
	bool IsAllParentLoops   ( CCodeSection * Parent, bool IgnoreIfCompiled, DWORD Test );
	void ResetX86Protection ( void );
	static DWORD GetNewTestValue ( void );

	static void TestRegConstantStates ( CRegInfo & Base, CRegInfo & Reg  );
	
	//Handy Functions
	inline CRegInfo::REG_STATE & MipsRegState ( int Reg ) { return RegWorking.MipsRegState(Reg); }
	inline unsigned _int64 & MipsReg      ( int Reg ) { return RegWorking.MipsReg(Reg); }
	inline _int64 &          MipsReg_S    ( int Reg ) { return RegWorking.MipsReg_S(Reg); }
	inline DWORD &           MipsRegLo    ( int Reg ) { return RegWorking.MipsRegLo(Reg); }
	inline long &            MipsRegLo_S  ( int Reg ) { return RegWorking.MipsRegLo_S(Reg); }
	inline DWORD &           MipsRegHi    ( int Reg ) { return RegWorking.MipsRegHi(Reg); }
	inline long &            MipsRegHi_S  ( int Reg ) { return RegWorking.MipsRegHi_S(Reg); }
	inline DWORD & BlockCycleCount(void)     { return RegWorking.BlockCycleCount(); }
	inline DWORD & BlockRandomModifier(void) { return RegWorking.BlockRandomModifier(); }

	inline DWORD & x86MapOrder( int Reg )	{ return RegWorking.x86MapOrder(Reg); }
	inline bool & x86Protected( int Reg )	{ return RegWorking.x86Protected(Reg); }
	inline CRegInfo::REG_MAPPED & x86Mapped(int Reg)	{ return RegWorking.x86Mapped(Reg); }

	inline bool IsKnown(int Reg)        { return RegWorking.IsKnown(Reg); }
	inline bool IsUnknown(int Reg)		{ return RegWorking.IsUnknown(Reg); }

	inline bool IsMapped(int Reg)		{ return RegWorking.IsMapped(Reg); }
	inline bool IsConst(int Reg)		{ return RegWorking.IsConst(Reg); }

	inline bool IsSigned(int Reg)		{ return RegWorking.IsSigned(Reg); }
	inline bool IsUnsigned(int Reg)		{ return RegWorking.IsUnsigned(Reg); }

	inline bool Is32Bit(int Reg)		{ return RegWorking.Is32Bit(Reg); }
	inline bool Is64Bit(int Reg)		{ return RegWorking.Is64Bit(Reg); }

	inline bool Is32BitMapped(int Reg)	{ return RegWorking.Is32BitMapped(Reg); }
	inline bool Is64BitMapped(int Reg)	{ return RegWorking.Is64BitMapped(Reg); }

	inline int & StackTopPos ( void ) { return RegWorking.StackTopPos(); }
	inline DWORD & FpuMappedTo( int Reg) { return RegWorking.FpuMappedTo(Reg); }
	inline CRegInfo::FPU_STATE & FpuState(int Reg) { return RegWorking.FpuState(Reg); }
	inline CRegInfo::FPU_ROUND & FpuRoundingModel(int Reg) { return RegWorking.FpuRoundingModel(Reg); }
	inline bool & FpuBeenUsed (void )	{ return RegWorking.FpuBeenUsed(); }
	inline CRegInfo::FPU_ROUND & CurrentRoundingModel ( void ) { return RegWorking.CurrentRoundingModel(); }*/

private:

};

