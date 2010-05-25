class R4300iOp
{
public:
	typedef void ( * Func )();

	/************************* OpCode functions *************************/
	static void  J              ( void );
	static void  JAL            ( void );
	static void  BNE            ( void );
	static void  BEQ            ( void );
	static void  BLEZ           ( void );
	static void  BGTZ           ( void );
	static void  ADDI           ( void );
	static void  ADDIU          ( void );
	static void  SLTI           ( void );
	static void  SLTIU          ( void );
	static void  ANDI           ( void );
	static void  ORI            ( void );
	static void  XORI           ( void );
	static void  LUI            ( void );
	static void  BEQL           ( void );
	static void  BNEL           ( void );
	static void  BLEZL          ( void );
	static void  BGTZL          ( void );
	static void  DADDIU         ( void );
	static void  LDL            ( void );
	static void  LDR            ( void );
	static void  LB             ( void );
	static void  LH             ( void );
	static void  LWL            ( void );
	static void  LW             ( void );
	static void  LBU            ( void );
	static void  LHU            ( void );
	static void  LWR            ( void );
	static void  LWU            ( void );
	static void  SB             ( void );
	static void  SH             ( void );
	static void  SWL            ( void );
	static void  SW             ( void );
	static void  SDL            ( void );
	static void  SDR            ( void );
	static void  SWR            ( void );
	static void  CACHE          ( void );
	static void  LL             ( void );
	static void  LWC1           ( void );
	static void  LDC1           ( void );
	static void  LD             ( void );
	static void  SC             ( void );
	static void  SWC1           ( void );
	static void  SDC1           ( void );
	static void  SD             ( void );

	/********************** R4300i OpCodes: Special **********************/
	static void  SPECIAL_SLL    ( void );
	static void  SPECIAL_SRL    ( void );
	static void  SPECIAL_SRA    ( void );
	static void  SPECIAL_SLLV   ( void );
	static void  SPECIAL_SRLV   ( void );
	static void  SPECIAL_SRAV   ( void );
	static void  SPECIAL_JR     ( void );
	static void  SPECIAL_JALR   ( void );
	static void  SPECIAL_SYSCALL ( void );
	static void  SPECIAL_BREAK   ( void );
	static void  SPECIAL_SYNC    ( void );
	static void  SPECIAL_MFHI    ( void );
	static void  SPECIAL_MTHI    ( void );
	static void  SPECIAL_MFLO   ( void );
	static void  SPECIAL_MTLO   ( void );
	static void  SPECIAL_DSLLV  ( void );
	static void  SPECIAL_DSRLV  ( void );
	static void  SPECIAL_DSRAV  ( void );
	static void  SPECIAL_MULT   ( void );
	static void  SPECIAL_MULTU  ( void );
	static void  SPECIAL_DIV    ( void );
	static void  SPECIAL_DIVU   ( void );
	static void  SPECIAL_DMULT  ( void );
	static void  SPECIAL_DMULTU ( void );
	static void  SPECIAL_DDIV   ( void );
	static void  SPECIAL_DDIVU  ( void );
	static void  SPECIAL_ADD    ( void );
	static void  SPECIAL_ADDU   ( void );
	static void  SPECIAL_SUB    ( void );
	static void  SPECIAL_SUBU   ( void );
	static void  SPECIAL_AND    ( void );
	static void  SPECIAL_OR     ( void );
	static void  SPECIAL_XOR    ( void );
	static void  SPECIAL_NOR    ( void );
	static void  SPECIAL_SLT    ( void );
	static void  SPECIAL_SLTU   ( void );
	static void  SPECIAL_DADD   ( void );
	static void  SPECIAL_DADDU  ( void );
	static void  SPECIAL_DSUB   ( void );
	static void  SPECIAL_DSUBU  ( void );
	static void  SPECIAL_TEQ    ( void );
	static void  SPECIAL_DSLL   ( void );
	static void  SPECIAL_DSRL   ( void );
	static void  SPECIAL_DSRA   ( void );
	static void  SPECIAL_DSLL32 ( void );
	static void  SPECIAL_DSRL32 ( void );
	static void  SPECIAL_DSRA32 ( void );

	/********************** R4300i OpCodes: RegImm **********************/
	static void  REGIMM_BLTZ    ( void );
	static void  REGIMM_BGEZ    ( void );
	static void  REGIMM_BLTZL   ( void );
	static void  REGIMM_BGEZL   ( void );
	static void  REGIMM_BLTZAL  ( void );
	static void  REGIMM_BGEZAL  ( void );

	/************************** COP0 functions **************************/
	static void  COP0_MF        ( void );
	static void  COP0_MT        ( void );

	/************************** COP0 CO functions ***********************/
	static void  COP0_CO_TLBR   ( void );
	static void  COP0_CO_TLBWI  ( void );
	static void  COP0_CO_TLBWR  ( void );
	static void  COP0_CO_TLBP   ( void );
	static void  COP0_CO_ERET   ( void );

	/************************** COP1 functions **************************/
	static void  COP1_MF        ( void );
	static void  COP1_DMF       ( void );
	static void  COP1_CF        ( void );
	static void  COP1_MT        ( void );
	static void  COP1_DMT       ( void );
	static void  COP1_CT        ( void );

	/************************* COP1: BC1 functions ***********************/
	static void  COP1_BCF       ( void );
	static void  COP1_BCT       ( void );
	static void  COP1_BCFL      ( void );
	static void  COP1_BCTL      ( void );

	/************************** COP1: S functions ************************/
	static void  COP1_S_ADD     ( void );
	static void  COP1_S_SUB     ( void );
	static void  COP1_S_MUL     ( void );
	static void  COP1_S_DIV     ( void );
	static void  COP1_S_SQRT    ( void );
	static void  COP1_S_ABS     ( void );
	static void  COP1_S_MOV     ( void );
	static void  COP1_S_NEG     ( void );
	static void  COP1_S_TRUNC_L ( void );
	static void  COP1_S_CEIL_L  ( void );	//added by Witten
	static void  COP1_S_FLOOR_L ( void );	//added by Witten
	static void  COP1_S_ROUND_W ( void );
	static void  COP1_S_TRUNC_W ( void );
	static void  COP1_S_CEIL_W  ( void );	//added by Witten
	static void  COP1_S_FLOOR_W ( void );
	static void  COP1_S_CVT_D   ( void );
	static void  COP1_S_CVT_W   ( void );
	static void  COP1_S_CVT_L   ( void );
	static void  COP1_S_CMP     ( void );

	/************************** COP1: D functions ************************/
	static void  COP1_D_ADD     ( void );
	static void  COP1_D_SUB     ( void );
	static void  COP1_D_MUL     ( void );
	static void  COP1_D_DIV     ( void );
	static void  COP1_D_SQRT    ( void );
	static void  COP1_D_ABS     ( void );
	static void  COP1_D_MOV     ( void );
	static void  COP1_D_NEG     ( void );
	static void  COP1_D_TRUNC_L ( void );	//added by Witten	
	static void  COP1_D_CEIL_L  ( void );	//added by Witten
	static void  COP1_D_FLOOR_L ( void );	//added by Witten
	static void  COP1_D_ROUND_W ( void );
	static void  COP1_D_TRUNC_W ( void );
	static void  COP1_D_CEIL_W  ( void );	//added by Witten
	static void  COP1_D_FLOOR_W ( void );	//added by Witten
	static void  COP1_D_CVT_S   ( void );
	static void  COP1_D_CVT_W   ( void );
	static void  COP1_D_CVT_L   ( void );
	static void  COP1_D_CMP     ( void );

	/************************** COP1: W functions ************************/
	static void  COP1_W_CVT_S   ( void );
	static void  COP1_W_CVT_D   ( void );

	/************************** COP1: L functions ************************/
	static void  COP1_L_CVT_S   ( void );
	static void  COP1_L_CVT_D   ( void );

	/************************** Other functions **************************/	
	static void  UnknownOpcode ( void );


	static Func * BuildInterpreter (void );

	static BOOL        m_TestTimer;
	static DWORD       m_NextInstruction;
	static OPCODE      m_Opcode;
	static DWORD       m_JumpToLocation;

protected:
	static void  SPECIAL (void);
	static void  REGIMM  (void);
	static void  COP0    (void);
	static void  COP0_CO (void);
	static void  COP1    (void);
	static void  COP1_BC (void);
	static void  COP1_S  (void);
	static void  COP1_D  (void);
	static void  COP1_W  (void);
	static void  COP1_L  (void);

	static Func Jump_Opcode[64];
	static Func Jump_Special[64];
	static Func Jump_Regimm[32];
	static Func Jump_CoP0[32];
	static Func Jump_CoP0_Function[64];
	static Func Jump_CoP1[32];
	static Func Jump_CoP1_BC[32];
	static Func Jump_CoP1_S[64];
	static Func Jump_CoP1_D[64];
	static Func Jump_CoP1_W[64];
	static Func Jump_CoP1_L[64];

	static const DWORD SWL_MASK[4],  SWR_MASK[4],  LWL_MASK[4],  LWR_MASK[4];
	static const int   SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];

};

#ifdef __cplusplus
extern "C" {
#endif

extern int RoundingModel;


#ifdef __cplusplus
}
#endif
