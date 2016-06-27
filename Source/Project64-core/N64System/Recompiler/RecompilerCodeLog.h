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

/* vsprintf() needs to have both of these included. */
#include <stdio.h>
#include <stdarg.h>

#define CPU_Message(Message,... )  if (g_bRecompilerLogging) { Recompiler_Log_Message(Message,## __VA_ARGS__); }

void Recompiler_Log_Message (const char * Message, ...);
void Start_Recompiler_Log (void);
void Stop_Recompiler_Log (void);

extern bool g_bRecompilerLogging;
