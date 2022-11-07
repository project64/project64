#pragma once
#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Recompiler/asmjit.h>

#if !defined(_MSC_VER) && !defined(_Printf_format_string_)
#define _Printf_format_string_
#endif

class CCodeBlock;

static constexpr asmjit::x86::Gp x86Reg_Unknown = asmjit::x86::Gp();

class CX86Ops
{
public:
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

    static const char * x86_Name(const asmjit::x86::Gp & Reg);
    static const char * x86_ByteName(const asmjit::x86::Gp & Reg);
    static const char * x86_HalfName(const asmjit::x86::Gp & Reg);
    static const char * fpu_Name(x86FpuValues Reg);

    CX86Ops(CCodeBlock & CodeBlock);

    // Logging functions
    void WriteX86Comment(const char * Comment);
    void WriteX86Label(const char * Label);

    void AdcConstToVariable(void * Variable, const char * VariableName, uint8_t Constant);
    void AdcConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void AdcVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void AdcX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void AddConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void AddConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const, bool NeedCarry = false);
    void AddVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void AddX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void AddX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void AndConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void AndVariableToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void AndVariableDispToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, const asmjit::x86::Gp & AddrReg, Multipler Multiply);
    void AndX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void X86HardBreakPoint();
    void X86BreakPoint(const char * FileName, int32_t LineNumber);
    void CallFunc(uint32_t FunctPtr, const char * FunctName);
    void CallThis(uint32_t ThisPtr, uint32_t FunctPtr, char * FunctName, uint32_t StackSize);
    void CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void CompConstToX86reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void CompConstToX86regPointer(const asmjit::x86::Gp & Reg, uint32_t Const);
    void CompX86regToVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void CompX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void DecX86reg(const asmjit::x86::Gp & Reg);
    void DivX86reg(const asmjit::x86::Gp & Reg);
    void idivX86reg(const asmjit::x86::Gp & Reg);
    void imulX86reg(const asmjit::x86::Gp & Reg);
    void IncX86reg(const asmjit::x86::Gp & Reg);
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
    void JmpDirectReg(const asmjit::x86::Gp & Reg);
    void JmpIndirectLabel32(const char * Label, uint32_t location);
    void JmpIndirectReg(const asmjit::x86::Gp & Reg);
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
    void LeaRegReg(const asmjit::x86::Gp & RegDest, const asmjit::x86::Gp & RegSrc, uint32_t Const, Multipler multiplier);
    void LeaRegReg2(const asmjit::x86::Gp & RegDest, const asmjit::x86::Gp & RegSrc, const asmjit::x86::Gp & RegSrc2, Multipler multiplier);
    void LeaSourceAndOffset(const asmjit::x86::Gp & x86DestReg, const asmjit::x86::Gp & x86SourceReg, int32_t offset);
    void MoveConstByteToVariable(void * Variable, const char * VariableName, uint8_t Const);
    void MoveConstByteToX86regPointer(const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, uint8_t Const);
    void MoveConstHalfToVariable(void * Variable, const char * VariableName, uint16_t Const);
    void MoveConstHalfToX86regPointer(const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, uint16_t Const);
    void MoveConstToMemoryDisp(const asmjit::x86::Gp & AddrReg, uint32_t Disp, uint32_t Const);
    void MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void MoveConstToX86Pointer(const asmjit::x86::Gp & X86Pointer, uint32_t Const);
    void MoveConstToX86reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void MoveConstToX86regPointer(const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, uint32_t Const);
    void MoveSxByteX86regPointerToX86reg(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2);
    void MoveSxHalfX86regPointerToX86reg(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2);
    void MoveSxVariableToX86regByte(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveSxVariableToX86regHalf(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveVariableDispToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, const asmjit::x86::Gp & AddrReg, Multipler Multiplier);
    void MoveVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveX86PointerToX86reg(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & X86Pointer);
    void MoveX86PointerToX86regDisp(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & X86Pointer, uint8_t Disp);
    void MoveX86regByteToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void MoveX86regByteToX86regPointer(const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, const asmjit::x86::Gp & Reg);
    void MoveX86regHalfToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void MoveX86regHalfToX86regPointer(const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, const asmjit::x86::Gp & Reg);
    void MoveX86regPointerToX86reg(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2);
    void MoveX86regPointerToX86regDisp8(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, uint8_t offset);
    void MoveX86regToMemory(const asmjit::x86::Gp & AddrReg, uint32_t Disp, const asmjit::x86::Gp & Reg);
    void MoveX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void MoveX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void MoveX86regToX86Pointer(const asmjit::x86::Gp & X86Pointer, const asmjit::x86::Gp & Reg);
    void MoveX86regToX86regPointer(const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2, const asmjit::x86::Gp & Reg);
    void MoveZxByteX86regPointerToX86reg(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2);
    void MoveZxHalfX86regPointerToX86reg(const asmjit::x86::Gp & Reg, const asmjit::x86::Gp & AddrReg1, const asmjit::x86::Gp & AddrReg2);
    void MoveZxVariableToX86regByte(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveZxVariableToX86regHalf(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MulX86reg(const asmjit::x86::Gp & Reg);
    void NotX86Reg(const asmjit::x86::Gp & Reg);
    void OrConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void OrConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void OrVariableToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void OrX86RegToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void OrX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void Push(const asmjit::x86::Gp & Reg);
    void Pushad();
    void PushImm32(uint32_t Value);
    void PushImm32(const char * String, uint32_t Value);
    void Pop(const asmjit::x86::Gp & Reg);
    void Popad();
    void Ret();
    void Seta(const asmjit::x86::Gp & Reg);
    void Setae(const asmjit::x86::Gp & Reg);
    void SetaVariable(void * Variable, const char * VariableName);
    void Setb(const asmjit::x86::Gp & Reg);
    void SetbVariable(void * Variable, const char * VariableName);
    void Setg(const asmjit::x86::Gp & Reg);
    void SetgVariable(void * Variable, const char * VariableName);
    void Setl(const asmjit::x86::Gp & Reg);
    void SetlVariable(void * Variable, const char * VariableName);
    void Setz(const asmjit::x86::Gp & Reg);
    void Setnz(const asmjit::x86::Gp & Reg);
    void ShiftLeftDouble(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void ShiftLeftDoubleImmed(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source, uint8_t Immediate);
    void ShiftLeftSign(const asmjit::x86::Gp & Reg);
    void ShiftLeftSignImmed(const asmjit::x86::Gp & Reg, uint8_t Immediate);
    void ShiftRightDouble(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void ShiftRightDoubleImmed(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source, uint8_t Immediate);
    void ShiftRightSign(const asmjit::x86::Gp & Reg);
    void ShiftRightSignImmed(const asmjit::x86::Gp & Reg, uint8_t Immediate);
    void ShiftRightUnsign(const asmjit::x86::Gp & Reg);
    void ShiftRightUnsignImmed(const asmjit::x86::Gp & Reg, uint8_t Immediate);
    void SbbConstFromX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void SbbVariableFromX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void SbbX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    void SubConstFromX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void SubVariableFromX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void SubX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void TestConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void TestVariable(void * Variable, const char * VariableName, uint32_t Const);
    void TestX86RegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void TestX86ByteRegToX86Reg(const asmjit::x86::Gp & Destination, const asmjit::x86::Gp & Source);
    void XorConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void XorX86RegToX86Reg(const asmjit::x86::Gp & Source, const asmjit::x86::Gp & Destination);
    void XorVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);

    void fpuAbs();
    void fpuAddDword(void * Variable, const char * VariableName);
    void fpuAddDwordRegPointer(const asmjit::x86::Gp & x86Pointer);
    void fpuAddQword(void * Variable, const char * VariableName);
    void fpuAddQwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuAddReg(x86FpuValues Reg);
    void fpuAddRegPop(int32_t & StackPos, x86FpuValues Reg);
    void fpuComDword(void * Variable, const char * VariableName, bool Pop);
    void fpuComDwordRegPointer(const asmjit::x86::Gp & X86Pointer, bool Pop);
    void fpuComQword(void * Variable, const char * VariableName, bool Pop);
    void fpuComQwordRegPointer(const asmjit::x86::Gp & X86Pointer, bool Pop);
    void fpuComReg(x86FpuValues Reg, bool Pop);
    void fpuDivDword(void * Variable, const char * VariableName);
    void fpuDivDwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuDivQword(void * Variable, const char * VariableName);
    void fpuDivQwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuDivReg(x86FpuValues Reg);
    void fpuDivRegPop(x86FpuValues Reg);
    void fpuExchange(x86FpuValues Reg);
    void fpuFree(x86FpuValues Reg);
    void fpuDecStack(int32_t & StackPos);
    void fpuIncStack(int32_t & StackPos);
    void fpuLoadControl(void * Variable, const char * VariableName);
    void fpuLoadDword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadDwordFromN64Mem(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadInt32bFromN64Mem(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadIntegerDword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadIntegerDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadIntegerQword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadIntegerQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadQword(int32_t & StackPos, void * Variable, const char * VariableName);
    void fpuLoadQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadQwordFromN64Mem(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadReg(int32_t & StackPos, x86FpuValues Reg);
    void fpuMulDword(void * Variable, const char * VariableName);
    void fpuMulDwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuMulQword(void * Variable, const char * VariableName);
    void fpuMulQwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuMulReg(x86FpuValues Reg);
    void fpuMulRegPop(x86FpuValues Reg);
    void fpuNeg();
    void fpuRound();
    void fpuSqrt();
    void fpuStoreControl(void * Variable, const char * VariableName);
    void fpuStoreDword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop);
    void fpuStoreDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreDwordToN64Mem(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool Pop);
    void fpuStoreIntegerDword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop);
    void fpuStoreIntegerDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreIntegerQword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop);
    void fpuStoreIntegerQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreStatus();
    void fpuSubDword(void * Variable, const char * VariableName);
    void fpuSubDwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuSubDwordReverse(void * Variable, const char * VariableName);
    void fpuSubQword(void * Variable, const char * VariableName);
    void fpuSubQwordRegPointer(const asmjit::x86::Gp & X86Pointer);
    void fpuSubQwordReverse(void * Variable, const char * VariableName);
    void fpuSubReg(x86FpuValues Reg);
    void fpuSubRegPop(x86FpuValues Reg);

    static bool Is8BitReg(const asmjit::x86::Gp & Reg);
    static uint8_t CalcMultiplyCode(Multipler Multiply);
    static uint32_t GetAddressOf(int32_t value, ...);

    void SetJump32(uint32_t * Loc, uint32_t * JumpLoc);
    void SetJump8(uint8_t * Loc, uint8_t * JumpLoc);

private:
    CX86Ops(void);
    CX86Ops(const CX86Ops &);
    CX86Ops & operator=(const CX86Ops &);

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

        x86_AL = 0,
        x86_BL = 3,
        x86_CL = 1,
        x86_DL = 2,
        x86_AH = 4,
        x86_BH = 7,
        x86_CH = 5,
        x86_DH = 6
    };

    static x86Reg RegValue(const asmjit::x86::Gp & Reg);
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
