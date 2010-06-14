#include "stdafx.h"

CRecompiler::CRecompiler(CProfiling & Profile, bool & EndEmulation ) :
	m_Profile(Profile),
	PROGRAM_COUNTER(_Reg->m_PROGRAM_COUNTER),
	m_EndEmulation(EndEmulation)
{
}

CRecompiler::~CRecompiler()
{
	for (int i = 0, n = m_Functions.size(); i < n; i++)
	{
		delete m_Functions[i];
	}
}

void CRecompiler::Run()
{
	CoInitialize(NULL);
	if (g_LogX86Code)
	{
		Start_x86_Log();
	}

	if (!CRecompMemory::AllocateMemory())
	{
		WriteTrace(TraceError,"CRecompiler::Run: CRecompMemory::AllocateMemory failed");
		return;
	}
	m_EndEmulation = false;

	WriteTrace(TraceError,"CRecompiler::Run Fix g_MemoryStack");
#ifdef tofix
	*g_MemoryStack = (DWORD)(RDRAM+(_GPR[29].W[0] & 0x1FFFFFFF));
#endif
	__try {
		if (LookUpMode() == FuncFind_VirtualLookup)
		{
			if (!CFunctionMap::AllocateMemory())
			{
				WriteTrace(TraceError,"CRecompiler::Run: CFunctionMap::AllocateMemory failed");
				return;
			}
			if (bSMM_ValidFunc())
			{
				RecompilerMain_VirtualTable_validate();
			} else {
				RecompilerMain_VirtualTable();
			}
		}
		else if (LookUpMode() == FuncFind_ChangeMemory) 
		{
			RecompilerMain_ChangeMemory();
		} 
		else 
		{
			RecompilerMain_Lookup();
		}
	}
	__except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) 
	{
		Notify().DisplayError(MSG_UNKNOWN_MEM_ACTION);
	}
}

void CRecompiler::RecompilerMain_VirtualTable ( void )
{
	while(!m_EndEmulation) 
	{
		PCCompiledFunc_TABLE & table = FunctionTable()[PROGRAM_COUNTER >> 0xC];
		DWORD TableEntry = (PROGRAM_COUNTER & 0xFFF) >> 2;
		if (table)
		{
			CCompiledFunc * info = table[TableEntry];
			if (info != NULL)
			{
				(info->Function())();
				continue;
			}
		}
		if (!_TransVaddr->ValidVaddr(PROGRAM_COUNTER)) 
		{
			_Reg->DoTLBMiss(false,PROGRAM_COUNTER);
			if (!_TransVaddr->ValidVaddr(PROGRAM_COUNTER)) 
			{
				DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
				return;
			}
			continue;
		}

		CCompiledFunc * info = CompilerCode();
		if (info == NULL || m_EndEmulation)
		{
			break;
		}

		if (table == NULL) 
		{
			table = new PCCompiledFunc[(0x1000 >> 2)]; 
			if (table == NULL)
			{
				Notify().FatalError(MSG_MEM_ALLOC_ERROR);
			}
			memset(table,0,sizeof(PCCompiledFunc) * (0x1000 >> 2));
		}

		table[TableEntry] = info;
		(info->Function())();
	}
}

void CRecompiler::RecompilerMain_VirtualTable_validate ( void )
{
	_Notify->BreakPoint(__FILE__,__LINE__);
/*	PCCompiledFunc_TABLE * m_FunctionTable = m_Functions.GetFunctionTable();

	while(!m_EndEmulation) 
	{
		/*if (NextInstruction == DELAY_SLOT) 
		{
			CCompiledFunc * Info = m_FunctionsDelaySlot.FindFunction(PROGRAM_COUNTER);
			//Find Block on hash table
			if (Info == NULL) 
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
				if (!_TLB->ValidVaddr(PROGRAM_COUNTER)) 
				{
					DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
					NextInstruction = NORMAL;
					if (!_TLB->ValidVaddr(PROGRAM_COUNTER)) 
					{
						DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
						return;
					}
					continue;
				}
#endif
				//Find Block on hash table
				Info = CompileDelaySlot(PROGRAM_COUNTER);

				if (Info == NULL || EndEmulation())
				{
					break;
				}
			} 
			const BYTE * Block = Info->FunctionAddr();
			if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
				(*Info->MemLocation[1] != Info->MemContents[1])) 
			{
				ClearRecompCode_Virt((Info->VStartPC() - 0x1000) & ~0xFFF,0x2000,Remove_ValidateFunc);
				NextInstruction = DELAY_SLOT;
				Info = NULL;
				continue;
			}
			_asm {
				pushad
				call Block
				popad
			}
			continue;
		}*/
/*		PCCompiledFunc_TABLE table = m_FunctionTable[PROGRAM_COUNTER >> 0xC];
		if (table)
		{
			CCompiledFunc * info = table[(PROGRAM_COUNTER & 0xFFF) >> 2];
			if (info != NULL)
			{
				if ((*info->MemLocation[0] != info->MemContents[0]) ||
					(*info->MemLocation[1] != info->MemContents[1])) 
				{
					ClearRecompCode_Virt((info->VStartPC() - 0x1000) & ~0xFFF,0x3000,Remove_ValidateFunc);
					info = NULL;
					continue;
				}
				const BYTE * Block = info->FunctionAddr();
				_asm {
					pushad
					call Block
					popad
				}
				continue;
			}
		}
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if (!_TLB->ValidVaddr(PROGRAM_COUNTER)) 
		{
			DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
			NextInstruction = NORMAL;
			if (!_TLB->ValidVaddr(PROGRAM_COUNTER)) 
			{
				DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
				return;
			}
		}
#endif
		CCompiledFunc * info = CompilerCode();

		if (info == NULL || EndEmulation())
		{
			break;
		}

	}

/*
	while(!m_EndEmulation) 
	{
		if (!_MMU->ValidVaddr(PROGRAM_COUNTER)) 
		{
			DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
			NextInstruction = NORMAL;
			if (!_MMU->ValidVaddr(PROGRAM_COUNTER)) 
			{
				DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
				return;
			}
		}
		if (NextInstruction == DELAY_SLOT) 
		{
			CCompiledFunc * Info = m_FunctionsDelaySlot.FindFunction(PROGRAM_COUNTER);

			//Find Block on hash table
			if (Info == NULL) 
			{
				Info = CompileDelaySlot(PROGRAM_COUNTER);

				if (Info == NULL || EndEmulation())
				{
					break;
				}
			} 
			if (bSMM_ValidFunc())
			{
				if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
					(*Info->MemLocation[1] != Info->MemContents[1])) 
				{
					ClearRecompCode_Virt((Info->StartPC() - 0x1000) & ~0xFFF,0x2000,Remove_ValidateFunc);
					NextInstruction = DELAY_SLOT;
					Info = NULL;
					continue;
				}
			}
			const BYTE * Block = Info->FunctionAddr();
			_asm {
				pushad
				call Block
				popad
			}
			continue;
		}
		
		CCompiledFunc * Info = m_Functions.FindFunction(PROGRAM_COUNTER);

		//Find Block on hash table
		if (Info == NULL) 
		{
			Info = CompilerCode();

			if (Info == NULL || EndEmulation())
			{
				break;
			}
		} 
		if (bSMM_ValidFunc())
		{
			if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
				(*Info->MemLocation[1] != Info->MemContents[1])) 
			{
				ClearRecompCode_Virt((Info->StartPC() - 0x1000) & ~0xFFF,0x3000,Remove_ValidateFunc);
				Info = NULL;
				continue;
			}
		}
		const BYTE * Block = Info->FunctionAddr();
		_asm {
			pushad
			call Block
			popad
		}
	}
	*/
}

