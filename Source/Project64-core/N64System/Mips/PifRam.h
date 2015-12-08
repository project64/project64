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
#include "Eeprom.h"

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

	static int  m_RefCount;

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
	#define CHALLENGE_LENGTH 0x20
	void ProcessControllerCommand ( int Control, uint8_t * Command );
	void ReadControllerCommand    ( int Control, uint8_t * Command );
	void LogControllerPakData     ( char * Description );
	void n64_cic_nus_6105         (char challenge[], char response[], int length);
};
