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

class CNotification;
#include "N64System/ProfilingClass.h"

//General Mips Information
#include "N64System/N64RomClass.h"
#include "N64System/RomInformationClass.h"
#include "N64System/SpeedLimitorClass.h"
#include "N64System/Mips/OpCode.h"
#include "N64System/Recompiler/X86ops.h"
#include "N64System/Mips/Mempak.h"
#include "N64System/Mips/Rumblepak.h"
#include "N64System/Mips/FlashRam.h"
#include "N64System/Mips/Sram.h"
#include "N64System/Mips/Eeprom.h"
#include "N64System/Mips/Dma.h"
#include "N64System/Mips/PifRam.h"
#include "N64System/Mips/RegisterClass.h"
#include "N64System/Mips/TranslateVaddr.h"
#include "N64System/Mips/TLBClass.h"
#include "N64System/Mips/MemoryLabelsClass.h"
#include "N64System/Mips/MemoryClass.h"
#include "N64System/Mips/Audio.h"
#include "N64System/Mips/SystemTiming.h"
#include "N64System/Mips/SystemEvents.h"

//C Core - to be upgrdaded and removed
#include "N64System/C_Core/r4300iCommands.h"

//Interpter
#include "N64System/Interpreter/InterpreterOps.h"
#include "N64System/Interpreter/InterpreterOps32.h"
#include "N64System/Interpreter/InterpreterCPU.h"

//Recompiler
#include "N64System/Recompiler/RecompilerMemory.h"
#include "N64System/Recompiler/RegInfo.h"
#include "N64System/Recompiler/LoopAnalysis.h"
#include "N64System/Recompiler/RecompilerOps.h"
#include "N64System/Mips/MemoryVirtualMem.h" //needs to inherit Recompiler ops
#include "N64System/Recompiler/ExitInfo.h"
#include "N64System/Recompiler/JumpInfo.h"
#include "N64System/Recompiler/CodeSection.h"
#include "N64System/Recompiler/CodeBlock.h"
#include "N64System/Recompiler/SectionInfo.h"
#include "N64System/Recompiler/FunctionInfo.h"
#include "N64System/Recompiler/FunctionMapClass.h"
#include "N64System/Recompiler/RecompilerClass.h"
#include "N64System/Recompiler/x86CodeLog.h"

//cheats
#include "N64System/CheatClass.h"

//Debugger
#include "N64System/Debugger/Debugger.h"

//Main Files
#include "N64System/N64Class.h"
#include "N64System/SystemGlobals.h"
