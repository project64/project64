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
	m_Test(0),
	m_Test2(0),
	m_InLoop(false)
{
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
		if (m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT) {
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
		MoveConstToX86reg(m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT,x86_ECX);
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

	if ((CompilePC() & 0xFFC) == 0xFFC) {
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
		//MoveConstToVariable(CompilePC() + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
		m_RegWorkingSet.WriteBackRegisters();
		UpdateCounters(m_RegWorkingSet,false,true);
	//		WriteBackRegisters(Section);
	//		if (_SyncSystem) { Call_Direct(SyncSystem, "SyncSystem"); }
	//	MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
		PushImm32(stdstr_f("0x%08X",CompilePC() + 4).c_str(),CompilePC() + 4);
		
		// check if there is an existing section

		MoveConstToX86reg((DWORD)_Recompiler,x86_ECX);		
		Call_Direct(AddressOf(CRecompiler::CompileDelaySlot), "CRecompiler::CompileDelaySlot");
		JmpDirectReg(x86_EAX);
		ExitCodeBlock();
		return;
	}
	if (!g_UseLinking) {  
		if (CRecompilerOps::m_CompilePC == m_Jump.TargetPC && (m_Cont.FallThrough == false)) {
			if (!DelaySlotEffectsJump(CompilePC())) {
				MoveConstToVariable(CompilePC(),_PROGRAM_COUNTER,"PROGRAM_COUNTER");
				m_RegWorkingSet.WriteBackRegisters(); 
				UpdateCounters(m_RegWorkingSet,false, true);
				Call_Direct(InPermLoop,"InPermLoop");
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
				CompileExit (CompilePC(), JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet,CExitInfo::Normal,true,NULL);
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
				CompileExit (CompilePC(), JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet,CExitInfo::Normal,true,NULL);
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
			if (JumpInfo[count]->TargetPC <= CompilePC()) {
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
					UpdateCounters(JumpInfo[count]->RegSet,true,true);
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
	#endif
			SyncRegState(TargetSection[count]->m_RegEnter);						
			JmpLabel32(Label,0);
			SetJump32((DWORD *)m_RecompPos - 1,(DWORD *)(TargetSection[count]->m_CompiledLocation));
		}
	}
	//BlockCycleCount() = 0;
	//BlockRandomModifier() = 0;

	for (count = 0; count < 2; count ++) {
		if (TargetSection[count] == NULL) { continue; }
		if (TargetSection[count]->m_ParentSection.empty()) { continue; }
		for (SECTION_LIST::iterator iter = TargetSection[count]->m_ParentSection.begin(); iter != TargetSection[count]->m_ParentSection.end(); iter++)
		{
			CCodeSection * Parent = *iter;

			if (Parent->m_CompiledLocation != NULL) { continue; }
			if (JumpInfo[count]->PermLoop) 
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
				CPU_Message("PermLoop *** 2");
				MoveConstToVariable(JumpInfo[count]->TargetPC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
				JumpInfo[count]->RegSet.BlockCycleCount() -= g_CountPerOp;
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), false);
				Call_Direct(InPermLoop,"InPermLoop");
				JumpInfo[count]->RegSet.BlockCycleCount() += g_CountPerOp;
				UpdateCounters(&JumpInfo[count]->RegSet.BlockCycleCount(),&JumpInfo[count]->RegSet.BlockRandomModifier(), true);
				CompileSystemCheck(-1,JumpInfo[count]->RegSet);
#endif
			}
			if (JumpInfo[count]->FallThrough) { 
				JumpInfo[count]->FallThrough = false;
				JmpLabel32(JumpInfo[count]->BranchLabel.c_str(),0);
				JumpInfo[count]->LinkLocation = (DWORD*)(m_RecompPos - 4);
			}
		}
	}

	for (count = 0; count < 2; count ++) {
		if (JumpInfo[count]->FallThrough) { 
			if (JumpInfo[count]->TargetPC < CompilePC()) {
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
			CompileExit (CompilePC(),JumpInfo[count]->TargetPC,JumpInfo[count]->RegSet,CExitInfo::Normal,true,NULL);
			continue;
		}
		if (JumpInfo[count]->TargetPC != TargetSection[count]->m_EnterPC) {
			DisplayError("I need to add more code in GenerateSectionLinkage cause this is going to cause an exception");
			BreakPoint(__FILE__,__LINE__); 
		}
		if (TargetSection[count]->m_CompiledLocation == NULL) 
		{
			TargetSection[count]->GenerateX86Code(m_BlockInfo->NextTest());
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
			if (JumpInfo[count]->TargetPC <= CompilePC()) {
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
			SyncRegState(TargetSection[count]->m_RegEnter);						
			JmpLabel32(Label,0);
			SetJump32((DWORD *)m_RecompPos - 1,(DWORD *)(TargetSection[count]->m_CompiledLocation));
		}
	}
}

void CCodeSection::SyncRegState ( const CRegInfo & SyncTo ) 
{
	x86Reg Reg, x86RegHi;
	int count;
	
	bool changed = false;
	UnMap_AllFPRs();
	if (m_RegWorkingSet.GetRoundingModel() != SyncTo.GetRoundingModel()) { m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown); }
	Reg = Map_MemoryStack(x86_Any, false);
	//CPU_Message("MemoryStack for Original State = %s",x86Reg > 0?x86_Name(x86Reg):"Not Mapped");

	for (Reg = (x86Reg)0; Reg < 10; Reg = (x86Reg)((int)Reg + 1))
	{
		if (m_RegWorkingSet.GetX86Mapped(Reg) != CRegInfo::Stack_Mapped) { continue; }
		if (SyncTo.GetX86Mapped(Reg) != CRegInfo::Stack_Mapped) {
			UnMap_X86reg(Reg);
			for (count = 0; count < 10; count ++) {
				if (SyncTo.GetX86Mapped((x86Reg)count) == CRegInfo::Stack_Mapped) {
					MoveX86RegToX86Reg((x86Reg)count,Reg); 
					changed = true;
				}
			}
			if (!changed) 
			{
				MoveVariableToX86reg(&_Recompiler->MemoryStackPos(),"MemoryStack",Reg);
			}
			changed = true;
		}
	}
	for (Reg = (x86Reg)0; Reg < 10; Reg = (x86Reg)((int)Reg + 1))
	{
		if (SyncTo.GetX86Mapped((x86Reg)Reg) != CRegInfo::Stack_Mapped) { continue; }
		//CPU_Message("MemoryStack for Sync State = %s",x86Reg > 0?x86_Name(x86Reg):"Not Mapped");
		if (m_RegWorkingSet.GetX86Mapped((x86Reg)Reg) == CRegInfo::Stack_Mapped) { break; }
		UnMap_X86reg((x86Reg)Reg);		
	}
	
	for (count = 1; count < 32; count ++) {
		if (cMipsRegState(count) == SyncTo.cMipsRegState(count)) {
			switch (cMipsRegState(count)) {
			case CRegInfo::STATE_UNKNOWN: continue;
			case CRegInfo::STATE_MAPPED_64:
				if (MipsReg(count) == SyncTo.cMipsReg(count)) {
					continue;
				}
				break;
			case CRegInfo::STATE_MAPPED_32_ZERO:
			case CRegInfo::STATE_MAPPED_32_SIGN:
				if (MipsRegLo(count) == SyncTo.cMipsRegLo(count)) {
					continue;
				}
				break;
			case CRegInfo::STATE_CONST_64:
				if (MipsReg(count) != SyncTo.cMipsReg(count)) {
#if (!defined(EXTERNAL_RELEASE))
					DisplayError("Umm.. how ???");
#endif
				}
				continue;
			case CRegInfo::STATE_CONST_32:
				if (MipsRegLo(count) != SyncTo.cMipsRegLo(count)) {
#if (!defined(EXTERNAL_RELEASE))
					DisplayError("Umm.. how ???");
#endif
				}
				continue;
#ifndef EXTERNAL_RELEASE
			default:
				DisplayError("Unhandled Reg state %d\nin SyncRegState",MipsRegState(count));
#endif
			}			
		}
		changed = true;

		switch (SyncTo.cMipsRegState(count)) {
		case CRegInfo::STATE_UNKNOWN: UnMap_GPR(count,true);  break;
		case CRegInfo::STATE_MAPPED_64:
			Reg = SyncTo.cMipsRegMapLo(count);
			x86RegHi = SyncTo.cMipsRegMapHi(count);
			UnMap_X86reg(Reg);
			UnMap_X86reg(x86RegHi);
			switch (MipsRegState(count)) {
			case CRegInfo::STATE_UNKNOWN:
				MoveVariableToX86reg(&_GPR[count].UW[0],CRegName::GPR_Lo[count],Reg);
				MoveVariableToX86reg(&_GPR[count].UW[1],CRegName::GPR_Hi[count],x86RegHi);
				break;
			case CRegInfo::STATE_MAPPED_64:
				MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count),CRegInfo::NotMapped);
				MoveX86RegToX86Reg(MipsRegMapHi(count),x86RegHi); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapHi(count),CRegInfo::NotMapped);
				break;
			case CRegInfo::STATE_MAPPED_32_SIGN:
				MoveX86RegToX86Reg(MipsRegMapLo(count),x86RegHi); 
				ShiftRightSignImmed(x86RegHi,31);
				MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count),CRegInfo::NotMapped);
				break;
			case CRegInfo::STATE_MAPPED_32_ZERO:
				XorX86RegToX86Reg(x86RegHi,x86RegHi);
				MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count), CRegInfo::NotMapped);
				break;
			case CRegInfo::STATE_CONST_64:
				MoveConstToX86reg(MipsRegHi(count),x86RegHi); 
				MoveConstToX86reg(MipsRegLo(count),Reg); 
				break;
			case CRegInfo::STATE_CONST_32:
				MoveConstToX86reg(MipsRegLo_S(count) >> 31,x86RegHi); 
				MoveConstToX86reg(MipsRegLo(count),Reg); 
				break;
			default:
