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

#include <stdafx.h>

struct DMALOGENTRY
{
    uint32_t romAddr;
    uint32_t ramAddr;
    uint32_t length;
};

class CDMALog
{
private:
    vector<DMALOGENTRY> m_Log;

public:
    void         AddEntry(uint32_t romAddr, uint32_t ramAddr, uint32_t length);
    void         ClearEntries();
    size_t       GetNumEntries();
    DMALOGENTRY* GetEntryByIndex(uint32_t index);
    DMALOGENTRY* GetEntryByRamAddress(uint32_t ramAddr);
    DMALOGENTRY* GetEntryByRamAddress(uint32_t ramAddr, uint32_t* lpRomAddr, uint32_t* lpOffset);
    DMALOGENTRY* GetEntryByRomAddress(uint32_t romAddr);
    DMALOGENTRY* GetEntryByRomAddress(uint32_t romAddr, uint32_t* lpRamAddr, uint32_t* lpOffset);
};