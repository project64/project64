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
#include <Windows.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "x86.h"
#include "debugger.h"
#include "Recompiler Ops.h"
#include "../System Globals.h"

WORD FPU_RoundingMode = 0x0000;//_RC_NEAR
char Name[50];

void ChangeDefaultRoundingModel (void) {
	switch((_FPCR[31] & 3)) {
	case 0: FPU_RoundingMode = 0x0000; break; //_RC_NEAR
	case 1: FPU_RoundingMode = 0x0C00; break; //_RC_CHOP
	case 2: FPU_RoundingMode = 0x0800; break; //_RC_UP
	case 3: FPU_RoundingMode = 0x0400; break; //_RC_UP
	}
}

void CompileCop1Test (CCodeSection * Section) {
	if (Section->FpuBeenUsed()) { return; }
	TestVariable(STATUS_CU1,&_Reg->STATUS_REGISTER,"STATUS_REGISTER");
	_N64System->GetRecompiler()->CompileExit(Section,Section->m_CompilePC,Section->m_CompilePC,Section->RegWorking,CExitInfo::COP1_Unuseable,FALSE,JeLabel32);
	Section->FpuBeenUsed() = TRUE;
}

/********************** Load/store functions ************************/
#ifdef tofix
void Compile_R4300i_LWC1 (CCodeSection * Section) {
	DWORD TempReg1, TempReg2, TempReg3;
	char Name[50];

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	if ((m_CompileOpcode.ft & 1) != 0) {
		if (RegInStack(Section,m_CompileOpcode.ft-1,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.ft-1,CRegInfo::FPU_Qword)) {
			UnMap_FPR(Section,m_CompileOpcode.ft-1,TRUE);
		}
	}
	if (RegInStack(Section,m_CompileOpcode.ft,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.ft,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.ft,TRUE);
	} else {
		UnMap_FPR(Section,m_CompileOpcode.ft,FALSE);
	}
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->MipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;

		TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, TempReg1,Address);

		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg1,TempReg2);
		return;
	}
	if (Section->IsMapped(m_CompileOpcode.base) && m_CompileOpcode.offset == 0) { 
		if (UseTlb) {
			ProtectGPR(Section,m_CompileOpcode.base);
			TempReg1 = Section->MipsRegLo(m_CompileOpcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		}
	} else {
		if (Section->IsMapped(m_CompileOpcode.base)) { 
			ProtectGPR(Section,m_CompileOpcode.base);
			if (m_CompileOpcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,Section->MipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
			}
			UnProtectGPR(Section,m_CompileOpcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
			if (m_CompileOpcode.immediate == 0) { 
			} else if (m_CompileOpcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (m_CompileOpcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)m_CompileOpcode.immediate);
			}
		}
	}
	TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
	if (UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		
		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg3);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveN64MemToX86reg(TempReg3,TempReg1);
	}
	sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
	MoveVariableToX86reg(&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg2);
	MoveX86regToX86Pointer(TempReg3,TempReg2);
}

void Compile_R4300i_LDC1 (CCodeSection * Section) {
	DWORD TempReg1, TempReg2, TempReg3;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);

	UnMap_FPR(Section,m_CompileOpcode.ft,FALSE);
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->MipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;
		TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, TempReg1,Address);

		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg2);
		AddConstToX86Reg(TempReg2,4);
		MoveX86regToX86Pointer(TempReg1,TempReg2);

		_MMU->Compile_LW(Section,TempReg1,Address + 4);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg1,TempReg2);
		return;
	}
	if (Section->IsMapped(m_CompileOpcode.base) && m_CompileOpcode.offset == 0) { 
		if (UseTlb) {
			ProtectGPR(Section,m_CompileOpcode.base);
			TempReg1 = Section->MipsRegLo(m_CompileOpcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		}
	} else {
		if (Section->IsMapped(m_CompileOpcode.base)) { 
			ProtectGPR(Section,m_CompileOpcode.base);
			if (m_CompileOpcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,Section->MipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
			}
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
			if (m_CompileOpcode.immediate == 0) { 
			} else if (m_CompileOpcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (m_CompileOpcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)m_CompileOpcode.immediate);
			}
		}
	}

	TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
	if (UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg3);
		Push(TempReg2);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg2);
		AddConstToX86Reg(TempReg2,4);
		MoveX86regToX86Pointer(TempReg3,TempReg2);
		Pop(TempReg2);
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,TempReg3,4);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg3,TempReg2);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveN64MemToX86reg(TempReg3,TempReg1);

		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg2);
		AddConstToX86Reg(TempReg2,4);
		MoveX86regToX86Pointer(TempReg3,TempReg2);

		MoveN64MemDispToX86reg(TempReg3,TempReg1,4);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg3,TempReg2);
	}
}

