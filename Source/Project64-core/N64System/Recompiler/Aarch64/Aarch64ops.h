#pragma once
#if defined(__aarch64__)
#include <Project64-core/N64System/Recompiler/asmjit.h>

class CCodeBlock;

class CAarch64Ops :
    public asmjit::a64::Assembler,
    public asmjit::Logger
{
public:
    CAarch64Ops(CCodeBlock & CodeBlock);

private:
    CAarch64Ops(void);
    CAarch64Ops(const CAarch64Ops &);
    CAarch64Ops & operator=(const CAarch64Ops &);

    asmjit::Error _log(const char * data, size_t size) noexcept;

    typedef std::map<std::string, std::string> SymbolMap;

    SymbolMap m_Symbols;
    CCodeBlock & m_CodeBlock;
};

#endif
