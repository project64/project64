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

class CGuiSettings
{
protected:
	CGuiSettings();
	virtual ~CGuiSettings();
	
	static inline bool bCPURunning ( void) { return m_bCPURunning; }
	static inline bool bAutoSleep  ( void) { return m_bAutoSleep;  }

private:
	static void RefreshSettings (void *);

	static bool m_bCPURunning;
	static bool m_bAutoSleep;
	
	static int  m_RefCount;

};
