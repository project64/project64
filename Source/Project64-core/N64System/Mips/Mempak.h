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

class CMempak
{
public:
    CMempak();

    void ReadFrom(int32_t Control, uint32_t address, uint8_t * data);
    void WriteTo(int32_t Control, uint32_t address, uint8_t * data);

    static uint8_t CalculateCrc(uint8_t * DataToCrc);

private:
    CMempak(const CMempak&);				// Disable copy constructor
    CMempak& operator=(const CMempak&);		// Disable assignment

    void LoadMempak(int32_t Control, bool Create);
    void Format(int32_t Control);

    uint8_t m_Mempaks[4][128 * 256]; /* [CONTROLLERS][PAGES][BYTES_PER_PAGE] */
    CFile m_MempakHandle[4];
    bool m_Formatted[4];
    bool m_SaveExists[4];
};