void CRecompiler::RecompilerMain_Lookup( void )
{
	_Notify->BreakPoint(__FILE__,__LINE__);

	/*
	DWORD Addr;
	CCompiledFunc * Info;
	//const BYTE * Block;

	while(!m_EndEmulation) 
	{
		/*if (g_UseTlb)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			if (!_TLB->TranslateVaddr(PROGRAM_COUNTER, Addr))
			{
				DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
				NextInstruction = NORMAL;
				if (!TranslateVaddr(PROGRAM_COUNTER, &Addr)) {
					DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
					return;
				}
			}
#endif
		} else {
			Addr = PROGRAM_COUNTER & 0x1FFFFFFF;
		}*/

	/*	if (NextInstruction == DELAY_SLOT) {
			CCompiledFunc * Info = m_FunctionsDelaySlot.FindFunction(PROGRAM_COUNTER);

			//Find Block on hash table
			if (Info == NULL) 
			{
				Info = CompileDelaySlot(PROGRAM_COUNTER);

				if (Info == NULL || EndEmulation())
				{
					break;
				}
			} 
			if (bSMM_ValidFunc())
			{
				if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
					(*Info->MemLocation[1] != Info->MemContents[1])) 
				{
					ClearRecompCode_Virt((Info->VStartPC() - 0x1000) & ~0xFFF,0x2000,Remove_ValidateFunc);
					NextInstruction = DELAY_SLOT;
					Info = NULL;
					continue;
				}
			}
			const BYTE * Block = Info->FunctionAddr();
			_asm {
				pushad
				call Block
				popad
			}
			continue;
		}

		__try {
			if (Addr > 0x10000000)
			{
				if (bRomInMemory())
				{
					if (Addr > 0x20000000)
					{
						WriteTraceF(TraceDebug,"Executing from non mapped space .1 PC: %X Addr: %X",PROGRAM_COUNTER, Addr);
						DisplayError(GS(MSG_NONMAPPED_SPACE));
						break;
					}
					Info = (CCompiledFunc *)*(JumpTable + (Addr >> 2));
				} else {
					if (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
						while (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
							ExecuteInterpreterOpCode();
						}
						continue;
					} else {
						WriteTraceF(TraceDebug,"Executing from non mapped space .1 PC: %X Addr: %X",PROGRAM_COUNTER, Addr);
						DisplayError(GS(MSG_NONMAPPED_SPACE));
						break;
					}
				}
			} else {
				Info = (CCompiledFunc *)*(JumpTable + (Addr >> 2));
			}
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			if (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
				while (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
					ExecuteInterpreterOpCode();
				}
				continue;
			} else {
					WriteTraceF(TraceDebug,"Executing from non mapped space .2 PC: %X Addr: %X",PROGRAM_COUNTER, Addr);
				DisplayError(GS(MSG_NONMAPPED_SPACE));
				return;
			}
		}
		
		if (Info == NULL) 
		{
			Info = CompilerCode();

			if (Info == NULL || EndEmulation())
			{
				break;
			}
			*(JumpTable + (Addr >> 2)) = (void *)Info;

//			if (SelfModCheck == ModCode_ProtectedMemory) {
//				VirtualProtect(RDRAM + Addr, 4, PAGE_READONLY, &OldProtect);
//			}
		}
		if (bSMM_ValidFunc())
		{
			if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
				(*Info->MemLocation[1] != Info->MemContents[1])) 
			{
				ClearRecompCode_Virt((Info->VStartPC() - 0x1000) & ~0xFFF,0x3000,Remove_ValidateFunc);
				Info = NULL;
				continue;
			}
		}
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if (Profiling && IndvidualBlock) {
			static DWORD ProfAddress = 0;

			if ((PROGRAM_COUNTER & ~0xFFF) != ProfAddress) {
				char Label[100];

				ProfAddress = PROGRAM_COUNTER & ~0xFFF;
				sprintf(Label,"PC: %X to %X",ProfAddress,ProfAddress+ 0xFFC);
//						StartTimer(Label);				
			}
			/*if (PROGRAM_COUNTER >= 0x800DD000 && PROGRAM_COUNTER <= 0x800DDFFC) {
				char Label[100];
				sprintf(Label,"PC: %X   Block: %X",PROGRAM_COUNTER,Block);
				StartTimer(Label);				
			}*/
//				} else 	if ((Profiling || ShowCPUPer) && ProfilingLabel[0] == 0) { 
//					StartTimer("r4300i Running"); 
/*		}
#endif
		const BYTE * Block = Info->FunctionAddr();
		_asm {
			pushad
			call Block
			popad
		}
	}*/
}

void CRecompiler::ResetRecompCode()
{
	CRecompMemory::Reset();
	CFunctionMap::Reset();
}

BYTE * CRecompiler::CompileDelaySlot(DWORD PC) 
{
	int Index = PC >> 0xC;
	BYTE * delayFunc = DelaySlotTable()[Index];
	if (delayFunc)
	{
		return delayFunc;
	}
	
	WriteTraceF(TraceRecompiler,"Compile Delay Slot: %X",PC);
	if ((PC & 0xFFC) != 0) {
		DisplayError("Why are you compiling the Delay Slot at %X",PC);
		return NULL;
	}

	CheckRecompMem();

	CCodeBlock CodeBlock(PC, RecompPos(), true);
	if (!CodeBlock.Compile())
	{
		return NULL;
	}
	
	CCompiledFunc * Func = new CCompiledFunc(CodeBlock);
	delayFunc = (BYTE *)Func->Function();
	DelaySlotTable()[Index] = delayFunc;
	delete Func;
	return delayFunc;
}

bool CRecompiler::AnalyseBlock ( CCodeBlock & BlockInfo) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (bLinkBlocks())
	{ 	
		CCodeSection * Section = &BlockInfo.ParentSection;
		if (!CreateSectionLinkage (Section)) { return false; }
		DetermineLoop(Section,CCodeSection::GetNewTestValue(),CCodeSection::GetNewTestValue(), Section->m_SectionID);
		while (FixConstants(Section,CCodeSection::GetNewTestValue())) {}
	}
#endif
	return true;
}

bool CRecompiler::FixConstants (CCodeSection * Section, DWORD Test)
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (Section == NULL) { return false; }
	if (Section->Test == Test) { return false; }
	Section->Test = Test;

	InheritConstants(Section);

	bool Changed = false;
