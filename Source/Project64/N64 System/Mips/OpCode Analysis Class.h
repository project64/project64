class COpcodeAnalysis :
	private CRegistersName
{
	OPCODE &m_opcode;
	CMipsMemory   * const _MMU;
	CNotification & _Notify;
	
public:
	COpcodeAnalysis(CMipsMemory * MMU, OPCODE &opcode);

	//Functions dealing with the name of the opcode
	const char * OpcodeName  ( void );
	stdstr FullName    ( bool * MultipleOps );
	void   OpcodeParam ( char * CommandName );
	stdstr Name        ( void );

	//Analysising the opcode
	bool  DelaySlotEffectsCompare ( DWORD Reg1, DWORD Reg2 );
	bool  DelaySlotEffectsJump    ( void );
	bool  HasDelaySlot            ( void );
	bool  IsCop1Instruction       ( void );

	//Information about the jump
	bool  HardJump                ( void );  //Jump to a fixed address with out remembering current location
	bool  LinkedJump              ( void );  //Stores the Next opcode in GPR[31]
	bool  RelativeJump            ( void );  //Target relative to current PC (eg BNE, BEQ)
	bool  LikelyJump              ( void );  //Misses Delay slot on jumping
	bool  NonConditionalJump      ( void );  //will alway jump because of registers (eg BEQ r0, r0)
	DWORD JumpLocation            ( void );  //Target PC (uses m_opcode.VirtualAddress)

	//Stops execution
	bool  TerminateExecution      ( void );
};