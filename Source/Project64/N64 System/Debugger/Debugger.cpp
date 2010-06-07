#include "stdafx.h"
#include "Debugger UI.h"

CPj64Module _Module;

CDebugger::CDebugger () :
	m_MemoryDump(NULL),
	m_MemoryView(NULL),
	m_MemorySearch(NULL),
	m_DebugTLB(NULL)
{
}

CDebugger::~CDebugger (void)
{
	Debug_Reset();
}

void CDebugger::Debug_Reset ( void )
{
	if (m_MemoryDump)
	{
		m_MemoryDump->HideWindow();
		delete m_MemoryDump;
		m_MemoryDump = NULL;
	}
	if (m_MemoryView)
	{
		m_MemoryView->HideWindow();
		delete m_MemoryView;
		m_MemoryView = NULL;
	}
	if (m_MemorySearch)
	{
		m_MemorySearch->HideWindow();
		delete m_MemorySearch;
		m_MemorySearch = NULL;
	}
	if (m_DebugTLB)
	{
		m_DebugTLB->HideWindow();
		delete m_DebugTLB;
		m_DebugTLB = NULL;
	}
}

void CDebugger::Debug_ShowMemoryDump()
{
	if (_MMU == NULL)
	{
		return;
	}
	if (m_MemoryDump == NULL)
	{
		m_MemoryDump = new CDumpMemory(this);
	}
	if (m_MemoryDump)
	{
		m_MemoryDump->ShowWindow();
	}
}

void CDebugger::Debug_ShowMemoryWindow ( void )
{
	if (_MMU == NULL)
	{
		return;
	}
	if (m_MemoryView == NULL)
	{
		m_MemoryView = new CDebugMemoryView(this);
	}
	if (m_MemoryView)
	{
		m_MemoryView->ShowWindow();
	}
}

void CDebugger::Debug_ShowMemoryLocation ( DWORD Address, bool VAddr )
{
	Debug_ShowMemoryWindow();
	if (m_MemoryView)
	{
		m_MemoryView->ShowAddress(Address,VAddr);
	}	
}

void CDebugger::Debug_ShowTLBWindow (void)
{
	if (_MMU == NULL)
	{
		return;
	}
	if (m_DebugTLB == NULL)
	{
		m_DebugTLB = new CDebugTlb(this);
	}
	if (m_DebugTLB)
	{
		m_DebugTLB->ShowWindow();
	}
}

void CDebugger::Debug_RefreshTLBWindow(void)
{
	if (m_DebugTLB)
	{
		m_DebugTLB->RefreshTLBWindow();
	}
}

void CDebugger::Debug_ShowMemorySearch()
{
	if (m_MemorySearch == NULL)
	{
		m_MemorySearch = new CDebugMemorySearch(this);
	}
	if (m_MemorySearch)
	{
		m_MemorySearch->ShowWindow();
	}
}