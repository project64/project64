#include "..\..\N64 System.h"
#include "../C Core/C Core Interface.h"
#include <windows.h> //needed for memory allocation
#include <commctrl.h> //needed for debug window


//CLog TlbLog("TLB Log.txt");

CTLB::CTLB(CMipsTLB_CallBack * CallBack, BYTE * &BasePAddr, CRegisters * const &Registers ):	
	CBClass	(CallBack),
	m_BasePAddr(BasePAddr),
	PROGRAM_COUNTER(Registers->PROGRAM_COUNTER),
	INDEX_REGISTER(Registers->INDEX_REGISTER),
	PAGE_MASK_REGISTER(Registers->PAGE_MASK_REGISTER),
	ENTRYHI_REGISTER(Registers->ENTRYHI_REGISTER),
	ENTRYLO0_REGISTER(Registers->ENTRYLO0_REGISTER),
	ENTRYLO1_REGISTER(Registers->ENTRYLO1_REGISTER)
{
	WriteTrace(TraceTLB,"CTLB::CTLB - Start");
	TLB_ReadMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (TLB_ReadMap == NULL) {
		Notify().FatalError(MSG_MEM_ALLOC_ERROR);
	}

	TLB_WriteMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (TLB_WriteMap == NULL) {
		Notify().FatalError(MSG_MEM_ALLOC_ERROR);
	}

	//TLB_Reset(true);
	WriteTrace(TraceTLB,"CTLB::CTLB - Done");
}

CTLB::~CTLB (void) {
	WriteTrace(TraceTLB,"CTLB::~CTLB - Done");
	VirtualFree( TLB_ReadMap, 0 , MEM_RELEASE);
	VirtualFree( TLB_WriteMap, 0 , MEM_RELEASE);
	WriteTrace(TraceTLB,"CTLB::~CTLB - Done");
}

void CTLB::TLB_Reset (bool InvalidateTLB) {
	DWORD count;

	if (InvalidateTLB) {
		for (count = 0; count < 32; count++) { tlb[count].EntryDefined = false; }
	}
	for (count = 0; count < 64; count++) { FastTlb[count].ValidEntry = false; }	

	memset(TLB_ReadMap,0,(0xFFFFF * sizeof(DWORD)));
	memset(TLB_WriteMap,0,(0xFFFFF * sizeof(DWORD)));
	for (count = 0x80000000; count < 0xC0000000; count += 0x1000) {
		TLB_ReadMap[count >> 12] = ((DWORD)m_BasePAddr + (count & 0x1FFFFFFF)) - count;
		TLB_WriteMap[count >> 12] = ((DWORD)m_BasePAddr + (count & 0x1FFFFFFF)) - count;
	}
	for (count = 0; count < 32; count ++) { SetupTLB_Entry(count,false); }
	//GE Hack
	
	if (_Settings->LoadDword(ROM_TLB_VAddrStart) != 0)
	{
		DWORD Start = _Settings->LoadDword(ROM_TLB_VAddrStart); //0x7F000000;
		DWORD Len   = _Settings->LoadDword(ROM_TLB_VAddrLen);   //0x01000000;
		DWORD PAddr = _Settings->LoadDword(ROM_TLB_PAddrStart); //0x10034b30;
		DWORD End   = Start + Len;
		for (count = Start; count < End; count += 0x1000) {
			TLB_ReadMap[count >> 12] = ((DWORD)m_BasePAddr + (count - Start + PAddr)) - count;
			TLB_WriteMap[count >> 12] = ((DWORD)m_BasePAddr + (count - Start + PAddr)) - count;
		}
	}
}

bool CTLB::TLB_AddressDefined ( DWORD VAddr) {
	DWORD i;

	if (VAddr >= 0x80000000 && VAddr <= 0xBFFFFFFF) {
		return true;
	}

	for (i = 0; i < 64; i++) {
		if (FastTlb[i].ValidEntry == FALSE) { continue; }
		if (VAddr >= FastTlb[i].VSTART && VAddr <= FastTlb[i].VEND) {
			//TlbLog.Log("AddressDefined from tlb entry %d",i);
			return true;
		}
	}	
	return FALSE;	
}

void CTLB::TLB_Probe (void) {
	int Counter;
	
	WriteTrace(TraceTLB,"TLB Probe");
	INDEX_REGISTER |= 0x80000000;
	for (Counter = 0; Counter < 32; Counter ++) {		
		if (!tlb[Counter].EntryDefined) { continue; }
		DWORD TlbValue = tlb[Counter].EntryHi.Value & (~tlb[Counter].PageMask.Mask << 13);
		DWORD EntryHi = ENTRYHI_REGISTER & (~tlb[Counter].PageMask.Mask << 13);

		if (TlbValue == EntryHi) {			
			BOOL Global = (tlb[Counter].EntryHi.Value & 0x100) != 0;
			BOOL SameAsid = ((tlb[Counter].EntryHi.Value & 0xFF) == (ENTRYHI_REGISTER & 0xFF));
			
			if (Global || SameAsid) {
				INDEX_REGISTER = Counter;
				FastTlb[Counter << 1].Probed = true;
				FastTlb[(Counter << 1) + 1].Probed = true;
				return;
			}
		}
	}
}

