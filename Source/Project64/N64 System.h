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

#include "Support.h"
#include <string>				//needed for stl string (std::string)
#include <float.h>
#include <math.h>

#include "User Interface.h"

class CNotification;
#include "N64 System/Profiling Class.h"

//General Mips Information
#include "N64 System/N64 Rom Class.h"
#include "N64 System/Rom Information Class.h"
#include "N64 System/Speed Limitor Class.h"
#include "N64 System/Mips/OpCode.h"
#include "N64 System/Recompiler/X86ops.h"
#include "N64 System/Mips/Mempak.h"
#include "N64 System/Mips/Rumblepak.h"
#include "N64 System/Mips/FlashRam.h"
#include "N64 System/Mips/Sram.h"
#include "N64 System/Mips/Eeprom.h"
#include "N64 System/Mips/Dma.h"
#include "N64 System/Mips/Pif Ram.h"
#include "N64 System/Mips/Register Class.h"
#include "N64 System/Mips/TranslateVaddr.h"
#include "N64 System/Mips/TLB Class.h"
#include "N64 System/Mips/Memory Labels Class.h"
#include "N64 System/Mips/Memory Class.h"
#include "N64 System/Mips/Audio.h"
#include "N64 System/Mips/System Timing.h"
#include "N64 System/Mips/System Events.h"

//C Core - to be upgrdaded and removed
#include "N64 System/C Core/r4300i Commands.h"

//Interpter
#include "N64 System/Interpreter/Interpreter Ops.h"
#include "N64 System/Interpreter/Interpreter Ops 32.h"
#include "N64 System/Interpreter/Interpreter CPU.h"

//Recompiler
#include "N64 System/Recompiler/Recompiler Memory.h"
#include "N64 System/Recompiler/Reg Info.h"
#include "N64 System/Recompiler/Loop Analysis.h"
#include "N64 System/Recompiler/Recompiler Ops.h"
#include "N64 System/Mips/Memory Virtual Mem.h" //needs to inherit Recompiler ops
#include "N64 System/Recompiler/Exit Info.h"
#include "N64 System/Recompiler/Jump Info.h"
#include "N64 System/Recompiler/Code Section.h"
#include "N64 System/Recompiler/Code Block.h"
#include "N64 System/Recompiler/Section Info.h"
#include "N64 System/Recompiler/Function Info.h"
#include "N64 System/Recompiler/Function Map Class.h"
#include "N64 System/Recompiler/Recompiler Class.h"
#include "N64 System/Recompiler/x86CodeLog.h"

//cheats
#include "N64 System/Cheat Class.h"

//Debugger
#include "N64 System/Debugger/Debugger.h"

//Main Files
#include "N64 System/N64 Class.h"
#include "N64 System/System Globals.h"
