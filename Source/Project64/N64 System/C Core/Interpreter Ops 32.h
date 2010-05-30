typedef void (_fastcall * R4300iOp_FUNC )();

class R4300iOp32
{
public:
	/************************* OpCode functions *************************/
	static void _fastcall J              ( void );
	static void _fastcall JAL            ( void );
	static void _fastcall BNE            ( void );
	static void _fastcall BEQ            ( void );
	static void _fastcall BLEZ           ( void );
	static void _fastcall BGTZ           ( void );
	static void _fastcall ADDI           ( void );
	static void _fastcall ADDIU          ( void );
	static void _fastcall SLTI           ( void );
	static void _fastcall SLTIU          ( void );
	static void _fastcall ANDI           ( void );
	static void _fastcall ORI            ( void );
	static void _fastcall XORI           ( void );
	static void _fastcall LUI            ( void );
	static void _fastcall BEQL           ( void );
	static void _fastcall BNEL           ( void );
	static void _fastcall BLEZL          ( void );
	static void _fastcall BGTZL          ( void );
	static void _fastcall LB             ( void );
	static void _fastcall LH             ( void );
	static void _fastcall LWL            ( void );
	static void _fastcall LW             ( void );
	static void _fastcall LW_LOG         ( void );
	static void _fastcall LBU            ( void );
	static void _fastcall LHU            ( void );
	static void _fastcall LWR            ( void );
	static void _fastcall LWU            ( void );
	static void _fastcall SB             ( void );
	static void _fastcall SH             ( void );
	static void _fastcall SWL            ( void );
	static void _fastcall SW             ( void );
	static void _fastcall SWR            ( void );
	static void _fastcall CACHE          ( void );
	static void _fastcall LL             ( void );
	static void _fastcall LWC1           ( void );
	static void _fastcall SC             ( void );
	static void _fastcall SWC1           ( void );

	/********************** R4300i OpCodes: Special **********************/
	static void _fastcall SPECIAL_SLL    ( void );
	static void _fastcall SPECIAL_SRL    ( void );
	static void _fastcall SPECIAL_SRA    ( void );
	static void _fastcall SPECIAL_SLLV   ( void );
	static void _fastcall SPECIAL_SRLV   ( void );
	static void _fastcall SPECIAL_SRAV   ( void );
	static void _fastcall SPECIAL_JR     ( void );
	static void _fastcall SPECIAL_JALR   ( void );
	static void _fastcall SPECIAL_SYSCALL ( void );
	static void _fastcall SPECIAL_BREAK   ( void );
	static void _fastcall SPECIAL_SYNC    ( void );
	static void _fastcall SPECIAL_MTHI    ( void );
	static void _fastcall SPECIAL_MTLO   ( void );
	static void _fastcall SPECIAL_ADD    ( void );
	static void _fastcall SPECIAL_ADDU   ( void );
	static void _fastcall SPECIAL_SUB    ( void );
	static void _fastcall SPECIAL_SUBU   ( void );
	static void _fastcall SPECIAL_AND    ( void );
	static void _fastcall SPECIAL_OR     ( void );
	static void _fastcall SPECIAL_XOR    ( void );
	static void _fastcall SPECIAL_NOR    ( void );
	static void _fastcall SPECIAL_SLT    ( void );
	static void _fastcall SPECIAL_SLTU   ( void );
	static void _fastcall SPECIAL_TEQ    ( void );

	/********************** R4300i OpCodes: RegImm **********************/
	static void _fastcall REGIMM_BLTZ    ( void );
	static void _fastcall REGIMM_BGEZ    ( void );
	static void _fastcall REGIMM_BLTZL   ( void );
	static void _fastcall REGIMM_BGEZL   ( void );
	static void _fastcall REGIMM_BLTZAL  ( void );
	static void _fastcall REGIMM_BGEZAL  ( void );

	/************************** COP0 functions **************************/
	static void _fastcall COP0_MF        ( void );
	static void _fastcall COP0_MT        ( void );

	/************************** COP0 CO functions ***********************/
	static void _fastcall COP0_CO_TLBR   ( void );
	static void _fastcall COP0_CO_TLBWI  ( void );
	static void _fastcall COP0_CO_TLBWR  ( void );
	static void _fastcall COP0_CO_TLBP   ( void );
	static void _fastcall COP0_CO_ERET   ( void );

	/************************** COP1 functions **************************/
	static void _fastcall COP1_MF        ( void );
	static void _fastcall COP1_DMF       ( void );
	static void _fastcall COP1_CF        ( void );
	static void _fastcall COP1_MT        ( void );
	static void _fastcall COP1_DMT       ( void );
	static void _fastcall COP1_CT        ( void );