/*	BLOCK_SECTION * Parent;
	int count, NoOfParents;
	REG_INFO Original[2];
*/
	CRegInfo Original[2] = { Section->m_Cont.RegSet, Section->m_Jump.RegSet };

	if (!Section->ParentSection.empty()) {
		for (SECTION_LIST::iterator iter = Section->ParentSection.begin(); iter != Section->ParentSection.end(); iter++)
		{
			CCodeSection * Parent = *iter;
			if (Parent->m_ContinueSection == Section) {
				for (int count = 0; count < 32; count++) {
					if (Section->RegStart.MipsRegState(count) != Parent->m_Cont.RegSet.MipsRegState(count)) {
						Section->RegStart.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;							
						//*Changed = true;
					}
					Section->RegStart.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;							
				}
			}
			if (Parent->m_JumpSection == Section) {
				for (int count = 0; count < 32; count++) {
					if (Section->RegStart.MipsRegState(count) != Parent->m_Jump.RegSet.MipsRegState(count)) {
						Section->RegStart.MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
						//*Changed = true;
					}
				}
			}
			Section->RegWorking = Section->RegStart;
		}
	}

	FillSectionInfo(Section, NORMAL);
	if (Original[0] != Section->m_Cont.RegSet) 
	{
		Changed = true; 
	}
	if (Original[1] != Section->m_Jump.RegSet) 
	{
		Changed = true;
	}
	
	if (Section->m_JumpSection && FixConstants(Section->m_JumpSection,Test)) { Changed = true; }
	if (Section->m_ContinueSection && FixConstants(Section->m_ContinueSection,Test)) { Changed = true; }
	
	return Changed;
#endif
	return false;
}

void CRecompiler::InheritConstants(CCodeSection * Section)
{
_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (Section->ParentSection.empty())
	{
		Section->RegStart.Initilize();
		Section->RegWorking = Section->RegStart;
		return;
	}

	CCodeSection * Parent = *(Section->ParentSection.begin());
	CRegInfo * RegSet = (Section == Parent->m_ContinueSection?&Parent->m_Cont.RegSet:&Parent->m_Jump.RegSet);
	Section->RegStart = *RegSet;
	Section->RegWorking = *RegSet;		

	for (SECTION_LIST::iterator iter = Section->ParentSection.begin(); iter != Section->ParentSection.end(); iter++)
	{
		if (iter == Section->ParentSection.begin()) { continue; }
		Parent = *iter;
		RegSet = Section == Parent->m_ContinueSection?&Parent->m_Cont.RegSet:&Parent->m_Jump.RegSet;
			
		for (int count = 0; count < 32; count++) {
			if (Section->IsConst(count)) {
				if (Section->MipsRegState(count) != RegSet->MipsRegState(count)) {
					Section->MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				} else if (Section->Is32Bit(count) && Section->MipsRegLo(count) != RegSet->MipsRegLo(count)) {
					Section->MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				} else if (Section->Is64Bit(count) && Section->MipsReg(count) != RegSet->MipsReg(count)) {
					Section->MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
				}
			}
		}
	}
	Section->RegStart = Section->RegWorking;
#endif
}

CCodeSection * CRecompiler::ExistingSection(CCodeSection * StartSection, DWORD Addr, DWORD Test) 
{
_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (StartSection == NULL) { return NULL; }
	if (StartSection->StartPC == Addr && StartSection->LinkAllowed) 
	{ 
		return StartSection; 
	}
	if (StartSection->Test == Test) { return NULL; }
	StartSection->Test = Test;

	CCodeSection * Section = ExistingSection(StartSection->m_JumpSection,Addr,Test);
	if (Section != NULL) { return Section; }
	Section = ExistingSection(StartSection->m_ContinueSection,Addr,Test);
	if (Section != NULL) { return Section; }
#endif
	return NULL;
}

bool CRecompiler::CreateSectionLinkage (CCodeSection * Section) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	InheritConstants(Section);

	if (!FillSectionInfo(Section,NORMAL))
	{
		return false;
	}
	
	CCodeSection ** TargetSection[2];
	CJumpInfo * JumpInfo[2];
	if (Section->m_Jump.TargetPC < Section->m_Cont.TargetPC) {
		TargetSection[0] = (CCodeSection **)&Section->m_JumpSection;
		TargetSection[1] = (CCodeSection **)&Section->m_ContinueSection;
		JumpInfo[0] = &Section->m_Jump;
		JumpInfo[1] = &Section->m_Cont;	
	} else {
		TargetSection[0] = (CCodeSection **)&Section->m_ContinueSection;
		TargetSection[1] = (CCodeSection **)&Section->m_JumpSection;
		JumpInfo[0] = &Section->m_Cont;	
		JumpInfo[1] = &Section->m_Jump;
	}

	CCodeBlock * BlockInfo = Section->BlockInfo;

	for (int count = 0; count < 2; count ++) 
	{
		if (JumpInfo[count]->TargetPC == (DWORD)-1 || *TargetSection[count] != NULL) 
		{
			continue;
		}
		if (!JumpInfo[count]->DoneDelaySlot)
		{
			Section->m_Jump.RegSet = Section->RegWorking;

			//this is a special delay slot section
			BlockInfo->NoOfSections += 1;
			*TargetSection[count] = new CCodeSection(BlockInfo,CRecompilerOps::CompilePC() + 4,BlockInfo->NoOfSections);
			(*TargetSection[count])->AddParent(Section);
			(*TargetSection[count])->LinkAllowed = false;
			InheritConstants((*TargetSection[count]));

			if (!FillSectionInfo((*TargetSection[count]),END_BLOCK))
			{
				return false;
			}
			(*TargetSection[count])->m_Jump.TargetPC = -1;
			(*TargetSection[count])->m_Cont.TargetPC = JumpInfo[count]->TargetPC;
			(*TargetSection[count])->m_Cont.FallThrough = true;
			(*TargetSection[count])->m_Cont.RegSet = (*TargetSection[count])->RegWorking;
			JumpInfo[count]->TargetPC = CRecompilerOps::CompilePC() + 4;

			//Create the section that joins with that block
			(*TargetSection[count])->m_ContinueSection = ExistingSection(&BlockInfo->ParentSection,(*TargetSection[count])->m_Cont.TargetPC,CCodeSection::GetNewTestValue());
			if ((*TargetSection[count])->m_ContinueSection == NULL) {
				BlockInfo->NoOfSections += 1;
				(*TargetSection[count])->m_ContinueSection = new CCodeSection(BlockInfo,(*TargetSection[count])->m_Cont.TargetPC,BlockInfo->NoOfSections);
				(*TargetSection[count])->m_ContinueSection->AddParent((*TargetSection[count]));
				CreateSectionLinkage((*TargetSection[count])->m_ContinueSection);
			} else {
				(*TargetSection[count])->m_ContinueSection->AddParent((*TargetSection[count]));
			}
		} else { 	
			*TargetSection[count] = ExistingSection(&BlockInfo->ParentSection,JumpInfo[count]->TargetPC,CCodeSection::GetNewTestValue());
			if (*TargetSection[count] == NULL) {
				BlockInfo->NoOfSections += 1;
				*TargetSection[count] = new CCodeSection(BlockInfo,JumpInfo[count]->TargetPC,BlockInfo->NoOfSections);
				(*TargetSection[count])->AddParent(Section);
				CreateSectionLinkage(*TargetSection[count]);
			} else {
				(*TargetSection[count])->AddParent(Section);
			}
		}
	}
