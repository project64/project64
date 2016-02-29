/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

__interface CTransVaddr
{
    virtual bool TranslateVaddr  ( uint32_t VAddr, uint32_t &PAddr) const  = 0;
    virtual bool ValidVaddr      ( uint32_t VAddr ) const = 0;
    virtual bool VAddrToRealAddr ( uint32_t VAddr, void * &RealAddress ) const = 0;
};
