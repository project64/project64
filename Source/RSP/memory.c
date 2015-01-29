/*
 * RSP Compiler plug in for Project 64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
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

enum { MaxMaps	= 32 };

#include <windows.h>
#include "rsp.h"
#include "RSP Registers.h"

DWORD NoOfMaps, MapsCRC[MaxMaps], Table;
BYTE * RecompCode, * RecompCodeSecondary, * RecompPos, *JumpTables;
void ** JumpTable;

int AllocateMemory (void) {
	RecompCode=(BYTE *) VirtualAlloc( NULL, 0x00400004, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	RecompCode=(BYTE *) VirtualAlloc( RecompCode, 0x00400000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	
	if(RecompCode == NULL) {
		DisplayError("Not enough memory for RSP RecompCode!");
		return FALSE;
	}

	RecompCodeSecondary = (BYTE *)VirtualAlloc( NULL, 0x00200000, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
	if(RecompCodeSecondary == NULL) {
		DisplayError("Not enough memory for RSP RecompCode Secondary!");
		return FALSE;
	}

	JumpTables = (BYTE *)VirtualAlloc( NULL, 0x1000 * MaxMaps, MEM_COMMIT, PAGE_READWRITE );
	if( JumpTables == NULL ) {  
		DisplayError("Not enough memory for Jump Table!");
		return FALSE;
	}

	JumpTable = (void **)JumpTables;
	RecompPos = RecompCode;
	NoOfMaps = 0;
	return TRUE;
}

void FreeMemory (void) {
	VirtualFree( RecompCode, 0 , MEM_RELEASE);
	VirtualFree( JumpTable, 0 , MEM_RELEASE);
	VirtualFree( RecompCodeSecondary, 0 , MEM_RELEASE);
}

void ResetJumpTables ( void )
{
	memset(JumpTables,0,0x1000 * MaxMaps);
	RecompPos = RecompCode;
	NoOfMaps = 0;
}

void SetJumpTable (DWORD End) {
	DWORD CRC, count;

	CRC = 0;
	if (End < 0x800)
	{
		End = 0x800;
	}
	
	if (End == 0x1000 && ((*RSPInfo.SP_MEM_ADDR_REG & 0x0FFF) & ~7) == 0x80)
	{
		End = 0x800;
	}

	for (count = 0; count < End; count += 0x40) {
		CRC += *(DWORD *)(RSPInfo.IMEM + count);		
	}

	for (count = 0; count <	NoOfMaps; count++ ) {
		if (CRC == MapsCRC[count]) {
			JumpTable = (void **)(JumpTables + count * 0x1000);
			Table = count;
			return;
		}
	}
	//DisplayError("%X %X",NoOfMaps,CRC);
	if (NoOfMaps == MaxMaps) {
		ResetJumpTables();
	}
	MapsCRC[NoOfMaps] = CRC;
	JumpTable = (void **)(JumpTables + NoOfMaps * 0x1000);
	Table = NoOfMaps;
	NoOfMaps += 1;
}

void RSP_LB_DMEM ( DWORD Addr, BYTE * Value ) {
	* Value = *(BYTE *)(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) ;
}

void RSP_LBV_DMEM ( DWORD Addr, int vect, int element ) {
	RSP_Vect[vect].B[15 - element] = *(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF));
}

void RSP_LDV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = 8;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LFV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, count;
	VECTOR Temp;

	length = 8;
	if (length > 16 - element) {
		length = 16 - element;
	}
	
	Temp.HW[7] = *(RSPInfo.DMEM + (((Addr + element) ^3) & 0xFFF)) << 7;
	Temp.HW[6] = *(RSPInfo.DMEM + (((Addr + ((0x4 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[5] = *(RSPInfo.DMEM + (((Addr + ((0x8 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[4] = *(RSPInfo.DMEM + (((Addr + ((0xC - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[3] = *(RSPInfo.DMEM + (((Addr + ((0x8 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[2] = *(RSPInfo.DMEM + (((Addr + ((0xC - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[1] = *(RSPInfo.DMEM + (((Addr + ((0x10 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[0] = *(RSPInfo.DMEM + (((Addr + ((0x4 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	
	for (count = element; count < (length + element); count ++ ){
		RSP_Vect[vect].B[15 - count] = Temp.B[15 - count];
	}
}

void RSP_LH_DMEM ( DWORD Addr, WORD * Value ) {
	if ((Addr & 0x1) != 0) {
		if (Addr > 0xFFE) {
			DisplayError("hmmmm.... Problem with:\nRSP_LH_DMEM");
			return;
		}
		Addr &= 0xFFF;
		*Value = *(BYTE *)(RSPInfo.DMEM + (Addr^ 3)) << 8;		
		*Value += *(BYTE *)(RSPInfo.DMEM + ((Addr + 1)^ 3));
		return;
	}
	* Value = *(WORD *)(RSPInfo.DMEM + ((Addr ^ 2 ) & 0xFFF));	
}

void RSP_LHV_DMEM ( DWORD Addr, int vect, int element ) {	
	RSP_Vect[vect].HW[7] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[6] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 2) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[5] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 4) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[4] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 6) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[3] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 8) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[2] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 10) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[1] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 12) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[0] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 14) & 0xF) ^3) & 0xFFF)) << 7;
}

void RSP_LLV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = 4;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LPV_DMEM ( DWORD Addr, int vect, int element ) {	
	RSP_Vect[vect].HW[7] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[6] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 1) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[5] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 2) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[4] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 3) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[3] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 4) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[2] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 5) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[1] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 6) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[0] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 7) & 0xF)^3) & 0xFFF)) << 8;
}

void RSP_LRV_DMEM ( DWORD Addr, int vect, int element ) {	
	int length, Count, offset;

	offset = (Addr & 0xF) - 1;
	length = (Addr & 0xF) - element;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[offset - Count] = *(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LQV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = ((Addr + 0x10) & ~0xF) - Addr;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LSV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = 2;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}
}

void RSP_LTV_DMEM ( DWORD Addr, int vect, int element ) {
	int del, count, length;
	
	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	
	Addr = ((Addr + 8) & 0xFF0) + (element & 0x1);	
	for (count = 0; count < length; count ++) {
		del = ((8 - (element >> 1) + count) << 1) & 0xF;
		RSP_Vect[vect + count].B[15 - del] = *(RSPInfo.DMEM + (Addr ^ 3));
		RSP_Vect[vect + count].B[14 - del] = *(RSPInfo.DMEM + ((Addr + 1) ^ 3));
		Addr += 2;
	}
}

void RSP_LUV_DMEM ( DWORD Addr, int vect, int element ) {	
	RSP_Vect[vect].HW[7] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[6] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 1) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[5] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 2) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[4] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 3) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[3] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 4) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[2] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 5) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[1] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 6) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[0] = *(RSPInfo.DMEM + ((Addr + ((0x10 - element + 7) & 0xF)^3) & 0xFFF)) << 7;
}

void RSP_LW_DMEM ( DWORD Addr, DWORD * Value ) {
	if ((Addr & 0x3) != 0) {
		Addr &= 0xFFF;
		if (Addr > 0xFFC) {
			DisplayError("hmmmm.... Problem with:\nRSP_LW_DMEM");
			return;
		}
		*Value = *(BYTE *)(RSPInfo.DMEM + (Addr^ 3)) << 0x18;
		*Value += *(BYTE *)(RSPInfo.DMEM + ((Addr + 1)^ 3)) << 0x10;
		*Value += *(BYTE *)(RSPInfo.DMEM + ((Addr + 2)^ 3)) << 8;
		*Value += *(BYTE *)(RSPInfo.DMEM + ((Addr + 3)^ 3));
		return;
	}
	* Value = *(DWORD *)(RSPInfo.DMEM + (Addr & 0xFFF));	
}

void RSP_LW_IMEM ( DWORD Addr, DWORD * Value ) {
	if ((Addr & 0x3) != 0) {
		DisplayError("Unaligned RSP_LW_IMEM");
	}
	* Value = *(DWORD *)(RSPInfo.IMEM + (Addr & 0xFFF));	
}

void RSP_SB_DMEM ( DWORD Addr, BYTE Value ) {
	*(BYTE *)(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = Value;
}

void RSP_SBV_DMEM ( DWORD Addr, int vect, int element ) {
	*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - element];
}

void RSP_SDV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_SFV_DMEM ( DWORD Addr, int vect, int element ) {	
	int offset = Addr & 0xF;
	Addr &= 0xFF0;

	switch (element) {
	case 0:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[7] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[6] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[5] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[4] >> 7);
		break;
	case 1:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[1] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[0] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[3] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[2] >> 7);
		break;
	case 2:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 3:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF)^3))) = 0;
		break;
	case 4:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[6] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[5] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[4] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[7] >> 7);
		break;
	case 5:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[0] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[3] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[2] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[1] >> 7);
		break;
	case 6:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 7:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 8:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[3] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[2] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[1] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[0] >> 7);
		break;
	case 9:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 10:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 11:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[4] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[7] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[6] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[5] >> 7);
		break;
	case 12:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[2] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[1] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[0] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[3] >> 7);
		break;
	case 13:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 14:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 15:
		*(RSPInfo.DMEM + ((Addr + offset)^3)) = (BYTE)(RSP_Vect[vect].UHW[7] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[6] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[5] >> 7);
		*(RSPInfo.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = (BYTE)(RSP_Vect[vect].UHW[4] >> 7);
		break;
	}
}

void RSP_SH_DMEM ( DWORD Addr, WORD Value ) {
	if ((Addr & 0x1) != 0) {
		DisplayError("Unaligned RSP_SH_DMEM");
		return;
	}
	*(WORD *)(RSPInfo.DMEM + ((Addr ^ 2) & 0xFFF)) = Value;
}

void RSP_SHV_DMEM ( DWORD Addr, int vect, int element ) {	
	*(RSPInfo.DMEM + ((Addr^3) & 0xFFF)) = (RSP_Vect[vect].UB[(15 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(14 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 2)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(13 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(12 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 4)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(11 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(10 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 6)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(9 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(8 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 8)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(7 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(6 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 10)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(5 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(4 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 12)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(3 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(2 - element) & 0xF] >> 7);
	*(RSPInfo.DMEM + (((Addr + 14)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(1 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(0 - element) & 0xF] >> 7);
}

void RSP_SLV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (4 + element); Count ++ ){
		*(RSPInfo.DMEM + ((Addr ^3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_SPV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		if (((Count) & 0xF) < 8) {
			*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].UB[15 - ((Count & 0xF) << 1)];
		} else {
			*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = (RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)] << 1) +
				(RSP_Vect[vect].UB[14 - ((Count & 0x7) << 1)] >> 7);
		}
		Addr += 1;
	}
}

void RSP_SQV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = ((Addr + 0x10) & ~0xF) - Addr;
	for (Count = element; Count < (length + element); Count ++ ){
		*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_SRV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count, offset;

	length = (Addr & 0xF);
	offset = (0x10 - length) & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - ((Count + offset) & 0xF)];
		Addr += 1;
	}
}

void RSP_SSV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (2 + element); Count ++ ){
		*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_STV_DMEM ( DWORD Addr, int vect, int element ) {
	int del, count, length;
	
	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	length = length << 1;
	del = element >> 1;
	for (count = 0; count < length; count += 2) {
		*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect + del].UB[15 - count];
		*(RSPInfo.DMEM + (((Addr + 1) ^ 3) & 0xFFF)) = RSP_Vect[vect + del].UB[14 - count];
		del = (del + 1) & 7;
		Addr += 2;
	}
}

void RSP_SUV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		if (((Count) & 0xF) < 8) {
			*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = ((RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)] << 1) +
				(RSP_Vect[vect].UB[14 - ((Count & 0x7) << 1)] >> 7)) & 0xFF;
		} else {
			*(RSPInfo.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)];
		}
		Addr += 1;
	}
}

void RSP_SW_DMEM ( DWORD Addr, DWORD Value ) {
	Addr &= 0xFFF;
	if ((Addr & 0x3) != 0) {
		if (Addr > 0xFFC) {
			DisplayError("hmmmm.... Problem with:\nRSP_SW_DMEM");
			return;
		}
		*(BYTE *)(RSPInfo.DMEM + (Addr ^ 3)) = (BYTE)((Value >> 0x18) & 0xFF);
		*(BYTE *)(RSPInfo.DMEM + ((Addr + 1) ^ 3)) = (BYTE)((Value >> 0x10) & 0xFF);
		*(BYTE *)(RSPInfo.DMEM + ((Addr + 2) ^ 3)) = (BYTE)((Value >> 0x8) & 0xFF);
		*(BYTE *)(RSPInfo.DMEM + ((Addr + 3) ^ 3)) = (BYTE)(Value &0xFF);
		return;
	}
	*(DWORD *)(RSPInfo.DMEM + Addr) = Value;
}

void RSP_SWV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count, offset;

	offset = Addr & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (16 + element); Count ++ ){
		*(RSPInfo.DMEM + ((Addr + (offset & 0xF)) ^ 3)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		offset += 1;
	}
}