#endif
	return true;
}

bool CRecompiler::FillSectionInfo(CCodeSection * Section, STEP_TYPE StartStepType) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	OPCODE Command;

	if (Section->CompiledLocation != NULL) { return true; }
	CRecompilerOps::CompilePC()  = Section->StartPC;
	Section->RegWorking = Section->RegStart;
	NextInstruction = StartStepType;
	do {
		if (!_MMU->LW_VAddr(CRecompilerOps::CompilePC(), Command.Hex)) {
			DisplayError(GS(MSG_FAIL_LOAD_WORD));
			return false;
		}		
		switch (Command.op) {
		case R4300i_SPECIAL:
			switch (Command.funct) {
			case R4300i_SPECIAL_SLL: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) << Command.sa;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRL: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) >> Command.sa;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRA: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo_S(Command.rt) >> Command.sa;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SLLV: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) << (Section->MipsRegLo(Command.rs) & 0x1F);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRLV: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) >> (Section->MipsRegLo(Command.rs) & 0x1F);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SRAV: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo_S(Command.rt) >> (Section->MipsRegLo(Command.rs) & 0x1F);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_JR:				
				if (Section->IsConst(Command.rs)) {
					Section->m_Jump.TargetPC = Section->MipsRegLo(Command.rs);
				} else {
					Section->m_Jump.TargetPC = (DWORD)-1;
				}
				NextInstruction = DELAY_SLOT;
				break;
			case R4300i_SPECIAL_JALR: 
				Section->MipsRegLo(Opcode.rd) = CRecompilerOps::CompilePC() + 8;
				Section->MipsRegState(Opcode.rd) = CRegInfo::STATE_CONST_32;
				if (Section->IsConst(Command.rs)) {
					Section->m_Jump.TargetPC = Section->MipsRegLo(Command.rs);
				} else {
					Section->m_Jump.TargetPC = (DWORD)-1;
				}
				NextInstruction = DELAY_SLOT;
				break;
			case R4300i_SPECIAL_SYSCALL:
			case R4300i_SPECIAL_BREAK:
				NextInstruction = END_BLOCK;
				CRecompilerOps::CompilePC() -= 4;
				break;
			case R4300i_SPECIAL_MFHI: Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN; break;
			case R4300i_SPECIAL_MTHI: break;
			case R4300i_SPECIAL_MFLO: Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN; break;
			case R4300i_SPECIAL_MTLO: break;
			case R4300i_SPECIAL_DSLLV: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg(Command.rd) = Section->Is64Bit(Command.rt)?Section->MipsReg(Command.rt):(QWORD)Section->MipsRegLo_S(Command.rt) << (Section->MipsRegLo(Command.rs) & 0x3F);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRLV: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg(Command.rd) = Section->Is64Bit(Command.rt)?Section->MipsReg(Command.rt):(QWORD)Section->MipsRegLo_S(Command.rt) >> (Section->MipsRegLo(Command.rs) & 0x3F);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRAV: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg(Command.rd) = Section->Is64Bit(Command.rt)?Section->MipsReg_S(Command.rt):(_int64)Section->MipsRegLo_S(Command.rt) >> (Section->MipsRegLo(Command.rs) & 0x3F);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
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
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rs) + Section->MipsRegLo(Command.rt);
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SUB: 
			case R4300i_SPECIAL_SUBU: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rs) - Section->MipsRegLo(Command.rt);
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_AND: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					if (Section->Is64Bit(Command.rt) && Section->Is64Bit(Command.rs)) {
						Section->MipsReg(Command.rd) = Section->MipsReg(Command.rt) & Section->MipsReg(Command.rs);
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;					
					} else if (Section->Is64Bit(Command.rt) || Section->Is64Bit(Command.rs)) {
						if (Section->Is64Bit(Command.rt)) {
							Section->MipsReg(Command.rd) = Section->MipsReg(Command.rt) & Section->MipsRegLo(Command.rs);
						} else {
							Section->MipsReg(Command.rd) = Section->MipsRegLo(Command.rt) & Section->MipsReg(Command.rs);
						}						
						Section->MipsRegState(Command.rd) = CRegInfo::ConstantsType(Section->MipsReg(Command.rd));
					} else {
						Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) & Section->MipsRegLo(Command.rs);
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_OR: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					if (Section->Is64Bit(Command.rt) && Section->Is64Bit(Command.rs)) {
						Section->MipsReg(Command.rd) = Section->MipsReg(Command.rt) | Section->MipsReg(Command.rs);
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else if (Section->Is64Bit(Command.rt) || Section->Is64Bit(Command.rs)) {
						if (Section->Is64Bit(Command.rt)) {
							Section->MipsReg(Command.rd) = Section->MipsReg(Command.rt) | Section->MipsRegLo(Command.rs);
						} else {
							Section->MipsReg(Command.rd) = Section->MipsRegLo(Command.rt) | Section->MipsReg(Command.rs);
						}
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else {
						Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) | Section->MipsRegLo(Command.rs);
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_XOR: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					if (Section->Is64Bit(Command.rt) && Section->Is64Bit(Command.rs)) {
						Section->MipsReg(Command.rd) = Section->MipsReg(Command.rt) ^ Section->MipsReg(Command.rs);
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else if (Section->Is64Bit(Command.rt) || Section->Is64Bit(Command.rs)) {
						if (Section->Is64Bit(Command.rt)) {
							Section->MipsReg(Command.rd) = Section->MipsReg(Command.rt) ^ Section->MipsRegLo(Command.rs);
						} else {
							Section->MipsReg(Command.rd) = Section->MipsRegLo(Command.rt) ^ Section->MipsReg(Command.rs);
						}
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else {
						Section->MipsRegLo(Command.rd) = Section->MipsRegLo(Command.rt) ^ Section->MipsRegLo(Command.rs);
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_NOR: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					if (Section->Is64Bit(Command.rt) && Section->Is64Bit(Command.rs)) {
						Section->MipsReg(Command.rd) = ~(Section->MipsReg(Command.rt) | Section->MipsReg(Command.rs));
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else if (Section->Is64Bit(Command.rt) || Section->Is64Bit(Command.rs)) {
						if (Section->Is64Bit(Command.rt)) {
							Section->MipsReg(Command.rd) = ~(Section->MipsReg(Command.rt) | Section->MipsRegLo(Command.rs));
						} else {
							Section->MipsReg(Command.rd) = ~(Section->MipsRegLo(Command.rt) | Section->MipsReg(Command.rs));
						}
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					} else {
						Section->MipsRegLo(Command.rd) = ~(Section->MipsRegLo(Command.rt) | Section->MipsRegLo(Command.rs));
						Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					}
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SLT: 
				if (Command.rd == 0) { break; }
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					if (Section->Is64Bit(Command.rt) || Section->Is64Bit(Command.rs)) {
						if (Section->Is64Bit(Command.rt)) {
							Section->MipsRegLo(Command.rd) = (Section->MipsRegLo_S(Command.rs) < Section->MipsReg_S(Command.rt))?1:0;
						} else {
							Section->MipsRegLo(Command.rd) = (Section->MipsReg_S(Command.rs) < Section->MipsRegLo_S(Command.rt))?1:0;
						}
					} else {
						Section->MipsRegLo(Command.rd) = (Section->MipsRegLo_S(Command.rs) < Section->MipsRegLo_S(Command.rt))?1:0;
					}
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_SLTU: 
				if (Command.rd == 0) { break; }
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					if (Section->Is64Bit(Command.rt) || Section->Is64Bit(Command.rs)) {
						if (Section->Is64Bit(Command.rt)) {
							Section->MipsRegLo(Command.rd) = (Section->MipsRegLo(Command.rs) < Section->MipsReg(Command.rt))?1:0;
						} else {
							Section->MipsRegLo(Command.rd) = (Section->MipsReg(Command.rs) < Section->MipsRegLo(Command.rt))?1:0;
						}
					} else {
						Section->MipsRegLo(Command.rd) = (Section->MipsRegLo(Command.rs) < Section->MipsRegLo(Command.rt))?1:0;
					}
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DADD: 
			case R4300i_SPECIAL_DADDU: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsReg(Command.rd) = 
						Section->Is64Bit(Command.rs)?Section->MipsReg(Command.rs):(_int64)Section->MipsRegLo_S(Command.rs) +
						Section->Is64Bit(Command.rt)?Section->MipsReg(Command.rt):(_int64)Section->MipsRegLo_S(Command.rt);
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSUB: 
			case R4300i_SPECIAL_DSUBU: 
				if (Command.rd == 0) { break; }
				if (Section->InLoop && (Command.rt == Command.rd || Command.rs == Command.rd)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt) && Section->IsConst(Command.rs)) {
					Section->MipsReg(Command.rd) = 
						Section->Is64Bit(Command.rs)?Section->MipsReg(Command.rs):(_int64)Section->MipsRegLo_S(Command.rs) -
						Section->Is64Bit(Command.rt)?Section->MipsReg(Command.rt):(_int64)Section->MipsRegLo_S(Command.rt);
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSLL:
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg(Command.rd) = Section->Is64Bit(Command.rt)?Section->MipsReg(Command.rt):(_int64)Section->MipsRegLo_S(Command.rt) << Command.sa;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRL:
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg(Command.rd) = Section->Is64Bit(Command.rt)?Section->MipsReg(Command.rt):(QWORD)Section->MipsRegLo_S(Command.rt) >> Command.sa;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRA:
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg_S(Command.rd) = Section->Is64Bit(Command.rt)?Section->MipsReg_S(Command.rt):(_int64)Section->MipsRegLo_S(Command.rt) >> Command.sa;
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSLL32:
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_64;
					Section->MipsReg(Command.rd) = Section->MipsRegLo(Command.rt) << (Command.sa + 32);
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRL32:
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = (DWORD)(Section->MipsReg(Command.rt) >> (Command.sa + 32));
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			case R4300i_SPECIAL_DSRA32:
				if (Command.rd == 0) { break; }
				if (Section->InLoop && Command.rt == Command.rd) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;	
				}
				if (Section->IsConst(Command.rt)) {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_CONST_32;
					Section->MipsRegLo(Command.rd) = (DWORD)(Section->MipsReg_S(Command.rt) >> (Command.sa + 32));
				} else {
					Section->MipsRegState(Command.rd) = CRegInfo::STATE_UNKNOWN;
				}
				break;
			default:
#ifndef EXTERNAL_RELEASE
				if (Command.Hex == 0x00000001) { break; }
				DisplayError("Unhandled R4300i OpCode in FillSectionInfo 5\n%s",
					R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()));
#endif
				NextInstruction = END_BLOCK;
				CRecompilerOps::CompilePC() -= 4;
			}
			break;
		case R4300i_REGIMM:
			switch (Command.rt) {
			case R4300i_REGIMM_BLTZ:
			case R4300i_REGIMM_BGEZ:
				NextInstruction = DELAY_SLOT;
				Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
				Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
				if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
					if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,0)) {
						Section->m_Jump.PermLoop = true;
					}
				} 
				break;
			case R4300i_REGIMM_BLTZL:
			case R4300i_REGIMM_BGEZL:
				NextInstruction = LIKELY_DELAY_SLOT;
				Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
				Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
				if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) { 
					if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,0)) {
						Section->m_Jump.PermLoop = true;
					}
				} 
				break;
			case R4300i_REGIMM_BLTZAL:
				Section->MipsRegLo(31) = CRecompilerOps::CompilePC() + 8;
				Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
				Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
				Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
				if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) { 
					if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,0)) {
						Section->m_Jump.PermLoop = true;
					}
				} 
				break;
			case R4300i_REGIMM_BGEZAL:
				NextInstruction = DELAY_SLOT;
				if (Section->IsConst(Command.rs)) 
				{
					__int64 Value;
					if (Section->Is32Bit(Command.rs))
					{
						Value = Section->MipsRegLo_S(Command.rs);
					} else {
						Value = Section->MipsReg_S(Command.rs);
					}
					if (Value >= 0) {
						Section->MipsRegLo(31) = CRecompilerOps::CompilePC() + 8;
						Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
						Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
						if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
							if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),31,0)) {
								Section->m_Jump.PermLoop = true;
							}
						} 
						break;
					}
				} 

				
				Section->MipsRegLo(31) = CRecompilerOps::CompilePC() + 8;
				Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
				Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
				Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
				if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) { 
					if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,0)) {
						Section->m_Jump.PermLoop = true;
					}
				} 
				break;
			default:
