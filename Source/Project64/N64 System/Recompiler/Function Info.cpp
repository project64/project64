#include "stdafx.h"

CCompiledFunc::CCompiledFunc( const CCodeBlock & CodeBlock ) :
	m_EnterPC(CodeBlock.VAddrEnter()),
	m_MinPC(CodeBlock.VAddrFirst()),
	m_MaxPC(CodeBlock.VAddrLast()),
	m_Function((Func)CodeBlock.CompiledLocation()),
	m_Hash(CodeBlock.Hash()),
	m_Next(NULL)
{
}

