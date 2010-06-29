#include "stdafx.h"

void InPermLoop         ( void );

int  DelaySlotEffectsCompare ( DWORD PC, DWORD Reg1, DWORD Reg2 );

int DelaySlotEffectsJump (DWORD JumpPC) {
	OPCODE Command;

	if (!_MMU->LW_VAddr(JumpPC, Command.Hex)) { return TRUE; }

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_JR:	return DelaySlotEffectsCompare(JumpPC,Command.rs,0);
		case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC,Command.rs,31);
		}
		break;
	case R4300i_REGIMM:
		switch (Command.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
			return DelaySlotEffectsCompare(JumpPC,Command.rs,0);
		}
		break;
	case R4300i_JAL: 
	case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC,31,0); break;
	case R4300i_J: return FALSE;
	case R4300i_BEQ: 
	case R4300i_BNE: 
	case R4300i_BLEZ: 
	case R4300i_BGTZ: 
		return DelaySlotEffectsCompare(JumpPC,Command.rs,Command.rt);
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_BC:
			switch (Command.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				{
					int EffectDelaySlot;
					OPCODE NewCommand;

					if (!_MMU->LW_VAddr(JumpPC + 4, NewCommand.Hex)) { return TRUE; }
					
					EffectDelaySlot = FALSE;
					if (NewCommand.op == R4300i_CP1) {
						if (NewCommand.fmt == R4300i_COP1_S && (NewCommand.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
						if (NewCommand.fmt == R4300i_COP1_D && (NewCommand.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
					}
					return EffectDelaySlot;
				} 
				break;
			}
			break;
		}
		break;
	case R4300i_BEQL: 
	case R4300i_BNEL: 
	case R4300i_BLEZL: 
	case R4300i_BGTZL: 
		return DelaySlotEffectsCompare(JumpPC,Command.rs,Command.rt);
	}
	return TRUE;
}

CCodeSection::CCodeSection( CCodeBlock * CodeBlock, DWORD EnterPC, DWORD ID) :
	m_BlockInfo(CodeBlock),
	m_EnterPC(EnterPC),
	m_SectionID(ID),
	m_ContinueSection(NULL),
	m_JumpSection(NULL),
	m_LinkAllowed(true),
	m_CompiledLocation(NULL),
	m_DelaySlotSection(CodeBlock? CodeBlock->bDelaySlot() : false),
	m_Test(0)
{
	/*
	Test2              = 0;
	InLoop             = false;

	*/
	if (&CodeBlock->EnterSection() == this)
	{
		m_LinkAllowed = false;
		m_RegEnter.Initilize();
	}
}

CCodeSection::~CCodeSection( void )
{
}

void CCodeSection::CompileExit ( DWORD JumpPC, DWORD TargetPC, CRegInfo ExitRegSet, CExitInfo::EXIT_REASON reason, int CompileNow, void (*x86Jmp)(const char * Label, DWORD Value))
{
	if (!CompileNow) 
	{
		char String[100];
		sprintf(String,"Exit_%d",m_BlockInfo->m_ExitInfo.size());
		if (x86Jmp == NULL) 
		{ 
			DisplayError("CompileExit error");
			ExitThread(0);
		}
		x86Jmp(String,0);

		CExitInfo ExitInfo;
		ExitInfo.ID = m_BlockInfo->m_ExitInfo.size();
		ExitInfo.TargetPC = TargetPC;
		ExitInfo.ExitRegSet = ExitRegSet;
		ExitInfo.reason = reason;
		ExitInfo.NextInstruction = m_NextInstruction;
		ExitInfo.JumpLoc = (DWORD *)(m_RecompPos - 4);
		m_BlockInfo->m_ExitInfo.push_back(ExitInfo);
		return;
	}

	//CPU_Message("CompileExit: %d",reason);
	ExitRegSet.WriteBackRegisters();

	if (TargetPC != (DWORD)-1) 
	{
		MoveConstToVariable(TargetPC,&_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER"); 
		UpdateCounters(ExitRegSet,TargetPC <= JumpPC, reason == CExitInfo::Normal);
	} else {
		UpdateCounters(ExitRegSet,false,reason == CExitInfo::Normal);
	}

	switch (reason) {
	case CExitInfo::Normal: case CExitInfo::Normal_NoSysCheck:
		ExitRegSet.SetBlockCycleCount(0);
		if (TargetPC != (DWORD)-1)
		{
			if (TargetPC <= JumpPC && reason == CExitInfo::Normal)
			{
				CompileSystemCheck((DWORD)-1,ExitRegSet);
			}
		} else {
			if (reason == CExitInfo::Normal) { CompileSystemCheck((DWORD)-1,ExitRegSet);	}
		}
		if (_SyncSystem)
		{
			Call_Direct(SyncSystem, "SyncSystem"); 
		}
	#ifdef LinkBlocks
		if (bSMM_ValidFunc == false)
		{
			if (LookUpMode() == FuncFind_ChangeMemory) 
			{
				BreakPoint(__FILE__,__LINE__);
	//			BYTE * Jump, * Jump2;
	//			if (TargetPC >= 0x80000000 && TargetPC < 0xC0000000) {
	//				DWORD pAddr = TargetPC & 0x1FFFFFFF;
	//	
	//				MoveVariableToX86reg((BYTE *)RDRAM + pAddr,"RDRAM + pAddr",x86_EAX);
	//				Jump2 = NULL;
	//			} else {				
	//				MoveConstToX86reg((TargetPC >> 12),x86_ECX);
	//				MoveConstToX86reg(TargetPC,x86_EBX);
	//				MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",x86_ECX,x86_ECX,4);
	//				TestX86RegToX86Reg(x86_ECX,x86_ECX);
	//				JeLabel8("NoTlbEntry",0);
	//				Jump2 = m_RecompPos - 1;
	//				MoveX86regPointerToX86reg(x86_ECX, x86_EBX,x86_EAX);
	//			}
	//			MoveX86RegToX86Reg(x86_EAX,x86_ECX);
	//			AndConstToX86Reg(x86_ECX,0xFFFF0000);
	//			CompConstToX86reg(x86_ECX,0x7C7C0000);
	//			JneLabel8("NoCode",0);
	//			Jump = m_RecompPos - 1;
	//			AndConstToX86Reg(x86_EAX,0xFFFF);
	//			ShiftLeftSignImmed(x86_EAX,4);
	//			AddConstToX86Reg(x86_EAX,0xC);
	//			MoveVariableDispToX86Reg(OrigMem,"OrigMem",x86_ECX,x86_EAX,1);
	//			JmpDirectReg(x86_ECX);
	//			CPU_Message("      NoCode:");
	//			*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
	//			if (Jump2 != NULL) {
	//				CPU_Message("      NoTlbEntry:");
	//				*((BYTE *)(Jump2))=(BYTE)(m_RecompPos - Jump2 - 1);
	//			}
			} 
			else if (LookUpMode() == FuncFind_VirtualLookup)
			{			
				MoveConstToX86reg(TargetPC,x86_EDX);
				MoveConstToX86reg((DWORD)&m_Functions,x86_ECX);		
				Call_Direct(AddressOf(CFunctionMap::CompilerFindFunction), "CFunctionMap::CompilerFindFunction");
				MoveX86RegToX86Reg(x86_EAX,x86_ECX);
				JecxzLabel8("NullPointer",0);
				BYTE * Jump = m_RecompPos - 1;
				MoveX86PointerToX86regDisp(x86_EBX,x86_ECX,0xC);				
				JmpDirectReg(x86_EBX);
				CPU_Message("      NullPointer:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			}
			else if (LookUpMode() == FuncFind_PhysicalLookup) 
			{
				BYTE * Jump2 = NULL;
				if (TargetPC >= 0x80000000 && TargetPC < 0x90000000) {
					DWORD pAddr = TargetPC & 0x1FFFFFFF;
					MoveVariableToX86reg((BYTE *)JumpTable + pAddr,"JumpTable + pAddr",x86_ECX);
				} else if (TargetPC >= 0x90000000 && TargetPC < 0xC0000000) {
				} else {				
					MoveConstToX86reg((TargetPC >> 12),x86_ECX);
					MoveConstToX86reg(TargetPC,x86_EBX);
					MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",x86_ECX,x86_ECX,4);
					TestX86RegToX86Reg(x86_ECX,x86_ECX);
					JeLabel8("NoTlbEntry",0);
					Jump2 = m_RecompPos - 1;
					AddConstToX86Reg(x86_ECX,(DWORD)JumpTable - (DWORD)RDRAM);
					MoveX86regPointerToX86reg(x86_ECX, x86_EBX,x86_ECX);
				}
				if (TargetPC < 0x90000000 || TargetPC >= 0xC0000000)
				{
					JecxzLabel8("NullPointer",0);
					BYTE * Jump = m_RecompPos - 1;
					MoveX86PointerToX86regDisp(x86_EAX,x86_ECX,0xC);				
					JmpDirectReg(x86_EAX);
					CPU_Message("      NullPointer:");
					*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
					if (Jump2 != NULL) {
						CPU_Message("      NoTlbEntry:");
						*((BYTE *)(Jump2))=(BYTE)(m_RecompPos - Jump2 - 1);
					}
				}
			}
		}
		ExitCodeBlock();
	#else
		ExitCodeBlock();
	#endif
		break;
	case CExitInfo::DoCPU_Action:
		MoveConstToX86reg((DWORD)_SystemEvents,x86_ECX);		
		Call_Direct(AddressOf(CSystemEvents::ExecuteEvents),"CSystemEvents::ExecuteEvents");
		if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
		ExitCodeBlock();
		break;
	case CExitInfo::DoSysCall:
		{
			bool bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
			PushImm32(bDelay ? "true" : "false", bDelay);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);		
			Call_Direct(AddressOf(CRegisters::DoSysCallException), "CRegisters::DoSysCallException");
			if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
			ExitCodeBlock();
		}
		break;
	case CExitInfo::COP1_Unuseable:
		{
			bool bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
			PushImm32("1",1);
			PushImm32(bDelay ? "true" : "false", bDelay);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);		
			Call_Direct(AddressOf(CRegisters::DoCopUnusableException), "CRegisters::DoCopUnusableException");
			if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
			ExitCodeBlock();
		}
		break;
	case CExitInfo::ExitResetRecompCode:
	_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix		
		if (NextInstruction == JUMP || NextInstruction == DELAY_SLOT) {
			X86BreakPoint(__FILE__,__LINE__);
		}
		if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
		X86BreakPoint(__FILE__,__LINE__);
		MoveVariableToX86reg(this,"this",x86_ECX);		
		Call_Direct(AddressOf(ResetRecompCode), "ResetRecompCode");
	#endif
		ExitCodeBlock();
		break;
	case CExitInfo::TLBReadMiss:
	_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix		
		MoveConstToX86reg(NextInstruction == JUMP || NextInstruction == DELAY_SLOT,x86_ECX);
		MoveVariableToX86reg(&TLBLoadAddress,"TLBLoadAddress",x86_EDX);
		Call_Direct(DoTLBMiss,"DoTLBMiss");
		if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
		Ret();
	#endif
		break;
	default:
		DisplayError("how did you want to exit on reason (%d) ???",reason);
	}
}

void CCodeSection::CompileSystemCheck (DWORD TargetPC, const CRegInfo &  RegSet)
{
	CompConstToVariable(0,(void *)&_SystemEvents->DoSomething(),"_SystemEvents->DoSomething()");
	JeLabel32("Continue_From_Interrupt_Test",0);
	DWORD * Jump = (DWORD *)(m_RecompPos - 4);
	if (TargetPC != (DWORD)-1) 
	{
		MoveConstToVariable(TargetPC,&_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER"); 
	}

	CCodeSection Section(NULL,-1,0);
	Section.m_RegWorkingSet = RegSet;
	Section.m_RegWorkingSet.WriteBackRegisters();
	CompileExit(-1, -1,Section.m_RegWorkingSet,CExitInfo::DoCPU_Action,true,NULL);
	CPU_Message("");
	CPU_Message("      $Continue_From_Interrupt_Test:");
	SetJump32(Jump,(DWORD *)m_RecompPos);	
}

void CCodeSection::GenerateSectionLinkage (void)
{
	CCodeSection * TargetSection[] = { m_ContinueSection, m_JumpSection };
	CJumpInfo * JumpInfo[] = { &m_Cont, &m_Jump };
	int count;

	for (count = 0; count < 2; count ++) 
	{
		if (JumpInfo[count]->LinkLocation == NULL && 
			JumpInfo[count]->FallThrough == false) 
		{
			JumpInfo[count]->TargetPC = -1;
		}
	}

	if ((CRecompilerOps::CompilePC() & 0xFFC) == 0xFFC) {
		//Handle Fall througth
		BYTE * Jump = NULL;
		for (count = 0; count < 2; count ++) {
			if (!JumpInfo[count]->FallThrough) { continue; }
			JumpInfo[count]->FallThrough = false;
			if (JumpInfo[count]->LinkLocation != NULL) {
				SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
				JumpInfo[count]->LinkLocation = NULL;
				if (JumpInfo[count]->LinkLocation2 != NULL) { 
					SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
					JumpInfo[count]->LinkLocation2 = NULL;
				}
			}
			PushImm32(stdstr_f("0x%08X",JumpInfo[count]->TargetPC).c_str(),JumpInfo[count]->TargetPC);
			if (JumpInfo[(count + 1) & 1]->LinkLocation == NULL) { break; }
			JmpLabel8("FinishBlock",0);
			Jump = m_RecompPos - 1;
		}		
		for (count = 0; count < 2; count ++) {
			if (JumpInfo[count]->LinkLocation == NULL) { continue; }
			JumpInfo[count]->FallThrough = false;
			if (JumpInfo[count]->LinkLocation != NULL) {
				SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
				JumpInfo[count]->LinkLocation = NULL;
				if (JumpInfo[count]->LinkLocation2 != NULL) { 
					SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
					JumpInfo[count]->LinkLocation2 = NULL;
				}
			}
			PushImm32(stdstr_f("0x%08X",JumpInfo[count]->TargetPC).c_str(),JumpInfo[count]->TargetPC);
			if (JumpInfo[(count + 1) & 1]->LinkLocation == NULL) { break; }
			JmpLabel8("FinishBlock",0);
			Jump = m_RecompPos - 1;
		}
		if (Jump != NULL) {
			CPU_Message("      $FinishBlock:");
			SetJump8(Jump,m_RecompPos);
		}
		//MoveConstToVariable(CRecompilerOps::CompilePC() + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
		m_RegWorkingSet.WriteBackRegisters();
		UpdateCounters(m_RegWorkingSet,false,true);
	//		WriteBackRegisters(Section);
	//		if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
	//	MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
		PushImm32(stdstr_f("0x%08X",CRecompilerOps::CompilePC() + 4).c_str(),CRecompilerOps::CompilePC() + 4);
		
		// check if there is an existing section

		MoveConstToX86reg((DWORD)_Recompiler,x86_ECX);		
		Call_Direct(AddressOf(CRecompiler::CompileDelaySlot), "CRecompiler::CompileDelaySlot");
		JmpDirectReg(x86_EAX);
		ExitCodeBlock();
		return;
	}
	if (!g_UseLinking) {  
		if (CRecompilerOps::m_CompilePC == m_Jump.TargetPC && (m_Cont.FallThrough == false)) {
			if (!DelaySlotEffectsJump(CRecompilerOps::CompilePC())) {
				MoveConstToVariable(CRecompilerOps::CompilePC(),_PROGRAM_COUNTER,"PROGRAM_COUNTER");
				m_RegWorkingSet.WriteBackRegisters(); 
				m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_CountPerOp) ;
				UpdateCounters(m_RegWorkingSet,false, true);
				Call_Direct(InPermLoop,"InPermLoop");
				m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_CountPerOp) ;
				UpdateCounters(m_RegWorkingSet,true,true);
				CompileSystemCheck(-1,m_RegWorkingSet);
			}
		}
	}
	if (TargetSection[0] != TargetSection[1] || TargetSection[0] == NULL) {
		for (count = 0; count < 2; count ++) {
			if (JumpInfo[count]->LinkLocation == NULL && JumpInfo[count]->FallThrough == false) {
				if (TargetSection[count])
				{
_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix		
					TargetSection[count]->UnlinkParent(Section,true, count == 0);
#endif
				}
			} else if (TargetSection[count] == NULL && JumpInfo[count]->FallThrough) {
				if (JumpInfo[count]->LinkLocation != NULL) {
					SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
					JumpInfo[count]->LinkLocation = NULL;
					if (JumpInfo[count]->LinkLocation2 != NULL) { 
						SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
						JumpInfo[count]->LinkLocation2 = NULL;
					}			
				}
				CompileExit (CRecompilerOps::CompilePC(), JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet,CExitInfo::Normal,true,NULL);
				JumpInfo[count]->FallThrough = false;
			} else if (TargetSection[count] != NULL && JumpInfo[count] != NULL) {
				if (!JumpInfo[count]->FallThrough) { continue; }
				if (JumpInfo[count]->TargetPC == TargetSection[count]->m_EnterPC) { continue; }
				if (JumpInfo[count]->LinkLocation != NULL) {
					SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
					JumpInfo[count]->LinkLocation = NULL;
					if (JumpInfo[count]->LinkLocation2 != NULL) { 
						SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
						JumpInfo[count]->LinkLocation2 = NULL;
					}			
				}
				CompileExit (CRecompilerOps::CompilePC(), JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet,CExitInfo::Normal,true,NULL);
				//FreeSection(TargetSection[count],Section);
			}
		}
	} else {
		if (m_Cont.LinkLocation == NULL && m_Cont.FallThrough == false) { m_ContinueSection = NULL; }
		if (m_Jump.LinkLocation == NULL && m_Jump.FallThrough == false) { m_JumpSection = NULL; }
		if (m_JumpSection == NULL &&  m_ContinueSection == NULL) {
			//FreeSection(TargetSection[0],Section);
		}
	}

	TargetSection[0] = m_ContinueSection;
	TargetSection[1] = m_JumpSection;	

	for (count = 0; count < 2; count ++) {
		if (TargetSection[count] == NULL) { continue; }
		if (!JumpInfo[count]->FallThrough) { continue; }
			
		if (TargetSection[count]->m_CompiledLocation != NULL) {
			char Label[100];
			sprintf(Label,"Section_%d",TargetSection[count]->m_SectionID);
			JumpInfo[count]->FallThrough = false;
			if (JumpInfo[count]->LinkLocation != NULL) {
				SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
				JumpInfo[count]->LinkLocation = NULL;
				if (JumpInfo[count]->LinkLocation2 != NULL) { 
					SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
					JumpInfo[count]->LinkLocation2 = NULL;
				}
			}
			if (JumpInfo[count]->TargetPC <= CRecompilerOps::CompilePC()) {
				if (JumpInfo[count]->PermLoop) {
	CPU_Message("PermLoop *** 1");
					_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
					MoveConstToVariable(JumpInfo[count]->TargetPC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
					JumpInfo[count]->RegSet.BlockCycleCount() -= g_CountPerOp;
					UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), false);
					Call_Direct(InPermLoop,"InPermLoop");
					JumpInfo[count]->RegSet.BlockCycleCount() += g_CountPerOp;
					UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), true);
					CompileSystemCheck(-1,JumpInfo[count]->RegSet);
	#endif
				} else {
					_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
					UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(), &JumpInfo[count]->RegSet.BlockRandomModifier(), true);
					CompileSystemCheck(JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet);
	#endif
				}
			} else {
					_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(), &JumpInfo[count]->RegSet.BlockRandomModifier(), false);
	#endif
			}
			_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
			JumpInfo[count]->RegSet.BlockCycleCount() = 0;
			m_RegWorkingSet = JumpInfo[count]->RegSet;
			SyncRegState(Section,&TargetSection[count]->RegStart);						
	#endif
			JmpLabel32(Label,0);
			SetJump32((DWORD *)m_RecompPos - 1,(DWORD *)(TargetSection[count]->m_CompiledLocation));
		}
	}
	//BlockCycleCount() = 0;
	//BlockRandomModifier() = 0;

	for (count = 0; count < 2; count ++) {
		if (TargetSection[count] == NULL) { continue; }
	_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
		if (TargetSection[count]->ParentSection.empty()) { continue; }
		for (SECTION_LIST::iterator iter = TargetSection[count]->ParentSection.begin(); iter != TargetSection[count]->ParentSection.end(); iter++)
		{
			CCodeSection * Parent = *iter;

			if (Parent->CompiledLocation != NULL) { continue; }
			if (JumpInfo[count]->PermLoop) {
				CPU_Message("PermLoop *** 2");
				MoveConstToVariable(JumpInfo[count]->TargetPC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
				JumpInfo[count]->RegSet.BlockCycleCount() -= g_CountPerOp;
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), false);
				Call_Direct(InPermLoop,"InPermLoop");
				JumpInfo[count]->RegSet.BlockCycleCount() += g_CountPerOp;
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), true);
				CompileSystemCheck(-1,JumpInfo[count]->RegSet);
			}
			if (JumpInfo[count]->FallThrough) { 
				JumpInfo[count]->FallThrough = false;
				JmpLabel32(JumpInfo[count]->BranchLabel,0);
				JumpInfo[count]->LinkLocation = m_RecompPos - 4;
			}
		}
	#endif
	}

	for (count = 0; count < 2; count ++) {
		if (JumpInfo[count]->FallThrough) { 
			if (JumpInfo[count]->TargetPC < CRecompilerOps::CompilePC()) {
				_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(),true);
	#endif
				CompileSystemCheck(JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet);
			}
		}
	}

	CPU_Message("====== End of Section %d ======",m_SectionID);

	for (count = 0; count < 2; count ++) {
		if (JumpInfo[count]->FallThrough) { 
			TargetSection[count]->GenerateX86Code(m_BlockInfo->NextTest()); 
		}
	}

	//CPU_Message("Section %d",m_SectionID);
	for (count = 0; count < 2; count ++) {
		if (JumpInfo[count]->LinkLocation == NULL) { continue; }
		if (TargetSection[count] == NULL) {
			CPU_Message("ExitBlock (from %d):",m_SectionID);
			SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
			JumpInfo[count]->LinkLocation = NULL;
			if (JumpInfo[count]->LinkLocation2 != NULL) { 
				SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
				JumpInfo[count]->LinkLocation2 = NULL;
			}			
			CompileExit (CRecompilerOps::CompilePC(),JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet,CExitInfo::Normal,true,NULL);
			continue;
		}
		if (JumpInfo[count]->TargetPC != TargetSection[count]->m_EnterPC) {
			DisplayError("I need to add more code in GenerateSectionLinkage cause this is going to cause an exception");
			BreakPoint(__FILE__,__LINE__); 
		}
		if (TargetSection[count]->m_CompiledLocation == NULL) {
			_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
			GenerateX86Code(*TargetSection[count]->m_BlockInfo,TargetSection[count],m_BlockInfo->m_Test + 1); 
	#endif
		} else {
			char Label[100];

			CPU_Message("Section_%d (from %d):",TargetSection[count]->m_SectionID,m_SectionID);
			SetJump32(JumpInfo[count]->LinkLocation,(DWORD *)m_RecompPos);
			JumpInfo[count]->LinkLocation = NULL;
			if (JumpInfo[count]->LinkLocation2 != NULL) { 
				SetJump32(JumpInfo[count]->LinkLocation2,(DWORD *)m_RecompPos);
				JumpInfo[count]->LinkLocation2 = NULL;
			}			
			m_RegWorkingSet = JumpInfo[count]->RegSet;
			if (JumpInfo[count]->TargetPC <= CRecompilerOps::CompilePC()) {
				_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), true);
	#endif

				if (JumpInfo[count]->PermLoop) {
	CPU_Message("PermLoop *** 3");
					MoveConstToVariable(JumpInfo[count]->TargetPC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
					Call_Direct(InPermLoop,"InPermLoop");
					CompileSystemCheck(-1,JumpInfo[count]->RegSet);
				} else {
					CompileSystemCheck(JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet);
				}
			} else{
				_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), false);
	#endif
			}
			m_RegWorkingSet = JumpInfo[count]->RegSet;
			_Notify->BreakPoint(__FILE__,__LINE__);
	#ifdef tofix			
			SyncRegState(Section,&TargetSection[count]->RegStart);
	#endif
			JmpLabel32(Label,0);
			SetJump32((DWORD *)m_RecompPos - 1,(DWORD *)(TargetSection[count]->m_CompiledLocation));
		}
	}
}

