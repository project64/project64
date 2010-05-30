#include "stdafx.h"

CCodeSection * CRecompilerOps::m_Section = NULL;
CRegInfo	   CRecompilerOps::m_RegWorkingSet;
STEP_TYPE      CRecompilerOps::m_NextInstruction;
DWORD          CRecompilerOps::m_CompilePC;
OPCODE         CRecompilerOps::m_Opcode;


void CRecompilerOps::CompileReadTLBMiss (CCodeSection * Section, int AddressReg, int LookUpReg ) 
{
#ifdef tofix
	MoveX86regToVariable(AddressReg,&TLBLoadAddress,"TLBLoadAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC, m_CompilePC,m_RegWorkingSet,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
#endif
}

void CRecompilerOps::CompileWriteTLBMiss (CCodeSection * Section, int AddressReg, int LookUpReg ) 
{
#ifdef tofix
	MoveX86regToVariable(AddressReg,&TLBStoreAddress,"TLBStoreAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC, m_CompilePC,m_RegWorkingSet,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
#endif
}

#ifdef tofix
DWORD BranchCompare = 0;

#endif

int  DelaySlotEffectsCompare ( DWORD PC, DWORD Reg1, DWORD Reg2 );

/************************** Branch functions  ************************/
void CRecompilerOps::Compile_Branch (CRecompilerOps::BranchFunction CompareFunc, BRANCH_TYPE BranchType, BOOL Link)
{
	static int EffectDelaySlot, DoneJumpDelay, DoneContinueDelay;
	static char ContLabel[100], JumpLabel[100];
	static CRegInfo RegBeforeDelay;

	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		
		if ((m_CompilePC & 0xFFC) != 0xFFC) {
			switch (BranchType) {
			case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0); break;
			case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,m_Opcode.rt); break;
			case BranchTypeCop1: 
				{
					OPCODE Command;

					if (!_MMU ->LW_VAddr(m_CompilePC + 4, Command.Hex)) {
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
		if (m_Section->m_ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
		} else {
			strcpy(ContLabel,"Cont.LinkLocation");
		}
		if (m_Section->m_JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
		} else {
			strcpy(JumpLabel,"Jump.LinkLocation");
		}
		m_Section->m_Jump.TargetPC        = m_CompilePC + ((short)m_Opcode.offset << 2) + 4;
		m_Section->m_Jump.BranchLabel     = JumpLabel;
		m_Section->m_Jump.LinkLocation    = NULL;
		m_Section->m_Jump.LinkLocation2   = NULL;
		m_Section->m_Jump.DoneDelaySlot   = FALSE;
		m_Section->m_Cont.TargetPC        = m_CompilePC + 8;
		m_Section->m_Cont.BranchLabel     = ContLabel;
		m_Section->m_Cont.LinkLocation    = NULL;
		m_Section->m_Cont.LinkLocation2   = NULL;
		m_Section->m_Cont.DoneDelaySlot   = FALSE;
		if (m_Section->m_Jump.TargetPC < m_Section->m_Cont.TargetPC) {
			m_Section->m_Cont.FallThrough = FALSE;
			m_Section->m_Jump.FallThrough = TRUE;
		} else {
			m_Section->m_Cont.FallThrough = TRUE;
			m_Section->m_Jump.FallThrough = FALSE;
		}
		if (Link) {
			UnMap_GPR( 31, FALSE);
			MipsRegLo(31) = m_CompilePC + 8;
			MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		if (EffectDelaySlot) {
			if (m_Section->m_ContinueSection != NULL) {
				sprintf(ContLabel,"Continue",((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
			} else {
				strcpy(ContLabel,"ExitBlock");
			}
			if (m_Section->m_JumpSection != NULL) {
				sprintf(JumpLabel,"Jump",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
			} else {
				strcpy(JumpLabel,"ExitBlock");
			}
			CompareFunc(); 
			
			if ((m_CompilePC & 0xFFC) == 0xFFC) {
				m_Section->GenerateSectionLinkage();
				m_NextInstruction = END_BLOCK;
				return;
			}
			if (!m_Section->m_Jump.FallThrough && !m_Section->m_Cont.FallThrough) {
				if (m_Section->m_Jump.LinkLocation != NULL) {
					CPU_Message("");
					CPU_Message("      %s:",m_Section->m_Jump.BranchLabel);
					SetJump32((DWORD *)m_Section->m_Jump.LinkLocation,(DWORD *)m_RecompPos);
					m_Section->m_Jump.LinkLocation = NULL;
					if (m_Section->m_Jump.LinkLocation2 != NULL) {
						SetJump32((DWORD *)m_Section->m_Jump.LinkLocation2,(DWORD *)m_RecompPos);
						m_Section->m_Jump.LinkLocation2 = NULL;
					}
					m_Section->m_Jump.FallThrough = TRUE;
				} else if (m_Section->m_Cont.LinkLocation != NULL){
					CPU_Message("");
					CPU_Message("      %s:",m_Section->m_Cont.BranchLabel);
					SetJump32((DWORD *)m_Section->m_Cont.LinkLocation,(DWORD *)m_RecompPos);
					m_Section->m_Cont.LinkLocation = NULL;
					if (m_Section->m_Cont.LinkLocation2 != NULL) {
						SetJump32((DWORD *)m_Section->m_Cont.LinkLocation2,(DWORD *)m_RecompPos);
						m_Section->m_Cont.LinkLocation2 = NULL;
					}
					m_Section->m_Cont.FallThrough = TRUE;
				}
			}
			m_Section->ResetX86Protection();
			memcpy(&RegBeforeDelay,&m_RegWorkingSet,sizeof(CRegInfo));
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		if (EffectDelaySlot) { 
			CJumpInfo * FallInfo = m_Section->m_Jump.FallThrough?&m_Section->m_Jump:&m_Section->m_Cont;
			CJumpInfo * JumpInfo = m_Section->m_Jump.FallThrough?&m_Section->m_Cont:&m_Section->m_Jump;

			if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot) {
				m_Section->ResetX86Protection();
				FallInfo->RegSet = m_RegWorkingSet;
				if (FallInfo == &m_Section->m_Jump) {
					if (m_Section->m_JumpSection != NULL) {
						sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
					} else {
						strcpy(JumpLabel,"ExitBlock");
					}
					if (FallInfo->TargetPC <= m_CompilePC) 
					{
#ifdef tofix
						UpdateCounters(&(FallInfo->RegSet.BlockCycleCount()),&(FallInfo->RegSet.BlockRandomModifier()),true);
						m_Section->CompileSystemCheck(FallInfo->TargetPC,FallInfo->RegSet);
						m_Section->ResetX86Protection();
#endif
					}
				} else {
					if (m_Section->m_ContinueSection != NULL) {
						sprintf(ContLabel,"Section_%d",((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
					} else {
						strcpy(ContLabel,"ExitBlock");
					}
				}		
				FallInfo->DoneDelaySlot = TRUE;
				if (!JumpInfo->DoneDelaySlot) {
					FallInfo->FallThrough = FALSE;				
					JmpLabel32(FallInfo->BranchLabel,0);
					FallInfo->LinkLocation = (DWORD *)(m_RecompPos - 4);
					
					if (JumpInfo->LinkLocation != NULL) {
						CPU_Message("      %s:",JumpInfo->BranchLabel);
						SetJump32((DWORD *)JumpInfo->LinkLocation,(DWORD *)m_RecompPos);
						JumpInfo->LinkLocation = NULL;
						if (JumpInfo->LinkLocation2 != NULL) {
							SetJump32((DWORD *)JumpInfo->LinkLocation2,(DWORD *)m_RecompPos);
							JumpInfo->LinkLocation2 = NULL;
						}
						JumpInfo->FallThrough = TRUE;
						m_NextInstruction = DO_DELAY_SLOT;
						memcpy(&m_RegWorkingSet,&RegBeforeDelay,sizeof(CRegInfo));
						return; 
					}
				}
			}
		} else {
			CompareFunc();
			m_Section->ResetX86Protection();
			memcpy(&m_Section->m_Cont.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		}
		m_Section->GenerateSectionLinkage();
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_BranchLikely (BranchFunction CompareFunc, BOOL Link)
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	static char ContLabel[100], JumpLabel[100];
	if ( m_NextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		
		if (m_Section->m_ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
		} else {
			strcpy(ContLabel,"ExitBlock");
		}
		if (m_Section->m_JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		m_Section->m_Jump.TargetPC      = m_CompilePC + ((short)m_Opcode.offset << 2) + 4;
		m_Section->m_Jump.BranchLabel   = JumpLabel;
		m_Section->m_Jump.FallThrough   = TRUE;
		m_Section->m_Jump.LinkLocation  = NULL;
		m_Section->m_Jump.LinkLocation2 = NULL;
		m_Section->m_Cont.TargetPC      = m_CompilePC + 8;
		m_Section->m_Cont.BranchLabel   = ContLabel;
		m_Section->m_Cont.FallThrough   = FALSE;
		m_Section->m_Cont.LinkLocation  = NULL;
		m_Section->m_Cont.LinkLocation2 = NULL;
		if (Link) {
			UnMap_GPR( 31, FALSE);
			cMipsRegLo(31) = m_CompilePC + 8;
			MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		CompareFunc(m_Section); 
		m_Section->ResetX86Protection();
		memcpy(&m_Section->m_Cont.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		if (UseLinking && m_Section->m_Jump.TargetPC == m_Section->m_Cont.TargetPC)
		{
			if (m_Section->m_Cont.FallThrough)  
			{
				BreakPoint(__FILE__,__LINE__);
			}
			if (!m_Section->m_Jump.FallThrough)
			{
				BreakPoint(__FILE__,__LINE__);
			}
			m_Section->m_JumpSection->m_Cont.TargetPC = m_Section->m_Jump.TargetPC;
			m_Section->m_JumpSection->DelaySlotSection = true;
			m_Section->m_Jump.TargetPC = m_CompilePC + 4;
			m_Section->m_Jump.RegSet = m_RegWorkingSet;
			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			m_NextInstruction = END_BLOCK;
		} else {
			if (m_Section->m_Cont.FallThrough)  {
				if (m_Section->m_Jump.LinkLocation != NULL) {
	#ifndef EXTERNAL_RELEASE
					DisplayError("WTF .. problem with CRecompilerOps::BranchLikely");
	#endif
				}
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
				m_NextInstruction = END_BLOCK;
			} else {
				if ((m_CompilePC & 0xFFC) == 0xFFC) {
					m_Section->m_Jump.FallThrough = FALSE;
					if (m_Section->m_Jump.LinkLocation != NULL) {
						SetJump32(m_Section->m_Jump.LinkLocation,m_RecompPos);
						m_Section->m_Jump.LinkLocation = NULL;
						if (m_Section->m_Jump.LinkLocation2 != NULL) { 
							SetJump32(m_Section->m_Jump.LinkLocation2,m_RecompPos);
							m_Section->m_Jump.LinkLocation2 = NULL;
						}
					}
					JmpLabel32("DoDelaySlot",0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      %s:",m_Section->m_Cont.BranchLabel);
					if (m_Section->m_Cont.LinkLocation != NULL) {
						SetJump32(m_Section->m_Cont.LinkLocation,m_RecompPos);
						m_Section->m_Cont.LinkLocation = NULL;
						if (m_Section->m_Cont.LinkLocation2 != NULL) { 
							SetJump32(m_Section->m_Cont.LinkLocation2,m_RecompPos);
							m_Section->m_Cont.LinkLocation2 = NULL;
						}
					}
					_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC, m_CompilePC + 8,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
					CPU_Message("      ");
					CPU_Message("      DoDelaySlot");
					_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
					m_NextInstruction = END_BLOCK;
				} else {
					m_NextInstruction = DO_DELAY_SLOT;
				}
			}
		}
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {
		m_Section->ResetX86Protection();
		memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranchLikely\nNextInstruction = %X", m_NextInstruction);
#endif
	}
#endif
}

void CRecompilerOps::BNE_Compare (void) 
{
	BYTE *Jump;

	if (m_Section->IsKnown(m_Opcode.rs) && m_Section->IsKnown(m_Opcode.rt)) {
		if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rs) || m_Section->Is64Bit(m_Opcode.rt)) {
				CRecompilerOps::UnknownOpcode();
			} else if (cMipsRegLo(m_Opcode.rs) != cMipsRegLo(m_Opcode.rt)) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rs) || m_Section->Is64Bit(m_Opcode.rt)) {
				ProtectGPR(m_Opcode.rs);
				ProtectGPR(m_Opcode.rt);

				CompX86RegToX86Reg(
					m_Section->Is32Bit(m_Opcode.rs)?m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE):m_Section->MipsRegMapHi(m_Opcode.rs),
					m_Section->Is32Bit(m_Opcode.rt)?m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE):m_Section->MipsRegMapHi(m_Opcode.rt)
				);
					
				if (m_Section->m_Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (m_Section->Is64Bit(ConstReg) || m_Section->Is64Bit(MappedReg)) {
				if (m_Section->Is32Bit(ConstReg) || m_Section->Is32Bit(MappedReg)) {
					ProtectGPR(MappedReg);
					if (m_Section->Is32Bit(MappedReg)) {
						CompConstToX86reg(m_Section->Map_TempReg(x86_Any,MappedReg,TRUE),MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(m_Section->MipsRegMapHi(MappedReg),(int)cMipsRegMapLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(m_Section->MipsRegMapHi(MappedReg),MipsRegHi(ConstReg));
				}
				if (m_Section->m_Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompConstToX86reg(cMipsRegMapLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompConstToX86reg(cMipsRegMapLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rs) || m_Section->IsKnown(m_Opcode.rt)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

		if (IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompConstToVariable(((int)cMipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(m_Section->MipsRegMapHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				ProtectGPR(KnownReg);
				CompX86regToVariable(m_Section->Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (m_Section->m_Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (m_Section->m_Cont.FallThrough) {
			JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Jump.FallThrough) {
			JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else {
			JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		}
	} else {
		x86Reg Reg;

		Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE);		
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		if (m_Section->m_Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}

		Reg = m_Section->Map_TempReg(Reg,m_Opcode.rt,FALSE);
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		if (m_Section->m_Cont.FallThrough) {
			JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Jump.FallThrough) {
			JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else {
			JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		}
	}
}

void CRecompilerOps::BEQ_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE *Jump;

	if (m_Section->IsKnown(m_Opcode.rs) && m_Section->IsKnown(m_Opcode.rt)) {
		if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rs) || m_Section->Is64Bit(m_Opcode.rt)) {
				CRecompilerOps::UnknownOpcode();
			} else if (cMipsRegLo(m_Opcode.rs) == cMipsRegLo(m_Opcode.rt)) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rs) || m_Section->Is64Bit(m_Opcode.rt)) {
				ProtectGPR(m_Opcode.rs);
				ProtectGPR(m_Opcode.rt);

				CompX86RegToX86Reg(
					m_Section->Is32Bit(m_Opcode.rs)?m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE):MipsRegHi(m_Opcode.rs),
					m_Section->Is32Bit(m_Opcode.rt)?m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE):MipsRegHi(m_Opcode.rt)
				);
				if (m_Section->m_Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Cont.BranchLabel,0);
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (m_Section->Is64Bit(ConstReg) || m_Section->Is64Bit(MappedReg)) {
				if (m_Section->Is32Bit(ConstReg) || m_Section->Is32Bit(MappedReg)) {
					if (m_Section->Is32Bit(MappedReg)) {
						ProtectGPR(MappedReg);
						CompConstToX86reg(m_Section->Map_TempReg(x86_Any,MappedReg,TRUE),MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(MipsRegHi(MappedReg),(int)cMipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(MipsRegHi(MappedReg),MipsRegHi(ConstReg));
				}			
				if (m_Section->m_Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Cont.BranchLabel,0);
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompConstToX86reg(cMipsRegLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompConstToX86reg(cMipsRegLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel,0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rs) || m_Section->IsKnown(m_Opcode.rt)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

		if (IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompConstToVariable((int)cMipsRegLo(KnownReg) >> 31,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			ProtectGPR(KnownReg);
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompX86regToVariable(m_Section->Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (m_Section->m_Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Cont.BranchLabel,0);
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (m_Section->m_Cont.FallThrough) {
			JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else if (m_Section->m_Jump.FallThrough) {
			JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else {
			JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	} else {
		x86Reg Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		if (m_Section->m_Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Cont.BranchLabel,0);
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
		CompX86regToVariable(m_Section->Map_TempReg(Reg,m_Opcode.rs,FALSE),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		if (m_Section->m_Cont.FallThrough) {
			JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else if (m_Section->m_Jump.FallThrough) {
			JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else {
			JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	}
#endif
}

void CRecompilerOps::BGTZ_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (IsConst(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			if (m_Section->MipsReg_S(m_Opcode.rs) > 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) > 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		}
	} else if (IsMapped(m_Opcode.rs) && m_Section->Is32Bit(m_Opcode.rs)) {
		CompConstToX86reg(cMipsRegLo(m_Opcode.rs),0);
		if (m_Section->m_Jump.FallThrough) {
			JleLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Cont.FallThrough) {
			JgLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JleLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	} else {
		BYTE *Jump;

		if (IsMapped(m_Opcode.rs)) {
			CompConstToX86reg(MipsRegHi(m_Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		}
		if (m_Section->m_Jump.FallThrough) {
			JlLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JgLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else if (m_Section->m_Cont.FallThrough) {
			JlLabel8("continue",0);
			Jump = m_RecompPos - 1;
			JgLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JlLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JgLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}

		if (IsMapped(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		}
		if (m_Section->m_Jump.FallThrough) {
			JeLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
		} else if (m_Section->m_Cont.FallThrough) {
			JneLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
		} else {
			JneLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		}
	}
#endif
}

void CRecompilerOps::BLEZ_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (IsConst(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			if (m_Section->MipsReg_S(m_Opcode.rs) <= 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) <= 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			if (cMipsRegLo(m_Opcode.rs) == 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		}
	} else {
		if (IsMapped(m_Opcode.rs) && m_Section->Is32Bit(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),0);
			if (m_Section->m_Jump.FallThrough) {
				JgLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Cont.FallThrough) {
				JleLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else {
			BYTE *Jump;

			if (IsMapped(m_Opcode.rs)) {
				CompConstToX86reg(MipsRegHi(m_Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
			}
			if (m_Section->m_Jump.FallThrough) {
				JgLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JlLabel8("Continue",0);
				Jump = m_RecompPos - 1;
			} else if (m_Section->m_Cont.FallThrough) {
				JgLabel8("Continue",0);
				Jump = m_RecompPos - 1;
				JlLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JlLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}

			if (IsMapped(m_Opcode.rs)) {
				CompConstToX86reg(cMipsRegLo(m_Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
			}
			if (m_Section->m_Jump.FallThrough) {
				JneLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			} else if (m_Section->m_Cont.FallThrough) {
				JeLabel32 (m_Section->m_Jump.BranchLabel, 0 );
				m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			} else {
				JneLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				JmpLabel32("BranchToJump",0);
				m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			}
		}
	}
#endif
}

void CRecompilerOps::BLTZ_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (IsConst(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			if (m_Section->MipsReg_S(m_Opcode.rs) < 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) < 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			m_Section->m_Jump.FallThrough = FALSE;
			m_Section->m_Cont.FallThrough = TRUE;
		}
	} else if (IsMapped(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			CompConstToX86reg(MipsRegHi(m_Opcode.rs),0);
			if (m_Section->m_Jump.FallThrough) {
				JgeLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Cont.FallThrough) {
				JlLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgeLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),0);
			if (m_Section->m_Jump.FallThrough) {
				JgeLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Cont.FallThrough) {
				JlLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgeLabel32 (m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else {
			m_Section->m_Jump.FallThrough = FALSE;
			m_Section->m_Cont.FallThrough = TRUE;
		}
	} else if (m_Section->IsUnknown(m_Opcode.rs)) {
		CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		if (m_Section->m_Jump.FallThrough) {
			JgeLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Cont.FallThrough) {
			JlLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JlLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32 (m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	}
#endif
}

void CRecompilerOps::BGEZ_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (IsConst(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
			DisplayError("BGEZ 1");
#endif
			CRecompilerOps::UnknownOpcode();
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) >= 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			m_Section->m_Jump.FallThrough = TRUE;
			m_Section->m_Cont.FallThrough = FALSE;
		}
	} else if (IsMapped(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) { 
			CompConstToX86reg(MipsRegHi(m_Opcode.rs),0);
			if (m_Section->m_Cont.FallThrough) {
				JgeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Jump.FallThrough) {
				JlLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JlLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),0);
			if (m_Section->m_Cont.FallThrough) {
				JgeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Jump.FallThrough) {
				JlLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JlLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel,0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else { 
			m_Section->m_Jump.FallThrough = TRUE;
			m_Section->m_Cont.FallThrough = FALSE;
		}
	} else {
		CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);		
		if (m_Section->m_Cont.FallThrough) {
			JgeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Jump.FallThrough) {
			JlLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JlLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel,0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	}
#endif
}

void CRecompilerOps::COP1_BCF_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (m_Section->m_Cont.FallThrough) {
		JeLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else if (m_Section->m_Jump.FallThrough) {
		JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else {
		JneLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		JmpLabel32(m_Section->m_Jump.BranchLabel,0);
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	}
#endif
}

void CRecompilerOps::COP1_BCT_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (m_Section->m_Cont.FallThrough) {
		JneLabel32 ( m_Section->m_Jump.BranchLabel, 0 );
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else if (m_Section->m_Jump.FallThrough) {
		JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else {
		JeLabel32 ( m_Section->m_Cont.BranchLabel, 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		JmpLabel32(m_Section->m_Jump.BranchLabel,0);
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	}
#endif
}

/*************************  OpCode functions *************************/
void CRecompilerOps::J (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	static char JumpLabel[100];

	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

		if (m_Section->m_JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		m_Section->m_Jump.TargetPC      = (m_CompilePC & 0xF0000000) + (Opcode.target << 2);;
		m_Section->m_Jump.BranchLabel   = JumpLabel;
		m_Section->m_Jump.FallThrough   = TRUE;
		m_Section->m_Jump.LinkLocation  = NULL;
		m_Section->m_Jump.LinkLocation2 = NULL;
		m_NextInstruction = DO_DELAY_SLOT;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			m_NextInstruction = END_BLOCK;
		}
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", m_NextInstruction);
#endif
	}
#endif
}

void CRecompilerOps::JAL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	static char JumpLabel[100];

	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		UnMap_GPR( 31, FALSE);
		cMipsRegLo(31) = m_CompilePC + 8;
		MipsRegState(31) = CRegInfo::STATE_CONST_32;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			MoveConstToVariable((m_CompilePC & 0xF0000000) + (Opcode.target << 2),&JumpToLocation,"JumpToLocation");
			MoveConstToVariable(m_CompilePC + 4,_PROGRAM_COUNTER,"_PROGRAM_COUNTER");
			UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
			WriteBackRegisters(m_Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
			Ret();
			m_NextInstruction = END_BLOCK;
			return;
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		DWORD TargetPC = (m_CompilePC & 0xF0000000) + (Opcode.target << 2);
		_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC,TargetPC,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
	return;

	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

		UnMap_GPR( 31, FALSE);
		cMipsRegLo(31) = m_CompilePC + 8;
		MipsRegState(31) = CRegInfo::STATE_CONST_32;
		m_NextInstruction = DO_DELAY_SLOT;
		if (m_Section->m_JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		m_Section->m_Jump.TargetPC      = (m_CompilePC & 0xF0000000) + (Opcode.target << 2);
		m_Section->m_Jump.BranchLabel   = JumpLabel;
		m_Section->m_Jump.FallThrough   = TRUE;
		m_Section->m_Jump.LinkLocation  = NULL;
		m_Section->m_Jump.LinkLocation2 = NULL;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			m_NextInstruction = END_BLOCK;
		}
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", m_NextInstruction);
#endif
	}
#endif
}

void CRecompilerOps::ADDI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) { return; }

	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && m_Opcode.rs == 29 && m_Opcode.rt == 29) {
		AddConstToX86Reg(Map_MemoryStack(m_Section, x86_Any, true),(short)m_Opcode.immediate);
	}
#endif

	if (IsConst(m_Opcode.rs)) { 
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		cMipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) + (short)m_Opcode.immediate;
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rs);
		if (m_Opcode.immediate == 0) { 
		} else if (m_Opcode.immediate == 1) {
			IncX86reg(cMipsRegLo(m_Opcode.rt));
		} else if (m_Opcode.immediate == 0xFFFF) {			
			DecX86reg(cMipsRegLo(m_Opcode.rt));
		} else {
			AddConstToX86Reg(cMipsRegLo(m_Opcode.rt),(short)m_Opcode.immediate);
		}
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && m_Opcode.rt == 29 && m_Opcode.rs != 29) { 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
#endif
}

void CRecompilerOps::ADDIU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) { return; }

#ifdef tofix
	if (SPHack)
	{
		if (m_Opcode.rs == 29 && m_Opcode.rt == 29) 
		{
			AddConstToX86Reg(Map_MemoryStack(m_Section, x86_Any, TRUE),(short)m_Opcode.immediate);
		}
	}
#endif

	if (IsConst(m_Opcode.rs)) { 
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) + (short)m_Opcode.immediate;
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rs);
		AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),(short)m_Opcode.immediate);
	}

#ifdef tofix
	if (SPHack && m_Opcode.rt == 29 && m_Opcode.rs != 29) { 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::SLTIU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return; }

	if (IsConst(m_Opcode.rs)) { 
		DWORD Result;

		if (m_Section->Is64Bit(m_Opcode.rs)) {
			_int64 Immediate = (_int64)((short)m_Opcode.immediate);
			Result = m_Section->MipsReg(m_Opcode.rs) < ((unsigned)(Immediate))?1:0;
		} else if (m_Section->Is32Bit(m_Opcode.rs)) {
			Result = cMipsRegLo(m_Opcode.rs) < ((unsigned)((short)m_Opcode.immediate))?1:0;
		}
		UnMap_GPR(m_Opcode.rt, FALSE);
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
		cMipsRegLo(m_Opcode.rt) = Result;
	} else if (IsMapped(m_Opcode.rs)) { 
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(MipsRegHi(m_Opcode.rs),((short)m_Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = m_RecompPos - 1;
			SetbVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = m_RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
		} else {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
		}
	} else {
		BYTE * Jump;

		CompConstToVariable(((short)m_Opcode.immediate >> 31),&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		JneLabel8("CompareSet",0);
		Jump = m_RecompPos - 1;
		CompConstToVariable((short)m_Opcode.immediate,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		CPU_Message("");
		CPU_Message("      CompareSet:");
		*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
		SetbVariable(&BranchCompare,"BranchCompare");
		Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
		
		
		/*SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompConstToVariable((short)m_Opcode.immediate,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));*/
	}
#endif
}

void CRecompilerOps::SLTI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return; }

	if (IsConst(m_Opcode.rs)) { 
		DWORD Result;

		if (m_Section->Is64Bit(m_Opcode.rs)) {
			_int64 Immediate = (_int64)((short)m_Opcode.immediate);
			Result = (_int64)m_Section->MipsReg(m_Opcode.rs) < Immediate?1:0;
		} else if (m_Section->Is32Bit(m_Opcode.rs)) {
			Result = m_Section->MipsRegLo_S(m_Opcode.rs) < (short)m_Opcode.immediate?1:0;
		}
		UnMap_GPR(m_Opcode.rt, FALSE);
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
		cMipsRegLo(m_Opcode.rt) = Result;
	} else if (IsMapped(m_Opcode.rs)) { 
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(MipsRegHi(m_Opcode.rs),((short)m_Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = m_RecompPos - 1;
			SetlVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = m_RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
		} else {
		/*	CompConstToX86reg(cMipsRegLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetlVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
			*/
			ProtectGPR( m_Opcode.rs);
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			CompConstToX86reg(cMipsRegLo(m_Opcode.rs),(short)m_Opcode.immediate);
			
			if (cMipsRegLo(m_Opcode.rt) > x86_EDX) {
				SetlVariable(&BranchCompare,"BranchCompare");
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
			} else {
				Setl(cMipsRegLo(m_Opcode.rt));
				AndConstToX86Reg(cMipsRegLo(m_Opcode.rt), 1);
			}
		}
	} else {
		BYTE * Jump[2];

		CompConstToVariable(((short)m_Opcode.immediate >> 31),&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompConstToVariable((short)m_Opcode.immediate,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rt));
	}
#endif
}

void CRecompilerOps::ANDI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) { return;}

	if (IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
		cMipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) & m_Opcode.immediate;
	} else if (m_Opcode.immediate != 0) { 
		Map_GPR_32bit(m_Opcode.rt,FALSE,m_Opcode.rs);
		AndConstToX86Reg(cMipsRegLo(m_Opcode.rt),m_Opcode.immediate);
	} else {
		Map_GPR_32bit(m_Opcode.rt,FALSE,0);
	}
#endif
}

void CRecompilerOps::ORI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return;}

	if (IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegState(m_Opcode.rt) = MipsRegState(m_Opcode.rs);
		MipsRegHi(m_Opcode.rt) = MipsRegHi(m_Opcode.rs);
		cMipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) | m_Opcode.immediate;
	} else if (IsMapped(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			Map_GPR_64bit(m_Section,m_Opcode.rt,m_Opcode.rs);
		} else {
			Map_GPR_32bit(m_Opcode.rt,m_Section->IsSigned(m_Opcode.rs),m_Opcode.rs);
		}
		OrConstToX86Reg(m_Opcode.immediate,cMipsRegLo(m_Opcode.rt));
	} else {
		Map_GPR_64bit(m_Section,m_Opcode.rt,m_Opcode.rs);
		OrConstToX86Reg(m_Opcode.immediate,cMipsRegLo(m_Opcode.rt));
	}
#endif
}

void CRecompilerOps::XORI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return;}

	if (IsConst(m_Opcode.rs)) {
		if (m_Opcode.rs != m_Opcode.rt) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegState(m_Opcode.rt) = MipsRegState(m_Opcode.rs);
		MipsRegHi(m_Opcode.rt) = MipsRegHi(m_Opcode.rs);
		cMipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) ^ m_Opcode.immediate;
	} else {
		if (IsMapped(m_Opcode.rs) && m_Section->Is32Bit(m_Opcode.rs)) {
			Map_GPR_32bit(m_Opcode.rt,m_Section->IsSigned(m_Opcode.rs),m_Opcode.rs);
		} else {
			Map_GPR_64bit(m_Section,m_Opcode.rt,m_Opcode.rs);
		}
		if (m_Opcode.immediate != 0) { XorConstToX86Reg(cMipsRegLo(m_Opcode.rt),m_Opcode.immediate); }
	}
#endif
}

void CRecompilerOps::LUI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return;}

#ifdef tofix
	if (SPHack && m_Opcode.rt == 29) {
		int x86reg = Map_MemoryStack(m_Section, x86_Any, false);
		DWORD Address;

		TranslateVaddr (((short)m_Opcode.offset << 16), &Address);
		if (x86reg < 0) {
			MoveConstToVariable((DWORD)(Address + RDRAM), g_MemoryStack, "MemoryStack");
		} else {
			MoveConstToX86reg((DWORD)(Address + RDRAM), x86reg);
		}
	}
#endif
	UnMap_GPR(m_Opcode.rt, FALSE);
	MipsRegLo(m_Opcode.rt) = ((short)m_Opcode.offset << 16);
	MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
}

void CRecompilerOps::DADDIU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rs,TRUE); }
	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::DADDIU, "R4300iOp::DADDIU");
	Popad();
#endif
}

void CRecompilerOps::LDL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.base != 0) { UnMap_GPR(m_Opcode.base,TRUE); }
	if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::LDL, "R4300iOp::LDL");
	Popad();
#endif
}

void CRecompilerOps::LDR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.base != 0) { UnMap_GPR(m_Opcode.base,TRUE); }
	if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::LDR, "R4300iOp::LDR");
	Popad();
#endif
}


void CRecompilerOps::LB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset) ^ 3;
		Map_GPR_32bit(m_Opcode.rt,TRUE,0);
		_MMU->Compile_LB(cMipsRegLo(m_Opcode.rt),Address,TRUE);
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveSxByteX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regByte(cMipsRegLo(m_Opcode.rt), TempReg1);
	}
#endif
}

void CRecompilerOps::LH (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset) ^ 2;
		Map_GPR_32bit(m_Opcode.rt,TRUE,0);
		_MMU->Compile_LH(cMipsRegLo(m_Opcode.rt),Address,TRUE);
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveSxHalfX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regHalf(cMipsRegLo(m_Opcode.rt), TempReg1);
	}
#endif
}

void CRecompilerOps::LWL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address, Value;
		
		Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rt);
		Value = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(cMipsRegLo(m_Opcode.rt),LWL_MASK[Offset]);
		ShiftLeftSignImmed(Value,(BYTE)LWL_SHIFT[Offset]);
		AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),Value);
		return;
	}

	shift = m_Section->Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
	}
	Offset = m_Section->Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rt);
	AndVariableDispToX86Reg(LWL_MASK,"LWL_MASK",cMipsRegLo(m_Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWL_SHIFT,"LWL_SHIFT",shift,Offset,4);
	if (g_UseTlb) {			
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftLeftSign(TempReg1);
	AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),TempReg1);
#endif
}

void CRecompilerOps::LW (void) 
{
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	x86Reg TempReg1, TempReg2;
#ifdef tofix
	if (m_Opcode.base == 29 && SPHack) {
		char String[100];

		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		TempReg1 = Map_MemoryStack(m_Section,x86_Any,true);
		sprintf(String,"%Xh",(short)m_Opcode.offset);
		MoveVariableDispToX86Reg((void *)((DWORD)(short)m_Opcode.offset),String,cMipsRegLo(m_Opcode.rt),TempReg1,1);
	} else {
#endif
		if (IsConst(m_Opcode.base)) { 
			DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
			Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
			_MMU->Compile_LW(m_Section, cMipsRegMapLo(m_Opcode.rt),Address);
		} else {
			if (g_UseTlb) {	
				if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
				if (IsMapped(m_Opcode.base) && m_Opcode.offset == 0) { 
					ProtectGPR(m_Opcode.base);
					TempReg1 = cMipsRegMapLo(m_Opcode.base);
				} else {
					if (IsMapped(m_Opcode.base)) { 
						ProtectGPR(m_Opcode.base);
						if (m_Opcode.offset != 0) {
							TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
							LeaSourceAndOffset(TempReg1,cMipsRegMapLo(m_Opcode.base),(short)m_Opcode.offset);
						} else {
							TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
						}
					} else {
						TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
						AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
					}
				}
				TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
				MoveX86RegToX86Reg(TempReg1, TempReg2);
				ShiftRightUnsignImmed(TempReg2,12);
				MoveVariableDispToX86Reg(g_TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
				CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
				Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
				MoveX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegMapLo(m_Opcode.rt));
			} else {
				if (IsMapped(m_Opcode.base)) { 
					ProtectGPR(m_Opcode.base);
					if (m_Opcode.offset != 0) {
						Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
						LeaSourceAndOffset(cMipsRegMapLo(m_Opcode.rt),cMipsRegMapLo(m_Opcode.base),(short)m_Opcode.offset);
					} else {
						Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.base);
					}
				} else {
					Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.base);
					AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),(short)m_Opcode.immediate);
				}
				AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),0x1FFFFFFF);
				MoveN64MemToX86reg(cMipsRegMapLo(m_Opcode.rt),cMipsRegMapLo(m_Opcode.rt));
			}
		}
#ifdef tofix
	}
	if (SPHack && m_Opcode.rt == 29)
	{ 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::LBU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset) ^ 3;
		Map_GPR_32bit(m_Opcode.rt,FALSE,0);
		_MMU->Compile_LB(cMipsRegLo(m_Opcode.rt),Address,FALSE);
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
		MoveZxByteX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
		MoveZxN64MemToX86regByte(cMipsRegLo(m_Opcode.rt), TempReg1);
	}
#endif
}

