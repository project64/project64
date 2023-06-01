#include "OpCode.h"
#include <Windows.h>

extern UDWORD EleSpec[32], Indx[32];

extern p_func RSP_Opcode[64];
extern p_func RSP_RegImm[32];
extern p_func RSP_Special[64];
extern p_func RSP_Cop0[32];
extern p_func RSP_Cop2[32];
extern p_func RSP_Vector[64];
extern p_func RSP_Lc2[32];
extern p_func RSP_Sc2[32];
extern uint32_t *PrgCount, RSP_Running;
extern OPCODE RSPOpC;

void SetCPU(DWORD core);
void Build_RSP(void);

extern DWORD Mfc0Count, SemaphoreExit;