#ifndef EXTERNAL_RELEASE
				if (Command.Hex == 0x0407000D) { break; }
				DisplayError("Unhandled R4300i OpCode in FillSectionInfo 4\n%s",
					R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()));
#endif
				NextInstruction = END_BLOCK;
				CRecompilerOps::CompilePC() -= 4;
			}
			break;
		case R4300i_JAL: 
			NextInstruction = DELAY_SLOT;
			Section->MipsRegLo(31) = CRecompilerOps::CompilePC() + 8;
			Section->MipsRegState(31) = CRegInfo::STATE_CONST_32;
			Section->m_Jump.TargetPC = (CRecompilerOps::CompilePC() & 0xF0000000) + (Command.target << 2);
			if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),31,0)) {
					Section->m_Jump.PermLoop = true;
				}
			} 
			break;
		case R4300i_J: 
			NextInstruction = DELAY_SLOT;
			Section->m_Jump.TargetPC = (CRecompilerOps::CompilePC() & 0xF0000000) + (Command.target << 2);
			if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) { Section->m_Jump.PermLoop = true; } 
			break;
		case R4300i_BEQ: 
			NextInstruction = DELAY_SLOT;
			Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
			Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
			if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,Command.rt)) {
					Section->m_Jump.PermLoop = true;
				}
			} 
			if (Section->IsConst(Command.rs) && Section->IsConst(Command.rt)) 
			{
				__int64 Value1, Value2;
				if (Section->Is32Bit(Command.rs))
				{
					Value1 = Section->MipsRegLo_S(Command.rs);
				} else {
					Value1 = Section->MipsReg_S(Command.rs);
				}
				if (Section->Is32Bit(Command.rt))
				{
					Value2 = Section->MipsRegLo_S(Command.rt);
				} else {
					Value2 = Section->MipsReg_S(Command.rt);
				}
				if (Value1 == Value2) 
				{
					Section->m_Cont.TargetPC = -1;
				}
			} 
			break;
		case R4300i_BNE: 
		case R4300i_BLEZ: 
		case R4300i_BGTZ: 
			NextInstruction = DELAY_SLOT;
			Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
			Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
			if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,Command.rt)) {
					Section->m_Jump.PermLoop = true;
				}
			} 
			break;
		case R4300i_ADDI: 
		case R4300i_ADDIU: 
			if (Command.rt == 0) { break; }
			if (Section->InLoop && Command.rs == Command.rt) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (Section->IsConst(Command.rs)) { 
				Section->MipsRegLo(Command.rt) = Section->MipsRegLo(Command.rs) + (short)Command.immediate;
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_SLTI: 
			if (Command.rt == 0) { break; }
			if (Section->IsConst(Command.rs)) { 
				if (Section->Is64Bit(Command.rs)) {
					Section->MipsRegLo(Command.rt) = (Section->MipsReg_S(Command.rs) < (_int64)((short)Command.immediate))?1:0;
				} else {
					Section->MipsRegLo(Command.rt) = (Section->MipsRegLo_S(Command.rs) < (int)((short)Command.immediate))?1:0;
				}
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_SLTIU: 
			if (Command.rt == 0) { break; }
			if (Section->IsConst(Command.rs)) { 
				if (Section->Is64Bit(Command.rs)) {
					Section->MipsRegLo(Command.rt) = (Section->MipsReg(Command.rs) < (unsigned _int64)((short)Command.immediate))?1:0;
				} else {
					Section->MipsRegLo(Command.rt) = (Section->MipsRegLo(Command.rs) < (DWORD)((short)Command.immediate))?1:0;
				}
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_LUI: 
			if (Command.rt == 0) { break; }
			Section->MipsRegLo(Command.rt) = ((short)Command.offset << 16);
			Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
			break;
		case R4300i_ANDI: 
			if (Command.rt == 0) { break; }
			if (Section->InLoop && Command.rs == Command.rt) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (Section->IsConst(Command.rs)) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
				Section->MipsRegLo(Command.rt) = Section->MipsRegLo(Command.rs) & Command.immediate;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_ORI: 
			if (Command.rt == 0) { break; }
			if (Section->InLoop && Command.rs == Command.rt) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (Section->IsConst(Command.rs)) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
				Section->MipsRegLo(Command.rt) = Section->MipsRegLo(Command.rs) | Command.immediate;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_XORI: 
			if (Command.rt == 0) { break; }
			if (Section->InLoop && Command.rs == Command.rt) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (Section->IsConst(Command.rs)) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_32;
				Section->MipsRegLo(Command.rt) = Section->MipsRegLo(Command.rs) ^ Command.immediate;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			}
			break;
		case R4300i_CP0:
			switch (Command.rs) {
			case R4300i_COP0_MF:
				if (Command.rt == 0) { break; }
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
				break;
			case R4300i_COP0_MT: break;
			default:
				if ( (Command.rs & 0x10 ) != 0 ) {
					switch( Command.funct ) {
					case R4300i_COP0_CO_TLBR: break;
					case R4300i_COP0_CO_TLBWI: break;
					case R4300i_COP0_CO_TLBWR: break;
					case R4300i_COP0_CO_TLBP: break;
					case R4300i_COP0_CO_ERET: NextInstruction = END_BLOCK; break;
					default:
#ifndef EXTERNAL_RELEASE
						DisplayError("Unhandled R4300i OpCode in FillSectionInfo\n%s",
							R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()));
#endif
						NextInstruction = END_BLOCK;
						CRecompilerOps::CompilePC() -= 4;
					}
				} else {
#ifndef EXTERNAL_RELEASE
					DisplayError("Unhandled R4300i OpCode in FillSectionInfo 3\n%s",
						R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()));
#endif
					NextInstruction = END_BLOCK;
					CRecompilerOps::CompilePC() -= 4;
				}
			}
			break;
		case R4300i_CP1:
			switch (Command.fmt) {
			case R4300i_COP1_CF:
			case R4300i_COP1_MF:
			case R4300i_COP1_DMF:
				if (Command.rt == 0) { break; }
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
				break;
			case R4300i_COP1_BC:
				switch (Command.ft) {
				case R4300i_COP1_BC_BCFL:
				case R4300i_COP1_BC_BCTL:
					NextInstruction = LIKELY_DELAY_SLOT;
					Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
					Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
					if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
						int EffectDelaySlot;
						OPCODE NewCommand;

						if (!_MMU->LW_VAddr(CRecompilerOps::CompilePC() + 4, NewCommand.Hex)) {
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
							Section->m_Jump.PermLoop = true;
						}
					} 
					break;
				case R4300i_COP1_BC_BCF:
				case R4300i_COP1_BC_BCT:
					NextInstruction = DELAY_SLOT;
					Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
					Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
					if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
						int EffectDelaySlot;
						OPCODE NewCommand;

						if (!_MMU->LW_VAddr(CRecompilerOps::CompilePC() + 4, NewCommand.Hex)) {
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
							Section->m_Jump.PermLoop = true;
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
					R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()));
