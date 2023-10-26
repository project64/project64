#pragma once
#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Recompiler/asmjit.h>
#include <map>
#include <string>

#if !defined(_MSC_VER) && !defined(_Printf_format_string_)
#define _Printf_format_string_
#endif

class CCodeBlock;

static constexpr asmjit::x86::Gp x86Reg_Unknown = asmjit::x86::Gp();

class CX86Ops :
    public asmjit::x86::Assembler,
    public asmjit::Logger
{
public:
    enum Multipler
    {
        Multip_x1 = 1,
        Multip_x2 = 2,
        Multip_x4 = 4,
        Multip_x8 = 8
    };

    static const char * x86_Name(const asmjit::x86::Gp & Reg);

    CX86Ops(CCodeBlock & CodeBlock);

    void AdcVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void AddConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void AddConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void AddVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void AndVariableToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void AndVariableDispToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, const asmjit::x86::Gp & AddrReg, Multipler Multiply);
    void X86HardBreakPoint();
    void X86BreakPoint(const char * FileName, int32_t LineNumber);
    void CallFunc(uint32_t FunctPtr, const char * FunctName);
    void CallThis(uint32_t ThisPtr, uint32_t FunctPtr, const char * FunctName, uint32_t StackSize);
    void CompConstByteToVariable(void * Variable, const char * VariableName, uint8_t Const);
    void CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void CompConstToX86reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void CompX86regToVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void JaeLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JaLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JbLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JecxzLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JeLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JgeLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JgLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JleLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JlLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JmpLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JneLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JnsLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JnzLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JoLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JsLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JzLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void MoveConstByteToVariable(void * Variable, const char * VariableName, uint8_t Const);
    void MoveConstHalfToVariable(void * Variable, const char * VariableName, uint16_t Const);
    void MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void MoveConstToX86reg(const asmjit::x86::Gp & Reg, uint32_t Const);
    void MoveSxVariableToX86regByte(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveSxVariableToX86regHalf(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveVariableDispToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, const asmjit::x86::Gp & AddrReg, Multipler Multiplier);
    void MoveVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveX86regByteToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void MoveX86regHalfToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void MoveX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void MoveZxVariableToX86regByte(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveZxVariableToX86regHalf(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void OrConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void OrVariableToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void OrX86RegToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void PushImm32(const char * String, uint32_t Value);
    void SetaVariable(void * Variable, const char * VariableName);
    void SetbVariable(void * Variable, const char * VariableName);
    void SetgVariable(void * Variable, const char * VariableName);
    void SetlVariable(void * Variable, const char * VariableName);
    void SbbVariableFromX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    void SubVariableFromX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void TestVariable(void * Variable, const char * VariableName, uint32_t Const);
    void XorVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);

    void fpuIncStack(int32_t & StackPos);
    void fpuLoadControl(void * Variable, const char * VariableName);
    void fpuLoadDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadIntegerDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadIntegerQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg);
    void fpuLoadReg(int32_t & StackPos, const asmjit::x86::St & Reg);
    void fpuStoreControl(void * Variable, const char * VariableName);
    void fpuStoreDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreIntegerDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreIntegerQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);
    void fpuStoreQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop);

    static bool Is8BitReg(const asmjit::x86::Gp & Reg);
    static uint32_t GetAddressOf(int32_t value, ...);

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
    asmjit::Error _log(const char * data, size_t size) noexcept;
    void AddSymbol(const char * SymbolKey, const char * SymbolValue);
    void RemoveSymbol(const char * SymbolKey);
    std::string VariableSymbol(void * Variable) const;

    static void BreakPointNotification(const char * FileName, int32_t LineNumber);

    typedef std::map<std::string, std::string> SymbolMap;

    SymbolMap m_Symbols;
    CCodeBlock & m_CodeBlock;
};

#define AddressOf(Addr) CX86Ops::GetAddressOf(5, (Addr))

#endif