void Compile_R4300i_SWC1 (CCodeSection * Section){
	DWORD TempReg1, TempReg2, TempReg3;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->MipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;
		
		UnMap_FPR(Section,m_CompileOpcode.ft,TRUE);
		TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);

		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg1);
		MoveX86PointerToX86reg(TempReg1,TempReg1);
		_MMU->Compile_SW_Register(Section,TempReg1, Address);
		return;
	}
	if (Section->IsMapped(m_CompileOpcode.base)) { 
		ProtectGPR(Section,m_CompileOpcode.base);
		if (m_CompileOpcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		if (m_CompileOpcode.immediate == 0) { 
		} else if (m_CompileOpcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (m_CompileOpcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)m_CompileOpcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		UnMap_FPR(Section,m_CompileOpcode.ft,TRUE);
		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg3);
		MoveX86PointerToX86reg(TempReg3,TempReg3);
		MoveX86regToX86regPointer(TempReg3,TempReg1, TempReg2);
	} else {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		UnMap_FPR(Section,m_CompileOpcode.ft,TRUE);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg2);
		MoveX86PointerToX86reg(TempReg2,TempReg2);
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveX86regToN64Mem(TempReg2, TempReg1);
	}
}

void Compile_R4300i_SDC1 (CCodeSection * Section){
	DWORD TempReg1, TempReg2, TempReg3;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->MipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;
		
		TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg1);
		AddConstToX86Reg(TempReg1,4);
		MoveX86PointerToX86reg(TempReg1,TempReg1);
		_MMU->Compile_SW_Register(Section,TempReg1, Address);

		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg1);
		MoveX86PointerToX86reg(TempReg1,TempReg1);
		_MMU->Compile_SW_Register(Section,TempReg1, Address + 4);		
		return;
	}
	if (Section->IsMapped(m_CompileOpcode.base)) { 
		ProtectGPR(Section,m_CompileOpcode.base);
		if (m_CompileOpcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		if (m_CompileOpcode.immediate == 0) { 
		} else if (m_CompileOpcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (m_CompileOpcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)m_CompileOpcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg3);
		AddConstToX86Reg(TempReg3,4);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToX86regPointer(TempReg3,TempReg1, TempReg2);
		AddConstToX86Reg(TempReg1,4);

		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg3);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToX86regPointer(TempReg3,TempReg1, TempReg2);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		TempReg3 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg3);
		AddConstToX86Reg(TempReg3,4);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToN64Mem(TempReg3, TempReg1);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg3);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToN64MemDisp(TempReg3, TempReg1,4);
	}

}

/************************** COP1 functions **************************/
void Compile_R4300i_COP1_MF (CCodeSection * Section) {
	DWORD TempReg;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);

	UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	Map_GPR_32bit(Section,m_CompileOpcode.rt, TRUE, -1);
	TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
	sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[m_CompileOpcode.fs],Name,TempReg);
	MoveX86PointerToX86reg(Section->MipsRegLo(m_CompileOpcode.rt),TempReg);		
}

