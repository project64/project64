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
#include <Project64-core/Settings/DebugSettings.h>

class CFlashram :
    private CDebugSettings
{
    enum Modes
    {
        FLASHRAM_MODE_NOPES = 0,
        FLASHRAM_MODE_ERASE = 1,
        FLASHRAM_MODE_WRITE = 2,
        FLASHRAM_MODE_READ = 3,
        FLASHRAM_MODE_STATUS = 4,
    };

public:
    CFlashram(bool ReadOnly);
    ~CFlashram();

    void  DmaFromFlashram(uint8_t * dest, int StartOffset, int len);
    void  DmaToFlashram(uint8_t * Source, int StartOffset, int len);
    uint32_t ReadFromFlashStatus(uint32_t PAddr);
    void     WriteToFlashCommand(uint32_t Value);

private:
    bool  LoadFlashram();

    uint8_t * m_FlashRamPointer;
    Modes     m_FlashFlag;
    uint64_t  m_FlashStatus;
    uint32_t  m_FlashRAM_Offset;
    bool      m_ReadOnly;
    void *    m_hFile;
};
