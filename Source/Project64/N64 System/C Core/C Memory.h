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
#include <Windows.h>

#define ROM_IN_MAPSPACE

extern void ** JumpTable;
extern BYTE *RecompCode, *RecompPos;
extern BOOL WrittenToRom;

/* Memory Control */
int  Allocate_ROM                ( void );
int  Allocate_Memory             ( void );	
void Release_Memory              ( void );

/* CPU memory functions */
int  r4300i_Command_MemoryFilter ( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP );
int  r4300i_CPU_MemoryFilter     ( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP );
int  r4300i_LB_NonMemory         ( DWORD PAddr, DWORD * Value, BOOL SignExtend );
BOOL r4300i_LB_VAddr             ( DWORD VAddr, BYTE * Value );
BOOL r4300i_LD_VAddr             ( DWORD VAddr, unsigned _int64 * Value );
int  r4300i_LH_NonMemory         ( DWORD PAddr, DWORD * Value, int SignExtend );
BOOL r4300i_LH_VAddr             ( DWORD VAddr, WORD * Value );
int  r4300i_LW_NonMemory         ( DWORD PAddr, DWORD * Value );
void r4300i_LW_PAddr             ( DWORD PAddr, DWORD * Value );
BOOL r4300i_LW_VAddr             ( DWORD VAddr, DWORD * Value );
int  r4300i_SB_NonMemory         ( DWORD PAddr, BYTE Value );
BOOL r4300i_SB_VAddr             ( DWORD VAddr, BYTE Value );
BOOL r4300i_SD_VAddr             ( DWORD VAddr, unsigned _int64 Value );
int  r4300i_SH_NonMemory         ( DWORD PAddr, WORD Value );
BOOL r4300i_SH_VAddr             ( DWORD VAddr, WORD Value );
int  r4300i_SW_NonMemory         ( DWORD PAddr, DWORD Value );
BOOL r4300i_SW_VAddr             ( DWORD VAddr, DWORD Value );

/* Recompiler Memory Functions */
void Compile_LB                  ( int Reg, DWORD Addr, BOOL SignExtend );
void Compile_LH                  ( int Reg, DWORD Addr, BOOL SignExtend );
void Compile_SB_Const            ( BYTE Value, DWORD Addr );
void Compile_SB_Register         ( int x86Reg, DWORD Addr );
void Compile_SH_Const            ( WORD Value, DWORD Addr );
void Compile_SH_Register         ( int x86Reg, DWORD Addr );
void Compile_SW_Const            ( DWORD Value, DWORD Addr );
//void ResetRecompCode             ( void );

#ifdef __cplusplus

void Compile_LW                  ( CBlockSection * Section, int Reg, DWORD Addr );
void Compile_SW_Register         ( CBlockSection * Section, int x86Reg, DWORD Addr );
void ResetMemoryStack            ( CBlockSection * Section );

#endif
