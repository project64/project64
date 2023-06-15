#include <windows.h>

void StartCPULog(void);
void StopCPULog(void);
void CPU_Message(const char * Message, ...);

void StartRDPLog(void);
void StopRDPLog(void);
void RDP_Message(const char * Message, ...);
void RDP_LogDlist(void);
void RDP_LogMT0(DWORD PC, int Reg, DWORD Value);
void RDP_LogMF0(DWORD PC, int Reg);
void RDP_LogLoc(DWORD PC);