void Compile_R4300i_COP1_DMF (CCodeSection * Section) {
	DWORD TempReg;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);

	UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	Map_GPR_64bit(Section,m_CompileOpcode.rt, -1);
	TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
	sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.fs],Name,TempReg);
	AddConstToX86Reg(TempReg,4);
	MoveX86PointerToX86reg(Section->MipsRegHi(m_CompileOpcode.rt),TempReg);		
	sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.fs],Name,TempReg);
	MoveX86PointerToX86reg(Section->MipsRegLo(m_CompileOpcode.rt),TempReg);		
}

void Compile_R4300i_COP1_CF(CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	
	if (m_CompileOpcode.fs != 31 && m_CompileOpcode.fs != 0) { Compile_R4300i_UnknownOpcode (Section); return; }
	Map_GPR_32bit(Section,m_CompileOpcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs],Section->MipsRegLo(m_CompileOpcode.rt));
}

void Compile_R4300i_COP1_MT( CCodeSection * Section) {	
	DWORD TempReg;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);
	
	if ((m_CompileOpcode.fs & 1) != 0) {
		if (RegInStack(Section,m_CompileOpcode.fs-1,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs-1,CRegInfo::FPU_Qword)) {
			UnMap_FPR(Section,m_CompileOpcode.fs-1,TRUE);
		}
	}
	UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
	sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[m_CompileOpcode.fs],Name,TempReg);

	if (Section->IsConst(m_CompileOpcode.rt)) {
		MoveConstToX86Pointer(Section->MipsRegLo(m_CompileOpcode.rt),TempReg);
	} else if (Section->IsMapped(m_CompileOpcode.rt)) {
		MoveX86regToX86Pointer(Section->MipsRegLo(m_CompileOpcode.rt),TempReg);
	} else {
		MoveX86regToX86Pointer(Map_TempReg(Section,x86_Any, m_CompileOpcode.rt, FALSE),TempReg);
	}
}

void Compile_R4300i_COP1_DMT( CCodeSection * Section) {
	DWORD TempReg;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);
	
	if ((m_CompileOpcode.fs & 1) == 0) {
		if (RegInStack(Section,m_CompileOpcode.fs+1,CRegInfo::FPU_Float) || RegInStack(Section,m_CompileOpcode.fs+1,CRegInfo::FPU_Dword)) {
			UnMap_FPR(Section,m_CompileOpcode.fs+1,TRUE);
		}
	}
	UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
	sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.fs],Name,TempReg);
		
	if (Section->IsConst(m_CompileOpcode.rt)) {
		MoveConstToX86Pointer(Section->MipsRegLo(m_CompileOpcode.rt),TempReg);
		AddConstToX86Reg(TempReg,4);
		if (Section->Is64Bit(m_CompileOpcode.rt)) {
			MoveConstToX86Pointer(Section->MipsRegHi(m_CompileOpcode.rt),TempReg);
		} else {
			MoveConstToX86Pointer(Section->MipsRegLo_S(m_CompileOpcode.rt) >> 31,TempReg);
		}
	} else if (Section->IsMapped(m_CompileOpcode.rt)) {
		MoveX86regToX86Pointer(Section->MipsRegLo(m_CompileOpcode.rt),TempReg);
		AddConstToX86Reg(TempReg,4);
		if (Section->Is64Bit(m_CompileOpcode.rt)) {
			MoveX86regToX86Pointer(Section->MipsRegHi(m_CompileOpcode.rt),TempReg);
		} else {
			MoveX86regToX86Pointer(Map_TempReg(Section,x86_Any, m_CompileOpcode.rt, TRUE),TempReg);
		}
	} else {
		int x86Reg = Map_TempReg(Section,x86_Any, m_CompileOpcode.rt, FALSE);
		MoveX86regToX86Pointer(x86Reg,TempReg);
		AddConstToX86Reg(TempReg,4);
		MoveX86regToX86Pointer(Map_TempReg(Section,x86Reg, m_CompileOpcode.rt, TRUE),TempReg);
	}
}


