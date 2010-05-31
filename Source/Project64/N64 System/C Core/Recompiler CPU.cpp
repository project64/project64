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
#ifdef tofix
#include <Windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "c core.h"
#include "main.h"
#include "C Global Variable.h"
#include "cpu.h"
#include "x86.h"
#include "debugger.h"
#include "plugin.h"
#include "Recompiler CPU.h"

N64_BLOCKS N64_Blocks;
DWORD TLBLoadAddress, TLBStoreAddress;

void FixRandomReg (void) {
	while ((int)_CP0[1] < (int)_CP0[6]) {
		_CP0[1] += 32 - _CP0[6];
	}
}



BOOL DisplaySectionInformation (BLOCK_SECTION * Section, DWORD ID, DWORD Test);
BLOCK_SECTION * ExistingSection(BLOCK_SECTION * StartSection, DWORD Addr, DWORD Test);
void _fastcall FillSectionInfo(BLOCK_SECTION * Section);
void _fastcall FixConstants ( BLOCK_SECTION * Section, DWORD Test,int * Changed );
BOOL GenerateX86Code (BLOCK_SECTION * Section, DWORD Test );
DWORD GetNewTestValue( void );
void _fastcall InheritConstants(BLOCK_SECTION * Section);
BOOL InheritParentInfo (BLOCK_SECTION * Section);
void _fastcall InitilzeSection(BLOCK_SECTION * Section, BLOCK_SECTION * Parent, DWORD StartAddr, DWORD ID);
void InitilizeRegSet(REG_INFO * RegSet);
BOOL IsAllParentLoops(BLOCK_SECTION * Section, BLOCK_SECTION * Parent, BOOL IgnoreIfCompiled, DWORD Test);
void MarkCodeBlock (DWORD PAddr);
void SyncRegState (BLOCK_SECTION * Section, REG_INFO * SyncTo);

DWORD TLBLoadAddress, TargetIndex;
TARGET_INFO * TargetInfo = NULL;
BLOCK_INFO BlockInfo;
ORIGINAL_MEMMARKER * OrigMem = NULL;


void InitilizeInitialCompilerVariable ( void)
{
	memset(&BlockInfo,0,sizeof(BlockInfo));
}

void _fastcall AddParent(BLOCK_SECTION * Section, BLOCK_SECTION * Parent){
	int NoOfParents, count;


	if (Section == NULL) { return; }
	if (Parent == NULL) {
		InitilizeRegSet(&Section->RegStart);
		memcpy(&Section->RegWorking,&Section->RegStart,sizeof(REG_INFO));
		return;
	}
	
	if (Section->ParentSection != NULL) {	
		for (NoOfParents = 0;Section->ParentSection[NoOfParents] != NULL;NoOfParents++) {
			if (Section->ParentSection[NoOfParents] == Parent) {
				return;
			}
		}
		for (NoOfParents = 0;Section->ParentSection[NoOfParents] != NULL;NoOfParents++);
		NoOfParents += 1;
	} else {
		NoOfParents = 1;
	}

	if (NoOfParents == 1) {
		Section->ParentSection = (void **)malloc((NoOfParents + 1)*sizeof(void *));
	} else {
		Section->ParentSection = (void **)realloc(Section->ParentSection,(NoOfParents + 1)*sizeof(void *));
	}
	Section->ParentSection[NoOfParents - 1] = Parent;
	Section->ParentSection[NoOfParents] = NULL;

	if (NoOfParents == 1) {
		if (Parent->ContinueSection == Section) {
			memcpy(&Section->RegStart,&Parent->Cont.RegSet,sizeof(REG_INFO));
		} else if (Parent->JumpSection == Section) {
			memcpy(&Section->RegStart,&Parent->Jump.RegSet,sizeof(REG_INFO));
		} else {
#ifndef EXTERNAL_RELEASE
			DisplayError("How are these sections joined?????");
#endif
		}
		memcpy(&Section->RegWorking,&Section->RegStart,sizeof(REG_INFO));
	} else {
		if (Parent->ContinueSection == Section) {
			for (count = 0; count < 32; count++) {
				if (Section->RegStart.MIPS_RegState[count] != Parent->Cont.RegSet.MIPS_RegState[count]) {
					Section->RegStart.MIPS_RegState[count] = STATE_UNKNOWN;
				}
			}
		}
		if (Parent->JumpSection == Section) {
			for (count = 0; count < 32; count++) {
				if (Section->RegStart.MIPS_RegState[count] != Parent->Jump.RegSet.MIPS_RegState[count]) {
					Section->RegStart.MIPS_RegState[count] = STATE_UNKNOWN;
				}
			}
		}
		memcpy(&Section->RegWorking,&Section->RegStart,sizeof(REG_INFO));
	}
}

