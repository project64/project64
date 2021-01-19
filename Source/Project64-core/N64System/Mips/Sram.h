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

    void DmaFromSram(uint8_t * dest, int32_t StartOffset, uint32_t len);
    void DmaToSram(uint8_t * Source, int32_t StartOffset, uint32_t len);

private:
    CSram(void);                        // Disable default constructor
    CSram(const CSram&);              // Disable copy constructor
    CSram& operator=(const CSram&);   // Disable assignment

    bool LoadSram();

    bool m_ReadOnly;
    CFile m_File;
};
