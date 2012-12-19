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

#define CPU_Message(Message,... )  if (bX86Logging) { x86_Log_Message(Message,## __VA_ARGS__); }

void x86_Log_Message (const char * Message, ...);
void Start_x86_Log (void);
void Stop_x86_Log (void);

extern bool bX86Logging;
