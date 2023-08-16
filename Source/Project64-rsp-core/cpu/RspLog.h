#pragma once
#include <stdint.h>

void StartCPULog(void);
void StopCPULog(void);
void CPU_Message(const char * Message, ...);

void StartRDPLog(void);
void StopRDPLog(void);
void RDP_Message(const char * Message, ...);
void RDP_LogDlist(void);
void RDP_LogMT0(uint32_t PC, int Reg, uint32_t Value);
void RDP_LogMF0(uint32_t PC, int Reg);
void RDP_LogLoc(uint32_t PC);