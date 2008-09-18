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

int RoundingModel = _RC_NEAR;

#define ADDRESS_ERROR_EXCEPTION(Address,FromRead) \
	DoAddressError(NextInstruction == JUMP,Address,FromRead);\
	NextInstruction = JUMP;\
	JumpToLocation = PROGRAM_COUNTER;\
	return;

//#define TEST_COP1_USABLE_EXCEPTION
#define TEST_COP1_USABLE_EXCEPTION \
	if ((STATUS_REGISTER & STATUS_CU1) == 0) {\
		DoCopUnusableException(NextInstruction == JUMP,1);\
		NextInstruction = JUMP;\
		JumpToLocation = PROGRAM_COUNTER;\
		return;\
	}

#define TLB_READ_EXCEPTION(Address) \
	DoTLBMiss(NextInstruction == JUMP,Address);\
	NextInstruction = JUMP;\
	JumpToLocation = PROGRAM_COUNTER;\
	return;

/************************* OpCode functions *************************/
void _fastcall r4300i_J (void) {
	NextInstruction = DELAY_SLOT;
	JumpToLocation = (PROGRAM_COUNTER & 0xF0000000) + (Opcode.target << 2);
	TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,0,0);
}

void _fastcall r4300i_JAL (void) {
	NextInstruction = DELAY_SLOT;
	JumpToLocation = (PROGRAM_COUNTER & 0xF0000000) + (Opcode.target << 2);
	TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,0,0);
	GPR[31].DW= (long)(PROGRAM_COUNTER + 8);
}