void CRecompilerOps::LHU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset) ^ 2;
		Map_GPR_32bit(m_Opcode.rt,FALSE,0);
		_MMU->Compile_LH(cMipsRegLo(m_Opcode.rt),Address,FALSE);
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(cMipsRegLo(m_Opcode.rt), TempReg1);
	}
#endif
}

void CRecompilerOps::LWR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address, Value;
		
		Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rt);
		Value = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(cMipsRegLo(m_Opcode.rt),LWR_MASK[Offset]);
		ShiftRightUnsignImmed(Value,(BYTE)LWR_SHIFT[Offset]);
		AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),Value);
		return;
	}

	shift = m_Section->Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
	}
	Offset = m_Section->Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rt);
	AndVariableDispToX86Reg(LWR_MASK,"LWR_MASK",cMipsRegLo(m_Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWR_SHIFT,"LWR_SHIFT",shift,Offset,4);
	if (g_UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftRightUnsign(TempReg1);
	AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),TempReg1);
#endif
}

void CRecompilerOps::LWU (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset);
		Map_GPR_32bit(m_Opcode.rt,FALSE,0);
		_MMU->Compile_LW(m_Section, cMipsRegLo(m_Opcode.rt),Address);
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(cMipsRegLo(m_Opcode.rt), TempReg1);
	}
