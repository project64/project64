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
		VirtualFree(m_RecompCode, 0, MEM_RELEASE);
		m_RecompCode = NULL;
	}
	m_RecompPos = NULL;
}

bool CRecompMemory::AllocateMemory()
{
	BYTE * RecompCodeBase = (BYTE *)VirtualAlloc(NULL, MaxCompileBufferSize + 4, MEM_RESERVE | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
	if (RecompCodeBase == NULL)
	{
		WriteTrace(TraceError, __FUNCTION__ ": failed to allocate RecompCodeBase");
		g_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return false;
	}

	m_RecompCode = (BYTE *)VirtualAlloc(RecompCodeBase, InitialCompileBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (m_RecompCode == NULL)
	{
		WriteTrace(TraceError, __FUNCTION__ ": failed to commit initial buffer");
		VirtualFree(RecompCodeBase, 0, MEM_RELEASE);
		g_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return false;
	}
	m_RecompSize = InitialCompileBufferSize;
	m_RecompPos = m_RecompCode;
	memset(m_RecompCode, 0, InitialCompileBufferSize);
	return true;
}

void CRecompMemory::CheckRecompMem()
{
	DWORD Size = (DWORD)((BYTE *)m_RecompPos - (BYTE *)m_RecompCode);
	if ((Size + 0x20000) < m_RecompSize)
	{
		return;
	}
	if (m_RecompSize == MaxCompileBufferSize)
	{
		g_Recompiler->ResetRecompCode(true);
		return;
	}
	LPVOID MemAddr = VirtualAlloc(m_RecompCode + m_RecompSize, IncreaseCompileBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (MemAddr == NULL)
	{
		WriteTrace(TraceError, __FUNCTION__ ": failed to increase buffer");
		g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
	}
	m_RecompSize += IncreaseCompileBufferSize;
}

void CRecompMemory::Reset()
{
	m_RecompPos = m_RecompCode;
}

void CRecompMemory::ShowMemUsed()
{
	DWORD Size = m_RecompPos - m_RecompCode;
	DWORD MB = Size / 0x100000;
	Size -= MB * 0x100000;
	DWORD KB = Size / 1024;
	Size -= KB * 1024;

	DWORD TotalAvaliable = m_RecompSize / 0x100000;

	g_Notify->DisplayMessage(0, stdstr_f("Memory used: %d mb %-3d kb %-3d bytes     Total Available: %d mb", MB, KB, Size, TotalAvaliable).ToUTF16().c_str());
}