void _fastcall DetermineLoop(BLOCK_SECTION * Section, DWORD Test, DWORD Test2, DWORD TestID) {
	if (Section == NULL) { return; }
	if (Section->SectionID != TestID) {
		if (Section->Test2 == Test2) {
			return; 
		}
		Section->Test2 = Test2;
		DetermineLoop((BLOCK_SECTION *)Section->ContinueSection,Test,Test2,TestID);
		DetermineLoop((BLOCK_SECTION *)Section->JumpSection,Test,Test2,TestID);
		return;
	}
	if (Section->Test2 == Test2) { 
		Section->InLoop = TRUE;
		return; 
	}
	Section->Test2 = Test2;
	DetermineLoop((BLOCK_SECTION *)Section->ContinueSection,Test,Test2,TestID);
	DetermineLoop((BLOCK_SECTION *)Section->JumpSection,Test,Test2,TestID);
	if (Section->Test == Test) { return; }
	Section->Test = Test;
	if (Section->ContinueSection != NULL) {
		DetermineLoop((BLOCK_SECTION *)Section->ContinueSection,Test,GetNewTestValue(),((BLOCK_SECTION *)Section->ContinueSection)->SectionID);
	}
	if (Section->JumpSection != NULL) {
		DetermineLoop((BLOCK_SECTION *)Section->JumpSection,Test,GetNewTestValue(),((BLOCK_SECTION *)Section->JumpSection)->SectionID);
	}
}

void _fastcall FixConstants (BLOCK_SECTION * Section, DWORD Test, int * Changed) {
	BLOCK_SECTION * Parent;
	int count, NoOfParents;
	REG_INFO Original[2];

	if (Section == NULL) { return; }
	if (Section->Test == Test) { return; }
	Section->Test = Test;

	InheritConstants(Section);
		
	memcpy(&Original[0],&Section->Cont.RegSet,sizeof(REG_INFO));
	memcpy(&Original[1],&Section->Jump.RegSet,sizeof(REG_INFO));

	if (Section->ParentSection) {
		for (NoOfParents = 0;Section->ParentSection[NoOfParents] != NULL;NoOfParents++) {
			Parent = (BLOCK_SECTION *)Section->ParentSection[NoOfParents];
			if (Parent->ContinueSection == Section) {
				for (count = 0; count < 32; count++) {
					if (Section->RegStart.MIPS_RegState[count] != Parent->Cont.RegSet.MIPS_RegState[count]) {
						Section->RegStart.MIPS_RegState[count] = STATE_UNKNOWN;							
						//*Changed = TRUE;
					}
					Section->RegStart.MIPS_RegState[count] = STATE_UNKNOWN;							
				}
			}
			if (Parent->JumpSection == Section) {
				for (count = 0; count < 32; count++) {
					if (Section->RegStart.MIPS_RegState[count] != Parent->Jump.RegSet.MIPS_RegState[count]) {
						Section->RegStart.MIPS_RegState[count] = STATE_UNKNOWN;
						//*Changed = TRUE;
					}
				}
			}
			memcpy(&Section->RegWorking,&Section->RegStart,sizeof(REG_INFO));
		}
	}
	FillSectionInfo(Section);
	if (memcmp(&Original[0],&Section->Cont.RegSet,sizeof(REG_INFO)) != 0) { *Changed = TRUE; }
	if (memcmp(&Original[1],&Section->Jump.RegSet,sizeof(REG_INFO)) != 0) { *Changed = TRUE; }

	if (Section->JumpSection) { FixConstants((BLOCK_SECTION *)Section->JumpSection,Test,Changed); }
	if (Section->ContinueSection) { FixConstants((BLOCK_SECTION *)Section->ContinueSection,Test,Changed); }
}

