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

int  AllocateMemory ( void );
void FreeMemory     ( void );
void SetJumpTable   ( DWORD End );

extern BYTE * RecompCode, * RecompCodeSecondary, * RecompPos;
extern void ** JumpTable;
extern DWORD Table;

void RSP_LB_DMEM  ( DWORD Addr, BYTE * Value );
void RSP_LBV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LDV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LFV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LH_DMEM  ( DWORD Addr, WORD * Value );
void RSP_LHV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LLV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LPV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LRV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LQV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LSV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LTV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LUV_DMEM ( DWORD Addr, int vect, int element );
void RSP_LW_DMEM  ( DWORD Addr, DWORD * Value );
void RSP_LW_IMEM  ( DWORD Addr, DWORD * Value );
void RSP_SB_DMEM  ( DWORD Addr, BYTE Value );
void RSP_SBV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SDV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SFV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SH_DMEM  ( DWORD Addr, WORD Value );
void RSP_SHV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SLV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SPV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SQV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SRV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SSV_DMEM ( DWORD Addr, int vect, int element );
void RSP_STV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SUV_DMEM ( DWORD Addr, int vect, int element );
void RSP_SW_DMEM  ( DWORD Addr, DWORD Value );
void RSP_SWV_DMEM ( DWORD Addr, int vect, int element );
