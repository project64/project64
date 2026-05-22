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

    void MoveConstToX64reg(const asmjit::x86::Gp & Reg, uint64_t Const, const char * ValueName = nullptr);
    void MoveVariableToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveSxVariableToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);

private:
    CX64Ops(void);
    CX64Ops(const CX64Ops &);
    CX64Ops & operator=(const CX64Ops &);

    asmjit::Error _log(const char * data, size_t size) noexcept;
    void AddLabelSymbol(const asmjit::Label & Label, const char * Symbol);
    void AddNumberSymbol(uintptr_t Value, const char * Symbol);
    void AddNumberSymbol(uintptr_t Value, const std::string & Symbol);

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

#endif
