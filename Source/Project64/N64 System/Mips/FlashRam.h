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

class CFlashram :
	private CDebugSettings
{
	enum Modes
	{
		FLASHRAM_MODE_NOPES  = 0,
		FLASHRAM_MODE_ERASE  = 1,
		FLASHRAM_MODE_WRITE  = 2,
		FLASHRAM_MODE_READ   = 3,
		FLASHRAM_MODE_STATUS = 4,
	};

public:
	CFlashram(bool ReadOnly);
	~CFlashram();

	void  DmaFromFlashram     ( BYTE * dest, int StartOffset, int len );
	void  DmaToFlashram       ( BYTE * Source, int StartOffset, int len );
	DWORD ReadFromFlashStatus ( DWORD PAddr );
	void  WriteToFlashCommand ( DWORD Value );

private:
	bool  LoadFlashram();

	BYTE * m_FlashRamPointer;
	Modes  m_FlashFlag;
	QWORD  m_FlashStatus;
	DWORD  m_FlashRAM_Offset;
	bool   m_ReadOnly;
	HANDLE m_hFile;
};
