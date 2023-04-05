#include "stdafx.h"
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/Arm/ArmOps.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>

CArmOps::CArmOps(CCodeBlock & CodeBlock) :
    asmjit::a64::Assembler(&CodeBlock.CodeHolder()),
    m_CodeBlock(CodeBlock)
{
}

asmjit::Error CArmOps::_log(const char * data, size_t size) noexcept
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