#pragma once
#include <Project64-core/Settings/DebugSettings.h>

#define CPU_Message(Message,... )  if (CDebugSettings::bRecordRecompilerAsm()) { Recompiler_Log_Message(Message,## __VA_ARGS__); }

void Recompiler_Log_Message (const char * Message, ...);
void Start_Recompiler_Log (void);
void Stop_Recompiler_Log(void); 
void Flush_Recompiler_Log(void);
