class CBlockSection;
typedef std::list<CBlockSection *> SECTION_LIST;

class CRegInfo
{
public:
	//enums
	enum REG_STATE {
		STATE_UNKNOWN        = 0,
		STATE_KNOWN_VALUE    = 1,
		STATE_X86_MAPPED     = 2,
		STATE_SIGN           = 4,
		STATE_32BIT          = 8,

		STATE_MAPPED_64      = (STATE_KNOWN_VALUE | STATE_X86_MAPPED), // = 3
		STATE_MAPPED_32_ZERO = (STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT), // = 11
		STATE_MAPPED_32_SIGN = (STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT | STATE_SIGN), // = 15

		STATE_CONST_32       = (STATE_KNOWN_VALUE | STATE_32BIT | STATE_SIGN), // = 13
		STATE_CONST_64       = (STATE_KNOWN_VALUE), // = 1
	};

	enum REG_MAPPED {
		NotMapped            = 0,
		GPR_Mapped           = 1,
		Temp_Mapped          = 2,
		Stack_Mapped         = 3,
	};

	enum FPU_STATE {
		FPU_Unkown,FPU_Dword, FPU_Qword, FPU_Float, FPU_Double
	};

	enum FPU_ROUND {
		RoundUnknown, RoundDefault, RoundTruncate, RoundNearest, RoundDown, RoundUp
	};

private:
	//r4k
	REG_STATE   MIPS_RegState[32];
	MIPS_DWORD	MIPS_RegVal[32];

	REG_MAPPED	x86reg_MappedTo[10];
	DWORD		x86reg_MapOrder[10];
	bool		x86reg_Protected[10];
	
	DWORD		CycleCount;
	DWORD		RandomModifier;

	//FPU
	int			Stack_TopPos;
	DWORD		x86fpu_MappedTo[8];
	FPU_STATE	x86fpu_State[8];
	FPU_ROUND	x86fpu_RoundingModel[8];
	
	bool        Fpu_Used;
	FPU_ROUND   RoundingModel;
	
	bool        compare(const CRegInfo& right) const;

public:
	bool operator==(const CRegInfo& right) const;
	bool operator!=(const CRegInfo& right) const;

	static REG_STATE ConstantsType ( __int64 Value );

	void Initilize ( void );

	inline bool IsKnown(int Reg)        { return ((MipsRegState(Reg) & STATE_KNOWN_VALUE) != 0); }
	inline bool IsUnknown(int Reg)		{ return (!IsKnown(Reg)); }

	inline bool IsMapped(int Reg)		{ return (IsKnown(Reg) && (MipsRegState(Reg) & STATE_X86_MAPPED) != 0); }
	inline bool IsConst(int Reg)		{ return (IsKnown(Reg) && !IsMapped(Reg)); }

	inline bool IsSigned(int Reg)		{ return (IsKnown(Reg) && (MipsRegState(Reg) & STATE_SIGN) != 0); }
	inline bool IsUnsigned(int Reg)		{ return (IsKnown(Reg) && !IsSigned(Reg)); }

	inline bool Is32Bit(int Reg)		{ return (IsKnown(Reg) && (MipsRegState(Reg) & STATE_32BIT) != 0); }
	inline bool Is64Bit(int Reg)		{ return (IsKnown(Reg) && !Is32Bit(Reg)); }

	inline bool Is32BitMapped(int Reg)	{ return (Is32Bit(Reg) && (MipsRegState(Reg) & STATE_X86_MAPPED) != 0); }
	inline bool Is64BitMapped(int Reg)	{ return (Is64Bit(Reg) && !Is32BitMapped(Reg)); }

	inline REG_STATE &       MipsRegState ( int Reg ) { return MIPS_RegState[Reg]; }
	inline unsigned _int64 & MipsReg      ( int Reg ) { return MIPS_RegVal[Reg].UDW; }
	inline _int64 &          MipsReg_S    ( int Reg ) { return MIPS_RegVal[Reg].DW; }
	inline DWORD &           MipsRegLo    ( int Reg ) { return MIPS_RegVal[Reg].UW[0]; }
	inline long &            MipsRegLo_S  ( int Reg ) { return MIPS_RegVal[Reg].W[0]; }
	inline DWORD &           MipsRegHi    ( int Reg ) { return MIPS_RegVal[Reg].UW[1]; }
	inline long &            MipsRegHi_S  ( int Reg ) { return MIPS_RegVal[Reg].W[1]; }

