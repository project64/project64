#include "stdafx.h"

CRecompiler::CRecompiler(CProfiling & Profile, bool & EndEmulation ) :
	m_Profile(Profile),
	PROGRAM_COUNTER(_Reg->m_PROGRAM_COUNTER),
	m_EndEmulation(EndEmulation)
{
	ResetMemoryStackPos();
}

CRecompiler::~CRecompiler()
{
	ResetRecompCode();
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
	if (!CFunctionMap::AllocateMemory())
	{
		WriteTrace(TraceError,"CRecompiler::Run: CFunctionMap::AllocateMemory failed");
		return;
	}
	m_EndEmulation = false;

#ifdef tofix
	*g_MemoryStack = (DWORD)(RDRAM+(_GPR[29].W[0] & 0x1FFFFFFF));
#endif
	__try {
		if (LookUpMode() == FuncFind_VirtualLookup)
		{
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
			CInterpreterCPU::BuildCPU();
			if (bUseTlb())
			{
				if (bSMM_ValidFunc())
				{
					RecompilerMain_Lookup_validate_TLB();
				} else {
					RecompilerMain_Lookup_TLB();
				}
			} else {
				if (bSMM_ValidFunc())
				{
					RecompilerMain_Lookup_validate();
				} else {
					RecompilerMain_Lookup();
				}
			}
		}
	}
	__except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) 
	{
		_Notify->DisplayError(MSG_UNKNOWN_MEM_ACTION);
	}
}

