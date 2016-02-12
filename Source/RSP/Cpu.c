/*
 * RSP Compiler plug in for Project64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
 *
 * pj64 homepage: www.pj64.net
 * 
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#include <windows.h>
#include <stdio.h>
#include <float.h>
#include "Rsp.h"
#include "Cpu.h"
#include "RSP registers.h"
#include "RSP Command.h"
#include "Recompiler CPU.h"
#include "memory.h"
#include "opcode.h"
#include "log.h"
#include "Profiling.h"
#include "breakpoint.h"
#include "x86.h"

UDWORD EleSpec[32], Indx[32];
OPCODE RSPOpC;
DWORD *PrgCount, NextInstruction, RSP_Running, RSP_MfStatusCount;

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

void SetCPU(DWORD core)
{
	WaitForSingleObjectEx(hMutex, 1000 * 100, FALSE);
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

void Build_RSP ( void )
{
	int i;
	extern UWORD32 Recp, RecpResult, SQroot, SQrootResult;

	Recp.UW = 0;
	RecpResult.UW = 0;
	SQroot.UW = 0;
	SQrootResult.UW = 0;

	SetCPU(CPUCore);
	ResetTimerList();

	EleSpec[ 0].DW = 0;
	EleSpec[ 1].DW = 0;
	EleSpec[ 2].DW = 0;
	EleSpec[ 3].DW = 0;
	EleSpec[ 4].DW = 0;
	EleSpec[ 5].DW = 0;
	EleSpec[ 6].DW = 0;
	EleSpec[ 7].DW = 0;
	EleSpec[ 8].DW = 0;
	EleSpec[ 9].DW = 0;
	EleSpec[10].DW = 0;
	EleSpec[11].DW = 0;
	EleSpec[12].DW = 0;
	EleSpec[13].DW = 0;
	EleSpec[14].DW = 0;
	EleSpec[15].DW = 0;
	EleSpec[16].DW = 0x0001020304050607; /* None */
	EleSpec[17].DW = 0x0001020304050607; /* None */
	EleSpec[18].DW = 0x0000020204040606; /* 0q */
	EleSpec[19].DW = 0x0101030305050707; /* 1q */
	EleSpec[20].DW = 0x0000000004040404; /* 0h */
	EleSpec[21].DW = 0x0101010105050505; /* 1h */
	EleSpec[22].DW = 0x0202020206060606; /* 2h */
	EleSpec[23].DW = 0x0303030307070707; /* 3h */
	EleSpec[24].DW = 0x0000000000000000; /* 0 */
	EleSpec[25].DW = 0x0101010101010101; /* 1 */
	EleSpec[26].DW = 0x0202020202020202; /* 2 */
	EleSpec[27].DW = 0x0303030303030303; /* 3 */
	EleSpec[28].DW = 0x0404040404040404; /* 4 */
	EleSpec[29].DW = 0x0505050505050505; /* 5 */
	EleSpec[30].DW = 0x0606060606060606; /* 6 */
	EleSpec[31].DW = 0x0707070707070707; /* 7 */

	Indx[ 0].DW = 0;
	Indx[ 1].DW = 0;
	Indx[ 2].DW = 0;
	Indx[ 3].DW = 0;
	Indx[ 4].DW = 0;
	Indx[ 5].DW = 0;
	Indx[ 6].DW = 0;
	Indx[ 7].DW = 0;
	Indx[ 8].DW = 0;
	Indx[ 9].DW = 0;
	Indx[10].DW = 0;
	Indx[11].DW = 0;
	Indx[12].DW = 0;
	Indx[13].DW = 0;
	Indx[14].DW = 0;
	Indx[15].DW = 0;

	Indx[16].DW = 0x0001020304050607; /* None */
	Indx[17].DW = 0x0001020304050607; /* None */
	Indx[18].DW = 0x0103050700020406; /* 0q */
	Indx[19].DW = 0x0002040601030507; /* 1q */
	Indx[20].DW = 0x0102030506070004; /* 0h */
	Indx[21].DW = 0x0002030406070105; /* 1h */
	Indx[22].DW = 0x0001030405070206; /* 2h */
	Indx[23].DW = 0x0001020405060307; /* 3h */
	Indx[24].DW = 0x0102030405060700; /* 0 */
	Indx[25].DW = 0x0002030405060701; /* 1 */
	Indx[26].DW = 0x0001030405060702; /* 2 */
	Indx[27].DW = 0x0001020405060703; /* 3 */
	Indx[28].DW = 0x0001020305060704; /* 4 */
	Indx[29].DW = 0x0001020304060705; /* 5 */
	Indx[30].DW = 0x0001020304050706; /* 6 */
	Indx[31].DW = 0x0001020304050607; /* 7 */

	for (i = 16; i < 32; i ++)
	{
		int count;

		for (count = 0; count < 8; count ++)
		{
			Indx[i].B[count] = 7 - Indx[i].B[count];
			EleSpec[i].B[count] = 7 - EleSpec[i].B[count];
		}
		for (count = 0; count < 4; count ++)
		{
			BYTE Temp;

			Temp = Indx[i].B[count];
			Indx[i].B[count] = Indx[i].B[7 - count];
			Indx[i].B[7 - count] = Temp;
		}
	}

	PrgCount = RSPInfo.SP_PC_REG;
}