void _fastcall r4300i_BEQ (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW == GPR[Opcode.rt].DW) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,Opcode.rt);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_BNE (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW != GPR[Opcode.rt].DW) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,Opcode.rt);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_BLEZ (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW <= 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_BGTZ (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW > 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_ADDI (void) {
#ifdef Interpreter_StackTest
	if (Opcode.rs == 29 && Opcode.rt == 29) {
		StackValue += (short)Opcode.immediate;
	}
#endif
	if (Opcode.rt == 0) { return; }
	GPR[Opcode.rt].DW = (GPR[Opcode.rs].W[0] + ((short)Opcode.immediate));
#ifdef Interpreter_StackTest
	if (Opcode.rt == 29 && Opcode.rs != 29) {
		StackValue = GPR[Opcode.rt].W[0];		
	}
#endif
}

void _fastcall r4300i_ADDIU (void) {
#ifdef Interpreter_StackTest
	if (Opcode.rs == 29 && Opcode.rt == 29) {
		StackValue += (short)Opcode.immediate;
	}
#endif
	GPR[Opcode.rt].DW = (GPR[Opcode.rs].W[0] + ((short)Opcode.immediate));
#ifdef Interpreter_StackTest
	if (Opcode.rt == 29 && Opcode.rs != 29) {
		StackValue = GPR[Opcode.rt].W[0];		
	}
#endif
}

void _fastcall r4300i_SLTI (void) {
	if (GPR[Opcode.rs].DW < (_int64)((short)Opcode.immediate)) {
		GPR[Opcode.rt].DW = 1;
	} else {
		GPR[Opcode.rt].DW = 0;
	}
}

void _fastcall r4300i_SLTIU (void) {
	int imm32 = (short)Opcode.immediate;
	__int64 imm64;

	imm64 = imm32;
	GPR[Opcode.rt].DW = GPR[Opcode.rs].UDW < (unsigned __int64)imm64?1:0;
}

void _fastcall r4300i_ANDI (void) {
	GPR[Opcode.rt].DW = GPR[Opcode.rs].DW & Opcode.immediate;
}

void _fastcall r4300i_ORI (void) {
	GPR[Opcode.rt].DW = GPR[Opcode.rs].DW | Opcode.immediate;
}

void _fastcall r4300i_XORI (void) {
	GPR[Opcode.rt].DW = GPR[Opcode.rs].DW ^ Opcode.immediate;
}

void _fastcall r4300i_LUI (void) {
	if (Opcode.rt == 0) { return; }
	GPR[Opcode.rt].DW = (long)((short)Opcode.offset << 16);
#ifdef Interpreter_StackTest
	if (Opcode.rt == 29) {
		StackValue = GPR[Opcode.rt].W[0];
	}
#endif
}

void _fastcall r4300i_BEQL (void) {
	if (GPR[Opcode.rs].DW == GPR[Opcode.rt].DW) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,Opcode.rt);
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_BNEL (void) {
	if (GPR[Opcode.rs].DW != GPR[Opcode.rt].DW) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,Opcode.rt);
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_BLEZL (void) {
	if (GPR[Opcode.rs].DW <= 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_BGTZL (void) {
	if (GPR[Opcode.rs].DW > 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_DADDIU (void) {
	GPR[Opcode.rt].DW = GPR[Opcode.rs].DW + (_int64)((short)Opcode.immediate);
}

QWORD LDL_MASK[8] = { 0,0xFF,0xFFFF,0xFFFFFF,0xFFFFFFFF,0xFFFFFFFFFF,
					 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF };
int LDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void _fastcall r4300i_LDL (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr((Address & ~7),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LDL TLB: %X",Address);
#endif
		return;
	}
	GPR[Opcode.rt].DW = GPR[Opcode.rt].DW & LDL_MASK[Offset];
	GPR[Opcode.rt].DW += Value << LDL_SHIFT[Offset];
}

QWORD LDR_MASK[8] = { 0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000,
                      0xFFFFFFFFFF000000, 0xFFFFFFFF00000000,
                      0xFFFFFF0000000000, 0xFFFF000000000000,
                      0xFF00000000000000, 0 };
int LDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

void _fastcall r4300i_LDR (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr((Address & ~7),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LDL TLB: %X",Address);
#endif
		return;
	}

	GPR[Opcode.rt].DW = GPR[Opcode.rt].DW & LDR_MASK[Offset];
	GPR[Opcode.rt].DW += Value >> LDR_SHIFT[Offset];

}

void _fastcall r4300i_LB (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if (Opcode.rt == 0) { return; }
	if (!r4300i_LB_VAddr(Address,&GPR[Opcode.rt].UB[0])) {
		if (ShowTLBMisses) {
#ifndef EXTERNAL_RELEASE
			DisplayError("LB TLB: %X",Address);
#endif
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].DW = GPR[Opcode.rt].B[0];
	}
}

void _fastcall r4300i_LH (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!r4300i_LH_VAddr(Address,&GPR[Opcode.rt].UHW[0])) {
		if (ShowTLBMisses) {
			DisplayError("LH TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].DW = GPR[Opcode.rt].HW[0];
	}
}

DWORD LWL_MASK[4] = { 0,0xFF,0xFFFF,0xFFFFFF };
int LWL_SHIFT[4] = { 0, 8, 16, 24};

void _fastcall r4300i_LWL (void) {
	DWORD Offset, Address, Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr((Address & ~3),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LDL TLB: %X",Address);
#endif
		return;
	}
	
	GPR[Opcode.rt].DW = (int)(GPR[Opcode.rt].W[0] & LWL_MASK[Offset]);
	GPR[Opcode.rt].DW += (int)(Value << LWL_SHIFT[Offset]);
}

void _fastcall r4300i_LW (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }

#if (!defined(EXTERNAL_RELEASE))
	Log_LW(PROGRAM_COUNTER,Address);
#endif
	if (Opcode.rt == 0) { return; }

	if (Address >= 0xA3F00000 && Address < 0xC0000000)
	{
		if (Address < 0xA4000000 || Address >= 0xA4002000)
		{
			Address &= 0x1FFFFFFF;
			if (!r4300i_LW_NonMemory(Address,&GPR[Opcode.rt].UW[0]))
			{
				if (ShowUnhandledMemory)
				{
					DisplayError("Failed to load word\n\nIn LW",Address);
				}
			}
			GPR[Opcode.rt].DW = GPR[Opcode.rt].W[0];
			return;
		}
	}
	if (!r4300i_LW_VAddr(Address,&GPR[Opcode.rt].UW[0])) {
		if (ShowTLBMisses) {
			DisplayError("LW TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].DW = GPR[Opcode.rt].W[0];
		//TranslateVaddr(&Address);
		//if (Address == 0x00090AA0) {
		//	LogMessage("%X: Read %X from %X",PROGRAM_COUNTER,GPR[Opcode.rt].UW[0],GPR[Opcode.base].UW[0] + (short)Opcode.offset);
		//}
	}
}

void _fastcall r4300i_LBU (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if (!r4300i_LB_VAddr(Address,&GPR[Opcode.rt].UB[0])) {
		if (ShowTLBMisses) {
			DisplayError("LBU TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].UDW = GPR[Opcode.rt].UB[0];
	}
}

void _fastcall r4300i_LHU (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!r4300i_LH_VAddr(Address,&GPR[Opcode.rt].UHW[0])) {
		if (ShowTLBMisses) {
			DisplayError("LHU TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].UDW = GPR[Opcode.rt].UHW[0];
	}
}

DWORD LWR_MASK[4] = { 0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0 };
int LWR_SHIFT[4] = { 24, 16 ,8, 0 };

void _fastcall r4300i_LWR (void) {
	DWORD Offset, Address, Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr((Address & ~3),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LDL TLB: %X",Address);
#endif
		return;
	}
	
	GPR[Opcode.rt].DW = (int)(GPR[Opcode.rt].W[0] & LWR_MASK[Offset]);
	GPR[Opcode.rt].DW += (int)(Value >> LWR_SHIFT[Offset]);
}

void _fastcall r4300i_LWU (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (Opcode.rt == 0) { return; }

	if (!r4300i_LW_VAddr(Address,&GPR[Opcode.rt].UW[0])) {
		if (ShowTLBMisses) {
			DisplayError("LWU TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].UDW = GPR[Opcode.rt].UW[0];
	}
}

void _fastcall r4300i_SB (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if (!r4300i_SB_VAddr(Address,GPR[Opcode.rt].UB[0])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SB TLB: %X",Address);
#endif
	}
}

void _fastcall r4300i_SH (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
	if (!r4300i_SH_VAddr(Address,GPR[Opcode.rt].UHW[0])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SH TLB: %X",Address);
#endif
	}
}

DWORD SWL_MASK[4] = { 0,0xFF000000,0xFFFF0000,0xFFFFFF00 };
int SWL_SHIFT[4] = { 0, 8, 16, 24 };

void _fastcall r4300i_SWL (void) {
	DWORD Offset, Address, Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr((Address & ~3),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SWL_MASK[Offset];
	Value += GPR[Opcode.rt].UW[0] >> SWL_SHIFT[Offset];

	if (!r4300i_SW_VAddr((Address & ~0x03),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
	}
}


void _fastcall r4300i_SW (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
#if (!defined(EXTERNAL_RELEASE))
	Log_SW(PROGRAM_COUNTER,Address,GPR[Opcode.rt].UW[0]);
#endif
	if (Address >= 0xA3F00000 && Address < 0xC0000000)
	{
		if (Address < 0xA4000000 || Address >= 0xA4002000)
		{
			Address &= 0x1FFFFFFF;
			if (!r4300i_SW_NonMemory(Address,GPR[Opcode.rt].UW[0]))
			{
				if (ShowUnhandledMemory)
				{
					DisplayError("Failed to load word\n\nIn SW",Address);
				}
			}
			return;
		}
	}
	if (!r4300i_SW_VAddr(Address,GPR[Opcode.rt].UW[0])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SW TLB: %X",Address);
#endif
	}
	//TranslateVaddr(&Address);
	//if (Address == 0x00090AA0) {
	//	LogMessage("%X: Write %X to %X",PROGRAM_COUNTER,GPR[Opcode.rt].UW[0],GPR[Opcode.base].UW[0] + (short)Opcode.offset);
	//}
}

QWORD SDL_MASK[8] = { 0,0xFF00000000000000,
						0xFFFF000000000000,
						0xFFFFFF0000000000,
						0xFFFFFFFF00000000,
					    0xFFFFFFFFFF000000,
						0xFFFFFFFFFFFF0000,
						0xFFFFFFFFFFFFFF00 
					};
int SDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void _fastcall r4300i_SDL (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr((Address & ~7),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SDL_MASK[Offset];
	Value += GPR[Opcode.rt].UDW >> SDL_SHIFT[Offset];

	if (!r4300i_SD_VAddr((Address & ~7),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
	}
}

QWORD SDR_MASK[8] = { 0x00FFFFFFFFFFFFFF,
					  0x0000FFFFFFFFFFFF,
					  0x000000FFFFFFFFFF,
					  0x00000000FFFFFFFF,
					  0x0000000000FFFFFF,
					  0x000000000000FFFF,
					  0x00000000000000FF, 
					  0x0000000000000000 
					};
int SDR_SHIFT[8] = { 56,48,40,32,24,16,8,0 };

void _fastcall r4300i_SDR (void) {
	DWORD Offset, Address;
	QWORD Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr((Address & ~7),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SDR_MASK[Offset];
	Value += GPR[Opcode.rt].UDW << SDR_SHIFT[Offset];

	if (!r4300i_SD_VAddr((Address & ~7),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDL TLB: %X",Address);
#endif
	}
}

DWORD SWR_MASK[4] = { 0x00FFFFFF,0x0000FFFF,0x000000FF,0x00000000 };
int SWR_SHIFT[4] = { 24, 16 , 8, 0  };

void _fastcall r4300i_SWR (void) {
	DWORD Offset, Address, Value;
	
	Address = GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr((Address & ~3),&Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
		return;
	}
	
	Value &= SWR_MASK[Offset];
	Value += GPR[Opcode.rt].UW[0] << SWR_SHIFT[Offset];

	if (!r4300i_SW_VAddr((Address & ~0x03),Value)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWL TLB: %X",Address);
#endif
	}
}

void _fastcall r4300i_CACHE (void) {
#if (!defined(EXTERNAL_RELEASE))
	if (!LogOptions.LogCache) { return; }
	LogMessage("%08X: Cache operation %d, 0x%08X", PROGRAM_COUNTER, Opcode.rt,
		GPR[Opcode.base].UW[0] + (short)Opcode.offset );
#endif
}

void _fastcall r4300i_LL (void) {
#ifdef OLD_CODE
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }

	if (Opcode.rt == 0) { return; }

	if (!r4300i_LW_VAddr(Address,&GPR[Opcode.rt].UW[0])) {
		if (ShowTLBMisses) {
			DisplayError("LW TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	} else {
		GPR[Opcode.rt].DW = GPR[Opcode.rt].W[0];
	}
	LLBit = 1;
	LLAddr = Address;
	TranslateVaddr(&LLAddr);
#else
	BreakPoint(__FILE__,__LINE__); 
#endif
}

void _fastcall r4300i_LWC1 (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (DWORD)((short)Opcode.offset);
	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!r4300i_LW_VAddr(Address,&*(DWORD *)FPRFloatLocation[Opcode.ft])) {
		if (ShowTLBMisses) {
			DisplayError("LWC1 TLB: %X",Address);
		}
		TLB_READ_EXCEPTION(Address);
	}
}

void _fastcall r4300i_SC (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
#if (!defined(EXTERNAL_RELEASE))
	Log_SW(PROGRAM_COUNTER,Address,GPR[Opcode.rt].UW[0]);
#endif
	if (LLBit == 1) {
		if (!r4300i_SW_VAddr(Address,GPR[Opcode.rt].UW[0])) {
			DisplayError("SW TLB: %X",Address);
		}
	}
	GPR[Opcode.rt].UW[0] = LLBit;
}

void _fastcall r4300i_LD (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!r4300i_LD_VAddr(Address,&GPR[Opcode.rt].UDW)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LD TLB: %X",Address);
#endif
	}
#ifdef Interpreter_StackTest
	if (Opcode.rt == 29) {
		StackValue = GPR[Opcode.rt].W[0];
	}
#endif
}


void _fastcall r4300i_LDC1 (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;

	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,TRUE); }
	if (!r4300i_LD_VAddr(Address,&*(unsigned __int64 *)FPRDoubleLocation[Opcode.ft])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("LD TLB: %X",Address);
#endif
	}
}

void _fastcall r4300i_SWC1 (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;
	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }

	if (!r4300i_SW_VAddr(Address,*(DWORD *)FPRFloatLocation[Opcode.ft])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SWC1 TLB: %X",Address);
#endif
	}
}

void _fastcall r4300i_SDC1 (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;

	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
	if (!r4300i_SD_VAddr(Address,*(__int64 *)FPRDoubleLocation[Opcode.ft])) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SDC1 TLB: %X",Address);
#endif
	}
}

void _fastcall r4300i_SD (void) {
	DWORD Address =  GPR[Opcode.base].UW[0] + (short)Opcode.offset;	
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,FALSE); }
	if (!r4300i_SD_VAddr(Address,GPR[Opcode.rt].UDW)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SD TLB: %X",Address);
#endif
	}
}
/********************** R4300i OpCodes: Special **********************/
void _fastcall r4300i_SPECIAL_SLL (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].W[0] << Opcode.sa);
}

void _fastcall r4300i_SPECIAL_SRL (void) {
	GPR[Opcode.rd].DW = (int)(GPR[Opcode.rt].UW[0] >> Opcode.sa);
}

void _fastcall r4300i_SPECIAL_SRA (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].W[0] >> Opcode.sa);
}

void _fastcall r4300i_SPECIAL_SLLV (void) {
	if (Opcode.rd == 0) { return; }
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].W[0] << (GPR[Opcode.rs].UW[0] & 0x1F));
}

void _fastcall r4300i_SPECIAL_SRLV (void) {
	GPR[Opcode.rd].DW = (int)(GPR[Opcode.rt].UW[0] >> (GPR[Opcode.rs].UW[0] & 0x1F));
}

void _fastcall r4300i_SPECIAL_SRAV (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].W[0] >> (GPR[Opcode.rs].UW[0] & 0x1F));
}

void _fastcall r4300i_SPECIAL_JR (void) {
	NextInstruction = DELAY_SLOT;
	JumpToLocation = GPR[Opcode.rs].UW[0];
	TestTimer = TRUE;
}

void _fastcall r4300i_SPECIAL_JALR (void) {
	NextInstruction = DELAY_SLOT;
	JumpToLocation = GPR[Opcode.rs].UW[0];
	GPR[Opcode.rd].DW = (long)(PROGRAM_COUNTER + 8);
	TestTimer = TRUE;
}

void _fastcall r4300i_SPECIAL_SYSCALL (void) {
	DoSysCallException(NextInstruction == JUMP);
	NextInstruction = JUMP;
	JumpToLocation = PROGRAM_COUNTER;
}

void _fastcall r4300i_SPECIAL_BREAK (void) {
	DoBreakException(NextInstruction == JUMP);
	NextInstruction = JUMP;
	JumpToLocation = PROGRAM_COUNTER;
}

void _fastcall r4300i_SPECIAL_SYNC (void) {
}

void _fastcall r4300i_SPECIAL_MFHI (void) {
	GPR[Opcode.rd].DW = HI.DW;
}

void _fastcall r4300i_SPECIAL_MTHI (void) {
	HI.DW = GPR[Opcode.rs].DW;
}

void _fastcall r4300i_SPECIAL_MFLO (void) {
	GPR[Opcode.rd].DW = LO.DW;
}

void _fastcall r4300i_SPECIAL_MTLO (void) {
	LO.DW = GPR[Opcode.rs].DW;
}

void _fastcall r4300i_SPECIAL_DSLLV (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rt].DW << (GPR[Opcode.rs].UW[0] & 0x3F);
}

void _fastcall r4300i_SPECIAL_DSRLV (void) {
	GPR[Opcode.rd].UDW = GPR[Opcode.rt].UDW >> (GPR[Opcode.rs].UW[0] & 0x3F);
}

void _fastcall r4300i_SPECIAL_DSRAV (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rt].DW >> (GPR[Opcode.rs].UW[0] & 0x3F);
}

void _fastcall r4300i_SPECIAL_MULT (void) {
	HI.DW = (_int64)(GPR[Opcode.rs].W[0]) * (_int64)(GPR[Opcode.rt].W[0]);
	LO.DW = HI.W[0];
	HI.DW = HI.W[1];
}

void _fastcall r4300i_SPECIAL_MULTU (void) {
	HI.DW = (unsigned _int64)(GPR[Opcode.rs].UW[0]) * (unsigned _int64)(GPR[Opcode.rt].UW[0]);
	LO.DW = HI.W[0];
	HI.DW = HI.W[1];
}

void _fastcall r4300i_SPECIAL_DIV (void) {
	if ( GPR[Opcode.rt].UDW != 0 ) {
		LO.DW = GPR[Opcode.rs].W[0] / GPR[Opcode.rt].W[0];
		HI.DW = GPR[Opcode.rs].W[0] % GPR[Opcode.rt].W[0];
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DIV by 0 ???");
#endif
	}
}

void _fastcall r4300i_SPECIAL_DIVU (void) {
	if ( GPR[Opcode.rt].UDW != 0 ) {
		LO.DW = (int)(GPR[Opcode.rs].UW[0] / GPR[Opcode.rt].UW[0]);
		HI.DW = (int)(GPR[Opcode.rs].UW[0] % GPR[Opcode.rt].UW[0]);
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DIVU by 0 ???");
#endif
	}
}

void _fastcall r4300i_SPECIAL_DMULT (void) {
	MIPS_DWORD Tmp[3];
	
	LO.UDW = (QWORD)GPR[Opcode.rs].UW[0] * (QWORD)GPR[Opcode.rt].UW[0];
	Tmp[0].UDW = (_int64)GPR[Opcode.rs].W[1] * (_int64)(QWORD)GPR[Opcode.rt].UW[0];
	Tmp[1].UDW = (_int64)(QWORD)GPR[Opcode.rs].UW[0] * (_int64)GPR[Opcode.rt].W[1];
	HI.UDW = (_int64)GPR[Opcode.rs].W[1] * (_int64)GPR[Opcode.rt].W[1];
	
	Tmp[2].UDW = (QWORD)LO.UW[1] + (QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0];
	LO.UDW += ((QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0]) << 32;
	HI.UDW += (QWORD)Tmp[0].W[1] + (QWORD)Tmp[1].W[1] + Tmp[2].UW[1];
}

void _fastcall r4300i_SPECIAL_DMULTU (void) {
	MIPS_DWORD Tmp[3];
	
	LO.UDW = (QWORD)GPR[Opcode.rs].UW[0] * (QWORD)GPR[Opcode.rt].UW[0];
	Tmp[0].UDW = (QWORD)GPR[Opcode.rs].UW[1] * (QWORD)GPR[Opcode.rt].UW[0];
	Tmp[1].UDW = (QWORD)GPR[Opcode.rs].UW[0] * (QWORD)GPR[Opcode.rt].UW[1];
	HI.UDW = (QWORD)GPR[Opcode.rs].UW[1] * (QWORD)GPR[Opcode.rt].UW[1];
	
	Tmp[2].UDW = (QWORD)LO.UW[1] + (QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0];
	LO.UDW += ((QWORD)Tmp[0].UW[0] + (QWORD)Tmp[1].UW[0]) << 32;
	HI.UDW += (QWORD)Tmp[0].UW[1] + (QWORD)Tmp[1].UW[1] + Tmp[2].UW[1];
}

void _fastcall r4300i_SPECIAL_DDIV (void) {
	if ( GPR[Opcode.rt].UDW != 0 ) {
		LO.DW = GPR[Opcode.rs].DW / GPR[Opcode.rt].DW;
		HI.DW = GPR[Opcode.rs].DW % GPR[Opcode.rt].DW;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DDIV by 0 ???");
#endif
	}
}

void _fastcall r4300i_SPECIAL_DDIVU (void) {
	if ( GPR[Opcode.rt].UDW != 0 ) {
		LO.UDW = GPR[Opcode.rs].UDW / GPR[Opcode.rt].UDW;
		HI.UDW = GPR[Opcode.rs].UDW % GPR[Opcode.rt].UDW;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("DDIVU by 0 ???");
#endif
	}
}

void _fastcall r4300i_SPECIAL_ADD (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].W[0] + GPR[Opcode.rt].W[0];
}

void _fastcall r4300i_SPECIAL_ADDU (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].W[0] + GPR[Opcode.rt].W[0];
}

