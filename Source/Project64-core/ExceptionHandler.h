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

#ifdef _WIN32
#include <excpt.h>

#define __except_try() __try
#define __except_catch() __except (g_MMU->MemoryFilter(_exception_code(), _exception_info()))
#else
#define __except_try() __try
#define __except_catch() __catch (...)
#endif