void FreeSection (BLOCK_SECTION * Section, BLOCK_SECTION * Parent) {
	if (Section == NULL) { return; }

	if (Section->ParentSection) {
		int NoOfParents, count;
		
		for (NoOfParents = 0;Section->ParentSection[NoOfParents] != NULL;NoOfParents++);

		for (count = 0; count < NoOfParents; count++) {
			if (Section->ParentSection[count] == Parent) {
				if (NoOfParents == 1) {
					free(Section->ParentSection);
					//CPU_Message("Free Parent Section (Section: %d)",Section->SectionID);
					Section->ParentSection = NULL;
				} else {
					memmove(&Section->ParentSection[count],&Section->ParentSection[count + 1],
						sizeof(void*) * (NoOfParents - count));				
					Section->ParentSection = (void **)realloc(Section->ParentSection,NoOfParents*sizeof(void *));
				}
				NoOfParents -= 1;
			}
		}		
		
		if (Parent->JumpSection == Section) { Parent->JumpSection = NULL; }
		if (Parent->ContinueSection == Section) { Parent->ContinueSection = NULL; }
		
		if (Section->ParentSection) {
			for (count = 0; count < NoOfParents; count++) {
				if (!IsAllParentLoops(Section,(BLOCK_SECTION *)Section->ParentSection[count],FALSE,GetNewTestValue())) { return; }
			}
			for (count = 0; count < NoOfParents; count++) {
				Parent = (BLOCK_SECTION *)Section->ParentSection[count];
				if (Parent->JumpSection == Section) { Parent->JumpSection = NULL; }
				if (Parent->ContinueSection == Section) { Parent->ContinueSection = NULL; }
			}
			free(Section->ParentSection);
			//CPU_Message("Free Parent Section (Section: %d)",Section->SectionID);
			Section->ParentSection = NULL;
		}
	}
	if (Section->ParentSection == NULL) {
		FreeSection((BLOCK_SECTION *)Section->JumpSection,Section);
		FreeSection((BLOCK_SECTION *)Section->ContinueSection,Section);
		//CPU_Message("Free Section (Section: %d)",Section->SectionID);
		free(Section);
	}
}

void GenerateBasicSectionLinkage (BLOCK_SECTION * Section) {
	BreakPoint(__FILE__,__LINE__); 
}

DWORD GetNewTestValue(void) {
	static DWORD LastTest = 0;
	if (LastTest == 0xFFFFFFFF) { LastTest = 0; }
	LastTest += 1;
	return LastTest;
}

void InitilizeRegSet(REG_INFO * RegSet) {
	int count;
	
	RegSet->MIPS_RegState[0]  = STATE_CONST_32;
	RegSet->MIPS_RegVal[0].DW = 0;
	for (count = 1; count < 32; count ++ ) {
		RegSet->MIPS_RegState[count]   = STATE_UNKNOWN;
		RegSet->MIPS_RegVal[count].DW = 0;

	}
	for (count = 0; count < 10; count ++ ) {
		RegSet->x86reg_MappedTo[count]  = NotMapped;
		RegSet->x86reg_Protected[count] = FALSE;
		RegSet->x86reg_MapOrder[count]  = 0;
	}
	RegSet->CycleCount = 0;
	RegSet->RandomModifier = 0;

	RegSet->Stack_TopPos = 0;
	for (count = 0; count < 8; count ++ ) {
		RegSet->x86fpu_MappedTo[count] = -1;
		RegSet->x86fpu_State[count] = FPU_Unkown;
		RegSet->x86fpu_RoundingModel[count] = RoundDefault;
	}
	RegSet->Fpu_Used = FALSE;
	RegSet->RoundingModel = RoundUnknown;
}

