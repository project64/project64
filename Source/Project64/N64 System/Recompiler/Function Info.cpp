#include "stdafx.h"

CCompiledFunc::CCompiledFunc( const CCodeBlock & CodeBlock ) :
	m_EnterPC(CodeBlock.VAddrEnter()),
	m_MinPC(CodeBlock.VAddrFirst()),
	m_MaxPC(CodeBlock.VAddrLast()),
	m_Function((Func)CodeBlock.CompiledLocation()),
	m_Hash(CodeBlock.Hash()),
	m_Next(NULL)
{
	m_MemContents[0] = CodeBlock.MemContents(0);
	m_MemContents[1] = CodeBlock.MemContents(1);
	m_MemLocation[0] = CodeBlock.MemLocation(0);
	m_MemLocation[1] = CodeBlock.MemLocation(1);
}

