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
#include "stdafx.h"
#include "RecompilerCodeLog.h"
#include <Common/path.h>
#include <Common/Platform.h>

/* vsprintf() */
#include <stdio.h>
#include <stdarg.h>

static CLog * g_CPULogFile = NULL;

void Recompiler_Log_Message(const char * strFormat, ...)
{
    va_list args;
    va_start(args, strFormat);
    size_t nlen = _vscprintf(strFormat, args) + 3;
    char * buffer = (char *)alloca(nlen * sizeof(char));
    if (buffer != NULL)
    {
        buffer[nlen - 1] = 0;
        vsprintf(buffer, strFormat, args);
        strcat(buffer, "\r\n");
        g_CPULogFile->Log(buffer);
    }
    va_end(args);
}

void Start_Recompiler_Log (void)
{
    CPath LogFileName(g_Settings->LoadStringVal(Directory_Log).c_str(), "CPUoutput.log");
    if (g_CPULogFile != NULL)
    {
        Stop_Recompiler_Log();
    }
    g_CPULogFile = new CLog();
    if (g_CPULogFile)
    {
        if (g_CPULogFile->Open(LogFileName))
        {
            g_CPULogFile->SetMaxFileSize(300 * CLog::MB);
        }
        else
        {
            delete g_CPULogFile;
            g_CPULogFile = NULL;
        }
    }
}

void Stop_Recompiler_Log (void)
{
    if (g_CPULogFile != NULL)
    {
        delete g_CPULogFile;
        g_CPULogFile = NULL;
    }
}

void Flush_Recompiler_Log(void)
{
    if (g_CPULogFile != NULL)
    {
        g_CPULogFile->Flush();
    }
}
