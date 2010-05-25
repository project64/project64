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
#ifdef toremove

#include <windows.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "x86.h"
#include "plugin.h"
#include "debugger.h"

void ** JumpTable, ** DelaySlotTable;
BYTE *RecompPos;

BOOL WrittenToRom;
DWORD WroteToRom;
DWORD TempValue;


int r4300i_Command_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP) {
	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)N64MEM;
    EXCEPTION_RECORD exRec;

	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	exRec = *lpEP->ExceptionRecord;

    if ((int)((BYTE *)lpEP->ExceptionRecord->ExceptionInformation[1] - N64MEM) < 0) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
    if ((int)((BYTE *)lpEP->ExceptionRecord->ExceptionInformation[1] - N64MEM) > 0x1FFFFFFF) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	switch(*(unsigned char *)lpEP->ContextRecord->Eip) {
	case 0x8B:
		switch(*(unsigned char *)(lpEP->ContextRecord->Eip + 1)) {
		case 0x04:
			lpEP->ContextRecord->Eip += 3;
			r4300i_LW_NonMemory((char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
				&lpEP->ContextRecord->Eax);
			return EXCEPTION_CONTINUE_EXECUTION;
			break;
		case 0x0C:
			lpEP->ContextRecord->Eip += 3;
			r4300i_LW_NonMemory((char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
				&lpEP->ContextRecord->Ecx);
			return EXCEPTION_CONTINUE_EXECUTION;
			break;
		default:
			DisplayError("Unknown x86 opcode %X\nlocation %X\nN64mem loc: %X", 
				*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)N64MEM);
			//return EXCEPTION_EXECUTE_HANDLER;
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	default:
		DisplayError("Unknown x86 opcode %X\nlocation %X\nloc: %X\n2", 
			*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)N64MEM);
		//return EXCEPTION_EXECUTE_HANDLER;
		return EXCEPTION_CONTINUE_SEARCH;
	}
	DisplayError("Unknown x86 opcode %X\nlocation %X\nN64mem loc: %X\nAddress: %X", 
		*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)N64MEM, 
		(char *)exRec.ExceptionInformation[1] - (char *)N64MEM);
	DisplayError("r4300i: CPU Memory Filter\n\nWTF");
	return EXCEPTION_CONTINUE_SEARCH;
}

int r4300i_CPU_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP) {
	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)N64MEM;
    EXCEPTION_RECORD exRec;
	BYTE * ReadPos, *TypePos;
	DWORD * Reg;
	
	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	TypePos = (unsigned char *)lpEP->ContextRecord->Eip;
	exRec = *lpEP->ExceptionRecord;

    if ((int)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF) { return EXCEPTION_CONTINUE_SEARCH; }
	
	if (*TypePos == 0xF3 && *(TypePos + 1) == 0xA5) {
		DWORD Start, End, count, OldProtect;
		Start = (lpEP->ContextRecord->Edi - (DWORD)N64MEM);
		End = (Start + (lpEP->ContextRecord->Ecx << 2) - 1);
		if ((int)Start < 0) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("hmmm.... where does this dma start ?");
#endif
			return EXCEPTION_CONTINUE_SEARCH;
		}