#endif
}

void CRecompilerOps::SB (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset) ^ 3;
		
		if (IsConst(m_Opcode.rt)) {
			_MMU->Compile_SB_Const((BYTE)cMipsRegLo(m_Opcode.rt), Address);
		} else if (IsMapped(m_Opcode.rt) && Is8BitReg(cMipsRegLo(m_Opcode.rt))) {
			_MMU->Compile_SB_Register(cMipsRegLo(m_Opcode.rt), Address);
		} else {
			_MMU->Compile_SB_Register(m_Section->Map_TempReg(x86_Any8Bit,m_Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,3);	
		if (IsConst(m_Opcode.rt)) {
			MoveConstByteToX86regPointer((BYTE)cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(m_Opcode.rt) && Is8BitReg(cMipsRegLo(m_Opcode.rt))) {
			MoveX86regByteToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else {	
			UnProtectGPR(m_Opcode.rt);
			MoveX86regByteToX86regPointer(m_Section->Map_TempReg(x86_Any8Bit,m_Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		if (IsConst(m_Opcode.rt)) {
			MoveConstByteToN64Mem((BYTE)cMipsRegLo(m_Opcode.rt),TempReg1);
		} else if (IsMapped(m_Opcode.rt) && Is8BitReg(cMipsRegLo(m_Opcode.rt))) {
			MoveX86regByteToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
		} else {	
			UnProtectGPR(m_Opcode.rt);
			MoveX86regByteToN64Mem(m_Section->Map_TempReg(x86_Any8Bit,m_Opcode.rt,FALSE),TempReg1);
		}
	}
#endif
}

void CRecompilerOps::SH (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (IsConst(m_Opcode.base)) { 
		DWORD Address = (cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset) ^ 2;
		
		if (IsConst(m_Opcode.rt)) {
			_MMU->Compile_SH_Const((WORD)cMipsRegLo(m_Opcode.rt), Address);
		} else if (IsMapped(m_Opcode.rt)) {
			_MMU->Compile_SH_Register(cMipsRegLo(m_Opcode.rt), Address);
		} else {
			_MMU->Compile_SH_Register(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,2);	
		if (IsConst(m_Opcode.rt)) {
			MoveConstHalfToX86regPointer((WORD)cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regHalfToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regHalfToX86regPointer(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		XorConstToX86Reg(TempReg1,2);
		if (IsConst(m_Opcode.rt)) {
			MoveConstHalfToN64Mem((WORD)cMipsRegLo(m_Opcode.rt),TempReg1);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regHalfToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);		
		} else {	
			MoveX86regHalfToN64Mem(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1);		
		}
	}
#endif
}

void CRecompilerOps::SWL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (IsConst(m_Opcode.base)) { 
		DWORD Address;
	
		Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Offset  = Address & 3;
		
		Value = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(Value,SWL_MASK[Offset]);
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
		ShiftRightUnsignImmed(TempReg1,(BYTE)SWL_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		_MMU->Compile_SW_Register(m_Section,Value, (Address & ~3));
		return;
	}
	shift = m_Section->Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}		
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = m_Section->Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = m_Section->Map_TempReg(x86_Any,-1,FALSE);
	if (g_UseTlb) {	
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWL_MASK,"SWL_MASK",Value,Offset,4);
	if (!IsConst(m_Opcode.rt) || cMipsRegLo(m_Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWL_SHIFT,"SWL_SHIFT",shift,Offset,4);
		if (IsConst(m_Opcode.rt)) {
			MoveConstToX86reg(cMipsRegLo(m_Opcode.rt),Offset);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),Offset);
		} else {
			MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[0],CRegName::GPR_Lo[m_Opcode.rt],Offset);
		}
		ShiftRightUnsign(Offset);
		AddX86RegToX86Reg(Value,Offset);
	}

	if (g_UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);

		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
#endif
}

void CRecompilerOps::SW (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	_Notify->BreakPoint(__FILE__,__LINE__);
	DWORD TempReg1, TempReg2;
	if (m_Opcode.base == 29 && SPHack) {
		if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
		TempReg1 = Map_MemoryStack(m_Section,x86_Any,true);

		if (IsConst(m_Opcode.rt)) {
			MoveConstToMemoryDisp (cMipsRegLo(m_Opcode.rt),TempReg1, (DWORD)((short)m_Opcode.offset));
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToMemory(cMipsRegLo(m_Opcode.rt),TempReg1,(DWORD)((short)m_Opcode.offset));
		} else {	
			TempReg2 = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			MoveX86regToMemory(TempReg2,TempReg1,(DWORD)((short)m_Opcode.offset));
		}		
	} else {
		if (IsConst(m_Opcode.base)) { 
			DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
			
			if (IsConst(m_Opcode.rt)) {
				_MMU->Compile_SW_Const(cMipsRegLo(m_Opcode.rt), Address);
			} else if (IsMapped(m_Opcode.rt)) {
				_MMU->Compile_SW_Register(m_Section,cMipsRegLo(m_Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Register(m_Section,m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE), Address);
			}
			return;
		}
		if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
		if (IsMapped(m_Opcode.base)) { 
			ProtectGPR(m_Opcode.base);
			if (m_Opcode.offset != 0) {
				TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
			} else {
				TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
			}
			UnProtectGPR(m_Opcode.base);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
			AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
		}
		if (g_UseTlb) {
			TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			MoveX86RegToX86Reg(TempReg1, TempReg2);
			ShiftRightUnsignImmed(TempReg2,12);
			MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
			//For tlb miss
			//0041C522 85 C0                test        eax,eax
			//0041C524 75 01                jne         0041C527

			if (IsConst(m_Opcode.rt)) {
				MoveConstToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
			} else if (IsMapped(m_Opcode.rt)) {
				MoveX86regToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
			} else {	
				MoveX86regToX86regPointer(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1, TempReg2);
			}
		} else {
			AndConstToX86Reg(TempReg1,0x1FFFFFFF);
			if (IsConst(m_Opcode.rt)) {
				MoveConstToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
			} else if (IsMapped(m_Opcode.rt)) {
				MoveX86regToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
			} else {	
				MoveX86regToN64Mem(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1);
			}
		}
	}
#endif
}

void CRecompilerOps::SWR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (IsConst(m_Opcode.base)) { 
		DWORD Address;
	
		Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Offset  = Address & 3;
		
		Value = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		_MMU->Compile_LW(m_Section, Value,(Address & ~3));
		AndConstToX86Reg(Value,SWR_MASK[Offset]);
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
		ShiftLeftSignImmed(TempReg1,(BYTE)SWR_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		_MMU->Compile_SW_Register(m_Section,Value, (Address & ~3));
		return;
	}
	shift = m_Section->Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}		
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = m_Section->Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = m_Section->Map_TempReg(x86_Any,-1,FALSE);
	if (g_UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWR_MASK,"SWR_MASK",Value,Offset,4);
	if (!IsConst(m_Opcode.rt) || cMipsRegLo(m_Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWR_SHIFT,"SWR_SHIFT",shift,Offset,4);
		if (IsConst(m_Opcode.rt)) {
			MoveConstToX86reg(cMipsRegLo(m_Opcode.rt),Offset);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),Offset);
		} else {
			MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[0],CRegName::GPR_Lo[m_Opcode.rt],Offset);
		}
		ShiftLeftSign(Offset);
		AddX86RegToX86Reg(Value,Offset);
	}

	if (g_UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);

		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
#endif
}

void CRecompilerOps::SDL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.base != 0) { UnMap_GPR(m_Opcode.base,TRUE); }
	if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SDL, "R4300iOp::SDL");
	Popad();

