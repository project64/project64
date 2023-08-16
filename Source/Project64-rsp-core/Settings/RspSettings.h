#pragma once
#include <stdint.h>

void InitializeRspSetting(void);

extern uint16_t Set_AudioHle, Set_GraphicsHle;
extern bool GraphicsHle, AudioHle, ConditionalMove;
extern bool DebuggingEnabled, Profiling, IndvidualBlock, ShowErrors, BreakOnStart, LogRDP, LogX86Code;