void CCodeSection::CompileCop1Test (void) {
	if (m_RegWorkingSet.FpuBeenUsed()) { return; }
	TestVariable(STATUS_CU1,&_Reg->STATUS_REGISTER,"STATUS_REGISTER");
	CompileExit(m_CompilePC,m_CompilePC,m_RegWorkingSet,CExitInfo::COP1_Unuseable,FALSE,JeLabel32);
	m_RegWorkingSet.FpuBeenUsed() = TRUE;
}

bool CCodeSection::GenerateX86Code ( DWORD Test )
{
	if (this == NULL) { return false; }

	if (m_CompiledLocation != NULL) { 		
		if (m_Test == Test) 
		{
			return false; 
		}
		m_Test = Test;
		if (m_ContinueSection->GenerateX86Code(Test)) { return true; }
		if (m_JumpSection->GenerateX86Code(Test)) { return true; }
		return false; 
	}
#ifdef tofix
	if (ParentSection.size() > 0)
	{
		for (SECTION_LIST::iterator iter = ParentSection.begin(); iter != ParentSection.end(); iter++)
		{
			CBlockSection * Parent = *iter;
			if (Parent->CompiledLocation != NULL) { continue; }
			if (IsAllParentLoops(Parent,true,CBlockSection::GetNewTestValue())) { continue; }
			return false;
		}
	}
	if (!InheritParentInfo(Section)) { return false; }
#endif
	m_RegWorkingSet    = m_RegEnter;
	m_CompiledLocation = m_RecompPos;
	m_CompilePC        = m_EnterPC;
	m_NextInstruction  = m_DelaySlotSection ? END_BLOCK : NORMAL;	
	m_RegWorkingSet    = m_RegEnter;
	m_Section          = this;

	if (m_CompilePC < m_BlockInfo->VAddrFirst())
	{
		m_BlockInfo->SetVAddrFirst(m_CompilePC);
	}

	do {
		__try {
			if (!_MMU->LW_VAddr(m_CompilePC,m_Opcode.Hex))
			{
				DisplayError(GS(MSG_FAIL_LOAD_WORD));
				ExitThread(0);
			}
		} __except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
			DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
			ExitThread(0);
		}

		if (m_CompilePC > m_BlockInfo->VAddrLast())
		{
			m_BlockInfo->SetVAddrLast(m_CompilePC);
		}
		/*if (m_CompilePC == 0xF000044)
		{
			X86BreakPoint(__FILE__,__LINE__);
			//m_RegWorkingSet.UnMap_AllFPRs();
		}*/
		
		if (m_CompilePC >= 0x0F000000 && m_CompilePC <= 0x0F000048 && m_NextInstruction == NORMAL)
		{
			m_RegWorkingSet.WriteBackRegisters();
			UpdateCounters(m_RegWorkingSet,false,true);
			MoveConstToVariable(m_CompilePC,&_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			if (_SyncSystem) { Call_Direct(SyncToPC, "SyncToPC"); }
		}

		/*if (m_CompilePC == 0x803254F0 && m_NextInstruction == NORMAL)
		{
			m_RegWorkingSet.WriteBackRegisters();
			UpdateCounters(m_RegWorkingSet,false,true);
			MoveConstToVariable(m_CompilePC,&_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			if (_SyncSystem) { Call_Direct(SyncToPC, "SyncToPC"); }
		}*/
		/*if (m_CompilePC == 0x80000EC4 && m_NextInstruction == NORMAL)
		{
			//m_RegWorkingSet.UnMap_AllFPRs();
			_Notify->BreakPoint(__FILE__,__LINE__);
			//X86HardBreakPoint();
			//X86BreakPoint(__FILE__,__LINE__);
			//m_RegWorkingSet.UnMap_AllFPRs();
		}
		/*if (m_CompilePC >= 0x80179DC4 && m_CompilePC <= 0x80179DF0 && m_NextInstruction == NORMAL)
		{
			m_RegWorkingSet.UnMap_AllFPRs();
		}*/

		m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_CountPerOp);
		m_RegWorkingSet.ResetX86Protection();

		switch (m_Opcode.op) {
		case R4300i_SPECIAL:
			switch (m_Opcode.funct) {
			case R4300i_SPECIAL_SLL: SPECIAL_SLL(); break;
			case R4300i_SPECIAL_SRL: SPECIAL_SRL(); break;
			case R4300i_SPECIAL_SRA: SPECIAL_SRA(); break;
			case R4300i_SPECIAL_SLLV: SPECIAL_SLLV(); break;
			case R4300i_SPECIAL_SRLV: SPECIAL_SRLV(); break;
			case R4300i_SPECIAL_SRAV: SPECIAL_SRAV(); break;
			case R4300i_SPECIAL_JR: SPECIAL_JR(); break;
			case R4300i_SPECIAL_JALR: SPECIAL_JALR(); break;
			case R4300i_SPECIAL_MFLO: SPECIAL_MFLO(); break;
			case R4300i_SPECIAL_SYSCALL: SPECIAL_SYSCALL(); break;
			case R4300i_SPECIAL_MTLO: SPECIAL_MTLO(); break;
			case R4300i_SPECIAL_MFHI: SPECIAL_MFHI(); break;
			case R4300i_SPECIAL_MTHI: SPECIAL_MTHI(); break;
			case R4300i_SPECIAL_DSLLV: SPECIAL_DSLLV(); break;
			case R4300i_SPECIAL_DSRLV: SPECIAL_DSRLV(); break;
			case R4300i_SPECIAL_DSRAV: SPECIAL_DSRAV(); break;
			case R4300i_SPECIAL_MULT: SPECIAL_MULT(); break;
			case R4300i_SPECIAL_DIV: SPECIAL_DIV(); break;
			case R4300i_SPECIAL_DIVU: SPECIAL_DIVU(); break;
			case R4300i_SPECIAL_MULTU: SPECIAL_MULTU(); break;
			case R4300i_SPECIAL_DMULT: SPECIAL_DMULT(); break;
			case R4300i_SPECIAL_DMULTU: SPECIAL_DMULTU(); break;
			case R4300i_SPECIAL_DDIV: SPECIAL_DDIV(); break;
			case R4300i_SPECIAL_DDIVU: SPECIAL_DDIVU(); break;
			case R4300i_SPECIAL_ADD: SPECIAL_ADD(); break;
			case R4300i_SPECIAL_ADDU: SPECIAL_ADDU(); break;
			case R4300i_SPECIAL_SUB: SPECIAL_SUB(); break;
			case R4300i_SPECIAL_SUBU: SPECIAL_SUBU(); break;
			case R4300i_SPECIAL_AND: SPECIAL_AND(); break;
			case R4300i_SPECIAL_OR: SPECIAL_OR(); break;
			case R4300i_SPECIAL_XOR: SPECIAL_XOR(); break;
			case R4300i_SPECIAL_NOR: SPECIAL_NOR(); break;
			case R4300i_SPECIAL_SLT: SPECIAL_SLT(); break;
			case R4300i_SPECIAL_SLTU: SPECIAL_SLTU(); break;
			case R4300i_SPECIAL_DADD: SPECIAL_DADD(); break;
			case R4300i_SPECIAL_DADDU: SPECIAL_DADDU(); break;
			case R4300i_SPECIAL_DSUB: SPECIAL_DSUB(); break;
			case R4300i_SPECIAL_DSUBU: SPECIAL_DSUBU(); break;
			case R4300i_SPECIAL_DSLL: SPECIAL_DSLL(); break;
			case R4300i_SPECIAL_DSRL: SPECIAL_DSRL(); break;
			case R4300i_SPECIAL_DSRA: SPECIAL_DSRA(); break;
			case R4300i_SPECIAL_DSLL32: SPECIAL_DSLL32(); break;
			case R4300i_SPECIAL_DSRL32: SPECIAL_DSRL32(); break;
			case R4300i_SPECIAL_DSRA32: SPECIAL_DSRA32(); break;
			default:
				UnknownOpcode(); break;
			}
			break;
		case R4300i_REGIMM: 
			switch (m_Opcode.rt) {
			case R4300i_REGIMM_BLTZ:Compile_Branch(BLTZ_Compare,BranchTypeRs, false); break;
			case R4300i_REGIMM_BGEZ:Compile_Branch(BGEZ_Compare,BranchTypeRs, false); break;
			case R4300i_REGIMM_BLTZL:Compile_BranchLikely(BLTZ_Compare, false); break;
			case R4300i_REGIMM_BGEZL:Compile_BranchLikely(BGEZ_Compare, false); break;
			case R4300i_REGIMM_BLTZAL:Compile_Branch(BLTZ_Compare,BranchTypeRs, true); break;
			case R4300i_REGIMM_BGEZAL:Compile_Branch(BGEZ_Compare,BranchTypeRs, true); break;
			default:
				UnknownOpcode(); break;
			}
			break;
		case R4300i_BEQ: Compile_Branch(BEQ_Compare,BranchTypeRsRt,false); break;
		case R4300i_BNE: Compile_Branch(BNE_Compare,BranchTypeRsRt,false); break;
		case R4300i_BGTZ:Compile_Branch(BGTZ_Compare,BranchTypeRs,false); break;
		case R4300i_BLEZ:Compile_Branch(BLEZ_Compare,BranchTypeRs,false); break;
		case R4300i_J: J(); break;
		case R4300i_JAL: JAL(); break;
		case R4300i_ADDI: ADDI(); break;
		case R4300i_ADDIU: ADDIU(); break;
		case R4300i_SLTI: SLTI(); break;
		case R4300i_SLTIU: SLTIU(); break;
		case R4300i_ANDI: ANDI(); break;
		case R4300i_ORI: ORI(); break;
		case R4300i_XORI: XORI(); break;
		case R4300i_LUI: LUI(); break;
		case R4300i_CP0:
			switch (m_Opcode.rs) {
			case R4300i_COP0_MF: COP0_MF(); break;
			case R4300i_COP0_MT: COP0_MT(); break;
			default:
				if ( (m_Opcode.rs & 0x10 ) != 0 ) {
					switch( m_Opcode.funct ) {
					case R4300i_COP0_CO_TLBR: COP0_CO_TLBR(); break;
					case R4300i_COP0_CO_TLBWI: COP0_CO_TLBWI(); break;
					case R4300i_COP0_CO_TLBWR: COP0_CO_TLBWR(); break;
					case R4300i_COP0_CO_TLBP: COP0_CO_TLBP(); break;
					case R4300i_COP0_CO_ERET: COP0_CO_ERET(); break;
					default: UnknownOpcode(); break;
					}
				} else {
					UnknownOpcode();
				}
			}
			break;
		case R4300i_CP1:
			switch (m_Opcode.rs) {
			case R4300i_COP1_MF: COP1_MF(); break;
			case R4300i_COP1_DMF: COP1_DMF(); break;
			case R4300i_COP1_CF: COP1_CF(); break;
			case R4300i_COP1_MT: COP1_MT(); break;
			case R4300i_COP1_DMT: COP1_DMT(); break;
			case R4300i_COP1_CT: COP1_CT(); break;
			case R4300i_COP1_BC:
				switch (m_Opcode.ft) {
				case R4300i_COP1_BC_BCF: Compile_Branch(COP1_BCF_Compare,BranchTypeCop1,false); break;
				case R4300i_COP1_BC_BCT: Compile_Branch(COP1_BCT_Compare,BranchTypeCop1,false); break;
				case R4300i_COP1_BC_BCFL: Compile_BranchLikely(COP1_BCF_Compare,false); break;
				case R4300i_COP1_BC_BCTL: Compile_BranchLikely(COP1_BCT_Compare,false); break;
				default:
					UnknownOpcode(); break;
				}
				break;
			case R4300i_COP1_S: 
				switch (m_Opcode.funct) {
				case R4300i_COP1_FUNCT_ADD: COP1_S_ADD(); break;
				case R4300i_COP1_FUNCT_SUB: COP1_S_SUB(); break;
				case R4300i_COP1_FUNCT_MUL: COP1_S_MUL(); break;
				case R4300i_COP1_FUNCT_DIV: COP1_S_DIV(); break;
				case R4300i_COP1_FUNCT_ABS: COP1_S_ABS(); break;
				case R4300i_COP1_FUNCT_NEG: COP1_S_NEG(); break;
				case R4300i_COP1_FUNCT_SQRT: COP1_S_SQRT(); break;
				case R4300i_COP1_FUNCT_MOV: COP1_S_MOV(); break;
				case R4300i_COP1_FUNCT_TRUNC_L: COP1_S_TRUNC_L(); break;
				case R4300i_COP1_FUNCT_CEIL_L: COP1_S_CEIL_L(); break;	//added by Witten
				case R4300i_COP1_FUNCT_FLOOR_L: COP1_S_FLOOR_L(); break;	//added by Witten
				case R4300i_COP1_FUNCT_ROUND_W: COP1_S_ROUND_W(); break;
				case R4300i_COP1_FUNCT_TRUNC_W: COP1_S_TRUNC_W(); break;
				case R4300i_COP1_FUNCT_CEIL_W: COP1_S_CEIL_W(); break;	//added by Witten
				case R4300i_COP1_FUNCT_FLOOR_W: COP1_S_FLOOR_W(); break;
				case R4300i_COP1_FUNCT_CVT_D: COP1_S_CVT_D(); break;
				case R4300i_COP1_FUNCT_CVT_W: COP1_S_CVT_W(); break;
				case R4300i_COP1_FUNCT_CVT_L: COP1_S_CVT_L(); break;
				case R4300i_COP1_FUNCT_C_F:   case R4300i_COP1_FUNCT_C_UN:
				case R4300i_COP1_FUNCT_C_EQ:  case R4300i_COP1_FUNCT_C_UEQ:
				case R4300i_COP1_FUNCT_C_OLT: case R4300i_COP1_FUNCT_C_ULT:
				case R4300i_COP1_FUNCT_C_OLE: case R4300i_COP1_FUNCT_C_ULE:
				case R4300i_COP1_FUNCT_C_SF:  case R4300i_COP1_FUNCT_C_NGLE:
				case R4300i_COP1_FUNCT_C_SEQ: case R4300i_COP1_FUNCT_C_NGL:
				case R4300i_COP1_FUNCT_C_LT:  case R4300i_COP1_FUNCT_C_NGE:
				case R4300i_COP1_FUNCT_C_LE:  case R4300i_COP1_FUNCT_C_NGT:
					COP1_S_CMP(); break;
				default:
					UnknownOpcode(); break;
				}
				break;
			case R4300i_COP1_D: 
				switch (m_Opcode.funct) {
				case R4300i_COP1_FUNCT_ADD: COP1_D_ADD(); break;
				case R4300i_COP1_FUNCT_SUB: COP1_D_SUB(); break;
				case R4300i_COP1_FUNCT_MUL: COP1_D_MUL(); break;
				case R4300i_COP1_FUNCT_DIV: COP1_D_DIV(); break;
				case R4300i_COP1_FUNCT_ABS: COP1_D_ABS(); break;
				case R4300i_COP1_FUNCT_NEG: COP1_D_NEG(); break;
				case R4300i_COP1_FUNCT_SQRT: COP1_D_SQRT(); break;
				case R4300i_COP1_FUNCT_MOV: COP1_D_MOV(); break;
				case R4300i_COP1_FUNCT_TRUNC_L: COP1_D_TRUNC_L(); break;	//added by Witten
				case R4300i_COP1_FUNCT_CEIL_L: COP1_D_CEIL_L(); break;	//added by Witten
				case R4300i_COP1_FUNCT_FLOOR_L: COP1_D_FLOOR_L(); break;	//added by Witten
				case R4300i_COP1_FUNCT_ROUND_W: COP1_D_ROUND_W(); break;
				case R4300i_COP1_FUNCT_TRUNC_W: COP1_D_TRUNC_W(); break;
				case R4300i_COP1_FUNCT_CEIL_W: COP1_D_CEIL_W(); break;	//added by Witten
				case R4300i_COP1_FUNCT_FLOOR_W: COP1_D_FLOOR_W(); break;	//added by Witten
				case R4300i_COP1_FUNCT_CVT_S: COP1_D_CVT_S(); break;
				case R4300i_COP1_FUNCT_CVT_W: COP1_D_CVT_W(); break;
				case R4300i_COP1_FUNCT_CVT_L: COP1_D_CVT_L(); break;
				case R4300i_COP1_FUNCT_C_F:   case R4300i_COP1_FUNCT_C_UN:
				case R4300i_COP1_FUNCT_C_EQ:  case R4300i_COP1_FUNCT_C_UEQ:
				case R4300i_COP1_FUNCT_C_OLT: case R4300i_COP1_FUNCT_C_ULT:
				case R4300i_COP1_FUNCT_C_OLE: case R4300i_COP1_FUNCT_C_ULE:
				case R4300i_COP1_FUNCT_C_SF:  case R4300i_COP1_FUNCT_C_NGLE:
				case R4300i_COP1_FUNCT_C_SEQ: case R4300i_COP1_FUNCT_C_NGL:
				case R4300i_COP1_FUNCT_C_LT:  case R4300i_COP1_FUNCT_C_NGE:
				case R4300i_COP1_FUNCT_C_LE:  case R4300i_COP1_FUNCT_C_NGT:
					COP1_D_CMP(); break;
				default:
					UnknownOpcode(); break;
				}
				break;
			case R4300i_COP1_W: 
				switch (m_Opcode.funct) {
				case R4300i_COP1_FUNCT_CVT_S: COP1_W_CVT_S(); break;
				case R4300i_COP1_FUNCT_CVT_D: COP1_W_CVT_D(); break;
				default:
					UnknownOpcode(); break;
				}
				break;
			case R4300i_COP1_L: 
				switch (m_Opcode.funct) {
				case R4300i_COP1_FUNCT_CVT_S: COP1_L_CVT_S(); break;
				case R4300i_COP1_FUNCT_CVT_D: COP1_L_CVT_D(); break;
				default:
					UnknownOpcode(); break;
				}
				break;
			default:
				UnknownOpcode(); break;
			}
			break;
		case R4300i_BEQL: Compile_BranchLikely(BEQ_Compare,false); break;
		case R4300i_BNEL: Compile_BranchLikely(BNE_Compare,false); break;
		case R4300i_BGTZL:Compile_BranchLikely(BGTZ_Compare,false); break;
		case R4300i_BLEZL:Compile_BranchLikely(BLEZ_Compare,false); break;
		case R4300i_DADDIU: DADDIU(); break;
		case R4300i_LDL: _MMU->Compile_LDL(); break;
		case R4300i_LDR: _MMU->Compile_LDR(); break;
		case R4300i_LB: _MMU->Compile_LB(); break;
		case R4300i_LH: _MMU->Compile_LH(); break;
		case R4300i_LWL: _MMU->Compile_LWL(); break;
		case R4300i_LW: _MMU->Compile_LW(); break;
		case R4300i_LBU: _MMU->Compile_LBU(); break;
		case R4300i_LHU: _MMU->Compile_LHU(); break;
		case R4300i_LWR: _MMU->Compile_LWR(); break;
		case R4300i_LWU: _MMU->Compile_LWU(); break;	//added by Witten
		case R4300i_SB: _MMU->Compile_SB(); break;
		case R4300i_SH: _MMU->Compile_SH(); break;
		case R4300i_SWL: _MMU->Compile_SWL(); break;
		case R4300i_SW: _MMU->Compile_SW(); break;
		case R4300i_SWR: _MMU->Compile_SWR(); break;
		case R4300i_SDL: _MMU->Compile_SDL(); break;
		case R4300i_SDR: _MMU->Compile_SDR(); break;
		case R4300i_CACHE: CACHE(); break;
		case R4300i_LL: LL(); break;
		case R4300i_LWC1: _MMU->Compile_LWC1(); break;
		case R4300i_LDC1: _MMU->Compile_LDC1(); break;
		case R4300i_SC: SC(); break;
		case R4300i_LD: _MMU->Compile_LD(); break;
		case R4300i_SWC1: _MMU->Compile_SWC1(); break;
		case R4300i_SDC1: _MMU->Compile_SDC1(); break;
		case R4300i_SD: _MMU->Compile_SD(); break;
		default:
			UnknownOpcode(); break;
		}
		
	#ifdef tofix
		if (!bRegCaching()) { WriteBackRegisters(); }
	#endif
		m_RegWorkingSet.UnMap_AllFPRs();

		if ((m_CompilePC &0xFFC) == 0xFFC) 
		{
			if (m_NextInstruction == DO_DELAY_SLOT) 
			{
				DisplayError("Wanting to do delay slot over end of block");
			}
			if (m_NextInstruction == NORMAL) {
				CompileExit (m_CompilePC, m_CompilePC + 4,m_RegWorkingSet,CExitInfo::Normal,true,NULL);
				m_NextInstruction = END_BLOCK;
			}
		}

		switch (m_NextInstruction) {
		case NORMAL: 
			m_CompilePC += 4; 
			break;
		case DO_DELAY_SLOT:
			m_NextInstruction = DELAY_SLOT;
			m_CompilePC += 4; 
			break;
		case DELAY_SLOT:
			m_NextInstruction = DELAY_SLOT_DONE;
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_CountPerOp);
			m_CompilePC -= 4; 
			break;
		}
	} while (m_NextInstruction != END_BLOCK);

	if (m_DelaySlotSection)
	{
		CompileExit (m_CompilePC, -1,m_RegWorkingSet,CExitInfo::Normal,true,NULL);
	}
	return true;
}