#endif
				NextInstruction = END_BLOCK;
				CRecompilerOps::CompilePC() -= 4;
			}
			break;
		case R4300i_BEQL: 
		case R4300i_BNEL: 
		case R4300i_BLEZL: 
		case R4300i_BGTZL: 
			NextInstruction = LIKELY_DELAY_SLOT;
			Section->m_Cont.TargetPC = CRecompilerOps::CompilePC() + 8;
			Section->m_Jump.TargetPC = CRecompilerOps::CompilePC() + ((short)Command.offset << 2) + 4;
			if (CRecompilerOps::CompilePC() == Section->m_Jump.TargetPC) {
				if (!DelaySlotEffectsCompare(CRecompilerOps::CompilePC(),Command.rs,Command.rt)) {
					Section->m_Jump.PermLoop = true;
				}
			} 
			break;
		case R4300i_DADDI: 
		case R4300i_DADDIU: 
			if (Command.rt == 0) { break; }
			if (Section->InLoop && Command.rs == Command.rt) {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;	
			}
			if (Section->IsConst(Command.rs)) { 
				if (Section->Is64Bit(Command.rs)) { 
					int imm32 = (short)Opcode.immediate;
					__int64 imm64 = imm32;										
					Section->MipsReg_S(Command.rt) = Section->MipsRegLo_S(Command.rs) + imm64;
				} else {
					Section->MipsReg_S(Command.rt) = Section->MipsRegLo_S(Command.rs) + (short)Command.immediate;
				}
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_CONST_64;
			} else {
				Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
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
			Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
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
			Section->MipsRegState(Command.rt) = CRegInfo::STATE_UNKNOWN;
			break;
		case R4300i_SDC1: break;
		case R4300i_SD: break;
		default:
			NextInstruction = END_BLOCK;
			CRecompilerOps::CompilePC() -= 4;
			if (Command.Hex == 0x7C1C97C0) { break; }
			if (Command.Hex == 0x7FFFFFFF) { break; }
			if (Command.Hex == 0xF1F3F5F7) { break; }
			if (Command.Hex == 0xC1200000) { break; }
			if (Command.Hex == 0x4C5A5353) { break; }
#ifndef EXTERNAL_RELEASE
			DisplayError("Unhandled R4300i OpCode in FillSectionInfo 1\n%s\n%X",
				R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()),Command.Hex);
