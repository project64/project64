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

class Mempak
{
public:
	static void Close();
	static BYTE CalculateCrc(BYTE * DataToCrc);
	static void ReadFrom(int Control, BYTE * command);
	static void WriteTo(int Control, BYTE * command);

};
