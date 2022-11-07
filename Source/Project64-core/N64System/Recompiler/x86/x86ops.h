#pragma once
#if defined(__i386__) || defined(_M_IX86)

#if !defined(_MSC_VER) && !defined(_Printf_format_string_)
#define _Printf_format_string_
#endif

class CCodeBlock;

class CX86Ops
{
public:
    enum x86Reg
    {
        x86_EAX = 0,
        x86_EBX = 3,
        x86_ECX = 1,
        x86_EDX = 2,
        x86_ESI = 6,
        x86_EDI = 7,
        x86_EBP = 5,
        x86_ESP = 4,
        x86_Unknown = -1,

        x86_AL = 0,
        x86_BL = 3,
        x86_CL = 1,
        x86_DL = 2,
        x86_AH = 4,
        x86_BH = 7,
        x86_CH = 5,
        x86_DH = 6
    };

    enum x86FpuValues
    {
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

    static const char * x86_Name(x86Reg Reg);
    static const char * x86_ByteName(x86Reg Reg);
    static const char * x86_HalfName(x86Reg Reg);
    static const char * fpu_Name(x86FpuValues Reg);

    CX86Ops(CCodeBlock & CodeBlock);

    // Logging functions
    void WriteX86Comment(const char * Comment);
    void WriteX86Label(const char * Label);

