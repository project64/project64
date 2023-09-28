#include "RSPOpcode.h"
#include "RSPRegisterHandlerPlugin.h"
#include "RspTypes.h"
#include <memory>

enum RSPCpuType
{
    InterpreterCPU = 0,
    RecompilerCPU = 1,
};

extern UDWORD EleSpec[16], Indx[16];

typedef void (*p_func)(void);

extern p_func RSP_Opcode[64];
extern p_func RSP_RegImm[32];
extern p_func RSP_Special[64];
extern p_func RSP_Cop0[32];
extern p_func RSP_Cop2[32];
extern p_func RSP_Vector[64];
extern p_func RSP_Lc2[32];
extern p_func RSP_Sc2[32];
extern uint32_t *PrgCount, RSP_Running;
extern RSPOpcode RSPOpC;

void SetCPU(RSPCpuType core);
void Build_RSP(void);

extern uint32_t Mfc0Count, SemaphoreExit;
extern RSPCpuType g_CPUCore;
extern std::unique_ptr<RSPRegisterHandlerPlugin> g_RSPRegisterHandler;
