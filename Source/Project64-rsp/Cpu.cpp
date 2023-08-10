#include "Cpu.h"
#include "cpu/RSPOpcode.h"
#include "Profiling.h"
#include "RSP Command.h"
#include "RSP registers.h"
#include "Recompiler CPU.h"
#include "Rsp.h"
#include "cpu/RspTypes.h"
#include "breakpoint.h"
#include "log.h"
#include "memory.h"
#include "x86.h"
#include <float.h>
#include <stdio.h>
#include <windows.h>

UDWORD EleSpec[16], Indx[16];
RSPOpcode RSPOpC;
uint32_t *PrgCount, NextInstruction, RSP_Running, RSP_MfStatusCount;

p_func RSP_Opcode[64];
p_func RSP_RegImm[32];
p_func RSP_Special[64];
p_func RSP_Cop0[32];
p_func RSP_Cop2[32];
p_func RSP_Vector[64];
p_func RSP_Lc2[32];
p_func RSP_Sc2[32];

void BuildInterpreterCPU(void);
void BuildRecompilerCPU(void);

extern HANDLE hMutex;
DWORD Mfc0Count, SemaphoreExit = 0;

void SetCPU(DWORD core)
{
    WaitForSingleObjectEx(hMutex, 1000 * 100, false);
    CPUCore = core;
    switch (core)
    {
    case RecompilerCPU:
        BuildRecompilerCPU();
        break;
    case InterpreterCPU:
        BuildInterpreterCPU();
        break;
    }
    ReleaseMutex(hMutex);
}

void Build_RSP(void)
{
    extern UWORD32 Recp, RecpResult, SQroot, SQrootResult;

    Recp.UW = 0;
    RecpResult.UW = 0;
    SQroot.UW = 0;
    SQrootResult.UW = 0;

    SetCPU(CPUCore);
    ResetTimerList();

    EleSpec[0].DW = 0x0706050403020100; // None
    EleSpec[1].DW = 0x0706050403020100; // None
    EleSpec[2].DW = 0x0606040402020000; // 0q
    EleSpec[3].DW = 0x0707050503030101; // 1q
    EleSpec[4].DW = 0x0404040400000000; // 0h
    EleSpec[5].DW = 0x0505050501010101; // 1h
    EleSpec[6].DW = 0x0606060602020202; // 2h
    EleSpec[7].DW = 0x0707070703030303; // 3h
    EleSpec[8].DW = 0x0000000000000000; // 0
    EleSpec[9].DW = 0x0101010101010101; // 1
    EleSpec[10].DW = 0x0202020202020202; // 2
    EleSpec[11].DW = 0x0303030303030303; // 3
    EleSpec[12].DW = 0x0404040404040404; // 4
    EleSpec[13].DW = 0x0505050505050505; // 5
    EleSpec[14].DW = 0x0606060606060606; // 6
    EleSpec[15].DW = 0x0707070707070707; // 7

    Indx[0].DW = 0x0001020304050607; // None
    Indx[1].DW = 0x0001020304050607; // None
    Indx[2].DW = 0x0103050700020406; // 0q
    Indx[3].DW = 0x0002040601030507; // 1q
    Indx[4].DW = 0x0102030506070004; // 0h
    Indx[5].DW = 0x0002030406070105; // 1h
    Indx[6].DW = 0x0001030405070206; // 2h
    Indx[7].DW = 0x0001020405060307; // 3h
    Indx[8].DW = 0x0102030405060700; // 0
    Indx[9].DW = 0x0002030405060701; // 1
    Indx[10].DW = 0x0001030405060702; // 2
    Indx[11].DW = 0x0001020405060703; // 3
    Indx[12].DW = 0x0001020305060704; // 4
    Indx[13].DW = 0x0001020304060705; // 5
    Indx[14].DW = 0x0001020304050706; // 6
    Indx[15].DW = 0x0001020304050607; // 7

    for (uint8_t i = 0, n = sizeof(EleSpec) / sizeof(EleSpec[0]); i < n; i++)
    {
        for (uint8_t z = 0; z < 8; z++)
        {
            Indx[i].B[z] = 7 - Indx[i].B[z];
        }
        for (uint8_t z = 0; z < 4; z++)
        {
            uint8_t Temp = Indx[i].B[z];
            Indx[i].B[z] = Indx[i].B[7 - z];
            Indx[i].B[7 - z] = Temp;
        }
    }
    PrgCount = RSPInfo.SP_PC_REG;
}

/*
Function: DoRspCycles
Purpose: This function is to allow the RSP to run in parallel with
the r4300i switching control back to the r4300i once the
function ends.
Input: The number of cycles that is meant to be executed.
Output: The number of cycles that was executed. This value can
be greater than the number of cycles that the RSP should have performed.
(this value is ignored if the RSP has been stopped)
*/

DWORD RunInterpreterCPU(DWORD Cycles);
DWORD RunRecompilerCPU(DWORD Cycles);

#define MI_INTR_SP 0x01 /* Bit 0: SP intr */

uint32_t DoRspCycles(uint32_t Cycles)
{
    extern bool AudioHle, GraphicsHle;
    DWORD TaskType = *(DWORD *)(RSPInfo.DMEM + 0xFC0);

    /*	if (*RSPInfo.SP_STATUS_REG & SP_STATUS_SIG0)
	{
		*RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG0;
		*RSPInfo.MI_INTR_REG |= MI_INTR_SP;
		RSPInfo.CheckInterrupts();
		return Cycles;
	}
*/
    if (TaskType == 1 && GraphicsHle && *(DWORD *)(RSPInfo.DMEM + 0x0ff0) != 0)
    {
        if (RSPInfo.ProcessDList != NULL)
        {
            RSPInfo.ProcessDList();
        }
        *RSPInfo.SP_STATUS_REG |= (0x0203);
        if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
            RSPInfo.CheckInterrupts();
        }

        *RSPInfo.DPC_STATUS_REG &= ~0x0002;
        return Cycles;
    }
    else if (TaskType == 2 && AudioHle)
    {
        if (RSPInfo.ProcessAList != NULL)
        {
            RSPInfo.ProcessAList();
        }
        *RSPInfo.SP_STATUS_REG |= (0x0203);
        if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
            RSPInfo.CheckInterrupts();
        }
        return Cycles;
    }
    else if (TaskType == 7)
    {
        RSPInfo.ShowCFB();
    }

    Compiler.bAudioUcode = (TaskType == 2) ? true : false;

    /*
	*RSPInfo.SP_STATUS_REG |= (0x0203 );
	if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 )
	{
		*RSPInfo.MI_INTR_REG |= MI_INTR_SP;
		RSPInfo.CheckInterrupts();
	}
	//return Cycles;
*/

    if (Profiling && !IndvidualBlock)
    {
        StartTimer((DWORD)Timer_RSP_Running);
    }

    WaitForSingleObjectEx(hMutex, 1000 * 100, false);

    if (BreakOnStart)
    {
        Enter_RSP_Commands_Window();
    }
    RSP_MfStatusCount = 0;

    switch (CPUCore)
    {
    case RecompilerCPU:
        RunRecompilerCPU(Cycles);
        break;
    case InterpreterCPU:
        RunInterpreterCPU(Cycles);
        break;
    }
    ReleaseMutex(hMutex);

    if (Profiling && !IndvidualBlock)
    {
        StartTimer((DWORD)Timer_R4300_Running);
    }

    return Cycles;
}
