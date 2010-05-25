class CRecompilerOps
{
	CRecompilerOps();

protected:
	enum BRANCH_TYPE
	{
		BranchTypeCop1, BranchTypeRs, BranchTypeRsRt
	};

	typedef void (CRecompilerOps::* BranchFunction) (void);
	
	CRecompilerOps( CCodeSection * Section );

	/************************** Branch functions  ************************/
	void Compile_Branch         ( BranchFunction CompareFunc, BRANCH_TYPE BranchType, BOOL Link);
	void Compile_BranchLikely   ( BranchFunction CompareFunc, BOOL Link);
	void BNE_Compare            ( void );
	void BEQ_Compare            ( void );
	void BGTZ_Compare           ( void );
	void BLEZ_Compare           ( void );
	void BLTZ_Compare           ( void );
	void BGEZ_Compare           ( void );
	void COP1_BCF_Compare       ( void );
	void COP1_BCT_Compare       ( void );

	/*************************  OpCode functions *************************/
	void Compile_J              ( void );
	void Compile_JAL            ( void );
	void Compile_ADDI           ( void );
	void Compile_ADDIU          ( void );
	void Compile_SLTI           ( void );
	void Compile_SLTIU          ( void );
	void Compile_ANDI           ( void );
	void Compile_ORI            ( void );
	void Compile_XORI           ( void );
	void Compile_LUI            ( void );
	void Compile_DADDIU         ( void );
	void Compile_LDL            ( void );
	void Compile_LDR            ( void );
	void Compile_LB             ( void );
	void Compile_LH             ( void );
	void Compile_LWL            ( void );
	void Compile_LW             ( void );
	void Compile_LBU            ( void );
	void Compile_LHU            ( void );
	void Compile_LWR            ( void );
	void Compile_LWU            ( void );		//added by Witten
	void Compile_SB             ( void );
	void Compile_SH             ( void );
	void Compile_SWL            ( void );
	void Compile_SW             ( void );
	void Compile_SWR            ( void );
	void Compile_SDL            ( void );
	void Compile_SDR            ( void );
	void Compile_CACHE          ( void );
	void Compile_LL             ( void );
	void Compile_LWC1           ( void );
	void Compile_LDC1           ( void );
	void Compile_LD             ( void );
	void Compile_SC             ( void );
	void Compile_SWC1           ( void );
	void Compile_SDC1           ( void );
	void Compile_SD             ( void );

	/********************** R4300i OpCodes: Special **********************/
	void Compile_SPECIAL_SLL    ( void );
	void Compile_SPECIAL_SRL    ( void );
	void Compile_SPECIAL_SRA    ( void );
	void Compile_SPECIAL_SLLV   ( void );
	void Compile_SPECIAL_SRLV   ( void );
	void Compile_SPECIAL_SRAV   ( void );
	void Compile_SPECIAL_JR     ( void );
	void Compile_SPECIAL_JALR   ( void );
	void Compile_SPECIAL_SYSCALL( void );
	void Compile_SPECIAL_MFLO   ( void );
	void Compile_SPECIAL_MTLO   ( void );
	void Compile_SPECIAL_MFHI   ( void );
	void Compile_SPECIAL_MTHI   ( void );
	void Compile_SPECIAL_DSLLV  ( void );
	void Compile_SPECIAL_DSRLV  ( void );
	void Compile_SPECIAL_DSRAV  ( void );
	void Compile_SPECIAL_MULT   ( void );
	void Compile_SPECIAL_MULTU  ( void );
	void Compile_SPECIAL_DIV    ( void );
	void Compile_SPECIAL_DIVU   ( void );
	void Compile_SPECIAL_DMULT  ( void );
	void Compile_SPECIAL_DMULTU ( void );
	void Compile_SPECIAL_DDIV   ( void );
	void Compile_SPECIAL_DDIVU  ( void );
	void Compile_SPECIAL_ADD    ( void );
	void Compile_SPECIAL_ADDU   ( void );
	void Compile_SPECIAL_SUB    ( void );
	void Compile_SPECIAL_SUBU   ( void );
	void Compile_SPECIAL_AND    ( void );
	void Compile_SPECIAL_OR     ( void );
	void Compile_SPECIAL_XOR    ( void );
	void Compile_SPECIAL_NOR    ( void );
	void Compile_SPECIAL_SLT    ( void );
	void Compile_SPECIAL_SLTU   ( void );
	void Compile_SPECIAL_DADD   ( void );
	void Compile_SPECIAL_DADDU  ( void );
	void Compile_SPECIAL_DSUB   ( void );
	void Compile_SPECIAL_DSUBU  ( void );
	void Compile_SPECIAL_DSLL   ( void );
	void Compile_SPECIAL_DSRL   ( void );
	void Compile_SPECIAL_DSRA   ( void );
	void Compile_SPECIAL_DSLL32 ( void );
	void Compile_SPECIAL_DSRL32 ( void );
	void Compile_SPECIAL_DSRA32 ( void );

	/************************** COP0 functions **************************/
	void Compile_COP0_MF        ( void );
	void Compile_COP0_MT        ( void );

	/************************** COP0 CO functions ***********************/
	void Compile_COP0_CO_TLBR   ( void );
	void Compile_COP0_CO_TLBWI  ( void );
	void Compile_COP0_CO_TLBWR  ( void );
	void Compile_COP0_CO_TLBP   ( void );
	void Compile_COP0_CO_ERET   ( void );

	/************************** COP1 functions **************************/
	void Compile_COP1_MF        ( void );
	void Compile_COP1_DMF       ( void );
	void Compile_COP1_CF        ( void );
	void Compile_COP1_MT        ( void );
	void Compile_COP1_DMT       ( void );
	void Compile_COP1_CT        ( void );

	/************************** COP1: S functions ************************/
	void Compile_COP1_S_ADD     ( void );
	void Compile_COP1_S_SUB     ( void );
	void Compile_COP1_S_MUL     ( void );
	void Compile_COP1_S_DIV     ( void );
	void Compile_COP1_S_ABS     ( void );
	void Compile_COP1_S_NEG     ( void );
	void Compile_COP1_S_SQRT    ( void );
	void Compile_COP1_S_MOV     ( void );
	void Compile_COP1_S_TRUNC_L ( void );
	void Compile_COP1_S_CEIL_L  ( void );			//added by Witten
	void Compile_COP1_S_FLOOR_L ( void );			//added by Witten
	void Compile_COP1_S_ROUND_W ( void );
	void Compile_COP1_S_TRUNC_W ( void );
	void Compile_COP1_S_CEIL_W  ( void );			//added by Witten
	void Compile_COP1_S_FLOOR_W ( void );
	void Compile_COP1_S_CVT_D   ( void );
	void Compile_COP1_S_CVT_W   ( void );
	void Compile_COP1_S_CVT_L   ( void );
	void Compile_COP1_S_CMP     ( void );

	/************************** COP1: D functions ************************/
	void Compile_COP1_D_ADD     ( void );
	void Compile_COP1_D_SUB     ( void );
	void Compile_COP1_D_MUL     ( void );
	void Compile_COP1_D_DIV     ( void );
	void Compile_COP1_D_ABS     ( void );
	void Compile_COP1_D_NEG     ( void );
	void Compile_COP1_D_SQRT    ( void );
	void Compile_COP1_D_MOV     ( void );
	void Compile_COP1_D_TRUNC_L ( void );			//added by Witten
	void Compile_COP1_D_CEIL_L  ( void );			//added by Witten
	void Compile_COP1_D_FLOOR_L ( void );			//added by Witten
	void Compile_COP1_D_ROUND_W ( void );
	void Compile_COP1_D_TRUNC_W ( void );
	void Compile_COP1_D_CEIL_W  ( void );			//added by Witten
	void Compile_COP1_D_FLOOR_W ( void );			//added by Witten
	void Compile_COP1_D_CVT_S   ( void );
	void Compile_COP1_D_CVT_W   ( void );
	void Compile_COP1_D_CVT_L   ( void );
	void Compile_COP1_D_CMP     ( void );

	/************************** COP1: W functions ************************/
	void Compile_COP1_W_CVT_S   ( void );
	void Compile_COP1_W_CVT_D   ( void );

	/************************** COP1: L functions ************************/
	void Compile_COP1_L_CVT_S   ( void );
	void Compile_COP1_L_CVT_D   ( void );

	/************************** Other functions **************************/
	void Compile_UnknownOpcode  ( void );
	void CompileReadTLBMiss  (CCodeSection * Section, int AddressReg, int LookUpReg );
	void CompileWriteTLBMiss (CCodeSection * Section, int AddressReg, int LookUpReg );

private:
	CCodeSection * m_Section;
};