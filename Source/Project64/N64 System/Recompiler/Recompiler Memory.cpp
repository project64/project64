#include "stdafx.h"

CRecompMemory::CRecompMemory() :
	m_RecompCode(NULL),
	m_RecompSize(0)
{
	m_RecompPos = NULL;
}

CRecompMemory::~CRecompMemory()
{
	if (m_RecompCode)
	{
		VirtualFree( m_RecompCode, 0 , MEM_RELEASE);
		m_RecompCode = NULL;
	}
	m_RecompPos = NULL;
}

bool CRecompMemory::AllocateMemory()
{
	BYTE * RecompCodeBase = (BYTE *)VirtualAlloc( NULL, MaxCompileBufferSize + 4, MEM_RESERVE|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
	if (RecompCodeBase==NULL) 
	{  
		_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return FALSE;
	}

	m_RecompCode = (BYTE *)VirtualAlloc( RecompCodeBase, InitialCompileBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (m_RecompCode==NULL) 
	{  
		VirtualFree( RecompCodeBase, 0 , MEM_RELEASE);
		_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return FALSE;
	}
	m_RecompSize = InitialCompileBufferSize;
	m_RecompPos = m_RecompCode;
	return true;
}

void CRecompMemory::CheckRecompMem ( void )
{
	int Size = (int)((BYTE *)m_RecompPos - (BYTE *)m_RecompCode);
	if ((Size + 0x20000) < m_RecompSize)
	{
		return;
	}
	if (m_RecompSize == MaxCompileBufferSize) 
	{ 
		return; 
	}
	LPVOID MemAddr = VirtualAlloc( m_RecompCode + m_RecompSize , IncreaseCompileBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	m_RecompSize += IncreaseCompileBufferSize;

	if (MemAddr == NULL) 
	{
		_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
	}
}
