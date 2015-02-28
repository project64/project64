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

struct MempakFile
{
	HANDLE hMemPakHandle;		// Handle for our mempak files
	BYTE aMemPakData[0x8000];	// data stored in the mem pak, 0x8000 = 32kb;
};

class Mempak
{
public:
	static void Close		 ( void );
	static BYTE CalculateCrc ( BYTE * DataToCrc );
	static void ReadFrom	 ( int Control, int Address, BYTE * Buffer );
	static void WriteTo		 ( int Control, int Address, BYTE * Buffer );

};
