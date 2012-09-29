class CX86Ops
{
public:
	enum x86Reg {
		x86_EAX     = 0, 
		x86_EBX     = 3, 
		x86_ECX     = 1, 
		x86_EDX     = 2,
		x86_ESI     = 6, 
		x86_EDI     = 7, 
		x86_EBP     = 5, 
		x86_ESP     = 4,
		x86_Any8Bit = -3,
		x86_Any     = -2,
		x86_Unknown = -1,
		
		x86_AL  = 0, x86_BL  = 3, x86_CL  = 1, x86_DL  = 2,
		x86_AH  = 4, x86_BH  = 7, x86_CH  = 5, x86_DH  = 6
	};

	enum x86FpuValues {
		x86_ST_Unknown = -1,
		x86_ST0 = 0,
		x86_ST1 = 1,
		x86_ST2 = 2,
		x86_ST3 = 3,
		x86_ST4 = 4,
		x86_ST5 = 5,
		x86_ST6 = 6,
		x86_ST7 = 7
	};

	enum Multipler 
	{
		Multip_x1 = 1, 
		Multip_x2 = 2,
		Multip_x4 = 4,
		Multip_x8 = 8
	};

	static x86Reg x86_Registers[8];
	static const char * x86_Name     ( x86Reg Reg );
	static const char * x86_ByteName ( x86Reg Reg );
	static const char * x86_HalfName ( x86Reg Reg );
	static const char * fpu_Name     ( x86FpuValues Reg );

protected:
	//Logging Functions
	static void WriteX86Comment ( LPCSTR Comment );
	static void WriteX86Label   ( LPCSTR Label );

	static void AdcX86regToVariable             ( x86Reg reg, void * Variable, const char * VariableName );
	static void AdcConstToVariable              ( void *Variable, const char * VariableName, BYTE Constant );
	static void AdcConstToX86Reg                ( x86Reg Reg, DWORD Const );
	static void AdcVariableToX86reg             ( x86Reg reg, void * Variable, const char * VariableName );
	static void AdcX86RegToX86Reg               ( x86Reg Destination, x86Reg Source );
	static void AddConstToVariable              ( DWORD Const, void *Variable, const char * VariableName );
	static void AddConstToX86Reg                ( x86Reg Reg, DWORD Const );
	static void AddVariableToX86reg             ( x86Reg reg, void * Variable, const char * VariableName );
	static void AddX86regToVariable             ( x86Reg reg, void * Variable, const char * VariableName );
	static void AddX86RegToX86Reg               ( x86Reg Destination, x86Reg Source );
	static void AndConstToVariable              ( DWORD Const, void *Variable, const char * VariableName );
	static void AndConstToX86Reg                ( x86Reg Reg, DWORD Const );
	static void AndVariableToX86Reg             ( void * Variable, const char * VariableName, x86Reg Reg );
	static void AndVariableDispToX86Reg         ( void * Variable, const char * VariableName, x86Reg Reg, x86Reg AddrReg, Multipler Multiply);
	static void AndX86RegToX86Reg               ( x86Reg Destination, x86Reg Source );
	static void X86HardBreakPoint               ( void );
	static void X86BreakPoint                   ( LPCSTR FileName, int LineNumber );
	static void Call_Direct                     ( void * FunctAddress, const char * FunctName );
	static void Call_Indirect                   ( void * FunctAddress, const char * FunctName );
	static void CompConstToVariable             ( DWORD Const, void * Variable, const char * VariableName );
	static void CompConstToX86reg               ( x86Reg Reg, DWORD Const );
	static void CompConstToX86regPointer        ( x86Reg Reg, DWORD Const );
	static void CompX86regToVariable            ( x86Reg Reg, void * Variable, const char * VariableName );
	static void CompVariableToX86reg	        ( x86Reg Reg, void * Variable, const char * VariableName );
	static void CompX86RegToX86Reg              ( x86Reg Destination, x86Reg Source );
	static void DecX86reg                       ( x86Reg Reg );
	static void DivX86reg                       ( x86Reg reg );
	static void idivX86reg                      ( x86Reg reg );
	static void imulX86reg                      ( x86Reg reg );
	static void IncX86reg                       ( x86Reg Reg );
	static void JaeLabel8                       ( const char * Label, BYTE Value );
	static void JaeLabel32                      ( const char * Label, DWORD Value );
	static void JaLabel8                        ( const char * Label, BYTE Value );
	static void JaLabel32                       ( const char * Label, DWORD Value );
	static void JbLabel8                        ( const char * Label, BYTE Value );
	static void JbLabel32                       ( const char * Label, DWORD Value );
	static void JecxzLabel8                     ( const char * Label, BYTE Value );
	static void JeLabel8                        ( const char * Label, BYTE Value );
	static void JeLabel32                       ( const char * Label, DWORD Value );
	static void JgeLabel32                      ( const char * Label, DWORD Value );
	static void JgLabel8                        ( const char * Label, BYTE Value );
	static void JgLabel32                       ( const char * Label, DWORD Value );
	static void JleLabel8                       ( const char * Label, BYTE Value );
	static void JleLabel32                      ( const char * Label, DWORD Value );
	static void JlLabel8                        ( const char * Label, BYTE Value );
	static void JlLabel32                       ( const char * Label, DWORD Value );
	static void JmpDirectReg                    ( x86Reg reg );
	static void JmpIndirectLabel32              ( const char * Label, DWORD location );
	static void JmpIndirectReg                  ( x86Reg reg );
	static void JmpLabel8                       ( const char * Label, BYTE Value );
	static void JmpLabel32                      ( const char * Label, DWORD Value );
	static void JneLabel8                       ( const char * Label, BYTE Value );
	static void JneLabel32                      ( const char * Label, DWORD Value );
	static void JnsLabel8                       ( const char * Label, BYTE Value );
	static void JnsLabel32                      ( const char * Label, DWORD Value );
	static void JnzLabel8                       ( const char * Label, BYTE Value );
	static void JnzLabel32                      ( const char * Label, DWORD Value );
	static void JsLabel32                       ( const char * Label, DWORD Value );
	static void JzLabel8                        ( const char * Label, BYTE Value );
	static void JzLabel32                       ( const char * Label, DWORD Value );
	static void LeaRegReg                       ( x86Reg RegDest, x86Reg RegSrc, DWORD Const, Multipler multiplier );
	static void LeaRegReg2                      ( x86Reg RegDest, x86Reg RegSrc, x86Reg RegSrc2, Multipler multiplier );
	static void LeaSourceAndOffset              ( x86Reg x86DestReg, x86Reg x86SourceReg, int offset );
	static void MoveConstByteToN64Mem           ( BYTE Const, x86Reg AddrReg );
	static void MoveConstHalfToN64Mem           ( WORD Const, x86Reg AddrReg );
	static void MoveConstByteToVariable         ( BYTE Const, void * Variable, const char * VariableName );
	static void MoveConstByteToX86regPointer    ( BYTE Const, x86Reg AddrReg1, x86Reg AddrReg2 );
	static void MoveConstHalfToVariable         ( WORD Const, void * Variable, const char * VariableName );
	static void MoveConstHalfToX86regPointer    ( WORD Const, x86Reg AddrReg1, x86Reg AddrReg2 );
	static void MoveConstToMemoryDisp           ( DWORD Const, x86Reg AddrReg, DWORD Disp );
	static void MoveConstToN64Mem               ( DWORD Const, x86Reg AddrReg );
	static void MoveConstToN64MemDisp           ( DWORD Const, x86Reg AddrReg, BYTE Disp );
	static void MoveConstToVariable             ( DWORD Const, void * Variable, const char * VariableName );
	static void MoveConstToX86Pointer           ( DWORD Const, x86Reg X86Pointer );
	static void MoveConstToX86reg               ( DWORD Const, x86Reg reg );
	static void MoveConstToX86regPointer        ( DWORD Const, x86Reg AddrReg1, x86Reg AddrReg2 );
	static void MoveN64MemDispToX86reg          ( x86Reg reg, x86Reg AddrReg, BYTE Disp );
	static void MoveN64MemToX86reg              ( x86Reg reg, x86Reg AddrReg );
	static void MoveN64MemToX86regByte          ( x86Reg reg, x86Reg AddrReg );
	static void MoveN64MemToX86regHalf          ( x86Reg reg, x86Reg AddrReg );
	static void MoveSxByteX86regPointerToX86reg ( x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg );
	static void MoveSxHalfX86regPointerToX86reg ( x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg );
	static void MoveSxN64MemToX86regByte        ( x86Reg reg, x86Reg AddrReg );
	static void MoveSxN64MemToX86regHalf        ( x86Reg reg, x86Reg AddrReg );
	static void MoveSxVariableToX86regByte      ( void * Variable, const char * VariableName, x86Reg reg );
	static void MoveSxVariableToX86regHalf      ( void * Variable, const char * VariableName, x86Reg reg );
	static void MoveVariableDispToX86Reg        ( void * Variable, const char * VariableName, x86Reg Reg, x86Reg AddrReg, int Multiplier );
	static void MoveVariableToX86reg            ( void * Variable, const char * VariableName, x86Reg reg );
	static void MoveVariableToX86regByte        ( void * Variable, const char * VariableName, x86Reg reg );
	static void MoveVariableToX86regHalf        ( void * Variable, const char * VariableName, x86Reg reg );
	static void MoveX86PointerToX86reg          ( x86Reg reg, x86Reg X86Pointer );
	static void MoveX86PointerToX86regDisp      ( x86Reg reg, x86Reg X86Pointer, BYTE Disp );
	static void MoveX86regByteToN64Mem          ( x86Reg reg, x86Reg AddrReg );
	static void MoveX86regByteToVariable        ( x86Reg reg, void * Variable, const char * VariableName );
	static void MoveX86regByteToX86regPointer   ( x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2 );
	static void MoveX86regHalfToN64Mem          ( x86Reg reg, x86Reg AddrReg );
	static void MoveX86regHalfToVariable        ( x86Reg reg, void * Variable, const char * VariableName );
	static void MoveX86regHalfToX86regPointer   ( x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2 );
	static void MoveX86regPointerToX86reg       ( x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg );
	static void MoveX86regPointerToX86regDisp8  ( x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg, BYTE offset );
	static void MoveX86regToMemory              ( x86Reg reg, x86Reg AddrReg, DWORD Disp );
	static void MoveX86regToN64Mem              ( x86Reg reg, x86Reg AddrReg );
	static void MoveX86regToN64MemDisp          ( x86Reg reg, x86Reg AddrReg, BYTE Disp );
	static void MoveX86regToVariable            ( x86Reg reg, void * Variable, const char * VariableName );
	static void MoveX86RegToX86Reg              ( x86Reg Source, x86Reg Destination );
	static void MoveX86regToX86Pointer          ( x86Reg reg, x86Reg X86Pointer );
	static void MoveX86regToX86regPointer       ( x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2 );
	static void MoveZxByteX86regPointerToX86reg ( x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg );
	static void MoveZxHalfX86regPointerToX86reg ( x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg );
	static void MoveZxN64MemToX86regByte        ( x86Reg reg, x86Reg AddrReg );
	static void MoveZxN64MemToX86regHalf        ( x86Reg reg, x86Reg AddrReg );
	static void MoveZxVariableToX86regByte      ( void * Variable, const char * VariableName, x86Reg reg );
	static void MoveZxVariableToX86regHalf      ( void * Variable, const char * VariableName, x86Reg reg );
	static void MulX86reg                       ( x86Reg reg );
	static void NotX86Reg                       ( x86Reg Reg );
	static void OrConstToVariable               ( DWORD Const, void * Variable, const char * VariableName );
	static void OrConstToX86Reg                 ( DWORD Const, x86Reg reg );
	static void OrVariableToX86Reg              ( void * Variable, const char * VariableName, x86Reg Reg );
	static void OrX86RegToVariable              ( void * Variable, const char * VariableName, x86Reg Reg );
	static void OrX86RegToX86Reg                ( x86Reg Destination, x86Reg Source );
	static void Push					        ( x86Reg reg );
	static void Pushad                          ( void );
	static void PushImm32                       ( DWORD Value );
	static void PushImm32                       ( const char * String, DWORD Value );
	static void Pop					            ( x86Reg reg );
	static void Popad                           ( void );
	static void Ret                             ( void );
	static void Seta                            ( x86Reg reg );
	static void Setae                           ( x86Reg reg );
	static void SetaVariable                    ( void * Variable, const char * VariableName );
	static void Setb                            ( x86Reg reg );
	static void SetbVariable                    ( void * Variable, const char * VariableName );
	static void Setg                            ( x86Reg reg );
	static void SetgVariable                    ( void * Variable, const char * VariableName );
	static void Setl                            ( x86Reg reg );
	static void SetlVariable                    ( void * Variable, const char * VariableName );
	static void Setz					        ( x86Reg reg );
	static void Setnz					        ( x86Reg reg );
	static void ShiftLeftDouble                 ( x86Reg Destination, x86Reg Source );
	static void ShiftLeftDoubleImmed            ( x86Reg Destination, x86Reg Source, BYTE Immediate );
	static void ShiftLeftSign                   ( x86Reg reg );
	static void ShiftLeftSignImmed              ( x86Reg reg, BYTE Immediate );
	static void ShiftRightDouble                ( x86Reg Destination, x86Reg Source );
	static void ShiftRightDoubleImmed           ( x86Reg Destination, x86Reg Source, BYTE Immediate );
	static void ShiftRightSign                  ( x86Reg reg );
	static void ShiftRightSignImmed             ( x86Reg reg, BYTE Immediate );
	static void ShiftRightUnsign                ( x86Reg reg );
	static void ShiftRightUnsignImmed           ( x86Reg reg, BYTE Immediate );
	static void SbbConstFromX86Reg              ( x86Reg Reg, DWORD Const );
	static void SbbVariableFromX86reg           ( x86Reg reg, void * Variable, const char * VariableName );
	static void SbbX86RegToX86Reg               ( x86Reg Destination, x86Reg Source );
	static void SubConstFromVariable            ( DWORD Const, void * Variable, const char * VariableName );
	static void SubConstFromX86Reg              ( x86Reg Reg, DWORD Const );
	static void SubVariableFromX86reg           ( x86Reg reg, void * Variable, const char * VariableName );
	static void SubX86RegToX86Reg               ( x86Reg Destination, x86Reg Source );
	static void TestConstToX86Reg               ( DWORD Const, x86Reg reg );
	static void TestVariable                    ( DWORD Const, void * Variable, const char * VariableName );
	static void TestX86RegToX86Reg              ( x86Reg Destination, x86Reg Source );
	static void XorConstToX86Reg                ( x86Reg Reg, DWORD Const );
	static void XorX86RegToX86Reg               ( x86Reg Source, x86Reg Destination );
	static void XorVariableToX86reg             ( void * Variable, const char * VariableName, x86Reg reg );


	static void fpuAbs					        ( void );
	static void fpuAddDword			            ( void * Variable, const char * VariableName );
	static void fpuAddDwordRegPointer           ( x86Reg x86Pointer );
	static void fpuAddQword			            ( void * Variable, const char * VariableName );
	static void fpuAddQwordRegPointer           ( x86Reg X86Pointer );
	static void fpuAddReg				        ( x86FpuValues reg );
	static void fpuAddRegPop			        ( int * StackPos, x86FpuValues reg );
	static void fpuComDword			            ( void * Variable, const char * VariableName, BOOL Pop );
	static void fpuComDwordRegPointer           ( x86Reg X86Pointer, BOOL Pop );
	static void fpuComQword			            ( void * Variable, const char * VariableName, BOOL Pop );
	static void fpuComQwordRegPointer           ( x86Reg X86Pointer, BOOL Pop );
	static void fpuComReg                       ( x86FpuValues reg, BOOL Pop );
	static void fpuDivDword			            ( void * Variable, const char * VariableName );
	static void fpuDivDwordRegPointer           ( x86Reg X86Pointer );
	static void fpuDivQword			            ( void * Variable, const char * VariableName );
	static void fpuDivQwordRegPointer           ( x86Reg X86Pointer );
	static void fpuDivReg                       ( x86FpuValues Reg );
	static void fpuDivRegPop			        ( x86FpuValues reg );
	static void fpuExchange                     ( x86FpuValues Reg );
	static void fpuFree                         ( x86FpuValues Reg );
	static void fpuDecStack                     ( int * StackPos );
	static void fpuIncStack                     ( int * StackPos );
	static void fpuLoadControl			        ( void * Variable, const char * VariableName );
	static void fpuLoadDword			        ( int * StackPos, void * Variable, const char * VariableName );
	static void fpuLoadDwordFromX86Reg          ( int * StackPos, x86Reg reg );
	static void fpuLoadDwordFromN64Mem          ( int * StackPos, x86Reg reg );
	static void fpuLoadInt32bFromN64Mem         ( int * StackPos, x86Reg reg );
	static void fpuLoadIntegerDword	            ( int * StackPos, void * Variable, const char * VariableName );
	static void fpuLoadIntegerDwordFromX86Reg   ( int * StackPos,x86Reg Reg );
	static void fpuLoadIntegerQword	            ( int * StackPos, void * Variable, const char * VariableName );
	static void fpuLoadIntegerQwordFromX86Reg   ( int * StackPos,x86Reg Reg );
	static void fpuLoadQword			        ( int * StackPos, void * Variable, const char * VariableName );
	static void fpuLoadQwordFromX86Reg          ( int * StackPos, x86Reg Reg );
	static void fpuLoadQwordFromN64Mem          ( int * StackPos, x86Reg reg );
	static void fpuLoadReg                      ( int * StackPos, x86FpuValues Reg );
	static void fpuMulDword                     ( void * Variable, const char * VariableName);
	static void fpuMulDwordRegPointer           ( x86Reg X86Pointer );
	static void fpuMulQword                     ( void * Variable, const char * VariableName);
	static void fpuMulQwordRegPointer           ( x86Reg X86Pointer );
	static void fpuMulReg                       ( x86FpuValues reg );
	static void fpuMulRegPop                    ( x86FpuValues reg );
	static void fpuNeg					        ( void );
	static void fpuRound				        ( void );
	static void fpuSqrt				            ( void );
	static void fpuStoreControl		            ( void * Variable, const char * VariableName );
	static void fpuStoreDword			        ( int * StackPos, void * Variable, const char * VariableName, BOOL pop );
	static void fpuStoreDwordFromX86Reg         ( int * StackPos,x86Reg Reg, BOOL pop );
	static void fpuStoreDwordToN64Mem	        ( int * StackPos, x86Reg reg, BOOL Pop );
	static void fpuStoreIntegerDword            ( int * StackPos, void * Variable, const char * VariableName, BOOL pop );
	static void fpuStoreIntegerDwordFromX86Reg  ( int * StackPos,x86Reg Reg, BOOL pop );
	static void fpuStoreIntegerQword            ( int * StackPos, void * Variable, const char * VariableName, BOOL pop );
	static void fpuStoreIntegerQwordFromX86Reg  ( int * StackPos, x86Reg Reg, BOOL pop );
	static void fpuStoreQword			        ( int * StackPos, void * Variable, const char * VariableName, BOOL pop );
	static void fpuStoreQwordFromX86Reg         ( int * StackPos, x86Reg Reg, BOOL pop );
	static void fpuStoreStatus			        ( void );
	static void fpuSubDword			            ( void * Variable, const char * VariableName );
	static void fpuSubDwordRegPointer           ( x86Reg X86Pointer );
	static void fpuSubDwordReverse              ( void * Variable, const char * VariableName );
	static void fpuSubQword			            ( void * Variable, const char * VariableName );
	static void fpuSubQwordRegPointer           ( x86Reg X86Pointer );
	static void fpuSubQwordReverse              ( void * Variable, const char * VariableName );
	static void fpuSubReg				        ( x86FpuValues reg );
	static void fpuSubRegPop			        ( x86FpuValues reg );	
	
	static BOOL Is8BitReg                       ( x86Reg Reg );
	static BYTE CalcMultiplyCode                ( Multipler Multiply );
	static BYTE * m_RecompPos;

	static void * GetAddressOf(int value, ...);
	static void SetJump32(DWORD * Loc, DWORD * JumpLoc );
	static void SetJump8(BYTE * Loc, BYTE * JumpLoc);

private:
	static void BreakPointNotification (const char * const FileName, const int LineNumber);
	static char m_fpupop[2][2];
};

#define AddressOf(Addr) CX86Ops::GetAddressOf(5,(Addr))
