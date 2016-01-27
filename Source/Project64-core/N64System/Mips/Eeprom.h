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

class CEeprom :
    private CDebugSettings
{
public:
    CEeprom(bool ReadOnly);
    ~CEeprom();

    void EepromCommand(uint8_t * Command);

private:
    CEeprom(void);                        // Disable default constructor
    CEeprom(const CEeprom&);              // Disable copy constructor
    CEeprom& operator=(const CEeprom&);   // Disable assignment

    void LoadEeprom();
    void ReadFrom(uint8_t * Buffer, int32_t line);
    void WriteTo(uint8_t * Buffer, int32_t line);

    uint8_t m_EEPROM[0x800];
    bool    m_ReadOnly;
    CFile   m_File;
};
