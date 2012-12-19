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

class CSram 
{
public:
	CSram ( bool ReadOnly );
   ~CSram ( void );

	void DmaFromSram ( BYTE * dest, int StartOffset, int len);
	void DmaToSram   ( BYTE * Source, int StartOffset, int len);

private:
	BOOL LoadSram    ( void );

	bool   m_ReadOnly;
	HANDLE m_hFile;
};