#endif
}

void CRecompilerOps::SDR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.base != 0) { UnMap_GPR(m_Opcode.base,TRUE); }
	if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SDR, "R4300iOp::SDR");
	Popad();
#endif
}

void CRecompilerOps::CACHE (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (_Settings->LoadDword(Game_SMM_Cache) == 0)
	{
		return;
	}

	switch(m_Opcode.rt) {
	case 0:
	case 16:
		Pushad();
		PushImm32("CRecompiler::Remove_Cache",CRecompiler::Remove_Cache);
		PushImm32("20",20);
		if (IsConst(m_Opcode.base)) { 
			DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
			PushImm32("Address",Address);
		} else if (IsMapped(m_Opcode.base)) { 
			AddConstToX86Reg(cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
			Push(cMipsRegLo(m_Opcode.base));
		} else {
			MoveVariableToX86reg(&_GPR[m_Opcode.base].UW[0],CRegName::GPR_Lo[m_Opcode.base],x86_EAX);
			AddConstToX86Reg(x86_EAX,(short)m_Opcode.offset);
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
		DisplayError("cache: %d",m_Opcode.rt);
#endif
	}
#endif
}

void CRecompilerOps::LL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		_MMU->Compile_LW(m_Section, cMipsRegLo(m_Opcode.rt),Address);
		MoveConstToVariable(1,_LLBit,"LLBit");
		
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		TranslateVaddr(Address, &Address);
#endif
		MoveConstToVariable(Address,_LLAddr,"LLAddr");
		return;
	}
	if (g_UseTlb) {	
		if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
		if (IsMapped(m_Opcode.base) && m_Opcode.offset == 0) { 
			ProtectGPR(m_Opcode.base);
			TempReg1 = cMipsRegLo(m_Opcode.base);
		} else {
			if (IsMapped(m_Opcode.base)) { 
				ProtectGPR(m_Opcode.base);
				if (m_Opcode.offset != 0) {
					TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
					LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
				} else {
					TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
				}
			} else {
				TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
				AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
			}
		}
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
		MoveX86regToVariable(TempReg1,_LLAddr,"LLAddr");
		AddX86regToVariable(TempReg2,_LLAddr,"LLAddr");
		SubConstFromVariable((DWORD)_MMU->Rdram(),_LLAddr,"LLAddr");
	} else {
		if (IsMapped(m_Opcode.base)) { 
			ProtectGPR(m_Opcode.base);
			if (m_Opcode.offset != 0) {
				Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
				LeaSourceAndOffset(cMipsRegLo(m_Opcode.rt),cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
			} else {
				Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.base);
			}
		} else {
			Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.base);
			AddConstToX86Reg(cMipsRegLo(m_Opcode.rt),(short)m_Opcode.immediate);
		}
		AndConstToX86Reg(cMipsRegLo(m_Opcode.rt),0x1FFFFFFF);
		MoveX86regToVariable(cMipsRegLo(m_Opcode.rt),_LLAddr,"LLAddr");
		MoveN64MemToX86reg(cMipsRegLo(m_Opcode.rt),cMipsRegLo(m_Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
	}
#endif
}

void CRecompilerOps::SC (void){
#ifdef tofix
	DWORD TempReg1, TempReg2;
	BYTE * Jump;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	CompConstToVariable(1,_LLBit,"LLBit");
	JneLabel32("LLBitNotSet",0);
	Jump = (DWORD *)(m_RecompPos - 4);
	if (IsConst(m_Opcode.base)) { 
		DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
			
		if (IsConst(m_Opcode.rt)) {
			_MMU->Compile_SW_Const(cMipsRegLo(m_Opcode.rt), Address);
		} else if (IsMapped(m_Opcode.rt)) {
			_MMU->Compile_SW_Register(m_Section,cMipsRegLo(m_Opcode.rt), Address);
		} else {
			_MMU->Compile_SW_Register(m_Section,m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE), Address);
		}
		CPU_Message("      LLBitNotSet:");
		*((DWORD *)(Jump))=(BYTE)(m_RecompPos - Jump - 4);
		Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
		MoveVariableToX86reg(_LLBit,"LLBit",cMipsRegLo(m_Opcode.rt));
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (IsConst(m_Opcode.rt)) {
			MoveConstToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regToX86regPointer(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		if (IsConst(m_Opcode.rt)) {
			MoveConstToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
		} else {	
			MoveX86regToN64Mem(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1);
		}
	}
	CPU_Message("      LLBitNotSet:");
	*((DWORD *)(Jump))=(BYTE)(m_RecompPos - Jump - 4);
	Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
	MoveVariableToX86reg(_LLBit,"LLBit",cMipsRegLo(m_Opcode.rt));
#endif
}

void CRecompilerOps::LD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;
	
	_Notify->BreakPoint(__FILE__,__LINE__);
	DWORD TempReg1, TempReg2;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Map_GPR_64bit(m_Section,m_Opcode.rt,-1);
		_MMU->Compile_LW(m_Section, MipsRegHi(m_Opcode.rt),Address);
		_MMU->Compile_LW(m_Section, cMipsRegLo(m_Opcode.rt),Address + 4);
		if (SPHack && m_Opcode.rt == 29) { _MMU->ResetMemoryStack(m_Section); }
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base) && m_Opcode.offset == 0) { 
		if (g_UseTlb) {
			ProtectGPR(m_Opcode.base);
			TempReg1 = cMipsRegLo(m_Opcode.base);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		if (IsMapped(m_Opcode.base)) { 
			ProtectGPR(m_Opcode.base);
			if (m_Opcode.offset != 0) {
				TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
			} else {
				TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
			}
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
			AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
		}
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
		Map_GPR_64bit(m_Section,m_Opcode.rt,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,MipsRegHi(m_Opcode.rt));
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt),4);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_64bit(m_Section,m_Opcode.rt,-1);
		MoveN64MemToX86reg(MipsRegHi(m_Opcode.rt),TempReg1);
		MoveN64MemDispToX86reg(cMipsRegLo(m_Opcode.rt),TempReg1,4);
	}
	if (SPHack && m_Opcode.rt == 29) { 		
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::SD (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (IsConst(m_Opcode.base)) { 
		DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		
		if (IsConst(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rt)) {
				_MMU->Compile_SW_Const(MipsRegHi(m_Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Const((m_Section->MipsRegLo_S(m_Opcode.rt) >> 31), Address);
			}
			_MMU->Compile_SW_Const(cMipsRegLo(m_Opcode.rt), Address + 4);
		} else if (IsMapped(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rt)) {
				_MMU->Compile_SW_Register(m_Section,MipsRegHi(m_Opcode.rt), Address);
			} else {
				_MMU->Compile_SW_Register(m_Section,m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE), Address);
			}
			_MMU->Compile_SW_Register(m_Section,cMipsRegLo(m_Opcode.rt), Address + 4);		
		} else {
			_MMU->Compile_SW_Register(m_Section,TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE), Address);
			_MMU->Compile_SW_Register(m_Section,m_Section->Map_TempReg(TempReg1,m_Opcode.rt,FALSE), Address + 4);		
		}
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
	} else {
		TempReg1 = m_Section->Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = m_Section->Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (IsConst(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rt)) {
				MoveConstToX86regPointer(MipsRegHi(m_Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveConstToX86regPointer((m_Section->MipsRegLo_S(m_Opcode.rt) >> 31),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveConstToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rt)) {
				MoveX86regToX86regPointer(MipsRegHi(m_Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveX86regToX86regPointer(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else {	
			int X86Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			MoveX86regToX86regPointer(X86Reg,TempReg1, TempReg2);
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(m_Section->Map_TempReg(X86Reg,m_Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		if (IsConst(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rt)) {
				MoveConstToN64Mem(MipsRegHi(m_Opcode.rt),TempReg1);
			} else if (m_Section->IsSigned(m_Opcode.rt)) {
				MoveConstToN64Mem(((int)cMipsRegLo(m_Opcode.rt) >> 31),TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveConstToN64MemDisp(cMipsRegLo(m_Opcode.rt),TempReg1,4);
		} else if (m_Section->IsKnown(m_Opcode.rt) && IsMapped(m_Opcode.rt)) {
			if (m_Section->Is64Bit(m_Opcode.rt)) {
				MoveX86regToN64Mem(MipsRegHi(m_Opcode.rt),TempReg1);
			} else if (m_Section->IsSigned(m_Opcode.rt)) {
				MoveX86regToN64Mem(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE), TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveX86regToN64MemDisp(cMipsRegLo(m_Opcode.rt),TempReg1, 4);		
		} else {	
			int x86reg;
			MoveX86regToN64Mem(x86reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE), TempReg1);
			MoveX86regToN64MemDisp(m_Section->Map_TempReg(x86reg,m_Opcode.rt,FALSE), TempReg1,4);
		}
	}
#endif
}

/********************** R4300i OpCodes: Special **********************/
void CRecompilerOps::SPECIAL_SLL (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) << m_Opcode.sa;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	if (m_Opcode.rd != m_Opcode.rt && IsMapped(m_Opcode.rt)) {
		switch (m_Opcode.sa) {
		case 0: 
			Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
			break;
		case 1:
			ProtectGPR(m_Opcode.rt);
			Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			LeaRegReg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt), 2);
			break;			
		case 2:
			ProtectGPR(m_Opcode.rt);
			Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			LeaRegReg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt), 4);
			break;			
		case 3:
			ProtectGPR(m_Opcode.rt);
			Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			LeaRegReg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt), 8);
			break;
		default:
			Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
			ShiftLeftSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
	} else {
		Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
		ShiftLeftSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
	}
}

void CRecompilerOps::SPECIAL_SRL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		cMipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) >> m_Opcode.sa;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightUnsignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_SRA (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		cMipsRegLo(m_Opcode.rd) = m_Section->MipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightSignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_SLLV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x1F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			cMipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) << Shift;
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
			ShiftLeftSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)Shift);
		}
		return;
	}
	m_Section->Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftLeftSign(cMipsRegLo(m_Opcode.rd));
