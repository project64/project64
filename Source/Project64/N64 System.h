#ifndef __N64_SYSTEM__H__
#define __N64_SYSTEM__H__
#pragma warning(disable:4786)

#include "Support.h"
#include <string>				//needed for stl string (std::string)
#include "User Interface.h"
#include "N64 System/Types.h"

class CNotification;
#include "N64 System/Profiling Class.h"

//General Mips Information
#include "N64 System/N64 Rom Class.h"
#include "N64 System/Rom Information Class.h"
#include "N64 System/Speed Limitor Class.h"
#include "N64 System/Mips/OpCode.h"
#include "N64 System/Mips/Register Class.h"
#include "N64 System/Mips/TLB Class.h"
#include "N64 System/Mips/Memory Labels Class.h"
#include "N64 System/Mips/Memory Class.h"
#include "N64 System/Mips/OpCode Class.h"
#include "N64 System/Mips/Audio.h"

//Recompiler
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