#ifndef EXTERNAL_RELEASE
				CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d",MipsRegState(count));
				DisplayError("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d",MipsRegState(count));
#endif
				continue;
			}
			MipsRegLo(count) = Reg;
			MipsRegHi(count) = x86RegHi;
			MipsRegState(count) = CRegInfo::STATE_MAPPED_64;
			m_RegWorkingSet.SetX86Mapped(Reg,CRegInfo::GPR_Mapped);
			m_RegWorkingSet.SetX86Mapped(x86RegHi,CRegInfo::GPR_Mapped);
			m_RegWorkingSet.SetX86MapOrder(Reg,1);
			m_RegWorkingSet.SetX86MapOrder(x86RegHi,1);
			break;
		case CRegInfo::STATE_MAPPED_32_SIGN:
			Reg = SyncTo.cMipsRegMapLo(count);
			UnMap_X86reg(Reg);
			switch (MipsRegState(count)) {
			case CRegInfo::STATE_UNKNOWN: MoveVariableToX86reg(&_GPR[count].UW[0],CRegName::GPR_Lo[count],Reg); break;
			case CRegInfo::STATE_CONST_32: MoveConstToX86reg(MipsRegLo(count),Reg); break;
			case CRegInfo::STATE_MAPPED_32_SIGN: 
				MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count),CRegInfo::NotMapped);
				break;
			case CRegInfo::STATE_MAPPED_32_ZERO:
				if (MipsRegLo(count) != (DWORD)Reg) {
					MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
					m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count),CRegInfo::NotMapped);
				}
				break;
			case CRegInfo::STATE_MAPPED_64:
				MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count),CRegInfo::NotMapped) ;
				m_RegWorkingSet.SetX86Mapped(MipsRegMapHi(count),CRegInfo::NotMapped);
				break;
#ifndef EXTERNAL_RELEASE
			case CRegInfo::STATE_CONST_64:
				DisplayError("hi %X\nLo %X",MipsRegHi(count),MipsRegLo(count));
			default:				
				CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d",MipsRegState(count));
				DisplayError("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d",MipsRegState(count));
#endif
			}
			MipsRegLo(count) = Reg;
			MipsRegState(count) = CRegInfo::STATE_MAPPED_32_SIGN;
			m_RegWorkingSet.SetX86Mapped(Reg,CRegInfo::GPR_Mapped);
			m_RegWorkingSet.SetX86MapOrder(Reg,1);
			break;
		case CRegInfo::STATE_MAPPED_32_ZERO:
			Reg = SyncTo.cMipsRegMapLo(count);
			UnMap_X86reg(Reg);
			switch (MipsRegState(count)) {
			case CRegInfo::STATE_MAPPED_64:
			case CRegInfo::STATE_UNKNOWN:  
				MoveVariableToX86reg(&_GPR[count].UW[0],CRegName::GPR_Lo[count],Reg); 
				break;
			case CRegInfo::STATE_MAPPED_32_ZERO: 
				MoveX86RegToX86Reg(MipsRegMapLo(count),Reg); 
				m_RegWorkingSet.SetX86Mapped(MipsRegMapLo(count),CRegInfo::NotMapped);
				break;
			case CRegInfo::STATE_CONST_32:
				if (MipsRegLo_S(count) < 0) { 
					CPU_Message("Sign Problems in SyncRegState\nSTATE_MAPPED_32_ZERO");
					CPU_Message("%s: %X",CRegName::GPR[count],MipsRegLo_S(count));
#ifndef EXTERNAL_RELEASE
					DisplayError("Sign Problems in SyncRegState\nSTATE_MAPPED_32_ZERO");
#endif
				}
				MoveConstToX86reg(MipsRegLo(count),Reg);  
				break;
#ifndef EXTERNAL_RELEASE
			default:				
				CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d",MipsRegState(count));
				DisplayError("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d",MipsRegState(count));
#endif
			}
			MipsRegLo(count) = Reg;
			MipsRegState(count) = SyncTo.cMipsRegState(count);
			m_RegWorkingSet.SetX86Mapped(Reg,CRegInfo::GPR_Mapped);
			m_RegWorkingSet.SetX86MapOrder(Reg,1);
			break;
		default:
