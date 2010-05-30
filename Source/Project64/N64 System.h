#ifndef __N64_SYSTEM__H__
#define __N64_SYSTEM__H__
#pragma warning(disable:4786)

#include "Support.h"
#include <string>				//needed for stl string (std::string)
#include <float.h>
#include <math.h>

#include "User Interface.h"
#include "N64 System/Types.h"

class CNotification;
#include "N64 System/Profiling Class.h"

//General Mips Information
#include "N64 System/N64 Rom Class.h"
#include "N64 System/Rom Information Class.h"
#include "N64 System/Speed Limitor Class.h"
#include "N64 System/Mips/OpCode.h"
#include "N64 System/Mips/OpCode Analysis Class.h"
#include "N64 System/Recompiler/X86ops.h"
#include "N64 System/Mips/Register Class.h"
#include "N64 System/Mips/TranslateVaddr.h"
#include "N64 System/Mips/TLB Class.h"
#include "N64 System/Mips/Memory Labels Class.h"
#include "N64 System/Mips/Memory Class.h"
#include "N64 System/Mips/OpCode Class.h"
#include "N64 System/Mips/Audio.h"
#include "N64 System/Mips/System Timing.h"

//C Core - to be upgrdaded and removed
#include "N64 System/C Core/CPU Log.h"
#include "N64 System/C Core/r4300i Commands.h"

//Interpter
#include "N64 System/Interpreter/Interpreter Ops.h"
#include "N64 System/Interpreter/Interpreter Ops 32.h"
#include "N64 System/Interpreter/Interpreter CPU.h"

//Recompiler
#include "N64 System/Recompiler/Recompiler Memory.h"
#include "N64 System/Recompiler/Reg Info.h"
#include "N64 System/Recompiler/Recompiler Ops.h"
#include "N64 System/Recompiler/Exit Info.h"
#include "N64 System/Recompiler/Jump Info.h"
#include "N64 System/Recompiler/Code Section.h"
#include "N64 System/Recompiler/Code Block.h"
#include "N64 System/Recompiler/Section Info.h"
#include "N64 System/Recompiler/Function Info.h"
#include "N64 System/Recompiler/Function Map Class.h"
#include "N64 System/Recompiler/Delay Slot Map Class.h"
#include "N64 System/Recompiler/Recompiler Class.h"

//cheats
#include "N64 System/Cheat Class.h"

//Debugger
#include "N64 System/Debugger/Debugger.h"

//Main Files
#include "N64 System/N64 Class.h"
#include "N64 System/System Globals.h"

#endif