void Compile_R4300i_COP1_CT(CCodeSection * Section) {

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);
	if (m_CompileOpcode.fs != 31) { Compile_R4300i_UnknownOpcode (Section); return; }

	if (Section->IsConst(m_CompileOpcode.rt)) {
		MoveConstToVariable(Section->MipsRegLo(m_CompileOpcode.rt),&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs]);
	} else if (Section->IsMapped(m_CompileOpcode.rt)) {
		MoveX86regToVariable(Section->MipsRegLo(m_CompileOpcode.rt),&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs]);
	} else {
		MoveX86regToVariable(Map_TempReg(Section,x86_Any,m_CompileOpcode.rt,FALSE),&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs]);		
	}
	Pushad();
	Call_Direct(ChangeDefaultRoundingModel, "ChangeDefaultRoundingModel");
	Popad();
	Section->CurrentRoundingModel() = CRegInfo::RoundUnknown;
}

/************************** COP1: S functions ************************/
void Compile_R4300i_COP1_S_ADD (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);

	Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Float);
	if (RegInStack(Section,Reg2, CRegInfo::FPU_Float)) {
		fpuAddReg(StackPosition(Section,Reg2));
	} else {
		DWORD TempReg;

		UnMap_FPR(Section,Reg2,TRUE);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[Reg2],Name,TempReg);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Float);
		fpuAddDwordRegPointer(TempReg);
	}
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_SUB (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	DWORD TempReg;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);

	if (m_CompileOpcode.fd == m_CompileOpcode.ft) {
		UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);

		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg);
		fpuSubDwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Float);
		if (RegInStack(Section,Reg2, CRegInfo::FPU_Float)) {
			fpuSubReg(StackPosition(Section,Reg2));
		} else {
			UnMap_FPR(Section,Reg2,TRUE);
			Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Float);

			TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
			sprintf(Name,"_FPRFloatLocation[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[Reg2],Name,TempReg);
			fpuSubDwordRegPointer(TempReg);			
		}
	}
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_MUL (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	DWORD TempReg;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);

	Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Float);
	if (RegInStack(Section,Reg2, CRegInfo::FPU_Float)) {
		fpuMulReg(StackPosition(Section,Reg2));
	} else {
		UnMap_FPR(Section,Reg2,TRUE);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Float);

		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[Reg2],Name,TempReg);
		fpuMulDwordRegPointer(TempReg);			
	}
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_DIV (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	DWORD TempReg;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);

	if (m_CompileOpcode.fd == m_CompileOpcode.ft) {
		UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);

		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg);
		fpuDivDwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Float);
		if (RegInStack(Section,Reg2, CRegInfo::FPU_Float)) {
			fpuDivReg(StackPosition(Section,Reg2));
		} else {
			UnMap_FPR(Section,Reg2,TRUE);
			Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Float);

			TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
			sprintf(Name,"_FPRFloatLocation[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[Reg2],Name,TempReg);
			fpuDivDwordRegPointer(TempReg);			
		}
	}

	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_ABS (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	fpuAbs();
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_NEG (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	fpuNeg();
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_SQRT (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	fpuSqrt();
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
}

void Compile_R4300i_COP1_S_MOV (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
}

void Compile_R4300i_COP1_S_TRUNC_L (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundTruncate);
}

void Compile_R4300i_COP1_S_CEIL_L (CCodeSection * Section) {			//added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundUp);
}

void Compile_R4300i_COP1_S_FLOOR_L (CCodeSection * Section) {			//added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundDown);
}

void Compile_R4300i_COP1_S_ROUND_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundNearest);
}

void Compile_R4300i_COP1_S_TRUNC_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundTruncate);
}

void Compile_R4300i_COP1_S_CEIL_W (CCodeSection * Section) {			// added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundUp);
}

void Compile_R4300i_COP1_S_FLOOR_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundDown);
}