/******************************************************************
  Function: DoRspCycles
  Purpose:  This function is to allow the RSP to run in parrel with
            the r4300 switching control back to the r4300 once the
			function ends.
  input:    The number of cylces that is meant to be executed
  output:   The number of cycles that was executed. This value can
            be greater than the number of cycles that the RSP 
			should have performed.
			(this value is ignored if the RSP is stoped)
*******************************************************************/ 

DWORD RunInterpreterCPU(DWORD Cycles);
DWORD RunRecompilerCPU (DWORD Cycles);

#define MI_INTR_SP				0x01		/* Bit 0: SP intr */

__declspec(dllexport) DWORD DoRspCycles ( DWORD Cycles )
{
	extern BOOL AudioHle, GraphicsHle;
	DWORD TaskType = *(DWORD*)(RSPInfo.DMEM + 0xFC0);
		
/*	if (*RSPInfo.SP_STATUS_REG & SP_STATUS_SIG0)
	{
		*RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG0;
		*RSPInfo.MI_INTR_REG |= MI_INTR_SP;
		RSPInfo.CheckInterrupts();
		return Cycles;
	}
*/
	if (TaskType == 1 && GraphicsHle && *(DWORD*)(RSPInfo.DMEM + 0x0ff0) != 0)
	{
		if (RSPInfo.ProcessDList != NULL)
		{
			RSPInfo.ProcessDList();
		}
		*RSPInfo.SP_STATUS_REG |= (0x0203 );
		if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 )
		{
			*RSPInfo.MI_INTR_REG |= R4300i_SP_Intr;
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
		*RSPInfo.SP_STATUS_REG |= (0x0203 );
		if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 )
		{
			*RSPInfo.MI_INTR_REG |= R4300i_SP_Intr;
			RSPInfo.CheckInterrupts();
		}
		return Cycles;
	}
	else if (TaskType == 7)
	{
		RSPInfo.ShowCFB();
	}

	Compiler.bAudioUcode = (TaskType == 2) ? TRUE : FALSE;

/*
	*RSPInfo.SP_STATUS_REG |= (0x0203 );
	if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 )
	{
		*RSPInfo.MI_INTR_REG |= R4300i_SP_Intr;
		RSPInfo.CheckInterrupts();
	}
	//return Cycles;
*/	

	if (Profiling && !IndvidualBlock)
	{
		StartTimer((DWORD)Timer_RSP_Running);
	}

	WaitForSingleObjectEx(hMutex, 1000 * 100, FALSE);

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
