#pragma once
#include <Project64-plugin-spec/Rsp.h>

uint32_t AsciiToHex(char * HexValue);
void DisplayError(char * Message, ...);

extern DEBUG_INFO DebugInfo;
extern void * hinstDLL;