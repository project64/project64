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
#include <Project64/Settings/DebugSettings.h>
#include <Project64/N64System/Mips/FlashRam.h>
#include <Project64/N64System/Mips/Sram.h>

class CDMA :
    private CDebugSettings
{
    CDMA();

public:
    void SP_DMA_READ();
    void SP_DMA_WRITE();
    void PI_DMA_READ();
    void PI_DMA_WRITE();

protected:
    CDMA(CFlashram & FlashRam, CSram & Sram);

private:
    CDMA(const CDMA&);              // Disable copy constructor
    CDMA& operator=(const CDMA&);   // Disable assignment

    CFlashram & m_FlashRam;
    CSram     & m_Sram;

    void OnFirstDMA();
};
