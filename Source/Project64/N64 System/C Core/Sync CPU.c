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
#ifdef todelete

#include <Windows.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "debugger.h"
#include "plugin.h"

#define MaxMemory		2000

void StartErrorLog ( void );
void StopErrorLog  ( void );
void WriteSyncMemoryLineDump ( char * Label, BYTE * Memory );

DWORD CurrentBlock, *TLB_SyncReadMap, *TLB_SyncWriteMap, * MemAddrUsed[2] = { NULL,NULL };
int SyncNextInstruction, SyncJumpToLocation;
HANDLE hErrorLogFile = NULL;
N64_REGISTERS SyncRegisters;
int MemAddrUsedCount[2];
BYTE * SyncMemory;

#ifdef toremove
//TLB
//FASTTLB SyncFastTlb[64];
//TLB SyncTlb[32];
#endif

int Sync_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP);

void AllocateSyncMemory ( void ) {
	DWORD * TempReadMap, *TempWriteMap;
	BYTE * TempMemPtr;
	FreeSyncMemory();

	if(SyncMemory==NULL) {  
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}
	
	if(VirtualAlloc(SyncMemory, RdramSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}

	if(VirtualAlloc(SyncMemory + 0x04000000, 0x2000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}

	TLB_SyncReadMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (TLB_SyncReadMap == NULL) {
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}

	TLB_SyncWriteMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (TLB_SyncWriteMap == NULL) {
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}

	MemAddrUsed[0] = (DWORD *)malloc(MaxMemory * sizeof(DWORD *));
	MemAddrUsed[1] = (DWORD *)malloc(MaxMemory * sizeof(DWORD *));

	TempReadMap = TLB_ReadMap;
	TLB_ReadMap	= TLB_SyncReadMap;
	TLB_SyncReadMap = TempReadMap;
	
	TempWriteMap = TLB_WriteMap;
	TLB_WriteMap	= TLB_SyncWriteMap;
	TLB_SyncWriteMap = TempWriteMap;
	
	TempMemPtr = N64MEM;
	N64MEM = SyncMemory;
	RDRAM = (unsigned char *)(N64MEM);
	DMEM  = (unsigned char *)(N64MEM+0x04000000);
	IMEM  = (unsigned char *)(N64MEM+0x04001000);
	SyncMemory = TempMemPtr;
	
	InitilizeTLB();
	memcpy(SyncFastTlb,FastTlb,sizeof(FastTlb));
	memcpy(SyncTlb,tlb,sizeof(tlb));
}

void __cdecl Error_Message (char * Message, ...) {
	DWORD dwWritten;
	char Msg[400];
	va_list ap;
	
	if (hErrorLogFile == NULL) { StartErrorLog(); }
	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	
	strcat(Msg,"\r\n");

	WriteFile( hErrorLogFile,Msg,strlen(Msg),&dwWritten,NULL );
}

void FreeSyncMemory (void) {
	if (TLB_SyncReadMap) {
		VirtualFree( TLB_SyncReadMap, 0 , MEM_RELEASE);
		TLB_SyncReadMap = NULL;
	}
	if (TLB_SyncWriteMap) {
		VirtualFree( TLB_SyncWriteMap, 0 , MEM_RELEASE);
		TLB_SyncWriteMap = NULL;
	}
	if (MemAddrUsed[0]) { free(MemAddrUsed[0]); }
	MemAddrUsed[0] = NULL;
	if (MemAddrUsed[1]) { free(MemAddrUsed[1]); }
	MemAddrUsed[1] = NULL;
}

void ProtectMemory (void) {
	DWORD OldProtect;

	VirtualProtect(N64MEM,RdramSize,PAGE_READONLY,&OldProtect);
	VirtualProtect(N64MEM + 0x04000000,0x2000,PAGE_READONLY,&OldProtect);
}

void ReInitializeRSP (void) {
#ifdef hhhh	
	RSP_INFO_1_0 RspInfo10;
	RSP_INFO_1_1 RspInfo11;

	RspInfo10.CheckInterrupts = CheckInterrupts;
	RspInfo11.CheckInterrupts = CheckInterrupts;
	RspInfo10.ProcessDlist = ProcessDList;
	RspInfo11.ProcessDlist = ProcessDList;
	RspInfo10.ProcessAlist = ProcessAList;
	RspInfo11.ProcessAlist = ProcessAList;
	RspInfo10.ProcessRdpList = ProcessRDPList;
	RspInfo11.ProcessRdpList = ProcessRDPList;
	RspInfo11.ShowCFB = ShowCFB;

	RspInfo10.hInst = hInst;
	RspInfo11.hInst = hInst;
	RspInfo10.RDRAM = N64MEM;
	RspInfo11.RDRAM = N64MEM;
	RspInfo10.DMEM = DMEM;
	RspInfo11.DMEM = DMEM;
	RspInfo10.IMEM = IMEM;
	RspInfo11.IMEM = IMEM;
	RspInfo10.MemoryBswaped = FALSE;
	RspInfo11.MemoryBswaped = FALSE;

	RspInfo10.MI__INTR_REG = &MI_INTR_REG;
	RspInfo11.MI__INTR_REG = &MI_INTR_REG;
		
	RspInfo10.SP__MEM_ADDR_REG = &SP_MEM_ADDR_REG;
	RspInfo11.SP__MEM_ADDR_REG = &SP_MEM_ADDR_REG;
	RspInfo10.SP__DRAM_ADDR_REG = &SP_DRAM_ADDR_REG;
	RspInfo11.SP__DRAM_ADDR_REG = &SP_DRAM_ADDR_REG;
	RspInfo10.SP__RD_LEN_REG = &SP_RD_LEN_REG;
	RspInfo11.SP__RD_LEN_REG = &SP_RD_LEN_REG;
	RspInfo10.SP__WR_LEN_REG = &SP_WR_LEN_REG;
	RspInfo11.SP__WR_LEN_REG = &SP_WR_LEN_REG;
	RspInfo10.SP__STATUS_REG = &SP_STATUS_REG;
	RspInfo11.SP__STATUS_REG = &SP_STATUS_REG;
	RspInfo10.SP__DMA_FULL_REG = &SP_DMA_FULL_REG;
	RspInfo11.SP__DMA_FULL_REG = &SP_DMA_FULL_REG;
	RspInfo10.SP__DMA_BUSY_REG = &SP_DMA_BUSY_REG;
	RspInfo11.SP__DMA_BUSY_REG = &SP_DMA_BUSY_REG;
	RspInfo10.SP__PC_REG = &SP_PC_REG;
	RspInfo11.SP__PC_REG = &SP_PC_REG;
	RspInfo10.SP__SEMAPHORE_REG = &SP_SEMAPHORE_REG;
	RspInfo11.SP__SEMAPHORE_REG = &SP_SEMAPHORE_REG;
		
	RspInfo10.DPC__START_REG = &DPC_START_REG;
	RspInfo11.DPC__START_REG = &DPC_START_REG;
	RspInfo10.DPC__END_REG = &DPC_END_REG;
	RspInfo11.DPC__END_REG = &DPC_END_REG;
	RspInfo10.DPC__CURRENT_REG = &DPC_CURRENT_REG;
	RspInfo11.DPC__CURRENT_REG = &DPC_CURRENT_REG;
	RspInfo10.DPC__STATUS_REG = &DPC_STATUS_REG;
	RspInfo11.DPC__STATUS_REG = &DPC_STATUS_REG;
	RspInfo10.DPC__CLOCK_REG = &DPC_CLOCK_REG;
	RspInfo11.DPC__CLOCK_REG = &DPC_CLOCK_REG;
	RspInfo10.DPC__BUFBUSY_REG = &DPC_BUFBUSY_REG;
	RspInfo11.DPC__BUFBUSY_REG = &DPC_BUFBUSY_REG;
	RspInfo10.DPC__PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
	RspInfo11.DPC__PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
	RspInfo10.DPC__TMEM_REG = &DPC_TMEM_REG;
	RspInfo11.DPC__TMEM_REG = &DPC_TMEM_REG;
	
	if (RSPVersion == 0x0100) { InitiateRSP_1_0(RspInfo10, &RspTaskValue); }
	if (RSPVersion == 0x0101) { InitiateRSP_1_1(RspInfo11, &RspTaskValue); }
#else 
BreakPoint(__FILE__,__LINE__); 
#endif
}

void StartErrorLog (void) {
#ifdef ggg
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT], LogFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
   	_makepath( LogFileName, drive, dir, "CPU Error", "log" );
		
	hErrorLogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hErrorLogFile,0,NULL,FILE_BEGIN);
	
	Error_Message("=== Error Has occured !?! ===");
	Error_Message("Block Address: 0x%X",CurrentBlock);
	if (PROGRAM_COUNTER != Registers.PROGRAM_COUNTER) {
		Error_Message("interp PC: 0x%08X",PROGRAM_COUNTER);
		Error_Message("Recomp PC: 0x%08X",Registers.PROGRAM_COUNTER);
	} else {
		Error_Message("PC: 0x%08X",PROGRAM_COUNTER);
	}
	Error_Message("");
#else
	BreakPoint(__FILE__,__LINE__); 
#endif
}

