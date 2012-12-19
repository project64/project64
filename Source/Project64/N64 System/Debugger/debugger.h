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
#pragma once

class CDumpMemory;
class CDebugMemoryView;
class CDebugMemorySearch;

class CDebugger
{
	CDumpMemory        * m_MemoryDump;
	CDebugMemoryView   * m_MemoryView;
	CDebugMemorySearch * m_MemorySearch;
	CDebugTlb          * m_DebugTLB;

protected:
	CDebugger();
	virtual ~CDebugger();
	
public:	
	
	void Debug_Reset              ( void );
	void Debug_ShowMemoryDump     ( void );
	void Debug_ShowMemoryWindow   ( void );
	void Debug_ShowMemoryLocation ( DWORD Address, bool VAddr );
	void Debug_ShowMemorySearch   ( void );
	void Debug_ShowTLBWindow      ( void );
	void Debug_RefreshTLBWindow   ( void );
};