void _fastcall InheritConstants(BLOCK_SECTION * Section) {
	int NoOfParents, count;
	BLOCK_SECTION * Parent;
	REG_INFO * RegSet;


	if (Section->ParentSection == NULL) {
		InitilizeRegSet(&Section->RegStart);
		memcpy(&Section->RegWorking,&Section->RegStart,sizeof(REG_INFO));		
		return;
	} 

	Parent = (BLOCK_SECTION *)Section->ParentSection[0];
	RegSet = Section == Parent->ContinueSection?&Parent->Cont.RegSet:&Parent->Jump.RegSet;
	memcpy(&Section->RegStart,RegSet,sizeof(REG_INFO));		
	memcpy(&Section->RegWorking,&Section->RegStart,sizeof(REG_INFO));		

	for (NoOfParents = 1;Section->ParentSection[NoOfParents] != NULL;NoOfParents++) {
		Parent = (BLOCK_SECTION *)Section->ParentSection[NoOfParents];
		RegSet = Section == Parent->ContinueSection?&Parent->Cont.RegSet:&Parent->Jump.RegSet;
			
		for (count = 0; count < 32; count++) {
			if (IsConst(count)) {
				if (MipsRegState(count) != RegSet->MIPS_RegState[count]) {
					MipsRegState(count) = STATE_UNKNOWN;
				} else if (Is32Bit(count) && MipsRegLo(count) != RegSet->MIPS_RegVal[count].UW[0]) {
					MipsRegState(count) = STATE_UNKNOWN;
				} else if (Is64Bit(count) && MipsReg(count) != RegSet->MIPS_RegVal[count].UDW) {
					MipsRegState(count) = STATE_UNKNOWN;
				}
			}
		}
	}
	memcpy(&Section->RegStart,&Section->RegWorking,sizeof(REG_INFO));		
}


void _fastcall InitilzeSection (BLOCK_SECTION * Section, BLOCK_SECTION * Parent, DWORD StartAddr, DWORD ID) {
	Section->ParentSection      = NULL;
	Section->JumpSection        = NULL;
	Section->ContinueSection    = NULL;
	Section->CompiledLocation   = NULL;

	Section->SectionID          = ID;
	Section->Test               = 0;
	Section->Test2              = 0;
	Section->InLoop             = FALSE;

	Section->StartPC            = StartAddr;
	Section->CompilePC          = Section->StartPC;

	Section->Jump.BranchLabel   = NULL;
	Section->Jump.LinkLocation  = NULL;
	Section->Jump.LinkLocation2 = NULL;
	Section->Jump.FallThrough   = FALSE;
	Section->Jump.PermLoop      = FALSE;
	Section->Jump.TargetPC      = (DWORD)-1;
	Section->Cont.BranchLabel   = NULL;
	Section->Cont.LinkLocation  = NULL;
	Section->Cont.LinkLocation2 = NULL;
	Section->Cont.FallThrough   = FALSE;
	Section->Cont.PermLoop      = FALSE;
	Section->Cont.TargetPC      = (DWORD)-1;

	AddParent(Section,Parent);
}

void MarkCodeBlock (DWORD PAddr) {
	if (PAddr < RdramSize) {
		N64_Blocks.NoOfRDRamBlocks[PAddr >> 12] += 1;
	} else if (PAddr >= 0x04000000 && PAddr <= 0x04000FFC) {
		N64_Blocks.NoOfDMEMBlocks += 1;
	} else if (PAddr >= 0x04001000 && PAddr <= 0x04001FFC) {
		N64_Blocks.NoOfIMEMBlocks += 1;
	} else if (PAddr >= 0x1FC00000 && PAddr <= 0x1FC00800) {
		N64_Blocks.NoOfPifRomBlocks += 1;
	} else {
#ifndef ROM_IN_MAPSPACE
#ifndef EXTERNAL_RELEASE
		DisplayError("Ummm... Which code block should be marked on\nPC = 0x%08X\nRdramSize: %X",PAddr,RdramSize);
#endif
		ExitThread(0);
#endif
	}
}




#endif