#ifdef todelete
void StartSyncCPU (void ) { 
#ifdef hhh
	DWORD Addr;
	BYTE * Block;
#ifdef Log_x86Code
	Start_x86_Log();
#endif

	ResetRecompCode();
	AllocateSyncMemory();
	Registers.PROGRAM_COUNTER = PROGRAM_COUNTER;
	Registers.HI.DW = HI.DW;
	Registers.LO.DW = LO.DW;
	Registers.DMAUsed = DMAUsed;
	memcpy(&SyncRegisters,&Registers,sizeof(Registers));
	memcpy(N64MEM,SyncMemory,RdramSize);
	memcpy(N64MEM + 0x04000000,SyncMemory + 0x04000000,0x2000);
	ProtectMemory();
	SyncNextInstruction = NORMAL;
	SyncJumpToLocation = -1;
	__try {
		for (;;) {
			Addr = PROGRAM_COUNTER;
			if (UseTlb) {
				if (!TranslateVaddr(&Addr)) {
					DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
					NextInstruction = NORMAL;
					Addr = PROGRAM_COUNTER;
					if (!TranslateVaddr(&Addr)) {
						DisplayError("Failed to tranlate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
						ExitThread(0);
					}
				}
			} else {
				Addr &= 0x1FFFFFFF;
			}
			if (NextInstruction == DELAY_SLOT) {
				__try {
					Block = *(DelaySlotTable + (Addr >> 12));
				} __except(EXCEPTION_EXECUTE_HANDLER) {
					DisplayError("Executing Delay Slot from non maped space");
					ExitThread(0);
				}
				if (Block == NULL) {
					Block = CompileDelaySlot();
					*(DelaySlotTable + (Addr >> 12)) = Block;
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
				Block = *(JumpTable + (Addr >> 2));
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				DisplayError(GS(MSG_NONMAPPED_SPACE));
				ExitThread(0);
			}
			if (Block == NULL) {
				__try {
					Block = Compiler4300iBlock();
				} __except(EXCEPTION_EXECUTE_HANDLER) {
					DisplayError("Reset Recompiler Code");
					//ResetRecompCode();
					//Block = Compiler4300iBlock();
				}
				*(JumpTable + (Addr >> 2)) = Block;
				NextInstruction = NORMAL;
			}
			_asm {
				pushad
				call Block
				popad
			}
		}
	} __except( Sync_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
		ExitThread(0);
	}
#else
	BreakPoint(__FILE__,__LINE__); 
#endif
}
#endif

void StopErrorLog (void) {
	if (hErrorLogFile) {
		CloseHandle(hErrorLogFile);
		hErrorLogFile = NULL;
	}
}

void SwitchSyncRegisters (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	FASTTLB TempFastTlb[64];
	TLB Temptlb[32];
	BYTE * TempMemPtr;
	DWORD * Temp;

	if (GPR == Registers.GPR) {
		Registers.PROGRAM_COUNTER = PROGRAM_COUNTER;
		Registers.HI.DW = HI.DW;
		Registers.LO.DW = LO.DW;
		Registers.DMAUsed = DMAUsed;
		SetupRegisters(&SyncRegisters);
	} else {
		SyncRegisters.PROGRAM_COUNTER = PROGRAM_COUNTER;
		SyncRegisters.HI.DW = HI.DW;
		SyncRegisters.LO.DW = LO.DW;
		SyncRegisters.DMAUsed = DMAUsed;
		SetupRegisters(&Registers);
	}

	Temp = MemAddrUsed[1];
	MemAddrUsed[1] = MemAddrUsed[0];
	MemAddrUsed[0] = Temp;
	MemAddrUsedCount[1] = MemAddrUsedCount[0];	
	MemAddrUsedCount[0] = 0;
	
	memcpy(TempFastTlb,FastTlb,sizeof(FastTlb));
	memcpy(FastTlb,SyncFastTlb,sizeof(FastTlb));
	memcpy(SyncFastTlb,TempFastTlb,sizeof(FastTlb));
	
	memcpy(Temptlb,tlb,sizeof(tlb));
	memcpy(tlb,SyncTlb,sizeof(tlb));
	memcpy(SyncTlb,Temptlb,sizeof(tlb));

	Temp = TLB_ReadMap;
	TLB_ReadMap	= TLB_SyncReadMap;
	TLB_SyncReadMap = Temp;
	
	Temp = TLB_WriteMap;
	TLB_WriteMap	= TLB_SyncWriteMap;
	TLB_SyncWriteMap = Temp;
	
	TempMemPtr = N64MEM;
	N64MEM = SyncMemory;
	RDRAM = (unsigned char *)(N64MEM);
	DMEM  = (unsigned char *)(N64MEM+0x04000000);
	IMEM  = (unsigned char *)(N64MEM+0x04001000);
	SyncMemory = TempMemPtr;
	ReInitializeRSP();
	ProtectMemory();
#else
	BreakPoint(__FILE__,__LINE__); 
#endif
}

/*void SyncSystem (void) {	
#ifdef hhhh
	int count, i, error;

	error = FALSE;
	if (PROGRAM_COUNTER != Registers.PROGRAM_COUNTER) {
		error = TRUE;
		Error_Message("*** Program counter is not equal!!!");	
		Error_Message("");
	}

	//GPR
	for (count = 0; count < 32; count ++) {
		if (Registers.GPR[count].DW != SyncRegisters.GPR[count].DW) {
			error = TRUE;
			Error_Message("*** %s (GPR %d) is not equal!!!",GPR_Name[count],count);
			Error_Message("interp value: 0x%08X%08X",SyncRegisters.GPR[count].UW[1],SyncRegisters.GPR[count].UW[0]);
			Error_Message("Recomp value: 0x%08X%08X",Registers.GPR[count].UW[1],Registers.GPR[count].UW[0]);
			Error_Message("");
		}
	}
	
	//COP0
	FixRandomReg();
	for (count = 0; count < 33; count ++) {
		if (Registers.CP0[count] != SyncRegisters.CP0[count]) {
			error = TRUE;
			Error_Message("*** %s (CP0 %d) is not equal!!!",count == 32?"Fake cause":Cop0_Name[count],count);
			Error_Message("interp value: 0x%08X",SyncRegisters.CP0[count]);
			Error_Message("Recomp value: 0x%08X",Registers.CP0[count]);
			Error_Message("");
		}
	}

	//FPU
	for (count = 0; count < 32; count ++) {
		if (Registers.FPR[count].DW != SyncRegisters.FPR[count].DW) {
			error = TRUE;
			Error_Message("*** %s (FPR %d) is not equal!!!",FPR_Name[count],count);
			Error_Message("interp value: 0x%08X%08X",SyncRegisters.FPR[count].UW[1],SyncRegisters.FPR[count].UW[0]);
			Error_Message("Recomp value: 0x%08X%08X",Registers.FPR[count].UW[1],Registers.FPR[count].UW[0]);
			Error_Message("");
		}
	}

	if (Registers.FPCR[31] != SyncRegisters.FPCR[31]) {
		error = TRUE;
		Error_Message("*** %s is not equal!!!",FPR_Ctrl_Name[31]);
		Error_Message("interp value: 0x%08X",SyncRegisters.FPCR[31]);
		Error_Message("Recomp value: 0x%08X",Registers.FPCR[31]);
		Error_Message("");
	}
	//HI
	if (Registers.HI.DW != HI.DW) {
		error = TRUE;
		Error_Message("*** HI register is not equal!!!");	
		Error_Message("interp value: 0x%08X",HI.DW);
		Error_Message("Recomp value: 0x%08X",Registers.HI.DW);
		Error_Message("");
	}
	
	//LO
	if (Registers.LO.DW != LO.DW) {
		error = TRUE;
		Error_Message("*** LO register is not equal!!!");	
		Error_Message("interp value: 0x%08X",LO.DW);
		Error_Message("Recomp value: 0x%08X",Registers.LO.DW);
		Error_Message("");
	}

	//TLB
	for (count = 0; count < 32; count ++) {
		if (tlb[count].PageMask.Value != SyncTlb[count].PageMask.Value) {
			error = TRUE;
			Error_Message("*** tlb[%d].PageMask is not equal!!!",count);
			Error_Message("interp value: 0x%08X",SyncTlb[count].PageMask.Value);
			Error_Message("Recomp value: 0x%08X",tlb[count].PageMask.Value );
			Error_Message("");
		}
		if (tlb[count].EntryHi.Value != SyncTlb[count].EntryHi.Value) {
			error = TRUE;
			Error_Message("*** tlb[%d].EntryHi is not equal!!!",count);
			Error_Message("interp value: 0x%08X",SyncTlb[count].EntryHi.Value);
			Error_Message("Recomp value: 0x%08X",tlb[count].EntryHi.Value );
			Error_Message("");
		}
		if (tlb[count].EntryLo0.Value != SyncTlb[count].EntryLo0.Value) {
			error = TRUE;
			Error_Message("*** tlb[%d].PageMask is not equal!!!",count);
			Error_Message("interp value: 0x%08X",SyncTlb[count].EntryLo0.Value);
			Error_Message("Recomp value: 0x%08X",tlb[count].EntryLo0.Value );
			Error_Message("");
		}
		if (tlb[count].EntryLo1.Value != SyncTlb[count].EntryLo1.Value) {
			error = TRUE;
			Error_Message("*** tlb[%d].PageMask is not equal!!!",count);
			Error_Message("interp value: 0x%08X",SyncTlb[count].EntryLo1.Value);
			Error_Message("Recomp value: 0x%08X",tlb[count].EntryLo1.Value );
			Error_Message("");
		}
	}

	//Mips Interface
	for (count = 0; count < 4; count ++) {
		if (Registers.MI[count] != SyncRegisters.MI[count]) {
			error = TRUE;
			Error_Message("*** MI %d is not equal!!!",count);
			Error_Message("interp value: 0x%08X",SyncRegisters.MI[count]);
			Error_Message("Recomp value: 0x%08X",Registers.MI[count]);
			Error_Message("");
		}
	}

	//PI Interface
	for (count = 0; count < 12; count ++) {
		if (Registers.PI[count] != SyncRegisters.PI[count]) {
			error = TRUE;
			Error_Message("*** PI %d is not equal!!!",count);
			Error_Message("interp value: 0x%08X",SyncRegisters.PI[count]);
			Error_Message("Recomp value: 0x%08X",Registers.PI[count]);
			Error_Message("");
		}
	}
	
	//Memory
	for (count = 0; count < MemAddrUsedCount[0]; count ++) {
		int count2;
		
		for (count2 = 0; count2 < MemAddrUsedCount[1]; count2 ++) {
			if (MemAddrUsed[0][count] == MemAddrUsed[1][count2]) {
				MemAddrUsed[1][count2] = (DWORD)-1;
			}
		}		
	}
	for (i = 0; i < 2; i++) {
		for (count = 0; count < MemAddrUsedCount[i]; count ++) {
			DWORD count2;

			if (MemAddrUsed[i][count] == (DWORD)-1) { continue; }
			if (memcmp(N64MEM + MemAddrUsed[i][count], SyncMemory + MemAddrUsed[i][count],0xFFF) == 0) {
				continue;
			}
			error = TRUE;
			for (count2 = MemAddrUsed[i][count]; count2 < (MemAddrUsed[i][count] + 0x1000); count2 += 0x10) {
				if (memcmp(N64MEM + count2, SyncMemory + count2,0x10) == 0) { 	continue; }
				Error_Message("*** Memory (Address: %X) is not equal!!!",count2);
				WriteSyncMemoryLineDump("Interp",N64MEM + count2);
				WriteSyncMemoryLineDump("Recomp",SyncMemory + count2);
				Error_Message("");
			}
		}
	}

	if (error) {
		SwitchSyncRegisters();
		DisplayError("Sync Error has occured see log for details\n\nEmulation Has stoped");
		ExitThread(0);
	}
#else
	BreakPoint(__FILE__,__LINE__); 
#endif
}*/

void SyncToPC (void) {
	FixRandomReg();
	SyncSystem ();
	return;
#ifdef toremove	
	
	{
		int RecNextInstruction, RecJumpToLocation;

		//LogMessage("Recompiler (PC: %X  count: %X)",PROGRAM_COUNTER,CP0[9]);
		RecNextInstruction = NextInstruction;
		RecJumpToLocation = JumpToLocation;
		NextInstruction = SyncNextInstruction;
		JumpToLocation = SyncJumpToLocation;

		SwitchSyncRegisters();
		/*if ((DWORD)RecompPos > 0x609844D8) {
			while (PROGRAM_COUNTER != Registers.PROGRAM_COUNTER) {
				ExecuteInterpreterOpCode();
			}
		} else {
			while (CP0[9] != Registers.CP0[9]) {
				ExecuteInterpreterOpCode();
			}
		}*/
		while (CP0[9] != Registers.CP0[9]) {
			ExecuteInterpreterOpCode();
		}
		SyncRegisters.MI[2] = Registers.MI[2]; //MI_INTR_REG
		SyncRegisters.CP0[32] = Registers.CP0[32]; //FAKE_CAUSE_REGISTER
		SyncSystem ();
		SwitchSyncRegisters();

		SyncNextInstruction = NextInstruction;
		SyncJumpToLocation = JumpToLocation;
		NextInstruction = RecNextInstruction;
		JumpToLocation = RecJumpToLocation;
		
		MemAddrUsedCount[0] = 0;	
		MemAddrUsedCount[1] = 0;
	}
#endif
}

int Sync_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP) {
	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)N64MEM;
    EXCEPTION_RECORD exRec;

	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	exRec = *lpEP->ExceptionRecord;

    if ((int)((char *)lpEP->ExceptionRecord->ExceptionInformation[1] - N64MEM) < 0) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
    if ((int)((char *)lpEP->ExceptionRecord->ExceptionInformation[1] - N64MEM) > 0x1FFFFFFF) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	
	if (MemAddress < RdramSize || (MemAddress >= 0x4000000 && MemAddress < 0x4002000)) {
		DWORD OldProtect;

		VirtualProtect(N64MEM+(MemAddress & 0xFFFFF000),0xFFF,PAGE_READWRITE,&OldProtect);
		MemAddrUsed[0][MemAddrUsedCount[0]] = (MemAddress & 0xFFFFF000);
		MemAddrUsedCount[0] += 1;
		if (MemAddrUsedCount[0] == MaxMemory) {
			DisplayError("Used up all sync blocks ????");
			ExitThread(0);
		}
		if (CPU_Type != ModCode_ProtectedMemory) { return EXCEPTION_CONTINUE_EXECUTION; }
	}
	return r4300i_CPU_MemoryFilter(dwExptCode,lpEP);
}

void WriteSyncMemoryLineDump (char * Label, BYTE * Memory) {
 	char Hex[100], Ascii[30];
	DWORD count;

	memset(&Hex,0,sizeof(Hex));
	memset(&Ascii,0,sizeof(Ascii)); 

	for (count = 0; count < 0x10; count ++ ) {
		if ((count % 4) != 0 || count == 0) {
			sprintf(Hex,"%s %02X",Hex,Memory[count]);
		} else {
			sprintf(Hex,"%s - %02X",Hex,Memory[count]);
		}
		if (Memory[count] < 30 || Memory[count] > 127) {
			strcat(Ascii,".");
		} else {
			sprintf(Ascii,"%s%c",Ascii,Memory[count]);
		}
	}
	Error_Message("%s:%s    %s",Label,Hex,Ascii);
}

#endif