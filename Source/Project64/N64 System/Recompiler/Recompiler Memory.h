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

class CRecompMemory :
	protected CX86Ops
{
protected:
	CRecompMemory();
	~CRecompMemory();

	bool AllocateMemory ( void );
	void CheckRecompMem ( void );
	void Reset          ( void );
	void ShowMemUsed    ( void );
	
	inline BYTE * RecompPos ( void ) const { return m_RecompPos; }

private:
	BYTE          * m_RecompCode;
	DWORD           m_RecompSize;
	
	enum { MaxCompileBufferSize      = 0x03C00000 };
	enum { InitialCompileBufferSize  = 0x00500000 };
	enum { IncreaseCompileBufferSize = 0x00100000 };
};