void Compile_R4300i_COP1_S_CVT_D (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_S_CVT_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_S_CVT_L (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_S_CMP (CCodeSection * Section) {
	DWORD Reg1 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.fs:m_CompileOpcode.ft;
	int x86reg, cmp = 0;

	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	//if ((m_CompileOpcode.funct & 1) != 0) { Compile_R4300i_UnknownOpcode(Section); }
	if ((m_CompileOpcode.funct & 2) != 0) { cmp |= 0x4000; }
	if ((m_CompileOpcode.funct & 4) != 0) { cmp |= 0x0100; }
	
	Load_FPR_ToTop(Section,Reg1,Reg1, CRegInfo::FPU_Float);
	Map_TempReg(Section,x86_EAX, 0, FALSE);
	if (RegInStack(Section,Reg2, CRegInfo::FPU_Float)) {
		fpuComReg(StackPosition(Section,Reg2),FALSE);
	} else {
		DWORD TempReg;

		UnMap_FPR(Section,Reg2,TRUE);
		Load_FPR_ToTop(Section,Reg1,Reg1, CRegInfo::FPU_Float);

		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[Reg2],Name,TempReg);
		fpuComDwordRegPointer(TempReg,FALSE);
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	AndConstToVariable(~FPCSR_C, &FSTATUS_REGISTER, "FSTATUS_REGISTER");
#endif
	fpuStoreStatus();
	x86reg = Map_TempReg(Section,x86_Any8Bit, 0, FALSE);
	TestConstToX86Reg(cmp,x86_EAX);	
	Setnz(x86reg);
	
	if (cmp != 0) {
		TestConstToX86Reg(cmp,x86_EAX);	
		Setnz(x86reg);
		
		if ((m_CompileOpcode.funct & 1) != 0) {
			int x86reg2 = Map_TempReg(Section,x86_Any8Bit, 0, FALSE);
			AndConstToX86Reg(x86_EAX, 0x4300);
			CompConstToX86reg(x86_EAX, 0x4300);
			Setz(x86reg2);
	
			OrX86RegToX86Reg(x86reg, x86reg2);
		}
	} else if ((m_CompileOpcode.funct & 1) != 0) {
		AndConstToX86Reg(x86_EAX, 0x4300);
		CompConstToX86reg(x86_EAX, 0x4300);
		Setz(x86reg);
	}
	ShiftLeftSignImmed(x86reg, 23);
	OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", x86reg);
}

/************************** COP1: D functions ************************/
void Compile_R4300i_COP1_D_ADD (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	

	Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Double);
	if (RegInStack(Section,Reg2, CRegInfo::FPU_Double)) {
		fpuAddReg(StackPosition(Section,Reg2));
	} else {
		DWORD TempReg;

		UnMap_FPR(Section,Reg2,TRUE);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Double);
		fpuAddQwordRegPointer(TempReg);	
	}
}

void Compile_R4300i_COP1_D_SUB (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	DWORD TempReg;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	

	if (m_CompileOpcode.fd == m_CompileOpcode.ft) {
		UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
		fpuSubQwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Double);
		if (RegInStack(Section,Reg2, CRegInfo::FPU_Double)) {
			fpuSubReg(StackPosition(Section,Reg2));
		} else {
			UnMap_FPR(Section,Reg2,TRUE);

			TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
			sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
			Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Double);
			fpuSubQwordRegPointer(TempReg);
		}
	}
}

void Compile_R4300i_COP1_D_MUL (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	DWORD TempReg;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);

	Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Double);
	if (RegInStack(Section,Reg2, CRegInfo::FPU_Double)) {
		fpuMulReg(StackPosition(Section,Reg2));
	} else {
		UnMap_FPR(Section,Reg2,TRUE);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Double);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
		fpuMulQwordRegPointer(TempReg);
	}
}

void Compile_R4300i_COP1_D_DIV (CCodeSection * Section) {
	DWORD Reg1 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = m_CompileOpcode.ft == m_CompileOpcode.fd?m_CompileOpcode.fs:m_CompileOpcode.ft;
	DWORD TempReg;
	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	

	if (m_CompileOpcode.fd == m_CompileOpcode.ft) {
		UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_CompileOpcode.ft],Name,TempReg);
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
		fpuDivQwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,Reg1, CRegInfo::FPU_Double);
		if (RegInStack(Section,Reg2, CRegInfo::FPU_Double)) {
			fpuDivReg(StackPosition(Section,Reg2));
		} else {
			UnMap_FPR(Section,Reg2,TRUE);
			TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
			sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
			Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fd, CRegInfo::FPU_Double);
			fpuDivQwordRegPointer(TempReg);
		}
	}
}