void _fastcall r4300i_SPECIAL_SUB (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].W[0] - GPR[Opcode.rt].W[0];
}

void _fastcall r4300i_SPECIAL_SUBU (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].W[0] - GPR[Opcode.rt].W[0];
}

void _fastcall r4300i_SPECIAL_AND (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW & GPR[Opcode.rt].DW;
}

void _fastcall r4300i_SPECIAL_OR (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW | GPR[Opcode.rt].DW;
#ifdef Interpreter_StackTest
	if (Opcode.rd == 29) {
		StackValue = GPR[Opcode.rd].W[0];
	}
#endif
}

void _fastcall r4300i_SPECIAL_XOR (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW ^ GPR[Opcode.rt].DW;
}

void _fastcall r4300i_SPECIAL_NOR (void) {
	GPR[Opcode.rd].DW = ~(GPR[Opcode.rs].DW | GPR[Opcode.rt].DW);
}

void _fastcall r4300i_SPECIAL_SLT (void) {
	if (GPR[Opcode.rs].DW < GPR[Opcode.rt].DW) {
		GPR[Opcode.rd].DW = 1;
	} else {
		GPR[Opcode.rd].DW = 0;
	}
}

void _fastcall r4300i_SPECIAL_SLTU (void) {
	if (GPR[Opcode.rs].UDW < GPR[Opcode.rt].UDW) {
		GPR[Opcode.rd].DW = 1;
	} else {
		GPR[Opcode.rd].DW = 0;
	}
}

