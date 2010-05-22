/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
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
#include <math.h>
#include <float.h>
#include "main.h"
#include "cpu.h"
#include "debugger.h"
#include "plugin.h"

BOOL TestTimer = FALSE;

R4300iOp_FUNC * R4300i_Opcode;

BOOL ExecuteInterpreterOpCode (void) 
{
	if (!_MMU->LW_VAddr((*_PROGRAM_COUNTER), Opcode.Hex)) { 
		DoTLBMiss(NextInstruction == JUMP,(*_PROGRAM_COUNTER));
		NextInstruction = NORMAL;
		return FALSE;
	} 

	COUNT_REGISTER += CountPerOp;
	*_Timer -= CountPerOp;

	RANDOM_REGISTER -= 1;
	if ((int)RANDOM_REGISTER < (int)WIRED_REGISTER) {
		RANDOM_REGISTER = 31;
	}

	((void (_fastcall *)()) R4300i_Opcode[ Opcode.op ])();

	if (_GPR[0].DW != 0) {
#if (!defined(EXTERNAL_RELEASE))
		DisplayError("_GPR[0].DW has been written to");
#endif
		_GPR[0].DW = 0;
	}
#ifdef Interpreter_StackTest
	if (StackValue != _GPR[29].UW[0]) {
		DisplayError("Stack has Been changed");
	} 
#endif

	switch (NextInstruction) {
	case NORMAL: 
		(*_PROGRAM_COUNTER) += 4; 
		break;
	case DELAY_SLOT:
		NextInstruction = JUMP;
		(*_PROGRAM_COUNTER) += 4; 
		break;
	case JUMP:
		{
			BOOL CheckTimer = (JumpToLocation < (*_PROGRAM_COUNTER) || TestTimer); 
			(*_PROGRAM_COUNTER)  = JumpToLocation;
			NextInstruction = NORMAL;
			if (CheckTimer)
			{
				TestTimer = FALSE;
				if (*_Timer < 0) 
				{ 
					TimerDone();
				}
				if (CPU_Action.DoSomething) { DoSomething(); }
			}
		}
		if (CPU_Type != CPU_SyncCores) {
			if (Profiling) {
				if (IndvidualBlock) {
					StartTimer((*_PROGRAM_COUNTER));
				} else {
					StartTimer(Timer_R4300);
				}
			}
		}
	}		
	return TRUE;
}
	
void StartInterpreterCPU (void ) { 
	//DWORD Value, Value2, Addr = 0x80031000;

	CoInitialize(NULL);
	TestTimer = FALSE;
	NextInstruction = NORMAL;
	//Add_R4300iBPoint(0x802000C8,FALSE);
	ExecuteInterpreterOps(-1);
}

void ExecuteInterpreterOps (DWORD Cycles)
{
	DWORD CyclesLeft = Cycles;
	__try {
		while(!EndEmulation()) {
#if (!defined(EXTERNAL_RELEASE))
			if (NoOfBpoints != 0) {
				if (CheckForR4300iBPoint((*_PROGRAM_COUNTER))) {
					UpdateCurrentR4300iRegisterPanel();
					Refresh_Memory();
					if (InR4300iCommandsWindow) {
						Enter_R4300i_Commands_Window();
						SetR4300iCommandViewto( (*_PROGRAM_COUNTER) );
						if (CPU_Action.Stepping) {
							DisplayError ( "Encounted a R4300i Breakpoint" );
						} else {
							DisplayError ( "Encounted a R4300i Breakpoint\n\nNow Stepping" );
							SetR4300iCommandToStepping();
						}
					} else {
						DisplayError ( "Encounted a R4300i Breakpoint\n\nEntering Command Window" );
						Enter_R4300i_Commands_Window();
					}					
				}
			}

			//r4300i_LW_VAddr(Addr,&Value);
			//if (Value2 != Value) {
			//	DisplayError("%X changed",Addr);
			//}
			//Value2 = Value;
			if (CPU_Action.Stepping) {
				do {
					SetR4300iCommandViewto ((*_PROGRAM_COUNTER));
					UpdateCurrentR4300iRegisterPanel();
					Refresh_Memory();
					WaitForSingleObject( CPU_Action.hStepping, INFINITE );
					if (CPU_Action.Stepping) { ExecuteInterpreterOpCode(); }
				} while (CPU_Action.Stepping);
			}
#endif
			//if ((Profiling || ShowCPUPer) && ProfilingLabel[0] == 0) { StartTimer(Timer_R4300); };
			if (Cycles != -1) {
				if (CyclesLeft <= 0) { 
					return; 
				}
				if (ExecuteInterpreterOpCode())
				{
					CyclesLeft -= CountPerOp;
				}
			} else {
				ExecuteInterpreterOpCode();
			}
		}
	} __except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
		ExitThread(0);
	}
}

void TestInterpreterJump (DWORD PC, DWORD TargetPC, int Reg1, int Reg2) {
	if (PC != TargetPC) { return; }
	if (DelaySlotEffectsCompare(PC,Reg1,Reg2)) { return; }
	InPermLoop();
	NextInstruction = DELAY_SLOT;
	TestTimer = TRUE;
}