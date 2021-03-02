#pragma once

#if defined(__i386__) || defined(_M_IX86)
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
        x86_Any8Bit = -3,
        x86_Any = -2,
        x86_Unknown = -1,

        x86_AL = 0, x86_BL = 3, x86_CL = 1, x86_DL = 2,
        x86_AH = 4, x86_BH = 7, x86_CH = 5, x86_DH = 6
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

    static x86Reg x86_Registers[8];
    static const char * x86_Name(x86Reg Reg);
    static const char * x86_ByteName(x86Reg Reg);
    static const char * x86_HalfName(x86Reg Reg);
    static const char * fpu_Name(x86FpuValues Reg);

protected:
    //Logging Functions
    static void WriteX86Comment(const char * Comment);
    static void WriteX86Label(const char * Label);

    static void AdcX86regToVariable(x86Reg reg, void * Variable, const char * VariableName);
    static void AdcConstToVariable(void *Variable, const char * VariableName, uint8_t Constant);
    static void AdcConstToX86Reg(x86Reg Reg, uint32_t Const);
    static void AdcVariableToX86reg(x86Reg reg, void * Variable, const char * VariableName);
    static void AdcX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void AddConstToVariable(uint32_t Const, void *Variable, const char * VariableName);
    static void AddConstToX86Reg(x86Reg Reg, uint32_t Const);
    static void AddVariableToX86reg(x86Reg reg, void * Variable, const char * VariableName);
    static void AddX86regToVariable(x86Reg reg, void * Variable, const char * VariableName);
    static void AddX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void AndConstToVariable(uint32_t Const, void *Variable, const char * VariableName);
    static void AndConstToX86Reg(x86Reg Reg, uint32_t Const);
    static void AndVariableToX86Reg(void * Variable, const char * VariableName, x86Reg Reg);
    static void AndVariableDispToX86Reg(void * Variable, const char * VariableName, x86Reg Reg, x86Reg AddrReg, Multipler Multiply);
    static void AndX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void X86HardBreakPoint();
    static void X86BreakPoint(const char * FileName, int32_t LineNumber);
    static void Call_Direct(void * FunctAddress, const char * FunctName);
    static void Call_Indirect(void * FunctAddress, const char * FunctName);
    static void CompConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void CompConstToX86reg(x86Reg Reg, uint32_t Const);
    static void CompConstToX86regPointer(x86Reg Reg, uint32_t Const);
    static void CompX86regToVariable(x86Reg Reg, void * Variable, const char * VariableName);
    static void CompVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName);
    static void CompX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void DecX86reg(x86Reg Reg);
    static void DivX86reg(x86Reg reg);
    static void idivX86reg(x86Reg reg);
    static void imulX86reg(x86Reg reg);
    static void IncX86reg(x86Reg Reg);
    static void JaeLabel8(const char * Label, uint8_t Value);
    static void JaeLabel32(const char * Label, uint32_t Value);
    static void JaLabel8(const char * Label, uint8_t Value);
    static void JaLabel32(const char * Label, uint32_t Value);
    static void JbLabel8(const char * Label, uint8_t Value);
    static void JbLabel32(const char * Label, uint32_t Value);
    static void JecxzLabel8(const char * Label, uint8_t Value);
    static void JeLabel8(const char * Label, uint8_t Value);
    static void JeLabel32(const char * Label, uint32_t Value);
    static void JgeLabel32(const char * Label, uint32_t Value);
    static void JgLabel8(const char * Label, uint8_t Value);
    static void JgLabel32(const char * Label, uint32_t Value);
    static void JleLabel8(const char * Label, uint8_t Value);
    static void JleLabel32(const char * Label, uint32_t Value);
    static void JlLabel8(const char * Label, uint8_t Value);
    static void JlLabel32(const char * Label, uint32_t Value);
    static void JmpDirectReg(x86Reg reg);
    static void JmpIndirectLabel32(const char * Label, uint32_t location);
    static void JmpIndirectReg(x86Reg reg);
    static void JmpLabel8(const char * Label, uint8_t Value);
    static void JmpLabel32(const char * Label, uint32_t Value);
    static void JneLabel8(const char * Label, uint8_t Value);
    static void JneLabel32(const char * Label, uint32_t Value);
    static void JnsLabel8(const char * Label, uint8_t Value);
    static void JnsLabel32(const char * Label, uint32_t Value);
    static void JnzLabel8(const char * Label, uint8_t Value);
    static void JnzLabel32(const char * Label, uint32_t Value);
    static void JsLabel32(const char * Label, uint32_t Value);
    static void JzLabel8(const char * Label, uint8_t Value);
    static void JzLabel32(const char * Label, uint32_t Value);
    static void LeaRegReg(x86Reg RegDest, x86Reg RegSrc, uint32_t Const, Multipler multiplier);
    static void LeaRegReg2(x86Reg RegDest, x86Reg RegSrc, x86Reg RegSrc2, Multipler multiplier);
    static void LeaSourceAndOffset(x86Reg x86DestReg, x86Reg x86SourceReg, int32_t offset);
    static void MoveConstByteToN64Mem(uint8_t Const, x86Reg AddrReg);
    static void MoveConstHalfToN64Mem(uint16_t Const, x86Reg AddrReg);
    static void MoveConstByteToVariable(uint8_t Const, void * Variable, const char * VariableName);
    static void MoveConstByteToX86regPointer(uint8_t Const, x86Reg AddrReg1, x86Reg AddrReg2);
    static void MoveConstHalfToVariable(uint16_t Const, void * Variable, const char * VariableName);
    static void MoveConstHalfToX86regPointer(uint16_t Const, x86Reg AddrReg1, x86Reg AddrReg2);
    static void MoveConstToMemoryDisp(uint32_t Const, x86Reg AddrReg, uint32_t Disp);
    static void MoveConstToN64Mem(uint32_t Const, x86Reg AddrReg);
    static void MoveConstToN64MemDisp(uint32_t Const, x86Reg AddrReg, uint8_t Disp);
    static void MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void MoveConstToX86Pointer(uint32_t Const, x86Reg X86Pointer);
    static void MoveConstToX86reg(uint32_t Const, x86Reg reg);
    static void MoveConstToX86regPointer(uint32_t Const, x86Reg AddrReg1, x86Reg AddrReg2);
    static void MoveN64MemDispToX86reg(x86Reg reg, x86Reg AddrReg, uint8_t Disp);
    static void MoveN64MemToX86reg(x86Reg reg, x86Reg AddrReg);
    static void MoveN64MemToX86regByte(x86Reg reg, x86Reg AddrReg);
    static void MoveN64MemToX86regHalf(x86Reg reg, x86Reg AddrReg);
    static void MoveSxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg);
    static void MoveSxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg);
    static void MoveSxN64MemToX86regByte(x86Reg reg, x86Reg AddrReg);
    static void MoveSxN64MemToX86regHalf(x86Reg reg, x86Reg AddrReg);
    static void MoveSxVariableToX86regByte(void * Variable, const char * VariableName, x86Reg reg);
    static void MoveSxVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg reg);
    static void MoveVariableDispToX86Reg(void * Variable, const char * VariableName, x86Reg Reg, x86Reg AddrReg, int32_t Multiplier);
    static void MoveVariableToX86reg(void * Variable, const char * VariableName, x86Reg reg);
    static void MoveVariableToX86regByte(void * Variable, const char * VariableName, x86Reg reg);
    static void MoveVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg reg);
    static void MoveX86PointerToX86reg(x86Reg reg, x86Reg X86Pointer);
    static void MoveX86PointerToX86regDisp(x86Reg reg, x86Reg X86Pointer, uint8_t Disp);
    static void MoveX86regByteToN64Mem(x86Reg reg, x86Reg AddrReg);
    static void MoveX86regByteToVariable(x86Reg reg, void * Variable, const char * VariableName);
    static void MoveX86regByteToX86regPointer(x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2);
    static void MoveX86regHalfToN64Mem(x86Reg reg, x86Reg AddrReg);
    static void MoveX86regHalfToVariable(x86Reg reg, void * Variable, const char * VariableName);
    static void MoveX86regHalfToX86regPointer(x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2);
    static void MoveX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg);
    static void MoveX86regPointerToX86regDisp8(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg, uint8_t offset);
    static void MoveX86regToMemory(x86Reg reg, x86Reg AddrReg, uint32_t Disp);
    static void MoveX86regToN64Mem(x86Reg reg, x86Reg AddrReg);
    static void MoveX86regToN64MemDisp(x86Reg reg, x86Reg AddrReg, uint8_t Disp);
    static void MoveX86regToVariable(x86Reg reg, void * Variable, const char * VariableName);
    static void MoveX86RegToX86Reg(x86Reg Source, x86Reg Destination);
    static void MoveX86regToX86Pointer(x86Reg reg, x86Reg X86Pointer);
    static void MoveX86regToX86regPointer(x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2);
    static void MoveZxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg);
    static void MoveZxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg);
    static void MoveZxN64MemToX86regByte(x86Reg reg, x86Reg AddrReg);
    static void MoveZxN64MemToX86regHalf(x86Reg reg, x86Reg AddrReg);
    static void MoveZxVariableToX86regByte(void * Variable, const char * VariableName, x86Reg reg);
    static void MoveZxVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg reg);
    static void MulX86reg(x86Reg reg);
    static void NotX86Reg(x86Reg Reg);
    static void OrConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void OrConstToX86Reg(uint32_t Const, x86Reg reg);
    static void OrVariableToX86Reg(void * Variable, const char * VariableName, x86Reg Reg);
    static void OrX86RegToVariable(void * Variable, const char * VariableName, x86Reg Reg);
    static void OrX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void Push(x86Reg reg);
    static void Pushad();
    static void PushImm32(uint32_t Value);
    static void PushImm32(const char * String, uint32_t Value);
    static void Pop(x86Reg reg);
    static void Popad();
    static void Ret();
    static void Seta(x86Reg reg);
    static void Setae(x86Reg reg);
    static void SetaVariable(void * Variable, const char * VariableName);
    static void Setb(x86Reg reg);
    static void SetbVariable(void * Variable, const char * VariableName);
    static void Setg(x86Reg reg);
    static void SetgVariable(void * Variable, const char * VariableName);
    static void Setl(x86Reg reg);
    static void SetlVariable(void * Variable, const char * VariableName);
    static void Setz(x86Reg reg);
    static void Setnz(x86Reg reg);
    static void ShiftLeftDouble(x86Reg Destination, x86Reg Source);
    static void ShiftLeftDoubleImmed(x86Reg Destination, x86Reg Source, uint8_t Immediate);
    static void ShiftLeftSign(x86Reg reg);
    static void ShiftLeftSignImmed(x86Reg reg, uint8_t Immediate);
    static void ShiftRightDouble(x86Reg Destination, x86Reg Source);
    static void ShiftRightDoubleImmed(x86Reg Destination, x86Reg Source, uint8_t Immediate);
    static void ShiftRightSign(x86Reg reg);
    static void ShiftRightSignImmed(x86Reg reg, uint8_t Immediate);
    static void ShiftRightUnsign(x86Reg reg);
    static void ShiftRightUnsignImmed(x86Reg reg, uint8_t Immediate);
    static void SbbConstFromX86Reg(x86Reg Reg, uint32_t Const);
    static void SbbVariableFromX86reg(x86Reg reg, void * Variable, const char * VariableName);
    static void SbbX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void SubConstFromX86Reg(x86Reg Reg, uint32_t Const);
    static void SubVariableFromX86reg(x86Reg reg, void * Variable, const char * VariableName);
    static void SubX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void TestConstToX86Reg(uint32_t Const, x86Reg reg);
    static void TestVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void TestX86RegToX86Reg(x86Reg Destination, x86Reg Source);
    static void XorConstToX86Reg(x86Reg Reg, uint32_t Const);
    static void XorX86RegToX86Reg(x86Reg Source, x86Reg Destination);
    static void XorVariableToX86reg(void * Variable, const char * VariableName, x86Reg reg);

    static void fpuAbs();
    static void fpuAddDword(void * Variable, const char * VariableName);
    static void fpuAddDwordRegPointer(x86Reg x86Pointer);
    static void fpuAddQword(void * Variable, const char * VariableName);
    static void fpuAddQwordRegPointer(x86Reg X86Pointer);
    static void fpuAddReg(x86FpuValues reg);
    static void fpuAddRegPop(int32_t * StackPos, x86FpuValues reg);
    static void fpuComDword(void * Variable, const char * VariableName, bool Pop);
    static void fpuComDwordRegPointer(x86Reg X86Pointer, bool Pop);
    static void fpuComQword(void * Variable, const char * VariableName, bool Pop);
    static void fpuComQwordRegPointer(x86Reg X86Pointer, bool Pop);
    static void fpuComReg(x86FpuValues reg, bool Pop);
    static void fpuDivDword(void * Variable, const char * VariableName);
    static void fpuDivDwordRegPointer(x86Reg X86Pointer);
    static void fpuDivQword(void * Variable, const char * VariableName);
    static void fpuDivQwordRegPointer(x86Reg X86Pointer);
    static void fpuDivReg(x86FpuValues Reg);
    static void fpuDivRegPop(x86FpuValues reg);
    static void fpuExchange(x86FpuValues Reg);
    static void fpuFree(x86FpuValues Reg);
    static void fpuDecStack(int32_t * StackPos);
    static void fpuIncStack(int32_t * StackPos);
    static void fpuLoadControl(void * Variable, const char * VariableName);
    static void fpuLoadDword(int32_t * StackPos, void * Variable, const char * VariableName);
    static void fpuLoadDwordFromX86Reg(int32_t * StackPos, x86Reg reg);
    static void fpuLoadDwordFromN64Mem(int32_t * StackPos, x86Reg reg);
    static void fpuLoadInt32bFromN64Mem(int32_t * StackPos, x86Reg reg);
    static void fpuLoadIntegerDword(int32_t * StackPos, void * Variable, const char * VariableName);
    static void fpuLoadIntegerDwordFromX86Reg(int32_t * StackPos, x86Reg Reg);
    static void fpuLoadIntegerQword(int32_t * StackPos, void * Variable, const char * VariableName);
    static void fpuLoadIntegerQwordFromX86Reg(int32_t * StackPos, x86Reg Reg);
    static void fpuLoadQword(int32_t * StackPos, void * Variable, const char * VariableName);
    static void fpuLoadQwordFromX86Reg(int32_t * StackPos, x86Reg Reg);
    static void fpuLoadQwordFromN64Mem(int32_t * StackPos, x86Reg reg);
    static void fpuLoadReg(int32_t * StackPos, x86FpuValues Reg);
    static void fpuMulDword(void * Variable, const char * VariableName);
    static void fpuMulDwordRegPointer(x86Reg X86Pointer);
    static void fpuMulQword(void * Variable, const char * VariableName);
    static void fpuMulQwordRegPointer(x86Reg X86Pointer);
    static void fpuMulReg(x86FpuValues reg);
    static void fpuMulRegPop(x86FpuValues reg);
    static void fpuNeg();
    static void fpuRound();
    static void fpuSqrt();
    static void fpuStoreControl(void * Variable, const char * VariableName);
    static void fpuStoreDword(int32_t * StackPos, void * Variable, const char * VariableName, bool pop);
    static void fpuStoreDwordFromX86Reg(int32_t * StackPos, x86Reg Reg, bool pop);
    static void fpuStoreDwordToN64Mem(int32_t * StackPos, x86Reg reg, bool Pop);
    static void fpuStoreIntegerDword(int32_t * StackPos, void * Variable, const char * VariableName, bool pop);
    static void fpuStoreIntegerDwordFromX86Reg(int32_t * StackPos, x86Reg Reg, bool pop);
    static void fpuStoreIntegerQword(int32_t * StackPos, void * Variable, const char * VariableName, bool pop);
    static void fpuStoreIntegerQwordFromX86Reg(int32_t * StackPos, x86Reg Reg, bool pop);
    static void fpuStoreQwordFromX86Reg(int32_t * StackPos, x86Reg Reg, bool pop);
    static void fpuStoreStatus();
    static void fpuSubDword(void * Variable, const char * VariableName);
    static void fpuSubDwordRegPointer(x86Reg X86Pointer);
    static void fpuSubDwordReverse(void * Variable, const char * VariableName);
    static void fpuSubQword(void * Variable, const char * VariableName);
    static void fpuSubQwordRegPointer(x86Reg X86Pointer);
    static void fpuSubQwordReverse(void * Variable, const char * VariableName);
    static void fpuSubReg(x86FpuValues reg);
    static void fpuSubRegPop(x86FpuValues reg);

    static bool Is8BitReg(x86Reg Reg);
    static uint8_t CalcMultiplyCode(Multipler Multiply);

    static void * GetAddressOf(int32_t value, ...);
    static void SetJump32(uint32_t * Loc, uint32_t * JumpLoc);
    static void SetJump8(uint8_t * Loc, uint8_t * JumpLoc);

private:
    static void BreakPointNotification(const char * FileName, int32_t LineNumber);
    static char m_fpupop[2][2];
    static void AddCode8(uint8_t value);
    static void AddCode16(uint16_t value);
    static void AddCode32(uint32_t value);
};

#define AddressOf(Addr) CX86Ops::GetAddressOf(5,(Addr))

#endif