void CTLB::TLB_ReadEntry (void) {
	DWORD index = INDEX_REGISTER & 0x1F;

	PAGE_MASK_REGISTER = tlb[index].PageMask.Value ;
	ENTRYHI_REGISTER = (tlb[index].EntryHi.Value & ~tlb[index].PageMask.Value) ;
	ENTRYLO0_REGISTER = tlb[index].EntryLo0.Value;
	ENTRYLO1_REGISTER = tlb[index].EntryLo1.Value;		
}

void CTLB::TLB_WriteEntry (int index, bool Random) {
	int FastIndx;

	WriteTraceF(TraceTLB,"Write Entry %02d %d %08X %08X %08X %08X ",index,Random,PAGE_MASK_REGISTER,ENTRYHI_REGISTER,ENTRYLO0_REGISTER,ENTRYLO1_REGISTER);

	//Check to see if entry is unmapping it self
	if (tlb[index].EntryDefined) {
		FastIndx = index << 1;
		if ((PROGRAM_COUNTER >= FastTlb[FastIndx].VSTART && 
			PROGRAM_COUNTER < FastTlb[FastIndx].VEND &&
			FastTlb[FastIndx].ValidEntry && FastTlb[FastIndx].VALID)
			|| 
			(PROGRAM_COUNTER >= FastTlb[FastIndx + 1].VSTART && 
			PROGRAM_COUNTER < FastTlb[FastIndx + 1].VEND &&
			FastTlb[FastIndx + 1].ValidEntry && FastTlb[FastIndx + 1].VALID))
		{
			return;
		}
	}

	//see if tlb entry is the same

	//Reset old addresses
	if (tlb[index].EntryDefined) {
		DWORD count;

		for ( FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
			if (!FastTlb[FastIndx].ValidEntry) { continue; }
			if (!FastTlb[FastIndx].VALID) { continue; }
			if (tlb[index].PageMask.Value == PAGE_MASK_REGISTER &&
				tlb[index].EntryHi.Value == ENTRYHI_REGISTER) 
			{
				if (FastIndx == (index << 1) && tlb[index].EntryLo0.Value == ENTRYLO0_REGISTER) {
					continue;
				}
				if (FastIndx != (index << 1) && tlb[index].EntryLo1.Value == ENTRYLO1_REGISTER) {
					continue;
				}
			}
			if (FastTlb[FastIndx].Random && !FastTlb[FastIndx].Probed) {
				//Move tlb if possible
				//int NewIndex = TLB_UnusedEntry((int)WIRED_REGISTER);
				/*if (NewIndex > 0)  {
					tlb[NewIndex].PageMask.Value = tlb[index].PageMask.Value;
					tlb[NewIndex].EntryHi.Value = tlb[index].EntryHi.Value;
					tlb[NewIndex].EntryLo0.Value = tlb[index].EntryLo0.Value;
					tlb[NewIndex].EntryLo1.Value = tlb[index].EntryLo1.Value;
					tlb[NewIndex].EntryDefined = tlb[index].EntryDefined;
					SetupTLB_Entry(NewIndex,FastTlb[FastIndx].Random);
					break;
				}*/				
			}
			CBClass->TLB_Unmapping(index,FastIndx,FastTlb[FastIndx].VSTART,(FastTlb[FastIndx].VEND - FastTlb[FastIndx].VSTART));
			for (count = FastTlb[FastIndx].VSTART; count < FastTlb[FastIndx].VEND; count += 0x1000) {
				TLB_ReadMap[count >> 12] = 0;
				TLB_WriteMap[count >> 12] = 0;
			}
		}
	}
	
	//fill in tlb entry
	tlb[index].PageMask.Value = PAGE_MASK_REGISTER;
	tlb[index].EntryHi.Value = ENTRYHI_REGISTER;
	tlb[index].EntryLo0.Value = ENTRYLO0_REGISTER;
	tlb[index].EntryLo1.Value = ENTRYLO1_REGISTER;
	tlb[index].EntryDefined = true;
	SetupTLB_Entry(index,Random);
	CBClass->TLB_Changed();
	//RefreshTLBWindow();
}