#ifdef CFB_READ
		if (Start >= CFBStart && End < CFBEnd) {
			for ( count = Start; count < End; count += 0x1000 ) {
				VirtualProtect(N64MEM+count,4,PAGE_READONLY, &OldProtect);
				if (FrameBufferRead) { FrameBufferRead(count & ~0xFFF); }
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}	
#endif
		if ((int)End < RdramSize) {
			for ( count = Start; count < End; count += 0x1000 ) {
				BreakPoint(__FILE__,__LINE__);
				if (N64_Blocks.NoOfRDRamBlocks[(count >> 12)] > 0) {
					N64_Blocks.NoOfRDRamBlocks[(count >> 12)] = 0;		
					memset(JumpTable + ((count & 0x00FFFFF0) >> 2),0,0x1000);
					*(DelaySlotTable + count) = NULL;
					if (VirtualProtect(N64MEM + count, 4, PAGE_READWRITE, &OldProtect) == 0) {
#ifndef EXTERNAL_RELEASE
						DisplayError("Failed to unprotect %X\n1", count);
#endif
					}
				}
			}			
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04000000 && End < 0x04001000) {
			BreakPoint(__FILE__,__LINE__);
			N64_Blocks.NoOfDMEMBlocks = 0;
			memset(JumpTable + (0x04000000 >> 2),0,0x1000);
			*(DelaySlotTable + (0x04000000 >> 12)) = NULL;
			if (VirtualProtect(N64MEM + 0x04000000, 4, PAGE_READWRITE, &OldProtect) == 0) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Failed to unprotect %X\n7", 0x04000000);
#endif
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04001000 && End < 0x04002000) {
			BreakPoint(__FILE__,__LINE__);
			N64_Blocks.NoOfIMEMBlocks = 0;
			memset(JumpTable + (0x04001000 >> 2),0,0x1000);
			*(DelaySlotTable + (0x04001000 >> 12)) = NULL;
			if (VirtualProtect(N64MEM + 0x04001000, 4, PAGE_READWRITE, &OldProtect) == 0) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Failed to unprotect %X\n6", 0x04001000);
#endif
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}
#ifndef EXTERNAL_RELEASE
		DisplayError("hmmm.... where does this dma End ?\nstart: %X\nend:%X\nlocation %X", 
			Start,End,lpEP->ContextRecord->Eip);
#endif
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (*TypePos == 0x0F && *(TypePos + 1) == 0xB6) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xB7) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBE) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBF) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x66) {
		ReadPos = TypePos + 2;
	} else {
		ReadPos = TypePos + 1;
	}

	switch ((*ReadPos & 0x38)) {
	case 0x00: Reg = &lpEP->ContextRecord->Eax; break;
	case 0x08: Reg = &lpEP->ContextRecord->Ecx; break; 
	case 0x10: Reg = &lpEP->ContextRecord->Edx; break; 
	case 0x18: Reg = &lpEP->ContextRecord->Ebx; break; 
	case 0x20: Reg = &lpEP->ContextRecord->Esp; break;
	case 0x28: Reg = &lpEP->ContextRecord->Ebp; break;
	case 0x30: Reg = &lpEP->ContextRecord->Esi; break;
	case 0x38: Reg = &lpEP->ContextRecord->Edi; break;
	}

	switch ((*ReadPos & 0xC7)) {
	case 0: ReadPos += 1; break;
	case 1: ReadPos += 1; break;
	case 2: ReadPos += 1; break;
	case 3: ReadPos += 1; break;
	case 4: 
		ReadPos += 1; 
		switch ((*ReadPos & 0xC7)) {
		case 0: ReadPos += 1; break;
		case 1: ReadPos += 1; break;
		case 2: ReadPos += 1; break;
		case 3: ReadPos += 1; break;
		case 6: ReadPos += 1; break;
		case 7: ReadPos += 1; break;
		default:
			BreakPoint(__FILE__,__LINE__);
		}
		break;
	case 5: ReadPos += 5; break;
	case 6: ReadPos += 1; break;
	case 7: ReadPos += 1; break;
	case 0x40: ReadPos += 2; break;
	case 0x41: ReadPos += 2; break;
	case 0x42: ReadPos += 2; break;
	case 0x43: ReadPos += 2; break;
	case 0x46: ReadPos += 2; break;
	case 0x47: ReadPos += 2; break;
	case 0x80: ReadPos += 5; break;
	case 0x81: ReadPos += 5; break;
	case 0x82: ReadPos += 5; break;
	case 0x83: ReadPos += 5; break;
	case 0x86: ReadPos += 5; break;
	case 0x87: ReadPos += 5; break;
	default:
		DisplayError("Unknown x86 opcode %X\nlocation %X\nloc: %X\nfgh2", 
			*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)N64MEM);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	switch(*TypePos) {
	case 0x0F:
		switch(*(TypePos + 1)) {
		case 0xB6:
			if (!r4300i_LB_NonMemory(MemAddress,(DWORD *)Reg,FALSE)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xB7:
			if (!r4300i_LH_NonMemory(MemAddress,(DWORD *)Reg,FALSE)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to load half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xBE:
			if (!r4300i_LB_NonMemory(MemAddress,Reg,TRUE)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xBF:
			if (!r4300i_LH_NonMemory(MemAddress,Reg,TRUE)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to load half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		default:
			DisplayError("Unkown x86 opcode %X\nlocation %X\nloc: %X\nfhfgh2", 
				*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)N64MEM);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x66:
		switch(*(TypePos + 1)) {
		case 0x8B:
			if (!r4300i_LH_NonMemory(MemAddress,Reg,FALSE)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0x89:
			if (!r4300i_SH_NonMemory(MemAddress,*(WORD *)Reg)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to store half word\n\nMIPS Address: %X\nX86 Address",MemAddress,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xC7:
			if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
			if (!r4300i_SH_NonMemory(MemAddress,*(WORD *)ReadPos)) {
				if (ShowUnhandledMemory) {
					DisplayError("Failed to store half word\n\nMIPS Address: %X\nX86 Address",MemAddress,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 2);
			return EXCEPTION_CONTINUE_EXECUTION;		
		default:
			DisplayError("Unkown x86 opcode %X\nlocation %X\nloc: %X\nfhfgh2", 
				*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)N64MEM);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x88: 
		if (!r4300i_SB_NonMemory(MemAddress,*(BYTE *)Reg)) {
			if (ShowUnhandledMemory) {
				DisplayError("Failed to store byte\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x8A: 
		if (!r4300i_LB_NonMemory(MemAddress,Reg,FALSE)) {
			if (ShowUnhandledMemory) {
				DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x8B: 
		if (!r4300i_LW_NonMemory(MemAddress,Reg)) {
			if (ShowUnhandledMemory) {
				DisplayError("Failed to load word\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)N64MEM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x89:
		if (!r4300i_SW_NonMemory(MemAddress,*(DWORD *)Reg)) {
			if (ShowUnhandledMemory) {
				DisplayError("Failed to store word\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC6:
		if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!r4300i_SB_NonMemory(MemAddress,*(BYTE *)ReadPos)) {
			if (ShowUnhandledMemory) {
				DisplayError("Failed to store byte\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 1);
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC7:
		if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!r4300i_SW_NonMemory(MemAddress,*(DWORD *)ReadPos)) {
			if (ShowUnhandledMemory) {
				DisplayError("Failed to store word\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 4);
		return EXCEPTION_CONTINUE_EXECUTION;		
	default:
		DisplayError("Unkown x86 opcode %X\nlocation %X\nloc: %X\nfhfgh2", 
			*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)N64MEM);
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

void r4300i_LW_PAddr ( DWORD PAddr, DWORD * Value ) {
	*Value = *(DWORD *)(N64MEM+PAddr);

	if (LookUpMode == FuncFind_ChangeMemory)
	{
		BreakPoint(__FILE__,__LINE__);
//		if ( (Command.Hex >> 16) == 0x7C7C) {
//			Command.Hex = OrigMem[(Command.Hex & 0xFFFF)].OriginalValue;
//		}
	}
}

#endif