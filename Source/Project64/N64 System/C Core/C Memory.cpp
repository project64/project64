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
#include "plugin.h"
#include "debugger.h"

void ** JumpTable, ** DelaySlotTable;
BYTE *RecompCode, *RecompPos;

BOOL WrittenToRom;
DWORD WroteToRom;
DWORD TempValue;

void Compile_LB ( int Reg, DWORD VAddr, BOOL SignExtend) {
	DWORD PAddr;
	char VarName[100];

	if (!TranslateVaddr(VAddr,&PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LB\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_LB\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
	case 0x10000000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		if (SignExtend) {
			MoveSxVariableToX86regByte(PAddr + N64MEM,VarName,Reg); 
		} else {
			MoveZxVariableToX86regByte(PAddr + N64MEM,VarName,Reg); 
		}
		break;
	default:
		MoveConstToX86reg(0,Reg);
		if (ShowUnhandledMemory) { DisplayError("Compile_LB\nFailed to compile address: %X",VAddr); }
	}
}

void Compile_LH ( int Reg, DWORD VAddr, BOOL SignExtend) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LH\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_LH\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
	case 0x10000000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		if (SignExtend) {
			MoveSxVariableToX86regHalf(PAddr + N64MEM,VarName,Reg); 
		} else {
			MoveZxVariableToX86regHalf(PAddr + N64MEM,VarName,Reg); 
		}
		break;
	default:
		MoveConstToX86reg(0,Reg);
		if (ShowUnhandledMemory) { DisplayError("Compile_LHU\nFailed to compile address: %X",VAddr); }
	}
}

