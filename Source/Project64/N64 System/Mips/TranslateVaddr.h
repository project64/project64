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

class CTransVaddr
{
public:
	virtual bool TranslateVaddr  ( DWORD VAddr, DWORD &PAddr) const  = 0;
	virtual bool ValidVaddr      ( DWORD VAddr ) const = 0;
	virtual bool VAddrToRealAddr ( DWORD VAddr, void * &RealAddress ) const = 0;
};