#endif
		}

//		if (CRecompilerOps::CompilePC() == 0x8005E4B8) {
//CPU_Message("%X: %s %s = %d",CRecompilerOps::CompilePC(),R4300iOpcodeName(Command.Hex,CRecompilerOps::CompilePC()),
//			CRegName::GPR[8],Section->MipsRegState(8));
//_asm int 3
//		}
		switch (NextInstruction) {
		case NORMAL: 
			CRecompilerOps::CompilePC() += 4; 
			break;
		case DELAY_SLOT:
			NextInstruction = DELAY_SLOT_DONE;
			CRecompilerOps::CompilePC() += 4; 
			break;
		case LIKELY_DELAY_SLOT:
			if (Section->m_Cont.TargetPC == Section->m_Jump.TargetPC)
			{
				Section->m_Jump.RegSet = Section->RegWorking; 
				Section->m_Cont.DoneDelaySlot = false;
				Section->m_Cont.RegSet = Section->RegWorking;
				Section->m_Cont.DoneDelaySlot = true;
				NextInstruction = END_BLOCK;
			} else {
				Section->m_Cont.RegSet = Section->RegWorking;
				Section->m_Cont.DoneDelaySlot = true;
				NextInstruction = LIKELY_DELAY_SLOT_DONE;
				CRecompilerOps::CompilePC() += 4; 
			}
			break;
		case DELAY_SLOT_DONE:
			Section->m_Cont.RegSet = Section->RegWorking;
			Section->m_Jump.RegSet = Section->RegWorking; 
			Section->m_Cont.DoneDelaySlot = true;
			Section->m_Jump.DoneDelaySlot = true; 
			NextInstruction = END_BLOCK;
			break;
		case LIKELY_DELAY_SLOT_DONE:
			Section->m_Jump.RegSet = Section->RegWorking;
			Section->m_Jump.DoneDelaySlot = true; 
			NextInstruction = END_BLOCK;
			break;
		}		
		if ((CRecompilerOps::CompilePC() & 0xFFFFF000) != (Section->StartPC & 0xFFFFF000)) {
			if (NextInstruction != END_BLOCK && NextInstruction != NORMAL) {
			//	DisplayError("Branch running over delay slot ???\nNextInstruction == %d",NextInstruction);
				Section->m_Cont.TargetPC = (DWORD)-1;
				Section->m_Jump.TargetPC = (DWORD)-1;
			} 
			NextInstruction = END_BLOCK;
			CRecompilerOps::CompilePC() -= 4;
		}
	} while (NextInstruction != END_BLOCK);

	if (Section->m_Cont.TargetPC != (DWORD)-1) {
		if ((Section->m_Cont.TargetPC & 0xFFFFF000) != (Section->StartPC & 0xFFFFF000)) {
			Section->m_Cont.TargetPC = (DWORD)-1;
		}
	}
	if (Section->m_Jump.TargetPC != (DWORD)-1) {
		if (Section->m_Jump.TargetPC < Section->BlockInfo->StartVAddr)
		{
			Section->m_Jump.TargetPC = (DWORD)-1;
		}
		if ((Section->m_Jump.TargetPC & 0xFFFFF000) != (Section->StartPC & 0xFFFFF000)) {
			Section->m_Jump.TargetPC = (DWORD)-1;
		}
	}
#endif
	return true;
}

