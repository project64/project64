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

class CSram
{
public:
    CSram(bool ReadOnly);
    ~CSram();

    void DmaFromSram(uint8_t * dest, int32_t StartOffset, int32_t len);
    void DmaToSram(uint8_t * Source, int32_t StartOffset, int32_t len);

private:
    bool LoadSram();

    bool   m_ReadOnly;
    void * m_hFile;
};
