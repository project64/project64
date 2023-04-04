#include "stdafx.h"

#include "RecompilerOps.h"

CRecompilerOpsBase::CRecompilerOpsBase(CMipsMemoryVM & MMU, CRegisters & Reg, CCodeBlock & CodeBlock) :
    m_Reg(Reg),
    m_MMU(MMU),
    m_CodeBlock(CodeBlock),
    m_Section(nullptr)
{
}

CRecompilerOpsBase::~CRecompilerOpsBase()
{
}
