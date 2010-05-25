#include "stdafx.h"

CRecompMemory::CRecompMemory() :
	m_RecompCode(0),
	m_RecompSize(0)
{
}

CRecompMemory::~CRecompMemory()
{
	if (m_RecompCode)
	{
		VirtualFree( m_RecompCode, 0 , MEM_RELEASE);
		m_RecompCode = NULL;
	}
}

bool CRecompMemory::AllocateMemory()
{
	BYTE * RecompCodeBase = (BYTE *)VirtualAlloc( NULL, MaxCompileBufferSize + 4, MEM_RESERVE|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
	if(RecompCodeBase==NULL) 
	{  
		_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return FALSE;
	}
	m_RecompCode = (BYTE *)VirtualAlloc( RecompCodeBase, InitialCompileBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(m_RecompCode==NULL) 
	{  
		VirtualFree( RecompCodeBase, 0 , MEM_RELEASE);
		_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return FALSE;
	}
	m_RecompSize = InitialCompileBufferSize;

	return true;
}