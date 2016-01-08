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

#include "UserInterface.h"
#include <Project64-core/N64System/N64Types.h>

#include <Project64-core/N64System/ProfilingClass.h>

//General Mips Information
#include <Project64-core/N64System/N64RomClass.h>
#include <Project64-core/N64System/SpeedLimiterClass.h>
#include <Project64-core/N64System/Mips/OpCode.h>
#include <Project64-core/N64System/Recompiler/X86ops.h>
#include <Project64-core/N64System/Mips/Mempak.h>
#include <Project64-core/N64System/Mips/Rumblepak.h>
#include <Project64-core/N64System/Mips/FlashRam.h>
#include <Project64-core/N64System/Mips/Sram.h>
#include <Project64-core/N64System/Mips/Eeprom.h>
#include <Project64-core/N64System/Mips/Dma.h>
#include <Project64-core/N64System/Mips/PifRam.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Mips/TranslateVaddr.h>
#include <Project64-core/N64System/Mips/TLBClass.h>
#include <Project64-core/N64System/Mips/MemoryLabelsClass.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/Audio.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Mips/SystemEvents.h>

//Interpter
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps32.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>

//Recompiler
#include <Project64-core/N64System/Recompiler/RecompilerMemory.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/Recompiler/LoopAnalysis.h>
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h> //needs to inherit Recompiler ops
#include <Project64-core/N64System/Recompiler/ExitInfo.h>
#include <Project64-core/N64System/Recompiler/JumpInfo.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/SectionInfo.h>
#include <Project64-core/N64System/Recompiler/FunctionInfo.h>
#include <Project64-core/N64System/Recompiler/FunctionMapClass.h>
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <Project64-core/N64System/Recompiler/x86CodeLog.h>

//cheats
#include <Project64-core/N64System/CheatClass.h>

//Debugger
#include "N64System/Debugger/Debugger.h"

//Main Files
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/SystemGlobals.h>
