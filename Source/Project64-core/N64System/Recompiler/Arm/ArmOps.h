#pragma once
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/asmjit.h>

class CCodeBlock;

class CArmOps :
    public asmjit::a64::Assembler,
    public asmjit::Logger
{
public:
    CArmOps(CCodeBlock & CodeBlock);

private:
    CArmOps(void);
    CArmOps(const CArmOps &);
    CArmOps & operator=(const CArmOps &);

    asmjit::Error _log(const char * data, size_t size) noexcept;

    typedef std::map<std::string, std::string> SymbolMap;

    SymbolMap m_Symbols;
    CCodeBlock & m_CodeBlock;
};

#endif
