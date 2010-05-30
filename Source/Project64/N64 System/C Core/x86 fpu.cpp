#ifdef tofix

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
#include "main.h"
#include "cpu.h"
#include "x86.h"
#include "debugger.h"

#define PUTDST8(dest,value)  (*((BYTE *)(dest))=(BYTE)(value)); dest += 1;
#define PUTDST16(dest,value) (*((WORD *)(dest))=(WORD)(value)); dest += 2;
#define PUTDST32(dest,value) (*((DWORD *)(dest))=(DWORD)(value)); dest += 4;

#define fpu_Name(Reg)   (Reg) == x86_ST0  ? "ST(0)" : (Reg) == x86_ST1  ? "ST(1)" :\
						(Reg) == x86_ST2  ? "ST(2)" : (Reg) == x86_ST3  ? "ST(3)" :\
						(Reg) == x86_ST4  ? "ST(4)" : (Reg) == x86_ST5  ? "ST(5)" :\
						(Reg) == x86_ST6  ? "ST(6)" : (Reg) == x86_ST7  ? "ST(7)" :\
						"Unknown x86fpu Register"






















void fpuDivDwordReverse(void *Variable, const char * VariableName) {
	CPU_Message("      fdivr ST(0), dword ptr [%s]", VariableName);
	PUTDST16(RecompPos,0x3DD8);
	PUTDST32(RecompPos,Variable);
}


void fpuDivQwordReverse(void *Variable, const char * VariableName) {
	CPU_Message("      fdivr ST(0), qword ptr [%s]", VariableName);
	PUTDST16(RecompPos,0x3DDC);
	PUTDST32(RecompPos,Variable);
}

//
// FPU Utility
//

static unsigned int fpucontrol;

/* returns and pushes current fpu state, bool for set normal */
int fpuSaveControl(BOOL bSetNormal) {
	_asm {
		fnstcw word ptr [fpucontrol]
	}

	if (bSetNormal == TRUE) {
		unsigned short fpunormal = fpucontrol & 0xF3FF;
		_asm {
			fldcw word ptr [fpunormal]
		}
	}

	return fpucontrol;
}

/* returns and pops fpu state previously pushed */
int fpuRestoreControl() {
	_asm {
		fldcw word ptr [fpucontrol]
	}
	return fpucontrol;
}

void fpuSetupDouble(void) {
	int temp = 0;

	_asm {
		fnstcw word ptr [temp]
		or [temp], 0x300
		and [temp], 0xFFFFF3FF
		fldcw word ptr [temp]
	}
}

#endif