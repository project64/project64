#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/x64-86/x64ops.h>

CX64Ops::CX64Ops(CCodeBlock & CodeBlock) :
    m_CodeBlock(CodeBlock)
{
}

asmjit::Error CX64Ops::_log(const char * data, size_t size) noexcept
{
    stdstr AsmjitLog(std::string(data, size));
    AsmjitLog.Trim("\n");
    for (SymbolMap::const_iterator itr = m_Symbols.begin(); itr != m_Symbols.end(); itr++)
    {
        AsmjitLog.Replace(itr->first, itr->second);
    }
    m_CodeBlock.Log("      %s", AsmjitLog.c_str());
    return asmjit::kErrorOk;
}

#endif