	inline DWORD & x86MapOrder( int Reg )	{ return x86reg_MapOrder[Reg]; }
	inline bool & x86Protected( int Reg )	{ return x86reg_Protected[Reg]; }
	inline REG_MAPPED & x86Mapped(int Reg)	{ return x86reg_MappedTo[Reg]; }

	inline DWORD & BlockCycleCount(void)     { return CycleCount; }
	inline DWORD & BlockRandomModifier(void) { return RandomModifier; }


	inline int & StackTopPos ( void ) { return Stack_TopPos; }
	inline DWORD & FpuMappedTo( int Reg) { return x86fpu_MappedTo[Reg]; }
	inline FPU_STATE & FpuState(int Reg) { return x86fpu_State[Reg]; }
	inline FPU_ROUND & FpuRoundingModel(int Reg) { return x86fpu_RoundingModel[Reg]; }
	inline bool & FpuBeenUsed (void )	{ return Fpu_Used; }
	inline FPU_ROUND  & CurrentRoundingModel ( void ) { return RoundingModel; }

};

class CJumpInfo
{
public:
	CJumpInfo();

	DWORD		TargetPC;
	char *		BranchLabel;
	BYTE *		LinkLocation;
	BYTE *		LinkLocation2;	
	BOOL		FallThrough;	
	BOOL		PermLoop;
	BOOL		DoneDelaySlot;
	CRegInfo	RegSet;
};

class CBlockInfo;
class CBlockSection
{
public:
	CBlockSection( CBlockInfo * _BlockInfo, DWORD StartAddr, DWORD ID);
	~CBlockSection( void );

	CBlockInfo * const BlockInfo;

	/* Block Connection info */
	SECTION_LIST    ParentSection;
	CBlockSection *	ContinueSection;
	CBlockSection *	JumpSection;
	BYTE          * CompiledLocation;

	DWORD		SectionID;
	DWORD		Test;
	DWORD		Test2;
	bool		InLoop;
	bool        LinkAllowed;  // are other sections allowed to find block to link to it
	bool        DelaySlotSection;
	
	DWORD		StartPC;
	DWORD		CompilePC;

	/* Register Info */
	CRegInfo	RegStart;
	CRegInfo	RegWorking;

	/* Jump Info */
	CJumpInfo   Jump;
	CJumpInfo   Cont;

	void AddParent          ( CBlockSection * Parent );
	void UnlinkParent       ( CBlockSection * Parent, bool AllowDelete, bool ContinueLink );
	bool IsAllParentLoops   ( CBlockSection * Parent, bool IgnoreIfCompiled, DWORD Test );
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
	inline CRegInfo::FPU_ROUND & CurrentRoundingModel ( void ) { return RegWorking.CurrentRoundingModel(); }
};

class CExitInfo
{
public:
	enum EXIT_REASON
	{
		Normal					= 0,
		Normal_NoSysCheck		= 1,
		DoCPU_Action			= 2,
		COP1_Unuseable			= 3,
		DoSysCall				= 4,
		TLBReadMiss				= 5,
		TLBWriteMiss			= 6,
		ExitResetRecompCode		= 7,
	};

	DWORD       ID;
	DWORD       TargetPC;
	CRegInfo    ExitRegSet;
	EXIT_REASON reason;
	STEP_TYPE   NextInstruction;
	BYTE *      JumpLoc; //32bit jump
};

typedef std::list<CExitInfo> EXIT_LIST;

class CBlockInfo
{
public:
	CBlockInfo(DWORD VAddr, BYTE * RecompPos);
	
	DWORD	 	    StartVAddr;
	DWORD	 	    EndVAddr;
	BYTE *		    CompiledLocation;
	int             NoOfSections;
	CBlockSection   ParentSection;
	EXIT_LIST       ExitInfo;
};

typedef struct {
	CBlockSection * Parent;
	CJumpInfo     * JumpInfo;
} BLOCK_PARENT;

typedef std::vector<BLOCK_PARENT> BLOCK_PARENT_LIST;
