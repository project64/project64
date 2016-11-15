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

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Recompiler/x86/x86RegInfo.h>

typedef CX86RegInfo CRegInfo;

#elif defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Recompiler/x64-86/x64RegInfo.h>

typedef CX64RegInfo CRegInfo;

#elif defined(__arm__) || defined(_M_ARM)

#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>

typedef CArmRegInfo CRegInfo;

#endif
