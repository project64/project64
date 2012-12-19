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

class CEeprom 
{
public:
	CEeprom ( bool ReadOnly );
   ~CEeprom ( void );

   void EepromCommand ( BYTE * Command );

private:
	void LoadEeprom ( void );
	void ReadFrom   ( BYTE * Buffer, int line );
	void WriteTo    ( BYTE * Buffer, int line );

	BYTE   m_EEPROM[0x800];
	bool   m_ReadOnly;
	HANDLE m_hFile;
};
