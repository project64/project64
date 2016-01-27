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

#include <Project64-core/Logging.h>
#include <Project64-core/N64System/Mips/Eeprom.h>

class CPifRamSettings
{
protected:
    CPifRamSettings();
    virtual ~CPifRamSettings();

    bool  bShowPifRamErrors() const
    {
        return m_bShowPifRamErrors;
    }

private:
    static void RefreshSettings(void*);

    static bool m_bShowPifRamErrors;

    static int32_t m_RefCount;
};

class CPifRam :
    public CLogging,
    private CPifRamSettings,
    private CEeprom
{
public:
    CPifRam(bool SavesReadOnly);
    ~CPifRam();

    void Reset();

    void PifRamWrite();
    void PifRamRead();

    void SI_DMA_READ();
    void SI_DMA_WRITE();

protected:
    uint8_t m_PifRom[0x7C0];
    uint8_t m_PifRam[0x40];

private:
    CPifRam();                          // Disable default constructor
    CPifRam(const CPifRam&);            // Disable copy constructor
    CPifRam& operator=(const CPifRam&); // Disable assignment

    enum { CHALLENGE_LENGTH = 0x20 };
    void ProcessControllerCommand(int32_t Control, uint8_t * Command);
    void ReadControllerCommand(int32_t Control, uint8_t * Command);
    void LogControllerPakData(const char * Description);
    void n64_cic_nus_6105(char challenge[], char response[], int32_t length);
};