void CTLB::SetupTLB_Entry (int index, bool Random) {
	//Fix up Fast TLB entries
	if (!tlb[index].EntryDefined) { return; }

	int FastIndx = index << 1;
	FastTlb[FastIndx].VSTART=tlb[index].EntryHi.VPN2 << 13;
	FastTlb[FastIndx].VEND = FastTlb[FastIndx].VSTART + (tlb[index].PageMask.Mask << 12) + 0xFFF;
	FastTlb[FastIndx].PHYSSTART = tlb[index].EntryLo0.PFN << 12;
	FastTlb[FastIndx].PHYSEND = FastTlb[FastIndx].PHYSSTART + (tlb[index].PageMask.Mask << 12) + 0xFFF;
	FastTlb[FastIndx].VALID = tlb[index].EntryLo0.V;
	FastTlb[FastIndx].DIRTY = tlb[index].EntryLo0.D; 
	FastTlb[FastIndx].GLOBAL = tlb[index].EntryLo0.GLOBAL & tlb[index].EntryLo1.GLOBAL;
	FastTlb[FastIndx].ValidEntry = FALSE;
	FastTlb[FastIndx].Random = Random;
	FastTlb[FastIndx].Probed = false;


	FastIndx = (index << 1) + 1;
	FastTlb[FastIndx].VSTART=(tlb[index].EntryHi.VPN2 << 13) + ((tlb[index].PageMask.Mask << 12) + 0xFFF + 1);
	FastTlb[FastIndx].VEND = FastTlb[FastIndx].VSTART + (tlb[index].PageMask.Mask << 12) + 0xFFF;
	FastTlb[FastIndx].PHYSSTART = tlb[index].EntryLo1.PFN << 12;
	FastTlb[FastIndx].PHYSEND = FastTlb[FastIndx].PHYSSTART + (tlb[index].PageMask.Mask << 12) + 0xFFF;
	FastTlb[FastIndx].VALID = tlb[index].EntryLo1.V;
	FastTlb[FastIndx].DIRTY = tlb[index].EntryLo1.D; 
	FastTlb[FastIndx].GLOBAL = tlb[index].EntryLo0.GLOBAL & tlb[index].EntryLo1.GLOBAL;
	FastTlb[FastIndx].ValidEntry = FALSE;
	FastTlb[FastIndx].Random = Random;
	FastTlb[FastIndx].Probed = false;

	//Test both entries to see if they are valid
 	for ( FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
		DWORD count;

		if (!FastTlb[FastIndx].VALID) { 
			FastTlb[FastIndx].ValidEntry = true;
			continue; 
		}
		
		if (FastTlb[FastIndx].VEND <= FastTlb[FastIndx].VSTART) {
			continue;
		}
		if (FastTlb[FastIndx].VSTART >= 0x80000000 && FastTlb[FastIndx].VEND <= 0xBFFFFFFF) {
			continue;
		}
		if (FastTlb[FastIndx].PHYSSTART > 0x1FFFFFFF) {
			continue;				
		}
			
		//MAP the new tlb entry for reading and writing
		FastTlb[FastIndx].ValidEntry = true;
		for (count = FastTlb[FastIndx].VSTART; count < FastTlb[FastIndx].VEND; count += 0x1000) {
			TLB_ReadMap[count >> 12] = ((DWORD)m_BasePAddr + (count - FastTlb[FastIndx].VSTART + FastTlb[FastIndx].PHYSSTART)) - count;
			if (!FastTlb[FastIndx].DIRTY) { continue; }
			TLB_WriteMap[count >> 12] = ((DWORD)m_BasePAddr + (count - FastTlb[FastIndx].VSTART + FastTlb[FastIndx].PHYSSTART)) - count;
		}
	}
}

bool CTLB::VAddrToRealAddr(DWORD VAddr, void * &RealAddress) {
	if (TLB_ReadMap[VAddr >> 12] == 0) { return false; }
	RealAddress = (BYTE *)(TLB_ReadMap[VAddr >> 12] + VAddr);
	return true;
}

bool CTLB::TLB_EntryUsed(int index ) {
	if (!tlb[index].EntryDefined) { return false; }

	//Test both entries to see if they are valid	
	bool ValidEntry = false;
	for ( int FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
		if (!FastTlb[FastIndx].VALID) { continue; }	
		if (FastTlb[FastIndx].VEND <= FastTlb[FastIndx].VSTART) { continue; }
		if (FastTlb[FastIndx].VSTART >= 0x80000000 && FastTlb[FastIndx].VEND <= 0xBFFFFFFF) {  continue; }
		if (FastTlb[FastIndx].PHYSSTART > 0x1FFFFFFF) {  continue; }
		ValidEntry = true;
	}
	return ValidEntry;
}

int CTLB::TLB_UnusedEntry (int MinIndx ) {
	MinIndx &= 0x1f;
	
	for (int count = 31; count >= MinIndx; count --) {
		if (!TLB_EntryUsed(count)) { return count; }
	}
	return -1;
}

bool CTLB::PAddrToVAddr(DWORD PAddr, DWORD & VAddr, DWORD & Index )
{
	for (int i = Index; i < 64; i++)
	{
		if (FastTlb[i].ValidEntry == false) { continue; }
		if (PAddr >= FastTlb[i].PHYSSTART && PAddr < FastTlb[i].PHYSEND) 
		{
			VAddr = FastTlb[i].VSTART + (PAddr - FastTlb[i].PHYSSTART);
			Index = i + 1;
			return true;
		}

	}
	return false;
}