#endif
}

void CRecompilerOps::SPECIAL_SRLV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x1F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			cMipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) >> Shift;
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
		ShiftRightUnsignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)Shift);
		return;
	}
	m_Section->Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightUnsign(cMipsRegLo(m_Opcode.rd));
#endif
}

void CRecompilerOps::SPECIAL_SRAV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x1F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			cMipsRegLo(m_Opcode.rd) = m_Section->MipsRegLo_S(m_Opcode.rt) >> Shift;
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
		ShiftRightSignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)Shift);
		return;
	}
	m_Section->Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightSign(cMipsRegLo(m_Opcode.rd));
#endif
}

void CRecompilerOps::SPECIAL_JR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	static char JumpLabel[100];

	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		if (IsConst(m_Opcode.rs)) { 
			sprintf(JumpLabel,"0x%08X",cMipsRegLo(m_Opcode.rs));
			m_Section->m_Jump.BranchLabel   = JumpLabel;
			m_Section->m_Jump.TargetPC      = cMipsRegLo(m_Opcode.rs);
			m_Section->m_Jump.FallThrough   = TRUE;
			m_Section->m_Jump.LinkLocation  = NULL;
			m_Section->m_Jump.LinkLocation2 = NULL;
			m_Section->m_Cont.FallThrough   = FALSE;
			m_Section->m_Cont.LinkLocation  = NULL;
			m_Section->m_Cont.LinkLocation2 = NULL;
			if ((m_CompilePC & 0xFFC) == 0xFFC) {
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
				m_NextInstruction = END_BLOCK;
				return;
			}
		}
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(m_CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
			WriteBackRegisters(m_Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
			Ret();
			m_NextInstruction = END_BLOCK;
			return;
		}
		if (DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0)) {
			if (IsConst(m_Opcode.rs)) { 
				MoveConstToVariable(cMipsRegLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else 	if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		if (DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0)) {
			_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC,(DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
		} else {
			if (IsConst(m_Opcode.rs)) { 
				memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
				_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
			} else {
				if (IsMapped(m_Opcode.rs)) { 
					MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				} else {
					MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				}
				_N64System->GetRecompiler()->CompileExit (m_Section,-1, (DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
			}
		}
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
#endif
}

void CRecompilerOps::SPECIAL_JALR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	static char JumpLabel[100];	
	
	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		if (DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0)) {
			CRecompilerOps::UnknownOpcode();
		}
		UnMap_GPR( m_Opcode.rd, FALSE);
		cMipsRegLo(m_Opcode.rd) = m_CompilePC + 8;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(m_CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
			WriteBackRegisters(m_Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
			Ret();
			m_NextInstruction = END_BLOCK;
			return;
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		if (IsConst(m_Opcode.rs)) { 
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			sprintf(JumpLabel,"0x%08X",cMipsRegLo(m_Opcode.rs));
			m_Section->m_Jump.BranchLabel   = JumpLabel;
			m_Section->m_Jump.TargetPC      = cMipsRegLo(m_Opcode.rs);
			m_Section->m_Jump.FallThrough   = TRUE;
			m_Section->m_Jump.LinkLocation  = NULL;
			m_Section->m_Jump.LinkLocation2 = NULL;
			m_Section->m_Cont.FallThrough   = FALSE;
			m_Section->m_Cont.LinkLocation  = NULL;
			m_Section->m_Cont.LinkLocation2 = NULL;

			_N64System->GetRecompiler()->GenerateSectionLinkage(m_Section);
		} else {
			if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
			_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC, (DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
		}
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
#endif
}

void CRecompilerOps::SPECIAL_SYSCALL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC,m_CompilePC,m_RegWorkingSet,CExitInfo::DoSysCall,TRUE,NULL);
#endif
}

void CRecompilerOps::SPECIAL_MFLO (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	Map_GPR_64bit(m_Section,m_Opcode.rd,-1);
	MoveVariableToX86reg(&_RegLO->UW[0],"_RegLO->UW[0]",cMipsRegLo(m_Opcode.rd));
	MoveVariableToX86reg(&_RegLO->UW[1],"_RegLO->UW[1]",MipsRegHi(m_Opcode.rd));
#endif
}

void CRecompilerOps::SPECIAL_MTLO (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			MoveConstToVariable(MipsRegHi(m_Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs) && ((cMipsRegLo(m_Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveConstToVariable(cMipsRegLo(m_Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else if (m_Section->IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			MoveX86regToVariable(MipsRegHi(m_Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveX86regToVariable(cMipsRegLo(m_Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else {
		int x86reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&_RegLO->UW[1],"_RegLO->UW[1]");
		MoveX86regToVariable(m_Section->Map_TempReg(x86reg,m_Opcode.rs,FALSE), &_RegLO->UW[0],"_RegLO->UW[0]");
	}
#endif
}

void CRecompilerOps::SPECIAL_MFHI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	Map_GPR_64bit(m_Section,m_Opcode.rd,-1);
	MoveVariableToX86reg(&_RegHI->UW[0],"_RegHI->UW[0]",cMipsRegLo(m_Opcode.rd));
	MoveVariableToX86reg(&_RegHI->UW[1],"_RegHI->UW[1]",MipsRegHi(m_Opcode.rd));
#endif
}

void CRecompilerOps::SPECIAL_MTHI (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			MoveConstToVariable(MipsRegHi(m_Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs) && ((cMipsRegLo(m_Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveConstToVariable(cMipsRegLo(m_Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else if (m_Section->IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs)) {
		if (m_Section->Is64Bit(m_Opcode.rs)) {
			MoveX86regToVariable(MipsRegHi(m_Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveX86regToVariable(cMipsRegLo(m_Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else {
		int x86reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&_RegHI->UW[1],"_RegHI->UW[1]");
		MoveX86regToVariable(m_Section->Map_TempReg(x86reg,m_Opcode.rs,FALSE), &_RegHI->UW[0],"_RegHI->UW[0]");
	}
#endif
}

void CRecompilerOps::SPECIAL_DSLLV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x3F);
		CRecompilerOps::UnknownOpcode();
		return;
	}
	m_Section->Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = m_RecompPos - 1;
	ShiftLeftDouble(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	ShiftLeftSign(cMipsRegLo(m_Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = m_RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	XorX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftLeftSign(MipsRegHi(m_Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
#endif
}

void CRecompilerOps::SPECIAL_DSRLV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x3F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			m_Section->MipsReg(m_Opcode.rd) = m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt);
			m_Section->MipsReg(m_Opcode.rd) = m_Section->MipsReg(m_Opcode.rd) >> Shift;
			if ((MipsRegHi(m_Opcode.rd) == 0) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) == 0) {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else if ((MipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) != 0) {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
			}
			return;
		}
		//if (Shift < 0x20) {
		//} else {
		//}
		//CRecompilerOps::UnknownOpcode();
		//return;
	}
	m_Section->Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = m_RecompPos - 1;
	ShiftRightDouble(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	ShiftRightUnsign(MipsRegHi(m_Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = m_RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	XorX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightUnsign(cMipsRegLo(m_Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
#endif
}

void CRecompilerOps::SPECIAL_DSRAV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x3F);
		CRecompilerOps::UnknownOpcode();
		return;
	}
	m_Section->Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = m_RecompPos - 1;
	ShiftRightDouble(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	ShiftRightSign(MipsRegHi(m_Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = m_RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	ShiftRightSignImmed(MipsRegHi(m_Opcode.rd),0x1F);
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightSign(cMipsRegLo(m_Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
#endif
}

void CRecompilerOps::SPECIAL_MULT ( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	m_Section->x86Protected(x86_EDX) = TRUE;
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	m_Section->x86Protected(x86_EDX) = FALSE;
	m_Section->Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	imulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
#endif
}

void CRecompilerOps::SPECIAL_MULTU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	m_Section->x86Protected(x86_EDX) = TRUE;
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	m_Section->x86Protected(x86_EDX) = FALSE;
	m_Section->Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	MulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
#endif
}

void CRecompilerOps::SPECIAL_DIV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE *Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (IsConst(m_Opcode.rt)) {
		if (cMipsRegLo(m_Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (IsMapped(m_Opcode.rt)) {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rt),0);
		} else {
			CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = m_RecompPos - 1;

		MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
		MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
		MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
		MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	}
	/*	lo = (SD)rs / (SD)rt;
		hi = (SD)rs % (SD)rt; */

	m_Section->x86Protected(x86_EDX) = TRUE;
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);

	/* edx is the signed portion to eax */
	m_Section->x86Protected(x86_EDX) = FALSE;
	m_Section->Map_TempReg(x86_EDX, -1, FALSE);

	MoveX86RegToX86Reg(x86_EAX, x86_EDX);
	ShiftRightSignImmed(x86_EDX,31);

	if (IsMapped(m_Opcode.rt)) {
		idivX86reg(cMipsRegLo(m_Opcode.rt));
	} else {
		idivX86reg(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE));
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
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
	}
#endif
}

void CRecompilerOps::SPECIAL_DIVU ( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE *Jump[2];
	int x86reg;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (IsConst(m_Opcode.rt)) {
		if (cMipsRegLo(m_Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (IsMapped(m_Opcode.rt)) {
			CompConstToX86reg(cMipsRegLo(m_Opcode.rt),0);
		} else {
			CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = m_RecompPos - 1;

		MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
		MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
		MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
		MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	}


	/*	lo = (UD)rs / (UD)rt;
		hi = (UD)rs % (UD)rt; */

	m_Section->x86Protected(x86_EAX) = TRUE;
	m_Section->Map_TempReg(x86_EDX, 0, FALSE);
	m_Section->x86Protected(x86_EAX) = FALSE;

	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	x86reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);

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
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
	}
#endif
}

void CRecompilerOps::SPECIAL_DMULT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rs,TRUE); }
	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
	Popad();
#endif
}

void CRecompilerOps::SPECIAL_DMULTU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	/* _RegLO->UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
	m_Section->x86Protected(x86_EDX) = TRUE;
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	m_Section->x86Protected(x86_EDX) = FALSE;
	m_Section->Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

	/* _RegHI->UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,TRUE);
	m_Section->Map_TempReg(x86_EDX,m_Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

	/* Tmp[0].UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,TRUE);
	m_Section->Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	m_Section->Map_TempReg(x86_EBX,-1,FALSE);
	m_Section->Map_TempReg(x86_ECX,-1,FALSE);

	MulX86reg(x86_EDX);
	MoveX86RegToX86Reg(x86_EAX, x86_EBX); /* EDX:EAX -> ECX:EBX */
	MoveX86RegToX86Reg(x86_EDX, x86_ECX);

	/* Tmp[1].UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
	m_Section->Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	m_Section->Map_TempReg(x86_EDX,m_Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	m_Section->Map_TempReg(x86_ESI,-1,FALSE);
	m_Section->Map_TempReg(x86_EDI,-1,FALSE);
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
#endif
}

void CRecompilerOps::SPECIAL_DDIV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	UnMap_GPR(m_Opcode.rs,TRUE);
	UnMap_GPR(m_Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
	Popad();
#endif
}

void CRecompilerOps::SPECIAL_DDIVU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	UnMap_GPR(m_Opcode.rs,TRUE);
	UnMap_GPR(m_Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
	Popad();
#endif
}

void CRecompilerOps::SPECIAL_ADD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
	int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(source1) && IsConst(source2)) {
		DWORD temp = cMipsRegLo(source1) + cMipsRegLo(source2);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		cMipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(m_Opcode.rd,TRUE, source1);
	if (IsConst(source2)) {
		AddConstToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
	} else if (m_Section->IsKnown(source2) && IsMapped(source2)) {
		AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
	} else {
		AddVariableToX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
#endif
}

void CRecompilerOps::SPECIAL_ADDU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
	int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(source1) && IsConst(source2)) {
		DWORD temp = cMipsRegLo(source1) + cMipsRegLo(source2);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		cMipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(m_Opcode.rd,TRUE, source1);
	if (IsConst(source2)) {
		AddConstToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
	} else if (m_Section->IsKnown(source2) && IsMapped(source2)) {
		AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
	} else {
		AddVariableToX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
#endif
}

void CRecompilerOps::SPECIAL_SUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		DWORD temp = cMipsRegLo(m_Opcode.rs) - cMipsRegLo(m_Opcode.rt);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		cMipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			int x86Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
		} else {
			SubVariableFromX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_SUBU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		DWORD temp = cMipsRegLo(m_Opcode.rs) - cMipsRegLo(m_Opcode.rt);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		cMipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			int x86Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
		} else {
			SubVariableFromX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_AND (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				m_Section->MipsReg(m_Opcode.rd) = 
					(m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt)) &
					(m_Section->Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(_int64)m_Section->MipsRegLo_S(m_Opcode.rs));
				
				if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				m_Section->MipsReg(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) & m_Section->MipsReg(m_Opcode.rs);
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			}			
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
		
			ProtectGPR(source1);
			ProtectGPR(source2);
			if (m_Section->Is32Bit(source1) && m_Section->Is32Bit(source2)) {
				int Sign = (m_Section->IsSigned(m_Opcode.rt) && m_Section->IsSigned(m_Opcode.rs))?TRUE:FALSE;
				Map_GPR_32bit(m_Opcode.rd,Sign,source1);				
				AndX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			} else if (m_Section->Is32Bit(source1) || m_Section->Is32Bit(source2)) {
				if (m_Section->IsUnsigned(m_Section->Is32Bit(source1)?source1:source2)) {
					Map_GPR_32bit(m_Opcode.rd,FALSE,source1);
					AndX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
				} else {
					Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
					if (m_Section->Is32Bit(source2)) {
						AndX86RegToX86Reg(MipsRegHi(m_Opcode.rd),m_Section->Map_TempReg(x86_Any,source2,TRUE));
					} else {
						AndX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
					}
					AndX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
				}
			} else {
				Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
				AndX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
				AndX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			}
		} else {
			int ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			int MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (m_Section->Is64Bit(ConstReg)) {
				if (m_Section->Is32Bit(MappedReg) && m_Section->IsUnsigned(MappedReg)) {
					if (cMipsRegLo(ConstReg) == 0) {
						Map_GPR_32bit(m_Opcode.rd,FALSE, 0);
					} else {
						DWORD Value = cMipsRegLo(ConstReg);
						Map_GPR_32bit(m_Opcode.rd,FALSE, MappedReg);
						AndConstToX86Reg(cMipsRegLo(m_Opcode.rd),Value);
					}
				} else {
					_int64 Value = m_Section->MipsReg(ConstReg);
					Map_GPR_64bit(m_Section,m_Opcode.rd,MappedReg);
					AndConstToX86Reg(MipsRegHi(m_Opcode.rd),(DWORD)(Value >> 32));
					AndConstToX86Reg(cMipsRegLo(m_Opcode.rd),(DWORD)Value);
				}
			} else if (m_Section->Is64Bit(MappedReg)) {
				DWORD Value = cMipsRegLo(ConstReg); 
				if (Value != 0) {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(ConstReg)?TRUE:FALSE,MappedReg);					
					AndConstToX86Reg(cMipsRegLo(m_Opcode.rd),(DWORD)Value);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(ConstReg)?TRUE:FALSE, 0);
				}
			} else {
				DWORD Value = cMipsRegLo(ConstReg); 
				int Sign = FALSE;
				if (m_Section->IsSigned(ConstReg) && m_Section->IsSigned(MappedReg)) { Sign = TRUE; }				
				if (Value != 0) {
					Map_GPR_32bit(m_Opcode.rd,Sign,MappedReg);
					AndConstToX86Reg(cMipsRegLo(m_Opcode.rd),Value);
				} else {
					Map_GPR_32bit(m_Opcode.rd,FALSE, 0);
				}
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

		if (IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				unsigned __int64 Value = m_Section->MipsReg(KnownReg);
				Map_GPR_64bit(m_Section,m_Opcode.rd,UnknownReg);
				AndConstToX86Reg(MipsRegHi(m_Opcode.rd),(DWORD)(Value >> 32));
				AndConstToX86Reg(cMipsRegLo(m_Opcode.rd),(DWORD)Value);
			} else {
				DWORD Value = cMipsRegLo(KnownReg);
				Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(KnownReg),UnknownReg);
				AndConstToX86Reg(cMipsRegLo(m_Opcode.rd),(DWORD)Value);
			}
		} else {
			ProtectGPR(KnownReg);
			if (KnownReg == m_Opcode.rd) {
				if (m_Section->Is64Bit(KnownReg)) {
					Map_GPR_64bit(m_Section,m_Opcode.rd,KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegHi(m_Opcode.rd));
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegLo(m_Opcode.rd));
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(KnownReg),KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegLo(m_Opcode.rd));
				}
			} else {
				if (m_Section->Is64Bit(KnownReg)) {
					Map_GPR_64bit(m_Section,m_Opcode.rd,UnknownReg);
					AndX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(KnownReg));
					AndX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(KnownReg));
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(KnownReg),UnknownReg);
					AndX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(KnownReg));
				}
			}
		}
	} else {
		Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
		AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],MipsRegHi(m_Opcode.rd));
		AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_OR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {				
				m_Section->MipsReg(m_Opcode.rd) = 
					(m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt)) |
					(m_Section->Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(_int64)m_Section->MipsRegLo_S(m_Opcode.rs));
				if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				cMipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) | cMipsRegLo(m_Opcode.rs);
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
			
			ProtectGPR(m_Opcode.rt);
			ProtectGPR(m_Opcode.rs);
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
				if (m_Section->Is64Bit(source2)) {
					OrX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(MipsRegHi(m_Opcode.rd),m_Section->Map_TempReg(x86_Any,source2,TRUE));
				}
			} else {
				ProtectGPR(source2);
				Map_GPR_32bit(m_Opcode.rd,TRUE,source1);
			}
			OrX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				unsigned _int64 Value;

				if (m_Section->Is64Bit(ConstReg)) {
					Value = m_Section->MipsReg(ConstReg);
				} else {
					Value = m_Section->IsSigned(ConstReg)?m_Section->MipsRegLo_S(ConstReg):cMipsRegLo(ConstReg);
				}
				Map_GPR_64bit(m_Section,m_Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(m_Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,cMipsRegLo(m_Opcode.rd));
				}
			} else {
				int Value = cMipsRegLo(ConstReg);
				Map_GPR_32bit(m_Opcode.rd,TRUE, MappedReg);
				if (Value != 0) { OrConstToX86Reg(Value,cMipsRegLo(m_Opcode.rd)); }
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		int UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = m_Section->Is64Bit(KnownReg)?m_Section->MipsReg(KnownReg):m_Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(m_Section,m_Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(m_Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,cMipsRegLo(m_Opcode.rd));
			}
		} else {
			Map_GPR_64bit(m_Section,m_Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegHi(m_Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegLo(m_Opcode.rd));
		}
	} else {
		Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],MipsRegHi(m_Opcode.rd));
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegLo(m_Opcode.rd));
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (SPHack && m_Opcode.rd == 29) { 
		m_Section->ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
#endif

}

void CRecompilerOps::SPECIAL_XOR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (m_Opcode.rt == m_Opcode.rs) {
		UnMap_GPR( m_Opcode.rd, FALSE);
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		cMipsRegLo(m_Opcode.rd) = 0;
		return;
	}
	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("XOR 1");
#endif
				CRecompilerOps::UnknownOpcode();
			} else {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				cMipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) ^ cMipsRegLo(m_Opcode.rs);
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
			
			ProtectGPR(source1);
			ProtectGPR(source2);
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
				if (m_Section->Is64Bit(source2)) {
					XorX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
				} else if (m_Section->IsSigned(source2)) {
					XorX86RegToX86Reg(MipsRegHi(m_Opcode.rd),m_Section->Map_TempReg(x86_Any,source2,TRUE));
				}
				XorX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			} else {
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(m_Opcode.rt),source1);
				}
				XorX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				DWORD ConstHi, ConstLo;

				ConstHi = m_Section->Is32Bit(ConstReg)?(DWORD)(m_Section->MipsRegLo_S(ConstReg) >> 31):MipsRegHi(ConstReg);
				ConstLo = cMipsRegLo(ConstReg);
				Map_GPR_64bit(m_Section,m_Opcode.rd,MappedReg);
				if (ConstHi != 0) { XorConstToX86Reg(MipsRegHi(m_Opcode.rd),ConstHi); }
				if (ConstLo != 0) { XorConstToX86Reg(cMipsRegLo(m_Opcode.rd),ConstLo); }
			} else {
				int Value = cMipsRegLo(ConstReg);
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { XorConstToX86Reg(cMipsRegLo(m_Opcode.rd),Value); }
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		int UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned _int64 Value;

			if (m_Section->Is64Bit(KnownReg)) {
				Value = m_Section->MipsReg(KnownReg);
			} else {
				if (m_Section->IsSigned(KnownReg)) {
					Value = (int)cMipsRegLo(KnownReg);
				} else {
					Value = cMipsRegLo(KnownReg);
				}
			}
			Map_GPR_64bit(m_Section,m_Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				XorConstToX86Reg(MipsRegHi(m_Opcode.rd),(DWORD)(Value >> 32));
			}
			if ((DWORD)Value != 0) {
				XorConstToX86Reg(cMipsRegLo(m_Opcode.rd),(DWORD)Value);
			}
		} else {
			Map_GPR_64bit(m_Section,m_Opcode.rd,KnownReg);
			XorVariableToX86reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegHi(m_Opcode.rd));
			XorVariableToX86reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegLo(m_Opcode.rd));
		}
	} else {
		Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
		XorVariableToX86reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],MipsRegHi(m_Opcode.rd));
		XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_NOR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {				
				CRecompilerOps::UnknownOpcode();
			} else {
				cMipsRegLo(m_Opcode.rd) = ~(cMipsRegLo(m_Opcode.rt) | cMipsRegLo(m_Opcode.rs));
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
			
			ProtectGPR(source1);
			ProtectGPR(source2);
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
				if (m_Section->Is64Bit(source2)) {
					OrX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(MipsRegHi(m_Opcode.rd),m_Section->Map_TempReg(x86_Any,source2,TRUE));
				}
				OrX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
				NotX86Reg(MipsRegHi(m_Opcode.rd));
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			} else {
				ProtectGPR(source2);
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(m_Opcode.rt),source1);
				}
				OrX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				unsigned _int64 Value;

				if (m_Section->Is64Bit(ConstReg)) {
					Value = m_Section->MipsReg(ConstReg);
				} else {
					Value = m_Section->IsSigned(ConstReg)?m_Section->MipsRegLo_S(ConstReg):cMipsRegLo(ConstReg);
				}
				Map_GPR_64bit(m_Section,m_Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(m_Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,cMipsRegLo(m_Opcode.rd));
				}
				NotX86Reg(MipsRegHi(m_Opcode.rd));
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			} else {
				int Value = cMipsRegLo(ConstReg);
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { OrConstToX86Reg(Value,cMipsRegLo(m_Opcode.rd)); }
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		int UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = m_Section->Is64Bit(KnownReg)?m_Section->MipsReg(KnownReg):m_Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(m_Section,m_Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(m_Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,cMipsRegLo(m_Opcode.rd));
			}
		} else {
			Map_GPR_64bit(m_Section,m_Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegHi(m_Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegLo(m_Opcode.rd));
		}
		NotX86Reg(MipsRegHi(m_Opcode.rd));
		NotX86Reg(cMipsRegLo(m_Opcode.rd));
	} else {
		Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],MipsRegHi(m_Opcode.rd));
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegLo(m_Opcode.rd));
		NotX86Reg(MipsRegHi(m_Opcode.rd));
		NotX86Reg(cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_SLT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				DisplayError("1");
				CRecompilerOps::UnknownOpcode();
			} else {
				if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (m_Section->MipsRegLo_S(m_Opcode.rs) < m_Section->MipsRegLo_S(m_Opcode.rt)) {
					cMipsRegLo(m_Opcode.rd) = 1;
				} else {
					cMipsRegLo(m_Opcode.rd) = 0;
				}
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			ProtectGPR(m_Opcode.rt);
			ProtectGPR(m_Opcode.rs);
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					m_Section->Is64Bit(m_Opcode.rs)?MipsRegHi(m_Opcode.rs):m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE), 
					m_Section->Is64Bit(m_Opcode.rt)?MipsRegHi(m_Opcode.rt):m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				SetlVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(cMipsRegLo(m_Opcode.rs), cMipsRegLo(m_Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
			} else {
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				CompX86RegToX86Reg(cMipsRegLo(m_Opcode.rs), cMipsRegLo(m_Opcode.rt));

				if (cMipsRegLo(m_Opcode.rd) > x86_EDX) {
					SetlVariable(&BranchCompare,"BranchCompare");
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
				} else {					
					Setl(cMipsRegLo(m_Opcode.rd));
					AndConstToX86Reg(cMipsRegLo(m_Opcode.rd), 1);
				}
			}
		} else {
			DWORD ConstReg  = IsConst(m_Opcode.rs)?m_Opcode.rs:m_Opcode.rt;
			DWORD MappedReg = IsConst(m_Opcode.rs)?m_Opcode.rt:m_Opcode.rs;

			ProtectGPR(MappedReg);
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				BYTE *Jump[2];

				CompConstToX86reg(
					m_Section->Is64Bit(MappedReg)?MipsRegHi(MappedReg):m_Section->Map_TempReg(x86_Any,MappedReg,TRUE), 
					m_Section->Is64Bit(ConstReg)?MipsRegHi(ConstReg):(m_Section->MipsRegLo_S(ConstReg) >> 31)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				if (MappedReg == m_Opcode.rs) {
					SetlVariable(&BranchCompare,"BranchCompare");
				} else {
					SetgVariable(&BranchCompare,"BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompConstToX86reg(cMipsRegLo(MappedReg), cMipsRegLo(ConstReg));
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
			} else {
				DWORD Constant = cMipsRegLo(ConstReg);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				CompConstToX86reg(cMipsRegLo(MappedReg), Constant);
			
				if (cMipsRegLo(m_Opcode.rd) > x86_EDX) {
					if (MappedReg == m_Opcode.rs) {
						SetlVariable(&BranchCompare,"BranchCompare");
					} else {
						SetgVariable(&BranchCompare,"BranchCompare");
					}
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
				} else {					
					if (MappedReg == m_Opcode.rs) {
						Setl(cMipsRegLo(m_Opcode.rd));
					} else {
						Setg(cMipsRegLo(m_Opcode.rd));
					}
					AndConstToX86Reg(cMipsRegLo(m_Opcode.rd), 1);
				}
			}
		}		
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		BYTE *Jump[2];
			
		if (IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(((int)cMipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(KnownReg);
				CompX86regToVariable(m_Section->Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetgVariable(&BranchCompare,"BranchCompare");
		} else {
			SetlVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompX86regToVariable(m_Section->Map_TempReg(x86Reg,m_Opcode.rs,FALSE),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_SLTU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("1");
#endif
				CRecompilerOps::UnknownOpcode();
			} else {
				if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (cMipsRegLo(m_Opcode.rs) < cMipsRegLo(m_Opcode.rt)) {
					cMipsRegLo(m_Opcode.rd) = 1;
				} else {
					cMipsRegLo(m_Opcode.rd) = 0;
				}
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			ProtectGPR(m_Opcode.rt);
			ProtectGPR(m_Opcode.rs);
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					m_Section->Is64Bit(m_Opcode.rs)?MipsRegHi(m_Opcode.rs):m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE), 
					m_Section->Is64Bit(m_Opcode.rt)?MipsRegHi(m_Opcode.rt):m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				SetbVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(cMipsRegLo(m_Opcode.rs), cMipsRegLo(m_Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
			} else {
				CompX86RegToX86Reg(cMipsRegLo(m_Opcode.rs), cMipsRegLo(m_Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
			}
		} else {
			if (m_Section->Is64Bit(m_Opcode.rt) || m_Section->Is64Bit(m_Opcode.rs)) {
				DWORD MappedRegHi, MappedRegLo, ConstHi, ConstLo, MappedReg, ConstReg;
				BYTE *Jump[2];

				ConstReg  = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
				MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
				
				ConstLo = cMipsRegLo(ConstReg);
				ConstHi = (int)ConstLo >> 31;
				if (m_Section->Is64Bit(ConstReg)) { ConstHi = MipsRegHi(ConstReg); }

				ProtectGPR(MappedReg);
				MappedRegLo = cMipsRegLo(MappedReg);
				MappedRegHi = MipsRegHi(MappedReg);
				if (m_Section->Is32Bit(MappedReg)) {
					MappedRegHi = m_Section->Map_TempReg(x86_Any,MappedReg,TRUE);
				}

		
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				CompConstToX86reg(MappedRegHi, ConstHi);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
	
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompConstToX86reg(MappedRegLo, ConstLo);
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
			} else {
				DWORD Const = IsConst(m_Opcode.rs)?cMipsRegLo(m_Opcode.rs):cMipsRegLo(m_Opcode.rt);
				DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

				CompConstToX86reg(cMipsRegLo(MappedReg), Const);
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
			}
		}		
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		BYTE *Jump[2];
			
		if (IsConst(KnownReg)) {
			if (m_Section->Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(((int)cMipsRegLo(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (m_Section->Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(KnownReg);
				CompX86regToVariable(m_Section->Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = m_Section->Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompX86regToVariable(m_Section->Map_TempReg(x86Reg,m_Opcode.rs,FALSE),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_DADD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = 
			m_Section->Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(_int64)m_Section->MipsRegLo_S(m_Opcode.rs) +
			m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt);		
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
		int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

		Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
		if (IsConst(source2)) {
			AddConstToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			AddConstToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
		} else if (IsMapped(source2)) {
			int HiReg = m_Section->Is64Bit(source2)?MipsRegHi(source2):m_Section->Map_TempReg(x86_Any,source2,TRUE);
			ProtectGPR(source2);
			AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			AdcX86RegToX86Reg(MipsRegHi(m_Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(MipsRegHi(m_Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DADDU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		__int64 ValRs = m_Section->Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(_int64)m_Section->MipsRegLo_S(m_Opcode.rs);
		__int64 ValRt = m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = ValRs + ValRt;
		if ((MipsRegHi(m_Opcode.rd) == 0) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) == 0) {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if ((MipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) != 0) {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
		int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

		Map_GPR_64bit(m_Section,m_Opcode.rd,source1);
		if (IsConst(source2)) {
			AddConstToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			AddConstToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
		} else if (IsMapped(source2)) {
			int HiReg = m_Section->Is64Bit(source2)?MipsRegHi(source2):m_Section->Map_TempReg(x86_Any,source2,TRUE);
			ProtectGPR(source2);
			AddX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			AdcX86RegToX86Reg(MipsRegHi(m_Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(MipsRegHi(m_Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = 
			m_Section->Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(_int64)m_Section->MipsRegLo_S(m_Opcode.rs) -
			m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt);		
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			int HiReg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			int LoReg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),LoReg);
			SbbX86RegToX86Reg(MipsRegHi(m_Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbConstFromX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			int HiReg = m_Section->Is64Bit(m_Opcode.rt)?MipsRegHi(m_Opcode.rt):m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			ProtectGPR(m_Opcode.rt);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbX86RegToX86Reg(MipsRegHi(m_Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
			SbbVariableFromX86reg(MipsRegHi(m_Opcode.rd),&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSUBU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = 
			m_Section->Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(_int64)m_Section->MipsRegLo_S(m_Opcode.rs) -
			m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt);		
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			int HiReg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			int LoReg = m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),LoReg);
			SbbX86RegToX86Reg(MipsRegHi(m_Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbConstFromX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			int HiReg = m_Section->Is64Bit(m_Opcode.rt)?MipsRegHi(m_Opcode.rt):m_Section->Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			ProtectGPR(m_Opcode.rt);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbX86RegToX86Reg(MipsRegHi(m_Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
			SbbVariableFromX86reg(MipsRegHi(m_Opcode.rd),&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSLL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }

		m_Section->MipsReg(m_Opcode.rd) = m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt) << m_Opcode.sa;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}
	
	Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
	ShiftLeftDoubleImmed(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
	ShiftLeftSignImmed(	cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_DSRL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }

		m_Section->MipsReg(m_Opcode.rd) = m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(QWORD)m_Section->MipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
	ShiftRightDoubleImmed(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
	ShiftRightUnsignImmed(MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_DSRA (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }

		m_Section->MipsReg_S(m_Opcode.rd) = m_Section->Is64Bit(m_Opcode.rt)?m_Section->MipsReg_S(m_Opcode.rt):(_int64)m_Section->MipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(m_Section,m_Opcode.rd,m_Opcode.rt);
	ShiftRightDoubleImmed(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
	ShiftRightSignImmed(MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_DSLL32 (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (m_Opcode.rt != m_Opcode.rd) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegHi(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) << m_Opcode.sa;
		cMipsRegLo(m_Opcode.rd) = 0;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}

	} else if (IsMapped(m_Opcode.rt)) {
		ProtectGPR(m_Opcode.rt);
		Map_GPR_64bit(m_Section,m_Opcode.rd,-1);		
		if (m_Opcode.rt != m_Opcode.rd) {
			MoveX86RegToX86Reg(cMipsRegLo(m_Opcode.rt),MipsRegHi(m_Opcode.rd));
		} else {
			int HiReg = MipsRegHi(m_Opcode.rt);
			MipsRegHi(m_Opcode.rt) = cMipsRegLo(m_Opcode.rt);
			cMipsRegLo(m_Opcode.rt) = HiReg;
		}
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftLeftSignImmed(MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
		XorX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	} else {
		Map_GPR_64bit(m_Section,m_Opcode.rd,-1);
		MoveVariableToX86reg(&_GPR[m_Opcode.rt],CRegName::GPR_Hi[m_Opcode.rt],MipsRegHi(m_Opcode.rd));
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftLeftSignImmed(MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
		XorX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_DSRL32 (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (IsConst(m_Opcode.rt)) {
		if (m_Opcode.rt != m_Opcode.rd) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		cMipsRegLo(m_Opcode.rd) = (DWORD)(m_Section->MipsReg(m_Opcode.rt) >> (m_Opcode.sa + 32));
	} else if (IsMapped(m_Opcode.rt)) {
		ProtectGPR(m_Opcode.rt);
		if (m_Section->Is64Bit(m_Opcode.rt)) {
			if (m_Opcode.rt == m_Opcode.rd) {
				int HiReg = MipsRegHi(m_Opcode.rt);
				MipsRegHi(m_Opcode.rt) = cMipsRegLo(m_Opcode.rt);
				cMipsRegLo(m_Opcode.rt) = HiReg;
				Map_GPR_32bit(m_Opcode.rd,FALSE,-1);
			} else {
				Map_GPR_32bit(m_Opcode.rd,FALSE,-1);
				MoveX86RegToX86Reg(MipsRegHi(m_Opcode.rt),cMipsRegLo(m_Opcode.rd));
			}
			if ((BYTE)m_Opcode.sa != 0) {
				ShiftRightUnsignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
			}
		} else {
			CRecompilerOps::UnknownOpcode();
		}
	} else {
		Map_GPR_32bit(m_Opcode.rd,FALSE,-1);
		MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1],CRegName::GPR_Lo[m_Opcode.rt],cMipsRegLo(m_Opcode.rd));
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftRightUnsignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSRA32 (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (IsConst(m_Opcode.rt)) {
		if (m_Opcode.rt != m_Opcode.rd) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		cMipsRegLo(m_Opcode.rd) = (DWORD)(m_Section->MipsReg_S(m_Opcode.rt) >> (m_Opcode.sa + 32));
	} else if (IsMapped(m_Opcode.rt)) {
		ProtectGPR(m_Opcode.rt);
		if (m_Section->Is64Bit(m_Opcode.rt)) {
			if (m_Opcode.rt == m_Opcode.rd) {
				int HiReg = MipsRegHi(m_Opcode.rt);
				MipsRegHi(m_Opcode.rt) = cMipsRegLo(m_Opcode.rt);
				cMipsRegLo(m_Opcode.rt) = HiReg;
				Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			} else {
				Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
				MoveX86RegToX86Reg(MipsRegHi(m_Opcode.rt),cMipsRegLo(m_Opcode.rd));
			}
			if ((BYTE)m_Opcode.sa != 0) {
				ShiftRightSignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
			}
		} else {
			CRecompilerOps::UnknownOpcode();
		}
	} else {
		Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
		MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1],CRegName::GPR_Lo[m_Opcode.rt],cMipsRegLo(m_Opcode.rd));
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftRightSignImmed(cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
	}
#endif
}

/************************** COP0 functions **************************/
void CRecompilerOps::COP0_MF(void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	switch (m_Opcode.rd) {
	case 9: //Count
		UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
		m_RegWorkingSet.BlockCycleCount() = 0;
		m_RegWorkingSet.BlockRandomModifier() = 0;
	}
	Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_CP0[m_Opcode.rd],CRegName::Cop0[m_Opcode.rd],cMipsRegLo(m_Opcode.rt));
#endif
}

void ChangeCompareTimer (void) {
	_SystemTimer->SetTimer(CSystemTimer::CompareTimer, _Reg->COMPARE_REGISTER - _Reg->COUNT_REGISTER,false);
}

void CRecompilerOps::COP0_MT (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	BYTE *Jump;

	switch (m_Opcode.rd) {
	case 0: //Index
	case 2: //EntryLo0
	case 3: //EntryLo1
	case 4: //Context
	case 5: //PageMask
	case 10: //Entry Hi
	case 14: //EPC
	case 16: //Config
	case 18: //WatchLo 
	case 19: //WatchHi
	case 28: //Tag lo
	case 29: //Tag Hi
	case 30: //ErrEPC
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		if (m_Opcode.rd == 4) //Context
		{
			AndConstToVariable(0xFF800000,&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		break;
	case 11: //Compare
		UpdateCounters(m_RegWorkingSet,false,true);
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
		Popad();
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		AndConstToVariable(~CAUSE_IP7,&_Reg->FAKE_CAUSE_REGISTER,"FAKE_CAUSE_REGISTER");
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
		Popad();
		break;
	case 9: //Count
		UpdateCounters(m_RegWorkingSet,false,true);
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
		Popad();
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
		Popad();
		break;
	case 12: //Status
		{
			x86Reg OldStatusReg = m_Section->Map_TempReg(x86_Any,-1,FALSE);
			MoveVariableToX86reg(&_CP0[m_Opcode.rd],CRegName::Cop0[m_Opcode.rd],OldStatusReg);
			if (IsConst(m_Opcode.rt)) {
				MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
			} else if (IsMapped(m_Opcode.rt)) {
				MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
			} else {
				MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
			}
			XorVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd],OldStatusReg);
			TestConstToX86Reg(STATUS_FR,OldStatusReg);
			JeLabel8("FpuFlagFine",0);
			Jump = m_RecompPos - 1;
			Pushad();
			Call_Direct(SetFpuLocations,"SetFpuLocations");
			Popad();
			*(BYTE *)(Jump)= (BYTE )(((BYTE )(m_RecompPos)) - (((BYTE )(Jump)) + 1));
					
			//TestConstToX86Reg(STATUS_FR,OldStatusReg);
			//BreakPoint(__FILE__,__LINE__); //_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC+4,m_RegWorkingSet,ExitResetRecompCode,FALSE,JneLabel32);
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
		}
		break;
	case 6: //Wired
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		Pushad();
		UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
		m_RegWorkingSet.BlockCycleCount() = 0;
		m_RegWorkingSet.BlockRandomModifier() = 0;
		Call_Direct(FixRandomReg,"FixRandomReg");
		Popad();
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(m_Section->Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
#endif
		break;
	case 13: //cause
		if (IsConst(m_Opcode.rt)) {
			AndConstToVariable(0xFFFFCFF,&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
#ifndef EXTERNAL_RELEASE
			if ((cMipsRegLo(m_Opcode.rt) & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		} else {
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			CRecompilerOps::UnknownOpcode();
#endif
		}
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		CRecompilerOps::UnknownOpcode();
#endif
	}
}

/************************** COP0 CO functions ***********************/
void CRecompilerOps::COP0_CO_TLBR( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (!g_UseTlb) {	return; }
	Pushad();
	Call_Direct(TLB_ReadEntry,"TLB_ReadEntry");
	Popad();
#endif
}

void CRecompilerOps::COP0_CO_TLBWI( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (!g_UseTlb) {	return; }
	Pushad();
	PushImm32("FALSE",FALSE);
	MoveVariableToX86reg(&_Reg->INDEX_REGISTER,"INDEX_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Push(x86_ECX);
	Call_Direct(TLB_WriteEntry,"TLB_WriteEntry");
	AddConstToX86Reg(x86_ESP,8);
	Popad();
#endif
}

void CRecompilerOps::COP0_CO_TLBWR( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (!g_UseTlb) {	return; }

	UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
	m_RegWorkingSet.BlockCycleCount() = 0;
	m_RegWorkingSet.BlockRandomModifier() = 0;
	Pushad();
	Call_Direct(FixRandomReg,"FixRandomReg");
	PushImm32("TRUE",TRUE);
	MoveVariableToX86reg(&_Reg->RANDOM_REGISTER,"RANDOM_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Push(x86_ECX);
	Call_Direct(TLB_WriteEntry,"TLB_WriteEntry");
	AddConstToX86Reg(x86_ESP,8);
	Popad();
#endif
}

void CRecompilerOps::COP0_CO_TLBP( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (!g_UseTlb) {	return; }
	Pushad();
	MoveConstToX86reg((DWORD)_TLB,x86_ECX);		
	Call_Direct(AddressOf(CTLB::Probe), "CTLB::TLB_Probe");
	Popad();
#endif
}

void compiler_COP0_CO_ERET (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if ((_Reg->STATUS_REGISTER & STATUS_ERL) != 0) {
		*_PROGRAM_COUNTER = _Reg->ERROREPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		*_PROGRAM_COUNTER = _Reg->EPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_EXL;
	}
	*_LLBit = 0;
	CheckInterrupts();
#endif
}

void CRecompilerOps::COP0_CO_ERET( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	WriteBackRegisters(m_Section);
	Call_Direct(compiler_COP0_CO_ERET,"compiler_COP0_CO_ERET");
	_N64System->GetRecompiler()->CompileExit (m_Section,m_CompilePC, (DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
	m_NextInstruction = END_BLOCK;
#endif
}

/************************** FPU Options **************************/
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

void CompileCop1Test (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (Section->FpuBeenUsed()) { return; }
	TestVariable(STATUS_CU1,&_Reg->STATUS_REGISTER,"STATUS_REGISTER");
	_N64System->GetRecompiler()->CompileExit(Section,Section->m_CompilePC,Section->m_CompilePC,Section->RegWorking,CExitInfo::COP1_Unuseable,FALSE,JeLabel32);
	Section->FpuBeenUsed() = TRUE;
#endif
}

/********************** Load/store functions ************************/
void CRecompilerOps::LWC1 (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
		DWORD Address = Section->cMipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;

		TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
		_MMU->Compile_LW(Section, TempReg1,Address);

		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.ft);
		MoveVariableToX86reg(&_FPRFloatLocation[m_CompileOpcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg1,TempReg2);
		return;
	}
	if (Section->IsMapped(m_CompileOpcode.base) && m_CompileOpcode.offset == 0) { 
		if (g_UseTlb) {
			ProtectGPR(Section,m_CompileOpcode.base);
			TempReg1 = Section->cMipsRegLo(m_CompileOpcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		}
	} else {
		if (Section->IsMapped(m_CompileOpcode.base)) { 
			ProtectGPR(Section,m_CompileOpcode.base);
			if (m_CompileOpcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,Section->cMipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
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
	if (g_UseTlb) {
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
#endif
}

void CRecompilerOps::LDC1 (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, TempReg3;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);

	UnMap_FPR(Section,m_CompileOpcode.ft,FALSE);
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->cMipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;
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
		if (g_UseTlb) {
			ProtectGPR(Section,m_CompileOpcode.base);
			TempReg1 = Section->cMipsRegLo(m_CompileOpcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,m_CompileOpcode.base,FALSE);
		}
	} else {
		if (Section->IsMapped(m_CompileOpcode.base)) { 
			ProtectGPR(Section,m_CompileOpcode.base);
			if (m_CompileOpcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,Section->cMipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
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
	if (g_UseTlb) {
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
#endif
}

void CRecompilerOps::SWC1 (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, TempReg3;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->cMipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;
		
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
			LeaSourceAndOffset(TempReg1,Section->cMipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
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
	if (g_UseTlb) {
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
#endif
}

void CRecompilerOps::SDC1 (void){
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg1, TempReg2, TempReg3;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	
	if (Section->IsConst(m_CompileOpcode.base)) { 
		DWORD Address = Section->cMipsRegLo(m_CompileOpcode.base) + (short)m_CompileOpcode.offset;
		
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
			LeaSourceAndOffset(TempReg1,Section->cMipsRegLo(m_CompileOpcode.base),(short)m_CompileOpcode.offset);
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
	if (g_UseTlb) {
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
#endif
}

/************************** COP1 functions **************************/
void CRecompilerOps::COP1_MF (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD TempReg;

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);

	UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	Map_GPR_32bit(Section,m_CompileOpcode.rt, TRUE, -1);
	TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
	sprintf(Name,"_FPRFloatLocation[%d]",m_CompileOpcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRFloatLocation[m_CompileOpcode.fs],Name,TempReg);
	MoveX86PointerToX86reg(Section->cMipsRegLo(m_CompileOpcode.rt),TempReg);		
#endif
}

void CRecompilerOps::COP1_DMF (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
	MoveX86PointerToX86reg(Section->cMipsRegLo(m_CompileOpcode.rt),TempReg);		
#endif
}

void CRecompilerOps::COP1_CF(void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));

	CompileCop1Test(Section);
	
	if (m_CompileOpcode.fs != 31 && m_CompileOpcode.fs != 0) { CRecompilerOps::UnknownOpcode (Section); return; }
	Map_GPR_32bit(Section,m_CompileOpcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs],Section->cMipsRegLo(m_CompileOpcode.rt));
#endif
}

void CRecompilerOps::COP1_MT( void) {	
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
		MoveConstToX86Pointer(Section->cMipsRegLo(m_CompileOpcode.rt),TempReg);
	} else if (Section->IsMapped(m_CompileOpcode.rt)) {
		MoveX86regToX86Pointer(Section->cMipsRegLo(m_CompileOpcode.rt),TempReg);
	} else {
		MoveX86regToX86Pointer(Map_TempReg(Section,x86_Any, m_CompileOpcode.rt, FALSE),TempReg);
	}
#endif
}

void CRecompilerOps::COP1_DMT( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
		MoveConstToX86Pointer(Section->cMipsRegLo(m_CompileOpcode.rt),TempReg);
		AddConstToX86Reg(TempReg,4);
		if (Section->Is64Bit(m_CompileOpcode.rt)) {
			MoveConstToX86Pointer(Section->MipsRegHi(m_CompileOpcode.rt),TempReg);
		} else {
			MoveConstToX86Pointer(Section->MipsRegLo_S(m_CompileOpcode.rt) >> 31,TempReg);
		}
	} else if (Section->IsMapped(m_CompileOpcode.rt)) {
		MoveX86regToX86Pointer(Section->cMipsRegLo(m_CompileOpcode.rt),TempReg);
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
#endif
}


void CRecompilerOps::COP1_CT(void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix

	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);
	if (m_CompileOpcode.fs != 31) { CRecompilerOps::UnknownOpcode (Section); return; }

	if (Section->IsConst(m_CompileOpcode.rt)) {
		MoveConstToVariable(Section->cMipsRegLo(m_CompileOpcode.rt),&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs]);
	} else if (Section->IsMapped(m_CompileOpcode.rt)) {
		MoveX86regToVariable(Section->cMipsRegLo(m_CompileOpcode.rt),&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs]);
	} else {
		MoveX86regToVariable(Map_TempReg(Section,x86_Any,m_CompileOpcode.rt,FALSE),&_FPCR[m_CompileOpcode.fs],CRegName::FPR_Ctrl[m_CompileOpcode.fs]);		
	}
	Pushad();
	Call_Direct(ChangeDefaultRoundingModel, "ChangeDefaultRoundingModel");
	Popad();
	Section->CurrentRoundingModel() = CRegInfo::RoundUnknown;
#endif
}

/************************** COP1: S functions ************************/
void CRecompilerOps::COP1_S_ADD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_S_SUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_S_MUL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_S_DIV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_S_ABS (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	fpuAbs();
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_NEG (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	fpuNeg();
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_SQRT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	fpuSqrt();
	UnMap_FPR(Section,m_CompileOpcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_MOV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	CompileCop1Test(Section);	
	FixRoundModel(Section,CRegInfo::RoundDefault);
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
#endif
}

void CRecompilerOps::COP1_S_TRUNC_L (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_S_CEIL_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_S_FLOOR_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_S_ROUND_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundNearest);
#endif
}

void CRecompilerOps::COP1_S_TRUNC_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_S_CEIL_W (void) {			// added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_S_FLOOR_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_S_CVT_D (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_S_CVT_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_S_CVT_L (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_S_CMP (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.fs:m_CompileOpcode.ft;
	int x86reg, cmp = 0;

	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	//if ((m_CompileOpcode.funct & 1) != 0) { CRecompilerOps::UnknownOpcode(Section); }
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
#endif
}

/************************** COP1: D functions ************************/
void CRecompilerOps::COP1_D_ADD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_D_SUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_D_MUL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_D_DIV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
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
#endif
}

void CRecompilerOps::COP1_D_ABS (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
	fpuAbs();
#endif
}

void CRecompilerOps::COP1_D_NEG (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
	fpuNeg();
#endif
}

void CRecompilerOps::COP1_D_SQRT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
	fpuSqrt();
#endif
}

void CRecompilerOps::COP1_D_MOV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double);
#endif
}

void CRecompilerOps::COP1_D_TRUNC_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_D_CEIL_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_D_FLOOR_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_D_ROUND_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundNearest);
#endif
}

void CRecompilerOps::COP1_D_TRUNC_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_D_CEIL_W (void) {				// added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_D_FLOOR_W (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_D_CVT_S (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_D_CVT_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_D_CVT_L (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Double) || RegInStack(Section,m_CompileOpcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Section,m_CompileOpcode.fs,TRUE);
	}
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_D_CMP (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.ft:m_CompileOpcode.fs;
	DWORD Reg2 = RegInStack(Section,m_CompileOpcode.ft, CRegInfo::FPU_Float)?m_CompileOpcode.fs:m_CompileOpcode.ft;
	int x86reg, cmp = 0;

	
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	//if ((m_CompileOpcode.funct & 1) != 0) { CRecompilerOps::UnknownOpcode(Section); }
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
#endif
}

/************************** COP1: W functions ************************/
void CRecompilerOps::COP1_W_CVT_S (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Dword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_W_CVT_D (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Dword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Dword,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
#endif
}

/************************** COP1: L functions ************************/
void CRecompilerOps::COP1_L_CVT_S (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Qword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_L_CVT_D (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",Section->m_CompilePC,R4300iOpcodeName(m_CompileOpcode.Hex,Section->m_CompilePC));
	
	CompileCop1Test(Section);	
	if (m_CompileOpcode.fd != m_CompileOpcode.fs || !RegInStack(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword)) { 
		Load_FPR_ToTop(Section,m_CompileOpcode.fd,m_CompileOpcode.fs,CRegInfo::FPU_Qword); 
	}
	ChangeFPURegFormat(Section,m_CompileOpcode.fd,CRegInfo::FPU_Qword,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
#endif
}

/************************** Other functions **************************/
void CRecompilerOps::UnknownOpcode (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X Unhandled Opcode: %s",m_CompilePC, R4300iOpcodeName(Opcode.Hex,m_CompilePC));

//	FreeSection(m_Section->m_ContinueSection,m_Section);
//	FreeSection(m_Section->m_JumpSection,m_Section);
	m_RegWorkingSet.BlockCycleCount() -= CountPerOp;
	m_RegWorkingSet.BlockRandomModifier() -= 1;
	MoveConstToVariable(m_CompilePC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
	WriteBackRegisters(m_Section);
	UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
	if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
	MoveConstToVariable(Opcode.Hex,&Opcode.Hex,"Opcode.Hex");
	Call_Direct(R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
	Ret();
	if (m_NextInstruction == NORMAL) { m_NextInstruction = END_BLOCK; }
#endif
}

void CRecompilerOps::UpdateCounters ( CRegInfo & RegSet, bool CheckTimer, bool ClearValues )
{
	if (RegSet.GetBlockCycleCount() != 0)
	{
#ifdef tofix
		if (m_SyncSystem) {
			char text[100];

			WriteX86Comment("Updating Sync CPU");
			Pushad();
			sprintf(text,"%d",(DWORD)*Cycles);
			PushImm32(text,(DWORD)*Cycles);
			Call_Direct(UpdateSyncCPU,"UpdateSyncCPU");
			Popad();
		}
#endif
		WriteX86Comment("Update Counter");
		SubConstFromVariable(RegSet.GetBlockCycleCount(),_NextTimer,"_NextTimer"); // updates compare flag
		if (ClearValues)
		{
			RegSet.SetBlockCycleCount(0);
		}
	} else if (CheckTimer)	{
		CompConstToVariable(0,_NextTimer,"_NextTimer");
	}

	if (CheckTimer)
	{
		JnsLabel8("Continue_From_Timer_Test",0);
		BYTE * Jump = m_RecompPos - 1;
		Pushad();
		X86BreakPoint(__FILE__,__LINE__);
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(CSystemTimer::TimerDone),"CSystemTimer::TimerDone");
		Popad();
	
		CPU_Message("");
		CPU_Message("      $Continue_From_Timer_Test:");
		SetJump8(Jump,m_RecompPos);
	}
}

