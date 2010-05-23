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
#include "c core.h"
#include "main.h"
#include "cpu.h"
#include "x86.h"
#include "debugger.h"
#include "Recompiler Ops.h"


DWORD BranchCompare = 0;

void CompileReadTLBMiss (CBlockSection * Section, int AddressReg, int LookUpReg ) {
	MoveX86regToVariable(AddressReg,&TLBLoadAddress,"TLBLoadAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC, Section->CompilePC,Section->RegWorking,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
}

void CompileWriteTLBMiss (CBlockSection * Section, int AddressReg, int LookUpReg ) 
{
	MoveX86regToVariable(AddressReg,&TLBStoreAddress,"TLBStoreAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC, Section->CompilePC,Section->RegWorking,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
}

/************************** Branch functions  ************************/
void Compile_R4300i_Branch (CBlockSection * Section, void (*CompareFunc)(CBlockSection * Section), BRANCH_TYPE BranchType, BOOL Link) {
	static int EffectDelaySlot, DoneJumpDelay, DoneContinueDelay;
	static char ContLabel[100], JumpLabel[100];
	static CRegInfo RegBeforeDelay;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		
		if ((Section->CompilePC & 0xFFC) != 0xFFC) {
			switch (BranchType) {
			case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0); break;
			case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,Opcode.rt); break;
			case BranchTypeCop1: 
				{
					OPCODE Command;

					if (!_MMU ->LW_VAddr(Section->CompilePC + 4, Command.Hex)) {
						DisplayError(GS(MSG_FAIL_LOAD_WORD));
						ExitThread(0);
					}
					
					EffectDelaySlot = FALSE;
					if (Command.op == R4300i_CP1) {
						if (Command.fmt == R4300i_COP1_S && (Command.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
						if (Command.fmt == R4300i_COP1_D && (Command.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
					}
				}
				break;
#ifndef EXTERNAL_RELEASE
			default:
				DisplayError("Unknown branch type");
#endif
			}
		} else {
			EffectDelaySlot = TRUE;
		}
		if (Section->ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((CBlockSection *)Section->ContinueSection)->SectionID);
		} else {
			strcpy(ContLabel,"Cont.LinkLocationinue");
		}
		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CBlockSection *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"Jump.LinkLocation");
		}
		Section->Jump.TargetPC        = Section->CompilePC + ((short)Opcode.offset << 2) + 4;
		Section->Jump.BranchLabel     = JumpLabel;
		Section->Jump.LinkLocation    = NULL;
		Section->Jump.LinkLocation2   = NULL;
		Section->Jump.DoneDelaySlot   = FALSE;
		Section->Cont.TargetPC        = Section->CompilePC + 8;
		Section->Cont.BranchLabel     = ContLabel;
		Section->Cont.LinkLocation    = NULL;
		Section->Cont.LinkLocation2   = NULL;
		Section->Cont.DoneDelaySlot   = FALSE;
		if (Section->Jump.TargetPC < Section->Cont.TargetPC) {
			Section->Cont.FallThrough = FALSE;
			Section->Jump.FallThrough = TRUE;
		} else {
			Section->Cont.FallThrough = TRUE;
			Section->Jump.FallThrough = FALSE;
		}
		if (Link) {
			UnMap_GPR(Section, 31, FALSE);
			Section->MipsRegLo(31) = Section->CompilePC + 8;
			Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		if (EffectDelaySlot) {
			if (Section->ContinueSection != NULL) {
				sprintf(ContLabel,"Continue",((CBlockSection *)Section->ContinueSection)->SectionID);
			} else {
				strcpy(ContLabel,"ExitBlock");
			}
			if (Section->JumpSection != NULL) {
				sprintf(JumpLabel,"Jump",((CBlockSection *)Section->JumpSection)->SectionID);
			} else {
				strcpy(JumpLabel,"ExitBlock");
			}
			CompareFunc(Section); 
			
			if ((Section->CompilePC & 0xFFC) == 0xFFC) {
				_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
				NextInstruction = END_BLOCK;
				return;
			}
			if (!Section->Jump.FallThrough && !Section->Cont.FallThrough) {
				if (Section->Jump.LinkLocation != NULL) {
					CPU_Message("");
					CPU_Message("      %s:",Section->Jump.BranchLabel);
					SetJump32((DWORD *)Section->Jump.LinkLocation,(DWORD *)RecompPos);
					Section->Jump.LinkLocation = NULL;
					if (Section->Jump.LinkLocation2 != NULL) {
						SetJump32((DWORD *)Section->Jump.LinkLocation2,(DWORD *)RecompPos);
						Section->Jump.LinkLocation2 = NULL;
					}
					Section->Jump.FallThrough = TRUE;
				} else if (Section->Cont.LinkLocation != NULL){
					CPU_Message("");
					CPU_Message("      %s:",Section->Cont.BranchLabel);
					SetJump32((DWORD *)Section->Cont.LinkLocation,(DWORD *)RecompPos);
					Section->Cont.LinkLocation = NULL;
					if (Section->Cont.LinkLocation2 != NULL) {
						SetJump32((DWORD *)Section->Cont.LinkLocation2,(DWORD *)RecompPos);
						Section->Cont.LinkLocation2 = NULL;
					}
					Section->Cont.FallThrough = TRUE;
				}
			}
			Section->ResetX86Protection();
			memcpy(&RegBeforeDelay,&Section->RegWorking,sizeof(CRegInfo));
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (EffectDelaySlot) { 
			CJumpInfo * FallInfo = Section->Jump.FallThrough?&Section->Jump:&Section->Cont;
			CJumpInfo * JumpInfo = Section->Jump.FallThrough?&Section->Cont:&Section->Jump;

			if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot) {
				Section->ResetX86Protection();
				FallInfo->RegSet = Section->RegWorking;
				if (FallInfo == &Section->Jump) {
					if (Section->JumpSection != NULL) {
						sprintf(JumpLabel,"Section_%d",((CBlockSection *)Section->JumpSection)->SectionID);
					} else {
						strcpy(JumpLabel,"ExitBlock");
					}
					if (FallInfo->TargetPC <= Section->CompilePC) 
					{
						_N64System->GetRecompiler()->UpdateCounters(&(FallInfo->RegSet.BlockCycleCount()),&(FallInfo->RegSet.BlockRandomModifier()),true);
						_N64System->GetRecompiler()->CompileSystemCheck(FallInfo->TargetPC,FallInfo->RegSet);
						Section->ResetX86Protection();
					}
				} else {
					if (Section->ContinueSection != NULL) {
						sprintf(ContLabel,"Section_%d",((CBlockSection *)Section->ContinueSection)->SectionID);
					} else {
						strcpy(ContLabel,"ExitBlock");
					}
				}		
				FallInfo->DoneDelaySlot = TRUE;
				if (!JumpInfo->DoneDelaySlot) {
					FallInfo->FallThrough = FALSE;				
					JmpLabel32(FallInfo->BranchLabel,0);
					FallInfo->LinkLocation = RecompPos - 4;
					
					if (JumpInfo->LinkLocation != NULL) {
						CPU_Message("      %s:",JumpInfo->BranchLabel);
						SetJump32((DWORD *)JumpInfo->LinkLocation,(DWORD *)RecompPos);
						JumpInfo->LinkLocation = NULL;
						if (JumpInfo->LinkLocation2 != NULL) {
							SetJump32((DWORD *)JumpInfo->LinkLocation2,(DWORD *)RecompPos);
							JumpInfo->LinkLocation2 = NULL;
						}
						JumpInfo->FallThrough = TRUE;
						NextInstruction = DO_DELAY_SLOT;
						memcpy(&Section->RegWorking,&RegBeforeDelay,sizeof(CRegInfo));
						return; 
					}
				}
			}
		} else {
			CompareFunc(Section);
			Section->ResetX86Protection();
			memcpy(&Section->Cont.RegSet,&Section->RegWorking,sizeof(CRegInfo));
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
		}
		_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_BranchLikely (CBlockSection * Section, void (*CompareFunc)(CBlockSection * Section), BOOL Link) {
	static char ContLabel[100], JumpLabel[100];
	if ( NextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		
		if (Section->ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((CBlockSection *)Section->ContinueSection)->SectionID);
		} else {
			strcpy(ContLabel,"ExitBlock");
		}
		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CBlockSection *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		Section->Jump.TargetPC      = Section->CompilePC + ((short)Opcode.offset << 2) + 4;
		Section->Jump.BranchLabel   = JumpLabel;
		Section->Jump.FallThrough   = TRUE;
		Section->Jump.LinkLocation  = NULL;
		Section->Jump.LinkLocation2 = NULL;
		Section->Cont.TargetPC      = Section->CompilePC + 8;
		Section->Cont.BranchLabel   = ContLabel;
		Section->Cont.FallThrough   = FALSE;
		Section->Cont.LinkLocation  = NULL;
		Section->Cont.LinkLocation2 = NULL;
		if (Link) {
			UnMap_GPR(Section, 31, FALSE);
			Section->MipsRegLo(31) = Section->CompilePC + 8;
			Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		CompareFunc(Section); 
		Section->ResetX86Protection();
		memcpy(&Section->Cont.RegSet,&Section->RegWorking,sizeof(CRegInfo));
		if (UseLinking && Section->Jump.TargetPC == Section->Cont.TargetPC)
		{
			if (Section->Cont.FallThrough)  
			{
				BreakPoint(__FILE__,__LINE__);
			}
			if (!Section->Jump.FallThrough)
			{
				BreakPoint(__FILE__,__LINE__);
			}
			Section->JumpSection->Cont.TargetPC = Section->Jump.TargetPC;
			Section->JumpSection->DelaySlotSection = true;
			Section->Jump.TargetPC = Section->CompilePC + 4;
			Section->Jump.RegSet = Section->RegWorking;
			_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
			NextInstruction = END_BLOCK;
		} else {
			if (Section->Cont.FallThrough)  {
				if (Section->Jump.LinkLocation != NULL) {
	#ifndef EXTERNAL_RELEASE
					DisplayError("WTF .. problem with Compile_R4300i_BranchLikely");
	#endif
				}
				_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
				NextInstruction = END_BLOCK;
			} else {
				if ((Section->CompilePC & 0xFFC) == 0xFFC) {
					Section->Jump.FallThrough = FALSE;
					if (Section->Jump.LinkLocation != NULL) {
						SetJump32(Section->Jump.LinkLocation,RecompPos);
						Section->Jump.LinkLocation = NULL;
						if (Section->Jump.LinkLocation2 != NULL) { 
							SetJump32(Section->Jump.LinkLocation2,RecompPos);
							Section->Jump.LinkLocation2 = NULL;
						}
					}
					JmpLabel32("DoDelaySlot",0);
					Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      %s:",Section->Cont.BranchLabel);
					if (Section->Cont.LinkLocation != NULL) {
						SetJump32(Section->Cont.LinkLocation,RecompPos);
						Section->Cont.LinkLocation = NULL;
						if (Section->Cont.LinkLocation2 != NULL) { 
							SetJump32(Section->Cont.LinkLocation2,RecompPos);
							Section->Cont.LinkLocation2 = NULL;
						}
					}
					_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC, Section->CompilePC + 8,Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
					CPU_Message("      ");
					CPU_Message("      DoDelaySlot");
					_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
					NextInstruction = END_BLOCK;
				} else {
					NextInstruction = DO_DELAY_SLOT;
				}
			}
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {
		Section->ResetX86Protection();
		memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranchLikely\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void BNE_Compare (CBlockSection * Section) {
	BYTE *Jump;

	if (Section->IsKnown(Opcode.rs) && Section->IsKnown(Opcode.rt)) {
		if (Section->IsConst(Opcode.rs) && Section->IsConst(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rs) || Section->Is64Bit(Opcode.rt)) {
				Compile_R4300i_UnknownOpcode(Section);
			} else if (Section->MipsRegLo(Opcode.rs) != Section->MipsRegLo(Opcode.rt)) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (Section->IsMapped(Opcode.rs) && Section->IsMapped(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rs) || Section->Is64Bit(Opcode.rt)) {
				ProtectGPR(Section,Opcode.rs);
				ProtectGPR(Section,Opcode.rt);

				CompX86RegToX86Reg(
					Section->Is32Bit(Opcode.rs)?Map_TempReg(Section,x86_Any,Opcode.rs,TRUE):Section->MipsRegHi(Opcode.rs),
					Section->Is32Bit(Opcode.rt)?Map_TempReg(Section,x86_Any,Opcode.rt,TRUE):Section->MipsRegHi(Opcode.rt)
				);
					
				if (Section->Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs),Section->MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation2 = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation2 = RecompPos - 4;
				}
			} else {
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs),Section->MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		} else {
			DWORD ConstReg = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Section->Is64Bit(ConstReg) || Section->Is64Bit(MappedReg)) {
				if (Section->Is32Bit(ConstReg) || Section->Is32Bit(MappedReg)) {
					ProtectGPR(Section,MappedReg);
					if (Section->Is32Bit(MappedReg)) {
						CompConstToX86reg(Map_TempReg(Section,x86_Any,MappedReg,TRUE),Section->MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(Section->MipsRegHi(MappedReg),(int)Section->MipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(Section->MipsRegHi(MappedReg),Section->MipsRegHi(ConstReg));
				}
				if (Section->Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
				if (Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation2 = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation2 = RecompPos - 4;
				}
			} else {
				if (Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		}
	} else if (Section->IsKnown(Opcode.rs) || Section->IsKnown(Opcode.rt)) {
		DWORD KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (Section->IsConst(KnownReg)) {
			if (Section->Is64Bit(KnownReg)) {
				CompConstToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (Section->IsSigned(KnownReg)) {
				CompConstToVariable(((int)Section->MipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (Section->IsSigned(KnownReg)) {
				ProtectGPR(Section,KnownReg);
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (Section->Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
		if (Section->IsConst(KnownReg)) {
			CompConstToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (Section->Cont.FallThrough) {
			JneLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation2 = RecompPos - 4;
		} else if (Section->Jump.FallThrough) {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation2 = RecompPos - 4;
		}
	} else {
		int x86Reg;

		x86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);		
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		if (Section->Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}

		x86Reg = Map_TempReg(Section,x86Reg,Opcode.rt,FALSE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		if (Section->Cont.FallThrough) {
			JneLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation2 = RecompPos - 4;
		} else if (Section->Jump.FallThrough) {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation2 = RecompPos - 4;
		}
	}
}

void BEQ_Compare (CBlockSection * Section) {
	BYTE *Jump;

	if (Section->IsKnown(Opcode.rs) && Section->IsKnown(Opcode.rt)) {
		if (Section->IsConst(Opcode.rs) && Section->IsConst(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rs) || Section->Is64Bit(Opcode.rt)) {
				Compile_R4300i_UnknownOpcode(Section);
			} else if (Section->MipsRegLo(Opcode.rs) == Section->MipsRegLo(Opcode.rt)) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (Section->IsMapped(Opcode.rs) && Section->IsMapped(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rs) || Section->Is64Bit(Opcode.rt)) {
				ProtectGPR(Section,Opcode.rs);
				ProtectGPR(Section,Opcode.rt);

				CompX86RegToX86Reg(
					Section->Is32Bit(Opcode.rs)?Map_TempReg(Section,x86_Any,Opcode.rs,TRUE):Section->MipsRegHi(Opcode.rs),
					Section->Is32Bit(Opcode.rt)?Map_TempReg(Section,x86_Any,Opcode.rt,TRUE):Section->MipsRegHi(Opcode.rt)
				);
				if (Section->Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Cont.BranchLabel,0);
					Section->Cont.LinkLocation = RecompPos - 4;
				}
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs),Section->MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			} else {
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs),Section->MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		} else {
			DWORD ConstReg = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Section->Is64Bit(ConstReg) || Section->Is64Bit(MappedReg)) {
				if (Section->Is32Bit(ConstReg) || Section->Is32Bit(MappedReg)) {
					if (Section->Is32Bit(MappedReg)) {
						ProtectGPR(Section,MappedReg);
						CompConstToX86reg(Map_TempReg(Section,x86_Any,MappedReg,TRUE),Section->MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(Section->MipsRegHi(MappedReg),(int)Section->MipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(Section->MipsRegHi(MappedReg),Section->MipsRegHi(ConstReg));
				}			
				if (Section->Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Cont.BranchLabel,0);
					Section->Cont.LinkLocation = RecompPos - 4;
				}
				if (Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			} else {
				if (Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(Section->MipsRegLo(MappedReg),Section->MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		}
	} else if (Section->IsKnown(Opcode.rs) || Section->IsKnown(Opcode.rt)) {
		DWORD KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (Section->IsConst(KnownReg)) {
			if (Section->Is64Bit(KnownReg)) {
				CompConstToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (Section->IsSigned(KnownReg)) {
				CompConstToVariable((int)Section->MipsRegLo(KnownReg) >> 31,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			ProtectGPR(Section,KnownReg);
			if (Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (Section->IsSigned(KnownReg)) {
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (Section->Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Cont.BranchLabel,0);
			Section->Cont.LinkLocation = RecompPos - 4;
		}
		if (Section->IsConst(KnownReg)) {
			CompConstToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (Section->Cont.FallThrough) {
			JeLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else if (Section->Jump.FallThrough) {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
		} else {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	} else {
		int x86Reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		if (Section->Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Cont.BranchLabel,0);
			Section->Cont.LinkLocation = RecompPos - 4;
		}
		CompX86regToVariable(Map_TempReg(Section,x86Reg,Opcode.rs,FALSE),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		if (Section->Cont.FallThrough) {
			JeLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else if (Section->Jump.FallThrough) {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
		} else {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	}
}

void BGTZ_Compare (CBlockSection * Section) {
	if (Section->IsConst(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			if (Section->MipsReg_S(Opcode.rs) > 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			if (Section->MipsRegLo_S(Opcode.rs) > 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		}
	} else if (Section->IsMapped(Opcode.rs) && Section->Is32Bit(Opcode.rs)) {
		CompConstToX86reg(Section->MipsRegLo(Opcode.rs),0);
		if (Section->Jump.FallThrough) {
			JleLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		} else if (Section->Cont.FallThrough) {
			JgLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JleLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	} else {
		BYTE *Jump;

		if (Section->IsMapped(Opcode.rs)) {
			CompConstToX86reg(Section->MipsRegHi(Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		}
		if (Section->Jump.FallThrough) {
			JlLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JgLabel8("continue",0);
			Jump = RecompPos - 1;
		} else if (Section->Cont.FallThrough) {
			JlLabel8("continue",0);
			Jump = RecompPos - 1;
			JgLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JgLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}

		if (Section->IsMapped(Opcode.rs)) {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		}
		if (Section->Jump.FallThrough) {
			JeLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		} else if (Section->Cont.FallThrough) {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		} else {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
			JmpLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
		}
	}
}

void BLEZ_Compare (CBlockSection * Section) {
	if (Section->IsConst(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			if (Section->MipsReg_S(Opcode.rs) <= 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (Section->IsSigned(Opcode.rs)) {
			if (Section->MipsRegLo_S(Opcode.rs) <= 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			if (Section->MipsRegLo(Opcode.rs) == 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		}
	} else {
		if (Section->IsMapped(Opcode.rs) && Section->Is32Bit(Opcode.rs)) {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),0);
			if (Section->Jump.FallThrough) {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else if (Section->Cont.FallThrough) {
				JleLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else {
			BYTE *Jump;

			if (Section->IsMapped(Opcode.rs)) {
				CompConstToX86reg(Section->MipsRegHi(Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
			}
			if (Section->Jump.FallThrough) {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JlLabel8("Continue",0);
				Jump = RecompPos - 1;
			} else if (Section->Cont.FallThrough) {
				JgLabel8("Continue",0);
				Jump = RecompPos - 1;
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}

			if (Section->IsMapped(Opcode.rs)) {
				CompConstToX86reg(Section->MipsRegLo(Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
			}
			if (Section->Jump.FallThrough) {
				JneLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation2 = RecompPos - 4;
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			} else if (Section->Cont.FallThrough) {
				JeLabel32 (Section->Jump.BranchLabel, 0 );
				Section->Jump.LinkLocation2 = RecompPos - 4;
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			} else {
				JneLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation2 = RecompPos - 4;
				JmpLabel32("BranchToJump",0);
				Section->Jump.LinkLocation2 = RecompPos - 4;
			}
		}
	}
}

void BLTZ_Compare (CBlockSection * Section) {
	if (Section->IsConst(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			if (Section->MipsReg_S(Opcode.rs) < 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (Section->IsSigned(Opcode.rs)) {
			if (Section->MipsRegLo_S(Opcode.rs) < 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			Section->Jump.FallThrough = FALSE;
			Section->Cont.FallThrough = TRUE;
		}
	} else if (Section->IsMapped(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			CompConstToX86reg(Section->MipsRegHi(Opcode.rs),0);
			if (Section->Jump.FallThrough) {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else if (Section->Cont.FallThrough) {
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else if (Section->IsSigned(Opcode.rs)) {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),0);
			if (Section->Jump.FallThrough) {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else if (Section->Cont.FallThrough) {
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else {
			Section->Jump.FallThrough = FALSE;
			Section->Cont.FallThrough = TRUE;
		}
	} else if (Section->IsUnknown(Opcode.rs)) {
		CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		if (Section->Jump.FallThrough) {
			JgeLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		} else if (Section->Cont.FallThrough) {
			JlLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
			JmpLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		}
	}
}

void BGEZ_Compare (CBlockSection * Section) {
	if (Section->IsConst(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
			DisplayError("BGEZ 1");
#endif
			Compile_R4300i_UnknownOpcode(Section);
		} else if (Section->IsSigned(Opcode.rs)) {
			if (Section->MipsRegLo_S(Opcode.rs) >= 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			Section->Jump.FallThrough = TRUE;
			Section->Cont.FallThrough = FALSE;
		}
	} else if (Section->IsMapped(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) { 
			CompConstToX86reg(Section->MipsRegHi(Opcode.rs),0);
			if (Section->Cont.FallThrough) {
				JgeLabel32 ( Section->Jump.BranchLabel, 0 );
				Section->Jump.LinkLocation = RecompPos - 4;
			} else if (Section->Jump.FallThrough) {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else if (Section->IsSigned(Opcode.rs)) {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),0);
			if (Section->Cont.FallThrough) {
				JgeLabel32 ( Section->Jump.BranchLabel, 0 );
				Section->Jump.LinkLocation = RecompPos - 4;
			} else if (Section->Jump.FallThrough) {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else { 
			Section->Jump.FallThrough = TRUE;
			Section->Cont.FallThrough = FALSE;
		}
	} else {
		CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);		
		if (Section->Cont.FallThrough) {
			JgeLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation = RecompPos - 4;
		} else if (Section->Jump.FallThrough) {
			JlLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	}
}

void COP1_BCF_Compare (CBlockSection * Section) {
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (Section->Cont.FallThrough) {
		JeLabel32 ( Section->Jump.BranchLabel, 0 );
		Section->Jump.LinkLocation = RecompPos - 4;
	} else if (Section->Jump.FallThrough) {
		JneLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
	} else {
		JneLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
		JmpLabel32(Section->Jump.BranchLabel,0);
		Section->Jump.LinkLocation = RecompPos - 4;
	}
}

void COP1_BCT_Compare (CBlockSection * Section) {
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (Section->Cont.FallThrough) {
		JneLabel32 ( Section->Jump.BranchLabel, 0 );
		Section->Jump.LinkLocation = RecompPos - 4;
	} else if (Section->Jump.FallThrough) {
		JeLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
	} else {
		JeLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
		JmpLabel32(Section->Jump.BranchLabel,0);
		Section->Jump.LinkLocation = RecompPos - 4;
	}
}
/*************************  OpCode functions *************************/
void Compile_R4300i_J (CBlockSection * Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CBlockSection *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		Section->Jump.TargetPC      = (Section->CompilePC & 0xF0000000) + (Opcode.target << 2);;
		Section->Jump.BranchLabel   = JumpLabel;
		Section->Jump.FallThrough   = TRUE;
		Section->Jump.LinkLocation  = NULL;
		Section->Jump.LinkLocation2 = NULL;
		NextInstruction = DO_DELAY_SLOT;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
			_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
			NextInstruction = END_BLOCK;
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_JAL (CBlockSection * Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		UnMap_GPR(Section, 31, FALSE);
		Section->MipsRegLo(31) = Section->CompilePC + 8;
		Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			MoveConstToVariable((Section->CompilePC & 0xF0000000) + (Opcode.target << 2),&JumpToLocation,"JumpToLocation");
			MoveConstToVariable(Section->CompilePC + 4,_PROGRAM_COUNTER,"_PROGRAM_COUNTER");
			_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
			WriteBackRegisters(Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		DWORD TargetPC = (Section->CompilePC & 0xF0000000) + (Opcode.target << 2);
		_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC,TargetPC,Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
	return;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

		UnMap_GPR(Section, 31, FALSE);
		Section->MipsRegLo(31) = Section->CompilePC + 8;
		Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		NextInstruction = DO_DELAY_SLOT;
		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CBlockSection *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		Section->Jump.TargetPC      = (Section->CompilePC & 0xF0000000) + (Opcode.target << 2);
		Section->Jump.BranchLabel   = JumpLabel;
		Section->Jump.FallThrough   = TRUE;
		Section->Jump.LinkLocation  = NULL;
		Section->Jump.LinkLocation2 = NULL;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
			_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
			NextInstruction = END_BLOCK;
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_ADDI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) { return; }

	if (SPHack && Opcode.rs == 29 && Opcode.rt == 29) {
		AddConstToX86Reg(Map_MemoryStack(Section, x86_Any, true),(short)Opcode.immediate);
	}

	if (Section->IsConst(Opcode.rs)) { 
		if (Section->IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		Section->MipsRegLo(Opcode.rt) = Section->MipsRegLo(Opcode.rs) + (short)Opcode.immediate;
		Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rs);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(Section->MipsRegLo(Opcode.rt));
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(Section->MipsRegLo(Opcode.rt));
		} else {
			AddConstToX86Reg(Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
		}
	}
	if (SPHack && Opcode.rt == 29 && Opcode.rs != 29) { 
		Section->ResetX86Protection();
		_MMU->ResetMemoryStack(Section); 
	}

}

void Compile_R4300i_ADDIU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) { return; }

	if (SPHack)
	{
		if (Opcode.rs == 29 && Opcode.rt == 29) 
		{
			AddConstToX86Reg(Map_MemoryStack(Section, x86_Any, TRUE),(short)Opcode.immediate);
		}
	}

	if (Section->IsConst(Opcode.rs)) { 
		if (Section->IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		Section->MipsRegLo(Opcode.rt) = Section->MipsRegLo(Opcode.rs) + (short)Opcode.immediate;
		Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rs);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(Section->MipsRegLo(Opcode.rt));
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(Section->MipsRegLo(Opcode.rt));
		} else {
			AddConstToX86Reg(Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
		}
	}
	if (SPHack && Opcode.rt == 29 && Opcode.rs != 29) { 
		Section->ResetX86Protection();
		_MMU->ResetMemoryStack(Section); 
	}
}

void Compile_R4300i_SLTIU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return; }

	if (Section->IsConst(Opcode.rs)) { 
		DWORD Result;

		if (Section->Is64Bit(Opcode.rs)) {
			_int64 Immediate = (_int64)((short)Opcode.immediate);
			Result = Section->MipsReg(Opcode.rs) < ((unsigned)(Immediate))?1:0;
		} else if (Section->Is32Bit(Opcode.rs)) {
			Result = Section->MipsRegLo(Opcode.rs) < ((unsigned)((short)Opcode.immediate))?1:0;
		}
		UnMap_GPR(Section,Opcode.rt, FALSE);
		Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
		Section->MipsRegLo(Opcode.rt) = Result;
	} else if (Section->IsMapped(Opcode.rs)) { 
		if (Section->Is64Bit(Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(Section->MipsRegHi(Opcode.rs),((short)Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = RecompPos - 1;
			SetbVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
		} else {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
		}
	} else {
		BYTE * Jump;

		CompConstToVariable(((short)Opcode.immediate >> 31),&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		JneLabel8("CompareSet",0);
		Jump = RecompPos - 1;
		CompConstToVariable((short)Opcode.immediate,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		CPU_Message("");
		CPU_Message("      CompareSet:");
		*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		SetbVariable(&BranchCompare,"BranchCompare");
		Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
		
		
		/*SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompConstToVariable((short)Opcode.immediate,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));*/
	}
}

void Compile_R4300i_SLTI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return; }

	if (Section->IsConst(Opcode.rs)) { 
		DWORD Result;

		if (Section->Is64Bit(Opcode.rs)) {
			_int64 Immediate = (_int64)((short)Opcode.immediate);
			Result = (_int64)Section->MipsReg(Opcode.rs) < Immediate?1:0;
		} else if (Section->Is32Bit(Opcode.rs)) {
			Result = Section->MipsRegLo_S(Opcode.rs) < (short)Opcode.immediate?1:0;
		}
		UnMap_GPR(Section,Opcode.rt, FALSE);
		Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
		Section->MipsRegLo(Opcode.rt) = Result;
	} else if (Section->IsMapped(Opcode.rs)) { 
		if (Section->Is64Bit(Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(Section->MipsRegHi(Opcode.rs),((short)Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = RecompPos - 1;
			SetlVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
		} else {
		/*	CompConstToX86reg(Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetlVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
			*/
			ProtectGPR(Section, Opcode.rs);
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			CompConstToX86reg(Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			
			if (Section->MipsRegLo(Opcode.rt) > x86_EDX) {
				SetlVariable(&BranchCompare,"BranchCompare");
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
			} else {
				Setl(Section->MipsRegLo(Opcode.rt));
				AndConstToX86Reg(Section->MipsRegLo(Opcode.rt), 1);
			}
		}
	} else {
		BYTE * Jump[2];

		CompConstToVariable(((short)Opcode.immediate >> 31),&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompConstToVariable((short)Opcode.immediate,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rt));
	}
}

void Compile_R4300i_ANDI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) { return;}

	if (Section->IsConst(Opcode.rs)) {
		if (Section->IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
		Section->MipsRegLo(Opcode.rt) = Section->MipsRegLo(Opcode.rs) & Opcode.immediate;
	} else if (Opcode.immediate != 0) { 
		Map_GPR_32bit(Section,Opcode.rt,FALSE,Opcode.rs);
		AndConstToX86Reg(Section->MipsRegLo(Opcode.rt),Opcode.immediate);
	} else {
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
	}
}

void Compile_R4300i_ORI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (Section->IsConst(Opcode.rs)) {
		if (Section->IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		Section->MipsRegState(Opcode.rt) = Section->MipsRegState(Opcode.rs);
		Section->MipsRegHi(Opcode.rt) = Section->MipsRegHi(Opcode.rs);
		Section->MipsRegLo(Opcode.rt) = Section->MipsRegLo(Opcode.rs) | Opcode.immediate;
	} else if (Section->IsMapped(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			Map_GPR_64bit(Section,Opcode.rt,Opcode.rs);
		} else {
			Map_GPR_32bit(Section,Opcode.rt,Section->IsSigned(Opcode.rs),Opcode.rs);
		}
		OrConstToX86Reg(Opcode.immediate,Section->MipsRegLo(Opcode.rt));
	} else {
		Map_GPR_64bit(Section,Opcode.rt,Opcode.rs);
		OrConstToX86Reg(Opcode.immediate,Section->MipsRegLo(Opcode.rt));
	}
}

void Compile_R4300i_XORI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (Section->IsConst(Opcode.rs)) {
		if (Opcode.rs != Opcode.rt) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		Section->MipsRegState(Opcode.rt) = Section->MipsRegState(Opcode.rs);
		Section->MipsRegHi(Opcode.rt) = Section->MipsRegHi(Opcode.rs);
		Section->MipsRegLo(Opcode.rt) = Section->MipsRegLo(Opcode.rs) ^ Opcode.immediate;
	} else {
		if (Section->IsMapped(Opcode.rs) && Section->Is32Bit(Opcode.rs)) {
			Map_GPR_32bit(Section,Opcode.rt,Section->IsSigned(Opcode.rs),Opcode.rs);
		} else {
			Map_GPR_64bit(Section,Opcode.rt,Opcode.rs);
		}
		if (Opcode.immediate != 0) { XorConstToX86Reg(Section->MipsRegLo(Opcode.rt),Opcode.immediate); }
	}
}

void Compile_R4300i_LUI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && Opcode.rt == 29) {
		int x86reg = Map_MemoryStack(Section, x86_Any, false);
		DWORD Address;

		TranslateVaddr (((short)Opcode.offset << 16), &Address);
		if (x86reg < 0) {
			MoveConstToVariable((DWORD)(Address + RDRAM), g_MemoryStack, "MemoryStack");
		} else {
			MoveConstToX86reg((DWORD)(Address + RDRAM), x86reg);
		}
	}
#endif
	UnMap_GPR(Section,Opcode.rt, FALSE);
	Section->MipsRegLo(Opcode.rt) = ((short)Opcode.offset << 16);
	Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
}

void Compile_R4300i_DADDIU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rs,TRUE); }
	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::DADDIU, "R4300iOp::DADDIU");
	Popad();
}

void Compile_R4300i_LDL (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::LDL, "R4300iOp::LDL");
	Popad();

}

void Compile_R4300i_LDR (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::LDR, "R4300iOp::LDR");
	Popad();
}


void Compile_R4300i_LB (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(Section,Opcode.rt,TRUE,0);
		_MMU->Compile_LB(Section->MipsRegLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxByteX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regByte(Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LH (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(Section,Opcode.rt,TRUE,0);
		_MMU->Compile_LH(Section->MipsRegLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxHalfX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regHalf(Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LWL (CBlockSection * Section) {
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address, Value;
		
		Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, Value,(Address & ~3));
		AndConstToX86Reg(Section->MipsRegLo(Opcode.rt),LWL_MASK[Offset]);
		ShiftLeftSignImmed(Value,(BYTE)LWL_SHIFT[Offset]);
		AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
	}
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg(LWL_MASK,"LWL_MASK",Section->MipsRegLo(Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWL_SHIFT,"LWL_SHIFT",shift,Offset,4);
	if (UseTlb) {			
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftLeftSign(TempReg1);
	AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),TempReg1);
}

void Compile_R4300i_LW (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Opcode.base == 29 && SPHack) {
		char String[100];

		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		TempReg1 = Map_MemoryStack(Section,x86_Any,true);
		sprintf(String,"%Xh",(short)Opcode.offset);
		MoveVariableDispToX86Reg((void *)((DWORD)(short)Opcode.offset),String,Section->MipsRegLo(Opcode.rt),TempReg1,1);
	} else {
		if (Section->IsConst(Opcode.base)) { 
			DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
			_MMU->Compile_LW(Section, Section->MipsRegLo(Opcode.rt),Address);
		} else {
			if (UseTlb) {	
				if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
				if (Section->IsMapped(Opcode.base) && Opcode.offset == 0) { 
					ProtectGPR(Section,Opcode.base);
					TempReg1 = Section->MipsRegLo(Opcode.base);
				} else {
					if (Section->IsMapped(Opcode.base)) { 
						ProtectGPR(Section,Opcode.base);
						if (Opcode.offset != 0) {
							TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
							LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
						} else {
							TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
						}
					} else {
						TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
						if (Opcode.immediate == 0) { 
						} else if (Opcode.immediate == 1) {
							IncX86reg(TempReg1);
						} else if (Opcode.immediate == 0xFFFF) {			
							DecX86reg(TempReg1);
						} else {
							AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
						}
					}
				}
				TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
				MoveX86RegToX86Reg(TempReg1, TempReg2);
				ShiftRightUnsignImmed(TempReg2,12);
				MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
				CompileReadTLBMiss(Section,TempReg1,TempReg2);
				Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
				MoveX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
			} else {
				if (Section->IsMapped(Opcode.base)) { 
					ProtectGPR(Section,Opcode.base);
					if (Opcode.offset != 0) {
						Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
						LeaSourceAndOffset(Section->MipsRegLo(Opcode.rt),Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
					} else {
						Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
					}
				} else {
					Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
					if (Opcode.immediate == 0) { 
					} else if (Opcode.immediate == 1) {
						IncX86reg(Section->MipsRegLo(Opcode.rt));
					} else if (Opcode.immediate == 0xFFFF) {			
						DecX86reg(Section->MipsRegLo(Opcode.rt));
					} else {
						AddConstToX86Reg(Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
					}
				}
				AndConstToX86Reg(Section->MipsRegLo(Opcode.rt),0x1FFFFFFF);
				MoveN64MemToX86reg(Section->MipsRegLo(Opcode.rt),Section->MipsRegLo(Opcode.rt));
			}
		}
	}
	if (SPHack && Opcode.rt == 29)
	{ 
		Section->ResetX86Protection();
		_MMU->ResetMemoryStack(Section); 
	}
}

void Compile_R4300i_LBU (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
		_MMU->Compile_LB(Section->MipsRegLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxByteX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxN64MemToX86regByte(Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LHU (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
		_MMU->Compile_LH(Section->MipsRegLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LWR (CBlockSection * Section) {
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address, Value;
		
		Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, Value,(Address & ~3));
		AndConstToX86Reg(Section->MipsRegLo(Opcode.rt),LWR_MASK[Offset]);
		ShiftRightUnsignImmed(Value,(BYTE)LWR_SHIFT[Offset]);
		AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
	}
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg(LWR_MASK,"LWR_MASK",Section->MipsRegLo(Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWR_SHIFT,"LWR_SHIFT",shift,Offset,4);
	if (UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftRightUnsign(TempReg1);
	AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),TempReg1);
}

void Compile_R4300i_LWU (CBlockSection * Section) {			//added by Witten
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
		_MMU->Compile_LW(Section, Section->MipsRegLo(Opcode.rt),Address);
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_SB (CBlockSection * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		
		if (Section->IsConst(Opcode.rt)) {
			_MMU->Compile_SB_Const((BYTE)Section->MipsRegLo(Opcode.rt), Address);
		} else if (Section->IsMapped(Opcode.rt) && Is8BitReg(Section->MipsRegLo(Opcode.rt))) {
			_MMU->Compile_SB_Register(Section->MipsRegLo(Opcode.rt), Address);
		} else {
			_MMU->Compile_SB_Register(Map_TempReg(Section,x86_Any8Bit,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
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

		XorConstToX86Reg(TempReg1,3);	
		if (Section->IsConst(Opcode.rt)) {
			MoveConstByteToX86regPointer((BYTE)Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (Section->IsMapped(Opcode.rt) && Is8BitReg(Section->MipsRegLo(Opcode.rt))) {
			MoveX86regByteToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			UnProtectGPR(Section,Opcode.rt);
			MoveX86regByteToX86regPointer(Map_TempReg(Section,x86_Any8Bit,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		if (Section->IsConst(Opcode.rt)) {
			MoveConstByteToN64Mem((BYTE)Section->MipsRegLo(Opcode.rt),TempReg1);
		} else if (Section->IsMapped(Opcode.rt) && Is8BitReg(Section->MipsRegLo(Opcode.rt))) {
			MoveX86regByteToN64Mem(Section->MipsRegLo(Opcode.rt),TempReg1);
		} else {	
			UnProtectGPR(Section,Opcode.rt);
			MoveX86regByteToN64Mem(Map_TempReg(Section,x86_Any8Bit,Opcode.rt,FALSE),TempReg1);
		}
	}

}

void Compile_R4300i_SH (CBlockSection * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = (Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		
		if (Section->IsConst(Opcode.rt)) {
			_MMU->Compile_SH_Const((WORD)Section->MipsRegLo(Opcode.rt), Address);
		} else if (Section->IsMapped(Opcode.rt)) {
			_MMU->Compile_SH_Register(Section->MipsRegLo(Opcode.rt), Address);
		} else {
			_MMU->Compile_SH_Register(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
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

		XorConstToX86Reg(TempReg1,2);	
		if (Section->IsConst(Opcode.rt)) {
			MoveConstHalfToX86regPointer((WORD)Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regHalfToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regHalfToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		XorConstToX86Reg(TempReg1,2);
		if (Section->IsConst(Opcode.rt)) {
			MoveConstHalfToN64Mem((WORD)Section->MipsRegLo(Opcode.rt),TempReg1);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regHalfToN64Mem(Section->MipsRegLo(Opcode.rt),TempReg1);		
		} else {	
			MoveX86regHalfToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1);		
		}
	}
}

void Compile_R4300i_SWL (CBlockSection * Section) {
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;
		
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, Value,(Address & ~3));
		AndConstToX86Reg(Value,SWL_MASK[Offset]);
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
		ShiftRightUnsignImmed(TempReg1,(BYTE)SWL_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		_MMU->Compile_SW_Register(Section,Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}		
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = Map_TempReg(Section,x86_Any,-1,FALSE);
	if (UseTlb) {	
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWL_MASK,"SWL_MASK",Value,Offset,4);
	if (!Section->IsConst(Opcode.rt) || Section->MipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWL_SHIFT,"SWL_SHIFT",shift,Offset,4);
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToX86reg(Section->MipsRegLo(Opcode.rt),Offset);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),Offset);
		} else {
			MoveVariableToX86reg(&_GPR[Opcode.rt].UW[0],CRegName::GPR_Lo[Opcode.rt],Offset);
		}
		ShiftRightUnsign(Offset);
		AddX86RegToX86Reg(Value,Offset);
	}

	if (UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);

		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
}

void Compile_R4300i_SW (CBlockSection * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (Opcode.base == 29 && SPHack) {
		if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
		TempReg1 = Map_MemoryStack(Section,x86_Any,true);

		if (Section->IsConst(Opcode.rt)) {
			MoveConstToMemoryDisp (Section->MipsRegLo(Opcode.rt),TempReg1, (DWORD)((short)Opcode.offset));
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToMemory(Section->MipsRegLo(Opcode.rt),TempReg1,(DWORD)((short)Opcode.offset));
		} else {	
			TempReg2 = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			MoveX86regToMemory(TempReg2,TempReg1,(DWORD)((short)Opcode.offset));
		}		
	} else {
		if (Section->IsConst(Opcode.base)) { 
			DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			
			if (Section->IsConst(Opcode.rt)) {
				_MMU->Compile_SW_Const(Section->MipsRegLo(Opcode.rt), Address);
			} else if (Section->IsMapped(Opcode.rt)) {
				_MMU->Compile_SW_Register(Section,Section->MipsRegLo(Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Register(Section,Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), Address);
			}
			return;
		}
		if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
		if (Section->IsMapped(Opcode.base)) { 
			ProtectGPR(Section,Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			}
			UnProtectGPR(Section,Opcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
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

			if (Section->IsConst(Opcode.rt)) {
				MoveConstToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else if (Section->IsMapped(Opcode.rt)) {
				MoveX86regToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else {	
				MoveX86regToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
			}
		} else {
			AndConstToX86Reg(TempReg1,0x1FFFFFFF);
			if (Section->IsConst(Opcode.rt)) {
				MoveConstToN64Mem(Section->MipsRegLo(Opcode.rt),TempReg1);
			} else if (Section->IsMapped(Opcode.rt)) {
				MoveX86regToN64Mem(Section->MipsRegLo(Opcode.rt),TempReg1);
			} else {	
				MoveX86regToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1);
			}
		}
	}
}

void Compile_R4300i_SWR (CBlockSection * Section) {
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;
		
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, Value,(Address & ~3));
		AndConstToX86Reg(Value,SWR_MASK[Offset]);
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
		ShiftLeftSignImmed(TempReg1,(BYTE)SWR_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		_MMU->Compile_SW_Register(Section,Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}		
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = Map_TempReg(Section,x86_Any,-1,FALSE);
	if (UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWR_MASK,"SWR_MASK",Value,Offset,4);
	if (!Section->IsConst(Opcode.rt) || Section->MipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWR_SHIFT,"SWR_SHIFT",shift,Offset,4);
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToX86reg(Section->MipsRegLo(Opcode.rt),Offset);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),Offset);
		} else {
			MoveVariableToX86reg(&_GPR[Opcode.rt].UW[0],CRegName::GPR_Lo[Opcode.rt],Offset);
		}
		ShiftLeftSign(Offset);
		AddX86RegToX86Reg(Value,Offset);
	}

	if (UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);

		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
}

void Compile_R4300i_SDL (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SDL, "R4300iOp::SDL");
	Popad();

}

void Compile_R4300i_SDR (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SDR, "R4300iOp::SDR");
	Popad();

}

void Compile_R4300i_CACHE (CBlockSection * Section){
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (_Settings->LoadDword(Game_SMM_Cache) == 0)
	{
		return;
	}

	switch(Opcode.rt) {
	case 0:
	case 16:
		Pushad();
		PushImm32("CRecompiler::Remove_Cache",CRecompiler::Remove_Cache);
		PushImm32("20",20);
		if (Section->IsConst(Opcode.base)) { 
			DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			PushImm32("Address",Address);
		} else if (Section->IsMapped(Opcode.base)) { 
			AddConstToX86Reg(Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			Push(Section->MipsRegLo(Opcode.base));
		} else {
			MoveVariableToX86reg(&_GPR[Opcode.base].UW[0],CRegName::GPR_Lo[Opcode.base],x86_EAX);
			AddConstToX86Reg(x86_EAX,(short)Opcode.offset);
			Push(x86_EAX);
		}
		MoveConstToX86reg((DWORD)_N64System->GetRecompiler(),x86_ECX);		
		Call_Direct(AddressOf(CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
		Popad();
		break;
	case 1:
	case 3:
	case 13:
	case 5:
	case 8:
	case 9:
	case 17:
	case 21:
	case 25:
		break;
#ifndef EXTERNAL_RELEASE
	default:
		DisplayError("cache: %d",Opcode.rt);
#endif
	}
}

void Compile_R4300i_LL (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		_MMU->Compile_LW(Section, Section->MipsRegLo(Opcode.rt),Address);
		MoveConstToVariable(1,_LLBit,"LLBit");
		
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		TranslateVaddr(Address, &Address);
#endif
		MoveConstToVariable(Address,_LLAddr,"LLAddr");
		return;
	}
	if (UseTlb) {	
		if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
		if (Section->IsMapped(Opcode.base) && Opcode.offset == 0) { 
			ProtectGPR(Section,Opcode.base);
			TempReg1 = Section->MipsRegLo(Opcode.base);
		} else {
			if (Section->IsMapped(Opcode.base)) { 
				ProtectGPR(Section,Opcode.base);
				if (Opcode.offset != 0) {
					TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
					LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
				} else {
					TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
				}
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
				if (Opcode.immediate == 0) { 
				} else if (Opcode.immediate == 1) {
					IncX86reg(TempReg1);
				} else if (Opcode.immediate == 0xFFFF) {			
					DecX86reg(TempReg1);
				} else {
					AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
				}
			}
		}
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
		MoveX86regToVariable(TempReg1,_LLAddr,"LLAddr");
		AddX86regToVariable(TempReg2,_LLAddr,"LLAddr");
		SubConstFromVariable((DWORD)RDRAM,_LLAddr,"LLAddr");
	} else {
		if (Section->IsMapped(Opcode.base)) { 
			ProtectGPR(Section,Opcode.base);
			if (Opcode.offset != 0) {
				Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
				LeaSourceAndOffset(Section->MipsRegLo(Opcode.rt),Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
			}
		} else {
			Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(Section->MipsRegLo(Opcode.rt));
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(Section->MipsRegLo(Opcode.rt));
			} else {
				AddConstToX86Reg(Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
			}
		}
		AndConstToX86Reg(Section->MipsRegLo(Opcode.rt),0x1FFFFFFF);
		MoveX86regToVariable(Section->MipsRegLo(Opcode.rt),_LLAddr,"LLAddr");
		MoveN64MemToX86reg(Section->MipsRegLo(Opcode.rt),Section->MipsRegLo(Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
	}
}

void Compile_R4300i_SC (CBlockSection * Section){
	DWORD TempReg1, TempReg2;
	BYTE * Jump;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	CompConstToVariable(1,_LLBit,"LLBit");
	JneLabel32("LLBitNotSet",0);
	Jump = RecompPos - 4;
	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			
		if (Section->IsConst(Opcode.rt)) {
			_MMU->Compile_SW_Const(Section->MipsRegLo(Opcode.rt), Address);
		} else if (Section->IsMapped(Opcode.rt)) {
			_MMU->Compile_SW_Register(Section,Section->MipsRegLo(Opcode.rt), Address);
		} else {
			_MMU->Compile_SW_Register(Section,Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), Address);
		}
		CPU_Message("      LLBitNotSet:");
		*((DWORD *)(Jump))=(BYTE)(RecompPos - Jump - 4);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveVariableToX86reg(_LLBit,"LLBit",Section->MipsRegLo(Opcode.rt));
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
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

		if (Section->IsConst(Opcode.rt)) {
			MoveConstToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToN64Mem(Section->MipsRegLo(Opcode.rt),TempReg1);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToN64Mem(Section->MipsRegLo(Opcode.rt),TempReg1);
		} else {	
			MoveX86regToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1);
		}
	}
	CPU_Message("      LLBitNotSet:");
	*((DWORD *)(Jump))=(BYTE)(RecompPos - Jump - 4);
	Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
	MoveVariableToX86reg(_LLBit,"LLBit",Section->MipsRegLo(Opcode.rt));

}

void Compile_R4300i_LD (CBlockSection * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_64bit(Section,Opcode.rt,-1);
		_MMU->Compile_LW(Section, Section->MipsRegHi(Opcode.rt),Address);
		_MMU->Compile_LW(Section, Section->MipsRegLo(Opcode.rt),Address + 4);
		if (SPHack && Opcode.rt == 29) { _MMU->ResetMemoryStack(Section); }
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base) && Opcode.offset == 0) { 
		if (UseTlb) {
			ProtectGPR(Section,Opcode.base);
			TempReg1 = Section->MipsRegLo(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		if (Section->IsMapped(Opcode.base)) { 
			ProtectGPR(Section,Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			}
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
			}
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
		Map_GPR_64bit(Section,Opcode.rt,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Section->MipsRegHi(Opcode.rt));
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,Section->MipsRegLo(Opcode.rt),4);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_64bit(Section,Opcode.rt,-1);
		MoveN64MemToX86reg(Section->MipsRegHi(Opcode.rt),TempReg1);
		MoveN64MemDispToX86reg(Section->MipsRegLo(Opcode.rt),TempReg1,4);
	}
	if (SPHack && Opcode.rt == 29) { 		
		Section->ResetX86Protection();
		_MMU->ResetMemoryStack(Section); 
	}
}

void Compile_R4300i_SD (CBlockSection * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (Section->IsConst(Opcode.base)) { 
		DWORD Address = Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		
		if (Section->IsConst(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rt)) {
				_MMU->Compile_SW_Const(Section->MipsRegHi(Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Const((Section->MipsRegLo_S(Opcode.rt) >> 31), Address);
			}
			_MMU->Compile_SW_Const(Section->MipsRegLo(Opcode.rt), Address + 4);
		} else if (Section->IsMapped(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rt)) {
				_MMU->Compile_SW_Register(Section,Section->MipsRegHi(Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Register(Section,Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), Address);
			}
			_MMU->Compile_SW_Register(Section,Section->MipsRegLo(Opcode.rt), Address + 4);		
		} else {
			_MMU->Compile_SW_Register(Section,TempReg1 = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), Address);
			_MMU->Compile_SW_Register(Section,Map_TempReg(Section,TempReg1,Opcode.rt,FALSE), Address + 4);		
		}
		return;
	}
	if (Section->IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (Section->IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
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

		if (Section->IsConst(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rt)) {
				MoveConstToX86regPointer(Section->MipsRegHi(Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveConstToX86regPointer((Section->MipsRegLo_S(Opcode.rt) >> 31),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveConstToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (Section->IsMapped(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rt)) {
				MoveX86regToX86regPointer(Section->MipsRegHi(Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveX86regToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,TRUE),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			int X86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			MoveX86regToX86regPointer(X86Reg,TempReg1, TempReg2);
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(Map_TempReg(Section,X86Reg,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		if (Section->IsConst(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rt)) {
				MoveConstToN64Mem(Section->MipsRegHi(Opcode.rt),TempReg1);
			} else if (Section->IsSigned(Opcode.rt)) {
				MoveConstToN64Mem(((int)Section->MipsRegLo(Opcode.rt) >> 31),TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveConstToN64MemDisp(Section->MipsRegLo(Opcode.rt),TempReg1,4);
		} else if (Section->IsKnown(Opcode.rt) && Section->IsMapped(Opcode.rt)) {
			if (Section->Is64Bit(Opcode.rt)) {
				MoveX86regToN64Mem(Section->MipsRegHi(Opcode.rt),TempReg1);
			} else if (Section->IsSigned(Opcode.rt)) {
				MoveX86regToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveX86regToN64MemDisp(Section->MipsRegLo(Opcode.rt),TempReg1, 4);		
		} else {	
			int x86reg;
			MoveX86regToN64Mem(x86reg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), TempReg1);
			MoveX86regToN64MemDisp(Map_TempReg(Section,x86reg,Opcode.rt,FALSE), TempReg1,4);
		}
	}
}

/********************** R4300i OpCodes: Special **********************/
void Compile_R4300i_SPECIAL_SLL (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (Section->IsConst(Opcode.rt)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo(Opcode.rt) << Opcode.sa;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	if (Opcode.rd != Opcode.rt && Section->IsMapped(Opcode.rt)) {
		switch (Opcode.sa) {
		case 0: 
			Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
			break;
		case 1:
			ProtectGPR(Section,Opcode.rt);
			Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			LeaRegReg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt), 2);
			break;			
		case 2:
			ProtectGPR(Section,Opcode.rt);
			Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			LeaRegReg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt), 4);
			break;			
		case 3:
			ProtectGPR(Section,Opcode.rt);
			Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			LeaRegReg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt), 8);
			break;
		default:
			Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
			ShiftLeftSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	} else {
		Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftLeftSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
	}
}

void Compile_R4300i_SPECIAL_SRL (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsConst(Opcode.rt)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo(Opcode.rt) >> Opcode.sa;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightUnsignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_SRA (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsConst(Opcode.rt)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_SLLV (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsConst(Opcode.rs)) {
		DWORD Shift = (Section->MipsRegLo(Opcode.rs) & 0x1F);
		if (Section->IsConst(Opcode.rt)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo(Opcode.rt) << Shift;
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
			ShiftLeftSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Shift);
		}
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftLeftSign(Section->MipsRegLo(Opcode.rd));
}

void Compile_R4300i_SPECIAL_SRLV (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsKnown(Opcode.rs) && Section->IsConst(Opcode.rs)) {
		DWORD Shift = (Section->MipsRegLo(Opcode.rs) & 0x1F);
		if (Section->IsConst(Opcode.rt)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo(Opcode.rt) >> Shift;
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftRightUnsignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightUnsign(Section->MipsRegLo(Opcode.rd));
}

void Compile_R4300i_SPECIAL_SRAV (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsKnown(Opcode.rs) && Section->IsConst(Opcode.rs)) {
		DWORD Shift = (Section->MipsRegLo(Opcode.rs) & 0x1F);
		if (Section->IsConst(Opcode.rt)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo_S(Opcode.rt) >> Shift;
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftRightSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightSign(Section->MipsRegLo(Opcode.rd));
}

void Compile_R4300i_SPECIAL_JR (CBlockSection * Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		if (Section->IsConst(Opcode.rs)) { 
			sprintf(JumpLabel,"0x%08X",Section->MipsRegLo(Opcode.rs));
			Section->Jump.BranchLabel   = JumpLabel;
			Section->Jump.TargetPC      = Section->MipsRegLo(Opcode.rs);
			Section->Jump.FallThrough   = TRUE;
			Section->Jump.LinkLocation  = NULL;
			Section->Jump.LinkLocation2 = NULL;
			Section->Cont.FallThrough   = FALSE;
			Section->Cont.LinkLocation  = NULL;
			Section->Cont.LinkLocation2 = NULL;
			if ((Section->CompilePC & 0xFFC) == 0xFFC) {
				_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
				NextInstruction = END_BLOCK;
				return;
			}
		}
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			if (Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(Section->MipsRegLo(Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(Section->CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
			WriteBackRegisters(Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		if (DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0)) {
			if (Section->IsConst(Opcode.rs)) { 
				MoveConstToVariable(Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else 	if (Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0)) {
			_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC,(DWORD)-1,Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
		} else {
			if (Section->IsConst(Opcode.rs)) { 
				memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
				_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
			} else {
				if (Section->IsMapped(Opcode.rs)) { 
					MoveX86regToVariable(Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				} else {
					MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				}
				_N64System->GetRecompiler()->CompileExit (Section,-1, (DWORD)-1,Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
			}
		}
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_SPECIAL_JALR (CBlockSection * Section) {
	static char JumpLabel[100];	
	
	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		if (DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0)) {
			Compile_R4300i_UnknownOpcode(Section);
		}
		UnMap_GPR(Section, Opcode.rd, FALSE);
		Section->MipsRegLo(Opcode.rd) = Section->CompilePC + 8;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			if (Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(Section->MipsRegLo(Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(Section->CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
			WriteBackRegisters(Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (Section->IsConst(Opcode.rs)) { 
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(CRegInfo));
			sprintf(JumpLabel,"0x%08X",Section->MipsRegLo(Opcode.rs));
			Section->Jump.BranchLabel   = JumpLabel;
			Section->Jump.TargetPC      = Section->MipsRegLo(Opcode.rs);
			Section->Jump.FallThrough   = TRUE;
			Section->Jump.LinkLocation  = NULL;
			Section->Jump.LinkLocation2 = NULL;
			Section->Cont.FallThrough   = FALSE;
			Section->Cont.LinkLocation  = NULL;
			Section->Cont.LinkLocation2 = NULL;

			_N64System->GetRecompiler()->GenerateSectionLinkage(Section);
		} else {
			if (Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
			_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC, (DWORD)-1,Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
		}
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_SPECIAL_SYSCALL (CBlockSection * Section) {
	_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC,Section->CompilePC,Section->RegWorking,CExitInfo::DoSysCall,TRUE,NULL);
}

void Compile_R4300i_SPECIAL_MFLO (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	Map_GPR_64bit(Section,Opcode.rd,-1);
	MoveVariableToX86reg(&_RegLO->UW[0],"_RegLO->UW[0]",Section->MipsRegLo(Opcode.rd));
	MoveVariableToX86reg(&_RegLO->UW[1],"_RegLO->UW[1]",Section->MipsRegHi(Opcode.rd));
}

void Compile_R4300i_SPECIAL_MTLO (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsKnown(Opcode.rs) && Section->IsConst(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			MoveConstToVariable(Section->MipsRegHi(Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (Section->IsSigned(Opcode.rs) && ((Section->MipsRegLo(Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveConstToVariable(Section->MipsRegLo(Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else if (Section->IsKnown(Opcode.rs) && Section->IsMapped(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			MoveX86regToVariable(Section->MipsRegHi(Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (Section->IsSigned(Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,TRUE),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveX86regToVariable(Section->MipsRegLo(Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else {
		int x86reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&_RegLO->UW[1],"_RegLO->UW[1]");
		MoveX86regToVariable(Map_TempReg(Section,x86reg,Opcode.rs,FALSE), &_RegLO->UW[0],"_RegLO->UW[0]");
	}
}

void Compile_R4300i_SPECIAL_MFHI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	Map_GPR_64bit(Section,Opcode.rd,-1);
	MoveVariableToX86reg(&_RegHI->UW[0],"_RegHI->UW[0]",Section->MipsRegLo(Opcode.rd));
	MoveVariableToX86reg(&_RegHI->UW[1],"_RegHI->UW[1]",Section->MipsRegHi(Opcode.rd));
}

void Compile_R4300i_SPECIAL_MTHI (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Section->IsKnown(Opcode.rs) && Section->IsConst(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			MoveConstToVariable(Section->MipsRegHi(Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (Section->IsSigned(Opcode.rs) && ((Section->MipsRegLo(Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveConstToVariable(Section->MipsRegLo(Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else if (Section->IsKnown(Opcode.rs) && Section->IsMapped(Opcode.rs)) {
		if (Section->Is64Bit(Opcode.rs)) {
			MoveX86regToVariable(Section->MipsRegHi(Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (Section->IsSigned(Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,TRUE),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveX86regToVariable(Section->MipsRegLo(Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else {
		int x86reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&_RegHI->UW[1],"_RegHI->UW[1]");
		MoveX86regToVariable(Map_TempReg(Section,x86reg,Opcode.rs,FALSE), &_RegHI->UW[0],"_RegHI->UW[0]");
	}
}

void Compile_R4300i_SPECIAL_DSLLV (CBlockSection * Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsConst(Opcode.rs)) {
		DWORD Shift = (Section->MipsRegLo(Opcode.rs) & 0x3F);
		Compile_R4300i_UnknownOpcode(Section);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftLeftDouble(Section->MipsRegHi(Opcode.rd),Section->MipsRegLo(Opcode.rd));
	ShiftLeftSign(Section->MipsRegLo(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegHi(Opcode.rd));
	XorX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftLeftSign(Section->MipsRegHi(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void Compile_R4300i_SPECIAL_DSRLV (CBlockSection * Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsConst(Opcode.rs)) {
		DWORD Shift = (Section->MipsRegLo(Opcode.rs) & 0x3F);
		if (Section->IsConst(Opcode.rt)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			Section->MipsReg(Opcode.rd) = Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt);
			Section->MipsReg(Opcode.rd) = Section->MipsReg(Opcode.rd) >> Shift;
			if ((Section->MipsRegHi(Opcode.rd) == 0) && (Section->MipsRegLo(Opcode.rd) & 0x80000000) == 0) {
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else if ((Section->MipsRegHi(Opcode.rd) == 0xFFFFFFFF) && (Section->MipsRegLo(Opcode.rd) & 0x80000000) != 0) {
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else {
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
			}
			return;
		}
		//if (Shift < 0x20) {
		//} else {
		//}
		//Compile_R4300i_UnknownOpcode(Section);
		//return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftRightDouble(Section->MipsRegLo(Opcode.rd),Section->MipsRegHi(Opcode.rd));
	ShiftRightUnsign(Section->MipsRegHi(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegLo(Opcode.rd));
	XorX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightUnsign(Section->MipsRegLo(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void Compile_R4300i_SPECIAL_DSRAV (CBlockSection * Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (Section->IsConst(Opcode.rs)) {
		DWORD Shift = (Section->MipsRegLo(Opcode.rs) & 0x3F);
		Compile_R4300i_UnknownOpcode(Section);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftRightDouble(Section->MipsRegLo(Opcode.rd),Section->MipsRegHi(Opcode.rd));
	ShiftRightSign(Section->MipsRegHi(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegLo(Opcode.rd));
	ShiftRightSignImmed(Section->MipsRegHi(Opcode.rd),0x1F);
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightSign(Section->MipsRegLo(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void Compile_R4300i_SPECIAL_MULT ( CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	imulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
}

void Compile_R4300i_SPECIAL_MULTU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	MulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
}

void Compile_R4300i_SPECIAL_DIV (CBlockSection * Section) {
	BYTE *Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (Section->IsConst(Opcode.rt)) {
		if (Section->MipsRegLo(Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (Section->IsMapped(Opcode.rt)) {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rt),0);
		} else {
			CompConstToVariable(0, &_GPR[Opcode.rt].W[0], CRegName::GPR_Lo[Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = RecompPos - 1;

		MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
		MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
		MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
		MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	}
	/*	lo = (SD)rs / (SD)rt;
		hi = (SD)rs % (SD)rt; */

	Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);

	/* edx is the signed portion to eax */
	Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX, -1, FALSE);

	MoveX86RegToX86Reg(x86_EAX, x86_EDX);
	ShiftRightSignImmed(x86_EDX,31);

	if (Section->IsMapped(Opcode.rt)) {
		idivX86reg(Section->MipsRegLo(Opcode.rt));
	} else {
		idivX86reg(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE));
	}
		

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");

	if( Jump[1] != NULL ) {
		CPU_Message("");
		CPU_Message("      EndDivu:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
	}
}

void Compile_R4300i_SPECIAL_DIVU ( CBlockSection * Section) {
	BYTE *Jump[2];
	int x86reg;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsConst(Opcode.rt)) {
		if (Section->MipsRegLo(Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (Section->IsMapped(Opcode.rt)) {
			CompConstToX86reg(Section->MipsRegLo(Opcode.rt),0);
		} else {
			CompConstToVariable(0, &_GPR[Opcode.rt].W[0], CRegName::GPR_Lo[Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = RecompPos - 1;

		MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
		MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
		MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
		MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	}


	/*	lo = (UD)rs / (UD)rt;
		hi = (UD)rs % (UD)rt; */

	Section->x86Protected(x86_EAX) = TRUE;
	Map_TempReg(Section,x86_EDX, 0, FALSE);
	Section->x86Protected(x86_EAX) = FALSE;

	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	x86reg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);

	DivX86reg(x86reg);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");

	/* wouldnt these be zero (???) */

	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");

	if( Jump[1] != NULL ) {
		CPU_Message("");
		CPU_Message("      EndDivu:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
	}
}

void Compile_R4300i_SPECIAL_DMULT (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rs,TRUE); }
	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
	Popad();
}

void Compile_R4300i_SPECIAL_DMULTU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	/* _RegLO->UDW = (uint64)_GPR[Opcode.rs].UW[0] * (uint64)_GPR[Opcode.rt].UW[0]; */
	Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

	/* _RegHI->UDW = (uint64)_GPR[Opcode.rs].UW[1] * (uint64)_GPR[Opcode.rt].UW[1]; */
	Map_TempReg(Section,x86_EAX,Opcode.rs,TRUE);
	Map_TempReg(Section,x86_EDX,Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

	/* Tmp[0].UDW = (uint64)_GPR[Opcode.rs].UW[1] * (uint64)_GPR[Opcode.rt].UW[0]; */
	Map_TempReg(Section,x86_EAX,Opcode.rs,TRUE);
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	Map_TempReg(Section,x86_EBX,-1,FALSE);
	Map_TempReg(Section,x86_ECX,-1,FALSE);

	MulX86reg(x86_EDX);
	MoveX86RegToX86Reg(x86_EAX, x86_EBX); /* EDX:EAX -> ECX:EBX */
	MoveX86RegToX86Reg(x86_EDX, x86_ECX);

	/* Tmp[1].UDW = (uint64)_GPR[Opcode.rs].UW[0] * (uint64)_GPR[Opcode.rt].UW[1]; */
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	Map_TempReg(Section,x86_EDX,Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	Map_TempReg(Section,x86_ESI,-1,FALSE);
	Map_TempReg(Section,x86_EDI,-1,FALSE);
	MoveX86RegToX86Reg(x86_EAX, x86_ESI); /* EDX:EAX -> EDI:ESI */
	MoveX86RegToX86Reg(x86_EDX, x86_EDI);

	/* Tmp[2].UDW = (uint64)_RegLO->UW[1] + (uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]; */
	XorX86RegToX86Reg(x86_EDX, x86_EDX);
	MoveVariableToX86reg(&_RegLO->UW[1], "_RegLO->UW[1]", x86_EAX);
	AddX86RegToX86Reg(x86_EAX, x86_EBX);
	AddConstToX86Reg(x86_EDX, 0);
	AddX86RegToX86Reg(x86_EAX, x86_ESI);
	AddConstToX86Reg(x86_EDX, 0);			/* EDX:EAX */

	/* _RegLO->UDW += ((uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]) << 32; */
	/* [low+4] += ebx + esi */

	AddX86regToVariable(x86_EBX, &_RegLO->UW[1], "_RegLO->UW[1]");
	AddX86regToVariable(x86_ESI, &_RegLO->UW[1], "_RegLO->UW[1]");

	/* _RegHI->UDW += (uint64)Tmp[0].UW[1] + (uint64)Tmp[1].UW[1] + Tmp[2].UW[1]; */
	/* [hi] += ecx + edi + edx */
	
	AddX86regToVariable(x86_ECX, &_RegHI->UW[0], "_RegHI->UW[0]");
	AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

	AddX86regToVariable(x86_EDI, &_RegHI->UW[0], "_RegHI->UW[0]");
	AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

	AddX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
	AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
}

void Compile_R4300i_SPECIAL_DDIV (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	UnMap_GPR(Section,Opcode.rs,TRUE);
	UnMap_GPR(Section,Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
	Popad();
}

void Compile_R4300i_SPECIAL_DDIVU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	UnMap_GPR(Section,Opcode.rs,TRUE);
	UnMap_GPR(Section,Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
	Popad();
}

void Compile_R4300i_SPECIAL_ADD (CBlockSection * Section) {
	int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
	int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(source1) && Section->IsConst(source2)) {
		DWORD temp = Section->MipsRegLo(source1) + Section->MipsRegLo(source2);
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = temp;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(Section,Opcode.rd,TRUE, source1);
	if (Section->IsConst(source2)) {
		if (Section->MipsRegLo(source2) != 0) {
			AddConstToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
		}
	} else if (Section->IsKnown(source2) && Section->IsMapped(source2)) {
		AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
	} else {
		AddVariableToX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
}

void Compile_R4300i_SPECIAL_ADDU (CBlockSection * Section) {
	int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
	int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(source1) && Section->IsConst(source2)) {
		DWORD temp = Section->MipsRegLo(source1) + Section->MipsRegLo(source2);
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = temp;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(Section,Opcode.rd,TRUE, source1);
	if (Section->IsConst(source2)) {
		if (Section->MipsRegLo(source2) != 0) {
			AddConstToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
		}
	} else if (Section->IsKnown(source2) && Section->IsMapped(source2)) {
		AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
	} else {
		AddVariableToX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
}

void Compile_R4300i_SPECIAL_SUB (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(Opcode.rt)  && Section->IsConst(Opcode.rs)) {
		DWORD temp = Section->MipsRegLo(Opcode.rs) - Section->MipsRegLo(Opcode.rt);
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = temp;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (Opcode.rd == Opcode.rt) {
			int x86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
		if (Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
		} else if (Section->IsMapped(Opcode.rt)) {
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
		} else {
			SubVariableFromX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_SUBU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(Opcode.rt)  && Section->IsConst(Opcode.rs)) {
		DWORD temp = Section->MipsRegLo(Opcode.rs) - Section->MipsRegLo(Opcode.rt);
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegLo(Opcode.rd) = temp;
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (Opcode.rd == Opcode.rt) {
			int x86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
		if (Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
		} else if (Section->IsMapped(Opcode.rt)) {
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
		} else {
			SubVariableFromX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_AND (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Section->IsKnown(Opcode.rt) && Section->IsKnown(Opcode.rs)) {
		if (Section->IsConst(Opcode.rt) && Section->IsConst(Opcode.rs)) {
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				Section->MipsReg(Opcode.rd) = 
					(Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt)) &
					(Section->Is64Bit(Opcode.rs)?Section->MipsReg(Opcode.rs):(_int64)Section->MipsRegLo_S(Opcode.rs));
				
				if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
					Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
					Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				Section->MipsReg(Opcode.rd) = Section->MipsRegLo(Opcode.rt) & Section->MipsReg(Opcode.rs);
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			}			
		} else if (Section->IsMapped(Opcode.rt) && Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
		
			ProtectGPR(Section,source1);
			ProtectGPR(Section,source2);
			if (Section->Is32Bit(source1) && Section->Is32Bit(source2)) {
				int Sign = (Section->IsSigned(Opcode.rt) && Section->IsSigned(Opcode.rs))?TRUE:FALSE;
				Map_GPR_32bit(Section,Opcode.rd,Sign,source1);				
				AndX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			} else if (Section->Is32Bit(source1) || Section->Is32Bit(source2)) {
				if (Section->IsUnsigned(Section->Is32Bit(source1)?source1:source2)) {
					Map_GPR_32bit(Section,Opcode.rd,FALSE,source1);
					AndX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
				} else {
					Map_GPR_64bit(Section,Opcode.rd,source1);
					if (Section->Is32Bit(source2)) {
						AndX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
					} else {
						AndX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
					}
					AndX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
				}
			} else {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				AndX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
				AndX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			}
		} else {
			int ConstReg = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			int MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Section->Is64Bit(ConstReg)) {
				if (Section->Is32Bit(MappedReg) && Section->IsUnsigned(MappedReg)) {
					if (Section->MipsRegLo(ConstReg) == 0) {
						Map_GPR_32bit(Section,Opcode.rd,FALSE, 0);
					} else {
						DWORD Value = Section->MipsRegLo(ConstReg);
						Map_GPR_32bit(Section,Opcode.rd,FALSE, MappedReg);
						AndConstToX86Reg(Section->MipsRegLo(Opcode.rd),Value);
					}
				} else {
					_int64 Value = Section->MipsReg(ConstReg);
					Map_GPR_64bit(Section,Opcode.rd,MappedReg);
					AndConstToX86Reg(Section->MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
					AndConstToX86Reg(Section->MipsRegLo(Opcode.rd),(DWORD)Value);
				}
			} else if (Section->Is64Bit(MappedReg)) {
				DWORD Value = Section->MipsRegLo(ConstReg); 
				if (Value != 0) {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(ConstReg)?TRUE:FALSE,MappedReg);					
					AndConstToX86Reg(Section->MipsRegLo(Opcode.rd),(DWORD)Value);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(ConstReg)?TRUE:FALSE, 0);
				}
			} else {
				DWORD Value = Section->MipsRegLo(ConstReg); 
				int Sign = FALSE;
				if (Section->IsSigned(ConstReg) && Section->IsSigned(MappedReg)) { Sign = TRUE; }				
				if (Value != 0) {
					Map_GPR_32bit(Section,Opcode.rd,Sign,MappedReg);
					AndConstToX86Reg(Section->MipsRegLo(Opcode.rd),Value);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,FALSE, 0);
				}
			}
		}
	} else if (Section->IsKnown(Opcode.rt) || Section->IsKnown(Opcode.rs)) {
		DWORD KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (Section->IsConst(KnownReg)) {
			if (Section->Is64Bit(KnownReg)) {
				unsigned __int64 Value = Section->MipsReg(KnownReg);
				Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
				AndConstToX86Reg(Section->MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
				AndConstToX86Reg(Section->MipsRegLo(Opcode.rd),(DWORD)Value);
			} else {
				DWORD Value = Section->MipsRegLo(KnownReg);
				Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(KnownReg),UnknownReg);
				AndConstToX86Reg(Section->MipsRegLo(Opcode.rd),(DWORD)Value);
			}
		} else {
			ProtectGPR(Section,KnownReg);
			if (KnownReg == Opcode.rd) {
				if (Section->Is64Bit(KnownReg)) {
					Map_GPR_64bit(Section,Opcode.rd,KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],Section->MipsRegHi(Opcode.rd));
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],Section->MipsRegLo(Opcode.rd));
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(KnownReg),KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],Section->MipsRegLo(Opcode.rd));
				}
			} else {
				if (Section->Is64Bit(KnownReg)) {
					Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
					AndX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(KnownReg));
					AndX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(KnownReg));
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(KnownReg),UnknownReg);
					AndX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(KnownReg));
				}
			}
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		AndVariableToX86Reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],Section->MipsRegHi(Opcode.rd));
		AndVariableToX86Reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],Section->MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_OR (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsKnown(Opcode.rt) && Section->IsKnown(Opcode.rs)) {
		if (Section->IsConst(Opcode.rt) && Section->IsConst(Opcode.rs)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {				
				Section->MipsReg(Opcode.rd) = 
					(Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt)) |
					(Section->Is64Bit(Opcode.rs)?Section->MipsReg(Opcode.rs):(_int64)Section->MipsRegLo_S(Opcode.rs));
				if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
					Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
					Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo(Opcode.rt) | Section->MipsRegLo(Opcode.rs);
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (Section->IsMapped(Opcode.rt) && Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(Section,Opcode.rt);
			ProtectGPR(Section,Opcode.rs);
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				if (Section->Is64Bit(source2)) {
					OrX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
				}
			} else {
				ProtectGPR(Section,source2);
				Map_GPR_32bit(Section,Opcode.rd,TRUE,source1);
			}
			OrX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
		} else {
			DWORD ConstReg = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				unsigned _int64 Value;

				if (Section->Is64Bit(ConstReg)) {
					Value = Section->MipsReg(ConstReg);
				} else {
					Value = Section->IsSigned(ConstReg)?Section->MipsRegLo_S(ConstReg):Section->MipsRegLo(ConstReg);
				}
				Map_GPR_64bit(Section,Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),Section->MipsRegHi(Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,Section->MipsRegLo(Opcode.rd));
				}
			} else {
				int Value = Section->MipsRegLo(ConstReg);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, MappedReg);
				if (Value != 0) { OrConstToX86Reg(Value,Section->MipsRegLo(Opcode.rd)); }
			}
		}
	} else if (Section->IsKnown(Opcode.rt) || Section->IsKnown(Opcode.rs)) {
		int KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (Section->IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = Section->Is64Bit(KnownReg)?Section->MipsReg(KnownReg):Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),Section->MipsRegHi(Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,Section->MipsRegLo(Opcode.rd));
			}
		} else {
			Map_GPR_64bit(Section,Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],Section->MipsRegHi(Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],Section->MipsRegLo(Opcode.rd));
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],Section->MipsRegHi(Opcode.rd));
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],Section->MipsRegLo(Opcode.rd));
	}
	if (SPHack && Opcode.rd == 29) { 
		Section->ResetX86Protection();
		_MMU->ResetMemoryStack(Section); 
	}

}

void Compile_R4300i_SPECIAL_XOR (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Opcode.rt == Opcode.rs) {
		UnMap_GPR(Section, Opcode.rd, FALSE);
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		Section->MipsRegLo(Opcode.rd) = 0;
		return;
	}
	if (Section->IsKnown(Opcode.rt) && Section->IsKnown(Opcode.rs)) {
		if (Section->IsConst(Opcode.rt) && Section->IsConst(Opcode.rs)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("XOR 1");
#endif
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				Section->MipsRegLo(Opcode.rd) = Section->MipsRegLo(Opcode.rt) ^ Section->MipsRegLo(Opcode.rs);
			}
		} else if (Section->IsMapped(Opcode.rt) && Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(Section,source1);
			ProtectGPR(Section,source2);
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				if (Section->Is64Bit(source2)) {
					XorX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
				} else if (Section->IsSigned(source2)) {
					XorX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
				}
				XorX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			} else {
				if (Section->IsSigned(Opcode.rt) != Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(Opcode.rt),source1);
				}
				XorX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			}
		} else {
			DWORD ConstReg = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				DWORD ConstHi, ConstLo;

				ConstHi = Section->Is32Bit(ConstReg)?(DWORD)(Section->MipsRegLo_S(ConstReg) >> 31):Section->MipsRegHi(ConstReg);
				ConstLo = Section->MipsRegLo(ConstReg);
				Map_GPR_64bit(Section,Opcode.rd,MappedReg);
				if (ConstHi != 0) { XorConstToX86Reg(Section->MipsRegHi(Opcode.rd),ConstHi); }
				if (ConstLo != 0) { XorConstToX86Reg(Section->MipsRegLo(Opcode.rd),ConstLo); }
			} else {
				int Value = Section->MipsRegLo(ConstReg);
				if (Section->IsSigned(Opcode.rt) != Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { XorConstToX86Reg(Section->MipsRegLo(Opcode.rd),Value); }
			}
		}
	} else if (Section->IsKnown(Opcode.rt) || Section->IsKnown(Opcode.rs)) {
		int KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (Section->IsConst(KnownReg)) {
			unsigned _int64 Value;

			if (Section->Is64Bit(KnownReg)) {
				Value = Section->MipsReg(KnownReg);
			} else {
				if (Section->IsSigned(KnownReg)) {
					Value = (int)Section->MipsRegLo(KnownReg);
				} else {
					Value = Section->MipsRegLo(KnownReg);
				}
			}
			Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				XorConstToX86Reg(Section->MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
			}
			if ((DWORD)Value != 0) {
				XorConstToX86Reg(Section->MipsRegLo(Opcode.rd),(DWORD)Value);
			}
		} else {
			Map_GPR_64bit(Section,Opcode.rd,KnownReg);
			XorVariableToX86reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],Section->MipsRegHi(Opcode.rd));
			XorVariableToX86reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],Section->MipsRegLo(Opcode.rd));
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		XorVariableToX86reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],Section->MipsRegHi(Opcode.rd));
		XorVariableToX86reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],Section->MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_NOR (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsKnown(Opcode.rt) && Section->IsKnown(Opcode.rs)) {
		if (Section->IsConst(Opcode.rt) && Section->IsConst(Opcode.rs)) {
			if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {				
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				Section->MipsRegLo(Opcode.rd) = ~(Section->MipsRegLo(Opcode.rt) | Section->MipsRegLo(Opcode.rs));
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (Section->IsMapped(Opcode.rt) && Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(Section,source1);
			ProtectGPR(Section,source2);
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				if (Section->Is64Bit(source2)) {
					OrX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
				}
				OrX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
				NotX86Reg(Section->MipsRegHi(Opcode.rd));
				NotX86Reg(Section->MipsRegLo(Opcode.rd));
			} else {
				ProtectGPR(Section,source2);
				if (Section->IsSigned(Opcode.rt) != Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(Opcode.rt),source1);
				}
				OrX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
				NotX86Reg(Section->MipsRegLo(Opcode.rd));
			}
		} else {
			DWORD ConstReg = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				unsigned _int64 Value;

				if (Section->Is64Bit(ConstReg)) {
					Value = Section->MipsReg(ConstReg);
				} else {
					Value = Section->IsSigned(ConstReg)?Section->MipsRegLo_S(ConstReg):Section->MipsRegLo(ConstReg);
				}
				Map_GPR_64bit(Section,Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),Section->MipsRegHi(Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,Section->MipsRegLo(Opcode.rd));
				}
				NotX86Reg(Section->MipsRegHi(Opcode.rd));
				NotX86Reg(Section->MipsRegLo(Opcode.rd));
			} else {
				int Value = Section->MipsRegLo(ConstReg);
				if (Section->IsSigned(Opcode.rt) != Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { OrConstToX86Reg(Value,Section->MipsRegLo(Opcode.rd)); }
				NotX86Reg(Section->MipsRegLo(Opcode.rd));
			}
		}
	} else if (Section->IsKnown(Opcode.rt) || Section->IsKnown(Opcode.rs)) {
		int KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (Section->IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = Section->Is64Bit(KnownReg)?Section->MipsReg(KnownReg):Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),Section->MipsRegHi(Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,Section->MipsRegLo(Opcode.rd));
			}
		} else {
			Map_GPR_64bit(Section,Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],Section->MipsRegHi(Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],Section->MipsRegLo(Opcode.rd));
		}
		NotX86Reg(Section->MipsRegHi(Opcode.rd));
		NotX86Reg(Section->MipsRegLo(Opcode.rd));
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],Section->MipsRegHi(Opcode.rd));
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],Section->MipsRegLo(Opcode.rd));
		NotX86Reg(Section->MipsRegHi(Opcode.rd));
		NotX86Reg(Section->MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_SLT (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Section->IsKnown(Opcode.rt) && Section->IsKnown(Opcode.rs)) {
		if (Section->IsConst(Opcode.rt) && Section->IsConst(Opcode.rs)) {
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				DisplayError("1");
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (Section->MipsRegLo_S(Opcode.rs) < Section->MipsRegLo_S(Opcode.rt)) {
					Section->MipsRegLo(Opcode.rd) = 1;
				} else {
					Section->MipsRegLo(Opcode.rd) = 0;
				}
			}
		} else if (Section->IsMapped(Opcode.rt) && Section->IsMapped(Opcode.rs)) {
			ProtectGPR(Section,Opcode.rt);
			ProtectGPR(Section,Opcode.rs);
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					Section->Is64Bit(Opcode.rs)?Section->MipsRegHi(Opcode.rs):Map_TempReg(Section,x86_Any,Opcode.rs,TRUE), 
					Section->Is64Bit(Opcode.rt)?Section->MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				SetlVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs), Section->MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
			} else {
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs), Section->MipsRegLo(Opcode.rt));

				if (Section->MipsRegLo(Opcode.rd) > x86_EDX) {
					SetlVariable(&BranchCompare,"BranchCompare");
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
				} else {					
					Setl(Section->MipsRegLo(Opcode.rd));
					AndConstToX86Reg(Section->MipsRegLo(Opcode.rd), 1);
				}
			}
		} else {
			DWORD ConstReg  = Section->IsConst(Opcode.rs)?Opcode.rs:Opcode.rt;
			DWORD MappedReg = Section->IsConst(Opcode.rs)?Opcode.rt:Opcode.rs;

			ProtectGPR(Section,MappedReg);
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompConstToX86reg(
					Section->Is64Bit(MappedReg)?Section->MipsRegHi(MappedReg):Map_TempReg(Section,x86_Any,MappedReg,TRUE), 
					Section->Is64Bit(ConstReg)?Section->MipsRegHi(ConstReg):(Section->MipsRegLo_S(ConstReg) >> 31)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				if (MappedReg == Opcode.rs) {
					SetlVariable(&BranchCompare,"BranchCompare");
				} else {
					SetgVariable(&BranchCompare,"BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompConstToX86reg(Section->MipsRegLo(MappedReg), Section->MipsRegLo(ConstReg));
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
			} else {
				DWORD Constant = Section->MipsRegLo(ConstReg);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				CompConstToX86reg(Section->MipsRegLo(MappedReg), Constant);
			
				if (Section->MipsRegLo(Opcode.rd) > x86_EDX) {
					if (MappedReg == Opcode.rs) {
						SetlVariable(&BranchCompare,"BranchCompare");
					} else {
						SetgVariable(&BranchCompare,"BranchCompare");
					}
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
				} else {					
					if (MappedReg == Opcode.rs) {
						Setl(Section->MipsRegLo(Opcode.rd));
					} else {
						Setg(Section->MipsRegLo(Opcode.rd));
					}
					AndConstToX86Reg(Section->MipsRegLo(Opcode.rd), 1);
				}
			}
		}		
	} else if (Section->IsKnown(Opcode.rt) || Section->IsKnown(Opcode.rs)) {
		DWORD KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		BYTE *Jump[2];
			
		if (Section->IsConst(KnownReg)) {
			if (Section->Is64Bit(KnownReg)) {
				CompConstToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(((int)Section->MipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(Section,KnownReg);
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		if (KnownReg == (Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetgVariable(&BranchCompare,"BranchCompare");
		} else {
			SetlVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		if (Section->IsConst(KnownReg)) {
			CompConstToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(Section,x86Reg,Opcode.rs,FALSE),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_SLTU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsKnown(Opcode.rt) && Section->IsKnown(Opcode.rs)) {
		if (Section->IsConst(Opcode.rt) && Section->IsConst(Opcode.rs)) {
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("1");
#endif
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (Section->MipsRegLo(Opcode.rs) < Section->MipsRegLo(Opcode.rt)) {
					Section->MipsRegLo(Opcode.rd) = 1;
				} else {
					Section->MipsRegLo(Opcode.rd) = 0;
				}
			}
		} else if (Section->IsMapped(Opcode.rt) && Section->IsMapped(Opcode.rs)) {
			ProtectGPR(Section,Opcode.rt);
			ProtectGPR(Section,Opcode.rs);
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					Section->Is64Bit(Opcode.rs)?Section->MipsRegHi(Opcode.rs):Map_TempReg(Section,x86_Any,Opcode.rs,TRUE), 
					Section->Is64Bit(Opcode.rt)?Section->MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				SetbVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs), Section->MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
			} else {
				CompX86RegToX86Reg(Section->MipsRegLo(Opcode.rs), Section->MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
			}
		} else {
			if (Section->Is64Bit(Opcode.rt) || Section->Is64Bit(Opcode.rs)) {
				DWORD MappedRegHi, MappedRegLo, ConstHi, ConstLo, MappedReg, ConstReg;
				BYTE *Jump[2];

				ConstReg  = Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
				MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;
				
				ConstLo = Section->MipsRegLo(ConstReg);
				ConstHi = (int)ConstLo >> 31;
				if (Section->Is64Bit(ConstReg)) { ConstHi = Section->MipsRegHi(ConstReg); }

				ProtectGPR(Section,MappedReg);
				MappedRegLo = Section->MipsRegLo(MappedReg);
				MappedRegHi = Section->MipsRegHi(MappedReg);
				if (Section->Is32Bit(MappedReg)) {
					MappedRegHi = Map_TempReg(Section,x86_Any,MappedReg,TRUE);
				}

		
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				CompConstToX86reg(MappedRegHi, ConstHi);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
	
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompConstToX86reg(MappedRegLo, ConstLo);
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
			} else {
				DWORD Const = Section->IsConst(Opcode.rs)?Section->MipsRegLo(Opcode.rs):Section->MipsRegLo(Opcode.rt);
				DWORD MappedReg = Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

				CompConstToX86reg(Section->MipsRegLo(MappedReg), Const);
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
			}
		}		
	} else if (Section->IsKnown(Opcode.rt) || Section->IsKnown(Opcode.rs)) {
		DWORD KnownReg = Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		BYTE *Jump[2];
			
		if (Section->IsConst(KnownReg)) {
			if (Section->Is64Bit(KnownReg)) {
				CompConstToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(((int)Section->MipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(Section,KnownReg);
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		if (KnownReg == (Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		if (Section->IsConst(KnownReg)) {
			CompConstToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(Section,x86Reg,Opcode.rs,FALSE),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",Section->MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_DADD (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(Opcode.rt)  && Section->IsConst(Opcode.rs)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsReg(Opcode.rd) = 
			Section->Is64Bit(Opcode.rs)?Section->MipsReg(Opcode.rs):(_int64)Section->MipsRegLo_S(Opcode.rs) +
			Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt);		
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
		int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

		Map_GPR_64bit(Section,Opcode.rd,source1);
		if (Section->IsConst(source2)) {
			AddConstToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			AddConstToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
		} else if (Section->IsMapped(source2)) {
			int HiReg = Section->Is64Bit(source2)?Section->MipsRegHi(source2):Map_TempReg(Section,x86_Any,source2,TRUE);
			ProtectGPR(Section,source2);
			AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			AdcX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(Section->MipsRegHi(Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
}

void Compile_R4300i_SPECIAL_DADDU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(Opcode.rt)  && Section->IsConst(Opcode.rs)) {
		__int64 ValRs = Section->Is64Bit(Opcode.rs)?Section->MipsReg(Opcode.rs):(_int64)Section->MipsRegLo_S(Opcode.rs);
		__int64 ValRt = Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt);
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsReg(Opcode.rd) = ValRs + ValRt;
		if ((Section->MipsRegHi(Opcode.rd) == 0) && (Section->MipsRegLo(Opcode.rd) & 0x80000000) == 0) {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if ((Section->MipsRegHi(Opcode.rd) == 0xFFFFFFFF) && (Section->MipsRegLo(Opcode.rd) & 0x80000000) != 0) {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
		int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

		Map_GPR_64bit(Section,Opcode.rd,source1);
		if (Section->IsConst(source2)) {
			AddConstToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			AddConstToX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(source2));
		} else if (Section->IsMapped(source2)) {
			int HiReg = Section->Is64Bit(source2)?Section->MipsRegHi(source2):Map_TempReg(Section,x86_Any,source2,TRUE);
			ProtectGPR(Section,source2);
			AddX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(source2));
			AdcX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(Section->MipsRegHi(Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
}

void Compile_R4300i_SPECIAL_DSUB (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(Opcode.rt)  && Section->IsConst(Opcode.rs)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsReg(Opcode.rd) = 
			Section->Is64Bit(Opcode.rs)?Section->MipsReg(Opcode.rs):(_int64)Section->MipsRegLo_S(Opcode.rs) -
			Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt);		
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (Opcode.rd == Opcode.rt) {
			int HiReg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			int LoReg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),LoReg);
			SbbX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
		if (Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
			SbbConstFromX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(Opcode.rt));
		} else if (Section->IsMapped(Opcode.rt)) {
			int HiReg = Section->Is64Bit(Opcode.rt)?Section->MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(Section,Opcode.rt);
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
			SbbX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
			SbbVariableFromX86reg(Section->MipsRegHi(Opcode.rd),&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_DSUBU (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Section->IsConst(Opcode.rt)  && Section->IsConst(Opcode.rs)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsReg(Opcode.rd) = 
			Section->Is64Bit(Opcode.rs)?Section->MipsReg(Opcode.rs):(_int64)Section->MipsRegLo_S(Opcode.rs) -
			Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt);		
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (Opcode.rd == Opcode.rt) {
			int HiReg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			int LoReg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),LoReg);
			SbbX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
		if (Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
			SbbConstFromX86Reg(Section->MipsRegHi(Opcode.rd),Section->MipsRegHi(Opcode.rt));
		} else if (Section->IsMapped(Opcode.rt)) {
			int HiReg = Section->Is64Bit(Opcode.rt)?Section->MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(Section,Opcode.rt);
			SubX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rt));
			SbbX86RegToX86Reg(Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
			SbbVariableFromX86reg(Section->MipsRegHi(Opcode.rd),&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_DSLL (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (Section->IsConst(Opcode.rt)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }

		Section->MipsReg(Opcode.rd) = Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt) << Opcode.sa;
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}
	
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	ShiftLeftDoubleImmed(Section->MipsRegHi(Opcode.rd),Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
	ShiftLeftSignImmed(	Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_DSRL (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (Section->IsConst(Opcode.rt)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }

		Section->MipsReg(Opcode.rd) = Section->Is64Bit(Opcode.rt)?Section->MipsReg(Opcode.rt):(QWORD)Section->MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	ShiftRightDoubleImmed(Section->MipsRegLo(Opcode.rd),Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
	ShiftRightUnsignImmed(Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_DSRA (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (Section->IsConst(Opcode.rt)) {
		if (Section->IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }

		Section->MipsReg_S(Opcode.rd) = Section->Is64Bit(Opcode.rt)?Section->MipsReg_S(Opcode.rt):(_int64)Section->MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	ShiftRightDoubleImmed(Section->MipsRegLo(Opcode.rd),Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
	ShiftRightSignImmed(Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_DSLL32 (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (Section->IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegHi(Opcode.rd) = Section->MipsRegLo(Opcode.rt) << Opcode.sa;
		Section->MipsRegLo(Opcode.rd) = 0;
		if (Section->MipsRegLo_S(Opcode.rd) < 0 && Section->MipsRegHi_S(Opcode.rd) == -1){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (Section->MipsRegLo_S(Opcode.rd) >= 0 && Section->MipsRegHi_S(Opcode.rd) == 0){ 
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}

	} else if (Section->IsMapped(Opcode.rt)) {
		ProtectGPR(Section,Opcode.rt);
		Map_GPR_64bit(Section,Opcode.rd,-1);		
		if (Opcode.rt != Opcode.rd) {
			MoveX86RegToX86Reg(Section->MipsRegLo(Opcode.rt),Section->MipsRegHi(Opcode.rd));
		} else {
			int HiReg = Section->MipsRegHi(Opcode.rt);
			Section->MipsRegHi(Opcode.rt) = Section->MipsRegLo(Opcode.rt);
			Section->MipsRegLo(Opcode.rt) = HiReg;
		}
		if ((BYTE)Opcode.sa != 0) {
			ShiftLeftSignImmed(Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
		}
		XorX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rd));
	} else {
		Map_GPR_64bit(Section,Opcode.rd,-1);
		MoveVariableToX86reg(&_GPR[Opcode.rt],CRegName::GPR_Hi[Opcode.rt],Section->MipsRegHi(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftLeftSignImmed(Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
		}
		XorX86RegToX86Reg(Section->MipsRegLo(Opcode.rd),Section->MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_DSRL32 (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Section->IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		Section->MipsRegLo(Opcode.rd) = (DWORD)(Section->MipsReg(Opcode.rt) >> (Opcode.sa + 32));
	} else if (Section->IsMapped(Opcode.rt)) {
		ProtectGPR(Section,Opcode.rt);
		if (Section->Is64Bit(Opcode.rt)) {
			if (Opcode.rt == Opcode.rd) {
				int HiReg = Section->MipsRegHi(Opcode.rt);
				Section->MipsRegHi(Opcode.rt) = Section->MipsRegLo(Opcode.rt);
				Section->MipsRegLo(Opcode.rt) = HiReg;
				Map_GPR_32bit(Section,Opcode.rd,FALSE,-1);
			} else {
				Map_GPR_32bit(Section,Opcode.rd,FALSE,-1);
				MoveX86RegToX86Reg(Section->MipsRegHi(Opcode.rt),Section->MipsRegLo(Opcode.rd));
			}
			if ((BYTE)Opcode.sa != 0) {
				ShiftRightUnsignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
			}
		} else {
			Compile_R4300i_UnknownOpcode(Section);
		}
	} else {
		Map_GPR_32bit(Section,Opcode.rd,FALSE,-1);
		MoveVariableToX86reg(&_GPR[Opcode.rt].UW[1],CRegName::GPR_Lo[Opcode.rt],Section->MipsRegLo(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftRightUnsignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	}
}

void Compile_R4300i_SPECIAL_DSRA32 (CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Section->IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		Section->MipsRegLo(Opcode.rd) = (DWORD)(Section->MipsReg_S(Opcode.rt) >> (Opcode.sa + 32));
	} else if (Section->IsMapped(Opcode.rt)) {
		ProtectGPR(Section,Opcode.rt);
		if (Section->Is64Bit(Opcode.rt)) {
			if (Opcode.rt == Opcode.rd) {
				int HiReg = Section->MipsRegHi(Opcode.rt);
				Section->MipsRegHi(Opcode.rt) = Section->MipsRegLo(Opcode.rt);
				Section->MipsRegLo(Opcode.rt) = HiReg;
				Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			} else {
				Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
				MoveX86RegToX86Reg(Section->MipsRegHi(Opcode.rt),Section->MipsRegLo(Opcode.rd));
			}
			if ((BYTE)Opcode.sa != 0) {
				ShiftRightSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
			}
		} else {
			Compile_R4300i_UnknownOpcode(Section);
		}
	} else {
		Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
		MoveVariableToX86reg(&_GPR[Opcode.rt].UW[1],CRegName::GPR_Lo[Opcode.rt],Section->MipsRegLo(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftRightSignImmed(Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	}
}

/************************** COP0 functions **************************/
void Compile_R4300i_COP0_MF(CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	switch (Opcode.rd) {
	case 9: //Count
		_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
		Section->BlockCycleCount() = 0;
		Section->BlockRandomModifier() = 0;
	}
	Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_CP0[Opcode.rd],CRegName::Cop0[Opcode.rd],Section->MipsRegLo(Opcode.rt));
}

void Compile_R4300i_COP0_MT (CBlockSection * Section) {
	int OldStatusReg;
	BYTE *Jump;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	switch (Opcode.rd) {
	case 0: //Index
	case 2: //EntryLo0
	case 3: //EntryLo1
	case 4: //Context
	case 5: //PageMask
	case 10: //Entry Hi
	case 11: //Compare
	case 14: //EPC
	case 16: //Config
	case 18: //WatchLo 
	case 19: //WatchHi
	case 28: //Tag lo
	case 29: //Tag Hi
	case 30: //ErrEPC
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		switch (Opcode.rd) {
		case 4: //Context
			AndConstToVariable(0xFF800000,&_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
			break;			
		case 11: //Compare
			_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
			Section->BlockCycleCount() = 0;
			Section->BlockRandomModifier() = 0;
			AndConstToVariable(~CAUSE_IP7,&_Reg->FAKE_CAUSE_REGISTER,"FAKE_CAUSE_REGISTER");
			Pushad();
			_Notify->BreakPoint(__FILE__,__LINE__);
			//Call_Direct(ChangeCompareTimer,"ChangeCompareTimer");
			Popad();
		}
		break;
	case 9: //Count
		_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
		Section->BlockCycleCount() = 0;
		Section->BlockRandomModifier() = 0;
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		Pushad();
		_Notify->BreakPoint(__FILE__,__LINE__);
		//Call_Direct(ChangeCompareTimer,"ChangeCompareTimer");
		Popad();
		break;
	case 12: //Status
		OldStatusReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveVariableToX86reg(&_CP0[Opcode.rd],CRegName::Cop0[Opcode.rd],OldStatusReg);
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		XorVariableToX86reg(&_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd],OldStatusReg);
		TestConstToX86Reg(STATUS_FR,OldStatusReg);
		JeLabel8("FpuFlagFine",0);
		Jump = RecompPos - 1;
		Pushad();
		Call_Direct(SetFpuLocations,"SetFpuLocations");
		Popad();
		*(BYTE *)(Jump)= (BYTE )(((BYTE )(RecompPos)) - (((BYTE )(Jump)) + 1));
				
		//TestConstToX86Reg(STATUS_FR,OldStatusReg);
		//BreakPoint(__FILE__,__LINE__); //_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC+4,Section->RegWorking,ExitResetRecompCode,FALSE,JneLabel32);
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	case 6: //Wired
		Pushad();
		_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
		Section->BlockCycleCount() = 0;
		Section->BlockRandomModifier() = 0;
		Call_Direct(FixRandomReg,"FixRandomReg");
		Popad();
		if (Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		break;
	case 13: //cause
		if (Section->IsConst(Opcode.rt)) {
			AndConstToVariable(0xFFFFCFF,&_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
#ifndef EXTERNAL_RELEASE
			if ((Section->MipsRegLo(Opcode.rt) & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		} else {
			Compile_R4300i_UnknownOpcode(Section);
		}
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	default:
		Compile_R4300i_UnknownOpcode(Section);
	}
}

/************************** COP0 CO functions ***********************/
void Compile_R4300i_COP0_CO_TLBR( CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (!UseTlb) {	return; }
	Pushad();
	Call_Direct(TLB_ReadEntry,"TLB_ReadEntry");
	Popad();
}

void Compile_R4300i_COP0_CO_TLBWI( CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (!UseTlb) {	return; }
	Pushad();
	PushImm32("FALSE",FALSE);
	MoveVariableToX86reg(&_Reg->INDEX_REGISTER,"INDEX_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Push(x86_ECX);
	Call_Direct(TLB_WriteEntry,"TLB_WriteEntry");
	AddConstToX86Reg(x86_ESP,8);
	Popad();
}

void Compile_R4300i_COP0_CO_TLBWR( CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (!UseTlb) {	return; }

	_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
	Section->BlockCycleCount() = 0;
	Section->BlockRandomModifier() = 0;
	Pushad();
	Call_Direct(FixRandomReg,"FixRandomReg");
	PushImm32("TRUE",TRUE);
	MoveVariableToX86reg(&_Reg->RANDOM_REGISTER,"RANDOM_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Push(x86_ECX);
	Call_Direct(TLB_WriteEntry,"TLB_WriteEntry");
	AddConstToX86Reg(x86_ESP,8);
	Popad();
}

void Compile_R4300i_COP0_CO_TLBP( CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (!UseTlb) {	return; }
	Pushad();
	MoveConstToX86reg((DWORD)_TLB,x86_ECX);		
	Call_Direct(AddressOf(CTLB::Probe), "CTLB::TLB_Probe");
	Popad();
}

void compiler_COP0_CO_ERET (void) {
	if ((_Reg->STATUS_REGISTER & STATUS_ERL) != 0) {
		*_PROGRAM_COUNTER = _Reg->ERROREPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		*_PROGRAM_COUNTER = _Reg->EPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_EXL;
	}
	*_LLBit = 0;
	CheckInterrupts();
}

void Compile_R4300i_COP0_CO_ERET( CBlockSection * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	WriteBackRegisters(Section);
	Call_Direct(compiler_COP0_CO_ERET,"compiler_COP0_CO_ERET");
	_N64System->GetRecompiler()->CompileExit (Section,Section->CompilePC, (DWORD)-1,Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
	NextInstruction = END_BLOCK;
}

/************************** Other functions **************************/
void Compile_R4300i_UnknownOpcode (CBlockSection * Section) {
	CPU_Message("  %X Unhandled Opcode: %s",Section->CompilePC, R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

//	FreeSection(Section->ContinueSection,Section);
//	FreeSection(Section->JumpSection,Section);
	Section->BlockCycleCount() -= CountPerOp;
	Section->BlockRandomModifier() -= 1;
	MoveConstToVariable(Section->CompilePC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
	WriteBackRegisters(Section);
	_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);
	if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
	MoveConstToVariable(Opcode.Hex,&Opcode.Hex,"Opcode.Hex");
	Call_Direct(R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
	Ret();
	if (NextInstruction == NORMAL) { NextInstruction = END_BLOCK; }
}

