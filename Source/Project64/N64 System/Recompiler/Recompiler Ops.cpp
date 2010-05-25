#include "stdafx.h"

#ifdef tofix
DWORD BranchCompare = 0;
/*
void CompileReadTLBMiss (int AddressReg, int LookUpReg ) {
	MoveX86regToVariable(AddressReg,&TLBLoadAddress,"TLBLoadAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC, m_Section->CompilePC,m_Section->RegWorking,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
}

void CompileWriteTLBMiss (int AddressReg, int LookUpReg ) 
{
	MoveX86regToVariable(AddressReg,&TLBStoreAddress,"TLBStoreAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC, m_Section->CompilePC,m_Section->RegWorking,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
}
*/
/************************** Branch functions  ************************/
void CRecompilerOps::Compile_Branch (BranchFunction CompareFunc, BRANCH_TYPE BranchType, BOOL Link)
{
	static int EffectDelaySlot, DoneJumpDelay, DoneContinueDelay;
	static char ContLabel[100], JumpLabel[100];
	static CRegInfo RegBeforeDelay;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
		
		if ((m_Section->CompilePC & 0xFFC) != 0xFFC) {
			switch (BranchType) {
			case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(m_Section->CompilePC,Opcode.rs,0); break;
			case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(m_Section->CompilePC,Opcode.rs,Opcode.rt); break;
			case BranchTypeCop1: 
				{
					OPCODE Command;

					if (!_MMU ->LW_VAddr(m_Section->CompilePC + 4, Command.Hex)) {
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
		if (m_Section->ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((CCodeSection *)m_Section->ContinueSection)->SectionID);
		} else {
			strcpy(ContLabel,"Cont.LinkLocationinue");
		}
		if (m_Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"Jump.LinkLocation");
		}
		m_Section->Jump.TargetPC        = m_Section->CompilePC + ((short)Opcode.offset << 2) + 4;
		m_Section->Jump.BranchLabel     = JumpLabel;
		m_Section->Jump.LinkLocation    = NULL;
		m_Section->Jump.LinkLocation2   = NULL;
		m_Section->Jump.DoneDelaySlot   = FALSE;
		m_Section->Cont.TargetPC        = m_Section->CompilePC + 8;
		m_Section->Cont.BranchLabel     = ContLabel;
		m_Section->Cont.LinkLocation    = NULL;
		m_Section->Cont.LinkLocation2   = NULL;
		m_Section->Cont.DoneDelaySlot   = FALSE;
		if (m_Section->Jump.TargetPC < m_Section->Cont.TargetPC) {
			m_Section->Cont.FallThrough = FALSE;
			m_Section->Jump.FallThrough = TRUE;
		} else {
			m_Section->Cont.FallThrough = TRUE;
			m_Section->Jump.FallThrough = FALSE;
		}
		if (Link) {
			UnMap_GPR(m_Section, 31, FALSE);
			m_Section->MipsRegLo(31) = m_Section->CompilePC + 8;
			m_Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		if (EffectDelaySlot) {
			if (m_Section->ContinueSection != NULL) {
				sprintf(ContLabel,"Continue",((CCodeSection *)m_Section->ContinueSection)->SectionID);
			} else {
				strcpy(ContLabel,"ExitBlock");
			}
			if (m_Section->JumpSection != NULL) {
				sprintf(JumpLabel,"Jump",((CCodeSection *)m_Section->JumpSection)->SectionID);
			} else {
				strcpy(JumpLabel,"ExitBlock");
			}
			CompareFunc(m_Section); 
			
			if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
				NextInstruction = END_BLOCK;
				return;
			}
			if (!m_Section->Jump.FallThrough && !m_Section->Cont.FallThrough) {
				if (m_Section->Jump.LinkLocation != NULL) {
					CPU_Message("");
					CPU_Message("      %s:",m_Section->Jump.BranchLabel);
					SetJump32((DWORD *)m_Section->Jump.LinkLocation,(DWORD *)RecompPos);
					m_Section->Jump.LinkLocation = NULL;
					if (m_Section->Jump.LinkLocation2 != NULL) {
						SetJump32((DWORD *)m_Section->Jump.LinkLocation2,(DWORD *)RecompPos);
						m_Section->Jump.LinkLocation2 = NULL;
					}
					m_Section->Jump.FallThrough = TRUE;
				} else if (m_Section->Cont.LinkLocation != NULL){
					CPU_Message("");
					CPU_Message("      %s:",m_Section->Cont.BranchLabel);
					SetJump32((DWORD *)m_Section->Cont.LinkLocation,(DWORD *)RecompPos);
					m_Section->Cont.LinkLocation = NULL;
					if (m_Section->Cont.LinkLocation2 != NULL) {
						SetJump32((DWORD *)m_Section->Cont.LinkLocation2,(DWORD *)RecompPos);
						m_Section->Cont.LinkLocation2 = NULL;
					}
					m_Section->Cont.FallThrough = TRUE;
				}
			}
			m_Section->ResetX86Protection();
			memcpy(&RegBeforeDelay,&m_Section->RegWorking,sizeof(CRegInfo));
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (EffectDelaySlot) { 
			CJumpInfo * FallInfo = m_Section->Jump.FallThrough?&m_Section->Jump:&m_Section->Cont;
			CJumpInfo * JumpInfo = m_Section->Jump.FallThrough?&m_Section->Cont:&m_Section->Jump;

			if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot) {
				m_Section->ResetX86Protection();
				FallInfo->RegSet = m_Section->RegWorking;
				if (FallInfo == &m_Section->Jump) {
					if (m_Section->JumpSection != NULL) {
						sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->JumpSection)->SectionID);
					} else {
						strcpy(JumpLabel,"ExitBlock");
					}
					if (FallInfo->TargetPC <= m_Section->CompilePC) 
					{
						_N64System->GetRecompiler()->UpdateCounters(&(FallInfo->RegSet.BlockCycleCount()),&(FallInfo->RegSet.BlockRandomModifier()),true);
						_N64System->GetRecompiler()->CompileSystemCheck(FallInfo->TargetPC,FallInfo->RegSet);
						m_Section->ResetX86Protection();
					}
				} else {
					if (m_Section->ContinueSection != NULL) {
						sprintf(ContLabel,"Section_%d",((CCodeSection *)m_Section->ContinueSection)->SectionID);
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
						memcpy(&m_Section->RegWorking,&RegBeforeDelay,sizeof(CRegInfo));
						return; 
					}
				}
			}
		} else {
			CompareFunc(m_Section);
			m_Section->ResetX86Protection();
			memcpy(&m_Section->Cont.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
			memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
		}
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_BranchLikely (void (*CompareFunc)(CCodeSection * m_Section), BOOL Link) {
	static char ContLabel[100], JumpLabel[100];
	if ( NextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
		
		if (m_Section->ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((CCodeSection *)m_Section->ContinueSection)->SectionID);
		} else {
			strcpy(ContLabel,"ExitBlock");
		}
		if (m_Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		m_Section->Jump.TargetPC      = m_Section->CompilePC + ((short)Opcode.offset << 2) + 4;
		m_Section->Jump.BranchLabel   = JumpLabel;
		m_Section->Jump.FallThrough   = TRUE;
		m_Section->Jump.LinkLocation  = NULL;
		m_Section->Jump.LinkLocation2 = NULL;
		m_Section->Cont.TargetPC      = m_Section->CompilePC + 8;
		m_Section->Cont.BranchLabel   = ContLabel;
		m_Section->Cont.FallThrough   = FALSE;
		m_Section->Cont.LinkLocation  = NULL;
		m_Section->Cont.LinkLocation2 = NULL;
		if (Link) {
			UnMap_GPR(m_Section, 31, FALSE);
			m_Section->MipsRegLo(31) = m_Section->CompilePC + 8;
			m_Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		CompareFunc(m_Section); 
		m_Section->ResetX86Protection();
		memcpy(&m_Section->Cont.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
		if (UseLinking && m_Section->Jump.TargetPC == m_Section->Cont.TargetPC)
		{
			if (m_Section->Cont.FallThrough)  
			{
				BreakPoint(__FILE__,__LINE__);
			}
			if (!m_Section->Jump.FallThrough)
			{
				BreakPoint(__FILE__,__LINE__);
			}
			m_Section->JumpSection->Cont.TargetPC = m_Section->Jump.TargetPC;
			m_Section->JumpSection->DelaySlotSection = true;
			m_Section->Jump.TargetPC = m_Section->CompilePC + 4;
			m_Section->Jump.RegSet = m_Section->RegWorking;
			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			NextInstruction = END_BLOCK;
		} else {
			if (m_Section->Cont.FallThrough)  {
				if (m_Section->Jump.LinkLocation != NULL) {
	#ifndef EXTERNAL_RELEASE
					DisplayError("WTF .. problem with CRecompilerOps::Compile_BranchLikely");
	#endif
				}
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
				NextInstruction = END_BLOCK;
			} else {
				if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
					m_Section->Jump.FallThrough = FALSE;
					if (m_Section->Jump.LinkLocation != NULL) {
						SetJump32(m_Section->Jump.LinkLocation,RecompPos);
						m_Section->Jump.LinkLocation = NULL;
						if (m_Section->Jump.LinkLocation2 != NULL) { 
							SetJump32(m_Section->Jump.LinkLocation2,RecompPos);
							m_Section->Jump.LinkLocation2 = NULL;
						}
					}
					JmpLabel32("DoDelaySlot",0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      %s:",m_Section->Cont.BranchLabel);
					if (m_Section->Cont.LinkLocation != NULL) {
						SetJump32(m_Section->Cont.LinkLocation,RecompPos);
						m_Section->Cont.LinkLocation = NULL;
						if (m_Section->Cont.LinkLocation2 != NULL) { 
							SetJump32(m_Section->Cont.LinkLocation2,RecompPos);
							m_Section->Cont.LinkLocation2 = NULL;
						}
					}
					_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC, m_Section->CompilePC + 8,m_Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
					CPU_Message("      ");
					CPU_Message("      DoDelaySlot");
					_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
					NextInstruction = END_BLOCK;
				} else {
					NextInstruction = DO_DELAY_SLOT;
				}
			}
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {
		m_Section->ResetX86Protection();
		memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranchLikely\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void BNE_Compare (CCodeSection * m_Section) {
	BYTE *Jump;

	if (m_Section->IsKnown(Opcode.rs) && m_Section->IsKnown(Opcode.rt)) {
		if (m_Section->IsConst(Opcode.rs) && m_Section->IsConst(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rs) || m_Section->Is64Bit(Opcode.rt)) {
				CRecompilerOps::Compile_UnknownOpcode(m_Section);
			} else if (m_Section->MipsRegLo(Opcode.rs) != m_Section->MipsRegLo(Opcode.rt)) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsMapped(Opcode.rs) && m_Section->IsMapped(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rs) || m_Section->Is64Bit(Opcode.rt)) {
				ProtectGPR(m_Section,Opcode.rs);
				ProtectGPR(m_Section,Opcode.rt);

				CompX86RegToX86Reg(
					m_Section->Is32Bit(Opcode.rs)?Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE):m_Section->MipsRegHi(Opcode.rs),
					m_Section->Is32Bit(Opcode.rt)?Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE):m_Section->MipsRegHi(Opcode.rt)
				);
					
				if (m_Section->Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs),m_Section->MipsRegLo(Opcode.rt));
				if (m_Section->Cont.FallThrough) {
					JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation2 = RecompPos - 4;
				} else if (m_Section->Jump.FallThrough) {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation2 = RecompPos - 4;
				}
			} else {
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs),m_Section->MipsRegLo(Opcode.rt));
				if (m_Section->Cont.FallThrough) {
					JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation = RecompPos - 4;
				} else if (m_Section->Jump.FallThrough) {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		} else {
			DWORD ConstReg = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (m_Section->Is64Bit(ConstReg) || m_Section->Is64Bit(MappedReg)) {
				if (m_Section->Is32Bit(ConstReg) || m_Section->Is32Bit(MappedReg)) {
					ProtectGPR(m_Section,MappedReg);
					if (m_Section->Is32Bit(MappedReg)) {
						CompConstToX86reg(Map_TempReg(m_Section,x86_Any,MappedReg,TRUE),m_Section->MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(m_Section->MipsRegHi(MappedReg),(int)m_Section->MipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(m_Section->MipsRegHi(MappedReg),m_Section->MipsRegHi(ConstReg));
				}
				if (m_Section->Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
				if (m_Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(ConstReg));
				}
				if (m_Section->Cont.FallThrough) {
					JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation2 = RecompPos - 4;
				} else if (m_Section->Jump.FallThrough) {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation2 = RecompPos - 4;
				}
			} else {
				if (m_Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(ConstReg));
				}
				if (m_Section->Cont.FallThrough) {
					JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation = RecompPos - 4;
				} else if (m_Section->Jump.FallThrough) {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		}
	} else if (m_Section->IsKnown(Opcode.rs) || m_Section->IsKnown(Opcode.rt)) {
		DWORD KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (m_Section->IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompConstToVariable(((int)m_Section->MipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				ProtectGPR(m_Section,KnownReg);
				CompX86regToVariable(Map_TempReg(m_Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (m_Section->Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}
		if (m_Section->IsConst(KnownReg)) {
			CompConstToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (m_Section->Cont.FallThrough) {
			JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
			m_Section->Jump.LinkLocation2 = RecompPos - 4;
		} else if (m_Section->Jump.FallThrough) {
			JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else {
			JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation2 = RecompPos - 4;
		}
	} else {
		int x86Reg;

		x86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE);		
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		if (m_Section->Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}

		x86Reg = Map_TempReg(m_Section,x86Reg,Opcode.rt,FALSE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		if (m_Section->Cont.FallThrough) {
			JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
			m_Section->Jump.LinkLocation2 = RecompPos - 4;
		} else if (m_Section->Jump.FallThrough) {
			JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else {
			JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation2 = RecompPos - 4;
		}
	}
}

void BEQ_Compare (CCodeSection * m_Section) {
	BYTE *Jump;

	if (m_Section->IsKnown(Opcode.rs) && m_Section->IsKnown(Opcode.rt)) {
		if (m_Section->IsConst(Opcode.rs) && m_Section->IsConst(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rs) || m_Section->Is64Bit(Opcode.rt)) {
				CRecompilerOps::Compile_UnknownOpcode(m_Section);
			} else if (m_Section->MipsRegLo(Opcode.rs) == m_Section->MipsRegLo(Opcode.rt)) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsMapped(Opcode.rs) && m_Section->IsMapped(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rs) || m_Section->Is64Bit(Opcode.rt)) {
				ProtectGPR(m_Section,Opcode.rs);
				ProtectGPR(m_Section,Opcode.rt);

				CompX86RegToX86Reg(
					m_Section->Is32Bit(Opcode.rs)?Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE):m_Section->MipsRegHi(Opcode.rs),
					m_Section->Is32Bit(Opcode.rt)?Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE):m_Section->MipsRegHi(Opcode.rt)
				);
				if (m_Section->Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(m_Section->Cont.BranchLabel,0);
					m_Section->Cont.LinkLocation = RecompPos - 4;
				}
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs),m_Section->MipsRegLo(Opcode.rt));
				if (m_Section->Cont.FallThrough) {
					JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else if (m_Section->Jump.FallThrough) {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation2 = RecompPos - 4;
				} else {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation2 = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
			} else {
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs),m_Section->MipsRegLo(Opcode.rt));
				if (m_Section->Cont.FallThrough) {
					JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation = RecompPos - 4;
				} else if (m_Section->Jump.FallThrough) {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		} else {
			DWORD ConstReg = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (m_Section->Is64Bit(ConstReg) || m_Section->Is64Bit(MappedReg)) {
				if (m_Section->Is32Bit(ConstReg) || m_Section->Is32Bit(MappedReg)) {
					if (m_Section->Is32Bit(MappedReg)) {
						ProtectGPR(m_Section,MappedReg);
						CompConstToX86reg(Map_TempReg(m_Section,x86_Any,MappedReg,TRUE),m_Section->MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(m_Section->MipsRegHi(MappedReg),(int)m_Section->MipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(m_Section->MipsRegHi(MappedReg),m_Section->MipsRegHi(ConstReg));
				}			
				if (m_Section->Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(m_Section->Cont.BranchLabel,0);
					m_Section->Cont.LinkLocation = RecompPos - 4;
				}
				if (m_Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(ConstReg));
				}
				if (m_Section->Cont.FallThrough) {
					JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else if (m_Section->Jump.FallThrough) {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation2 = RecompPos - 4;
				} else {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation2 = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
			} else {
				if (m_Section->MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(m_Section->MipsRegLo(MappedReg),m_Section->MipsRegLo(ConstReg));
				}
				if (m_Section->Cont.FallThrough) {
					JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
					m_Section->Jump.LinkLocation = RecompPos - 4;
				} else if (m_Section->Jump.FallThrough) {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
					m_Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(m_Section->Jump.BranchLabel,0);
					m_Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		}
	} else if (m_Section->IsKnown(Opcode.rs) || m_Section->IsKnown(Opcode.rt)) {
		DWORD KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (m_Section->IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompConstToVariable((int)m_Section->MipsRegLo(KnownReg) >> 31,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			ProtectGPR(m_Section,KnownReg);
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompX86regToVariable(Map_TempReg(m_Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (m_Section->Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(m_Section->Cont.BranchLabel,0);
			m_Section->Cont.LinkLocation = RecompPos - 4;
		}
		if (m_Section->IsConst(KnownReg)) {
			CompConstToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (m_Section->Cont.FallThrough) {
			JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
			m_Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else if (m_Section->Jump.FallThrough) {
			JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation2 = RecompPos - 4;
		} else {
			JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation2 = RecompPos - 4;
			JmpLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}
	} else {
		int x86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		if (m_Section->Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(m_Section->Cont.BranchLabel,0);
			m_Section->Cont.LinkLocation = RecompPos - 4;
		}
		CompX86regToVariable(Map_TempReg(m_Section,x86Reg,Opcode.rs,FALSE),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		if (m_Section->Cont.FallThrough) {
			JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
			m_Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else if (m_Section->Jump.FallThrough) {
			JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation2 = RecompPos - 4;
		} else {
			JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation2 = RecompPos - 4;
			JmpLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}
	}
}

void BGTZ_Compare (CCodeSection * m_Section) {
	if (m_Section->IsConst(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			if (m_Section->MipsReg_S(Opcode.rs) > 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else {
			if (m_Section->MipsRegLo_S(Opcode.rs) > 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		}
	} else if (m_Section->IsMapped(Opcode.rs) && m_Section->Is32Bit(Opcode.rs)) {
		CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),0);
		if (m_Section->Jump.FallThrough) {
			JleLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
		} else if (m_Section->Cont.FallThrough) {
			JgLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JleLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}
	} else {
		BYTE *Jump;

		if (m_Section->IsMapped(Opcode.rs)) {
			CompConstToX86reg(m_Section->MipsRegHi(Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		}
		if (m_Section->Jump.FallThrough) {
			JlLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			JgLabel8("continue",0);
			Jump = RecompPos - 1;
		} else if (m_Section->Cont.FallThrough) {
			JlLabel8("continue",0);
			Jump = RecompPos - 1;
			JgLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			JgLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}

		if (m_Section->IsMapped(Opcode.rs)) {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
		}
		if (m_Section->Jump.FallThrough) {
			JeLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation2 = RecompPos - 4;
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		} else if (m_Section->Cont.FallThrough) {
			JneLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		} else {
			JneLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
			JmpLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation2 = RecompPos - 4;
		}
	}
}

void BLEZ_Compare (CCodeSection * m_Section) {
	if (m_Section->IsConst(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			if (m_Section->MipsReg_S(Opcode.rs) <= 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsSigned(Opcode.rs)) {
			if (m_Section->MipsRegLo_S(Opcode.rs) <= 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else {
			if (m_Section->MipsRegLo(Opcode.rs) == 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		}
	} else {
		if (m_Section->IsMapped(Opcode.rs) && m_Section->Is32Bit(Opcode.rs)) {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),0);
			if (m_Section->Jump.FallThrough) {
				JgLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
			} else if (m_Section->Cont.FallThrough) {
				JleLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else {
			BYTE *Jump;

			if (m_Section->IsMapped(Opcode.rs)) {
				CompConstToX86reg(m_Section->MipsRegHi(Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
			}
			if (m_Section->Jump.FallThrough) {
				JgLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JlLabel8("Continue",0);
				Jump = RecompPos - 1;
			} else if (m_Section->Cont.FallThrough) {
				JgLabel8("Continue",0);
				Jump = RecompPos - 1;
				JlLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JlLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			}

			if (m_Section->IsMapped(Opcode.rs)) {
				CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs]);
			}
			if (m_Section->Jump.FallThrough) {
				JneLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation2 = RecompPos - 4;
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			} else if (m_Section->Cont.FallThrough) {
				JeLabel32 (m_Section->Jump.BranchLabel, 0 );
				m_Section->Jump.LinkLocation2 = RecompPos - 4;
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			} else {
				JneLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation2 = RecompPos - 4;
				JmpLabel32("BranchToJump",0);
				m_Section->Jump.LinkLocation2 = RecompPos - 4;
			}
		}
	}
}

void BLTZ_Compare (CCodeSection * m_Section) {
	if (m_Section->IsConst(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			if (m_Section->MipsReg_S(Opcode.rs) < 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsSigned(Opcode.rs)) {
			if (m_Section->MipsRegLo_S(Opcode.rs) < 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else {
			m_Section->Jump.FallThrough = FALSE;
			m_Section->Cont.FallThrough = TRUE;
		}
	} else if (m_Section->IsMapped(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			CompConstToX86reg(m_Section->MipsRegHi(Opcode.rs),0);
			if (m_Section->Jump.FallThrough) {
				JgeLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
			} else if (m_Section->Cont.FallThrough) {
				JlLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgeLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else if (m_Section->IsSigned(Opcode.rs)) {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),0);
			if (m_Section->Jump.FallThrough) {
				JgeLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
			} else if (m_Section->Cont.FallThrough) {
				JlLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgeLabel32 (m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else {
			m_Section->Jump.FallThrough = FALSE;
			m_Section->Cont.FallThrough = TRUE;
		}
	} else if (m_Section->IsUnknown(Opcode.rs)) {
		CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);
		if (m_Section->Jump.FallThrough) {
			JgeLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
		} else if (m_Section->Cont.FallThrough) {
			JlLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
			JmpLabel32 (m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
		}
	}
}

void BGEZ_Compare (CCodeSection * m_Section) {
	if (m_Section->IsConst(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
			DisplayError("BGEZ 1");
#endif
			CRecompilerOps::Compile_UnknownOpcode(m_Section);
		} else if (m_Section->IsSigned(Opcode.rs)) {
			if (m_Section->MipsRegLo_S(Opcode.rs) >= 0) {
				m_Section->Jump.FallThrough = TRUE;
				m_Section->Cont.FallThrough = FALSE;
			} else {
				m_Section->Jump.FallThrough = FALSE;
				m_Section->Cont.FallThrough = TRUE;
			}
		} else {
			m_Section->Jump.FallThrough = TRUE;
			m_Section->Cont.FallThrough = FALSE;
		}
	} else if (m_Section->IsMapped(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) { 
			CompConstToX86reg(m_Section->MipsRegHi(Opcode.rs),0);
			if (m_Section->Cont.FallThrough) {
				JgeLabel32 ( m_Section->Jump.BranchLabel, 0 );
				m_Section->Jump.LinkLocation = RecompPos - 4;
			} else if (m_Section->Jump.FallThrough) {
				JlLabel32 ( m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
			} else {
				JlLabel32 ( m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else if (m_Section->IsSigned(Opcode.rs)) {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),0);
			if (m_Section->Cont.FallThrough) {
				JgeLabel32 ( m_Section->Jump.BranchLabel, 0 );
				m_Section->Jump.LinkLocation = RecompPos - 4;
			} else if (m_Section->Jump.FallThrough) {
				JlLabel32 ( m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
			} else {
				JlLabel32 ( m_Section->Cont.BranchLabel, 0 );
				m_Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(m_Section->Jump.BranchLabel,0);
				m_Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else { 
			m_Section->Jump.FallThrough = TRUE;
			m_Section->Cont.FallThrough = FALSE;
		}
	} else {
		CompConstToVariable(0,&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs]);		
		if (m_Section->Cont.FallThrough) {
			JgeLabel32 ( m_Section->Jump.BranchLabel, 0 );
			m_Section->Jump.LinkLocation = RecompPos - 4;
		} else if (m_Section->Jump.FallThrough) {
			JlLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32 ( m_Section->Cont.BranchLabel, 0 );
			m_Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(m_Section->Jump.BranchLabel,0);
			m_Section->Jump.LinkLocation = RecompPos - 4;
		}
	}
}

void COP1_BCF_Compare (CCodeSection * m_Section) {
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (m_Section->Cont.FallThrough) {
		JeLabel32 ( m_Section->Jump.BranchLabel, 0 );
		m_Section->Jump.LinkLocation = RecompPos - 4;
	} else if (m_Section->Jump.FallThrough) {
		JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
		m_Section->Cont.LinkLocation = RecompPos - 4;
	} else {
		JneLabel32 ( m_Section->Cont.BranchLabel, 0 );
		m_Section->Cont.LinkLocation = RecompPos - 4;
		JmpLabel32(m_Section->Jump.BranchLabel,0);
		m_Section->Jump.LinkLocation = RecompPos - 4;
	}
}

void COP1_BCT_Compare (CCodeSection * m_Section) {
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (m_Section->Cont.FallThrough) {
		JneLabel32 ( m_Section->Jump.BranchLabel, 0 );
		m_Section->Jump.LinkLocation = RecompPos - 4;
	} else if (m_Section->Jump.FallThrough) {
		JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
		m_Section->Cont.LinkLocation = RecompPos - 4;
	} else {
		JeLabel32 ( m_Section->Cont.BranchLabel, 0 );
		m_Section->Cont.LinkLocation = RecompPos - 4;
		JmpLabel32(m_Section->Jump.BranchLabel,0);
		m_Section->Jump.LinkLocation = RecompPos - 4;
	}
}
/*************************  OpCode functions *************************/
void CRecompilerOps::Compile_J (CCodeSection * m_Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

		if (m_Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		m_Section->Jump.TargetPC      = (m_Section->CompilePC & 0xF0000000) + (Opcode.target << 2);;
		m_Section->Jump.BranchLabel   = JumpLabel;
		m_Section->Jump.FallThrough   = TRUE;
		m_Section->Jump.LinkLocation  = NULL;
		m_Section->Jump.LinkLocation2 = NULL;
		NextInstruction = DO_DELAY_SLOT;
		if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			NextInstruction = END_BLOCK;
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_JAL (CCodeSection * m_Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
		UnMap_GPR(m_Section, 31, FALSE);
		m_Section->MipsRegLo(31) = m_Section->CompilePC + 8;
		m_Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
			MoveConstToVariable((m_Section->CompilePC & 0xF0000000) + (Opcode.target << 2),&JumpToLocation,"JumpToLocation");
			MoveConstToVariable(m_Section->CompilePC + 4,_PROGRAM_COUNTER,"_PROGRAM_COUNTER");
			_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
			WriteBackRegisters(m_Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		DWORD TargetPC = (m_Section->CompilePC & 0xF0000000) + (Opcode.target << 2);
		_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC,TargetPC,m_Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
	return;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

		UnMap_GPR(m_Section, 31, FALSE);
		m_Section->MipsRegLo(31) = m_Section->CompilePC + 8;
		m_Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
		NextInstruction = DO_DELAY_SLOT;
		if (m_Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		m_Section->Jump.TargetPC      = (m_Section->CompilePC & 0xF0000000) + (Opcode.target << 2);
		m_Section->Jump.BranchLabel   = JumpLabel;
		m_Section->Jump.FallThrough   = TRUE;
		m_Section->Jump.LinkLocation  = NULL;
		m_Section->Jump.LinkLocation2 = NULL;
		if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			NextInstruction = END_BLOCK;
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_ADDI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) { return; }

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && Opcode.rs == 29 && Opcode.rt == 29) {
		AddConstToX86Reg(Map_MemoryStack(m_Section, x86_Any, true),(short)Opcode.immediate);
	}
#endif

	if (m_Section->IsConst(Opcode.rs)) { 
		if (m_Section->IsMapped(Opcode.rt)) { UnMap_GPR(m_Section,Opcode.rt, FALSE); }
		m_Section->MipsRegLo(Opcode.rt) = m_Section->MipsRegLo(Opcode.rs) + (short)Opcode.immediate;
		m_Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.rs);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(m_Section->MipsRegLo(Opcode.rt));
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(m_Section->MipsRegLo(Opcode.rt));
		} else {
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
		}
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && Opcode.rt == 29 && Opcode.rs != 29) { 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::Compile_ADDIU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) { return; }

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack)
	{
		if (Opcode.rs == 29 && Opcode.rt == 29) 
		{
			AddConstToX86Reg(Map_MemoryStack(m_Section, x86_Any, TRUE),(short)Opcode.immediate);
		}
	}
#endif

	if (m_Section->IsConst(Opcode.rs)) { 
		if (m_Section->IsMapped(Opcode.rt)) { UnMap_GPR(m_Section,Opcode.rt, FALSE); }
		m_Section->MipsRegLo(Opcode.rt) = m_Section->MipsRegLo(Opcode.rs) + (short)Opcode.immediate;
		m_Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.rs);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(m_Section->MipsRegLo(Opcode.rt));
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(m_Section->MipsRegLo(Opcode.rt));
		} else {
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
		}
	}

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && Opcode.rt == 29 && Opcode.rs != 29) { 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::Compile_SLTIU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rt == 0) { return; }

	if (m_Section->IsConst(Opcode.rs)) { 
		DWORD Result;

		if (m_Section->Is64Bit(Opcode.rs)) {
			_int64 Immediate = (_int64)((short)Opcode.immediate);
			Result = m_Section->MipsReg(Opcode.rs) < ((unsigned)(Immediate))?1:0;
		} else if (m_Section->Is32Bit(Opcode.rs)) {
			Result = m_Section->MipsRegLo(Opcode.rs) < ((unsigned)((short)Opcode.immediate))?1:0;
		}
		UnMap_GPR(m_Section,Opcode.rt, FALSE);
		m_Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
		m_Section->MipsRegLo(Opcode.rt) = Result;
	} else if (m_Section->IsMapped(Opcode.rs)) { 
		if (m_Section->Is64Bit(Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(m_Section->MipsRegHi(Opcode.rs),((short)Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = RecompPos - 1;
			SetbVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
			Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
		} else {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
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
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
		
		
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
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));*/
	}
}

void CRecompilerOps::Compile_SLTI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rt == 0) { return; }

	if (m_Section->IsConst(Opcode.rs)) { 
		DWORD Result;

		if (m_Section->Is64Bit(Opcode.rs)) {
			_int64 Immediate = (_int64)((short)Opcode.immediate);
			Result = (_int64)m_Section->MipsReg(Opcode.rs) < Immediate?1:0;
		} else if (m_Section->Is32Bit(Opcode.rs)) {
			Result = m_Section->MipsRegLo_S(Opcode.rs) < (short)Opcode.immediate?1:0;
		}
		UnMap_GPR(m_Section,Opcode.rt, FALSE);
		m_Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
		m_Section->MipsRegLo(Opcode.rt) = Result;
	} else if (m_Section->IsMapped(Opcode.rs)) { 
		if (m_Section->Is64Bit(Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(m_Section->MipsRegHi(Opcode.rs),((short)Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = RecompPos - 1;
			SetlVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
			Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
		} else {
		/*	CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetlVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
			*/
			ProtectGPR(m_Section, Opcode.rs);
			Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			
			if (m_Section->MipsRegLo(Opcode.rt) > x86_EDX) {
				SetlVariable(&BranchCompare,"BranchCompare");
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
			} else {
				Setl(m_Section->MipsRegLo(Opcode.rt));
				AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rt), 1);
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
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rt));
	}
}

void CRecompilerOps::Compile_ANDI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) { return;}

	if (m_Section->IsConst(Opcode.rs)) {
		if (m_Section->IsMapped(Opcode.rt)) { UnMap_GPR(m_Section,Opcode.rt, FALSE); }
		m_Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
		m_Section->MipsRegLo(Opcode.rt) = m_Section->MipsRegLo(Opcode.rs) & Opcode.immediate;
	} else if (Opcode.immediate != 0) { 
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,Opcode.rs);
		AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),Opcode.immediate);
	} else {
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,0);
	}
}

void CRecompilerOps::Compile_ORI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (m_Section->IsConst(Opcode.rs)) {
		if (m_Section->IsMapped(Opcode.rt)) { UnMap_GPR(m_Section,Opcode.rt, FALSE); }
		m_Section->MipsRegState(Opcode.rt) = m_Section->MipsRegState(Opcode.rs);
		m_Section->MipsRegHi(Opcode.rt) = m_Section->MipsRegHi(Opcode.rs);
		m_Section->MipsRegLo(Opcode.rt) = m_Section->MipsRegLo(Opcode.rs) | Opcode.immediate;
	} else if (m_Section->IsMapped(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			Map_GPR_64bit(m_Section,Opcode.rt,Opcode.rs);
		} else {
			Map_GPR_32bit(m_Section,Opcode.rt,m_Section->IsSigned(Opcode.rs),Opcode.rs);
		}
		OrConstToX86Reg(Opcode.immediate,m_Section->MipsRegLo(Opcode.rt));
	} else {
		Map_GPR_64bit(m_Section,Opcode.rt,Opcode.rs);
		OrConstToX86Reg(Opcode.immediate,m_Section->MipsRegLo(Opcode.rt));
	}
}

void CRecompilerOps::Compile_XORI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (m_Section->IsConst(Opcode.rs)) {
		if (Opcode.rs != Opcode.rt) { UnMap_GPR(m_Section,Opcode.rt, FALSE); }
		m_Section->MipsRegState(Opcode.rt) = m_Section->MipsRegState(Opcode.rs);
		m_Section->MipsRegHi(Opcode.rt) = m_Section->MipsRegHi(Opcode.rs);
		m_Section->MipsRegLo(Opcode.rt) = m_Section->MipsRegLo(Opcode.rs) ^ Opcode.immediate;
	} else {
		if (m_Section->IsMapped(Opcode.rs) && m_Section->Is32Bit(Opcode.rs)) {
			Map_GPR_32bit(m_Section,Opcode.rt,m_Section->IsSigned(Opcode.rs),Opcode.rs);
		} else {
			Map_GPR_64bit(m_Section,Opcode.rt,Opcode.rs);
		}
		if (Opcode.immediate != 0) { XorConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),Opcode.immediate); }
	}
}

void CRecompilerOps::Compile_LUI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && Opcode.rt == 29) {
		int x86reg = Map_MemoryStack(m_Section, x86_Any, false);
		DWORD Address;

		TranslateVaddr (((short)Opcode.offset << 16), &Address);
		if (x86reg < 0) {
			MoveConstToVariable((DWORD)(Address + RDRAM), g_MemoryStack, "MemoryStack");
		} else {
			MoveConstToX86reg((DWORD)(Address + RDRAM), x86reg);
		}
	}
#endif
	UnMap_GPR(m_Section,Opcode.rt, FALSE);
	m_Section->MipsRegLo(Opcode.rt) = ((short)Opcode.offset << 16);
	m_Section->MipsRegState(Opcode.rt) = CRegInfo::STATE_CONST_32;
}

void CRecompilerOps::Compile_DADDIU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rs != 0) { UnMap_GPR(m_Section,Opcode.rs,TRUE); }
	if (Opcode.rs != 0) { UnMap_GPR(m_Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::DADDIU, "R4300iOp::DADDIU");
	Popad();
}

void CRecompilerOps::Compile_LDL (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(m_Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(m_Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::LDL, "R4300iOp::LDL");
	Popad();

}

void CRecompilerOps::Compile_LDR (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(m_Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(m_Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::LDR, "R4300iOp::LDR");
	Popad();
}


void CRecompilerOps::Compile_LB (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,0);
		_MMU->Compile_LB(m_Section->MipsRegLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveSxByteX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regByte(m_Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void CRecompilerOps::Compile_LH (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,0);
		_MMU->Compile_LH(m_Section->MipsRegLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveSxHalfX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regHalf(m_Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void CRecompilerOps::Compile_LWL (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address, Value;
		
		Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.rt);
		Value = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),LWL_MASK[Offset]);
		ShiftLeftSignImmed(Value,(BYTE)LWL_SHIFT[Offset]);
		AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(m_Section,x86_ECX,-1,FALSE);
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
	}
	Offset = Map_TempReg(m_Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg(LWL_MASK,"LWL_MASK",m_Section->MipsRegLo(Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWL_SHIFT,"LWL_SHIFT",shift,Offset,4);
	if (UseTlb) {			
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftLeftSign(TempReg1);
	AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),TempReg1);
}

void CRecompilerOps::Compile_LW (CCodeSection * m_Section) 
{
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;
	if (Opcode.base == 29 && SPHack) {
		char String[100];

		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		TempReg1 = Map_MemoryStack(m_Section,x86_Any,true);
		sprintf(String,"%Xh",(short)Opcode.offset);
		MoveVariableDispToX86Reg((void *)((DWORD)(short)Opcode.offset),String,m_Section->MipsRegLo(Opcode.rt),TempReg1,1);
	} else {
		if (m_Section->IsConst(Opcode.base)) { 
			DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
			_MMU->Compile_LW(m_Section, m_Section->MipsRegLo(Opcode.rt),Address);
		} else {
			if (UseTlb) {	
				if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
				if (m_Section->IsMapped(Opcode.base) && Opcode.offset == 0) { 
					ProtectGPR(m_Section,Opcode.base);
					TempReg1 = m_Section->MipsRegLo(Opcode.base);
				} else {
					if (m_Section->IsMapped(Opcode.base)) { 
						ProtectGPR(m_Section,Opcode.base);
						if (Opcode.offset != 0) {
							TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
							LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
						} else {
							TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
						}
					} else {
						TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
				TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
				MoveX86RegToX86Reg(TempReg1, TempReg2);
				ShiftRightUnsignImmed(TempReg2,12);
				MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
				CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
				Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
				MoveX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
			} else {
				if (m_Section->IsMapped(Opcode.base)) { 
					ProtectGPR(m_Section,Opcode.base);
					if (Opcode.offset != 0) {
						Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
						LeaSourceAndOffset(m_Section->MipsRegLo(Opcode.rt),m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
					} else {
						Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.base);
					}
				} else {
					Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.base);
					if (Opcode.immediate == 0) { 
					} else if (Opcode.immediate == 1) {
						IncX86reg(m_Section->MipsRegLo(Opcode.rt));
					} else if (Opcode.immediate == 0xFFFF) {			
						DecX86reg(m_Section->MipsRegLo(Opcode.rt));
					} else {
						AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
					}
				}
				AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),0x1FFFFFFF);
				MoveN64MemToX86reg(m_Section->MipsRegLo(Opcode.rt),m_Section->MipsRegLo(Opcode.rt));
			}
		}
	}
	if (SPHack && Opcode.rt == 29)
	{ 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::Compile_LBU (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,0);
		_MMU->Compile_LB(m_Section->MipsRegLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,-1);
		MoveZxByteX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,-1);
		MoveZxN64MemToX86regByte(m_Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void CRecompilerOps::Compile_LHU (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,0);
		_MMU->Compile_LH(m_Section->MipsRegLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(m_Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void CRecompilerOps::Compile_LWR (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address, Value;
		
		Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.rt);
		Value = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),LWR_MASK[Offset]);
		ShiftRightUnsignImmed(Value,(BYTE)LWR_SHIFT[Offset]);
		AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(m_Section,x86_ECX,-1,FALSE);
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
	}
	Offset = Map_TempReg(m_Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg(LWR_MASK,"LWR_MASK",m_Section->MipsRegLo(Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWR_SHIFT,"LWR_SHIFT",shift,Offset,4);
	if (UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftRightUnsign(TempReg1);
	AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),TempReg1);
}

void CRecompilerOps::Compile_LWU (CCodeSection * m_Section) {			//added by Witten
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset);
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,0);
		_MMU->Compile_LW(m_Section, m_Section->MipsRegLo(Opcode.rt),Address);
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(m_Section->MipsRegLo(Opcode.rt), TempReg1);
	}
}

void CRecompilerOps::Compile_SB (CCodeSection * m_Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		
		if (m_Section->IsConst(Opcode.rt)) {
			_MMU->Compile_SB_Const((BYTE)m_Section->MipsRegLo(Opcode.rt), Address);
		} else if (m_Section->IsMapped(Opcode.rt) && Is8BitReg(m_Section->MipsRegLo(Opcode.rt))) {
			_MMU->Compile_SB_Register(m_Section->MipsRegLo(Opcode.rt), Address);
		} else {
			_MMU->Compile_SB_Register(Map_TempReg(m_Section,x86_Any8Bit,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,3);	
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstByteToX86regPointer((BYTE)m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (m_Section->IsMapped(Opcode.rt) && Is8BitReg(m_Section->MipsRegLo(Opcode.rt))) {
			MoveX86regByteToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			UnProtectGPR(m_Section,Opcode.rt);
			MoveX86regByteToX86regPointer(Map_TempReg(m_Section,x86_Any8Bit,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstByteToN64Mem((BYTE)m_Section->MipsRegLo(Opcode.rt),TempReg1);
		} else if (m_Section->IsMapped(Opcode.rt) && Is8BitReg(m_Section->MipsRegLo(Opcode.rt))) {
			MoveX86regByteToN64Mem(m_Section->MipsRegLo(Opcode.rt),TempReg1);
		} else {	
			UnProtectGPR(m_Section,Opcode.rt);
			MoveX86regByteToN64Mem(Map_TempReg(m_Section,x86_Any8Bit,Opcode.rt,FALSE),TempReg1);
		}
	}

}

void CRecompilerOps::Compile_SH (CCodeSection * m_Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = (m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		
		if (m_Section->IsConst(Opcode.rt)) {
			_MMU->Compile_SH_Const((WORD)m_Section->MipsRegLo(Opcode.rt), Address);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			_MMU->Compile_SH_Register(m_Section->MipsRegLo(Opcode.rt), Address);
		} else {
			_MMU->Compile_SH_Register(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,2);	
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstHalfToX86regPointer((WORD)m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regHalfToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regHalfToX86regPointer(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		XorConstToX86Reg(TempReg1,2);
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstHalfToN64Mem((WORD)m_Section->MipsRegLo(Opcode.rt),TempReg1);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regHalfToN64Mem(m_Section->MipsRegLo(Opcode.rt),TempReg1);		
		} else {	
			MoveX86regHalfToN64Mem(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE),TempReg1);		
		}
	}
}

void CRecompilerOps::Compile_SWL (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;
		
		Value = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(Value,SWL_MASK[Offset]);
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
		ShiftRightUnsignImmed(TempReg1,(BYTE)SWL_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		_MMU->Compile_SW_Register(m_Section,Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(m_Section,x86_ECX,-1,FALSE);
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = Map_TempReg(m_Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = Map_TempReg(m_Section,x86_Any,-1,FALSE);
	if (UseTlb) {	
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWL_MASK,"SWL_MASK",Value,Offset,4);
	if (!m_Section->IsConst(Opcode.rt) || m_Section->MipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWL_SHIFT,"SWL_SHIFT",shift,Offset,4);
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToX86reg(m_Section->MipsRegLo(Opcode.rt),Offset);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),Offset);
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

void CRecompilerOps::Compile_SW (CCodeSection * m_Section){

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;
	if (Opcode.base == 29 && SPHack) {
		if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
		TempReg1 = Map_MemoryStack(m_Section,x86_Any,true);

		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToMemoryDisp (m_Section->MipsRegLo(Opcode.rt),TempReg1, (DWORD)((short)Opcode.offset));
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToMemory(m_Section->MipsRegLo(Opcode.rt),TempReg1,(DWORD)((short)Opcode.offset));
		} else {	
			TempReg2 = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
			MoveX86regToMemory(TempReg2,TempReg1,(DWORD)((short)Opcode.offset));
		}		
	} else {
		if (m_Section->IsConst(Opcode.base)) { 
			DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			
			if (m_Section->IsConst(Opcode.rt)) {
				_MMU->Compile_SW_Const(m_Section->MipsRegLo(Opcode.rt), Address);
			} else if (m_Section->IsMapped(Opcode.rt)) {
				_MMU->Compile_SW_Register(m_Section,m_Section->MipsRegLo(Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Register(m_Section,Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), Address);
			}
			return;
		}
		if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
		if (m_Section->IsMapped(Opcode.base)) { 
			ProtectGPR(m_Section,Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
			}
			UnProtectGPR(m_Section,Opcode.base);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
			TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			MoveX86RegToX86Reg(TempReg1, TempReg2);
			ShiftRightUnsignImmed(TempReg2,12);
			MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
			//For tlb miss
			//0041C522 85 C0                test        eax,eax
			//0041C524 75 01                jne         0041C527

			if (m_Section->IsConst(Opcode.rt)) {
				MoveConstToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else if (m_Section->IsMapped(Opcode.rt)) {
				MoveX86regToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else {	
				MoveX86regToX86regPointer(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
			}
		} else {
			AndConstToX86Reg(TempReg1,0x1FFFFFFF);
			if (m_Section->IsConst(Opcode.rt)) {
				MoveConstToN64Mem(m_Section->MipsRegLo(Opcode.rt),TempReg1);
			} else if (m_Section->IsMapped(Opcode.rt)) {
				MoveX86regToN64Mem(m_Section->MipsRegLo(Opcode.rt),TempReg1);
			} else {	
				MoveX86regToN64Mem(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE),TempReg1);
			}
		}
	}
#endif
}

void CRecompilerOps::Compile_SWR (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;
		
		Value = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(Value,SWR_MASK[Offset]);
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
		ShiftLeftSignImmed(TempReg1,(BYTE)SWR_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		_MMU->Compile_SW_Register(m_Section,Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(m_Section,x86_ECX,-1,FALSE);
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = Map_TempReg(m_Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = Map_TempReg(m_Section,x86_Any,-1,FALSE);
	if (UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWR_MASK,"SWR_MASK",Value,Offset,4);
	if (!m_Section->IsConst(Opcode.rt) || m_Section->MipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWR_SHIFT,"SWR_SHIFT",shift,Offset,4);
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToX86reg(m_Section->MipsRegLo(Opcode.rt),Offset);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),Offset);
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

void CRecompilerOps::Compile_SDL (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(m_Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(m_Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SDL, "R4300iOp::SDL");
	Popad();

}

void CRecompilerOps::Compile_SDR (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(m_Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(m_Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SDR, "R4300iOp::SDR");
	Popad();

}

void CRecompilerOps::Compile_CACHE (CCodeSection * m_Section){
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

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
		if (m_Section->IsConst(Opcode.base)) { 
			DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			PushImm32("Address",Address);
		} else if (m_Section->IsMapped(Opcode.base)) { 
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			Push(m_Section->MipsRegLo(Opcode.base));
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

void CRecompilerOps::Compile_LL (CCodeSection * m_Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		_MMU->Compile_LW(m_Section, m_Section->MipsRegLo(Opcode.rt),Address);
		MoveConstToVariable(1,_LLBit,"LLBit");
		
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		TranslateVaddr(Address, &Address);
#endif
		MoveConstToVariable(Address,_LLAddr,"LLAddr");
		return;
	}
	if (UseTlb) {	
		if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
		if (m_Section->IsMapped(Opcode.base) && Opcode.offset == 0) { 
			ProtectGPR(m_Section,Opcode.base);
			TempReg1 = m_Section->MipsRegLo(Opcode.base);
		} else {
			if (m_Section->IsMapped(Opcode.base)) { 
				ProtectGPR(m_Section,Opcode.base);
				if (Opcode.offset != 0) {
					TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
					LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
				} else {
					TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
				}
			} else {
				TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
		MoveX86regToVariable(TempReg1,_LLAddr,"LLAddr");
		AddX86regToVariable(TempReg2,_LLAddr,"LLAddr");
		SubConstFromVariable((DWORD)_MMU->Rdram(),_LLAddr,"LLAddr");
	} else {
		if (m_Section->IsMapped(Opcode.base)) { 
			ProtectGPR(m_Section,Opcode.base);
			if (Opcode.offset != 0) {
				Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
				LeaSourceAndOffset(m_Section->MipsRegLo(Opcode.rt),m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.base);
			}
		} else {
			Map_GPR_32bit(m_Section,Opcode.rt,TRUE,Opcode.base);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(m_Section->MipsRegLo(Opcode.rt));
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(m_Section->MipsRegLo(Opcode.rt));
			} else {
				AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),(short)Opcode.immediate);
			}
		}
		AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rt),0x1FFFFFFF);
		MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rt),_LLAddr,"LLAddr");
		MoveN64MemToX86reg(m_Section->MipsRegLo(Opcode.rt),m_Section->MipsRegLo(Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
	}
}

void CRecompilerOps::Compile_SC (CCodeSection * m_Section){
	DWORD TempReg1, TempReg2;
	BYTE * Jump;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
	CompConstToVariable(1,_LLBit,"LLBit");
	JneLabel32("LLBitNotSet",0);
	Jump = RecompPos - 4;
	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
			
		if (m_Section->IsConst(Opcode.rt)) {
			_MMU->Compile_SW_Const(m_Section->MipsRegLo(Opcode.rt), Address);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			_MMU->Compile_SW_Register(m_Section,m_Section->MipsRegLo(Opcode.rt), Address);
		} else {
			_MMU->Compile_SW_Register(m_Section,Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), Address);
		}
		CPU_Message("      LLBitNotSet:");
		*((DWORD *)(Jump))=(BYTE)(RecompPos - Jump - 4);
		Map_GPR_32bit(m_Section,Opcode.rt,FALSE,-1);
		MoveVariableToX86reg(_LLBit,"LLBit",m_Section->MipsRegLo(Opcode.rt));
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(m_Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regToX86regPointer(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToN64Mem(m_Section->MipsRegLo(Opcode.rt),TempReg1);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToN64Mem(m_Section->MipsRegLo(Opcode.rt),TempReg1);
		} else {	
			MoveX86regToN64Mem(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE),TempReg1);
		}
	}
	CPU_Message("      LLBitNotSet:");
	*((DWORD *)(Jump))=(BYTE)(RecompPos - Jump - 4);
	Map_GPR_32bit(m_Section,Opcode.rt,FALSE,-1);
	MoveVariableToX86reg(_LLBit,"LLBit",m_Section->MipsRegLo(Opcode.rt));

}

void CRecompilerOps::Compile_LD (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rt == 0) return;
	
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_64bit(m_Section,Opcode.rt,-1);
		_MMU->Compile_LW(m_Section, m_Section->MipsRegHi(Opcode.rt),Address);
		_MMU->Compile_LW(m_Section, m_Section->MipsRegLo(Opcode.rt),Address + 4);
		if (SPHack && Opcode.rt == 29) { _MMU->ResetMemoryStack(m_Section); }
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base) && Opcode.offset == 0) { 
		if (UseTlb) {
			ProtectGPR(m_Section,Opcode.base);
			TempReg1 = m_Section->MipsRegLo(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		if (m_Section->IsMapped(Opcode.base)) { 
			ProtectGPR(m_Section,Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
			}
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
		Map_GPR_64bit(m_Section,Opcode.rt,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,m_Section->MipsRegHi(Opcode.rt));
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,m_Section->MipsRegLo(Opcode.rt),4);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_64bit(m_Section,Opcode.rt,-1);
		MoveN64MemToX86reg(m_Section->MipsRegHi(Opcode.rt),TempReg1);
		MoveN64MemDispToX86reg(m_Section->MipsRegLo(Opcode.rt),TempReg1,4);
	}
	if (SPHack && Opcode.rt == 29) { 		
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::Compile_SD (CCodeSection * m_Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
	if (m_Section->IsConst(Opcode.base)) { 
		DWORD Address = m_Section->MipsRegLo(Opcode.base) + (short)Opcode.offset;
		
		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rt)) {
				_MMU->Compile_SW_Const(m_Section->MipsRegHi(Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Const((m_Section->MipsRegLo_S(Opcode.rt) >> 31), Address);
			}
			_MMU->Compile_SW_Const(m_Section->MipsRegLo(Opcode.rt), Address + 4);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rt)) {
				_MMU->Compile_SW_Register(m_Section,m_Section->MipsRegHi(Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Register(m_Section,Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE), Address);
			}
			_MMU->Compile_SW_Register(m_Section,m_Section->MipsRegLo(Opcode.rt), Address + 4);		
		} else {
			_MMU->Compile_SW_Register(m_Section,TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE), Address);
			_MMU->Compile_SW_Register(m_Section,Map_TempReg(m_Section,TempReg1,Opcode.rt,FALSE), Address + 4);		
		}
		return;
	}
	if (m_Section->IsMapped(Opcode.rt)) { ProtectGPR(m_Section,Opcode.rt); }
	if (m_Section->IsMapped(Opcode.base)) { 
		ProtectGPR(m_Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,m_Section->MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(m_Section,x86_Any,Opcode.base,FALSE);
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
		TempReg2 = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rt)) {
				MoveConstToX86regPointer(m_Section->MipsRegHi(Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveConstToX86regPointer((m_Section->MipsRegLo_S(Opcode.rt) >> 31),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveConstToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rt)) {
				MoveX86regToX86regPointer(m_Section->MipsRegHi(Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveX86regToX86regPointer(Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(m_Section->MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			int X86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE);
			MoveX86regToX86regPointer(X86Reg,TempReg1, TempReg2);
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(Map_TempReg(m_Section,X86Reg,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rt)) {
				MoveConstToN64Mem(m_Section->MipsRegHi(Opcode.rt),TempReg1);
			} else if (m_Section->IsSigned(Opcode.rt)) {
				MoveConstToN64Mem(((int)m_Section->MipsRegLo(Opcode.rt) >> 31),TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveConstToN64MemDisp(m_Section->MipsRegLo(Opcode.rt),TempReg1,4);
		} else if (m_Section->IsKnown(Opcode.rt) && m_Section->IsMapped(Opcode.rt)) {
			if (m_Section->Is64Bit(Opcode.rt)) {
				MoveX86regToN64Mem(m_Section->MipsRegHi(Opcode.rt),TempReg1);
			} else if (m_Section->IsSigned(Opcode.rt)) {
				MoveX86regToN64Mem(Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE), TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveX86regToN64MemDisp(m_Section->MipsRegLo(Opcode.rt),TempReg1, 4);		
		} else {	
			int x86reg;
			MoveX86regToN64Mem(x86reg = Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE), TempReg1);
			MoveX86regToN64MemDisp(Map_TempReg(m_Section,x86reg,Opcode.rt,FALSE), TempReg1,4);
		}
	}
}

/********************** R4300i OpCodes: Special **********************/
void CRecompilerOps::Compile_SPECIAL_SLL (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) << Opcode.sa;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	if (Opcode.rd != Opcode.rt && m_Section->IsMapped(Opcode.rt)) {
		switch (Opcode.sa) {
		case 0: 
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
			break;
		case 1:
			ProtectGPR(m_Section,Opcode.rt);
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE,-1);
			LeaRegReg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt), 2);
			break;			
		case 2:
			ProtectGPR(m_Section,Opcode.rt);
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE,-1);
			LeaRegReg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt), 4);
			break;			
		case 3:
			ProtectGPR(m_Section,Opcode.rt);
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE,-1);
			LeaRegReg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt), 8);
			break;
		default:
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
			ShiftLeftSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	} else {
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftLeftSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
	}
}

void CRecompilerOps::Compile_SPECIAL_SRL (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) >> Opcode.sa;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightUnsignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void CRecompilerOps::Compile_SPECIAL_SRA (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void CRecompilerOps::Compile_SPECIAL_SLLV (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsConst(Opcode.rs)) {
		DWORD Shift = (m_Section->MipsRegLo(Opcode.rs) & 0x1F);
		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) << Shift;
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
			ShiftLeftSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Shift);
		}
		return;
	}
	Map_TempReg(m_Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftLeftSign(m_Section->MipsRegLo(Opcode.rd));
}

void CRecompilerOps::Compile_SPECIAL_SRLV (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsKnown(Opcode.rs) && m_Section->IsConst(Opcode.rs)) {
		DWORD Shift = (m_Section->MipsRegLo(Opcode.rs) & 0x1F);
		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) >> Shift;
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftRightUnsignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(m_Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightUnsign(m_Section->MipsRegLo(Opcode.rd));
}

void CRecompilerOps::Compile_SPECIAL_SRAV (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsKnown(Opcode.rs) && m_Section->IsConst(Opcode.rs)) {
		DWORD Shift = (m_Section->MipsRegLo(Opcode.rs) & 0x1F);
		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo_S(Opcode.rt) >> Shift;
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftRightSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(m_Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightSign(m_Section->MipsRegLo(Opcode.rd));
}

void CRecompilerOps::Compile_SPECIAL_JR (CCodeSection * m_Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
		if (m_Section->IsConst(Opcode.rs)) { 
			sprintf(JumpLabel,"0x%08X",m_Section->MipsRegLo(Opcode.rs));
			m_Section->Jump.BranchLabel   = JumpLabel;
			m_Section->Jump.TargetPC      = m_Section->MipsRegLo(Opcode.rs);
			m_Section->Jump.FallThrough   = TRUE;
			m_Section->Jump.LinkLocation  = NULL;
			m_Section->Jump.LinkLocation2 = NULL;
			m_Section->Cont.FallThrough   = FALSE;
			m_Section->Cont.LinkLocation  = NULL;
			m_Section->Cont.LinkLocation2 = NULL;
			if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
				NextInstruction = END_BLOCK;
				return;
			}
		}
		if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
			if (m_Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(m_Section->CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
			WriteBackRegisters(m_Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		if (DelaySlotEffectsCompare(m_Section->CompilePC,Opcode.rs,0)) {
			if (m_Section->IsConst(Opcode.rs)) { 
				MoveConstToVariable(m_Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else 	if (m_Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (DelaySlotEffectsCompare(m_Section->CompilePC,Opcode.rs,0)) {
			_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC,(DWORD)-1,m_Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
		} else {
			if (m_Section->IsConst(Opcode.rs)) { 
				memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			} else {
				if (m_Section->IsMapped(Opcode.rs)) { 
					MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				} else {
					MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				}
				_N64System->GetRecompiler()->CompileExit (m_Section,-1, (DWORD)-1,m_Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
			}
		}
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_SPECIAL_JALR (CCodeSection * m_Section) {
	static char JumpLabel[100];	
	
	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
		if (DelaySlotEffectsCompare(m_Section->CompilePC,Opcode.rs,0)) {
			CRecompilerOps::Compile_UnknownOpcode(m_Section);
		}
		UnMap_GPR(m_Section, Opcode.rd, FALSE);
		m_Section->MipsRegLo(Opcode.rd) = m_Section->CompilePC + 8;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		if ((m_Section->CompilePC & 0xFFC) == 0xFFC) {
			if (m_Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(m_Section->CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
			WriteBackRegisters(m_Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (m_Section->IsConst(Opcode.rs)) { 
			memcpy(&m_Section->Jump.RegSet,&m_Section->RegWorking,sizeof(CRegInfo));
			sprintf(JumpLabel,"0x%08X",m_Section->MipsRegLo(Opcode.rs));
			m_Section->Jump.BranchLabel   = JumpLabel;
			m_Section->Jump.TargetPC      = m_Section->MipsRegLo(Opcode.rs);
			m_Section->Jump.FallThrough   = TRUE;
			m_Section->Jump.LinkLocation  = NULL;
			m_Section->Jump.LinkLocation2 = NULL;
			m_Section->Cont.FallThrough   = FALSE;
			m_Section->Cont.LinkLocation  = NULL;
			m_Section->Cont.LinkLocation2 = NULL;

			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		} else {
			if (m_Section->IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
			_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC, (DWORD)-1,m_Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
		}
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_SPECIAL_SYSCALL (CCodeSection * m_Section) {
	_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC,m_Section->CompilePC,m_Section->RegWorking,CExitInfo::DoSysCall,TRUE,NULL);
}

void CRecompilerOps::Compile_SPECIAL_MFLO (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	Map_GPR_64bit(m_Section,Opcode.rd,-1);
	MoveVariableToX86reg(&_RegLO->UW[0],"_RegLO->UW[0]",m_Section->MipsRegLo(Opcode.rd));
	MoveVariableToX86reg(&_RegLO->UW[1],"_RegLO->UW[1]",m_Section->MipsRegHi(Opcode.rd));
}

void CRecompilerOps::Compile_SPECIAL_MTLO (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsKnown(Opcode.rs) && m_Section->IsConst(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			MoveConstToVariable(m_Section->MipsRegHi(Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (m_Section->IsSigned(Opcode.rs) && ((m_Section->MipsRegLo(Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveConstToVariable(m_Section->MipsRegLo(Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else if (m_Section->IsKnown(Opcode.rs) && m_Section->IsMapped(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			MoveX86regToVariable(m_Section->MipsRegHi(Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (m_Section->IsSigned(Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else {
		int x86reg = Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&_RegLO->UW[1],"_RegLO->UW[1]");
		MoveX86regToVariable(Map_TempReg(m_Section,x86reg,Opcode.rs,FALSE), &_RegLO->UW[0],"_RegLO->UW[0]");
	}
}

void CRecompilerOps::Compile_SPECIAL_MFHI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	Map_GPR_64bit(m_Section,Opcode.rd,-1);
	MoveVariableToX86reg(&_RegHI->UW[0],"_RegHI->UW[0]",m_Section->MipsRegLo(Opcode.rd));
	MoveVariableToX86reg(&_RegHI->UW[1],"_RegHI->UW[1]",m_Section->MipsRegHi(Opcode.rd));
}

void CRecompilerOps::Compile_SPECIAL_MTHI (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (m_Section->IsKnown(Opcode.rs) && m_Section->IsConst(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			MoveConstToVariable(m_Section->MipsRegHi(Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (m_Section->IsSigned(Opcode.rs) && ((m_Section->MipsRegLo(Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveConstToVariable(m_Section->MipsRegLo(Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else if (m_Section->IsKnown(Opcode.rs) && m_Section->IsMapped(Opcode.rs)) {
		if (m_Section->Is64Bit(Opcode.rs)) {
			MoveX86regToVariable(m_Section->MipsRegHi(Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (m_Section->IsSigned(Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else {
		int x86reg = Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&_RegHI->UW[1],"_RegHI->UW[1]");
		MoveX86regToVariable(Map_TempReg(m_Section,x86reg,Opcode.rs,FALSE), &_RegHI->UW[0],"_RegHI->UW[0]");
	}
}

void CRecompilerOps::Compile_SPECIAL_DSLLV (CCodeSection * m_Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsConst(Opcode.rs)) {
		DWORD Shift = (m_Section->MipsRegLo(Opcode.rs) & 0x3F);
		CRecompilerOps::Compile_UnknownOpcode(m_Section);
		return;
	}
	Map_TempReg(m_Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftLeftDouble(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegLo(Opcode.rd));
	ShiftLeftSign(m_Section->MipsRegLo(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegHi(Opcode.rd));
	XorX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftLeftSign(m_Section->MipsRegHi(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void CRecompilerOps::Compile_SPECIAL_DSRLV (CCodeSection * m_Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsConst(Opcode.rs)) {
		DWORD Shift = (m_Section->MipsRegLo(Opcode.rs) & 0x3F);
		if (m_Section->IsConst(Opcode.rt)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			m_Section->MipsReg(Opcode.rd) = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt);
			m_Section->MipsReg(Opcode.rd) = m_Section->MipsReg(Opcode.rd) >> Shift;
			if ((m_Section->MipsRegHi(Opcode.rd) == 0) && (m_Section->MipsRegLo(Opcode.rd) & 0x80000000) == 0) {
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else if ((m_Section->MipsRegHi(Opcode.rd) == 0xFFFFFFFF) && (m_Section->MipsRegLo(Opcode.rd) & 0x80000000) != 0) {
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else {
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
			}
			return;
		}
		//if (Shift < 0x20) {
		//} else {
		//}
		//CRecompilerOps::Compile_UnknownOpcode(m_Section);
		//return;
	}
	Map_TempReg(m_Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftRightDouble(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegHi(Opcode.rd));
	ShiftRightUnsign(m_Section->MipsRegHi(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegLo(Opcode.rd));
	XorX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightUnsign(m_Section->MipsRegLo(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void CRecompilerOps::Compile_SPECIAL_DSRAV (CCodeSection * m_Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (m_Section->IsConst(Opcode.rs)) {
		DWORD Shift = (m_Section->MipsRegLo(Opcode.rs) & 0x3F);
		CRecompilerOps::Compile_UnknownOpcode(m_Section);
		return;
	}
	Map_TempReg(m_Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftRightDouble(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegHi(Opcode.rd));
	ShiftRightSign(m_Section->MipsRegHi(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegLo(Opcode.rd));
	ShiftRightSignImmed(m_Section->MipsRegHi(Opcode.rd),0x1F);
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightSign(m_Section->MipsRegLo(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void CRecompilerOps::Compile_SPECIAL_MULT ( CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	m_Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,FALSE);
	m_Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(m_Section,x86_EDX,Opcode.rt,FALSE);

	imulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
}

void CRecompilerOps::Compile_SPECIAL_MULTU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	m_Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,FALSE);
	m_Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(m_Section,x86_EDX,Opcode.rt,FALSE);

	MulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
}

void CRecompilerOps::Compile_SPECIAL_DIV (CCodeSection * m_Section) {
	BYTE *Jump[2];

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->MipsRegLo(Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (m_Section->IsMapped(Opcode.rt)) {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rt),0);
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

	m_Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,FALSE);

	/* edx is the signed portion to eax */
	m_Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(m_Section,x86_EDX, -1, FALSE);

	MoveX86RegToX86Reg(x86_EAX, x86_EDX);
	ShiftRightSignImmed(x86_EDX,31);

	if (m_Section->IsMapped(Opcode.rt)) {
		idivX86reg(m_Section->MipsRegLo(Opcode.rt));
	} else {
		idivX86reg(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE));
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

void CRecompilerOps::Compile_SPECIAL_DIVU ( CCodeSection * m_Section) {
	BYTE *Jump[2];
	int x86reg;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->MipsRegLo(Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (m_Section->IsMapped(Opcode.rt)) {
			CompConstToX86reg(m_Section->MipsRegLo(Opcode.rt),0);
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

	m_Section->x86Protected(x86_EAX) = TRUE;
	Map_TempReg(m_Section,x86_EDX, 0, FALSE);
	m_Section->x86Protected(x86_EAX) = FALSE;

	Map_TempReg(m_Section,x86_EAX,Opcode.rs,FALSE);
	x86reg = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);

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

void CRecompilerOps::Compile_SPECIAL_DMULT (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rs != 0) { UnMap_GPR(m_Section,Opcode.rs,TRUE); }
	if (Opcode.rs != 0) { UnMap_GPR(m_Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
	Popad();
}

void CRecompilerOps::Compile_SPECIAL_DMULTU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	/* _RegLO->UDW = (uint64)_GPR[Opcode.rs].UW[0] * (uint64)_GPR[Opcode.rt].UW[0]; */
	m_Section->x86Protected(x86_EDX) = TRUE;
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,FALSE);
	m_Section->x86Protected(x86_EDX) = FALSE;
	Map_TempReg(m_Section,x86_EDX,Opcode.rt,FALSE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

	/* _RegHI->UDW = (uint64)_GPR[Opcode.rs].UW[1] * (uint64)_GPR[Opcode.rt].UW[1]; */
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,TRUE);
	Map_TempReg(m_Section,x86_EDX,Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

	/* Tmp[0].UDW = (uint64)_GPR[Opcode.rs].UW[1] * (uint64)_GPR[Opcode.rt].UW[0]; */
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,TRUE);
	Map_TempReg(m_Section,x86_EDX,Opcode.rt,FALSE);

	Map_TempReg(m_Section,x86_EBX,-1,FALSE);
	Map_TempReg(m_Section,x86_ECX,-1,FALSE);

	MulX86reg(x86_EDX);
	MoveX86RegToX86Reg(x86_EAX, x86_EBX); /* EDX:EAX -> ECX:EBX */
	MoveX86RegToX86Reg(x86_EDX, x86_ECX);

	/* Tmp[1].UDW = (uint64)_GPR[Opcode.rs].UW[0] * (uint64)_GPR[Opcode.rt].UW[1]; */
	Map_TempReg(m_Section,x86_EAX,Opcode.rs,FALSE);
	Map_TempReg(m_Section,x86_EDX,Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	Map_TempReg(m_Section,x86_ESI,-1,FALSE);
	Map_TempReg(m_Section,x86_EDI,-1,FALSE);
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

void CRecompilerOps::Compile_SPECIAL_DDIV (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	UnMap_GPR(m_Section,Opcode.rs,TRUE);
	UnMap_GPR(m_Section,Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
	Popad();
}

void CRecompilerOps::Compile_SPECIAL_DDIVU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	UnMap_GPR(m_Section,Opcode.rs,TRUE);
	UnMap_GPR(m_Section,Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
	Popad();
}

void CRecompilerOps::Compile_SPECIAL_ADD (CCodeSection * m_Section) {
	int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
	int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(source1) && m_Section->IsConst(source2)) {
		DWORD temp = m_Section->MipsRegLo(source1) + m_Section->MipsRegLo(source2);
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = temp;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(m_Section,Opcode.rd,TRUE, source1);
	if (m_Section->IsConst(source2)) {
		if (m_Section->MipsRegLo(source2) != 0) {
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
		}
	} else if (m_Section->IsKnown(source2) && m_Section->IsMapped(source2)) {
		AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
	} else {
		AddVariableToX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
}

void CRecompilerOps::Compile_SPECIAL_ADDU (CCodeSection * m_Section) {
	int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
	int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(source1) && m_Section->IsConst(source2)) {
		DWORD temp = m_Section->MipsRegLo(source1) + m_Section->MipsRegLo(source2);
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = temp;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(m_Section,Opcode.rd,TRUE, source1);
	if (m_Section->IsConst(source2)) {
		if (m_Section->MipsRegLo(source2) != 0) {
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
		}
	} else if (m_Section->IsKnown(source2) && m_Section->IsMapped(source2)) {
		AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
	} else {
		AddVariableToX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
}

void CRecompilerOps::Compile_SPECIAL_SUB (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(Opcode.rt)  && m_Section->IsConst(Opcode.rs)) {
		DWORD temp = m_Section->MipsRegLo(Opcode.rs) - m_Section->MipsRegLo(Opcode.rt);
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = temp;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (Opcode.rd == Opcode.rt) {
			int x86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE, Opcode.rs);
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE, Opcode.rs);
		if (m_Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
		} else if (m_Section->IsMapped(Opcode.rt)) {
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
		} else {
			SubVariableFromX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_SUBU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(Opcode.rt)  && m_Section->IsConst(Opcode.rs)) {
		DWORD temp = m_Section->MipsRegLo(Opcode.rs) - m_Section->MipsRegLo(Opcode.rt);
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegLo(Opcode.rd) = temp;
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (Opcode.rd == Opcode.rt) {
			int x86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_32bit(m_Section,Opcode.rd,TRUE, Opcode.rs);
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE, Opcode.rs);
		if (m_Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
		} else if (m_Section->IsMapped(Opcode.rt)) {
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
		} else {
			SubVariableFromX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_AND (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (m_Section->IsKnown(Opcode.rt) && m_Section->IsKnown(Opcode.rs)) {
		if (m_Section->IsConst(Opcode.rt) && m_Section->IsConst(Opcode.rs)) {
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				m_Section->MipsReg(Opcode.rd) = 
					(m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt)) &
					(m_Section->Is64Bit(Opcode.rs)?m_Section->MipsReg(Opcode.rs):(_int64)m_Section->MipsRegLo_S(Opcode.rs));
				
				if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
					m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
					m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				m_Section->MipsReg(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) & m_Section->MipsReg(Opcode.rs);
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			}			
		} else if (m_Section->IsMapped(Opcode.rt) && m_Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
		
			ProtectGPR(m_Section,source1);
			ProtectGPR(m_Section,source2);
			if (m_Section->Is32Bit(source1) && m_Section->Is32Bit(source2)) {
				int Sign = (m_Section->IsSigned(Opcode.rt) && m_Section->IsSigned(Opcode.rs))?TRUE:FALSE;
				Map_GPR_32bit(m_Section,Opcode.rd,Sign,source1);				
				AndX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			} else if (m_Section->Is32Bit(source1) || m_Section->Is32Bit(source2)) {
				if (m_Section->IsUnsigned(m_Section->Is32Bit(source1)?source1:source2)) {
					Map_GPR_32bit(m_Section,Opcode.rd,FALSE,source1);
					AndX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
				} else {
					Map_GPR_64bit(m_Section,Opcode.rd,source1);
					if (m_Section->Is32Bit(source2)) {
						AndX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),Map_TempReg(m_Section,x86_Any,source2,TRUE));
					} else {
						AndX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
					}
					AndX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
				}
			} else {
				Map_GPR_64bit(m_Section,Opcode.rd,source1);
				AndX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
				AndX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			}
		} else {
			int ConstReg = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			int MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (m_Section->Is64Bit(ConstReg)) {
				if (m_Section->Is32Bit(MappedReg) && m_Section->IsUnsigned(MappedReg)) {
					if (m_Section->MipsRegLo(ConstReg) == 0) {
						Map_GPR_32bit(m_Section,Opcode.rd,FALSE, 0);
					} else {
						DWORD Value = m_Section->MipsRegLo(ConstReg);
						Map_GPR_32bit(m_Section,Opcode.rd,FALSE, MappedReg);
						AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),Value);
					}
				} else {
					_int64 Value = m_Section->MipsReg(ConstReg);
					Map_GPR_64bit(m_Section,Opcode.rd,MappedReg);
					AndConstToX86Reg(m_Section->MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
					AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),(DWORD)Value);
				}
			} else if (m_Section->Is64Bit(MappedReg)) {
				DWORD Value = m_Section->MipsRegLo(ConstReg); 
				if (Value != 0) {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(ConstReg)?TRUE:FALSE,MappedReg);					
					AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),(DWORD)Value);
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(ConstReg)?TRUE:FALSE, 0);
				}
			} else {
				DWORD Value = m_Section->MipsRegLo(ConstReg); 
				int Sign = FALSE;
				if (m_Section->IsSigned(ConstReg) && m_Section->IsSigned(MappedReg)) { Sign = TRUE; }				
				if (Value != 0) {
					Map_GPR_32bit(m_Section,Opcode.rd,Sign,MappedReg);
					AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),Value);
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,FALSE, 0);
				}
			}
		}
	} else if (m_Section->IsKnown(Opcode.rt) || m_Section->IsKnown(Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (m_Section->IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				unsigned __int64 Value = m_Section->MipsReg(KnownReg);
				Map_GPR_64bit(m_Section,Opcode.rd,UnknownReg);
				AndConstToX86Reg(m_Section->MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
				AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),(DWORD)Value);
			} else {
				DWORD Value = m_Section->MipsRegLo(KnownReg);
				Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(KnownReg),UnknownReg);
				AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),(DWORD)Value);
			}
		} else {
			ProtectGPR(m_Section,KnownReg);
			if (KnownReg == Opcode.rd) {
				if (m_Section->Is64Bit(KnownReg)) {
					Map_GPR_64bit(m_Section,Opcode.rd,KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],m_Section->MipsRegHi(Opcode.rd));
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],m_Section->MipsRegLo(Opcode.rd));
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(KnownReg),KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],m_Section->MipsRegLo(Opcode.rd));
				}
			} else {
				if (m_Section->Is64Bit(KnownReg)) {
					Map_GPR_64bit(m_Section,Opcode.rd,UnknownReg);
					AndX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(KnownReg));
					AndX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(KnownReg));
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(KnownReg),UnknownReg);
					AndX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(KnownReg));
				}
			}
		}
	} else {
		Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
		AndVariableToX86Reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],m_Section->MipsRegHi(Opcode.rd));
		AndVariableToX86Reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],m_Section->MipsRegLo(Opcode.rd));
	}
}

void CRecompilerOps::Compile_SPECIAL_OR (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsKnown(Opcode.rt) && m_Section->IsKnown(Opcode.rs)) {
		if (m_Section->IsConst(Opcode.rt) && m_Section->IsConst(Opcode.rs)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {				
				m_Section->MipsReg(Opcode.rd) = 
					(m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt)) |
					(m_Section->Is64Bit(Opcode.rs)?m_Section->MipsReg(Opcode.rs):(_int64)m_Section->MipsRegLo_S(Opcode.rs));
				if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
					m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
					m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) | m_Section->MipsRegLo(Opcode.rs);
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (m_Section->IsMapped(Opcode.rt) && m_Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(m_Section,Opcode.rt);
			ProtectGPR(m_Section,Opcode.rs);
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(m_Section,Opcode.rd,source1);
				if (m_Section->Is64Bit(source2)) {
					OrX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),Map_TempReg(m_Section,x86_Any,source2,TRUE));
				}
			} else {
				ProtectGPR(m_Section,source2);
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE,source1);
			}
			OrX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
		} else {
			DWORD ConstReg = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				unsigned _int64 Value;

				if (m_Section->Is64Bit(ConstReg)) {
					Value = m_Section->MipsReg(ConstReg);
				} else {
					Value = m_Section->IsSigned(ConstReg)?m_Section->MipsRegLo_S(ConstReg):m_Section->MipsRegLo(ConstReg);
				}
				Map_GPR_64bit(m_Section,Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),m_Section->MipsRegHi(Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,m_Section->MipsRegLo(Opcode.rd));
				}
			} else {
				int Value = m_Section->MipsRegLo(ConstReg);
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, MappedReg);
				if (Value != 0) { OrConstToX86Reg(Value,m_Section->MipsRegLo(Opcode.rd)); }
			}
		}
	} else if (m_Section->IsKnown(Opcode.rt) || m_Section->IsKnown(Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (m_Section->IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = m_Section->Is64Bit(KnownReg)?m_Section->MipsReg(KnownReg):m_Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(m_Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),m_Section->MipsRegHi(Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,m_Section->MipsRegLo(Opcode.rd));
			}
		} else {
			Map_GPR_64bit(m_Section,Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],m_Section->MipsRegHi(Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],m_Section->MipsRegLo(Opcode.rd));
		}
	} else {
		Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],m_Section->MipsRegHi(Opcode.rd));
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],m_Section->MipsRegLo(Opcode.rd));
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && Opcode.rd == 29) { 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif

}

void CRecompilerOps::Compile_SPECIAL_XOR (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Opcode.rt == Opcode.rs) {
		UnMap_GPR(m_Section, Opcode.rd, FALSE);
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		m_Section->MipsRegLo(Opcode.rd) = 0;
		return;
	}
	if (m_Section->IsKnown(Opcode.rt) && m_Section->IsKnown(Opcode.rs)) {
		if (m_Section->IsConst(Opcode.rt) && m_Section->IsConst(Opcode.rs)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("XOR 1");
#endif
				CRecompilerOps::Compile_UnknownOpcode(m_Section);
			} else {
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				m_Section->MipsRegLo(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) ^ m_Section->MipsRegLo(Opcode.rs);
			}
		} else if (m_Section->IsMapped(Opcode.rt) && m_Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(m_Section,source1);
			ProtectGPR(m_Section,source2);
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(m_Section,Opcode.rd,source1);
				if (m_Section->Is64Bit(source2)) {
					XorX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
				} else if (m_Section->IsSigned(source2)) {
					XorX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),Map_TempReg(m_Section,x86_Any,source2,TRUE));
				}
				XorX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			} else {
				if (m_Section->IsSigned(Opcode.rt) != m_Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(m_Section,Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(Opcode.rt),source1);
				}
				XorX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			}
		} else {
			DWORD ConstReg = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				DWORD ConstHi, ConstLo;

				ConstHi = m_Section->Is32Bit(ConstReg)?(DWORD)(m_Section->MipsRegLo_S(ConstReg) >> 31):m_Section->MipsRegHi(ConstReg);
				ConstLo = m_Section->MipsRegLo(ConstReg);
				Map_GPR_64bit(m_Section,Opcode.rd,MappedReg);
				if (ConstHi != 0) { XorConstToX86Reg(m_Section->MipsRegHi(Opcode.rd),ConstHi); }
				if (ConstLo != 0) { XorConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),ConstLo); }
			} else {
				int Value = m_Section->MipsRegLo(ConstReg);
				if (m_Section->IsSigned(Opcode.rt) != m_Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(m_Section,Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { XorConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),Value); }
			}
		}
	} else if (m_Section->IsKnown(Opcode.rt) || m_Section->IsKnown(Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (m_Section->IsConst(KnownReg)) {
			unsigned _int64 Value;

			if (m_Section->Is64Bit(KnownReg)) {
				Value = m_Section->MipsReg(KnownReg);
			} else {
				if (m_Section->IsSigned(KnownReg)) {
					Value = (int)m_Section->MipsRegLo(KnownReg);
				} else {
					Value = m_Section->MipsRegLo(KnownReg);
				}
			}
			Map_GPR_64bit(m_Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				XorConstToX86Reg(m_Section->MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
			}
			if ((DWORD)Value != 0) {
				XorConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),(DWORD)Value);
			}
		} else {
			Map_GPR_64bit(m_Section,Opcode.rd,KnownReg);
			XorVariableToX86reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],m_Section->MipsRegHi(Opcode.rd));
			XorVariableToX86reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],m_Section->MipsRegLo(Opcode.rd));
		}
	} else {
		Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
		XorVariableToX86reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],m_Section->MipsRegHi(Opcode.rd));
		XorVariableToX86reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],m_Section->MipsRegLo(Opcode.rd));
	}
}

void CRecompilerOps::Compile_SPECIAL_NOR (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsKnown(Opcode.rt) && m_Section->IsKnown(Opcode.rs)) {
		if (m_Section->IsConst(Opcode.rt) && m_Section->IsConst(Opcode.rs)) {
			if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {				
				CRecompilerOps::Compile_UnknownOpcode(m_Section);
			} else {
				m_Section->MipsRegLo(Opcode.rd) = ~(m_Section->MipsRegLo(Opcode.rt) | m_Section->MipsRegLo(Opcode.rs));
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (m_Section->IsMapped(Opcode.rt) && m_Section->IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(m_Section,source1);
			ProtectGPR(m_Section,source2);
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(m_Section,Opcode.rd,source1);
				if (m_Section->Is64Bit(source2)) {
					OrX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),Map_TempReg(m_Section,x86_Any,source2,TRUE));
				}
				OrX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
				NotX86Reg(m_Section->MipsRegHi(Opcode.rd));
				NotX86Reg(m_Section->MipsRegLo(Opcode.rd));
			} else {
				ProtectGPR(m_Section,source2);
				if (m_Section->IsSigned(Opcode.rt) != m_Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(m_Section,Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(Opcode.rt),source1);
				}
				OrX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
				NotX86Reg(m_Section->MipsRegLo(Opcode.rd));
			}
		} else {
			DWORD ConstReg = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				unsigned _int64 Value;

				if (m_Section->Is64Bit(ConstReg)) {
					Value = m_Section->MipsReg(ConstReg);
				} else {
					Value = m_Section->IsSigned(ConstReg)?m_Section->MipsRegLo_S(ConstReg):m_Section->MipsRegLo(ConstReg);
				}
				Map_GPR_64bit(m_Section,Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),m_Section->MipsRegHi(Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,m_Section->MipsRegLo(Opcode.rd));
				}
				NotX86Reg(m_Section->MipsRegHi(Opcode.rd));
				NotX86Reg(m_Section->MipsRegLo(Opcode.rd));
			} else {
				int Value = m_Section->MipsRegLo(ConstReg);
				if (m_Section->IsSigned(Opcode.rt) != m_Section->IsSigned(Opcode.rs)) {
					Map_GPR_32bit(m_Section,Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(m_Section,Opcode.rd,m_Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { OrConstToX86Reg(Value,m_Section->MipsRegLo(Opcode.rd)); }
				NotX86Reg(m_Section->MipsRegLo(Opcode.rd));
			}
		}
	} else if (m_Section->IsKnown(Opcode.rt) || m_Section->IsKnown(Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (m_Section->IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = m_Section->Is64Bit(KnownReg)?m_Section->MipsReg(KnownReg):m_Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(m_Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),m_Section->MipsRegHi(Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,m_Section->MipsRegLo(Opcode.rd));
			}
		} else {
			Map_GPR_64bit(m_Section,Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],m_Section->MipsRegHi(Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],m_Section->MipsRegLo(Opcode.rd));
		}
		NotX86Reg(m_Section->MipsRegHi(Opcode.rd));
		NotX86Reg(m_Section->MipsRegLo(Opcode.rd));
	} else {
		Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[1],CRegName::GPR_Hi[Opcode.rs],m_Section->MipsRegHi(Opcode.rd));
		OrVariableToX86Reg(&_GPR[Opcode.rs].W[0],CRegName::GPR_Lo[Opcode.rs],m_Section->MipsRegLo(Opcode.rd));
		NotX86Reg(m_Section->MipsRegHi(Opcode.rd));
		NotX86Reg(m_Section->MipsRegLo(Opcode.rd));
	}
}

void CRecompilerOps::Compile_SPECIAL_SLT (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (m_Section->IsKnown(Opcode.rt) && m_Section->IsKnown(Opcode.rs)) {
		if (m_Section->IsConst(Opcode.rt) && m_Section->IsConst(Opcode.rs)) {
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				DisplayError("1");
				CRecompilerOps::Compile_UnknownOpcode(m_Section);
			} else {
				if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (m_Section->MipsRegLo_S(Opcode.rs) < m_Section->MipsRegLo_S(Opcode.rt)) {
					m_Section->MipsRegLo(Opcode.rd) = 1;
				} else {
					m_Section->MipsRegLo(Opcode.rd) = 0;
				}
			}
		} else if (m_Section->IsMapped(Opcode.rt) && m_Section->IsMapped(Opcode.rs)) {
			ProtectGPR(m_Section,Opcode.rt);
			ProtectGPR(m_Section,Opcode.rs);
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					m_Section->Is64Bit(Opcode.rs)?m_Section->MipsRegHi(Opcode.rs):Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE), 
					m_Section->Is64Bit(Opcode.rt)?m_Section->MipsRegHi(Opcode.rt):Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				SetlVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs), m_Section->MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
			} else {
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs), m_Section->MipsRegLo(Opcode.rt));

				if (m_Section->MipsRegLo(Opcode.rd) > x86_EDX) {
					SetlVariable(&BranchCompare,"BranchCompare");
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
				} else {					
					Setl(m_Section->MipsRegLo(Opcode.rd));
					AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd), 1);
				}
			}
		} else {
			DWORD ConstReg  = m_Section->IsConst(Opcode.rs)?Opcode.rs:Opcode.rt;
			DWORD MappedReg = m_Section->IsConst(Opcode.rs)?Opcode.rt:Opcode.rs;

			ProtectGPR(m_Section,MappedReg);
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompConstToX86reg(
					m_Section->Is64Bit(MappedReg)?m_Section->MipsRegHi(MappedReg):Map_TempReg(m_Section,x86_Any,MappedReg,TRUE), 
					m_Section->Is64Bit(ConstReg)?m_Section->MipsRegHi(ConstReg):(m_Section->MipsRegLo_S(ConstReg) >> 31)
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
				CompConstToX86reg(m_Section->MipsRegLo(MappedReg), m_Section->MipsRegLo(ConstReg));
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
			} else {
				DWORD Constant = m_Section->MipsRegLo(ConstReg);
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				CompConstToX86reg(m_Section->MipsRegLo(MappedReg), Constant);
			
				if (m_Section->MipsRegLo(Opcode.rd) > x86_EDX) {
					if (MappedReg == Opcode.rs) {
						SetlVariable(&BranchCompare,"BranchCompare");
					} else {
						SetgVariable(&BranchCompare,"BranchCompare");
					}
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
				} else {					
					if (MappedReg == Opcode.rs) {
						Setl(m_Section->MipsRegLo(Opcode.rd));
					} else {
						Setg(m_Section->MipsRegLo(Opcode.rd));
					}
					AndConstToX86Reg(m_Section->MipsRegLo(Opcode.rd), 1);
				}
			}
		}		
	} else if (m_Section->IsKnown(Opcode.rt) || m_Section->IsKnown(Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		BYTE *Jump[2];
			
		if (m_Section->IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(((int)m_Section->MipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(m_Section,KnownReg);
				CompX86regToVariable(Map_TempReg(m_Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		if (KnownReg == (m_Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetgVariable(&BranchCompare,"BranchCompare");
		} else {
			SetlVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		if (m_Section->IsConst(KnownReg)) {
			CompConstToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (m_Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(m_Section,x86Reg,Opcode.rs,FALSE),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
	}
}

void CRecompilerOps::Compile_SPECIAL_SLTU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsKnown(Opcode.rt) && m_Section->IsKnown(Opcode.rs)) {
		if (m_Section->IsConst(Opcode.rt) && m_Section->IsConst(Opcode.rs)) {
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("1");
#endif
				CRecompilerOps::Compile_UnknownOpcode(m_Section);
			} else {
				if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
				m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (m_Section->MipsRegLo(Opcode.rs) < m_Section->MipsRegLo(Opcode.rt)) {
					m_Section->MipsRegLo(Opcode.rd) = 1;
				} else {
					m_Section->MipsRegLo(Opcode.rd) = 0;
				}
			}
		} else if (m_Section->IsMapped(Opcode.rt) && m_Section->IsMapped(Opcode.rs)) {
			ProtectGPR(m_Section,Opcode.rt);
			ProtectGPR(m_Section,Opcode.rs);
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					m_Section->Is64Bit(Opcode.rs)?m_Section->MipsRegHi(Opcode.rs):Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE), 
					m_Section->Is64Bit(Opcode.rt)?m_Section->MipsRegHi(Opcode.rt):Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				SetbVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs), m_Section->MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
			} else {
				CompX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rs), m_Section->MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
			}
		} else {
			if (m_Section->Is64Bit(Opcode.rt) || m_Section->Is64Bit(Opcode.rs)) {
				DWORD MappedRegHi, MappedRegLo, ConstHi, ConstLo, MappedReg, ConstReg;
				BYTE *Jump[2];

				ConstReg  = m_Section->IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
				MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;
				
				ConstLo = m_Section->MipsRegLo(ConstReg);
				ConstHi = (int)ConstLo >> 31;
				if (m_Section->Is64Bit(ConstReg)) { ConstHi = m_Section->MipsRegHi(ConstReg); }

				ProtectGPR(m_Section,MappedReg);
				MappedRegLo = m_Section->MipsRegLo(MappedReg);
				MappedRegHi = m_Section->MipsRegHi(MappedReg);
				if (m_Section->Is32Bit(MappedReg)) {
					MappedRegHi = Map_TempReg(m_Section,x86_Any,MappedReg,TRUE);
				}

		
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
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
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
			} else {
				DWORD Const = m_Section->IsConst(Opcode.rs)?m_Section->MipsRegLo(Opcode.rs):m_Section->MipsRegLo(Opcode.rt);
				DWORD MappedReg = m_Section->IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

				CompConstToX86reg(m_Section->MipsRegLo(MappedReg), Const);
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
			}
		}		
	} else if (m_Section->IsKnown(Opcode.rt) || m_Section->IsKnown(Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		BYTE *Jump[2];
			
		if (m_Section->IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(((int)m_Section->MipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(m_Section->MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(m_Section,KnownReg);
				CompX86regToVariable(Map_TempReg(m_Section,x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		if (KnownReg == (m_Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		if (m_Section->IsConst(KnownReg)) {
			CompConstToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(m_Section->MipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (m_Section->IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = Map_TempReg(m_Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(m_Section,x86Reg,Opcode.rs,FALSE),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",m_Section->MipsRegLo(Opcode.rd));
	}
}

void CRecompilerOps::Compile_SPECIAL_DADD (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(Opcode.rt)  && m_Section->IsConst(Opcode.rs)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsReg(Opcode.rd) = 
			m_Section->Is64Bit(Opcode.rs)?m_Section->MipsReg(Opcode.rs):(_int64)m_Section->MipsRegLo_S(Opcode.rs) +
			m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt);		
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
		int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

		Map_GPR_64bit(m_Section,Opcode.rd,source1);
		if (m_Section->IsConst(source2)) {
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			AddConstToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
		} else if (m_Section->IsMapped(source2)) {
			int HiReg = m_Section->Is64Bit(source2)?m_Section->MipsRegHi(source2):Map_TempReg(m_Section,x86_Any,source2,TRUE);
			ProtectGPR(m_Section,source2);
			AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			AdcX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(m_Section->MipsRegHi(Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_DADDU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(Opcode.rt)  && m_Section->IsConst(Opcode.rs)) {
		__int64 ValRs = m_Section->Is64Bit(Opcode.rs)?m_Section->MipsReg(Opcode.rs):(_int64)m_Section->MipsRegLo_S(Opcode.rs);
		__int64 ValRt = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt);
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsReg(Opcode.rd) = ValRs + ValRt;
		if ((m_Section->MipsRegHi(Opcode.rd) == 0) && (m_Section->MipsRegLo(Opcode.rd) & 0x80000000) == 0) {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if ((m_Section->MipsRegHi(Opcode.rd) == 0xFFFFFFFF) && (m_Section->MipsRegLo(Opcode.rd) & 0x80000000) != 0) {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
		int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

		Map_GPR_64bit(m_Section,Opcode.rd,source1);
		if (m_Section->IsConst(source2)) {
			AddConstToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			AddConstToX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(source2));
		} else if (m_Section->IsMapped(source2)) {
			int HiReg = m_Section->Is64Bit(source2)?m_Section->MipsRegHi(source2):Map_TempReg(m_Section,x86_Any,source2,TRUE);
			ProtectGPR(m_Section,source2);
			AddX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(source2));
			AdcX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(m_Section->MipsRegHi(Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_DSUB (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(Opcode.rt)  && m_Section->IsConst(Opcode.rs)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsReg(Opcode.rd) = 
			m_Section->Is64Bit(Opcode.rs)?m_Section->MipsReg(Opcode.rs):(_int64)m_Section->MipsRegLo_S(Opcode.rs) -
			m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt);		
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (Opcode.rd == Opcode.rt) {
			int HiReg = Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE);
			int LoReg = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rs);
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),LoReg);
			SbbX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rs);
		if (m_Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
			SbbConstFromX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(Opcode.rt));
		} else if (m_Section->IsMapped(Opcode.rt)) {
			int HiReg = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsRegHi(Opcode.rt):Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(m_Section,Opcode.rt);
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
			SbbX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
			SbbVariableFromX86reg(m_Section->MipsRegHi(Opcode.rd),&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_DSUBU (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (m_Section->IsConst(Opcode.rt)  && m_Section->IsConst(Opcode.rs)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsReg(Opcode.rd) = 
			m_Section->Is64Bit(Opcode.rs)?m_Section->MipsReg(Opcode.rs):(_int64)m_Section->MipsRegLo_S(Opcode.rs) -
			m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt);		
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (Opcode.rd == Opcode.rt) {
			int HiReg = Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE);
			int LoReg = Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rs);
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),LoReg);
			SbbX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rs);
		if (m_Section->IsConst(Opcode.rt)) {
			SubConstFromX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
			SbbConstFromX86Reg(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegHi(Opcode.rt));
		} else if (m_Section->IsMapped(Opcode.rt)) {
			int HiReg = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsRegHi(Opcode.rt):Map_TempReg(m_Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(m_Section,Opcode.rt);
			SubX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rt));
			SbbX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(m_Section->MipsRegLo(Opcode.rd),&_GPR[Opcode.rt].W[0],CRegName::GPR_Lo[Opcode.rt]);
			SbbVariableFromX86reg(m_Section->MipsRegHi(Opcode.rd),&_GPR[Opcode.rt].W[1],CRegName::GPR_Hi[Opcode.rt]);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_DSLL (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }

		m_Section->MipsReg(Opcode.rd) = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt) << Opcode.sa;
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}
	
	Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
	ShiftLeftDoubleImmed(m_Section->MipsRegHi(Opcode.rd),m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
	ShiftLeftSignImmed(	m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void CRecompilerOps::Compile_SPECIAL_DSRL (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }

		m_Section->MipsReg(Opcode.rd) = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg(Opcode.rt):(QWORD)m_Section->MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
	ShiftRightDoubleImmed(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
	ShiftRightUnsignImmed(m_Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
}

void CRecompilerOps::Compile_SPECIAL_DSRA (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (m_Section->IsConst(Opcode.rt)) {
		if (m_Section->IsMapped(Opcode.rd)) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }

		m_Section->MipsReg_S(Opcode.rd) = m_Section->Is64Bit(Opcode.rt)?m_Section->MipsReg_S(Opcode.rt):(_int64)m_Section->MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(m_Section,Opcode.rd,Opcode.rt);
	ShiftRightDoubleImmed(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
	ShiftRightSignImmed(m_Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
}

void CRecompilerOps::Compile_SPECIAL_DSLL32 (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (m_Section->IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegHi(Opcode.rd) = m_Section->MipsRegLo(Opcode.rt) << Opcode.sa;
		m_Section->MipsRegLo(Opcode.rd) = 0;
		if (m_Section->MipsRegLo_S(Opcode.rd) < 0 && m_Section->MipsRegHi_S(Opcode.rd) == -1){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(Opcode.rd) >= 0 && m_Section->MipsRegHi_S(Opcode.rd) == 0){ 
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_64;
		}

	} else if (m_Section->IsMapped(Opcode.rt)) {
		ProtectGPR(m_Section,Opcode.rt);
		Map_GPR_64bit(m_Section,Opcode.rd,-1);		
		if (Opcode.rt != Opcode.rd) {
			MoveX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rt),m_Section->MipsRegHi(Opcode.rd));
		} else {
			int HiReg = m_Section->MipsRegHi(Opcode.rt);
			m_Section->MipsRegHi(Opcode.rt) = m_Section->MipsRegLo(Opcode.rt);
			m_Section->MipsRegLo(Opcode.rt) = HiReg;
		}
		if ((BYTE)Opcode.sa != 0) {
			ShiftLeftSignImmed(m_Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
		}
		XorX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rd));
	} else {
		Map_GPR_64bit(m_Section,Opcode.rd,-1);
		MoveVariableToX86reg(&_GPR[Opcode.rt],CRegName::GPR_Hi[Opcode.rt],m_Section->MipsRegHi(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftLeftSignImmed(m_Section->MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
		}
		XorX86RegToX86Reg(m_Section->MipsRegLo(Opcode.rd),m_Section->MipsRegLo(Opcode.rd));
	}
}

void CRecompilerOps::Compile_SPECIAL_DSRL32 (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (m_Section->IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		m_Section->MipsRegLo(Opcode.rd) = (DWORD)(m_Section->MipsReg(Opcode.rt) >> (Opcode.sa + 32));
	} else if (m_Section->IsMapped(Opcode.rt)) {
		ProtectGPR(m_Section,Opcode.rt);
		if (m_Section->Is64Bit(Opcode.rt)) {
			if (Opcode.rt == Opcode.rd) {
				int HiReg = m_Section->MipsRegHi(Opcode.rt);
				m_Section->MipsRegHi(Opcode.rt) = m_Section->MipsRegLo(Opcode.rt);
				m_Section->MipsRegLo(Opcode.rt) = HiReg;
				Map_GPR_32bit(m_Section,Opcode.rd,FALSE,-1);
			} else {
				Map_GPR_32bit(m_Section,Opcode.rd,FALSE,-1);
				MoveX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rt),m_Section->MipsRegLo(Opcode.rd));
			}
			if ((BYTE)Opcode.sa != 0) {
				ShiftRightUnsignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
			}
		} else {
			CRecompilerOps::Compile_UnknownOpcode(m_Section);
		}
	} else {
		Map_GPR_32bit(m_Section,Opcode.rd,FALSE,-1);
		MoveVariableToX86reg(&_GPR[Opcode.rt].UW[1],CRegName::GPR_Lo[Opcode.rt],m_Section->MipsRegLo(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftRightUnsignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	}
}

void CRecompilerOps::Compile_SPECIAL_DSRA32 (CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (m_Section->IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(m_Section,Opcode.rd, FALSE); }
		m_Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
		m_Section->MipsRegLo(Opcode.rd) = (DWORD)(m_Section->MipsReg_S(Opcode.rt) >> (Opcode.sa + 32));
	} else if (m_Section->IsMapped(Opcode.rt)) {
		ProtectGPR(m_Section,Opcode.rt);
		if (m_Section->Is64Bit(Opcode.rt)) {
			if (Opcode.rt == Opcode.rd) {
				int HiReg = m_Section->MipsRegHi(Opcode.rt);
				m_Section->MipsRegHi(Opcode.rt) = m_Section->MipsRegLo(Opcode.rt);
				m_Section->MipsRegLo(Opcode.rt) = HiReg;
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE,-1);
			} else {
				Map_GPR_32bit(m_Section,Opcode.rd,TRUE,-1);
				MoveX86RegToX86Reg(m_Section->MipsRegHi(Opcode.rt),m_Section->MipsRegLo(Opcode.rd));
			}
			if ((BYTE)Opcode.sa != 0) {
				ShiftRightSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
			}
		} else {
			CRecompilerOps::Compile_UnknownOpcode(m_Section);
		}
	} else {
		Map_GPR_32bit(m_Section,Opcode.rd,TRUE,-1);
		MoveVariableToX86reg(&_GPR[Opcode.rt].UW[1],CRegName::GPR_Lo[Opcode.rt],m_Section->MipsRegLo(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftRightSignImmed(m_Section->MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	}
}

/************************** COP0 functions **************************/
void CRecompilerOps::Compile_COP0_MF(CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	switch (Opcode.rd) {
	case 9: //Count
		_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
		m_Section->BlockCycleCount() = 0;
		m_Section->BlockRandomModifier() = 0;
	}
	Map_GPR_32bit(m_Section,Opcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_CP0[Opcode.rd],CRegName::Cop0[Opcode.rd],m_Section->MipsRegLo(Opcode.rt));
}

void CRecompilerOps::Compile_COP0_MT (CCodeSection * m_Section) {
	int OldStatusReg;
	BYTE *Jump;

	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

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
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		switch (Opcode.rd) {
		case 4: //Context
			AndConstToVariable(0xFF800000,&_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
			break;			
		case 11: //Compare
			_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
			m_Section->BlockCycleCount() = 0;
			m_Section->BlockRandomModifier() = 0;
			AndConstToVariable(~CAUSE_IP7,&_Reg->FAKE_CAUSE_REGISTER,"FAKE_CAUSE_REGISTER");
			Pushad();
			_Notify->BreakPoint(__FILE__,__LINE__);
			//Call_Direct(ChangeCompareTimer,"ChangeCompareTimer");
			Popad();
		}
		break;
	case 9: //Count
		_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
		m_Section->BlockCycleCount() = 0;
		m_Section->BlockRandomModifier() = 0;
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		Pushad();
		_Notify->BreakPoint(__FILE__,__LINE__);
		//Call_Direct(ChangeCompareTimer,"ChangeCompareTimer");
		Popad();
		break;
	case 12: //Status
		OldStatusReg = Map_TempReg(m_Section,x86_Any,-1,FALSE);
		MoveVariableToX86reg(&_CP0[Opcode.rd],CRegName::Cop0[Opcode.rd],OldStatusReg);
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
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
		//BreakPoint(__FILE__,__LINE__); //_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC+4,m_Section->RegWorking,ExitResetRecompCode,FALSE,JneLabel32);
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	case 6: //Wired
		Pushad();
		_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
		m_Section->BlockCycleCount() = 0;
		m_Section->BlockRandomModifier() = 0;
		Call_Direct(FixRandomReg,"FixRandomReg");
		Popad();
		if (m_Section->IsConst(Opcode.rt)) {
			MoveConstToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else if (m_Section->IsMapped(Opcode.rt)) {
			MoveX86regToVariable(m_Section->MipsRegLo(Opcode.rt), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(m_Section,x86_Any,Opcode.rt,FALSE), &_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
		}
		break;
	case 13: //cause
		if (m_Section->IsConst(Opcode.rt)) {
			AndConstToVariable(0xFFFFCFF,&_CP0[Opcode.rd], CRegName::Cop0[Opcode.rd]);
#ifndef EXTERNAL_RELEASE
			if ((m_Section->MipsRegLo(Opcode.rt) & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		} else {
			CRecompilerOps::Compile_UnknownOpcode(m_Section);
		}
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	default:
		CRecompilerOps::Compile_UnknownOpcode(m_Section);
	}
}

/************************** COP0 CO functions ***********************/
void CRecompilerOps::Compile_COP0_CO_TLBR( CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (!UseTlb) {	return; }
	Pushad();
	Call_Direct(TLB_ReadEntry,"TLB_ReadEntry");
	Popad();
}

void CRecompilerOps::Compile_COP0_CO_TLBWI( CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
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

void CRecompilerOps::Compile_COP0_CO_TLBWR( CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	if (!UseTlb) {	return; }

	_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
	m_Section->BlockCycleCount() = 0;
	m_Section->BlockRandomModifier() = 0;
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

void CRecompilerOps::Compile_COP0_CO_TLBP( CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));
	
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

void CRecompilerOps::Compile_COP0_CO_ERET( CCodeSection * m_Section) {
	CPU_Message("  %X %s",m_Section->CompilePC,R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

	WriteBackRegisters(m_Section);
	Call_Direct(compiler_COP0_CO_ERET,"compiler_COP0_CO_ERET");
	_N64System->GetRecompiler()->CompileExit (m_Section,m_Section->CompilePC, (DWORD)-1,m_Section->RegWorking,CExitInfo::Normal,TRUE,NULL);
	NextInstruction = END_BLOCK;
}

/************************** Other functions **************************/
void CRecompilerOps::Compile_UnknownOpcode (CCodeSection * m_Section) {
	CPU_Message("  %X Unhandled Opcode: %s",m_Section->CompilePC, R4300iOpcodeName(Opcode.Hex,m_Section->CompilePC));

//	FreeSection(m_Section->ContinueSection,m_Section);
//	FreeSection(m_Section->JumpSection,m_Section);
	m_Section->BlockCycleCount() -= CountPerOp;
	m_Section->BlockRandomModifier() -= 1;
	MoveConstToVariable(m_Section->CompilePC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
	WriteBackRegisters(m_Section);
	_N64System->GetRecompiler()->UpdateCounters(&m_Section->BlockCycleCount(),&m_Section->BlockRandomModifier(),FALSE);
	if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
	MoveConstToVariable(Opcode.Hex,&Opcode.Hex,"Opcode.Hex");
	Call_Direct(R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
	Ret();
	if (NextInstruction == NORMAL) { NextInstruction = END_BLOCK; }
}


#endif