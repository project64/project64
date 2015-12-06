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
#include <Project64-core/Settings/LoggingSettings.h>
#include <Common/FileClass.h>

class CLogging :
    public CLogSettings
{
public:
    static void StartLog(void);
    static void StopLog(void);

    static void Log_LW(uint32_t PC, uint32_t VAddr);
    static void Log_SW(uint32_t PC, uint32_t VAddr, uint32_t Value);
    static void LogMessage(const char * Message, ...);

private:
    static CFile * m_hLogFile;
};