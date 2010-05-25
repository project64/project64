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
#include "c core.h"
#include "main.h"
#include "cpu.h"
#include "x86.h"
#include "debugger.h"

char *GPR_Name[32] = {"r0","at","v0","v1","a0","a1","a2","a3",
                     "t0","t1","t2","t3","t4","t5","t6","t7",
                     "s0","s1","s2","s3","s4","s5","s6","s7",
                     "t8","t9","k0","k1","gp","sp","s8","ra"};

char *GPR_NameHi[32] = {"r0.HI","at.HI","v0.HI","v1.HI","a0.HI","a1.HI",
						"a2.HI","a3.HI","t0.HI","t1.HI","t2.HI","t3.HI",
						"t4.HI","t5.HI","t6.HI","t7.HI","s0.HI","s1.HI",
						"s2.HI","s3.HI","s4.HI","s5.HI","s6.HI","s7.HI",
						"t8.HI","t9.HI","k0.HI","k1.HI","gp.HI","sp.HI",
						"s8.HI","ra.HI"};

char *GPR_NameLo[32] = {"r0.LO","at.LO","v0.LO","v1.LO","a0.LO","a1.LO",
						"a2.LO","a3.LO","t0.LO","t1.LO","t2.LO","t3.LO",
						"t4.LO","t5.LO","t6.LO","t7.LO","s0.LO","s1.LO",
						"s2.LO","s3.LO","s4.LO","s5.LO","s6.LO","s7.LO",
						"t8.LO","t9.LO","k0.LO","k1.LO","gp.LO","sp.LO",
						"s8.LO","ra.LO"};

char *FPR_Name[32] = {"f0","f1","f2","f3","f4","f5","f6","f7",
                     "f8","f9","f10","f11","f12","f13","f14","f15",
                     "f16","f17","f18","f19","f20","f21","f22","f23",
                     "f24","f25","f26","f27","f28","f29","f30","f31"};

char *FPR_NameHi[32] = {"f0.hi","f1.hi","f2.hi","f3.hi","f4.hi","f5.hi","f6.hi","f7.hi",
                     "f8.hi","f9.hi","f10.hi","f11.hi","f12.hi","f13.hi","f14.hi","f15.hi",
                     "f16.hi","f17.hi","f18.hi","f19.hi","f20.hi","f21.hi","f22.hi","f23.hi",
                     "f24.hi","f25.hi","f26.hi","f27.hi","f28.hi","f29.hi","f30.hi","f31.hi"};

char *FPR_NameLo[32] = {"f0.lo","f1.lo","f2.lo","f3.lo","f4.lo","f5.lo","f6.lo","f7.lo",
                     "f8.lo","f9.lo","f10.lo","f11.lo","f12.lo","f13.lo","f14.lo","f15.lo",
                     "f16.lo","f17.lo","f18.lo","f19.lo","f20.lo","f21.lo","f22.lo","f23.lo",
                     "f24.lo","f25.lo","f26.lo","f27.lo","f28.lo","f29.lo","f30.lo","f31.lo"};

char *FPR_Ctrl_Name[32] = {"Revision","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","FCSR"};

DWORD RegModValue;
//DWORD RegModValue, ViFieldNumber, LLBit, LLAddr;
//N64_REGISTERS Registers;
int fpuControl;


int  UnMap_8BitTempReg (CCodeSection * Section);
int  UnMap_TempReg     (CCodeSection * Section);
BOOL UnMap_X86reg      (CCodeSection * Section, DWORD x86Reg);

char *Format_Name[] = {"Unkown","dword","qword","float","double"};

void ChangeFPURegFormat (CCodeSection * Section, int Reg, CRegInfo::FPU_STATE OldFormat, CRegInfo::FPU_STATE NewFormat, CRegInfo::FPU_ROUND RoundingModel) {
	DWORD i;

	for (i = 0; i < 8; i++) {
		if (Section->FpuMappedTo(i) == (DWORD)Reg) {
			if (Section->FpuState(i) != OldFormat) {		
				UnMap_FPR(Section,Reg,TRUE);
				Load_FPR_ToTop(Section,Reg,Reg,OldFormat);
				ChangeFPURegFormat(Section,Reg,OldFormat,NewFormat,RoundingModel);
				return;
			}
			CPU_Message("    regcache: Changed format of ST(%d) from %s to %s", 
				(i - Section->StackTopPos() + 8) & 7,Format_Name[OldFormat],Format_Name[NewFormat]);			
			Section->FpuRoundingModel(i) = RoundingModel;
			Section->FpuState(i)         = NewFormat;
			return;
		}
	}

#ifndef EXTERNAL_RELEASE
	DisplayError("ChangeFormat: Register not on stack!!");
#endif
}