void CRecompiler::RecompilerMain_VirtualTable ( void )
{
	bool & Done = m_EndEmulation;
	DWORD & PC = PROGRAM_COUNTER;

	while(!Done) 
	{
		PCCompiledFunc_TABLE & table = FunctionTable()[PC >> 0xC];
		DWORD TableEntry = (PC & 0xFFF) >> 2;
		if (table)
		{
			CCompiledFunc * info = table[TableEntry];
			if (info != NULL)
			{
				(info->Function())();
				continue;
			}
		}
		if (!_TransVaddr->ValidVaddr(PC)) 
		{
			_Reg->DoTLBMiss(false,PC);
			if (!_TransVaddr->ValidVaddr(PC)) 
			{
				DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PC);
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
				WriteTrace(TraceError,"CRecompiler::RecompilerMain_VirtualTable: failed to allocate PCCompiledFunc");
				_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
			}
			memset(table,0,sizeof(PCCompiledFunc) * (0x1000 >> 2));
			if (bSMM_Protect())
			{
				WriteTraceF(TraceError,"Create Table (%X): Index = %d",table, PC >> 0xC);
				_MMU->ProtectMemory(PC & ~0xFFF,PC | 0xFFF);
			}
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
						DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
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
				DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
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
				DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
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
	while(!m_EndEmulation) 
	{
		DWORD PhysicalAddr = PROGRAM_COUNTER & 0x1FFFFFFF;
		if (PhysicalAddr < RdramSize())
		{
			CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];
			if (info == NULL)
			{
				info = CompilerCode();
				if (info == NULL || m_EndEmulation)
				{
					break;
				}
				if (bSMM_Protect())
				{
					_MMU->ProtectMemory(PROGRAM_COUNTER & ~0xFFF,PROGRAM_COUNTER | 0xFFF);
				}
				JumpTable()[PhysicalAddr >> 2] = info;
			}
			(info->Function())();
		} else {
			DWORD opsExecuted = 0;

			while (_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= RdramSize())
			{
				CInterpreterCPU::ExecuteOps(CountPerOp());
				opsExecuted += CountPerOp();
			}

			if (_SyncSystem)
			{
				_System->UpdateSyncCPU(_SyncSystem,opsExecuted);
				_System->SyncCPU(_SyncSystem);
			}
		}
	}
	/*
	DWORD Addr;
	CCompiledFunc * Info;
	//const BYTE * Block;

	while(!m_EndEmulation) 
	{
		/*if (bUseTlb())
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			if (!_TLB->TranslateVaddr(PROGRAM_COUNTER, Addr))
			{
				DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
				NextInstruction = NORMAL;
				if (!TranslateVaddr(PROGRAM_COUNTER, &Addr)) {
					DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
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

void CRecompiler::RecompilerMain_Lookup_TLB( void )
{
	DWORD PhysicalAddr;

	while(!m_EndEmulation) 
	{
		if (!_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr))
		{
			_Reg->DoTLBMiss(false,PROGRAM_COUNTER);
			if (!_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr))
			{
				DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
				m_EndEmulation = true;
			}
			continue;
		}
		if (PhysicalAddr < RdramSize())
		{
			CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];

			if (info == NULL)
			{
				info = CompilerCode();
				if (info == NULL || m_EndEmulation)
				{
					break;
				}
				if (bSMM_Protect())
				{
					_MMU->ProtectMemory(PROGRAM_COUNTER & ~0xFFF,PROGRAM_COUNTER | 0xFFF);
				}
				JumpTable()[PhysicalAddr >> 2] = info;
			}
			(info->Function())();
		} else {
			DWORD opsExecuted = 0;

			while (_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= RdramSize())
			{
				CInterpreterCPU::ExecuteOps(CountPerOp());
				opsExecuted += CountPerOp();
			}

			if (_SyncSystem)
			{
				_System->UpdateSyncCPU(_SyncSystem,opsExecuted);
				_System->SyncCPU(_SyncSystem);
			}
		}
	}
}

void CRecompiler::RecompilerMain_Lookup_validate( void )
{
	while(!m_EndEmulation) 
	{
		DWORD PhysicalAddr = PROGRAM_COUNTER & 0x1FFFFFFF;
		if (PhysicalAddr < RdramSize())
		{
			CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];
			if (info == NULL)
			{
				info = CompilerCode();
				if (info == NULL || m_EndEmulation)
				{
					break;
				}
				if (bSMM_Protect())
				{
					_MMU->ProtectMemory(PROGRAM_COUNTER & ~0xFFF,PROGRAM_COUNTER | 0xFFF);
				}
				JumpTable()[PhysicalAddr >> 2] = info;
			} else {
				if (*(info->MemLocation(0)) != info->MemContents(0) ||
					*(info->MemLocation(1)) != info->MemContents(1))
				{
					ClearRecompCode_Virt((info->EnterPC() - 0x1000) & ~0xFFF,0x3000,Remove_ValidateFunc);
					info = NULL;
					continue;
				}
			}
			(info->Function())();
		} else {
			DWORD opsExecuted = 0;

			while (_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= RdramSize())
			{
				CInterpreterCPU::ExecuteOps(CountPerOp());
				opsExecuted += CountPerOp();
			}

			if (_SyncSystem)
			{
				_System->UpdateSyncCPU(_SyncSystem,opsExecuted);
				_System->SyncCPU(_SyncSystem);
			}
		}
	}
}

void CRecompiler::RecompilerMain_Lookup_validate_TLB( void )
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD PhysicalAddr;

	while(!m_EndEmulation) 
	{
		if (!_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr))
		{
			_Reg->DoTLBMiss(false,PROGRAM_COUNTER);
			if (!_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr))
			{
				DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
				m_EndEmulation = true;
			}
			continue;
		}
		if (PhysicalAddr < RdramSize())
		{
			CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];

			if (info == NULL)
			{
				info = CompilerCode();
				if (info == NULL || m_EndEmulation)
				{
					break;
				}
				if (bSMM_Protect())
				{
					_MMU->ProtectMemory(PROGRAM_COUNTER & ~0xFFF,PROGRAM_COUNTER | 0xFFF);
				}
				JumpTable()[PhysicalAddr >> 2] = info;
			}
			(info->Function())();
		} else {
			DWORD opsExecuted = 0;

			while (_TransVaddr->TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= RdramSize())
			{
				CInterpreterCPU::ExecuteOps(CountPerOp());
				opsExecuted += CountPerOp();
			}

			if (_SyncSystem)
			{
				_System->UpdateSyncCPU(_SyncSystem,opsExecuted);
				_System->SyncCPU(_SyncSystem);
			}
		}
	}
#endif
}

void CRecompiler::Reset()
{
	ResetRecompCode();
	ResetMemoryStackPos();
}

void CRecompiler::ResetRecompCode()
{
	CRecompMemory::Reset();
	CFunctionMap::Reset();

	for (CCompiledFuncList::iterator iter = m_Functions.begin(); iter != m_Functions.end(); iter++)
	{
		CCompiledFunc * Func = iter->second;
		while (Func != NULL)
		{
			CCompiledFunc * CurrentFunc = Func;
			Func = Func->Next();

			delete CurrentFunc;
		}
	}
	m_Functions.clear();
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
					DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
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
	DWORD pAddr = 0;
	if (!_TransVaddr->TranslateVaddr(PROGRAM_COUNTER,pAddr))
	{
		WriteTraceF(TraceError,"CRecompiler::CompilerCode: Failed to translate %X",PROGRAM_COUNTER);
		return NULL;
	}
	
	CCompiledFuncList::iterator iter = m_Functions.find(PROGRAM_COUNTER);
	if (iter != m_Functions.end())
	{
		for (CCompiledFunc * Func = iter->second; Func != NULL; Func = Func->Next())
		{
			DWORD PAddr;
			if (_TransVaddr->TranslateVaddr(Func->MinPC(),PAddr))
			{
				MD5Digest Hash;
				MD5(_MMU->Rdram() + PAddr,(Func->MaxPC() - Func->MinPC())+ 4).get_digest(Hash);
				if (memcmp(Hash.digest,Func->Hash().digest,sizeof(Hash.digest)) == 0)
				{
					return Func;
				}
			}			
		}
	}
	
	CheckRecompMem();

	DWORD StartTime = timeGetTime();
	WriteTraceF(TraceRecompiler,"Compile Block-Start: Program Counter: %X pAddr: %X",PROGRAM_COUNTER,pAddr);

	CCodeBlock CodeBlock(PROGRAM_COUNTER, RecompPos());
	if (!CodeBlock.Compile())
	{
		return NULL;
	}

	if (bShowRecompMemSize()) 
	{
		ShowMemUsed();
	}
	
	CCompiledFunc * Func = new CCompiledFunc(CodeBlock);
	CCompiledFuncList::_Pairib ret = m_Functions.insert(CCompiledFuncList::value_type(Func->EnterPC(),Func));
	if (ret.second == false)
	{
		Func->SetNext(ret.first->second->Next());
		ret.first->second->SetNext(Func);
		return Func;
	}
	return Func;
}


void CRecompiler::ClearRecompCode_Phys(DWORD Address, int length, REMOVE_REASON Reason ) {
	//WriteTraceF(TraceError,"CRecompiler::ClearRecompCode_Phys Not Implemented (Address: %X, Length: %d Reason: %d)",Address,length,Reason);

	if (LookUpMode() == FuncFind_VirtualLookup) 
	{
		ClearRecompCode_Virt(Address + 0x80000000,length,Reason);
		ClearRecompCode_Virt(Address + 0xA0000000,length,Reason);

		if (bUseTlb())
		{
			DWORD VAddr, Index = 0;
			while (_TLB->PAddrToVAddr(Address,VAddr,Index))
			{
				WriteTraceF(TraceRecompiler,"ClearRecompCode Vaddr %X  len: %d",VAddr,length);
				ClearRecompCode_Virt(VAddr,length,Reason);
			}
		}
	}
	else if (LookUpMode() == FuncFind_PhysicalLookup) 
	{
		WriteTraceF(TraceRecompiler,"Reseting Jump Table, Addr: %X  len: %d",Address,((length + 3) & ~3));
		memset((BYTE *)JumpTable() + Address,0,((length + 3) & ~3));
	}
}

void CRecompiler::ClearRecompCode_Virt(DWORD Address, int length,REMOVE_REASON Reason ) 
{
	//WriteTraceF(TraceError,"CRecompiler::ClearRecompCode_Virt Not Implemented (Address: %X, Length: %d Reason: %d)",Address,length,Reason);

	switch (LookUpMode())
	{
	case FuncFind_VirtualLookup:
		{
			DWORD AddressIndex = Address >> 0xC;
			DWORD WriteStart = (Address & 0xFFC);
			bool bUnProtect = false;
			length = ((length + 3) & ~0x3);

			BYTE ** DelaySlotFuncs = DelaySlotTable();

			if (WriteStart == 0 && DelaySlotFuncs[AddressIndex] != NULL)
			{
				DelaySlotFuncs[AddressIndex] = NULL;
				_MMU->UnProtectMemory(Address,Address+ 4);
			}

			int DataInBlock =  0x1000 - WriteStart;	
			int DataToWrite = length < DataInBlock ? length : DataInBlock;
			int DataLeft = length - DataToWrite;

			PCCompiledFunc_TABLE & table = FunctionTable()[AddressIndex];
			if (table)
			{
				WriteTraceF(TraceError,"Delete Table (%X): Index = %d",table, AddressIndex);
				delete table;
				table = NULL;
				_MMU->UnProtectMemory(Address,Address + length);
			}
			
			if (DataLeft > 0)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
		}
		break;
	case FuncFind_PhysicalLookup:
		{
			DWORD pAddr = 0;
			if (_TransVaddr->TranslateVaddr(Address,pAddr))
			{
				ClearRecompCode_Phys(pAddr,length,Reason);
			}
		}
		break;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
}

void CRecompiler::ResetMemoryStackPos( void ) 
{
	if (_Reg->m_GPR[29].UW[0] == 0)
	{
		m_MemoryStack = NULL;
		return;
	}
	if (_MMU == NULL || _Reg == NULL)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
	if (_Reg->m_GPR[29].UW[0] < 0x80000000 || _Reg->m_GPR[29].UW[0] >= 0xC0000000)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
	m_MemoryStack = (DWORD)(_MMU->Rdram() + (_Reg->m_GPR[29].UW[0] & 0x1FFFFFFF));
}
