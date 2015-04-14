/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

class CDMA :
	private CDebugSettings
{
	CDMA();

public:
	void SP_DMA_READ  ( void ); 
	void SP_DMA_WRITE ( void );
	void PI_DMA_READ  ( void );
	void PI_DMA_WRITE ( void );

protected:
	CDMA (CFlashram & FlashRam, CSram & Sram);

	//void SI_DMA_READ  ( void );
	//void SI_DMA_WRITE ( void );

private:
	CFlashram & m_FlashRam;
	CSram     & m_Sram;
	
	void OnFirstDMA   ( void );
};