void ChangeMiIntrMask (void) {
	if ( ( RegModValue & MI_INTR_MASK_CLR_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
	if ( ( RegModValue & MI_INTR_MASK_SET_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
	if ( ( RegModValue & MI_INTR_MASK_SET_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
}

void ChangeMiModeReg (void) {
	_Reg->MI_MODE_REG &= ~0x7F;
	_Reg->MI_MODE_REG |= (RegModValue & 0x7F);
	if ( ( RegModValue & MI_CLR_INIT ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_INIT; }
	if ( ( RegModValue & MI_SET_INIT ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_INIT; }
	if ( ( RegModValue & MI_CLR_EBUS ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_EBUS; }
	if ( ( RegModValue & MI_SET_EBUS ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_EBUS; }
	if ( ( RegModValue & MI_CLR_DP_INTR ) != 0 ) { _Reg->MI_INTR_REG &= ~MI_INTR_DP; }
	if ( ( RegModValue & MI_CLR_RDRAM ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_RDRAM; }
	if ( ( RegModValue & MI_SET_RDRAM ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_RDRAM; }
}

void ChangeSpStatus (void) {
	if ( ( RegModValue & SP_CLR_HALT ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_HALT; }
	if ( ( RegModValue & SP_SET_HALT ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_HALT;  }
	if ( ( RegModValue & SP_CLR_BROKE ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_BROKE; }
	if ( ( RegModValue & SP_CLR_INTR ) != 0) { 
		_Reg->MI_INTR_REG &= ~MI_INTR_SP; 
		CheckInterrupts();
	}
#ifndef EXTERNAL_RELEASE
	if ( ( RegModValue & SP_SET_INTR ) != 0) { DisplayError("SP_SET_INTR"); }
#endif
	if ( ( RegModValue & SP_CLR_SSTEP ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
	if ( ( RegModValue & SP_SET_SSTEP ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SSTEP;  }
	if ( ( RegModValue & SP_CLR_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
	if ( ( RegModValue & SP_SET_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
	if ( ( RegModValue & SP_CLR_SIG0 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG0; }
	if ( ( RegModValue & SP_SET_SIG0 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG0;  }
	if ( ( RegModValue & SP_CLR_SIG1 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG1; }
	if ( ( RegModValue & SP_SET_SIG1 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG1;  }
	if ( ( RegModValue & SP_CLR_SIG2 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG2; }
	if ( ( RegModValue & SP_SET_SIG2 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG2;  }
	if ( ( RegModValue & SP_CLR_SIG3 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG3; }
	if ( ( RegModValue & SP_SET_SIG3 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG3;  }
	if ( ( RegModValue & SP_CLR_SIG4 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG4; }
	if ( ( RegModValue & SP_SET_SIG4 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG4;  }
	if ( ( RegModValue & SP_CLR_SIG5 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG5; }
	if ( ( RegModValue & SP_SET_SIG5 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG5;  }
	if ( ( RegModValue & SP_CLR_SIG6 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG6; }
	if ( ( RegModValue & SP_SET_SIG6 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG6;  }
	if ( ( RegModValue & SP_CLR_SIG7 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG7; }
	if ( ( RegModValue & SP_SET_SIG7 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG7;  }

	if ( ( RegModValue & SP_SET_SIG0 ) != 0 && AudioSignal)
	{
		_Reg->MI_INTR_REG |= MI_INTR_SP; 
		CheckInterrupts();				
	}
	//if (*( DWORD *)(DMEM + 0xFC0) == 1) {
	//	ChangeTimer(RspTimer,0x40000);
	//} else {
		RunRsp();
	//}
}

int Free8BitX86Reg (CCodeSection * Section) {
	int x86Reg, count, MapCount[10], MapReg[10];
	
	if (Section->x86Mapped(x86_EBX) == CRegInfo::NotMapped && !Section->x86Protected(x86_EBX)) {return x86_EBX; }
	if (Section->x86Mapped(x86_EAX) == CRegInfo::NotMapped && !Section->x86Protected(x86_EAX)) {return x86_EAX; }
	if (Section->x86Mapped(x86_EDX) == CRegInfo::NotMapped && !Section->x86Protected(x86_EDX)) {return x86_EDX; }
	if (Section->x86Mapped(x86_ECX) == CRegInfo::NotMapped && !Section->x86Protected(x86_ECX)) {return x86_ECX; }

	x86Reg = UnMap_8BitTempReg(Section);
	if (x86Reg > 0) { return x86Reg; }
	
	for (count = 0; count < 10; count ++) {
		MapCount[count] = Section->x86MapOrder(count);
		MapReg[count] = count;
	}
	for (count = 0; count < 10; count ++) {
		int i;
		
		for (i = 0; i < 9; i ++) {
			int temp;

			if (MapCount[i] < MapCount[i+1]) {
				temp = MapCount[i];
				MapCount[i] = MapCount[i+1];
				MapCount[i+1] = temp;
				temp = MapReg[i];
				MapReg[i] = MapReg[i+1];
				MapReg[i+1] = temp;
			}
		}

	}
	for (count = 0; count < 10; count ++) {
		if (MapCount[count] > 0) {			
			if (!Is8BitReg(count)) {  continue; }
			if (UnMap_X86reg(Section,count)) {
				return count;
			}
		}
	}

	return -1;
}

int FreeX86Reg (CCodeSection * Section) {
	int x86Reg, count, MapCount[10], MapReg[10], StackReg;

	if (Section->x86Mapped(x86_EDI) == CRegInfo::NotMapped && !Section->x86Protected(x86_EDI)) {return x86_EDI; }
	if (Section->x86Mapped(x86_ESI) == CRegInfo::NotMapped && !Section->x86Protected(x86_ESI)) {return x86_ESI; }
	if (Section->x86Mapped(x86_EBX) == CRegInfo::NotMapped && !Section->x86Protected(x86_EBX)) {return x86_EBX; }
	if (Section->x86Mapped(x86_EAX) == CRegInfo::NotMapped && !Section->x86Protected(x86_EAX)) {return x86_EAX; }
	if (Section->x86Mapped(x86_EDX) == CRegInfo::NotMapped && !Section->x86Protected(x86_EDX)) {return x86_EDX; }
	if (Section->x86Mapped(x86_ECX) == CRegInfo::NotMapped && !Section->x86Protected(x86_ECX)) {return x86_ECX; }

	x86Reg = UnMap_TempReg(Section);
	if (x86Reg > 0) { return x86Reg; }

	for (count = 0; count < 10; count ++) {
		MapCount[count] = Section->x86MapOrder(count);
		MapReg[count] = count;
	}
	for (count = 0; count < 10; count ++) {
		int i;
		
		for (i = 0; i < 9; i ++) {
			int temp;

			if (MapCount[i] < MapCount[i+1]) {
				temp = MapCount[i];
				MapCount[i] = MapCount[i+1];
				MapCount[i+1] = temp;
				temp = MapReg[i];
				MapReg[i] = MapReg[i+1];
				MapReg[i+1] = temp;
			}
		}

	}
	StackReg = -1;
	for (count = 0; count < 10; count ++) {
		if (MapCount[count] > 0 && Section->x86Mapped(MapReg[count]) != CRegInfo::Stack_Mapped) {
			if (UnMap_X86reg(Section,MapReg[count])) {
				return MapReg[count];
			}			
		}
		if (Section->x86Mapped(MapReg[count]) == CRegInfo::Stack_Mapped) { StackReg = MapReg[count]; }
	}
	if (StackReg > 0) {
		UnMap_X86reg(Section,StackReg);
		return StackReg;
	}	
	return -1;
}

#ifdef toremove
void InitalizeR4300iRegisters (int UsePif, int Country, int CIC_Chip) {
	memset(CP0,0,sizeof(Registers.CP0));	
	memset(FPCR,0,sizeof(Registers.FPCR));	
	memset(RegRDRAM,0,sizeof(Registers.RDRAM));	
	memset(RegSP,0,sizeof(Registers.SP));	
	memset(RegDPC,0,sizeof(Registers.DPC));	
	memset(RegMI,0,sizeof(Registers.MI));	
	memset(RegVI,0,sizeof(Registers.VI));	
	memset(RegAI,0,sizeof(Registers.AI));	
	memset(RegPI,0,sizeof(Registers.PI));	
	memset(RegRI,0,sizeof(Registers.RI));	
	memset(RegSI,0,sizeof(Registers.SI));	
	memset(_GPR,0,sizeof(Registers._GPR));	
	memset(FPR,0,sizeof(Registers.FPR));	
	
	if (CIC_Chip < 0) {
		DisplayError(GS(MSG_UNKNOWN_CIC_CHIP));
		CIC_Chip = 2;
	}
	LO.DW                 = 0x0;
	HI.DW                 = 0x0;
	RANDOM_REGISTER	  = 0x1F;
	COUNT_REGISTER	  = 0x5000;
	MI_VERSION_REG	  = 0x02020102;
	SP_STATUS_REG      = 0x00000001;
	CAUSE_REGISTER	  = 0x0000005C;
	//ENTRYHI_REGISTER	  = 0xFFFFE0FF;
	CONTEXT_REGISTER   = 0x007FFFF0;
	EPC_REGISTER       = 0xFFFFFFFF;
	BAD_VADDR_REGISTER = 0xFFFFFFFF;
	ERROREPC_REGISTER  = 0xFFFFFFFF;
	CONFIG_REGISTER     = 0x0006E463;
	REVISION_REGISTER   = 0x00000511;
	STATUS_REGISTER     = 0x34000000;
	SetFpuLocations();
	if (UsePif) {
		PROGRAM_COUNTER = 0xBFC00000;			
		switch (CIC_Chip) {
		case 1:
			PIF_Ram[36] = 0x00;
			PIF_Ram[37] = 0x06;
			PIF_Ram[38] = 0x3F;
			PIF_Ram[39] = 0x3F;
			break;
		case 2:
			PIF_Ram[36] = 0x00;
			PIF_Ram[37] = 0x02;
			PIF_Ram[38] = 0x3F;
			PIF_Ram[39] = 0x3F;
			break;
		case 3:			
			PIF_Ram[36] = 0x00;
			PIF_Ram[37] = 0x02;
			PIF_Ram[38] = 0x78;
			PIF_Ram[39] = 0x3F;
			break;
		case 5:			
			PIF_Ram[36] = 0x00;
			PIF_Ram[37] = 0x02;
			PIF_Ram[38] = 0x91;
			PIF_Ram[39] = 0x3F;
			break;
		case 6:			
			PIF_Ram[36] = 0x00;
			PIF_Ram[37] = 0x02;
			PIF_Ram[38] = 0x85;
			PIF_Ram[39] = 0x3F;
			break;
		}
	} else {
		memcpy( (N64MEM+0x4000040), (ROM + 0x040), 0xFBC);
		PROGRAM_COUNTER	  = 0xA4000040;	
		
		_GPR[0].DW=0x0000000000000000;
		_GPR[6].DW=0xFFFFFFFFA4001F0C;
		_GPR[7].DW=0xFFFFFFFFA4001F08;
		_GPR[8].DW=0x00000000000000C0;
		_GPR[9].DW=0x0000000000000000;
		_GPR[10].DW=0x0000000000000040;
		_GPR[11].DW=0xFFFFFFFFA4000040;
		_GPR[16].DW=0x0000000000000000;
		_GPR[17].DW=0x0000000000000000;
		_GPR[18].DW=0x0000000000000000;
		_GPR[19].DW=0x0000000000000000;
		_GPR[21].DW=0x0000000000000000; 
		_GPR[26].DW=0x0000000000000000;
		_GPR[27].DW=0x0000000000000000;
		_GPR[28].DW=0x0000000000000000;
		_GPR[29].DW=0xFFFFFFFFA4001FF0;
		_GPR[30].DW=0x0000000000000000;
		
		switch (Country) {
		case 0x44: //Germany
		case 0x46: //french
		case 0x49: //Italian
		case 0x50: //Europe
		case 0x53: //Spanish
		case 0x55: //Australia
		case 0x58: // ????
		case 0x59: // X (PAL)
			switch (CIC_Chip) {
			case 2:
				_GPR[5].DW=0xFFFFFFFFC0F1D859;
				_GPR[14].DW=0x000000002DE108EA;
				_GPR[24].DW=0x0000000000000000;
				break;
			case 3:
				_GPR[5].DW=0xFFFFFFFFD4646273;
				_GPR[14].DW=0x000000001AF99984;
				_GPR[24].DW=0x0000000000000000;
				break;
			case 5:
				*(DWORD *)&IMEM[0x04] = 0xBDA807FC;
				_GPR[5].DW=0xFFFFFFFFDECAAAD1;
				_GPR[14].DW=0x000000000CF85C13;
				_GPR[24].DW=0x0000000000000002;
				break;
			case 6:
				_GPR[5].DW=0xFFFFFFFFB04DC903;
				_GPR[14].DW=0x000000001AF99984;
				_GPR[24].DW=0x0000000000000002;
				break;
			}

			_GPR[20].DW=0x0000000000000000;
			_GPR[23].DW=0x0000000000000006;
			_GPR[31].DW=0xFFFFFFFFA4001554;
			break;
		case 0x37: // 7 (Beta)
		case 0x41: // ????
		case 0x45: //USA
		case 0x4A: //Japan
		default:
			switch (CIC_Chip) {
			case 2:
				_GPR[5].DW=0xFFFFFFFFC95973D5;
				_GPR[14].DW=0x000000002449A366;
				break;
			case 3:
				_GPR[5].DW=0xFFFFFFFF95315A28;
				_GPR[14].DW=0x000000005BACA1DF;
				break;
			case 5:
				*(DWORD *)&IMEM[0x04] = 0x8DA807FC;
				_GPR[5].DW=0x000000005493FB9A;
				_GPR[14].DW=0xFFFFFFFFC2C20384;
			case 6:
				_GPR[5].DW=0xFFFFFFFFE067221F;
				_GPR[14].DW=0x000000005CD2B70F;
				break;
			}
			_GPR[20].DW=0x0000000000000001;
			_GPR[23].DW=0x0000000000000000;
			_GPR[24].DW=0x0000000000000003;
			_GPR[31].DW=0xFFFFFFFFA4001550;
		}

		switch (CIC_Chip) {
		case 1: 
			_GPR[22].DW=0x000000000000003F; 
			break;
		case 2: 
			_GPR[1].DW=0x0000000000000001;
			_GPR[2].DW=0x000000000EBDA536;
			_GPR[3].DW=0x000000000EBDA536;
			_GPR[4].DW=0x000000000000A536;
			_GPR[12].DW=0xFFFFFFFFED10D0B3;
			_GPR[13].DW=0x000000001402A4CC;
			_GPR[15].DW=0x000000003103E121;
			_GPR[22].DW=0x000000000000003F; 
			_GPR[25].DW=0xFFFFFFFF9DEBB54F;
			break;
		case 3: 
			_GPR[1].DW=0x0000000000000001;
			_GPR[2].DW=0x0000000049A5EE96;
			_GPR[3].DW=0x0000000049A5EE96;
			_GPR[4].DW=0x000000000000EE96;
			_GPR[12].DW=0xFFFFFFFFCE9DFBF7;
			_GPR[13].DW=0xFFFFFFFFCE9DFBF7;
			_GPR[15].DW=0x0000000018B63D28;
			_GPR[22].DW=0x0000000000000078; 
			_GPR[25].DW=0xFFFFFFFF825B21C9;
			break;
		case 5: 
			*(DWORD *)&IMEM[0x00] = 0x3C0DBFC0;
			*(DWORD *)&IMEM[0x08] = 0x25AD07C0;
			*(DWORD *)&IMEM[0x0C] = 0x31080080;
			*(DWORD *)&IMEM[0x10] = 0x5500FFFC;
			*(DWORD *)&IMEM[0x14] = 0x3C0DBFC0;
			*(DWORD *)&IMEM[0x18] = 0x8DA80024;
			*(DWORD *)&IMEM[0x1C] = 0x3C0BB000;
			_GPR[1].DW=0x0000000000000000;
			_GPR[2].DW=0xFFFFFFFFF58B0FBF;
			_GPR[3].DW=0xFFFFFFFFF58B0FBF;
			_GPR[4].DW=0x0000000000000FBF;
			_GPR[12].DW=0xFFFFFFFF9651F81E;
			_GPR[13].DW=0x000000002D42AAC5;
			_GPR[15].DW=0x0000000056584D60;
			_GPR[22].DW=0x0000000000000091; 
			_GPR[25].DW=0xFFFFFFFFCDCE565F;
			break;
		case 6: 
			_GPR[1].DW=0x0000000000000000;
			_GPR[2].DW=0xFFFFFFFFA95930A4;
			_GPR[3].DW=0xFFFFFFFFA95930A4;
			_GPR[4].DW=0x00000000000030A4;
			_GPR[12].DW=0xFFFFFFFFBCB59510;
			_GPR[13].DW=0xFFFFFFFFBCB59510;
			_GPR[15].DW=0x000000007A3C07F4;
			_GPR[22].DW=0x0000000000000085; 
			_GPR[25].DW=0x00000000465E3F72;
			break;
		}
	}
#ifdef Interpreter_StackTest
	StackValue = _GPR[29].W[0];
#endif
	MemoryStack = (DWORD)(N64MEM+(_GPR[29].W[0] & 0x1FFFFFFF));
}
#endif

BOOL Is8BitReg (int x86Reg) {
	if (x86Reg == x86_EAX) { return TRUE; }
	if (x86Reg == x86_EBX) { return TRUE; }
	if (x86Reg == x86_ECX) { return TRUE; }
	if (x86Reg == x86_EDX) { return TRUE; }
	return FALSE;
}

void Load_FPR_ToTop (CCodeSection * Section, int Reg, int RegToLoad, CRegInfo::FPU_STATE Format) {
	int i;

	if (RegToLoad < 0) { DisplayError("Load_FPR_ToTop\nRegToLoad < 0 ???"); return; }
	if (Reg < 0) { DisplayError("Load_FPR_ToTop\nReg < 0 ???"); return; }

	if (Format == CRegInfo::FPU_Double || Format == CRegInfo::FPU_Qword) {
		UnMap_FPR(Section,Reg + 1,TRUE);
		UnMap_FPR(Section,RegToLoad + 1,TRUE);
	} else {
		if ((Reg & 1) != 0) {
			for (i = 0; i < 8; i++) {
				if (Section->FpuMappedTo(i) == (DWORD)(Reg - 1)) {
					if (Section->FpuState(i) == CRegInfo::FPU_Double || Section->FpuState(i) == CRegInfo::FPU_Qword) {
						UnMap_FPR(Section,Reg,TRUE);
					}
					i = 8;
				}
			}		
		}
		if ((RegToLoad & 1) != 0) {
			for (i = 0; i < 8; i++) {
				if (Section->FpuMappedTo(i) == (DWORD)(RegToLoad - 1)) {
					if (Section->FpuState(i) == CRegInfo::FPU_Double || Section->FpuState(i) == CRegInfo::FPU_Qword) {
						UnMap_FPR(Section,RegToLoad,TRUE);
					}
					i = 8;
				}
			}		
		}
	}

	if (Reg == RegToLoad) {
		//if different format then unmap original reg from stack
		for (i = 0; i < 8; i++) {
			if (Section->FpuMappedTo(i) == (DWORD)Reg) {
				if (Section->FpuState(i) != (DWORD)Format) {
					UnMap_FPR(Section,Reg,TRUE);
				}
				i = 8;
			}
		}
	} else {
		UnMap_FPR(Section,Reg,FALSE);
	}

	if (RegInStack(Section,RegToLoad,Format)) {
		if (Reg != RegToLoad) {
			if (Section->FpuMappedTo((Section->StackTopPos() - 1) & 7) != (DWORD)RegToLoad) {
				UnMap_FPR(Section,Section->FpuMappedTo((Section->StackTopPos() - 1) & 7),TRUE);
				CPU_Message("    regcache: allocate ST(0) to %s", FPR_Name[Reg]);
				fpuLoadReg(&Section->StackTopPos(),StackPosition(Section,RegToLoad));		
				Section->FpuRoundingModel(Section->StackTopPos()) = CRegInfo::RoundDefault;
				Section->FpuMappedTo(Section->StackTopPos())      = Reg;
				Section->FpuState(Section->StackTopPos())         = Format;
			} else {
				UnMap_FPR(Section,Section->FpuMappedTo((Section->StackTopPos() - 1) & 7),TRUE);
				Load_FPR_ToTop (Section,Reg, RegToLoad, Format);
			}
		} else {
			DWORD RegPos, StackPos, i;

			for (i = 0; i < 8; i++) {
				if (Section->FpuMappedTo(i) == (DWORD)Reg) {
					RegPos = i;
					i = 8;
				}
			}

			if (RegPos == Section->StackTopPos()) {
				return;
			}
			StackPos = StackPosition(Section,Reg);

			Section->FpuRoundingModel(RegPos) = Section->FpuRoundingModel(Section->StackTopPos());
			Section->FpuMappedTo(RegPos)      = Section->FpuMappedTo(Section->StackTopPos());
			Section->FpuState(RegPos)         = Section->FpuState(Section->StackTopPos());
			CPU_Message("    regcache: allocate ST(%d) to %s", StackPos,FPR_Name[Section->FpuMappedTo(RegPos)]);
			CPU_Message("    regcache: allocate ST(0) to %s", FPR_Name[Reg]);

			fpuExchange(StackPos);

			Section->FpuRoundingModel(Section->StackTopPos()) = CRegInfo::RoundDefault;
			Section->FpuMappedTo(Section->StackTopPos())      = Reg;
			Section->FpuState(Section->StackTopPos())         = Format;
		}
	} else {
		char Name[50];
		int TempReg;

		UnMap_FPR(Section,Section->FpuMappedTo((Section->StackTopPos() - 1) & 7),TRUE);
		for (i = 0; i < 8; i++) {
			if (Section->FpuMappedTo(i) == (DWORD)RegToLoad) {
				UnMap_FPR(Section,RegToLoad,TRUE);
				i = 8;
			}
		}
		CPU_Message("    regcache: allocate ST(0) to %s", FPR_Name[Reg]);
		TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		switch (Format) {
		case CRegInfo::FPU_Dword:
			sprintf(Name,"_FPRFloatLocation[%d]",RegToLoad);
			MoveVariableToX86reg(&_FPRFloatLocation[RegToLoad],Name,TempReg);
			fpuLoadIntegerDwordFromX86Reg(&Section->StackTopPos(),TempReg);
			break;
		case CRegInfo::FPU_Qword:
			sprintf(Name,"_FPRDoubleLocation[%d]",RegToLoad);
			MoveVariableToX86reg(&_FPRDoubleLocation[RegToLoad],Name,TempReg);
			fpuLoadIntegerQwordFromX86Reg(&Section->StackTopPos(),TempReg);
			break;
		case CRegInfo::FPU_Float:
			sprintf(Name,"_FPRFloatLocation[%d]",RegToLoad);
			MoveVariableToX86reg(&_FPRFloatLocation[RegToLoad],Name,TempReg);
			fpuLoadDwordFromX86Reg(&Section->StackTopPos(),TempReg);
			break;
		case CRegInfo::FPU_Double:
			sprintf(Name,"_FPRDoubleLocation[%d]",RegToLoad);
			MoveVariableToX86reg(&_FPRDoubleLocation[RegToLoad],Name,TempReg);
			fpuLoadQwordFromX86Reg(&Section->StackTopPos(),TempReg);
			break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Load_FPR_ToTop\nUnkown format to load %d",Format);
#endif
		}
		Section->x86Protected(TempReg) = FALSE;
		Section->FpuRoundingModel(Section->StackTopPos()) = CRegInfo::RoundDefault;
		Section->FpuMappedTo(Section->StackTopPos())      = Reg;
		Section->FpuState(Section->StackTopPos())         = Format;
	}
	CPU_Message("CurrentRoundingModel: %d  FpuRoundingModel(StackTopPos()): %d",
		Section->CurrentRoundingModel(),Section->FpuRoundingModel(Section->StackTopPos()));
}

void Map_GPR_32bit (CCodeSection * Section, int Reg, BOOL SignValue, int MipsRegToLoad) {
	int x86Reg,count;

	if (Reg == 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Map_GPR_32bit\n\nWhy are you trying to map reg 0");
#endif
		return;
	}

	if (Section->IsUnknown(Reg) || Section->IsConst(Reg)) {		
		x86Reg = FreeX86Reg(Section);		
		if (x86Reg < 0) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("Map_GPR_32bit\n\nOut of registers"); 
			BreakPoint(__FILE__,__LINE__); 
#endif
			return; 
		}		
		CPU_Message("    regcache: allocate %s to %s",x86_Name(x86Reg),GPR_Name[Reg]);
	} else {
		if (Section->Is64Bit(Reg)) { 
			CPU_Message("    regcache: unallocate %s from high 32bit of %s",x86_Name(Section->MipsRegHi(Reg)),GPR_NameHi[Reg]);
			Section->x86MapOrder(Section->MipsRegHi(Reg)) = 0;
			Section->x86Mapped(Section->MipsRegHi(Reg)) = CRegInfo::NotMapped;
			Section->x86Protected(Section->MipsRegHi(Reg)) = FALSE;
			Section->MipsRegHi(Reg) = 0;
		}
		x86Reg = Section->MipsRegLo(Reg);
	}
	for (count = 0; count < 10; count ++) {
		if (Section->x86MapOrder(count) > 0) { 
			Section->x86MapOrder(count) += 1;
		}
	}
	Section->x86MapOrder(x86Reg) = 1;
	
	if (MipsRegToLoad > 0) {
		if (Section->IsUnknown(MipsRegToLoad)) {
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0],GPR_NameLo[MipsRegToLoad],x86Reg);
		} else if (Section->IsMapped(MipsRegToLoad)) {
			if (Reg != MipsRegToLoad) {
				MoveX86RegToX86Reg(Section->MipsRegLo(MipsRegToLoad),x86Reg);
			}
		} else {
			if (Section->MipsRegLo(MipsRegToLoad) != 0) {
				MoveConstToX86reg(Section->MipsRegLo(MipsRegToLoad),x86Reg);
			} else {
				XorX86RegToX86Reg(x86Reg,x86Reg);
			}
		}
	} else if (MipsRegToLoad == 0) {
		XorX86RegToX86Reg(x86Reg,x86Reg);
	}
	Section->x86Mapped(x86Reg) = CRegInfo::GPR_Mapped;
	Section->x86Protected(x86Reg) = TRUE;
	Section->MipsRegLo(Reg) = x86Reg;
	Section->MipsRegState(Reg) = SignValue ? CRegInfo::STATE_MAPPED_32_SIGN : CRegInfo::STATE_MAPPED_32_ZERO;
}

void Map_GPR_64bit (CCodeSection * Section, int Reg, int MipsRegToLoad) {
	int x86Hi, x86lo, count;

	if (Reg == 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Map_GPR_32bit\n\nWhy are you trying to map reg 0");
#endif
		return;
	}

	ProtectGPR(Section,Reg);
	if (Section->IsUnknown(Reg) || Section->IsConst(Reg)) {
		x86Hi = FreeX86Reg(Section);
		if (x86Hi < 0) {  DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
		Section->x86Protected(x86Hi) = TRUE;

		x86lo = FreeX86Reg(Section);
		if (x86lo < 0) {  DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
		Section->x86Protected(x86lo) = TRUE;
		
		CPU_Message("    regcache: allocate %s to hi word of %s",x86_Name(x86Hi),GPR_Name[Reg]);
		CPU_Message("    regcache: allocate %s to low word of %s",x86_Name(x86lo),GPR_Name[Reg]);
	} else {
		x86lo = Section->MipsRegLo(Reg);
		if (Section->Is32Bit(Reg)) {
			Section->x86Protected(x86lo) = TRUE;
			x86Hi = FreeX86Reg(Section);
			if (x86Hi < 0) {  DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
			Section->x86Protected(x86Hi) = TRUE;
		} else {
			x86Hi = Section->MipsRegHi(Reg);
		}
	}
	
	for (count = 0; count < 10; count ++) {
		if (Section->x86MapOrder(count) > 0) { Section->x86MapOrder(count) += 1; }
	}
	
	Section->x86MapOrder(x86Hi) = 1;
	Section->x86MapOrder(x86lo) = 1;
	if (MipsRegToLoad > 0) {
		if (Section->IsUnknown(MipsRegToLoad)) {
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[1],GPR_NameHi[MipsRegToLoad],x86Hi);
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0],GPR_NameLo[MipsRegToLoad],x86lo);
		} else if (Section->IsMapped(MipsRegToLoad)) {
			if (Section->Is32Bit(MipsRegToLoad)) {
				if (Section->IsSigned(MipsRegToLoad)) {
					MoveX86RegToX86Reg(Section->MipsRegLo(MipsRegToLoad),x86Hi);
					ShiftRightSignImmed(x86Hi,31);
				} else {
					XorX86RegToX86Reg(x86Hi,x86Hi);
				}
				if (Reg != MipsRegToLoad) {
					MoveX86RegToX86Reg(Section->MipsRegLo(MipsRegToLoad),x86lo);
				}
			} else {
				if (Reg != MipsRegToLoad) {
					MoveX86RegToX86Reg(Section->MipsRegHi(MipsRegToLoad),x86Hi);
					MoveX86RegToX86Reg(Section->MipsRegLo(MipsRegToLoad),x86lo);
				}
			}
		} else {
CPU_Message("Map_GPR_64bit 11");
			if (Section->Is32Bit(MipsRegToLoad)) {
				if (Section->IsSigned(MipsRegToLoad)) {
					if (Section->MipsRegLo((int)Section->MipsRegLo(MipsRegToLoad) >> 31) != 0) {
						MoveConstToX86reg((int)Section->MipsRegLo(MipsRegToLoad) >> 31,x86Hi);
					} else {
						XorX86RegToX86Reg(x86Hi,x86Hi);
					}
				} else {
					XorX86RegToX86Reg(x86Hi,x86Hi);
				}
			} else {
				if (Section->MipsRegHi(MipsRegToLoad) != 0) {
					MoveConstToX86reg(Section->MipsRegHi(MipsRegToLoad),x86Hi);
				} else {
					XorX86RegToX86Reg(x86Hi,x86Hi);
				}
			}
			if (Section->MipsRegLo(MipsRegToLoad) != 0) {
				MoveConstToX86reg(Section->MipsRegLo(MipsRegToLoad),x86lo);
			} else {
				XorX86RegToX86Reg(x86lo,x86lo);
			}
		}
	} else if (MipsRegToLoad == 0) {
		XorX86RegToX86Reg(x86Hi,x86Hi);
		XorX86RegToX86Reg(x86lo,x86lo);
	}
	Section->x86Mapped(x86Hi) = CRegInfo::GPR_Mapped;
	Section->x86Mapped(x86lo) = CRegInfo::GPR_Mapped;
	Section->MipsRegHi(Reg) = x86Hi;
	Section->MipsRegLo(Reg) = x86lo;
	Section->MipsRegState(Reg) = CRegInfo::STATE_MAPPED_64;
}

int Map_MemoryStack (CCodeSection * Section, int Reg, bool MapRegister)
{
	int CurrentMap = -1;

	// check to see what the current mapping is
	for (int x86Reg = 0; x86Reg < 10; x86Reg ++ ) {
		if (Section->x86Mapped(x86Reg) == CRegInfo::Stack_Mapped) {
			CurrentMap = x86Reg;
		}
	}
	if (!MapRegister)
	{
		//if not mapping then just return what the current mapping is
		return CurrentMap;
	}
	
	if (CurrentMap > 0 && CurrentMap == Reg)
	{
		//already mapped to correct reg
		return CurrentMap;
	}
	// map a register
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (Reg == x86_Any)
	{
		if (CurrentMap > 0)
		{
			return CurrentMap;
		}
		x86Reg = FreeX86Reg(Section);	
		if (x86Reg < 0) {
	#ifndef EXTERNAL_RELEASE
			DisplayError("Map_MemoryStack\n\nOut of registers");
			BreakPoint(__FILE__,__LINE__); 
	#endif
		}
		Section->x86Mapped(x86Reg) = CRegInfo::Stack_Mapped;
		CPU_Message("    regcache: allocate %s as Memory Stack",x86_Name(x86Reg));		
		MoveVariableToX86reg(g_MemoryStack,"MemoryStack",x86Reg);
		return x86Reg;
	}

	//move to a register/allocate register
	UnMap_X86reg(Section, Reg);
	if (CurrentMap >= 0)
	{
		CPU_Message("    regcache: change allocation of Memory Stack from %s to %s",x86_Name(CurrentMap),x86_Name(Reg));
		Section->x86Mapped(Reg) = CRegInfo::Stack_Mapped;
		Section->x86Mapped(CurrentMap) = CRegInfo::NotMapped;
		MoveX86RegToX86Reg(CurrentMap,Reg);
	} else {
		Section->x86Mapped(Reg) = CRegInfo::Stack_Mapped;
		CPU_Message("    regcache: allocate %s as Memory Stack",x86_Name(Reg));		
		MoveVariableToX86reg(g_MemoryStack,"MemoryStack",Reg);
	}
#endif
	return Reg;
}

int Map_TempReg (CCodeSection * Section, int x86Reg, int MipsReg, BOOL LoadHiWord) {
	int count;

	if (x86Reg == x86_Any) {		
		for (count = 0; count < 10; count ++ ) {
			if (Section->x86Mapped(count) == CRegInfo::Temp_Mapped) {
				if (Section->x86Protected(count) == FALSE) { x86Reg = count; }
			}
		}

		if (x86Reg == x86_Any) {
			x86Reg = FreeX86Reg(Section);
			if (x86Reg < 0) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Map_TempReg\n\nOut of registers");
				BreakPoint(__FILE__,__LINE__); 
#endif
					
				x86Reg = FreeX86Reg(Section);
				return -1;
			}
			CPU_Message("    regcache: allocate %s as temp storage",x86_Name(x86Reg));		
		}
	} else if (x86Reg == x86_Any8Bit) {
		if (Section->x86Mapped(x86_EBX) == CRegInfo::Temp_Mapped && !Section->x86Protected(x86_EBX)) { x86Reg = x86_EBX; }
		if (Section->x86Mapped(x86_EAX) == CRegInfo::Temp_Mapped && !Section->x86Protected(x86_EAX)) { x86Reg = x86_EAX; }
		if (Section->x86Mapped(x86_EDX) == CRegInfo::Temp_Mapped && !Section->x86Protected(x86_EDX)) { x86Reg = x86_EDX; }
		if (Section->x86Mapped(x86_ECX) == CRegInfo::Temp_Mapped && !Section->x86Protected(x86_ECX)) { x86Reg = x86_ECX; }
		
		if (x86Reg == x86_Any8Bit) {	
			x86Reg = Free8BitX86Reg(Section);
			if (x86Reg < 0) { 
#ifndef EXTERNAL_RELEASE
				DisplayError("Map_GPR_8bit\n\nOut of registers");
				BreakPoint(__FILE__,__LINE__); 
#endif
				return -1;
			}
		}
	} else {
		int NewReg;

		if (Section->x86Mapped(x86Reg) == CRegInfo::GPR_Mapped) {
			if (Section->x86Protected(x86Reg) == TRUE) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Map_TempReg\nRegister is protected !!!");
#endif
				return -1;
			}
			Section->x86Protected(x86Reg) = TRUE;
			NewReg = FreeX86Reg(Section);
			for (count = 1; count < 32; count ++) {
				if (Section->IsMapped(count)) {
					if (Section->MipsRegLo(count) == (DWORD)x86Reg) {
						if (NewReg < 0) {
							UnMap_GPR(Section,count,TRUE);
							count = 32;
							continue;
						}
						CPU_Message("    regcache: change allocation of %s from %s to %s",
							GPR_Name[count],x86_Name(x86Reg),x86_Name(NewReg));
						Section->x86Mapped(NewReg) = CRegInfo::GPR_Mapped;
						Section->x86MapOrder(NewReg) = Section->x86MapOrder(x86Reg);
						Section->MipsRegLo(count) = NewReg;
						MoveX86RegToX86Reg(x86Reg,NewReg);
						if (MipsReg == count && LoadHiWord == FALSE) { MipsReg = -1; }
						count = 32;
					}
					if (Section->Is64Bit(count) && Section->MipsRegHi(count) == (DWORD)x86Reg) {
						if (NewReg < 0) {
							UnMap_GPR(Section,count,TRUE);
							count = 32;
							continue;
						}
						CPU_Message("    regcache: change allocation of %s from %s to %s",
							GPR_NameHi[count],x86_Name(x86Reg),x86_Name(NewReg));
						Section->x86Mapped(NewReg) = CRegInfo::GPR_Mapped;
						Section->x86MapOrder(NewReg) = Section->x86MapOrder(x86Reg);
						Section->MipsRegHi(count) = NewReg;
						MoveX86RegToX86Reg(x86Reg,NewReg);
						if (MipsReg == count && LoadHiWord == TRUE) { MipsReg = -1; }
						count = 32;
					}
				}
			}
		}
		if (Section->x86Mapped(x86Reg) == CRegInfo::Stack_Mapped) {
			UnMap_X86reg(Section,x86Reg);
		}
		CPU_Message("    regcache: allocate %s as temp storage",x86_Name(x86Reg));		
	}
	if (MipsReg >= 0) {
		if (LoadHiWord) {
			if (Section->IsUnknown(MipsReg)) {
				MoveVariableToX86reg(&_GPR[MipsReg].UW[1],GPR_NameHi[MipsReg],x86Reg);
			} else if (Section->IsMapped(MipsReg)) {
				if (Section->Is64Bit(MipsReg)) {
					MoveX86RegToX86Reg(Section->MipsRegHi(MipsReg),x86Reg);
				} else if (Section->IsSigned(MipsReg)){
					MoveX86RegToX86Reg(Section->MipsRegLo(MipsReg),x86Reg);
					ShiftRightSignImmed(x86Reg,31);
				} else {
					MoveConstToX86reg(0,x86Reg);
				}
			} else {
				if (Section->Is64Bit(MipsReg)) {
					if (Section->MipsRegHi(MipsReg) != 0) {
						MoveConstToX86reg(Section->MipsRegHi(MipsReg),x86Reg);
					} else {
						XorX86RegToX86Reg(x86Reg,x86Reg);
					}
				} else {
					if ((int)Section->MipsRegLo(MipsReg) >> 31 != 0) {
						MoveConstToX86reg((int)Section->MipsRegLo(MipsReg) >> 31,x86Reg);
					} else {
						XorX86RegToX86Reg(x86Reg,x86Reg);
					}
				}
			}
		} else {
			if (Section->IsUnknown(MipsReg)) {
				MoveVariableToX86reg(&_GPR[MipsReg].UW[0],GPR_NameLo[MipsReg],x86Reg);
			} else if (Section->IsMapped(MipsReg)) {
				MoveX86RegToX86Reg(Section->MipsRegLo(MipsReg),x86Reg);
			} else {
				if (Section->MipsRegLo(MipsReg) != 0) {
					MoveConstToX86reg(Section->MipsRegLo(MipsReg),x86Reg);
				} else {
					XorX86RegToX86Reg(x86Reg,x86Reg);
				}
			}
		}
	}
	Section->x86Mapped(x86Reg) = CRegInfo::Temp_Mapped;
	Section->x86Protected(x86Reg) = TRUE;
	for (count = 0; count < 10; count ++) {
		if (Section->x86MapOrder(count) > 0) { 
			Section->x86MapOrder(count) += 1;
		}
	}
	Section->x86MapOrder(x86Reg) = 1;
	return x86Reg;
}

void ProtectGPR(CCodeSection * Section, DWORD Reg) {
	if (Section->IsUnknown(Reg)) { return; }
	if (Section->IsConst(Reg)) { return; }
	if (Section->Is64Bit(Reg)) {
		Section->x86Protected(Section->MipsRegHi(Reg)) = TRUE;
	}
	Section->x86Protected(Section->MipsRegLo(Reg)) = TRUE;
}

BOOL RegInStack(CCodeSection * Section,int Reg, int Format) {
	int i;

	for (i = 0; i < 8; i++) {
		if (Section->FpuMappedTo(i) == (DWORD)Reg) {
			if (Section->FpuState(i) == (DWORD)Format) { return TRUE; }
			else if (Format == -1) { return TRUE; }
			return FALSE;
		}
	}
	return FALSE;
}


#ifdef ggg
void SetFpuLocations (void) {
	int count;

	if ((STATUS_REGISTER & STATUS_FR) == 0) {
		for (count = 0; count < 32; count ++) {
			_FPRFloatLocation[count] = (void *)(&FPR[count >> 1].W[count & 1]);
			//_FPRDoubleLocation[count] = _FPRFloatLocation[count];
			_FPRDoubleLocation[count] = (void *)(&FPR[count >> 1].DW);
		}
	} else {
		for (count = 0; count < 32; count ++) {
			_FPRFloatLocation[count] = (void *)(&FPR[count].W[1]);
			//_FPRDoubleLocation[count] = _FPRFloatLocation[count];
			_FPRDoubleLocation[count] = (void *)(&FPR[count].DW);
		}
	}
}

void SetupRegisters(N64_REGISTERS * n64_Registers) {
	PROGRAM_COUNTER = n64_Registers->PROGRAM_COUNTER;
	HI.DW    = n64_Registers->HI.DW;
	LO.DW    = n64_Registers->LO.DW;
	CP0      = n64_Registers->CP0;
	_GPR      = n64_Registers->_GPR;
	FPR      = n64_Registers->FPR;
	FPCR     = n64_Registers->FPCR;
	RegRDRAM = n64_Registers->RDRAM;
	RegSP    = n64_Registers->SP;
	RegDPC   = n64_Registers->DPC;
	RegMI    = n64_Registers->MI;
	RegVI    = n64_Registers->VI;
	RegAI    = n64_Registers->AI;
	RegPI    = n64_Registers->PI;
	RegRI    = n64_Registers->RI;
	RegSI    = n64_Registers->SI;
	PIF_Ram  = n64_Registers->PIF_Ram;
	DMAUsed  = n64_Registers->DMAUsed;
}
#endif

int StackPosition (CCodeSection * Section,int Reg) {
	int i;

	for (i = 0; i < 8; i++) {
		if (Section->FpuMappedTo(i) == (DWORD)Reg) {
			return ((i - Section->StackTopPos()) & 7);
		}
	}
	return -1;
}

int UnMap_8BitTempReg (CCodeSection * Section) {
	int count;

	for (count = 0; count < 10; count ++) {
		if (!Is8BitReg(count)) { continue; }
		if (Section->MipsRegState(count) == CRegInfo::Temp_Mapped) {
			if (Section->x86Protected(count) == FALSE) {
				CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(count));
				Section->x86Mapped(count) = CRegInfo::NotMapped;
				return count;
			}		
		}
	}
	return -1;
}

void UnMap_AllFPRs ( CCodeSection * Section ) {
	DWORD StackPos;

	for (;;) {
		int i, StartPos;
		StackPos = Section->StackTopPos();
		if (Section->FpuMappedTo(Section->StackTopPos()) != -1 ) {
			UnMap_FPR(Section,Section->FpuMappedTo(Section->StackTopPos()),TRUE);
			continue;
		}
		//see if any more registers mapped
		StartPos = Section->StackTopPos();
		for (i = 0; i < 8; i++) {
			if (Section->FpuMappedTo((StartPos + i) & 7) != -1 ) { fpuIncStack(&Section->StackTopPos()); }
		}
		if (StackPos != Section->StackTopPos()) { continue; }
		return;
	}
}

void FixRoundModel(CCodeSection * Section, CRegInfo::FPU_ROUND RoundMethod )
{
	if (Section->CurrentRoundingModel() == RoundMethod) 
	{
		return;
	}

	fpuControl = 0;			
	fpuStoreControl(&fpuControl, "fpuControl");
	int x86reg = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveVariableToX86reg(&fpuControl, "fpuControl", x86reg);
	AndConstToX86Reg(x86reg, 0xF3FF);
	
	switch (RoundMethod) {
	case CRegInfo::RoundDefault: OrVariableToX86Reg(&FPU_RoundingMode,"FPU_RoundingMode", x86reg); break;
	case CRegInfo::RoundTruncate: OrConstToX86Reg(0x0C00, x86reg); break;
	case CRegInfo::RoundNearest: /*OrConstToX86Reg(0x0000, x86reg);*/ break;
	case CRegInfo::RoundDown: OrConstToX86Reg(0x0400, x86reg); break;
	case CRegInfo::RoundUp: OrConstToX86Reg(0x0800, x86reg); break;
	default:
		DisplayError("Unknown Rounding model");
	}
	MoveX86regToVariable(x86reg, &fpuControl, "fpuControl");
	fpuLoadControl(&fpuControl, "fpuControl");
	Section->CurrentRoundingModel() = RoundMethod;
}

void UnMap_FPR (CCodeSection * Section, int Reg, int WriteBackValue ) {
	char Name[50];
	int TempReg;
	int i;

	if (Reg < 0) { return; }
	for (i = 0; i < 8; i++) {
		if (Section->FpuMappedTo(i) != (DWORD)Reg) { continue; }
		CPU_Message("    regcache: unallocate %s from ST(%d)",FPR_Name[Reg],(i - Section->StackTopPos() + 8) & 7);
		if (WriteBackValue) {
			int RegPos;
			
			if (((i - Section->StackTopPos() + 8) & 7) != 0) {
				CRegInfo::FPU_ROUND RoundingModel = Section->FpuRoundingModel(Section->StackTopPos());
				CRegInfo::FPU_STATE RegState      = Section->FpuState(Section->StackTopPos());
				DWORD MappedTo      = Section->FpuMappedTo(Section->StackTopPos());
				Section->FpuRoundingModel(Section->StackTopPos()) = Section->FpuRoundingModel(i);
				Section->FpuMappedTo(Section->StackTopPos())      = Section->FpuMappedTo(i);
				Section->FpuState(Section->StackTopPos())         = Section->FpuState(i);
				Section->FpuRoundingModel(i) = RoundingModel; 
				Section->FpuMappedTo(i)      = MappedTo;
				Section->FpuState(i)         = RegState;
				fpuExchange((i - Section->StackTopPos()) & 7);
			}
			
			CPU_Message("CurrentRoundingModel: %d  Section->FpuRoundingModel(i): %d",
				Section->CurrentRoundingModel(),Section->FpuRoundingModel(i));

			FixRoundModel(Section,Section->FpuRoundingModel(i));

			RegPos = Section->StackTopPos();
			TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
			switch (Section->FpuState(Section->StackTopPos())) {
			case CRegInfo::FPU_Dword: 
				sprintf(Name,"_FPRFloatLocation[%d]",Section->FpuMappedTo(Section->StackTopPos()));
				MoveVariableToX86reg(&_FPRFloatLocation[Section->FpuMappedTo(Section->StackTopPos())],Name,TempReg);
				fpuStoreIntegerDwordFromX86Reg(&Section->StackTopPos(),TempReg, TRUE); 
				break;
			case CRegInfo::FPU_Qword: 
				sprintf(Name,"_FPRDoubleLocation[%d]",Section->FpuMappedTo(Section->StackTopPos()));
				MoveVariableToX86reg(&_FPRDoubleLocation[Section->FpuMappedTo(Section->StackTopPos())],Name,TempReg);
				fpuStoreIntegerQwordFromX86Reg(&Section->StackTopPos(),TempReg, TRUE); 
				break;
			case CRegInfo::FPU_Float: 
				sprintf(Name,"_FPRFloatLocation[%d]",Section->FpuMappedTo(Section->StackTopPos()));
				MoveVariableToX86reg(&_FPRFloatLocation[Section->FpuMappedTo(Section->StackTopPos())],Name,TempReg);
				fpuStoreDwordFromX86Reg(&Section->StackTopPos(),TempReg, TRUE); 
				break;
			case CRegInfo::FPU_Double: 
				sprintf(Name,"_FPRDoubleLocation[%d]",Section->FpuMappedTo(Section->StackTopPos()));
				MoveVariableToX86reg(&_FPRDoubleLocation[Section->FpuMappedTo(Section->StackTopPos())],Name,TempReg);
				fpuStoreQwordFromX86Reg(&Section->StackTopPos(),TempReg, TRUE); 
				break;
#ifndef EXTERNAL_RELEASE
			default:
				DisplayError("UnMap_FPR\nUnknown format to load %d",Section->FpuState(Section->StackTopPos()));
#endif
			}
			Section->x86Protected(TempReg) = FALSE;
			Section->FpuRoundingModel(RegPos) = CRegInfo::RoundDefault;
			Section->FpuMappedTo(RegPos)      = -1;
			Section->FpuState(RegPos)         = CRegInfo::FPU_Unkown;
		} else {				
			fpuFree((i - Section->StackTopPos()) & 7);
			Section->FpuRoundingModel(i) = CRegInfo::RoundDefault;
			Section->FpuMappedTo(i)      = -1;
			Section->FpuState(i)         = CRegInfo::FPU_Unkown;
		}
		return;
	}
}

void UnMap_GPR (CCodeSection * Section, DWORD Reg, int WriteBackValue) {
	if (Reg == 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("UnMap_GPR\n\nWhy are you trying to unmap reg 0");
#endif
		return;
	}

	if (Section->IsUnknown(Reg)) { return; }
	//CPU_Message("UnMap_GPR: State: %X\tReg: %s\tWriteBack: %s",State,GPR_Name[Reg],WriteBackValue?"TRUE":"FALSE");
	if (Section->IsConst(Reg)) { 
		if (!WriteBackValue) { 
			Section->MipsRegState(Reg) = CRegInfo::STATE_UNKNOWN;
			return; 
		}
		if (Section->Is64Bit(Reg)) {
			MoveConstToVariable(Section->MipsRegHi(Reg),&_GPR[Reg].UW[1],GPR_NameHi[Reg]);
			MoveConstToVariable(Section->MipsRegLo(Reg),&_GPR[Reg].UW[0],GPR_NameLo[Reg]);
			Section->MipsRegState(Reg) = CRegInfo::STATE_UNKNOWN;
			return;
		}
		if ((Section->MipsRegLo(Reg) & 0x80000000) != 0) {
			MoveConstToVariable(0xFFFFFFFF,&_GPR[Reg].UW[1],GPR_NameHi[Reg]);
		} else {
			MoveConstToVariable(0,&_GPR[Reg].UW[1],GPR_NameHi[Reg]);
		}
		MoveConstToVariable(Section->MipsRegLo(Reg),&_GPR[Reg].UW[0],GPR_NameLo[Reg]);
		Section->MipsRegState(Reg) = CRegInfo::STATE_UNKNOWN;
		return;
	}
	if (Section->Is64Bit(Reg)) {
		CPU_Message("    regcache: unallocate %s from %s",x86_Name(Section->MipsRegHi(Reg)),GPR_NameHi[Reg]);
		Section->x86Mapped(Section->MipsRegHi(Reg)) = CRegInfo::NotMapped;
		Section->x86Protected(Section->MipsRegHi(Reg)) = FALSE;
	}
	CPU_Message("    regcache: unallocate %s from %s",x86_Name(Section->MipsRegLo(Reg)),GPR_NameLo[Reg]);
	Section->x86Mapped(Section->MipsRegLo(Reg)) = CRegInfo::NotMapped;
	Section->x86Protected(Section->MipsRegLo(Reg)) = FALSE;
	if (!WriteBackValue) { 
		Section->MipsRegState(Reg) = CRegInfo::STATE_UNKNOWN;
		return; 
	}
	MoveX86regToVariable(Section->MipsRegLo(Reg),&_GPR[Reg].UW[0],GPR_NameLo[Reg]);
	if (Section->Is64Bit(Reg)) {
		MoveX86regToVariable(Section->MipsRegHi(Reg),&_GPR[Reg].UW[1],GPR_NameHi[Reg]);
	} else {
		if (Section->IsSigned(Reg)) {
			ShiftRightSignImmed(Section->MipsRegLo(Reg),31);
			MoveX86regToVariable(Section->MipsRegLo(Reg),&_GPR[Reg].UW[1],GPR_NameHi[Reg]);
		} else {
			MoveConstToVariable(0,&_GPR[Reg].UW[1],GPR_NameHi[Reg]);
		}
	}
	Section->MipsRegState(Reg) = CRegInfo::STATE_UNKNOWN;
}

int UnMap_TempReg (CCodeSection * Section) {
	int count;

	for (count = 0; count < 10; count ++) {
		if (Section->x86Mapped(count) == CRegInfo::Temp_Mapped) {
			if (Section->x86Protected(count) == FALSE) {
				CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(count));
				Section->x86Mapped(count) = CRegInfo::NotMapped;
				return count;
			}		
		}
	}
	return -1;
}

BOOL UnMap_X86reg (CCodeSection * Section, DWORD x86Reg) {
	int count;

	if (Section->x86Mapped(x86Reg) == CRegInfo::NotMapped && Section->x86Protected(x86Reg) == FALSE) { return TRUE; }
	if (Section->x86Mapped(x86Reg) == CRegInfo::Temp_Mapped) { 
		if (Section->x86Protected(x86Reg) == FALSE) {
			CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(x86Reg));
			Section->x86Mapped(x86Reg) = CRegInfo::NotMapped;
			return TRUE;
		}
		return FALSE;
	}
	for (count = 1; count < 32; count ++) {
		if (Section->IsMapped(count)) {
			if (Section->Is64Bit(count)) {
				if (Section->MipsRegHi(count) == x86Reg) {
					if (Section->x86Protected(x86Reg) == FALSE) {
						UnMap_GPR(Section,count,TRUE);
						return TRUE;
					}
					break;
				}
			} 
			if (Section->MipsRegLo(count) == x86Reg) {
				if (Section->x86Protected(x86Reg) == FALSE) {
					UnMap_GPR(Section,count,TRUE);
					return TRUE;
				}
				break;
			}
		}
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (Section->x86Mapped(x86Reg) == CRegInfo::Stack_Mapped) { 
		CPU_Message("    regcache: unallocate %s from Memory Stack",x86_Name(x86Reg));
		MoveX86regToVariable(x86Reg,g_MemoryStack,"MemoryStack");
		Section->x86Mapped(x86Reg) = CRegInfo::NotMapped;
		return TRUE;
	}
#endif
	return FALSE;
}

void UnProtectGPR(CCodeSection * Section, DWORD Reg) {
	if (Section->IsUnknown(Reg)) { return; }
	if (Section->IsConst(Reg)) { return; }
	if (Section->Is64Bit(Reg)) {
		Section->x86Protected(Section->MipsRegHi(Reg)) = FALSE;
	}
	Section->x86Protected(Section->MipsRegLo(Reg)) = FALSE;
}

/*void WriteBackRegisters (CCodeSection * Section) {
	int count;

	for (count = 1; count < 10; count ++) { Section->x86Protected(count) = FALSE; }
	for (count = 1; count < 10; count ++) { UnMap_X86reg (Section, count); }
	for (count = 1; count < 32; count ++) {
		switch (Section->MipsRegState(count)) {
		case CRegInfo::STATE_UNKNOWN: break;
		case STATE_CONST_32:
			if ((Section->MipsRegLo(count) & 0x80000000) != 0) {
				MoveConstToVariable(0xFFFFFFFF,&_GPR[count].UW[1],GPR_NameHi[count]);
			} else {
				MoveConstToVariable(0,&_GPR[count].UW[1],GPR_NameHi[count]);
			}
			MoveConstToVariable(Section->MipsRegLo(count),&_GPR[count].UW[0],GPR_NameLo[count]);
			Section->MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
			break;
		default:
			DisplayError("Unknown State: %d\nin WriteBackRegisters",Section->MipsRegState(count));
		}
	}
	UnMap_AllFPRs(Section);
}*/
void WriteBackRegisters (CCodeSection * Section) {
	int count;
	BOOL bEdiZero = FALSE;
	BOOL bEsiSign = FALSE;
	/*** coming soon ***/
	BOOL bEaxGprLo = FALSE;
	BOOL bEbxGprHi = FALSE;

	for (count = 1; count < 10; count ++) { Section->x86Protected(count) = FALSE; }
	for (count = 1; count < 10; count ++) { UnMap_X86reg (Section, count); }

	/*************************************/
	
	for (count = 1; count < 32; count ++) {
		switch (Section->MipsRegState(count)) {
		case CRegInfo::STATE_UNKNOWN: break;
		case CRegInfo::STATE_CONST_32:
			if (!bEdiZero && (!Section->MipsRegLo(count) || !(Section->MipsRegLo(count) & 0x80000000))) {
				XorX86RegToX86Reg(x86_EDI, x86_EDI);
				bEdiZero = TRUE;
			}
			if (!bEsiSign && (Section->MipsRegLo(count) & 0x80000000)) {
				MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
				bEsiSign = TRUE;
			}

			if ((Section->MipsRegLo(count) & 0x80000000) != 0) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[1],GPR_NameHi[count]);
			} else {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],GPR_NameHi[count]);
			}

			if (Section->MipsRegLo(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],GPR_NameLo[count]);
			} else if (Section->MipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[0],GPR_NameLo[count]);
			} else
				MoveConstToVariable(Section->MipsRegLo(count),&_GPR[count].UW[0],GPR_NameLo[count]);

			Section->MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
			break;
		case CRegInfo::STATE_CONST_64:
			if (Section->MipsRegLo(count) == 0 || Section->MipsRegHi(count) == 0) {
				XorX86RegToX86Reg(x86_EDI, x86_EDI);
				bEdiZero = TRUE;
			}
			if (Section->MipsRegLo(count) == 0xFFFFFFFF || Section->MipsRegHi(count) == 0xFFFFFFFF) {
				MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
				bEsiSign = TRUE;
			}

			if (Section->MipsRegHi(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],GPR_NameHi[count]);
			} else if (Section->MipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[1],GPR_NameHi[count]);
			} else {
				MoveConstToVariable(Section->MipsRegHi(count),&_GPR[count].UW[1],GPR_NameHi[count]);
			} 

			if (Section->MipsRegLo(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],GPR_NameLo[count]);
			} else if (Section->MipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[0],GPR_NameLo[count]);
			} else {
				MoveConstToVariable(Section->MipsRegLo(count),&_GPR[count].UW[0],GPR_NameLo[count]);
			}
			Section->MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
			break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Unknown State: %d\nin WriteBackRegisters",Section->MipsRegState(count));
#endif
		}
	}
	UnMap_AllFPRs(Section);
}

