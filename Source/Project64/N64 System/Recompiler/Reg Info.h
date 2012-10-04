class CRegInfo :
	private CX86Ops,
	private CSystemRegisters
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
		FPU_Any     = -1,
		FPU_Unknown = 0,
		FPU_Dword   = 1, 
		FPU_Qword   = 2, 
		FPU_Float   = 3, 
		FPU_Double  = 4,
	};

	enum FPU_ROUND {
		RoundUnknown  = -1, 
		RoundDefault  = 0, 
		RoundTruncate = 1, 
		RoundNearest  = 2, 
		RoundDown     = 3, 
		RoundUp       = 4,
	};
public:
	bool operator==(const CRegInfo& right) const;
	bool operator!=(const CRegInfo& right) const;

	static REG_STATE ConstantsType ( __int64 Value );

	void Initilize ( bool b32bitCore );

	void   FixRoundModel      ( FPU_ROUND RoundMethod );
	void   ChangeFPURegFormat ( int Reg, FPU_STATE OldFormat, FPU_STATE NewFormat, FPU_ROUND RoundingModel );
	void   Load_FPR_ToTop     ( int Reg, int RegToLoad, FPU_STATE Format);
	BOOL   RegInStack         ( int Reg, FPU_STATE Format );
	void   UnMap_AllFPRs      ( void );
	void   UnMap_FPR          ( int Reg, int WriteBackValue );
	x86FpuValues StackPosition      ( int Reg );

	x86Reg FreeX86Reg         ( void );
	x86Reg Free8BitX86Reg     ( void );
	void   Map_GPR_32bit      ( int MipsReg, BOOL SignValue, int MipsRegToLoad );
	void   Map_GPR_64bit      ( int MipsReg, int MipsRegToLoad );
	x86Reg Get_MemoryStack    ( void ) const;
	x86Reg Map_MemoryStack    ( x86Reg Reg, bool bMapRegister, bool LoadValue = true );
	x86Reg Map_TempReg        ( x86Reg Reg, int MipsReg, BOOL LoadHiWord );
	void   ProtectGPR         ( DWORD Reg );
	void   UnProtectGPR       ( DWORD Reg );
	void   ResetX86Protection ( void );
	x86Reg UnMap_TempReg      ( void );
	void   UnMap_GPR          ( DWORD Reg, bool WriteBackValue );
	bool   UnMap_X86reg       ( x86Reg Reg );
	void   WriteBackRegisters ( void );

	inline bool IsKnown(int Reg) const   { return ((MipsRegState(Reg) & STATE_KNOWN_VALUE) != 0); }
	inline bool IsUnknown(int Reg) const { return ((MipsRegState(Reg) & STATE_KNOWN_VALUE) == 0); }

	inline bool IsMapped(int Reg) const	{ return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_X86_MAPPED)); }
	inline bool IsConst(int Reg) const	{ return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_X86_MAPPED)) == STATE_KNOWN_VALUE); }

	inline bool IsSigned(int Reg) const	{ return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_SIGN)) == (STATE_KNOWN_VALUE | STATE_SIGN)); }
	inline bool IsUnsigned(int Reg) const { return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_SIGN)) == STATE_KNOWN_VALUE); }

	inline bool Is32Bit(int Reg) const	{ return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT)) == (STATE_KNOWN_VALUE | STATE_32BIT)); }
	inline bool Is64Bit(int Reg) const	{ return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT)) == STATE_KNOWN_VALUE); }

	inline bool Is32BitMapped(int Reg) const { return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)); }
	inline bool Is64BitMapped(int Reg) const { return ((MipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_X86_MAPPED)); }

	inline _int64            cMipsReg_S    ( int Reg ) const { return MIPS_RegVal[Reg].DW; }
	inline DWORD             cMipsRegLo    ( int Reg ) const { return MIPS_RegVal[Reg].UW[0]; }
	inline long              cMipsRegLo_S  ( int Reg ) const { return MIPS_RegVal[Reg].W[0]; }
	inline DWORD             cMipsRegHi    ( int Reg ) const { return MIPS_RegVal[Reg].UW[1]; }
	inline long              cMipsRegHi_S  ( int Reg ) const { return MIPS_RegVal[Reg].W[1]; }

	inline REG_STATE         MipsRegState ( int Reg ) const { return MIPS_RegState[Reg]; }
	inline unsigned _int64   MipsReg      ( int Reg ) const { return MIPS_RegVal[Reg].UDW; }
	inline _int64 &          MipsReg_S    ( int Reg ) { return MIPS_RegVal[Reg].DW; }
	inline DWORD &           MipsRegLo    ( int Reg ) { return MIPS_RegVal[Reg].UW[0]; }
	inline long &            MipsRegLo_S  ( int Reg ) { return MIPS_RegVal[Reg].W[0]; }
	inline DWORD &           MipsRegHi    ( int Reg ) { return MIPS_RegVal[Reg].UW[1]; }
	inline long &            MipsRegHi_S  ( int Reg ) { return MIPS_RegVal[Reg].W[1]; }
	inline CX86Ops::x86Reg   MipsRegMapLo ( int Reg ) const { return RegMapLo[Reg]; }
	inline CX86Ops::x86Reg   MipsRegMapHi ( int Reg ) const { return RegMapHi[Reg]; }
	inline bool              X86Protected ( x86Reg Reg ) const { return x86reg_Protected[Reg]; }

	inline DWORD             GetX86MapOrder  ( x86Reg Reg ) const { return x86reg_MapOrder[Reg]; }
	inline bool              GetX86Protected ( x86Reg Reg )	const { return x86reg_Protected[Reg]; }
	inline REG_MAPPED        GetX86Mapped    ( x86Reg Reg )	const { return x86reg_MappedTo[Reg]; }

	inline DWORD             GetBlockCycleCount ( void ) const { return m_CycleCount; }

	inline void              SetMipsReg      ( int Reg, unsigned __int64 value ) { MIPS_RegVal[Reg].UDW = value; }
	inline void              SetMipsRegMapLo ( int MipsReg, x86Reg Reg ) 
	{
		RegMapLo[MipsReg] = Reg;
	}
	inline void              SetMipsRegMapHi ( int MipsReg, x86Reg Reg )
	{
		RegMapHi[MipsReg] = Reg; 
	}
	inline void              SetMipsRegState ( int MipsReg, REG_STATE State ) { MIPS_RegState[MipsReg] = State; }

	inline void              SetX86MapOrder  ( x86Reg Reg, DWORD Order )    { x86reg_MapOrder[Reg] = Order; }
	inline void              SetX86Protected ( x86Reg Reg, bool Protected )	{ x86reg_Protected[Reg] = Protected; }
	inline void              SetX86Mapped    ( x86Reg Reg, REG_MAPPED Mapping )	{ x86reg_MappedTo[Reg] = Mapping; }


	inline void  SetBlockCycleCount ( DWORD CyleCount ) { m_CycleCount = CyleCount; }

	inline int & StackTopPos ( void ) { return Stack_TopPos; }
	inline int & FpuMappedTo( int Reg) { return x86fpu_MappedTo[Reg]; }
	inline FPU_STATE & FpuState(int Reg) { return x86fpu_State[Reg]; }
	inline FPU_ROUND & FpuRoundingModel(int Reg) { return x86fpu_RoundingModel[Reg]; }
	inline bool & FpuBeenUsed (void )	{ return Fpu_Used; }
	
	inline FPU_ROUND GetRoundingModel ( void ) const { return m_RoundingModel; }
	inline void      SetRoundingModel ( FPU_ROUND RoundingModel ) { m_RoundingModel = RoundingModel; }

private:
	x86Reg UnMap_8BitTempReg ( void );

	//r4k
	REG_STATE   MIPS_RegState[32];
	MIPS_DWORD	MIPS_RegVal[32];
	x86Reg	    RegMapHi[32];
	x86Reg	    RegMapLo[32];

	REG_MAPPED	x86reg_MappedTo[10];
	DWORD		x86reg_MapOrder[10];
	bool		x86reg_Protected[10];
	
	DWORD		m_CycleCount;

	//FPU
	int			Stack_TopPos;
	int			x86fpu_MappedTo[8];
	FPU_STATE	x86fpu_State[8];
	BOOL		x86fpu_StateChanged[8];
	FPU_ROUND	x86fpu_RoundingModel[8];
	
	bool        Fpu_Used;
	FPU_ROUND   m_RoundingModel;
	bool        m_b32bitCore;
	
	bool        compare(const CRegInfo& right) const;
	const char * RoundingModelName ( FPU_ROUND RoundType );

	static unsigned int m_fpuControl;
};
