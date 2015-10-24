/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

CFunctionMap::CFunctionMap() :
	m_JumpTable(NULL),
	m_FunctionTable(NULL)
{
}

CFunctionMap::~CFunctionMap()
{
	CleanBuffers();
}

bool CFunctionMap::AllocateMemory()
{
	if (g_System->LookUpMode() == FuncFind_VirtualLookup && m_FunctionTable == NULL)
	{
		m_FunctionTable = (PCCompiledFunc_TABLE *)VirtualAlloc(NULL,0xFFFFF * sizeof(CCompiledFunc *),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		if (m_FunctionTable == NULL) {
			WriteTrace(TraceError,__FUNCTION__ ": failed to allocate function table");
			g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
			return false;
		}
		memset(m_FunctionTable,0,0xFFFFF * sizeof(CCompiledFunc *));
	}
	if (g_System->LookUpMode() == FuncFind_PhysicalLookup && m_JumpTable == NULL)
	{
		m_JumpTable = new PCCompiledFunc[g_MMU->RdramSize() >> 2];
		if (m_JumpTable == NULL) 
		{
			WriteTrace(TraceError,__FUNCTION__ ": failed to allocate jump table");
			g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
			return false;
		}
		memset(m_JumpTable,0,(g_MMU->RdramSize() >> 2) * sizeof(PCCompiledFunc));
	}
	return true;
}

void CFunctionMap::CleanBuffers()
{
	if (m_FunctionTable)
	{
		for (int i = 0, n = 0x100000; i < n; i++)
		{
			if (m_FunctionTable[i] != NULL)
			{
				delete m_FunctionTable[i];
			}
		}
		VirtualFree( m_FunctionTable, 0 , MEM_RELEASE);
		m_FunctionTable = NULL;
	}
	if (m_JumpTable)
	{
		delete [] m_JumpTable;
		m_JumpTable = NULL;
	}
}

void CFunctionMap::Reset ( bool bAllocate )
{
	CleanBuffers();
	if (bAllocate && (g_System->LookUpMode() == FuncFind_VirtualLookup || g_System->LookUpMode() == FuncFind_PhysicalLookup))
	{
		AllocateMemory();
	}
}