void Compile_R4300i_COP1_D_ABS (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
	fpuAbs();
}

void Compile_R4300i_COP1_D_NEG (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
	fpuNeg();
}

void Compile_R4300i_COP1_D_SQRT (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
	fpuSqrt();
}

void Compile_R4300i_COP1_D_MOV (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
}

void Compile_R4300i_COP1_D_TRUNC_L (CCodeSection * Section) {			//added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundTruncate);
}

void Compile_R4300i_COP1_D_CEIL_L (CCodeSection * Section) {			//added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundUp);
}

void Compile_R4300i_COP1_D_FLOOR_L (CCodeSection * Section) {			//added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundDown);
}

void Compile_R4300i_COP1_D_ROUND_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundNearest);
}

void Compile_R4300i_COP1_D_TRUNC_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundTruncate);
}

void Compile_R4300i_COP1_D_CEIL_W (CCodeSection * Section) {				// added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundUp);
}

void Compile_R4300i_COP1_D_FLOOR_W (CCodeSection * Section) {			//added by Witten
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundDown);
}

void Compile_R4300i_COP1_D_CVT_S (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_D_CVT_W (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_D_CVT_L (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_D_CMP (CCodeSection * Section) {
	DWORD Reg1 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.fs:m_CompileOpcode.ft;
	int x86reg, cmp = 0;

	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	//if ((m_CompileOpcode.funct & 1) != 0) { Compile_R4300i_UnknownOpcode(Section); }
	if ((m_CompileOpcode.funct & 2) != 0) { cmp |= 0x4000; }
	if ((m_CompileOpcode.funct & 4) != 0) { cmp |= 0x0100; }
	
	Load_FPR_ToTop(Section,Reg1,Reg1, CRegInfo::FPU_Double);
	Map_TempReg(Section,x86_EAX, 0, FALSE);
	if (RegInStack(Section,Reg2, CRegInfo::FPU_Double)) {
		fpuComReg(StackPosition(Section,Reg2),FALSE);
	} else {
		DWORD TempReg;

		UnMap_FPR(Section,Reg2,TRUE);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
		Load_FPR_ToTop(Section,Reg1,Reg1, CRegInfo::FPU_Double);
		fpuComQwordRegPointer(TempReg,FALSE);
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	AndConstToVariable(~FPCSR_C, &FSTATUS_REGISTER, "FSTATUS_REGISTER");
#endif
	fpuStoreStatus();
	x86reg = Map_TempReg(Section,x86_Any8Bit, 0, FALSE);
	TestConstToX86Reg(cmp,x86_EAX);	
	Setnz(x86reg);
	if (cmp != 0) {
		TestConstToX86Reg(cmp,x86_EAX);	
		Setnz(x86reg);
		
		if ((m_CompileOpcode.funct & 1) != 0) {
			int x86reg2 = Map_TempReg(Section,x86_Any8Bit, 0, FALSE);
			AndConstToX86Reg(x86_EAX, 0x4300);
			CompConstToX86reg(x86_EAX, 0x4300);
			Setz(x86reg2);
	
			OrX86RegToX86Reg(x86reg, x86reg2);
		}
	} else if ((m_CompileOpcode.funct & 1) != 0) {
		AndConstToX86Reg(x86_EAX, 0x4300);
		CompConstToX86reg(x86_EAX, 0x4300);
		Setz(x86reg);
	}
	ShiftLeftSignImmed(x86reg, 23);
	OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", x86reg);
}

/************************** COP1: W functions ************************/
void Compile_R4300i_COP1_W_CVT_S (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Dword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_W_CVT_D (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Dword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
}

/************************** COP1: L functions ************************/
void Compile_R4300i_COP1_L_CVT_S (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Qword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
}

void Compile_R4300i_COP1_L_CVT_D (CCodeSection * Section) {
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Qword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
}

#endif