void _fastcall r4300i_SPECIAL_DADD (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW + GPR[Opcode.rt].DW;
}

void _fastcall r4300i_SPECIAL_DADDU (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW + GPR[Opcode.rt].DW;
}

void _fastcall r4300i_SPECIAL_DSUB (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW - GPR[Opcode.rt].DW;
}

void _fastcall r4300i_SPECIAL_DSUBU (void) {
	GPR[Opcode.rd].DW = GPR[Opcode.rs].DW - GPR[Opcode.rt].DW;
}

void _fastcall r4300i_SPECIAL_TEQ (void) {
	if (GPR[Opcode.rs].DW == GPR[Opcode.rt].DW) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Should trap this ???");
#endif
	}
}

void _fastcall r4300i_SPECIAL_DSLL (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].DW << Opcode.sa);
}

void _fastcall r4300i_SPECIAL_DSRL (void) {
	GPR[Opcode.rd].UDW = (GPR[Opcode.rt].UDW >> Opcode.sa);
}

void _fastcall r4300i_SPECIAL_DSRA (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].DW >> Opcode.sa);
}

void _fastcall r4300i_SPECIAL_DSLL32 (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].DW << (Opcode.sa + 32));
}

void _fastcall r4300i_SPECIAL_DSRL32 (void) {
   GPR[Opcode.rd].UDW = (GPR[Opcode.rt].UDW >> (Opcode.sa + 32));
}