	/************************* COP1: BC1 functions ***********************/
	static void _fastcall COP1_BCF       ( void );
	static void _fastcall COP1_BCT       ( void );
	static void _fastcall COP1_BCFL      ( void );
	static void _fastcall COP1_BCTL      ( void );

	/************************** COP1: S functions ************************/
	static void _fastcall COP1_S_ADD     ( void );
	static void _fastcall COP1_S_SUB     ( void );
	static void _fastcall COP1_S_MUL     ( void );
	static void _fastcall COP1_S_DIV     ( void );
	static void _fastcall COP1_S_SQRT    ( void );
	static void _fastcall COP1_S_ABS     ( void );
	static void _fastcall COP1_S_MOV     ( void );
	static void _fastcall COP1_S_NEG     ( void );
	static void _fastcall COP1_S_TRUNC_L ( void );
	static void _fastcall COP1_S_CEIL_L  ( void );	//added by Witten
	static void _fastcall COP1_S_FLOOR_L ( void );	//added by Witten
	static void _fastcall COP1_S_ROUND_W ( void );
	static void _fastcall COP1_S_TRUNC_W ( void );
	static void _fastcall COP1_S_CEIL_W  ( void );	//added by Witten
	static void _fastcall COP1_S_FLOOR_W ( void );
	static void _fastcall COP1_S_CVT_D   ( void );
	static void _fastcall COP1_S_CVT_W   ( void );
	static void _fastcall COP1_S_CVT_L   ( void );
	static void _fastcall COP1_S_CMP     ( void );

	/************************** COP1: D functions ************************/
	static void _fastcall COP1_D_ADD     ( void );
	static void _fastcall COP1_D_SUB     ( void );
	static void _fastcall COP1_D_MUL     ( void );
	static void _fastcall COP1_D_DIV     ( void );
	static void _fastcall COP1_D_SQRT    ( void );
	static void _fastcall COP1_D_ABS     ( void );
	static void _fastcall COP1_D_MOV     ( void );
	static void _fastcall COP1_D_NEG     ( void );
	static void _fastcall COP1_D_TRUNC_L ( void );	//added by Witten	
	static void _fastcall COP1_D_CEIL_L  ( void );	//added by Witten
	static void _fastcall COP1_D_FLOOR_L ( void );	//added by Witten
	static void _fastcall COP1_D_ROUND_W ( void );
	static void _fastcall COP1_D_TRUNC_W ( void );
	static void _fastcall COP1_D_CEIL_W  ( void );	//added by Witten
	static void _fastcall COP1_D_FLOOR_W ( void );	//added by Witten
	static void _fastcall COP1_D_CVT_S   ( void );
	static void _fastcall COP1_D_CVT_W   ( void );
	static void _fastcall COP1_D_CVT_L   ( void );
	static void _fastcall COP1_D_CMP     ( void );

	/************************** COP1: W functions ************************/
	static void _fastcall COP1_W_CVT_S   ( void );
	static void _fastcall COP1_W_CVT_D   ( void );

	/************************** COP1: L functions ************************/
	static void _fastcall COP1_L_CVT_S   ( void );
	static void _fastcall COP1_L_CVT_D   ( void );

	/************************** Other functions **************************/	
	static void _fastcall UnknownOpcode ( void );


	static R4300iOp_FUNC * BuildInterpreter (void );

private:
	static void _fastcall SPECIAL (void);
	static void _fastcall REGIMM  (void);
	static void _fastcall COP0    (void);
	static void _fastcall COP0_CO (void);
	static void _fastcall COP1    (void);
	static void _fastcall COP1_BC (void);
	static void _fastcall COP1_S  (void);
	static void _fastcall COP1_D  (void);
	static void _fastcall COP1_W  (void);
	static void _fastcall COP1_L  (void);

	static R4300iOp_FUNC Jump_Opcode[64];
	static R4300iOp_FUNC Jump_Special[64];
	static R4300iOp_FUNC Jump_Regimm[32];
	static R4300iOp_FUNC Jump_CoP0[32];
	static R4300iOp_FUNC Jump_CoP0_Function[64];
	static R4300iOp_FUNC Jump_CoP1[32];
	static R4300iOp_FUNC Jump_CoP1_BC[32];
	static R4300iOp_FUNC Jump_CoP1_S[64];
	static R4300iOp_FUNC Jump_CoP1_D[64];
	static R4300iOp_FUNC Jump_CoP1_W[64];
	static R4300iOp_FUNC Jump_CoP1_L[64];
};