void CRecompiler::RecompilerMain_ChangeMemory ( void )
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Value, Addr;
	BYTE * Block;

	while(!EndEmulation()) {
		if (UseTlb) {
			if (!TranslateVaddr(PROGRAM_COUNTER, &Addr)) {
				DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
				NextInstruction = NORMAL;
				if (!TranslateVaddr(PROGRAM_COUNTER, &Addr)) {
#ifndef EXTERNAL_RELEASE
					DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
#endif
					ExitThread(0);
				}
			}
		} else {
			Addr = PROGRAM_COUNTER & 0x1FFFFFFF;
		}

		if (NextInstruction == DELAY_SLOT) {
			__try {
				Value = (DWORD)(*(DelaySlotTable + (Addr >> 12)));
			} __except(EXCEPTION_EXECUTE_HANDLER) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Executing Delay Slot from non maped space\nPROGRAM_COUNTER = 0x%X",PROGRAM_COUNTER);
#endif
				ExitThread(0);
			}
			if ( (Value >> 16) == 0x7C7C) {
				DWORD Index = (Value & 0xFFFF);
				Block = (BYTE *)OrigMem[Index].CompiledLocation;
				if (OrigMem[Index].PAddr != Addr) { Block = NULL; }
				if (OrigMem[Index].VAddr != PROGRAM_COUNTER) { Block = NULL; }
				if (Index >= TargetIndex) { Block = NULL; }
			} else {
				Block = NULL;
			}						
			if (Block == NULL) {
				DWORD MemValue;

				Block = CompileDelaySlot();
				Value = 0x7C7C0000;
				Value += (WORD)(TargetIndex);
				MemValue = *(DWORD *)(RDRAM + Addr);
				if ( (MemValue >> 16) == 0x7C7C) {
					MemValue = OrigMem[(MemValue & 0xFFFF)].OriginalValue;
				}
				OrigMem[(WORD)(TargetIndex)].OriginalValue = MemValue;
				OrigMem[(WORD)(TargetIndex)].CompiledLocation = Block;
				OrigMem[(WORD)(TargetIndex)].PAddr = Addr;
				OrigMem[(WORD)(TargetIndex)].VAddr = PROGRAM_COUNTER;
				TargetIndex += 1;
				*(DelaySlotTable + (Addr >> 12)) = (void *)Value;
				NextInstruction = NORMAL;
			}
			_asm {
				pushad
				call Block
				popad
			}
			continue;
		}

		__try {
			Value = *(DWORD *)(RDRAM + Addr);
			if ( (Value >> 16) == 0x7C7C) {
				DWORD Index = (Value & 0xFFFF);
				Block = (BYTE *)OrigMem[Index].CompiledLocation;						
				if (OrigMem[Index].PAddr != Addr) { Block = NULL; }
				if (OrigMem[Index].VAddr != PROGRAM_COUNTER) { Block = NULL; }
				if (Index >= TargetIndex) { Block = NULL; }
			} else {
				Block = NULL;
			}
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			DisplayError(GS(MSG_NONMAPPED_SPACE));
			ExitThread(0);
		}
						
		if (Block == NULL) {
			DWORD MemValue;

			__try {
				Block = Compiler4300iBlock();
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				ResetRecompCode();
				Block = Compiler4300iBlock();
			}
			if (EndEmulation())
			{
				continue;
			}
			if (TargetIndex == MaxOrigMem) {
				ResetRecompCode();
				continue;
			}
			Value = 0x7C7C0000;
			Value += (WORD)(TargetIndex);
			MemValue = *(DWORD *)(RDRAM + Addr);
			if ( (MemValue >> 16) == 0x7C7C) {
				MemValue = OrigMem[(MemValue & 0xFFFF)].OriginalValue;
			}
			OrigMem[(WORD)(TargetIndex)].OriginalValue = MemValue;
			OrigMem[(WORD)(TargetIndex)].CompiledLocation = Block;
			OrigMem[(WORD)(TargetIndex)].PAddr = Addr;					
			OrigMem[(WORD)(TargetIndex)].VAddr = PROGRAM_COUNTER;
			TargetIndex += 1;
			*(DWORD *)(RDRAM + Addr) = Value;					
			NextInstruction = NORMAL;
		}
		if (Profiling && IndvidualBlock) {
			static DWORD ProfAddress = 0;

			/*if ((PROGRAM_COUNTER & ~0xFFF) != ProfAddress) {
				char Label[100];

				ProfAddress = PROGRAM_COUNTER & ~0xFFF;
				sprintf(Label,"PC: %X to %X",ProfAddress,ProfAddress+ 0xFFC);
				StartTimer(Label);				
			}*/
			/*if (PROGRAM_COUNTER >= 0x800DD000 && PROGRAM_COUNTER <= 0x800DDFFC) {
				char Label[100];
				sprintf(Label,"PC: %X   Block: %X",PROGRAM_COUNTER,Block);
				StartTimer(Label);				
			}*/
//				} else 	if ((Profiling || ShowCPUPer) && ProfilingLabel[0] == 0) { 
//					StartTimer("r4300i Running"); 
		}
		_asm {
			pushad
			call Block
			popad
		}
	} // end for(;;)
#endif
}

CCompiledFunc * CRecompiler::CompilerCode ( void )
{
	CheckRecompMem();

	DWORD pAddr;
	if (!_TransVaddr->TranslateVaddr(*_PROGRAM_COUNTER,pAddr))
	{
		WriteTraceF(TraceError,"CRecompiler::CompilerCode: Failed to translate %X",*_PROGRAM_COUNTER);
		return NULL;
	}

	DWORD StartTime = timeGetTime();
	WriteTraceF(TraceRecompiler,"Compile Block-Start: Program Counter: %X pAddr: %X",*_PROGRAM_COUNTER,pAddr);


	CCodeBlock CodeBlock(*_PROGRAM_COUNTER, RecompPos(),false);
	if (!CodeBlock.Compile())
	{
		return NULL;
	}
	
	CCompiledFunc * Func = new CCompiledFunc(CodeBlock);
	m_Functions.push_back(Func);
	return Func;
}


bool CRecompiler::ClearRecompCode_Phys(DWORD Address, int length, REMOVE_REASON Reason ) {
	WriteTraceF(TraceError,"CRecompiler::ClearRecompCode_Phys Not Implemented (Address: %X, Length: %d Reason: %d)",Address,length,Reason);

	bool Result = true;
#ifdef tofix
	if (!ClearRecompCode_Virt(Address + 0x80000000,length,Reason)) { Result = false; }
	if (!ClearRecompCode_Virt(Address + 0xA0000000,length,Reason)) { Result = false; }

	if (g_UseTlb)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		DWORD VAddr, Index = 0;
		while (_TLB->PAddrToVAddr(Address,VAddr,Index))
		{
			WriteTraceF(TraceRecompiler,"ClearRecompCode Vaddr %X  len: %d",VAddr,length);
			if (!ClearRecompCode_Virt(VAddr,length,Reason))
			{
				Result = false; 
			}
		}
#endif
	}
	if (LookUpMode() == FuncFind_PhysicalLookup) 
	{
		WriteTraceF(TraceRecompiler,"Reseting Jump Table, Addr: %X  len: %d",Address,((length + 3) & ~3));
		memset((BYTE *)JumpTable + Address,0,((length + 3) & ~3));
	}

#endif
	return Result;
}

void CRecompiler::ClearRecompCode_Virt(DWORD Address, int length,REMOVE_REASON Reason ) 
{
	switch (LookUpMode())
	{
	case FuncFind_VirtualLookup:
		{
			DWORD AddressIndex = Address >> 0xC;
			BYTE ** DelaySlotFuncs = DelaySlotTable();

			if ((Address & 0xFFC) == 0 && DelaySlotFuncs[AddressIndex] != NULL)
			{
				DelaySlotFuncs[AddressIndex] = NULL;
			}

			DWORD WriteStart = (Address & 0xFFC);
			DWORD DataInBlock =  0x1000 - WriteStart;	
			int DataToWrite = length < DataInBlock ? length : DataInBlock;
			int DataLeft = length - DataToWrite;

			PCCompiledFunc_TABLE & table = FunctionTable()[AddressIndex];
			if (table)
			{
				memset(((BYTE *)&table[0]) + WriteStart,0,DataToWrite);
			}
			
			if (DataLeft > 0)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
		}
		break;
	}
	WriteTraceF(TraceError,"CRecompiler::ClearRecompCode_Virt Not Implemented (Address: %X, Length: %d Reason: %d)",Address,length,Reason);

/*	CCompiledFunc * info; 
	do 
	{
		info = m_Functions.FindFunction(Address,length);
		if (info)
		{
			RemoveFunction(info,false,Reason);
		}
	} while (info != NULL);
	do 
	{
		info = m_FunctionsDelaySlot.FindFunction(Address,length);
		if (info)
		{
			RemoveFunction(info,true,Reason);
		}
	} while (info != NULL);

	if (bSMM_Protect())
	{
		DWORD Start = Address  & ~0xFFF;
		info = m_Functions.FindFunction(Start,0xFFF);
		if (info) 
		{
			WriteTraceF(TraceDebug,"Function exists at %X End: %X",info->VStartPC(),info->VEndPC());
			return false; 
		}
		info = m_FunctionsDelaySlot.FindFunction(Start,0xFFF);
		if (info) 
		{
			WriteTraceF(TraceDebug,"Delay function exists at %X End: %X",info->VStartPC(),info->VEndPC());
			return false; 
		}
		return true;
	}*/
}