void _fastcall r4300i_SPECIAL_DSRA32 (void) {
	GPR[Opcode.rd].DW = (GPR[Opcode.rt].DW >> (Opcode.sa + 32));
}

/********************** R4300i OpCodes: RegImm **********************/
void _fastcall r4300i_REGIMM_BLTZ (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW < 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_REGIMM_BGEZ (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW >= 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_REGIMM_BLTZL (void) {
	if (GPR[Opcode.rs].DW < 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_REGIMM_BGEZL (void) {
	if (GPR[Opcode.rs].DW >= 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_REGIMM_BLTZAL (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW < 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
	GPR[31].DW= (long)(PROGRAM_COUNTER + 8);
}

void _fastcall r4300i_REGIMM_BGEZAL (void) {
	NextInstruction = DELAY_SLOT;
	if (GPR[Opcode.rs].DW >= 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
		TestInterpreterJump(PROGRAM_COUNTER,JumpToLocation,Opcode.rs,0);
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
	GPR[31].DW = (long)(PROGRAM_COUNTER + 8);
}
/************************** COP0 functions **************************/
void _fastcall r4300i_COP0_MF (void) {
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.LogCP0reads) {
		LogMessage("%08X: R4300i Read from %s (0x%08X)", PROGRAM_COUNTER,
			Cop0_Name[Opcode.rd], CP0[Opcode.rd]);
	}
#endif
	GPR[Opcode.rt].DW = (int)CP0[Opcode.rd];
}

void _fastcall r4300i_COP0_MT (void) {
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.LogCP0changes) {
		LogMessage("%08X: Writing 0x%X to %s register (Originally: 0x%08X)",PROGRAM_COUNTER,
			GPR[Opcode.rt].UW[0],Cop0_Name[Opcode.rd], CP0[Opcode.rd]);
		if (Opcode.rd == 11) { //Compare
			LogMessage("%08X: Cause register changed from %08X to %08X",PROGRAM_COUNTER,
				CAUSE_REGISTER, (CAUSE_REGISTER & ~CAUSE_IP7));
		}
	}
#endif
	switch (Opcode.rd) {	
	case 0: //Index
	case 2: //EntryLo0
	case 3: //EntryLo1
	case 5: //PageMask
	case 6: //Wired
	case 10: //Entry Hi
	case 14: //EPC
	case 16: //Config
	case 18: //WatchLo
	case 19: //WatchHi
	case 28: //Tag lo
	case 29: //Tag Hi
	case 30: //ErrEPC
		CP0[Opcode.rd] = GPR[Opcode.rt].UW[0];
		break;
	case 4: //Context
		CP0[Opcode.rd] = GPR[Opcode.rt].UW[0] & 0xFF800000;
		break;
	case 9: //Count
		CP0[Opcode.rd]= GPR[Opcode.rt].UW[0];
		ChangeCompareTimer();
		break;		
	case 11: //Compare
		CP0[Opcode.rd] = GPR[Opcode.rt].UW[0];
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP7;
		ChangeCompareTimer();
		break;		
	case 12: //Status
		if ((CP0[Opcode.rd] ^ GPR[Opcode.rt].UW[0]) != 0) {
			CP0[Opcode.rd] = GPR[Opcode.rt].UW[0];
			SetFpuLocations();
		} else {
			CP0[Opcode.rd] = GPR[Opcode.rt].UW[0];
		}
		if ((CP0[Opcode.rd] & 0x18) != 0) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("Left kernel mode ??");
#endif
		}
		CheckInterrupts();
		break;		
	case 13: //cause
		CP0[Opcode.rd] &= 0xFFFFCFF;
#ifndef EXTERNAL_RELEASE
		if ((GPR[Opcode.rt].UW[0] & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		break;
	default:
		R4300i_UnknownOpcode();
	}
}

/************************** COP0 CO functions ***********************/
void _fastcall r4300i_COP0_CO_TLBR (void) {
	if (!UseTlb) { return; }
	TLB_ReadEntry();
}

void _fastcall r4300i_COP0_CO_TLBWI (void) {
	if (!UseTlb) { return; }
	TLB_WriteEntry(INDEX_REGISTER & 0x1F,FALSE);
}

void _fastcall r4300i_COP0_CO_TLBWR (void) {
	if (!UseTlb) { return; }
	TLB_WriteEntry(RANDOM_REGISTER & 0x1F,TRUE);
}

void _fastcall r4300i_COP0_CO_TLBP (void) {
	if (!UseTlb) { return; }
	TLB_Probe();
}

void _fastcall r4300i_COP0_CO_ERET (void) {
	NextInstruction = JUMP;
	if ((STATUS_REGISTER & STATUS_ERL) != 0) {
		JumpToLocation = ERROREPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		JumpToLocation = EPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_EXL;
	}
	LLBit = 0;
	CheckInterrupts();
	TestTimer = TRUE;
}

/************************** COP1 functions **************************/
void _fastcall r4300i_COP1_MF (void) {
	TEST_COP1_USABLE_EXCEPTION	
	GPR[Opcode.rt].DW = *(int *)FPRFloatLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_DMF (void) {
	TEST_COP1_USABLE_EXCEPTION
	GPR[Opcode.rt].DW = *(__int64 *)FPRDoubleLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_CF (void) {
	TEST_COP1_USABLE_EXCEPTION
	if (Opcode.fs != 31 && Opcode.fs != 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("CFC1 what register are you writing to ?");
#endif
		return;
	}
	GPR[Opcode.rt].DW = (int)FPCR[Opcode.fs];
}

void _fastcall r4300i_COP1_MT (void) {
	TEST_COP1_USABLE_EXCEPTION
	*(int *)FPRFloatLocation[Opcode.fs] = GPR[Opcode.rt].W[0];
}

void _fastcall r4300i_COP1_DMT (void) {
	TEST_COP1_USABLE_EXCEPTION
	*(__int64 *)FPRDoubleLocation[Opcode.fs] = GPR[Opcode.rt].DW;
}

void _fastcall r4300i_COP1_CT (void) {
	TEST_COP1_USABLE_EXCEPTION
	if (Opcode.fs == 31) {
		FPCR[Opcode.fs] = GPR[Opcode.rt].W[0];
		switch((FPCR[Opcode.fs] & 3)) {
		case 0: RoundingModel = _RC_NEAR; break;
		case 1: RoundingModel = _RC_CHOP; break;
		case 2: RoundingModel = _RC_UP; break;
		case 3: RoundingModel = _RC_DOWN; break;
		}
		return;
	}
#ifndef EXTERNAL_RELEASE
	DisplayError("CTC1 what register are you writing to ?");
#endif
}

/************************* COP1: BC1 functions ***********************/
void _fastcall r4300i_COP1_BCF (void) {
	TEST_COP1_USABLE_EXCEPTION
	NextInstruction = DELAY_SLOT;
	if ((FPCR[31] & FPCSR_C) == 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_COP1_BCT (void) {
	TEST_COP1_USABLE_EXCEPTION
	NextInstruction = DELAY_SLOT;
	if ((FPCR[31] & FPCSR_C) != 0) {
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_COP1_BCFL (void) {
	TEST_COP1_USABLE_EXCEPTION
	if ((FPCR[31] & FPCSR_C) == 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}

void _fastcall r4300i_COP1_BCTL (void) {
	TEST_COP1_USABLE_EXCEPTION
	if ((FPCR[31] & FPCSR_C) != 0) {
		NextInstruction = DELAY_SLOT;
		JumpToLocation = PROGRAM_COUNTER + ((short)Opcode.offset << 2) + 4;
	} else {
		NextInstruction = JUMP;
		JumpToLocation = PROGRAM_COUNTER + 8;
	}
}
/************************** COP1: S functions ************************/
__inline void Float_RoundToInteger32( int * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void Float_RoundToInteger64( __int64 * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp qword ptr [edi]
	}
}

void _fastcall r4300i_COP1_S_ADD (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (*(float *)FPRFloatLocation[Opcode.fs] + *(float *)FPRFloatLocation[Opcode.ft]); 
}

void _fastcall r4300i_COP1_S_SUB (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (*(float *)FPRFloatLocation[Opcode.fs] - *(float *)FPRFloatLocation[Opcode.ft]); 
}

void _fastcall r4300i_COP1_S_MUL (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (*(float *)FPRFloatLocation[Opcode.fs] * *(float *)FPRFloatLocation[Opcode.ft]); 
}

void _fastcall r4300i_COP1_S_DIV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (*(float *)FPRFloatLocation[Opcode.fs] / *(float *)FPRFloatLocation[Opcode.ft]); 
}

void _fastcall r4300i_COP1_S_SQRT (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (float)sqrt(*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_ABS (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (float)fabs(*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_MOV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = *(float *)FPRFloatLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_S_NEG (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (*(float *)FPRFloatLocation[Opcode.fs] * -1.0f);
}

void _fastcall r4300i_COP1_S_TRUNC_L (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_CEIL_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_UP,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_FLOOR_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_ROUND_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_NEAR,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_TRUNC_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_CEIL_W (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_UP,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_FLOOR_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_CVT_D (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = (double)(*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_CVT_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	Float_RoundToInteger32(&*(int *)FPRFloatLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_CVT_L (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	Float_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[Opcode.fd],&*(float *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_S_CMP (void) {
	int less, equal, unorded, condition;
	float Temp0, Temp1;

	TEST_COP1_USABLE_EXCEPTION

	Temp0 = *(float *)FPRFloatLocation[Opcode.fs];
	Temp1 = *(float *)FPRFloatLocation[Opcode.ft];

	if (_isnan(Temp0) || _isnan(Temp1)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Nan ?");
#endif
		less = FALSE;
		equal = FALSE;
		unorded = TRUE;
		if ((Opcode.funct & 8) != 0) {
#ifndef EXTERNAL_RELEASE
			DisplayError("Signal InvalidOperationException\nin r4300i_COP1_S_CMP\n%X  %ff\n%X  %ff",
				Temp0,Temp0,Temp1,Temp1);
#endif
		}
	} else {
		less = Temp0 < Temp1;
		equal = Temp0 == Temp1;
		unorded = FALSE;
	}
	
	condition = ((Opcode.funct & 4) && less) | ((Opcode.funct & 2) && equal) | 
		((Opcode.funct & 1) && unorded);

	if (condition) {
		FPCR[31] |= FPCSR_C;
	} else {
		FPCR[31] &= ~FPCSR_C;
	}
	
}

/************************** COP1: D functions ************************/
__inline void Double_RoundToInteger32( int * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void Double_RoundToInteger64( __int64 * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp qword ptr [edi]
	}
}

void _fastcall r4300i_COP1_D_ADD (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = *(double *)FPRDoubleLocation[Opcode.fs] + *(double *)FPRDoubleLocation[Opcode.ft]; 
}

void _fastcall r4300i_COP1_D_SUB (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = *(double *)FPRDoubleLocation[Opcode.fs] - *(double *)FPRDoubleLocation[Opcode.ft]; 
}

void _fastcall r4300i_COP1_D_MUL (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = *(double *)FPRDoubleLocation[Opcode.fs] * *(double *)FPRDoubleLocation[Opcode.ft]; 
}

void _fastcall r4300i_COP1_D_DIV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = *(double *)FPRDoubleLocation[Opcode.fs] / *(double *)FPRDoubleLocation[Opcode.ft]; 
}

void _fastcall r4300i_COP1_D_SQRT (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = (double)sqrt(*(double *)FPRDoubleLocation[Opcode.fs]); 
}

void _fastcall r4300i_COP1_D_ABS (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = fabs(*(double *)FPRDoubleLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_D_MOV (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(__int64 *)FPRDoubleLocation[Opcode.fd] = *(__int64 *)FPRDoubleLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_D_NEG (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = (*(double *)FPRDoubleLocation[Opcode.fs] * -1.0);
}

void _fastcall r4300i_COP1_D_TRUNC_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_CHOP,_MCW_RC);
	Double_RoundToInteger64(&*(__int64 *)FPRFloatLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs] );
}

void _fastcall r4300i_COP1_D_CEIL_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_UP,_MCW_RC);
	Double_RoundToInteger64(&*(__int64 *)FPRFloatLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs] );
}

void _fastcall r4300i_COP1_D_FLOOR_L (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Double_RoundToInteger64(&*(__int64 *)FPRDoubleLocation[Opcode.fd],&*(double *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_D_ROUND_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_NEAR,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)FPRFloatLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs] );
}

void _fastcall r4300i_COP1_D_TRUNC_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_CHOP,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)FPRFloatLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs] );
}

void _fastcall r4300i_COP1_D_CEIL_W (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RC_UP,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)FPRFloatLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs] );
}

void _fastcall r4300i_COP1_D_FLOOR_W (void) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(_RC_DOWN,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)FPRDoubleLocation[Opcode.fd],&*(double *)FPRFloatLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_D_CVT_S (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (float)*(double *)FPRDoubleLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_D_CVT_W (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	Double_RoundToInteger32(&*(DWORD *)FPRFloatLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs] );
}

void _fastcall r4300i_COP1_D_CVT_L (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	Double_RoundToInteger64(&*(unsigned __int64 *)FPRDoubleLocation[Opcode.fd],&*(double *)FPRDoubleLocation[Opcode.fs]);
}

void _fastcall r4300i_COP1_D_CMP (void) {
	int less, equal, unorded, condition;
	MIPS_DWORD Temp0, Temp1;

	TEST_COP1_USABLE_EXCEPTION

	Temp0.DW = *(__int64 *)FPRDoubleLocation[Opcode.fs];
	Temp1.DW = *(__int64 *)FPRDoubleLocation[Opcode.ft];

	if (_isnan(Temp0.D) || _isnan(Temp1.D)) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Nan ?");
#endif
		less = FALSE;
		equal = FALSE;
		unorded = TRUE;
		if ((Opcode.funct & 8) != 0) {
#ifndef EXTERNAL_RELEASE
			DisplayError("Signal InvalidOperationException\nin r4300i_COP1_D_CMP");
#endif
		}
	} else {
		less = Temp0.D < Temp1.D;
		equal = Temp0.D == Temp1.D;
		unorded = FALSE;
	}
	
	condition = ((Opcode.funct & 4) && less) | ((Opcode.funct & 2) && equal) | 
		((Opcode.funct & 1) && unorded);

	if (condition) {
		FPCR[31] |= FPCSR_C;
	} else {
		FPCR[31] &= ~FPCSR_C;
	}	
}

/************************** COP1: W functions ************************/
void _fastcall r4300i_COP1_W_CVT_S (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (float)*(int *)FPRFloatLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_W_CVT_D (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = (double)*(int *)FPRFloatLocation[Opcode.fs];
}

/************************** COP1: L functions ************************/
void _fastcall r4300i_COP1_L_CVT_S (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(float *)FPRFloatLocation[Opcode.fd] = (float)*(__int64 *)FPRDoubleLocation[Opcode.fs];
}

void _fastcall r4300i_COP1_L_CVT_D (void) {
	TEST_COP1_USABLE_EXCEPTION
	_controlfp(RoundingModel,_MCW_RC);
	*(double *)FPRDoubleLocation[Opcode.fd] = (double)*(__int64 *)FPRDoubleLocation[Opcode.fs];
}

/************************** Other functions **************************/
void _fastcall R4300i_UnknownOpcode (void) {
#ifdef OLD_CODE
	char Message[200];

	sprintf(Message,"%s: %08X\n%s\n\n", GS(MSG_UNHANDLED_OP), PROGRAM_COUNTER,
		R4300iOpcodeName(Opcode.Hex,PROGRAM_COUNTER));
	strcat(Message,"Stopping Emulation !");
	
#if (!defined(EXTERNAL_RELEASE))
	if (HaveDebugger && !inFullScreen) {
		int response;

		strcat(Message,"\n\nDo you wish to enter the debugger ?");
	
		response = MessageBox(NULL,Message,GS(MSG_MSGBOX_TITLE), MB_YESNO | MB_ICONERROR );
		if (response == IDYES) {
			Enter_R4300i_Commands_Window ();
		}
		ExitThread(0);
	} else {
		DisplayError(Message);
		ExitThread(0);
	}
#else
	DisplayError(Message);
	ExitThread(0);
#endif
#else
	BreakPoint(__FILE__,__LINE__); 

#endif
}