void Compile_LW ( CBlockSection * Section, int Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LW\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address %X",VAddr); }
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
	case 0x10000000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveVariableToX86reg(PAddr + N64MEM,VarName,Reg); 
		break;
	case 0x04000000:
		if (PAddr < 0x04002000) { 
			sprintf(VarName,"N64MEM + %X",PAddr);
			MoveVariableToX86reg(PAddr + N64MEM,VarName,Reg); 
			break; 
		}
		switch (PAddr) {
		case 0x04040010: MoveVariableToX86reg(&SP_STATUS_REG,"SP_STATUS_REG",Reg); break;
		case 0x04040014: MoveVariableToX86reg(&SP_DMA_FULL_REG,"SP_DMA_FULL_REG",Reg); break;
		case 0x04040018: MoveVariableToX86reg(&SP_DMA_BUSY_REG,"SP_DMA_BUSY_REG",Reg); break;
		case 0x04080000: MoveVariableToX86reg(&SP_PC_REG,"SP_PC_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04100000:
		if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveVariableToX86reg(PAddr + N64MEM,VarName,Reg); 
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: MoveVariableToX86reg(&MI_MODE_REG,"MI_MODE_REG",Reg); break;
		case 0x04300004: MoveVariableToX86reg(&MI_VERSION_REG,"MI_VERSION_REG",Reg); break;
		case 0x04300008: MoveVariableToX86reg(&MI_INTR_REG,"MI_INTR_REG",Reg); break;
		case 0x0430000C: MoveVariableToX86reg(&MI_INTR_MASK_REG,"MI_INTR_MASK_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400010:
			g_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
			Section->BlockCycleCount() = 0;
			Section->BlockRandomModifier() = 0;
			Pushad();
			Call_Direct(&UpdateCurrentHalfLine,"UpdateCurrentHalfLine");
			Popad();
			MoveVariableToX86reg(g_HalfLine,"g_HalfLine",Reg);
			break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500004: 
			if (g_FixedAudio)
			{
				g_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);			
				Pushad();
				MoveConstToX86reg((DWORD)g_Audio,x86_ECX);				
				Call_Direct(CAudio::AiGetLength,"AiGetLength");
				MoveX86regToVariable(x86_EAX,&TempValue,"TempValue"); 
				Popad();
				MoveVariableToX86reg(&TempValue,"TempValue",Reg);
			} else {
				if (AiReadLength != NULL) {
					Pushad();
					Call_Direct(AiReadLength,"AiReadLength");
					MoveX86regToVariable(x86_EAX,&TempValue,"TempValue"); 
					Popad();
					MoveVariableToX86reg(&TempValue,"TempValue",Reg);
				} else {
					MoveConstToX86reg(0,Reg);
				}						
			}
			break;
		case 0x0450000C: 
			if (g_FixedAudio)
			{
				Pushad();
				MoveConstToX86reg((DWORD)g_Audio,x86_ECX);
				Call_Direct(CAudio::AiGetStatus,"AiGetStatus");
				MoveX86regToVariable(x86_EAX,&TempValue,"TempValue"); 
				Popad();
				MoveVariableToX86reg(&TempValue,"TempValue",Reg);
			} else {
				MoveVariableToX86reg(&AI_STATUS_REG,"AI_STATUS_REG",Reg); 
			}
			break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600010: MoveVariableToX86reg(&PI_STATUS_REG,"PI_STATUS_REG",Reg); break;
		case 0x04600014: MoveVariableToX86reg(&PI_DOMAIN1_REG,"PI_DOMAIN1_REG",Reg); break;
		case 0x04600018: MoveVariableToX86reg(&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG",Reg); break;
		case 0x0460001C: MoveVariableToX86reg(&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG",Reg); break;
		case 0x04600020: MoveVariableToX86reg(&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG",Reg); break;
		case 0x04600024: MoveVariableToX86reg(&PI_DOMAIN2_REG,"PI_DOMAIN2_REG",Reg); break;
		case 0x04600028: MoveVariableToX86reg(&PI_BSD_DOM2_PWD_REG,"PI_BSD_DOM2_PWD_REG",Reg); break;
		case 0x0460002C: MoveVariableToX86reg(&PI_BSD_DOM2_PGS_REG,"PI_BSD_DOM2_PGS_REG",Reg); break;
		case 0x04600030: MoveVariableToX86reg(&PI_BSD_DOM2_RLS_REG,"PI_BSD_DOM2_RLS_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x0470000C: MoveVariableToX86reg(&RI_SELECT_REG,"RI_SELECT_REG",Reg); break;
		case 0x04700010: MoveVariableToX86reg(&RI_REFRESH_REG,"RI_REFRESH_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: MoveVariableToX86reg(&SI_STATUS_REG,"SI_STATUS_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (ShowUnhandledMemory) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x1FC00000:
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveVariableToX86reg(PAddr + N64MEM,VarName,Reg); 
		break;
	default:
		MoveConstToX86reg(((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF),Reg);
		if (ShowUnhandledMemory) { 
			CPU_Message("Compile_LW\nFailed to translate address: %X",VAddr); 
			DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); 
		}
	}
}

void Compile_SB_Const ( BYTE Value, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		CPU_Message("Compile_SB\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SB\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveConstByteToVariable(Value,PAddr + N64MEM,VarName); 
		break;
	default:
		if (ShowUnhandledMemory) { DisplayError("Compile_SB_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
}

void Compile_SB_Register ( int x86Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		CPU_Message("Compile_SB\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SB\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveX86regByteToVariable(x86Reg,PAddr + N64MEM,VarName); 
		break;
	default:
		if (ShowUnhandledMemory) { DisplayError("Compile_SB_Register\ntrying to store in %X?",VAddr); }
	}
}

void Compile_SH_Const ( WORD Value, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		CPU_Message("Compile_SH\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SH\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveConstHalfToVariable(Value,PAddr + N64MEM,VarName); 
		break;
	default:
		if (ShowUnhandledMemory) { DisplayError("Compile_SH_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
}

void Compile_SH_Register ( int x86Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		CPU_Message("Compile_SH\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SH\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveX86regHalfToVariable(x86Reg,PAddr + N64MEM,VarName); 
		break;
	default:
		if (ShowUnhandledMemory) { DisplayError("Compile_SH_Register\ntrying to store in %X?",PAddr); }
	}
}

void Compile_SW_Const ( DWORD Value, DWORD VAddr ) {
	char VarName[100];
	BYTE * Jump;
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		CPU_Message("Compile_SW\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SW\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveConstToVariable(Value,PAddr + N64MEM,VarName); 
		break;
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: MoveConstToVariable(Value,&RDRAM_CONFIG_REG,"RDRAM_CONFIG_REG"); break;
		case 0x03F00004: MoveConstToVariable(Value,&RDRAM_DEVICE_ID_REG,"RDRAM_DEVICE_ID_REG"); break;
		case 0x03F00008: MoveConstToVariable(Value,&RDRAM_DELAY_REG,"RDRAM_DELAY_REG"); break;
		case 0x03F0000C: MoveConstToVariable(Value,&RDRAM_MODE_REG,"RDRAM_MODE_REG"); break;
		case 0x03F00010: MoveConstToVariable(Value,&RDRAM_REF_INTERVAL_REG,"RDRAM_REF_INTERVAL_REG"); break;
		case 0x03F00014: MoveConstToVariable(Value,&RDRAM_REF_ROW_REG,"RDRAM_REF_ROW_REG"); break;
		case 0x03F00018: MoveConstToVariable(Value,&RDRAM_RAS_INTERVAL_REG,"RDRAM_RAS_INTERVAL_REG"); break;
		case 0x03F0001C: MoveConstToVariable(Value,&RDRAM_MIN_INTERVAL_REG,"RDRAM_MIN_INTERVAL_REG"); break;
		case 0x03F00020: MoveConstToVariable(Value,&RDRAM_ADDR_SELECT_REG,"RDRAM_ADDR_SELECT_REG"); break;
		case 0x03F00024: MoveConstToVariable(Value,&RDRAM_DEVICE_MANUF_REG,"RDRAM_DEVICE_MANUF_REG"); break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04000000:
		if (PAddr < 0x04002000) { 
			sprintf(VarName,"N64MEM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + N64MEM,VarName); 
			break;
		}
		switch (PAddr) {
		case 0x04040000: MoveConstToVariable(Value,&SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG"); break;
		case 0x04040004: MoveConstToVariable(Value,&SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG"); break;
		case 0x04040008:
			MoveConstToVariable(Value,&SP_RD_LEN_REG,"SP_RD_LEN_REG");
			Pushad();
			Call_Direct(&SP_DMA_READ,"SP_DMA_READ");
			Popad();
			break;
		case 0x04040010: 
			{
				DWORD ModValue;
				ModValue = 0;
				if ( ( Value & SP_CLR_HALT ) != 0 ) { ModValue |= SP_STATUS_HALT; }
				if ( ( Value & SP_CLR_BROKE ) != 0 ) { ModValue |= SP_STATUS_BROKE; }
				if ( ( Value & SP_CLR_SSTEP ) != 0 ) { ModValue |= SP_STATUS_SSTEP; }
				if ( ( Value & SP_CLR_INTR_BREAK ) != 0 ) { ModValue |= SP_STATUS_INTR_BREAK; }
				if ( ( Value & SP_CLR_SIG0 ) != 0 ) { ModValue |= SP_STATUS_SIG0; }
				if ( ( Value & SP_CLR_SIG1 ) != 0 ) { ModValue |= SP_STATUS_SIG1; }
				if ( ( Value & SP_CLR_SIG2 ) != 0 ) { ModValue |= SP_STATUS_SIG2; }
				if ( ( Value & SP_CLR_SIG3 ) != 0 ) { ModValue |= SP_STATUS_SIG3; }
				if ( ( Value & SP_CLR_SIG4 ) != 0 ) { ModValue |= SP_STATUS_SIG4; }
				if ( ( Value & SP_CLR_SIG5 ) != 0 ) { ModValue |= SP_STATUS_SIG5; }
				if ( ( Value & SP_CLR_SIG6 ) != 0 ) { ModValue |= SP_STATUS_SIG6; }
				if ( ( Value & SP_CLR_SIG7 ) != 0 ) { ModValue |= SP_STATUS_SIG7; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&SP_STATUS_REG,"SP_STATUS_REG");
				}

				ModValue = 0;
				if ( ( Value & SP_SET_HALT ) != 0 ) { ModValue |= SP_STATUS_HALT; }
				if ( ( Value & SP_SET_SSTEP ) != 0 ) { ModValue |= SP_STATUS_SSTEP; }
				if ( ( Value & SP_SET_INTR_BREAK ) != 0) { ModValue |= SP_STATUS_INTR_BREAK;  }
				if ( ( Value & SP_SET_SIG0 ) != 0 ) { ModValue |= SP_STATUS_SIG0; }
				if ( ( Value & SP_SET_SIG1 ) != 0 ) { ModValue |= SP_STATUS_SIG1; }
				if ( ( Value & SP_SET_SIG2 ) != 0 ) { ModValue |= SP_STATUS_SIG2; }
				if ( ( Value & SP_SET_SIG3 ) != 0 ) { ModValue |= SP_STATUS_SIG3; }
				if ( ( Value & SP_SET_SIG4 ) != 0 ) { ModValue |= SP_STATUS_SIG4; }
				if ( ( Value & SP_SET_SIG5 ) != 0 ) { ModValue |= SP_STATUS_SIG5; }
				if ( ( Value & SP_SET_SIG6 ) != 0 ) { ModValue |= SP_STATUS_SIG6; }
				if ( ( Value & SP_SET_SIG7 ) != 0 ) { ModValue |= SP_STATUS_SIG7; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&SP_STATUS_REG,"SP_STATUS_REG");
				}
				if ( ( Value & SP_SET_SIG0 ) != 0 && AudioSignal ) 
				{ 
					OrConstToVariable(MI_INTR_SP,&MI_INTR_REG,"MI_INTR_REG");
					Pushad();
					Call_Direct(CheckInterrupts,"CheckInterrupts");
					Popad();
				}
				if ( ( Value & SP_CLR_INTR ) != 0) { 
					AndConstToVariable(~MI_INTR_SP,&MI_INTR_REG,"MI_INTR_REG");
					Pushad();
					Call_Direct(RunRsp,"RunRsp");
					Call_Direct(CheckInterrupts,"CheckInterrupts");
					Popad();
				} else {
					Pushad();
					Call_Direct(RunRsp,"RunRsp");
					Popad();
				}
			}
			break;
		case 0x0404001C: MoveConstToVariable(0,&SP_SEMAPHORE_REG,"SP_SEMAPHORE_REG"); break;
		case 0x04080000: MoveConstToVariable(Value & 0xFFC,&SP_PC_REG,"SP_PC_REG"); break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			{
				DWORD ModValue;
				ModValue = 0x7F;
				if ( ( Value & MI_CLR_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_CLR_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_CLR_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&MI_MODE_REG,"MI_MODE_REG");
				}

				ModValue = (Value & 0x7F);
				if ( ( Value & MI_SET_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_SET_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_SET_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&MI_MODE_REG,"MI_MODE_REG");
				}
				if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
					AndConstToVariable(~MI_INTR_DP,&MI_INTR_REG,"MI_INTR_REG");
				}
			}
			break;
		case 0x0430000C: 
			{
				DWORD ModValue;
				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&MI_INTR_MASK_REG,"MI_INTR_MASK_REG");
				}

				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&MI_INTR_MASK_REG,"MI_INTR_MASK_REG");
				}
			}
			break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (ViStatusChanged != NULL) {
				CompConstToVariable(Value,&VI_STATUS_REG,"VI_STATUS_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveConstToVariable(Value,&VI_STATUS_REG,"VI_STATUS_REG");
				Pushad();
				Call_Direct(ViStatusChanged,"ViStatusChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x04400004: MoveConstToVariable((Value & 0xFFFFFF),&VI_ORIGIN_REG,"VI_ORIGIN_REG"); break;
		case 0x04400008: 
			if (ViWidthChanged != NULL) {
				CompConstToVariable(Value,&VI_WIDTH_REG,"VI_WIDTH_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveConstToVariable(Value,&VI_WIDTH_REG,"VI_WIDTH_REG");
				Pushad();
				Call_Direct(ViWidthChanged,"ViWidthChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x0440000C: MoveConstToVariable(Value,&VI_INTR_REG,"VI_INTR_REG"); break;
		case 0x04400010: 
			AndConstToVariable(~MI_INTR_VI,&MI_INTR_REG,"MI_INTR_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04400014: MoveConstToVariable(Value,&VI_BURST_REG,"VI_BURST_REG"); break;
		case 0x04400018: MoveConstToVariable(Value,&VI_V_SYNC_REG,"VI_V_SYNC_REG"); break;
		case 0x0440001C: MoveConstToVariable(Value,&VI_H_SYNC_REG,"VI_H_SYNC_REG"); break;
		case 0x04400020: MoveConstToVariable(Value,&VI_LEAP_REG,"VI_LEAP_REG"); break;
		case 0x04400024: MoveConstToVariable(Value,&VI_H_START_REG,"VI_H_START_REG"); break;
		case 0x04400028: MoveConstToVariable(Value,&VI_V_START_REG,"VI_V_START_REG"); break;
		case 0x0440002C: MoveConstToVariable(Value,&VI_V_BURST_REG,"VI_V_BURST_REG"); break;
		case 0x04400030: MoveConstToVariable(Value,&VI_X_SCALE_REG,"VI_X_SCALE_REG"); break;
		case 0x04400034: MoveConstToVariable(Value,&VI_Y_SCALE_REG,"VI_Y_SCALE_REG"); break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500000: MoveConstToVariable(Value,&AI_DRAM_ADDR_REG,"AI_DRAM_ADDR_REG"); break;
		case 0x04500004: 
			MoveConstToVariable(Value,&AI_LEN_REG,"AI_LEN_REG");
			Pushad();
			if (g_FixedAudio)
			{
				X86BreakPoint(__FILE__,__LINE__);
				MoveConstToX86reg((DWORD)Value,x86_EDX);				
				MoveConstToX86reg((DWORD)g_Audio,x86_ECX);				
				Call_Direct(CAudio::AiSetLength,"AiSetLength");
			}
			Call_Direct(AiLenChanged,"AiLenChanged");
			Popad();
			break;
		case 0x04500008: MoveConstToVariable((Value & 1),&AI_CONTROL_REG,"AI_CONTROL_REG"); break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			AndConstToVariable(~MI_INTR_AI,&MI_INTR_REG,"MI_INTR_REG");
			if (!g_FixedAudio)
			{
				AndConstToVariable(~MI_INTR_AI,&AudioIntrReg,"AudioIntrReg");
			}
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04500010: 
			sprintf(VarName,"N64MEM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + N64MEM,VarName); 
			break;
		case 0x04500014: MoveConstToVariable(Value,&AI_BITRATE_REG,"AI_BITRATE_REG"); break;
		default:
			sprintf(VarName,"N64MEM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + N64MEM,VarName); 
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: MoveConstToVariable(Value,&PI_DRAM_ADDR_REG,"PI_DRAM_ADDR_REG"); break;
		case 0x04600004: MoveConstToVariable(Value,&PI_CART_ADDR_REG,"PI_CART_ADDR_REG"); break;
		case 0x04600008: 
			MoveConstToVariable(Value,&PI_RD_LEN_REG,"PI_RD_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_READ,"PI_DMA_READ");
			Popad();
			break;
		case 0x0460000C:
			MoveConstToVariable(Value,&PI_WR_LEN_REG,"PI_WR_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_WRITE,"PI_DMA_WRITE");
			Popad();
			break;
		case 0x04600010: 
			if ((Value & PI_CLR_INTR) != 0 ) {
				AndConstToVariable(~MI_INTR_PI,&MI_INTR_REG,"MI_INTR_REG");
				Pushad();
				Call_Direct(CheckInterrupts,"CheckInterrupts");
				Popad();
			}
			break;
		case 0x04600014: MoveConstToVariable((Value & 0xFF),&PI_DOMAIN1_REG,"PI_DOMAIN1_REG"); break;
		case 0x04600018: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); break;
		case 0x0460001C: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); break;
		case 0x04600020: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: MoveConstToVariable(Value,&RI_MODE_REG,"RI_MODE_REG"); break;
		case 0x04700004: MoveConstToVariable(Value,&RI_CONFIG_REG,"RI_CONFIG_REG"); break;
		case 0x04700008: MoveConstToVariable(Value,&RI_CURRENT_LOAD_REG,"RI_CURRENT_LOAD_REG"); break;
		case 0x0470000C: MoveConstToVariable(Value,&RI_SELECT_REG,"RI_SELECT_REG"); break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: MoveConstToVariable(Value,&SI_DRAM_ADDR_REG,"SI_DRAM_ADDR_REG"); break;
		case 0x04800004: 			
			MoveConstToVariable(Value,&SI_PIF_ADDR_RD64B_REG,"SI_PIF_ADDR_RD64B_REG");		
			Pushad();
			Call_Direct(&SI_DMA_READ,"SI_DMA_READ");
			Popad();
			break;
		case 0x04800010: 
			MoveConstToVariable(Value,&SI_PIF_ADDR_WR64B_REG,"SI_PIF_ADDR_WR64B_REG");
			Pushad();
			Call_Direct(&SI_DMA_WRITE,"SI_DMA_WRITE");
			Popad();
			break;
		case 0x04800018: 
			AndConstToVariable(~MI_INTR_SI,&MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable(~SI_STATUS_INTERRUPT,&SI_STATUS_REG,"SI_STATUS_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	default:
		if (ShowUnhandledMemory) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
}

void Compile_SW_Register ( CBlockSection * Section, int x86Reg, DWORD VAddr ) {
	char VarName[100];
	BYTE * Jump;
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, &PAddr)) {
		CPU_Message("Compile_SW_Register\nFailed to translate address %X",VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveX86regToVariable(x86Reg,PAddr + N64MEM,VarName); 
		break;
	case 0x04000000: 
		switch (PAddr) {
		case 0x04040000: MoveX86regToVariable(x86Reg,&SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG"); break;
		case 0x04040004: MoveX86regToVariable(x86Reg,&SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG"); break;
		case 0x04040008: 
			MoveX86regToVariable(x86Reg,&SP_RD_LEN_REG,"SP_RD_LEN_REG");
			Pushad();
			Call_Direct(&SP_DMA_READ,"SP_DMA_READ");
			Popad();
			break;
		case 0x0404000C: 
			MoveX86regToVariable(x86Reg,&SP_WR_LEN_REG,"SP_WR_LEN_REG");
			Pushad();
			Call_Direct(&SP_DMA_WRITE,"SP_DMA_WRITE");
			Popad();
			break;
		case 0x04040010: 
			MoveX86regToVariable(x86Reg,&RegModValue,"RegModValue");
			Pushad();
			Call_Direct(ChangeSpStatus,"ChangeSpStatus");
			Popad();
			break;
		case 0x0404001C: MoveConstToVariable(0,&SP_SEMAPHORE_REG,"SP_SEMAPHORE_REG"); break;
		case 0x04080000: 
			MoveX86regToVariable(x86Reg,&SP_PC_REG,"SP_PC_REG");
			AndConstToVariable(0xFFC,&SP_PC_REG,"SP_PC_REG");
			break;
		default:
			if (PAddr < 0x04002000) {
				sprintf(VarName,"N64MEM + %X",PAddr);
				MoveX86regToVariable(x86Reg,PAddr + N64MEM,VarName); 
			} else {
				CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
				if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
			}
		}
		break;
	case 0x04100000: 
		CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveX86regToVariable(x86Reg,PAddr + N64MEM,VarName); 
		if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			MoveX86regToVariable(x86Reg,&RegModValue,"RegModValue");
			Pushad();
			Call_Direct(ChangeMiIntrMask,"ChangeMiModeReg");
			Popad();
			break;
		case 0x0430000C: 
			MoveX86regToVariable(x86Reg,&RegModValue,"RegModValue");
			Pushad();
			Call_Direct(ChangeMiIntrMask,"ChangeMiIntrMask");
			Popad();
			break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (ViStatusChanged != NULL) {
				CompX86regToVariable(x86Reg,&VI_STATUS_REG,"VI_STATUS_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveX86regToVariable(x86Reg,&VI_STATUS_REG,"VI_STATUS_REG");
				Pushad();
				Call_Direct(ViStatusChanged,"ViStatusChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x04400004: 
			MoveX86regToVariable(x86Reg,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			AndConstToVariable(0xFFFFFF,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			break;
		case 0x04400008: 
			if (ViWidthChanged != NULL) {
				CompX86regToVariable(x86Reg,&VI_WIDTH_REG,"VI_WIDTH_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveX86regToVariable(x86Reg,&VI_WIDTH_REG,"VI_WIDTH_REG");
				Pushad();
				Call_Direct(ViWidthChanged,"ViWidthChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x0440000C: MoveX86regToVariable(x86Reg,&VI_INTR_REG,"VI_INTR_REG"); break;
		case 0x04400010: 
			AndConstToVariable(~MI_INTR_VI,&MI_INTR_REG,"MI_INTR_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04400014: MoveX86regToVariable(x86Reg,&VI_BURST_REG,"VI_BURST_REG"); break;
		case 0x04400018: MoveX86regToVariable(x86Reg,&VI_V_SYNC_REG,"VI_V_SYNC_REG"); break;
		case 0x0440001C: MoveX86regToVariable(x86Reg,&VI_H_SYNC_REG,"VI_H_SYNC_REG"); break;
		case 0x04400020: MoveX86regToVariable(x86Reg,&VI_LEAP_REG,"VI_LEAP_REG"); break;
		case 0x04400024: MoveX86regToVariable(x86Reg,&VI_H_START_REG,"VI_H_START_REG"); break;
		case 0x04400028: MoveX86regToVariable(x86Reg,&VI_V_START_REG,"VI_V_START_REG"); break;
		case 0x0440002C: MoveX86regToVariable(x86Reg,&VI_V_BURST_REG,"VI_V_BURST_REG"); break;
		case 0x04400030: MoveX86regToVariable(x86Reg,&VI_X_SCALE_REG,"VI_X_SCALE_REG"); break;
		case 0x04400034: MoveX86regToVariable(x86Reg,&VI_Y_SCALE_REG,"VI_Y_SCALE_REG"); break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500000: MoveX86regToVariable(x86Reg,&AI_DRAM_ADDR_REG,"AI_DRAM_ADDR_REG"); break;
		case 0x04500004: 
			MoveX86regToVariable(x86Reg,&AI_LEN_REG,"AI_LEN_REG");
			Pushad();
			if (g_FixedAudio)
			{
				g_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);			
				MoveX86RegToX86Reg(x86Reg,x86_EDX);				
				MoveConstToX86reg((DWORD)g_Audio,x86_ECX);				
				Call_Direct(CAudio::AiSetLength,"AiSetLength");
			}
			Call_Direct(AiLenChanged,"AiLenChanged");
			Popad();
			break;
		case 0x04500008: 
			MoveX86regToVariable(x86Reg,&AI_CONTROL_REG,"AI_CONTROL_REG");
			AndConstToVariable(1,&AI_CONTROL_REG,"AI_CONTROL_REG");
		case 0x0450000C:
			/* Clear Interrupt */; 
			AndConstToVariable(~MI_INTR_AI,&MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable(~MI_INTR_AI,&AudioIntrReg,"AudioIntrReg");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04500010: 
			sprintf(VarName,"N64MEM + %X",PAddr);
			MoveX86regToVariable(x86Reg,PAddr + N64MEM,VarName); 
			break;
		case 0x04500014: MoveX86regToVariable(x86Reg,&AI_BITRATE_REG,"AI_BITRATE_REG"); break;
		default:
			sprintf(VarName,"N64MEM + %X",PAddr);
			MoveX86regToVariable(x86Reg,PAddr + N64MEM,VarName); 
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: MoveX86regToVariable(x86Reg,&PI_DRAM_ADDR_REG,"PI_DRAM_ADDR_REG"); break;
		case 0x04600004: MoveX86regToVariable(x86Reg,&PI_CART_ADDR_REG,"PI_CART_ADDR_REG"); break;
		case 0x04600008:
			MoveX86regToVariable(x86Reg,&PI_RD_LEN_REG,"PI_RD_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_READ,"PI_DMA_READ");
			Popad();
			break;
		case 0x0460000C:
			MoveX86regToVariable(x86Reg,&PI_WR_LEN_REG,"PI_WR_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_WRITE,"PI_DMA_WRITE");
			Popad();
			break;
		case 0x04600010: 
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
			AndConstToVariable(~MI_INTR_PI,&MI_INTR_REG,"MI_INTR_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
			MoveX86regToVariable(x86Reg,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			AndConstToVariable(0xFFFFFF,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
		case 0x04600014: 
			MoveX86regToVariable(x86Reg,&PI_DOMAIN1_REG,"PI_DOMAIN1_REG");
			AndConstToVariable(0xFF,&PI_DOMAIN1_REG,"PI_DOMAIN1_REG"); 
			break;
		case 0x04600018: 
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); 
			AndConstToVariable(0xFF,&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); 
			break;
		case 0x0460001C: 
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); 
			AndConstToVariable(0xFF,&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); 
			break;
		case 0x04600020: 
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); 
			AndConstToVariable(0xFF,&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); 
			break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700010: MoveX86regToVariable(x86Reg,&RI_REFRESH_REG,"RI_REFRESH_REG"); break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: MoveX86regToVariable(x86Reg,&SI_DRAM_ADDR_REG,"SI_DRAM_ADDR_REG"); break;
		case 0x04800004: 
			MoveX86regToVariable(x86Reg,&SI_PIF_ADDR_RD64B_REG,"SI_PIF_ADDR_RD64B_REG"); 
			Pushad();
			Call_Direct(&SI_DMA_READ,"SI_DMA_READ");
			Popad();
			break;
		case 0x04800010: 
			MoveX86regToVariable(x86Reg,&SI_PIF_ADDR_WR64B_REG,"SI_PIF_ADDR_WR64B_REG"); 
			Pushad();
			Call_Direct(&SI_DMA_WRITE,"SI_DMA_WRITE");
			Popad();
			break;
		case 0x04800018: 
			AndConstToVariable(~MI_INTR_SI,&MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable(~SI_STATUS_INTERRUPT,&SI_STATUS_REG,"SI_STATUS_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		default:
			if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x1FC00000:
		sprintf(VarName,"N64MEM + %X",PAddr);
		MoveX86regToVariable(x86Reg,PAddr + N64MEM,VarName); 
		break;
	default:
		CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
		if (ShowUnhandledMemory) { DisplayError("Compile_SW_Register\ntrying to store in %X?",VAddr); }
	}
}

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

int r4300i_LB_NonMemory ( DWORD PAddr, DWORD * Value, BOOL SignExtend ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if (WrittenToRom) { return FALSE; }
		if ((PAddr & 2) == 0) { PAddr = (PAddr + 4) ^ 2; }
		if ((PAddr - 0x10000000) < RomFileSize) {
			if (SignExtend) {
				*Value = (int)((char)ROM[PAddr - 0x10000000]);
			} else {
				*Value = ROM[PAddr - 0x10000000];
			}
			return TRUE;
		} else {
			*Value = 0;
			return FALSE;
		}
	}

//	switch (PAddr & 0xFFF00000) {
//	default:
		* Value = 0;
		return FALSE;
//		break;
//	}
//	return TRUE;
}

BOOL r4300i_LB_VAddr ( DWORD VAddr, BYTE * Value ) {
	if (TLB_ReadMap[VAddr >> 12] == 0) { return FALSE; }
	*Value = *(BYTE *)(TLB_ReadMap[VAddr >> 12] + (VAddr ^ 3));
	return TRUE;
}

BOOL r4300i_LD_VAddr ( DWORD VAddr, unsigned _int64 * Value ) {
	if (TLB_ReadMap[VAddr >> 12] == 0) { return FALSE; }
	*((DWORD *)(Value) + 1) = *(DWORD *)(TLB_ReadMap[VAddr >> 12] + VAddr);
	*((DWORD *)(Value)) = *(DWORD *)(TLB_ReadMap[VAddr >> 12] + VAddr + 4);
	return TRUE;
}

int r4300i_LH_NonMemory ( DWORD PAddr, DWORD * Value, int SignExtend ) {
//	switch (PAddr & 0xFFF00000) {
//	default:
		* Value = 0;
		return FALSE;
//		break;
//	}
//	return TRUE;
}

BOOL r4300i_LH_VAddr ( DWORD VAddr, WORD * Value ) {
	if (TLB_ReadMap[VAddr >> 12] == 0) { return FALSE; }
	*Value = *(WORD *)(TLB_ReadMap[VAddr >> 12] + (VAddr ^ 2));
	return TRUE;
}

int r4300i_LW_NonMemory ( DWORD PAddr, DWORD * Value ) {
#ifdef CFB_READ
	if (PAddr >= CFBStart && PAddr < CFBEnd) {
		DWORD OldProtect;
		VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,PAGE_READONLY, &OldProtect);
		if (FrameBufferRead) { FrameBufferRead(PAddr & ~0xFFF); }
		*Value = *(DWORD *)(N64MEM+PAddr);
		return TRUE;
	}	
#endif
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if (WrittenToRom) { 
			*Value = WroteToRom;
			//LogMessage("%X: Read crap from Rom %X from %X",PROGRAM_COUNTER,*Value,PAddr);
			WrittenToRom = FALSE;
#ifdef ROM_IN_MAPSPACE
			{
				DWORD OldProtect;
				VirtualProtect(ROM,RomFileSize,PAGE_READONLY, &OldProtect);
			}
#endif
			return TRUE;
		}
		if ((PAddr - 0x10000000) < RomFileSize) {
			*Value = *(DWORD *)&ROM[PAddr - 0x10000000];
			return TRUE;
		} else {
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return FALSE;
		}
	}

	switch (PAddr & 0xFFF00000) {
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: * Value = RDRAM_CONFIG_REG; break;
		case 0x03F00004: * Value = RDRAM_DEVICE_ID_REG; break;
		case 0x03F00008: * Value = RDRAM_DELAY_REG; break;
		case 0x03F0000C: * Value = RDRAM_MODE_REG; break;
		case 0x03F00010: * Value = RDRAM_REF_INTERVAL_REG; break;
		case 0x03F00014: * Value = RDRAM_REF_ROW_REG; break;
		case 0x03F00018: * Value = RDRAM_RAS_INTERVAL_REG; break;
		case 0x03F0001C: * Value = RDRAM_MIN_INTERVAL_REG; break;
		case 0x03F00020: * Value = RDRAM_ADDR_SELECT_REG; break;
		case 0x03F00024: * Value = RDRAM_DEVICE_MANUF_REG; break;	
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04000000:
		switch (PAddr) {
		case 0x04040010: *Value = SP_STATUS_REG; break;
		case 0x04040014: *Value = SP_DMA_FULL_REG; break;
		case 0x04040018: *Value = SP_DMA_BUSY_REG; break;
		case 0x04080000: *Value = SP_PC_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x0410000C: *Value = DPC_STATUS_REG; break;
		case 0x04100010: *Value = DPC_CLOCK_REG; break;
		case 0x04100014: *Value = DPC_BUFBUSY_REG; break;
		case 0x04100018: *Value = DPC_PIPEBUSY_REG; break;
		case 0x0410001C: *Value = DPC_TMEM_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: * Value = MI_MODE_REG; break;
		case 0x04300004: * Value = MI_VERSION_REG; break;
		case 0x04300008: * Value = MI_INTR_REG; break;
		case 0x0430000C: * Value = MI_INTR_MASK_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04400000:
		switch (PAddr) {
		case 0x04400000: *Value = VI_STATUS_REG; break;
		case 0x04400004: *Value = VI_ORIGIN_REG; break;
		case 0x04400008: *Value = VI_WIDTH_REG; break;
		case 0x0440000C: *Value = VI_INTR_REG; break;
		case 0x04400010: 
			UpdateCurrentHalfLine();
			*Value = *g_HalfLine; 
			break;
		case 0x04400014: *Value = VI_BURST_REG; break;
		case 0x04400018: *Value = VI_V_SYNC_REG; break;
		case 0x0440001C: *Value = VI_H_SYNC_REG; break;
		case 0x04400020: *Value = VI_LEAP_REG; break;
		case 0x04400024: *Value = VI_H_START_REG; break;
		case 0x04400028: *Value = VI_V_START_REG ; break;
		case 0x0440002C: *Value = VI_V_BURST_REG; break;
		case 0x04400030: *Value = VI_X_SCALE_REG; break;
		case 0x04400034: *Value = VI_Y_SCALE_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04500000:
		switch (PAddr) {
		case 0x04500004: 
			if (g_FixedAudio)
			{
				*Value = CAudio::AiGetLength(g_Audio);
			} else {
				if (AiReadLength != NULL) {
					*Value = AiReadLength(); 
				} else {
					*Value = 0;
				}
			}
			break;
		case 0x0450000C: 
			if (g_FixedAudio)
			{
				*Value = CAudio::AiGetStatus(g_Audio);
			} else {
				*Value = AI_STATUS_REG; 
			}
			break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600010: *Value = PI_STATUS_REG; break;
		case 0x04600014: *Value = PI_DOMAIN1_REG; break;
		case 0x04600018: *Value = PI_BSD_DOM1_PWD_REG; break;
		case 0x0460001C: *Value = PI_BSD_DOM1_PGS_REG; break;
		case 0x04600020: *Value = PI_BSD_DOM1_RLS_REG; break;
		case 0x04600024: *Value = PI_DOMAIN2_REG; break;
		case 0x04600028: *Value = PI_BSD_DOM2_PWD_REG; break;
		case 0x0460002C: *Value = PI_BSD_DOM2_PGS_REG; break;
		case 0x04600030: *Value = PI_BSD_DOM2_RLS_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: * Value = RI_MODE_REG; break;
		case 0x04700004: * Value = RI_CONFIG_REG; break;
		case 0x04700008: * Value = RI_CURRENT_LOAD_REG; break;
		case 0x0470000C: * Value = RI_SELECT_REG; break;
		case 0x04700010: * Value = RI_REFRESH_REG; break;
		case 0x04700014: * Value = RI_LATENCY_REG; break;
		case 0x04700018: * Value = RI_RERROR_REG; break;
		case 0x0470001C: * Value = RI_WERROR_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: *Value = SI_STATUS_REG; break;
		default:
			*Value = 0;
			return FALSE;
		}
		break;
	case 0x05000000:
		*Value = PAddr & 0xFFFF;
		*Value = (*Value << 16) | *Value;
		return FALSE;
	case 0x08000000:
		if (SaveUsing == SaveChip_Auto) { SaveUsing = SaveChip_FlashRam; }
		if (SaveUsing != SaveChip_FlashRam) { 
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return FALSE;
		}
		*Value = ReadFromFlashStatus(PAddr);
		break;
	case 0x1FC00000:
		if (PAddr < 0x1FC007C0) {
/*			DWORD ToSwap = *(DWORD *)(&PifRom[PAddr - 0x1FC00000]);
			_asm {
				mov eax,ToSwap
				bswap eax
				mov ToSwap,eax
			}
			* Value = ToSwap;*/
			BreakPoint(__FILE__,__LINE__);
			return TRUE;
		} else if (PAddr < 0x1FC00800) {
			DWORD ToSwap = *(DWORD *)(&PIF_Ram[PAddr - 0x1FC007C0]);
			_asm {
				mov eax,ToSwap
				bswap eax
				mov ToSwap,eax
			}
			* Value = ToSwap;
			return TRUE;
		} else {
			* Value = 0;
			return FALSE;
		}
		BreakPoint(__FILE__,__LINE__);
		break;
	default:
		*Value = PAddr & 0xFFFF;
		*Value = (*Value << 16) | *Value;
		return FALSE;
		break;
	}
	return TRUE;
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

BOOL r4300i_LW_VAddr ( DWORD VAddr, DWORD * Value ) {
	BYTE * BaseAddress = (BYTE *)TLB_ReadMap[VAddr >> 12];
	if (BaseAddress == 0) { return FALSE; }
	*Value = *(DWORD *)(BaseAddress + VAddr);

	if (LookUpMode == FuncFind_ChangeMemory)
	{
		BreakPoint(__FILE__,__LINE__);
//		if ( (Command.Hex >> 16) == 0x7C7C) {
//			Command.Hex = OrigMem[(Command.Hex & 0xFFFF)].OriginalValue;
//		}
	}
	return TRUE;
}

int r4300i_SB_NonMemory ( DWORD PAddr, BYTE Value ) {
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
#ifdef CFB_READ
		if (PAddr >= CFBStart && PAddr < CFBEnd) {
			DWORD OldProtect;
			VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(BYTE *)(N64MEM+PAddr) = Value;
			VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,OldProtect, &OldProtect);
			DisplayError("FrameBufferWrite");
			if (FrameBufferWrite) { FrameBufferWrite(PAddr,1); }
			break;
		}	
#endif
		if (PAddr < RdramSize) {
			bool ProtectMem = false;
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((N64MEM + PAddr), 1, PAGE_READWRITE, &OldProtect);
			*(BYTE *)(N64MEM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((N64MEM + PAddr), 1, PAGE_READONLY, &OldProtect);
			}
		}
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

BOOL r4300i_SB_VAddr ( DWORD VAddr, BYTE Value ) {
	if (TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(BYTE *)(TLB_WriteMap[VAddr >> 12] + (VAddr ^ 3)) = Value;
	return TRUE;
}

int r4300i_SH_NonMemory ( DWORD PAddr, WORD Value ) {
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
#ifdef CFB_READ
		if (PAddr >= CFBStart && PAddr < CFBEnd) {
			DWORD OldProtect;
			VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(WORD *)(N64MEM+PAddr) = Value;
			if (FrameBufferWrite) { FrameBufferWrite(PAddr & ~0xFFF,2); }
			//*(WORD *)(N64MEM+PAddr) = 0xFFFF;
			//VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,PAGE_NOACCESS, &OldProtect);
			DisplayError("PAddr = %x",PAddr);
			break;
		}	
#endif
		if (PAddr < RdramSize) {
			bool ProtectMem = false;
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((N64MEM + PAddr), 1, PAGE_READWRITE, &OldProtect);
			*(WORD *)(N64MEM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((N64MEM + PAddr), 1, PAGE_READONLY, &OldProtect);
			}
		}
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

BOOL r4300i_SD_VAddr ( DWORD VAddr, unsigned _int64 Value ) {
	if (TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(DWORD *)(TLB_WriteMap[VAddr >> 12] + VAddr) = *((DWORD *)(&Value) + 1);
	*(DWORD *)(TLB_WriteMap[VAddr >> 12] + VAddr + 4) = *((DWORD *)(&Value));
	return TRUE;
}

BOOL r4300i_SH_VAddr ( DWORD VAddr, WORD Value ) {
	if (TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(WORD *)(TLB_WriteMap[VAddr >> 12] + (VAddr ^ 2)) = Value;
	return TRUE;
}

int r4300i_SW_NonMemory ( DWORD PAddr, DWORD Value ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if ((PAddr - 0x10000000) < RomFileSize) {
			WrittenToRom = TRUE;
			WroteToRom = Value;
#ifdef ROM_IN_MAPSPACE
			{
				DWORD OldProtect;
				VirtualProtect(ROM,RomFileSize,PAGE_NOACCESS, &OldProtect);
			}
#endif
			//LogMessage("%X: Wrote To Rom %X from %X",PROGRAM_COUNTER,Value,PAddr);
		} else {
			return FALSE;
		}
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
#ifdef CFB_READ
		if (PAddr >= CFBStart && PAddr < CFBEnd) {
			DWORD OldProtect;
			VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(DWORD *)(N64MEM+PAddr) = Value;
			VirtualProtect(N64MEM+(PAddr & ~0xFFF),0xFFC,OldProtect, &OldProtect);
			DisplayError("FrameBufferWrite %X",PAddr);
			if (FrameBufferWrite) { FrameBufferWrite(PAddr,4); }
			break;
		}	
#endif
		if (PAddr < RdramSize) {
			bool ProtectMem = false;
			
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((N64MEM + PAddr), 4, PAGE_READWRITE, &OldProtect);
			*(DWORD *)(N64MEM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((N64MEM + PAddr), 4, PAGE_READONLY, &OldProtect);
			}
		}
		break;
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: RDRAM_CONFIG_REG = Value; break;
		case 0x03F00004: RDRAM_DEVICE_ID_REG = Value; break;
		case 0x03F00008: RDRAM_DELAY_REG = Value; break;
		case 0x03F0000C: RDRAM_MODE_REG = Value; break;
		case 0x03F00010: RDRAM_REF_INTERVAL_REG = Value; break;
		case 0x03F00014: RDRAM_REF_ROW_REG = Value; break;
		case 0x03F00018: RDRAM_RAS_INTERVAL_REG = Value; break;
		case 0x03F0001C: RDRAM_MIN_INTERVAL_REG = Value; break;
		case 0x03F00020: RDRAM_ADDR_SELECT_REG = Value; break;
		case 0x03F00024: RDRAM_DEVICE_MANUF_REG = Value; break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			return FALSE;
		}
		break;
	case 0x04000000: 
		if (PAddr < 0x04002000) {
			bool ProtectMem = false;
			
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((N64MEM + PAddr), 4, PAGE_READWRITE, &OldProtect);
			*(DWORD *)(N64MEM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((N64MEM + PAddr), 4, PAGE_READONLY, &OldProtect);
			}
		} else {
			switch (PAddr) {
			case 0x04040000: SP_MEM_ADDR_REG = Value; break;
			case 0x04040004: SP_DRAM_ADDR_REG = Value; break;
			case 0x04040008: 
				SP_RD_LEN_REG = Value; 
				SP_DMA_READ();
				break;
			case 0x0404000C: 
				SP_WR_LEN_REG = Value; 
				SP_DMA_WRITE();
				break;
			case 0x04040010: 
				if ( ( Value & SP_CLR_HALT ) != 0) { SP_STATUS_REG &= ~SP_STATUS_HALT; }
				if ( ( Value & SP_SET_HALT ) != 0) { SP_STATUS_REG |= SP_STATUS_HALT;  }
				if ( ( Value & SP_CLR_BROKE ) != 0) { SP_STATUS_REG &= ~SP_STATUS_BROKE; }
				if ( ( Value & SP_CLR_INTR ) != 0) { 
					MI_INTR_REG &= ~MI_INTR_SP; 
					CheckInterrupts();
				}
	#ifndef EXTERNAL_RELEASE
				if ( ( Value & SP_SET_INTR ) != 0) { DisplayError("SP_SET_INTR"); }
	#endif
				if ( ( Value & SP_CLR_SSTEP ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
				if ( ( Value & SP_SET_SSTEP ) != 0) { SP_STATUS_REG |= SP_STATUS_SSTEP;  }
				if ( ( Value & SP_CLR_INTR_BREAK ) != 0) { SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
				if ( ( Value & SP_SET_INTR_BREAK ) != 0) { SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
				if ( ( Value & SP_CLR_SIG0 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG0; }
				if ( ( Value & SP_SET_SIG0 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG0;  }
				if ( ( Value & SP_CLR_SIG1 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG1; }
				if ( ( Value & SP_SET_SIG1 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG1;  }
				if ( ( Value & SP_CLR_SIG2 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG2; }
				if ( ( Value & SP_SET_SIG2 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG2;  }
				if ( ( Value & SP_CLR_SIG3 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG3; }
				if ( ( Value & SP_SET_SIG3 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG3;  }
				if ( ( Value & SP_CLR_SIG4 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG4; }
				if ( ( Value & SP_SET_SIG4 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG4;  }
				if ( ( Value & SP_CLR_SIG5 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG5; }
				if ( ( Value & SP_SET_SIG5 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG5;  }
				if ( ( Value & SP_CLR_SIG6 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG6; }
				if ( ( Value & SP_SET_SIG6 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG6;  }
				if ( ( Value & SP_CLR_SIG7 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG7; }
				if ( ( Value & SP_SET_SIG7 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG7;  }
				
				if ( ( Value & SP_SET_SIG0 ) != 0 && AudioSignal) 
				{ 
					MI_INTR_REG |= MI_INTR_SP; 
					CheckInterrupts();				
				}
				//if (*( DWORD *)(DMEM + 0xFC0) == 1) {
				//	ChangeTimer(RspTimer,0x30000);
				//} else {
					RunRsp();
				//}
				break;
			case 0x0404001C: SP_SEMAPHORE_REG = 0; break;
			case 0x04080000: SP_PC_REG = Value & 0xFFC; break;
			default:
				return FALSE;
			}
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x04100000: 
			DPC_START_REG = Value; 
			DPC_CURRENT_REG = Value; 
			break;
		case 0x04100004: 
			DPC_END_REG = Value; 
			if (ProcessRDPList) { ProcessRDPList(); }
			break;
		//case 0x04100008: DPC_CURRENT_REG = Value; break;
		case 0x0410000C:
			if ( ( Value & DPC_CLR_XBUS_DMEM_DMA ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA; }
			if ( ( Value & DPC_SET_XBUS_DMEM_DMA ) != 0) { DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_FREEZE; }
			if ( ( Value & DPC_SET_FREEZE ) != 0) { DPC_STATUS_REG |= DPC_STATUS_FREEZE;  }		
			if ( ( Value & DPC_CLR_FLUSH ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_FLUSH; }
			if ( ( Value & DPC_SET_FLUSH ) != 0) { DPC_STATUS_REG |= DPC_STATUS_FLUSH;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) 
			{
				if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0) 
				{
					if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) 
					{
						RunRsp();
					}
				}
			}
			if (ShowUnhandledMemory) {
				//if ( ( Value & DPC_CLR_TMEM_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_TMEM_CTR"); }
				//if ( ( Value & DPC_CLR_PIPE_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR"); }
				//if ( ( Value & DPC_CLR_CMD_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR"); }
				//if ( ( Value & DPC_CLR_CLOCK_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR"); }
			}
			break;
		default:
			return FALSE;
		}
		break;
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			MI_MODE_REG &= ~0x7F;
			MI_MODE_REG |= (Value & 0x7F);
			if ( ( Value & MI_CLR_INIT ) != 0 ) { MI_MODE_REG &= ~MI_MODE_INIT; }
			if ( ( Value & MI_SET_INIT ) != 0 ) { MI_MODE_REG |= MI_MODE_INIT; }
			if ( ( Value & MI_CLR_EBUS ) != 0 ) { MI_MODE_REG &= ~MI_MODE_EBUS; }
			if ( ( Value & MI_SET_EBUS ) != 0 ) { MI_MODE_REG |= MI_MODE_EBUS; }
			if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
				MI_INTR_REG &= ~MI_INTR_DP; 
				CheckInterrupts();
			}
			if ( ( Value & MI_CLR_RDRAM ) != 0 ) { MI_MODE_REG &= ~MI_MODE_RDRAM; }
			if ( ( Value & MI_SET_RDRAM ) != 0 ) { MI_MODE_REG |= MI_MODE_RDRAM; }
			break;
		case 0x0430000C: 
			if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
			if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
			break;
		default:
			return FALSE;
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (VI_STATUS_REG != Value) { 
				VI_STATUS_REG = Value; 
				if (ViStatusChanged != NULL ) { ViStatusChanged(); }
			}
			break;
		case 0x04400004: 
#ifdef CFB_READ
			if (VI_ORIGIN_REG > 0x280) {
				SetFrameBuffer(VI_ORIGIN_REG, (DWORD)(VI_WIDTH_REG * (VI_WIDTH_REG *.75)));
			}
#endif
			VI_ORIGIN_REG = (Value & 0xFFFFFF); 
			//if (UpdateScreen != NULL ) { UpdateScreen(); }
			break;
		case 0x04400008: 
			if (VI_WIDTH_REG != Value) {
				VI_WIDTH_REG = Value; 
				if (ViWidthChanged != NULL ) { ViWidthChanged(); }
			}
			break;
		case 0x0440000C: VI_INTR_REG = Value; break;
		case 0x04400010: 
			MI_INTR_REG &= ~MI_INTR_VI;
			CheckInterrupts();
			break;
		case 0x04400014: VI_BURST_REG = Value; break;
		case 0x04400018: VI_V_SYNC_REG = Value; break;
		case 0x0440001C: VI_H_SYNC_REG = Value; break;
		case 0x04400020: VI_LEAP_REG = Value; break;
		case 0x04400024: VI_H_START_REG = Value; break;
		case 0x04400028: VI_V_START_REG = Value; break;
		case 0x0440002C: VI_V_BURST_REG = Value; break;
		case 0x04400030: VI_X_SCALE_REG = Value; break;
		case 0x04400034: VI_Y_SCALE_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04500000: 
		switch (PAddr) {
		case 0x04500000: AI_DRAM_ADDR_REG = Value; break;
		case 0x04500004: 
			AI_LEN_REG = Value; 
			if (g_FixedAudio)
			{
				CAudio::AiSetLength(g_Audio,Value);
			}
			if (AiLenChanged != NULL) { AiLenChanged(); }				
			break;
		case 0x04500008: AI_CONTROL_REG = (Value & 1); break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			MI_INTR_REG &= ~MI_INTR_AI;
			AudioIntrReg &= ~MI_INTR_AI;
			CheckInterrupts();
			break;
		case 0x04500010: 
			AI_DACRATE_REG = Value;  
			DacrateChanged(g_SystemType);
			if (g_FixedAudio)
			{
				g_Audio->AiSetFrequency(Value,g_SystemType);
			}
			break;
		case 0x04500014:  AI_BITRATE_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04600000: 
		switch (PAddr) {
		case 0x04600000: PI_DRAM_ADDR_REG = Value; break;
		case 0x04600004: PI_CART_ADDR_REG = Value; break;
		case 0x04600008: 
			PI_RD_LEN_REG = Value; 
			PI_DMA_READ();
			break;
		case 0x0460000C: 
			PI_WR_LEN_REG = Value; 
			PI_DMA_WRITE();
			break;
		case 0x04600010:
			//if ((Value & PI_SET_RESET) != 0 ) { DisplayError("reset Controller"); }
			if ((Value & PI_CLR_INTR) != 0 ) {
				MI_INTR_REG &= ~MI_INTR_PI;
				CheckInterrupts();
			}
			break;
		case 0x04600014: PI_DOMAIN1_REG = (Value & 0xFF); break; 
		case 0x04600018: PI_BSD_DOM1_PWD_REG = (Value & 0xFF); break; 
		case 0x0460001C: PI_BSD_DOM1_PGS_REG = (Value & 0xFF); break; 
		case 0x04600020: PI_BSD_DOM1_RLS_REG = (Value & 0xFF); break; 
		default:
			return FALSE;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: RI_MODE_REG = Value; break;
		case 0x04700004: RI_CONFIG_REG = Value; break;
		case 0x04700008: RI_CURRENT_LOAD_REG = Value; break;
		case 0x0470000C: RI_SELECT_REG = Value; break;
		case 0x04700010: RI_REFRESH_REG = Value; break;
		case 0x04700014: RI_LATENCY_REG = Value; break;
		case 0x04700018: RI_RERROR_REG = Value; break;
		case 0x0470001C: RI_WERROR_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: SI_DRAM_ADDR_REG = Value; break;
		case 0x04800004: 
			SI_PIF_ADDR_RD64B_REG = Value; 
			SI_DMA_READ ();
			break;
		case 0x04800010: 
			SI_PIF_ADDR_WR64B_REG = Value; 
			SI_DMA_WRITE();
			break;
		case 0x04800018: 
			MI_INTR_REG &= ~MI_INTR_SI; 
			SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
			CheckInterrupts();
			break;
		default:
			return FALSE;
		}
		break;
	case 0x08000000:
		if (PAddr != 0x08010000) { return FALSE; }
		if (SaveUsing == SaveChip_Auto) { SaveUsing = SaveChip_FlashRam; }
		if (SaveUsing != SaveChip_FlashRam) { return TRUE; }
		WriteToFlashCommand(Value);
		break;
	case 0x1FC00000:
		if (PAddr < 0x1FC007C0) {
			return FALSE;
		} else if (PAddr < 0x1FC00800) {
			_asm {
				mov eax,Value
				bswap eax
				mov Value,eax
			}
			*(DWORD *)(&PIF_Ram[PAddr - 0x1FC007C0]) = Value;
			if (PAddr == 0x1FC007FC) {
				PifRamWrite();
			}
			return TRUE;
		}
		return FALSE;
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

BOOL r4300i_SW_VAddr ( DWORD VAddr, DWORD Value ) {
	if (TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(DWORD *)(TLB_WriteMap[VAddr >> 12] + VAddr) = Value;
	return TRUE;
}

void ResetMemoryStack (CBlockSection * Section) {
	int x86reg, TempReg;

	CPU_Message("    ResetMemoryStack");
	x86reg = Map_MemoryStack(Section, x86_Any, false);
	if (x86reg >= 0) { UnMap_X86reg(Section,x86reg); }

	x86reg = Map_TempReg(Section,x86_Any, 29, FALSE);
	if (UseTlb) {	
	    TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(x86reg,TempReg);
		ShiftRightUnsignImmed(TempReg,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg,TempReg,4);
		AddX86RegToX86Reg(x86reg,TempReg);
	} else {
		AndConstToX86Reg(x86reg,0x1FFFFFFF);
		AddConstToX86Reg(x86reg,(DWORD)N64MEM);
	}
	MoveX86regToVariable(x86reg, g_MemoryStack, "MemoryStack");
}
