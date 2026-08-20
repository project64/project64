#pragma once
#if defined(__amd64__) || defined(_M_X64)
#include <Project64-core/N64System/Recompiler/asmjit.h>
#include <map>
#include <string>

class CCodeBlock;

class CX64Ops :
    public asmjit::x86::Assembler,
    public asmjit::Logger
{
public:
    CX64Ops(CCodeBlock & CodeBlock);

    void JoLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JsLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JeLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JneLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JmpLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void CmpConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void CmpRegToVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void MoveConstToX64reg(const asmjit::x86::Gp & Reg, uint64_t Const, const char * ValueName = nullptr);
    void MoveVariableToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, bool SignExtend);
    void MoveVariable32ToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveVariable64ToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveVariable32SignExtendToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MovDwordToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Src);
    void MovQwordToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Src);
    void AddDwordFromVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void SubVariableFromX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    void AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void XorVariableToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void TestVariable(void * Variable, const char * VariableName, uint32_t Const);
    void EnterPrimarySection();
    void EnterSecondarySection();
    void X64BreakPoint(const char * FileName, int32_t LineNumber);
    void CallFunc(uintptr_t FunctPtr, const char * FunctName);
    void CallThis(void * ThisPtr, uintptr_t FunctPtr, const char * FunctName);

private:
    CX64Ops(void);
    CX64Ops(const CX64Ops &);
    CX64Ops & operator=(const CX64Ops &);

    asmjit::Error _log(const char * data, size_t size) noexcept;
    void AddLabelSymbol(const asmjit::Label & Label, const char * Symbol);
    void AddNumberSymbol(uintptr_t Value, const char * Symbol);
    void AddNumberSymbol(uintptr_t Value, const std::string & Symbol);

    static void BreakPointNotification(const char * FileName, int32_t LineNumber);

    typedef struct
    {
        std::string Symbol;
        uint32_t Count;
    } NumberSymbol;

    typedef std::map<uintptr_t, NumberSymbol> NumberSymbolMap;

    NumberSymbolMap m_LabelSymbols;
    NumberSymbolMap m_NumberSymbols;
    CCodeBlock & m_CodeBlock;
    asmjit::Section * m_PrimarySection;
    asmjit::Section * m_SecondarySection;
};

template <typename T>
uintptr_t MemberFuncAddress(T func)
{
    uintptr_t result = 0;
    memcpy(&result, &func, sizeof(uintptr_t));
    return result;
}

#endif