    void AdcConstToVariable(void * Variable, const char * VariableName, uint8_t Constant);
    void AdcConstToX86Reg(x86Reg Reg, uint32_t Const);
    void AdcVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName);
    void AdcX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void AddConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void AddConstToX86Reg(x86Reg Reg, uint32_t Const, bool NeedCarry = false);
    void AddVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName);
    void AddX86regToVariable(void * Variable, const char * VariableName, x86Reg Reg);
    void AddX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void AndConstToX86Reg(x86Reg Reg, uint32_t Const);
    void AndVariableToX86Reg(x86Reg Reg, void * Variable, const char * VariableName);
    void AndVariableDispToX86Reg(x86Reg Reg, void * Variable, const char * VariableName, x86Reg AddrReg, Multipler Multiply);
    void AndX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void X86HardBreakPoint();
    void X86BreakPoint(const char * FileName, int32_t LineNumber);
    void CallFunc(uint32_t FunctPtr, const char * FunctName);
    void CallThis(uint32_t ThisPtr, uint32_t FunctPtr, char * FunctName, uint32_t StackSize);
    void CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void CompConstToX86reg(x86Reg Reg, uint32_t Const);
    void CompConstToX86regPointer(x86Reg Reg, uint32_t Const);
    void CompX86regToVariable(x86Reg Reg, void * Variable, const char * VariableName);
    void CompX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void DecX86reg(x86Reg Reg);
    void DivX86reg(x86Reg Reg);
    void idivX86reg(x86Reg Reg);
    void imulX86reg(x86Reg Reg);
    void IncX86reg(x86Reg Reg);
    void JaeLabel8(const char * Label, uint8_t Value);
    void JaeLabel32(const char * Label, uint32_t Value);
    void JaLabel8(const char * Label, uint8_t Value);
    void JaLabel32(const char * Label, uint32_t Value);
    void JbLabel8(const char * Label, uint8_t Value);
    void JbLabel32(const char * Label, uint32_t Value);
    void JecxzLabel8(const char * Label, uint8_t Value);
    void JeLabel8(const char * Label, uint8_t Value);
    void JeLabel32(const char * Label, uint32_t Value);
    void JgeLabel8(const char * Label, uint8_t Value);
    void JgeLabel32(const char * Label, uint32_t Value);
    void JgLabel8(const char * Label, uint8_t Value);
    void JgLabel32(const char * Label, uint32_t Value);
    void JleLabel8(const char * Label, uint8_t Value);
    void JleLabel32(const char * Label, uint32_t Value);
    void JlLabel8(const char * Label, uint8_t Value);
    void JlLabel32(const char * Label, uint32_t Value);
    void JmpDirectReg(x86Reg Reg);
    void JmpIndirectLabel32(const char * Label, uint32_t location);
    void JmpIndirectReg(x86Reg Reg);
    void JmpLabel8(const char * Label, uint8_t Value);
    void JmpLabel32(const char * Label, uint32_t Value);
    void JneLabel8(const char * Label, uint8_t Value);
    void JneLabel32(const char * Label, uint32_t Value);
    void JnsLabel8(const char * Label, uint8_t Value);
    void JnsLabel32(const char * Label, uint32_t Value);
    void JnzLabel8(const char * Label, uint8_t Value);
    void JnzLabel32(const char * Label, uint32_t Value);
    void JoLabel32(const char * Label, uint32_t Value);
    void JsLabel32(const char * Label, uint32_t Value);
    void JzLabel8(const char * Label, uint8_t Value);
    void JzLabel32(const char * Label, uint32_t Value);
    void LeaRegReg(x86Reg RegDest, x86Reg RegSrc, uint32_t Const, Multipler multiplier);
    void LeaRegReg2(x86Reg RegDest, x86Reg RegSrc, x86Reg RegSrc2, Multipler multiplier);
    void LeaSourceAndOffset(x86Reg x86DestReg, x86Reg x86SourceReg, int32_t offset);
    void MoveConstByteToVariable(void * Variable, const char * VariableName, uint8_t Const);
    void MoveConstByteToX86regPointer(uint8_t Const, x86Reg AddrReg1, x86Reg AddrReg2);
    void MoveConstHalfToVariable(void * Variable, const char * VariableName, uint16_t Const);
    void MoveConstHalfToX86regPointer(x86Reg AddrReg1, x86Reg AddrReg2, uint16_t Const);
    void MoveConstToMemoryDisp(x86Reg AddrReg, uint32_t Disp, uint32_t Const);
    void MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    void MoveConstToX86Pointer(x86Reg X86Pointer, uint32_t Const);
    void MoveConstToX86reg(x86Reg Reg, uint32_t Const);
    void MoveConstToX86regPointer(uint32_t Const, x86Reg AddrReg1, x86Reg AddrReg2);
    void MoveN64MemDispToX86reg(x86Reg Reg, x86Reg AddrReg, uint8_t Disp);
    void MoveN64MemToX86reg(x86Reg Reg, x86Reg AddrReg);
    void MoveN64MemToX86regByte(x86Reg Reg, x86Reg AddrReg);
    void MoveN64MemToX86regHalf(x86Reg Reg, x86Reg AddrReg);
    void MoveSxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg);
    void MoveSxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg);
    void MoveSxN64MemToX86regByte(x86Reg Reg, x86Reg AddrReg);
    void MoveSxN64MemToX86regHalf(x86Reg Reg, x86Reg AddrReg);
    void MoveSxVariableToX86regByte(void * Variable, const char * VariableName, x86Reg Reg);
    void MoveSxVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg Reg);
    void MoveVariableDispToX86Reg(void * Variable, const char * VariableName, x86Reg Reg, x86Reg AddrReg, int32_t Multiplier);
    void MoveVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName);
    void MoveVariableToX86regByte(void * Variable, const char * VariableName, x86Reg Reg);
    void MoveVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg Reg);
    void MoveX86PointerToX86reg(x86Reg Reg, x86Reg X86Pointer);
    void MoveX86PointerToX86regDisp(x86Reg Reg, x86Reg X86Pointer, uint8_t Disp);
    void MoveX86regByteToN64Mem(x86Reg Reg, x86Reg AddrReg);
    void MoveX86regByteToVariable(x86Reg Reg, void * Variable, const char * VariableName);
    void MoveX86regByteToX86regPointer(x86Reg Reg, x86Reg AddrReg1, x86Reg AddrReg2);
    void MoveX86regHalfToN64Mem(x86Reg Reg, x86Reg AddrReg);
    void MoveX86regHalfToVariable(x86Reg Reg, void * Variable, const char * VariableName);
    void MoveX86regHalfToX86regPointer(x86Reg Reg, x86Reg AddrReg1, x86Reg AddrReg2);
    void MoveX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg);
    void MoveX86regPointerToX86regDisp8(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg, uint8_t offset);
    void MoveX86regToMemory(x86Reg Reg, x86Reg AddrReg, uint32_t Disp);
    void MoveX86regToN64Mem(x86Reg Reg, x86Reg AddrReg);
    void MoveX86regToN64MemDisp(x86Reg Reg, x86Reg AddrReg, uint8_t Disp);
    void MoveX86regToVariable(x86Reg Reg, void * Variable, const char * VariableName);
    void MoveX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void MoveX86regToX86Pointer(x86Reg X86Pointer, x86Reg Reg);
    void MoveX86regToX86regPointer(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg);
    void MoveZxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg);
    void MoveZxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg);
    void MoveZxN64MemToX86regByte(x86Reg Reg, x86Reg AddrReg);
    void MoveZxN64MemToX86regHalf(x86Reg Reg, x86Reg AddrReg);
    void MoveZxVariableToX86regByte(void * Variable, const char * VariableName, x86Reg Reg);
    void MoveZxVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg Reg);
    void MulX86reg(x86Reg Reg);
    void NotX86Reg(x86Reg Reg);
    void OrConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    void OrConstToX86Reg(uint32_t Const, x86Reg Reg);
    void OrVariableToX86Reg(void * Variable, const char * VariableName, x86Reg Reg);
    void OrX86RegToVariable(void * Variable, const char * VariableName, x86Reg Reg);
    void OrX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void Push(x86Reg Reg);
    void Pushad();
    void PushImm32(uint32_t Value);
    void PushImm32(const char * String, uint32_t Value);
    void Pop(x86Reg Reg);
    void Popad();
    void Ret();
    void Seta(x86Reg Reg);
    void Setae(x86Reg Reg);
    void SetaVariable(void * Variable, const char * VariableName);
    void Setb(x86Reg Reg);
    void SetbVariable(void * Variable, const char * VariableName);
    void Setg(x86Reg Reg);
    void SetgVariable(void * Variable, const char * VariableName);
    void Setl(x86Reg Reg);
    void SetlVariable(void * Variable, const char * VariableName);
    void Setz(x86Reg Reg);
    void Setnz(x86Reg Reg);
    void ShiftLeftDouble(x86Reg Destination, x86Reg Source);
    void ShiftLeftDoubleImmed(x86Reg Destination, x86Reg Source, uint8_t Immediate);
    void ShiftLeftSign(x86Reg Reg);
    void ShiftLeftSignImmed(x86Reg Reg, uint8_t Immediate);
    void ShiftRightDouble(x86Reg Destination, x86Reg Source);
    void ShiftRightDoubleImmed(x86Reg Destination, x86Reg Source, uint8_t Immediate);
    void ShiftRightSign(x86Reg Reg);
    void ShiftRightSignImmed(x86Reg Reg, uint8_t Immediate);
    void ShiftRightUnsign(x86Reg Reg);
    void ShiftRightUnsignImmed(x86Reg Reg, uint8_t Immediate);
    void SbbConstFromX86Reg(x86Reg Reg, uint32_t Const);
    void SbbVariableFromX86reg(x86Reg Reg, void * Variable, const char * VariableName);
    void SbbX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    void SubConstFromX86Reg(x86Reg Reg, uint32_t Const);
    void SubVariableFromX86reg(x86Reg Reg, void * Variable, const char * VariableName);
    void SubX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void TestConstToX86Reg(uint32_t Const, x86Reg Reg);
    void TestVariable(uint32_t Const, void * Variable, const char * VariableName);
    void TestX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    void TestX86ByteRegToX86Reg(x86Reg Destination, x86Reg Source);
    void XorConstToX86Reg(x86Reg Reg, uint32_t Const);
    void XorX86RegToX86Reg(x86Reg Source, x86Reg Destination);
    void XorVariableToX86reg(void * Variable, const char * VariableName, x86Reg Reg);

    void fpuAbs();
    void fpuAddDword(void * Variable, const char * VariableName);
    void fpuAddDwordRegPointer(x86Reg x86Pointer);
    void fpuAddQword(void * Variable, const char * VariableName);
    void fpuAddQwordRegPointer(x86Reg X86Pointer);
    void fpuAddReg(x86FpuValues Reg);
    void fpuAddRegPop(int32_t & StackPos, x86FpuValues Reg);
    void fpuComDword(void * Variable, const char * VariableName, bool Pop);
    void fpuComDwordRegPointer(x86Reg X86Pointer, bool Pop);
    void fpuComQword(void * Variable, const char * VariableName, bool Pop);
    void fpuComQwordRegPointer(x86Reg X86Pointer, bool Pop);
    void fpuComReg(x86FpuValues Reg, bool Pop);
    void fpuDivDword(void * Variable, const char * VariableName);
    void fpuDivDwordRegPointer(x86Reg X86Pointer);
    void fpuDivQword(void * Variable, const char * VariableName);
    void fpuDivQwordRegPointer(x86Reg X86Pointer);
    void fpuDivReg(x86FpuValues Reg);
    void fpuDivRegPop(x86FpuValues Reg);
    void fpuExchange(x86FpuValues Reg);
    void fpuFree(x86FpuValues Reg);
    void fpuDecStack(int32_t & StackPos);
    void fpuIncStack(int32_t & StackPos);
    void fpuLoadControl(void * Variable, const char * VariableName);
    void fpuLoadDword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadDwordFromX86Reg(int32_t & StackPos, x86Reg Reg);
    void fpuLoadDwordFromN64Mem(int32_t & StackPos, x86Reg Reg);
    void fpuLoadInt32bFromN64Mem(int32_t & StackPos, x86Reg Reg);
    void fpuLoadIntegerDword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadIntegerDwordFromX86Reg(int32_t & StackPos, x86Reg Reg);
    void fpuLoadIntegerQword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadIntegerQwordFromX86Reg(int32_t & StackPos, x86Reg Reg);
    void fpuLoadQword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadQwordFromX86Reg(int32_t & StackPos, x86Reg Reg);
    void fpuLoadQwordFromN64Mem(int32_t & StackPos, x86Reg Reg);
    void fpuLoadReg(int32_t & StackPos, x86FpuValues Reg);
    void fpuMulDword(void * Variable, const char * VariableName);
    void fpuMulDwordRegPointer(x86Reg X86Pointer);
    void fpuMulQword(void * Variable, const char * VariableName);
    void fpuMulQwordRegPointer(x86Reg X86Pointer);
    void fpuMulReg(x86FpuValues Reg);
    void fpuMulRegPop(x86FpuValues Reg);
    void fpuNeg();
    void fpuRound();
    void fpuSqrt();
    void fpuStoreControl(void * Variable, const char * VariableName);
    void fpuStoreDword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop);
    void fpuStoreDwordFromX86Reg(int32_t & StackPos, x86Reg Reg, bool pop);
    void fpuStoreDwordToN64Mem(int32_t & StackPos, x86Reg Reg, bool Pop);
    void fpuStoreIntegerDword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop);
    void fpuStoreIntegerDwordFromX86Reg(int32_t & StackPos, x86Reg Reg, bool pop);
    void fpuStoreIntegerQword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop);
    void fpuStoreIntegerQwordFromX86Reg(int32_t & StackPos, x86Reg Reg, bool pop);
    void fpuStoreQwordFromX86Reg(int32_t & StackPos, x86Reg Reg, bool pop);
    void fpuStoreStatus();
    void fpuSubDword(void * Variable, const char * VariableName);
    void fpuSubDwordRegPointer(x86Reg X86Pointer);
    void fpuSubDwordReverse(void * Variable, const char * VariableName);
    void fpuSubQword(void * Variable, const char * VariableName);
    void fpuSubQwordRegPointer(x86Reg X86Pointer);
    void fpuSubQwordReverse(void * Variable, const char * VariableName);
    void fpuSubReg(x86FpuValues Reg);
    void fpuSubRegPop(x86FpuValues Reg);

    static bool Is8BitReg(x86Reg Reg);
    static uint8_t CalcMultiplyCode(Multipler Multiply);
    static uint32_t GetAddressOf(int32_t value, ...);

    void SetJump32(uint32_t * Loc, uint32_t * JumpLoc);
    void SetJump8(uint8_t * Loc, uint8_t * JumpLoc);

private:
    CX86Ops(void);
    CX86Ops(const CX86Ops &);
    CX86Ops & operator=(const CX86Ops &);

    void CodeLog(_Printf_format_string_ const char * Text, ...);

    static void BreakPointNotification(const char * FileName, int32_t LineNumber);
    static char m_fpupop[2][2];
    void AddCode8(uint8_t value);
    void AddCode16(uint16_t value);
    void AddCode32(uint32_t value);

    CCodeBlock & m_CodeBlock;
};

#define AddressOf(Addr) CX86Ops::GetAddressOf(5, (Addr))

#endif
