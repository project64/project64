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

private:
    CX64Ops(void);
    CX64Ops(const CX64Ops &);
    CX64Ops & operator=(const CX64Ops &);

    asmjit::Error _log(const char * data, size_t size) noexcept;

    typedef std::map<std::string, std::string> SymbolMap;

    SymbolMap m_Symbols;
    CCodeBlock & m_CodeBlock;
};

#endif