#if (!defined(EXTERNAL_RELEASE))
			CPU_Message("%d\n%d\nreg: %s (%d)",SyncTo.cMipsRegState(count),MipsRegState(count),CRegName::GPR[count],count);
			DisplayError("%d\n%d\nreg: %s (%d)",SyncTo.cMipsRegState(count),MipsRegState(count),CRegName::GPR[count],count);
			DisplayError("Do something with states in SyncRegState");
#endif
			changed = false;
		}
	}
}

void CCodeSection::CompileCop1Test (void) {
	if (m_RegWorkingSet.FpuBeenUsed()) { return; }
	TestVariable(STATUS_CU1,&_Reg->STATUS_REGISTER,"STATUS_REGISTER");
	CompileExit(m_CompilePC,m_CompilePC,m_RegWorkingSet,CExitInfo::COP1_Unuseable,FALSE,JeLabel32);
	m_RegWorkingSet.FpuBeenUsed() = TRUE;
}

bool CCodeSection::CreateSectionLinkage ( void )
{
	InheritConstants();

	if (!FillSectionInfo(NORMAL))
	{
		return false;
	}
	
	CCodeSection ** TargetSection[2];
	CJumpInfo * JumpInfo[2];
	if (m_Jump.TargetPC < m_Cont.TargetPC) {
		TargetSection[0] = (CCodeSection **)&m_JumpSection;
		TargetSection[1] = (CCodeSection **)&m_ContinueSection;
		JumpInfo[0] = &m_Jump;
		JumpInfo[1] = &m_Cont;	
	} else {
		TargetSection[0] = (CCodeSection **)&m_ContinueSection;
		TargetSection[1] = (CCodeSection **)&m_JumpSection;
		JumpInfo[0] = &m_Cont;	
		JumpInfo[1] = &m_Jump;
	}

	CCodeBlock * BlockInfo = m_BlockInfo;

	for (int count = 0; count < 2; count ++) 
	{
		if (JumpInfo[count]->TargetPC == (DWORD)-1 || *TargetSection[count] != NULL) 
		{
			continue;
		}
		if (!JumpInfo[count]->DoneDelaySlot)
		{
			m_Jump.RegSet = m_RegWorkingSet;

			//this is a special delay slot section
			BlockInfo->IncSectionCount();
			*TargetSection[count] = new CCodeSection(BlockInfo,CompilePC() + 4,BlockInfo->NoOfSections());
			(*TargetSection[count])->AddParent(this);
			(*TargetSection[count])->m_LinkAllowed = false;
			(*TargetSection[count])->InheritConstants();

			if (!(*TargetSection[count])->FillSectionInfo(END_BLOCK))
			{
				return false;
			}
			(*TargetSection[count])->m_Jump.TargetPC = -1;
			(*TargetSection[count])->m_Cont.TargetPC = JumpInfo[count]->TargetPC;
			(*TargetSection[count])->m_Cont.FallThrough = true;
			(*TargetSection[count])->m_Cont.RegSet = (*TargetSection[count])->m_RegWorkingSet;
			JumpInfo[count]->TargetPC = CompilePC() + 4;

			//Create the section that joins with that block
			(*TargetSection[count])->m_ContinueSection = BlockInfo->ExistingSection((*TargetSection[count])->m_Cont.TargetPC);
			if ((*TargetSection[count])->m_ContinueSection == NULL) {
				BlockInfo->IncSectionCount();
				(*TargetSection[count])->m_ContinueSection = new CCodeSection(BlockInfo,(*TargetSection[count])->m_Cont.TargetPC,BlockInfo->NoOfSections());
				(*TargetSection[count])->m_ContinueSection->AddParent((*TargetSection[count]));
				(*TargetSection[count])->m_ContinueSection->CreateSectionLinkage();
			} else {
				(*TargetSection[count])->m_ContinueSection->AddParent((*TargetSection[count]));
			}
		} else { 	
			*TargetSection[count] = BlockInfo->ExistingSection(JumpInfo[count]->TargetPC);
			if (*TargetSection[count] == NULL) {
				BlockInfo->IncSectionCount();
				*TargetSection[count] = new CCodeSection(BlockInfo,JumpInfo[count]->TargetPC,BlockInfo->NoOfSections());
				(*TargetSection[count])->AddParent(this);
				(*TargetSection[count])->CreateSectionLinkage();
			} else {
				(*TargetSection[count])->AddParent(this);
			}
		}
	}
	return true;
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
		
		/*if (m_CompilePC >= 0x80000000 && m_CompilePC <= 0x80400000 && m_NextInstruction == NORMAL)
		{
			m_RegWorkingSet.WriteBackRegisters();
			UpdateCounters(m_RegWorkingSet,false,true);
			MoveConstToVariable(m_CompilePC,&_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			if (_SyncSystem) { Call_Direct(SyncToPC, "SyncToPC"); }
		}*/

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
		
		if (!bRegCaching()) { m_RegWorkingSet.WriteBackRegisters(); }
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

void CCodeSection::AddParent(CCodeSection * Parent )
{
	if (this == NULL) { return; }
	if (Parent == NULL) 
	{
		m_RegEnter.Initilize();
		m_RegWorkingSet = m_RegEnter;
		return;
	}

	// check to see if we already have the parent in the list
	for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
	{
		if (*iter == Parent)
		{
			return;
		}
	}
	m_ParentSection.push_back(Parent);

	if (m_ParentSection.size() == 1)
	{
		if (Parent->m_ContinueSection == this) {
			m_RegEnter = Parent->m_Cont.RegSet;
		} else if (Parent->m_JumpSection == this) {
			m_RegEnter = Parent->m_Jump.RegSet;
		} else {
			_Notify->DisplayError("How are these sections joined?????");
		}
		m_RegWorkingSet = m_RegEnter;
	} else {
		if (Parent->m_ContinueSection == this) {
			TestRegConstantStates(Parent->m_Cont.RegSet,m_RegEnter);
		}
		if (Parent->m_JumpSection == this) {
			TestRegConstantStates(Parent->m_Jump.RegSet,m_RegEnter);
		}
		m_RegWorkingSet = m_RegEnter;
	}
}

void CCodeSection::TestRegConstantStates( CRegInfo & Base, CRegInfo & Reg  )
{
	for (int count = 0; count < 32; count++) {
		if (Reg.MipsRegState(count) != Base.MipsRegState(count)) {
			Reg.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
		}
		if (Reg.IsConst(count))
		{
			if (Reg.Is32Bit(count))
			{
				if (Reg.MipsRegLo(count) != Base.MipsRegLo(count)) {
					Reg.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				}
			} else {
				if (Reg.MipsReg(count) != Base.MipsReg(count)) {
					Reg.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				}
			}

		}
	}
}

void CCodeSection::DetermineLoop(DWORD Test, DWORD Test2, DWORD TestID) 
{
	if (this == NULL) { return; }
	
	if (m_SectionID == TestID) 
	{
		if (m_Test2 != Test2) 
		{ 
			m_Test2 = Test2;
			m_ContinueSection->DetermineLoop(Test,Test2,TestID);
			m_JumpSection->DetermineLoop(Test,Test2,TestID);
			
			if (m_Test != Test)
			{ 
				m_Test = Test;
				if (m_ContinueSection != NULL) 
				{
					m_ContinueSection->DetermineLoop(Test,m_BlockInfo->NextTest(),m_ContinueSection->m_SectionID);
				}
				if (m_JumpSection != NULL) 
				{
					m_JumpSection->DetermineLoop(Test,m_BlockInfo->NextTest(),m_JumpSection->m_SectionID);
				}
			}
		} else {
			m_InLoop = true;
		}
	} else {
		if (m_Test2 != Test2) 
		{
			m_Test2 = Test2;
			m_ContinueSection->DetermineLoop(Test,Test2,TestID);
			m_JumpSection->DetermineLoop(Test,Test2,TestID);
		}
	}
}

bool CCodeSection::FixConstants (DWORD Test)
{
	if (this == NULL) { return false; }
	if (m_Test == Test) { return false; }
	m_Test = Test;

	InheritConstants();

	bool Changed = false;
	CRegInfo Original[2] = { m_Cont.RegSet, m_Jump.RegSet };

	if (!m_ParentSection.empty()) 
	{
		for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
		{
			CCodeSection * Parent = *iter;
			if (Parent->m_ContinueSection == this) 
			{
				for (int count = 0; count < 32; count++) 
				{
					if (m_RegEnter.MipsRegState(count) != Parent->m_Cont.RegSet.MipsRegState(count)) {
						m_RegEnter.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;							
						//*Changed = true;
					}
					m_RegEnter.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;							
				}
			}
			if (Parent->m_JumpSection == this) {
				for (int count = 0; count < 32; count++) {
					if (m_RegEnter.MipsRegState(count) != Parent->m_Jump.RegSet.MipsRegState(count)) {
						m_RegEnter.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
						//*Changed = true;
					}
				}
			} 
			m_RegWorkingSet = m_RegEnter;
		}
	}

	FillSectionInfo(NORMAL);
	if (Original[0] != m_Cont.RegSet) 
	{
		Changed = true; 
	}
	if (Original[1] != m_Jump.RegSet) 
	{
		Changed = true;
	}
	
	if (m_JumpSection && m_JumpSection->FixConstants(Test)) { Changed = true; }
	if (m_ContinueSection && m_ContinueSection->FixConstants(Test)) { Changed = true; }
	
	return Changed;
}

CCodeSection * CCodeSection::ExistingSection(DWORD Addr, DWORD Test) 
{
	if (this == NULL) { return NULL; }
	if (m_EnterPC == Addr && m_LinkAllowed) 
	{ 
		return this; 
	}
	if (m_Test == Test) { return NULL; }
	m_Test = Test;

	CCodeSection * Section = m_JumpSection->ExistingSection(Addr,Test);
	if (Section != NULL) { return Section; }
	Section = m_ContinueSection->ExistingSection(Addr,Test);
	if (Section != NULL) { return Section; }

	return NULL;
}

void CCodeSection::InheritConstants( void )
{
	if (m_ParentSection.empty())
	{
		m_RegEnter.Initilize();
		m_RegWorkingSet = m_RegEnter;
		return;
	}

	CCodeSection * Parent = *(m_ParentSection.begin());
	CRegInfo * RegSet = (this == Parent->m_ContinueSection?&Parent->m_Cont.RegSet:&Parent->m_Jump.RegSet);
	m_RegEnter = *RegSet;
	m_RegWorkingSet = *RegSet;		

	for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
	{
		if (iter == m_ParentSection.begin()) { continue; }
		Parent = *iter;
		RegSet = this == Parent->m_ContinueSection?&Parent->m_Cont.RegSet:&Parent->m_Jump.RegSet;
			
		for (int count = 0; count < 32; count++) {
			if (IsConst(count)) {
				if (cMipsRegState(count) != RegSet->MipsRegState(count)) {
					MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				} else if (Is32Bit(count) && cMipsRegLo(count) != RegSet->cMipsRegLo(count)) {
					MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				} else if (Is64Bit(count) && cMipsReg(count) != RegSet->cMipsReg(count)) {
					MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				}
			}
		}
	}
	m_RegEnter = m_RegWorkingSet;
}

bool CCodeSection::FillSectionInfo(STEP_TYPE StartStepType) 
{
	OPCODE Command;

	if (m_CompiledLocation != NULL) { return true; }
	m_CompilePC = m_EnterPC;
	m_RegWorkingSet = m_RegEnter;
	m_NextInstruction = StartStepType;
	do {
		if (!_MMU->LW_VAddr(CompilePC(), Command.Hex)) {
			DisplayError(GS(MSG_FAIL_LOAD_WORD));
			return false;
		}		
		switch (Command.op) {
		case R4300i_SPECIAL:
			switch (Command.funct) {
			case R4300i_SPECIAL_SLL: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = MipsRegLo(Command.rt) << Command.sa;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRL: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = MipsRegLo(Command.rt) >> Command.sa;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRA: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = MipsRegLo_S(Command.rt) >> Command.sa;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SLLV: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = MipsRegLo(Command.rt) << (MipsRegLo(Command.rs) & 0x1F);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRLV: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = MipsRegLo(Command.rt) >> (MipsRegLo(Command.rs) & 0x1F);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRAV: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = MipsRegLo_S(Command.rt) >> (MipsRegLo(Command.rs) & 0x1F);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_JR:				
				if (IsConst(Command.rs)) {
					m_Jump.TargetPC = MipsRegLo(Command.rs);
				} else {
					m_Jump.TargetPC = (DWORD)-1;
				}
				m_NextInstruction = DELAY_SLOT;
				break;
			case R4300i_SPECIAL_JALR: 
				MipsRegLo(Command.rd) = CompilePC() + 8;
				MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				if (IsConst(Command.rs)) {
					m_Jump.TargetPC = MipsRegLo(Command.rs);
				} else {
					m_Jump.TargetPC = (DWORD)-1;
				}
				m_NextInstruction = DELAY_SLOT;
				break;
			case R4300i_SPECIAL_SYSCALL:
			case R4300i_SPECIAL_BREAK:
				m_NextInstruction = END_BLOCK;
				m_CompilePC -= 4;
				break;
			case R4300i_SPECIAL_MFHI: MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN; break;
			case R4300i_SPECIAL_MTHI: break;
			case R4300i_SPECIAL_MFLO: MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN; break;
			case R4300i_SPECIAL_MTLO: break;
			case R4300i_SPECIAL_DSLLV: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg(Command.rd) = Is64Bit(Command.rt)?cMipsReg(Command.rt):(QWORD)MipsRegLo_S(Command.rt) << (MipsRegLo(Command.rs) & 0x3F);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRLV: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg(Command.rd) = Is64Bit(Command.rt)?cMipsReg(Command.rt):(QWORD)MipsRegLo_S(Command.rt) >> (MipsRegLo(Command.rs) & 0x3F);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRAV: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg(Command.rd) = Is64Bit(Command.rt)?cMipsReg_S(Command.rt):(_int64)MipsRegLo_S(Command.rt) >> (MipsRegLo(Command.rs) & 0x3F);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_MULT: break;
			case R4300i_SPECIAL_MULTU: break;
			case R4300i_SPECIAL_DIV: break;
			case R4300i_SPECIAL_DIVU: break;
			case R4300i_SPECIAL_DMULT: break;
			case R4300i_SPECIAL_DMULTU: break;
			case R4300i_SPECIAL_DDIV: break;
			case R4300i_SPECIAL_DDIVU: break;
			case R4300i_SPECIAL_ADD:
			case R4300i_SPECIAL_ADDU:
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegLo(Command.rd) = MipsRegLo(Command.rs) + MipsRegLo(Command.rt);
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SUB: 
			case R4300i_SPECIAL_SUBU: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsRegLo(Command.rd) = MipsRegLo(Command.rs) - MipsRegLo(Command.rt);
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_AND: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					if (Is64Bit(Command.rt) && Is64Bit(Command.rs)) {
						MipsReg(Command.rd) = cMipsReg(Command.rt) & cMipsReg(Command.rs);
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;					
					} else if (Is64Bit(Command.rt) || Is64Bit(Command.rs)) {
						if (Is64Bit(Command.rt)) {
							MipsReg(Command.rd) = cMipsReg(Command.rt) & MipsRegLo(Command.rs);
						} else {
							MipsReg(Command.rd) = MipsRegLo(Command.rt) & cMipsReg(Command.rs);
						}						
						MipsRegState(Command.rd) = CRegInfo::ConstantsType(cMipsReg(Command.rd));
					} else {
						MipsRegLo(Command.rd) = MipsRegLo(Command.rt) & MipsRegLo(Command.rs);
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_OR: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					if (Is64Bit(Command.rt) && Is64Bit(Command.rs)) {
						MipsReg(Command.rd) = cMipsReg(Command.rt) | cMipsReg(Command.rs);
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else if (Is64Bit(Command.rt) || Is64Bit(Command.rs)) {
						if (Is64Bit(Command.rt)) {
							MipsReg(Command.rd) = cMipsReg(Command.rt) | MipsRegLo(Command.rs);
						} else {
							MipsReg(Command.rd) = MipsRegLo(Command.rt) | cMipsReg(Command.rs);
						}
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else {
						MipsRegLo(Command.rd) = MipsRegLo(Command.rt) | MipsRegLo(Command.rs);
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_XOR: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					if (Is64Bit(Command.rt) && Is64Bit(Command.rs)) {
						MipsReg(Command.rd) = cMipsReg(Command.rt) ^ cMipsReg(Command.rs);
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else if (Is64Bit(Command.rt) || Is64Bit(Command.rs)) {
						if (Is64Bit(Command.rt)) {
							MipsReg(Command.rd) = cMipsReg(Command.rt) ^ MipsRegLo(Command.rs);
						} else {
							MipsReg(Command.rd) = MipsRegLo(Command.rt) ^ cMipsReg(Command.rs);
						}
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else {
						MipsRegLo(Command.rd) = MipsRegLo(Command.rt) ^ MipsRegLo(Command.rs);
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_NOR: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					if (Is64Bit(Command.rt) && Is64Bit(Command.rs)) {
						MipsReg(Command.rd) = ~(cMipsReg(Command.rt) | cMipsReg(Command.rs));
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else if (Is64Bit(Command.rt) || Is64Bit(Command.rs)) {
						if (Is64Bit(Command.rt)) {
							MipsReg(Command.rd) = ~(cMipsReg(Command.rt) | MipsRegLo(Command.rs));
						} else {
							MipsReg(Command.rd) = ~(MipsRegLo(Command.rt) | cMipsReg(Command.rs));
						}
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else {
						MipsRegLo(Command.rd) = ~(MipsRegLo(Command.rt) | MipsRegLo(Command.rs));
						MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SLT: 
				if (Command.rd == 0) { break; }
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					if (Is64Bit(Command.rt) || Is64Bit(Command.rs)) {
						if (Is64Bit(Command.rt)) {
							MipsRegLo(Command.rd) = (MipsRegLo_S(Command.rs) < cMipsReg_S(Command.rt))?1:0;
						} else {
							MipsRegLo(Command.rd) = (cMipsReg_S(Command.rs) < MipsRegLo_S(Command.rt))?1:0;
						}
					} else {
						MipsRegLo(Command.rd) = (MipsRegLo_S(Command.rs) < MipsRegLo_S(Command.rt))?1:0;
					}
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SLTU: 
				if (Command.rd == 0) { break; }
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					if (Is64Bit(Command.rt) || Is64Bit(Command.rs)) {
						if (Is64Bit(Command.rt)) {
							MipsRegLo(Command.rd) = (MipsRegLo(Command.rs) < cMipsReg(Command.rt))?1:0;
						} else {
							MipsRegLo(Command.rd) = (cMipsReg(Command.rs) < MipsRegLo(Command.rt))?1:0;
						}
					} else {
						MipsRegLo(Command.rd) = (MipsRegLo(Command.rs) < MipsRegLo(Command.rt))?1:0;
					}
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DADD: 
			case R4300i_SPECIAL_DADDU: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsReg(Command.rd) = 
						Is64Bit(Command.rs)?cMipsReg(Command.rs):(_int64)MipsRegLo_S(Command.rs) +
						Is64Bit(Command.rt)?cMipsReg(Command.rt):(_int64)MipsRegLo_S(Command.rt);
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSUB: 
			case R4300i_SPECIAL_DSUBU: 
				if (Command.rd == 0) { break; }
				if (m_InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt) && IsConst(Command.rs)) {
					MipsReg(Command.rd) = 
						Is64Bit(Command.rs)?cMipsReg(Command.rs):(_int64)MipsRegLo_S(Command.rs) -
						Is64Bit(Command.rt)?cMipsReg(Command.rt):(_int64)MipsRegLo_S(Command.rt);
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSLL:
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg(Command.rd) = Is64Bit(Command.rt)?cMipsReg(Command.rt):(_int64)MipsRegLo_S(Command.rt) << Command.sa;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRL:
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg(Command.rd) = Is64Bit(Command.rt)?cMipsReg(Command.rt):(QWORD)MipsRegLo_S(Command.rt) >> Command.sa;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRA:
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg_S(Command.rd) = Is64Bit(Command.rt)?cMipsReg_S(Command.rt):(_int64)MipsRegLo_S(Command.rt) >> Command.sa;
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSLL32:
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					MipsReg(Command.rd) = MipsRegLo(Command.rt) << (Command.sa + 32);
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRL32:
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = (DWORD)(cMipsReg(Command.rt) >> (Command.sa + 32));
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRA32:
				if (Command.rd == 0) { break; }
				if (m_InLoop && Command.rt == Command.rd) {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (IsConst(Command.rt)) {
					MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					MipsRegLo(Command.rd) = (DWORD)(cMipsReg_S(Command.rt) >> (Command.sa + 32));
				} else {
					MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			default:
#ifndef EXTERNAL_RELEASE
				if (Command.Hex == 0x00000001) { break; }
				DisplayError("Unhandled R4300i OpCode in FillSectionInfo 5\n%s",
					R4300iOpcodeName(Command.Hex,CompilePC()));
#endif
				m_NextInstruction = END_BLOCK;
				m_CompilePC -= 4;
			}
			break;
		case R4300i_REGIMM:
			switch (Command.rt) {
			case R4300i_REGIMM_BLTZ:
			case R4300i_REGIMM_BGEZ:
				m_NextInstruction = DELAY_SLOT;
				m_Cont.TargetPC = CompilePC() + 8;
				m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
				if (CompilePC() == m_Jump.TargetPC) {
					if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,0)) {
						m_Jump.PermLoop = true;
					}
				} 
				break;
			case R4300i_REGIMM_BLTZL:
			case R4300i_REGIMM_BGEZL:
				m_NextInstruction = LIKELY_DELAY_SLOT;
				m_Cont.TargetPC = CompilePC() + 8;
				m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
				if (CompilePC() == m_Jump.TargetPC) { 
					if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,0)) {
						m_Jump.PermLoop = true;
					}
				} 
				break;
			case R4300i_REGIMM_BLTZAL:
				MipsRegLo(31) = CompilePC() + 8;
				MipsRegState(31) = CRegInfo::STATE_CONST_32;
				m_Cont.TargetPC = CompilePC() + 8;
				m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
				if (CompilePC() == m_Jump.TargetPC) { 
					if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,0)) {
						m_Jump.PermLoop = true;
					}
				} 
				break;
			case R4300i_REGIMM_BGEZAL:
				m_NextInstruction = DELAY_SLOT;
				if (IsConst(Command.rs)) 
				{
					__int64 Value;
					if (Is32Bit(Command.rs))
					{
						Value = MipsRegLo_S(Command.rs);
					} else {
						Value = cMipsReg_S(Command.rs);
					}
					if (Value >= 0) {
						MipsRegLo(31) = CompilePC() + 8;
						MipsRegState(31) = CRegInfo::STATE_CONST_32;
						m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
						if (CompilePC() == m_Jump.TargetPC) {
							if (!DelaySlotEffectsCompare(CompilePC(),31,0)) {
								m_Jump.PermLoop = true;
							}
						} 
						break;
					}
				} 

				
				MipsRegLo(31) = CompilePC() + 8;
				MipsRegState(31) = CRegInfo::STATE_CONST_32;
				m_Cont.TargetPC = CompilePC() + 8;
				m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
				if (CompilePC() == m_Jump.TargetPC) { 
					if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,0)) {
						m_Jump.PermLoop = true;
					}
				} 
				break;
			default:
#ifndef EXTERNAL_RELEASE
				if (Command.Hex == 0x0407000D) { break; }
				DisplayError("Unhandled R4300i OpCode in FillSectionInfo 4\n%s",
					R4300iOpcodeName(Command.Hex,CompilePC()));
#endif
				m_NextInstruction = END_BLOCK;
				m_CompilePC -= 4;
			}
			break;
		case R4300i_JAL: 
			m_NextInstruction = DELAY_SLOT;
			MipsRegLo(31) = CompilePC() + 8;
			MipsRegState(31) = CRegInfo::STATE_CONST_32;
			m_Jump.TargetPC = (CompilePC() & 0xF0000000) + (Command.target << 2);
			if (CompilePC() == m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CompilePC(),31,0)) {
					m_Jump.PermLoop = true;
				}
			} 
			break;
		case R4300i_J: 
			m_NextInstruction = DELAY_SLOT;
			m_Jump.TargetPC = (CompilePC() & 0xF0000000) + (Command.target << 2);
			if (CompilePC() == m_Jump.TargetPC) { m_Jump.PermLoop = true; } 
			break;
		case R4300i_BEQ: 
			m_NextInstruction = DELAY_SLOT;
			m_Cont.TargetPC = CompilePC() + 8;
			m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
			if (CompilePC() == m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,Command.rt)) {
					m_Jump.PermLoop = true;
				}
			} 
			if (IsConst(Command.rs) && IsConst(Command.rt)) 
			{
				__int64 Value1, Value2;
				if (Is32Bit(Command.rs))
				{
					Value1 = MipsRegLo_S(Command.rs);
				} else {
					Value1 = cMipsReg_S(Command.rs);
				}
				if (Is32Bit(Command.rt))
				{
					Value2 = MipsRegLo_S(Command.rt);
				} else {
					Value2 = cMipsReg_S(Command.rt);
				}
				if (Value1 == Value2) 
				{
					m_Cont.TargetPC = -1;
				}
			} 
			break;
		case R4300i_BNE: 
		case R4300i_BLEZ: 
		case R4300i_BGTZ: 
			m_NextInstruction = DELAY_SLOT;
			m_Cont.TargetPC = CompilePC() + 8;
			m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
			if (CompilePC() == m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,Command.rt)) {
					m_Jump.PermLoop = true;
				}
			} 
			break;
		case R4300i_ADDI: 
		case R4300i_ADDIU: 
			if (Command.rt == 0) { break; }
			if (m_InLoop && Command.rs == Command.rt) {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (IsConst(Command.rs)) { 
				MipsRegLo(Command.rt) = MipsRegLo(Command.rs) + (short)Command.immediate;
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_SLTI: 
			if (Command.rt == 0) { break; }
			if (IsConst(Command.rs)) { 
				if (Is64Bit(Command.rs)) {
					MipsRegLo(Command.rt) = (cMipsReg_S(Command.rs) < (_int64)((short)Command.immediate))?1:0;
				} else {
					MipsRegLo(Command.rt) = (MipsRegLo_S(Command.rs) < (int)((short)Command.immediate))?1:0;
				}
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_SLTIU: 
			if (Command.rt == 0) { break; }
			if (IsConst(Command.rs)) { 
				if (Is64Bit(Command.rs)) {
					MipsRegLo(Command.rt) = (cMipsReg(Command.rs) < (unsigned _int64)((short)Command.immediate))?1:0;
				} else {
					MipsRegLo(Command.rt) = (MipsRegLo(Command.rs) < (DWORD)((short)Command.immediate))?1:0;
				}
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_LUI: 
			if (Command.rt == 0) { break; }
			MipsRegLo(Command.rt) = ((short)Command.offset << 16);
			MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			break;
		case R4300i_ANDI: 
			if (Command.rt == 0) { break; }
			if (m_InLoop && Command.rs == Command.rt) {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (IsConst(Command.rs)) {
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
				MipsRegLo(Command.rt) = MipsRegLo(Command.rs) & Command.immediate;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_ORI: 
			if (Command.rt == 0) { break; }
			if (m_InLoop && Command.rs == Command.rt) {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (IsConst(Command.rs)) {
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
				MipsRegLo(Command.rt) = MipsRegLo(Command.rs) | Command.immediate;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_XORI: 
			if (Command.rt == 0) { break; }
			if (m_InLoop && Command.rs == Command.rt) {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (IsConst(Command.rs)) {
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
				MipsRegLo(Command.rt) = MipsRegLo(Command.rs) ^ Command.immediate;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_CP0:
			switch (Command.rs) {
			case R4300i_COP0_MF:
				if (Command.rt == 0) { break; }
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
				break;
			case R4300i_COP0_MT: break;
			default:
				if ( (Command.rs & 0x10 ) != 0 ) {
					switch( Command.funct ) {
					case R4300i_COP0_CO_TLBR: break;
					case R4300i_COP0_CO_TLBWI: break;
					case R4300i_COP0_CO_TLBWR: break;
					case R4300i_COP0_CO_TLBP: break;
					case R4300i_COP0_CO_ERET: m_NextInstruction = END_BLOCK; break;
					default:
#ifndef EXTERNAL_RELEASE
						DisplayError("Unhandled R4300i OpCode in FillSectionInfo\n%s",
							R4300iOpcodeName(Command.Hex,CompilePC()));
#endif
						m_NextInstruction = END_BLOCK;
						m_CompilePC -= 4;
					}
				} else {
#ifndef EXTERNAL_RELEASE
					DisplayError("Unhandled R4300i OpCode in FillSectionInfo 3\n%s",
						R4300iOpcodeName(Command.Hex,CompilePC()));
#endif
					m_NextInstruction = END_BLOCK;
					m_CompilePC -= 4;
				}
			}
			break;
		case R4300i_CP1:
			switch (Command.fmt) {
			case R4300i_COP1_CF:
			case R4300i_COP1_MF:
			case R4300i_COP1_DMF:
				if (Command.rt == 0) { break; }
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
				break;
			case R4300i_COP1_BC:
				switch (Command.ft) {
				case R4300i_COP1_BC_BCFL:
				case R4300i_COP1_BC_BCTL:
					m_NextInstruction = LIKELY_DELAY_SLOT;
					m_Cont.TargetPC = CompilePC() + 8;
					m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
					if (CompilePC() == m_Jump.TargetPC) {
						int EffectDelaySlot;
						OPCODE NewCommand;

						if (!_MMU->LW_VAddr(CompilePC() + 4, NewCommand.Hex)) {
							DisplayError(GS(MSG_FAIL_LOAD_WORD));
							ExitThread(0);
						}
						
						EffectDelaySlot = false;
						if (NewCommand.op == R4300i_CP1) {
							if (NewCommand.fmt == R4300i_COP1_S && (NewCommand.funct & 0x30) == 0x30 ) {
								EffectDelaySlot = true;
							} 
							if (NewCommand.fmt == R4300i_COP1_D && (NewCommand.funct & 0x30) == 0x30 ) {
								EffectDelaySlot = true;
							} 
						}						
						if (!EffectDelaySlot) {
							m_Jump.PermLoop = true;
						}
					} 
					break;
				case R4300i_COP1_BC_BCF:
				case R4300i_COP1_BC_BCT:
					m_NextInstruction = DELAY_SLOT;
					m_Cont.TargetPC = CompilePC() + 8;
					m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
					if (CompilePC() == m_Jump.TargetPC) {
						int EffectDelaySlot;
						OPCODE NewCommand;

						if (!_MMU->LW_VAddr(CompilePC() + 4, NewCommand.Hex)) {
							DisplayError(GS(MSG_FAIL_LOAD_WORD));
							ExitThread(0);
						}
						
						EffectDelaySlot = false;
						if (NewCommand.op == R4300i_CP1) {
							if (NewCommand.fmt == R4300i_COP1_S && (NewCommand.funct & 0x30) == 0x30 ) {
								EffectDelaySlot = true;
							} 
							if (NewCommand.fmt == R4300i_COP1_D && (NewCommand.funct & 0x30) == 0x30 ) {
								EffectDelaySlot = true;
							} 
						}						
						if (!EffectDelaySlot) {
							m_Jump.PermLoop = true;
						}
					} 
					break;
				}
				break;
			case R4300i_COP1_MT: break;
			case R4300i_COP1_DMT: break;
			case R4300i_COP1_CT: break;
			case R4300i_COP1_S: break;
			case R4300i_COP1_D: break;
			case R4300i_COP1_W: break;
			case R4300i_COP1_L: break;
			default:
#ifndef EXTERNAL_RELEASE
				DisplayError("Unhandled R4300i OpCode in FillSectionInfo 2\n%s",
					R4300iOpcodeName(Command.Hex,CompilePC()));
#endif
				m_NextInstruction = END_BLOCK;
				m_CompilePC -= 4;
			}
			break;
		case R4300i_BEQL: 
		case R4300i_BNEL: 
		case R4300i_BLEZL: 
		case R4300i_BGTZL: 
			m_NextInstruction = LIKELY_DELAY_SLOT;
			m_Cont.TargetPC = CompilePC() + 8;
			m_Jump.TargetPC = CompilePC() + ((short)Command.offset << 2) + 4;
			if (CompilePC() == m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CompilePC(),Command.rs,Command.rt)) {
					m_Jump.PermLoop = true;
				}
			} 
			break;
		case R4300i_DADDI: 
		case R4300i_DADDIU: 
			if (Command.rt == 0) { break; }
			if (m_InLoop && Command.rs == Command.rt) {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (IsConst(Command.rs)) { 
				if (Is64Bit(Command.rs)) { 
					int imm32 = (short)Command.immediate;
					__int64 imm64 = imm32;										
					MipsReg_S(Command.rt) = MipsRegLo_S(Command.rs) + imm64;
				} else {
					MipsReg_S(Command.rt) = MipsRegLo_S(Command.rs) + (short)Command.immediate;
				}
				MipsRegState(Command.rt) = CRegInfo::STATE_CONST_64;
			} else {
				MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_LDR:
		case R4300i_LDL:
		case R4300i_LB:
		case R4300i_LH: 
		case R4300i_LWL: 
		case R4300i_LW: 
		case R4300i_LWU: 
		case R4300i_LL: 
		case R4300i_LBU:
		case R4300i_LHU: 
		case R4300i_LWR: 
		case R4300i_SC: 
			if (Command.rt == 0) { break; }
			MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			break;
		case R4300i_SB: break;
		case R4300i_SH: break;
		case R4300i_SWL: break;
		case R4300i_SW: break;
		case R4300i_SWR: break;
		case R4300i_SDL: break;
		case R4300i_SDR: break;
		case R4300i_CACHE: break;
		case R4300i_LWC1: break;
		case R4300i_SWC1: break;
		case R4300i_LDC1: break;
		case R4300i_LD:
			if (Command.rt == 0) { break; }
			MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			break;
		case R4300i_SDC1: break;
		case R4300i_SD: break;
		default:
			m_NextInstruction = END_BLOCK;
			m_CompilePC -= 4;
			if (Command.Hex == 0x7C1C97C0) { break; }
			if (Command.Hex == 0x7FFFFFFF) { break; }
			if (Command.Hex == 0xF1F3F5F7) { break; }
			if (Command.Hex == 0xC1200000) { break; }
			if (Command.Hex == 0x4C5A5353) { break; }
#ifndef EXTERNAL_RELEASE
			DisplayError("Unhandled R4300i OpCode in FillSectionInfo 1\n%s\n%X",
				R4300iOpcodeName(Command.Hex,CompilePC()),Command.Hex);
#endif
		}

//		if (CompilePC() == 0x8005E4B8) {
//CPU_Message("%X: %s %s = %d",CompilePC(),R4300iOpcodeName(Command.Hex,CompilePC()),
//			CRegName::GPR[8],cMipsRegState(8));
//_asm int 3
//		}
		switch (m_NextInstruction) {
		case NORMAL: 
			m_CompilePC += 4; 
			break;
		case DELAY_SLOT:
			m_NextInstruction = DELAY_SLOT_DONE;
			m_CompilePC += 4; 
			break;
		case LIKELY_DELAY_SLOT:
			if (m_Cont.TargetPC == m_Jump.TargetPC)
			{
				m_Jump.RegSet = m_RegWorkingSet; 
				m_Cont.DoneDelaySlot = false;
				m_Cont.RegSet = m_RegWorkingSet;
				m_Cont.DoneDelaySlot = true;
				m_NextInstruction = END_BLOCK;
			} else {
				m_Cont.RegSet = m_RegWorkingSet;
				m_Cont.DoneDelaySlot = true;
				m_NextInstruction = LIKELY_DELAY_SLOT_DONE;
				m_CompilePC += 4; 
			}
			break;
		case DELAY_SLOT_DONE:
			m_Cont.RegSet = m_RegWorkingSet;
			m_Jump.RegSet = m_RegWorkingSet; 
			m_Cont.DoneDelaySlot = true;
			m_Jump.DoneDelaySlot = true; 
			m_NextInstruction = END_BLOCK;
			break;
		case LIKELY_DELAY_SLOT_DONE:
			m_Jump.RegSet = m_RegWorkingSet;
			m_Jump.DoneDelaySlot = true; 
			m_NextInstruction = END_BLOCK;
			break;
		}		
		if ((CompilePC() & 0xFFFFF000) != (m_EnterPC & 0xFFFFF000)) {
			if (m_NextInstruction != END_BLOCK && m_NextInstruction != NORMAL) {
			//	DisplayError("Branch running over delay slot ???\nm_NextInstruction == %d",m_NextInstruction);
				m_Cont.TargetPC = (DWORD)-1;
				m_Jump.TargetPC = (DWORD)-1;
			} 
			m_NextInstruction = END_BLOCK;
			m_CompilePC -= 4;
		}
	} while (m_NextInstruction != END_BLOCK);

	if (m_Cont.TargetPC != (DWORD)-1) {
		if ((m_Cont.TargetPC & 0xFFFFF000) != (m_EnterPC & 0xFFFFF000)) {
			m_Cont.TargetPC = (DWORD)-1;
		}
	}
	if (m_Jump.TargetPC != (DWORD)-1) {
		if (m_Jump.TargetPC < m_BlockInfo->VAddrEnter())
		{
			m_Jump.TargetPC = (DWORD)-1;
		}
		if ((m_Jump.TargetPC & 0xFFFFF000) != (m_EnterPC & 0xFFFFF000)) {
			m_Jump.TargetPC = (DWORD)-1;
		}
	}
	return true;
}

