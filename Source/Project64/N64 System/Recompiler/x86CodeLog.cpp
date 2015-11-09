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
#include "stdafx.h"

static HANDLE hCPULogFile = NULL;
bool bX86Logging = false;

void x86_Log_Message (const char * Message, ...)
{
    DWORD dwWritten;
    char Msg[400];

    va_list ap;
    va_start( ap, Message );
    vsprintf( Msg, Message, ap );
    va_end( ap );

    strcat(Msg,"\r\n");

    WriteFile( hCPULogFile,Msg,strlen(Msg),&dwWritten,NULL );
}

void Start_x86_Log (void)
{
    CPath LogFileName(CPath::MODULE_DIRECTORY);
    LogFileName.AppendDirectory("Logs");
    LogFileName.SetNameExtension("CPUoutput.log");

    if (hCPULogFile) { Stop_x86_Log(); }
    hCPULogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hCPULogFile)
    {
        bX86Logging = true;
        SetFilePointer(hCPULogFile,0,NULL,FILE_BEGIN);
    }
}

void Stop_x86_Log (void)
{
    if (hCPULogFile)
    {
        CloseHandle(hCPULogFile);
        hCPULogFile = NULL;
        bX86Logging = false;
    }
}
