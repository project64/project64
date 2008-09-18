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
#define	EXC_CODE(x)	((x)<<2)
#define	EXC_INT					EXC_CODE(0)	/* interrupt */
#define	EXC_MOD					EXC_CODE(1)	/* TLB mod */
#define	EXC_RMISS				EXC_CODE(2)	/* Read TLB Miss */
#define	EXC_WMISS				EXC_CODE(3)	/* Write TLB Miss */
#define	EXC_RADE				EXC_CODE(4)	/* Read Address Error */
#define	EXC_WADE				EXC_CODE(5)	/* Write Address Error */
#define	EXC_IBE					EXC_CODE(6)	/* Instruction Bus Error */
#define	EXC_DBE					EXC_CODE(7)	/* Data Bus Error */
#define	EXC_SYSCALL				EXC_CODE(8)	/* SYSCALL */
#define	EXC_BREAK				EXC_CODE(9)	/* BREAKpoint */
#define	EXC_II					EXC_CODE(10)/* Illegal Instruction */
#define	EXC_CPU					EXC_CODE(11)/* CoProcessor Unusable */
#define	EXC_OV					EXC_CODE(12)/* OVerflow */
#define	EXC_TRAP				EXC_CODE(13)/* Trap exception */
#define	EXC_VCEI				EXC_CODE(14)/* Virt. Coherency on Inst. fetch */
#define	EXC_FPE					EXC_CODE(15)/* Floating Point Exception */
#define	EXC_WATCH				EXC_CODE(23)/* Watchpoint reference */
#define	EXC_VCED				EXC_CODE(31)/* Virt. Coherency on data read */

#define Exception_Name(Except)\
	(Except) == EXC_INT     ? "interrupt" :\
	(Except) == EXC_MOD     ? "TLB mod" :\
	(Except) == EXC_RMISS   ? "Read TLB Miss" :\
	(Except) == EXC_WMISS   ? "Write TLB Miss" :\
	(Except) == EXC_RADE    ? "Read Address Error" :\
	(Except) == EXC_WADE    ? "Write Address Error" :\
	(Except) == EXC_IBE     ? "Instruction Bus Error" :\
	(Except) == EXC_DBE     ? "Data Bus Error" :\
	(Except) == EXC_SYSCALL ? "SYSCALL" :\
	(Except) == EXC_BREAK   ? "Break" :\
	(Except) == EXC_II      ? "Illegal Instruction" :\
	(Except) == EXC_CPU     ? "CoProcessor Unusable" :\
	(Except) == EXC_OV      ? "OVerflow" :\
	(Except) == EXC_TRAP    ? "Trap exception" :\
	(Except) == EXC_VCEI    ? "Virt. Coherency on Inst. fetch" :\
	(Except) == EXC_FPE     ? "Floating Point Exception" :\
	(Except) == EXC_WATCH   ? "Watchpoint reference" :\
	(Except) == EXC_VCED    ? "Virt. Coherency on data read" :\
	"Unkown"

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl AiCheckInterrupts      ( void );
void __cdecl CheckInterrupts        ( void );
void DoAddressError         ( BOOL DelaySlot, DWORD BadVaddr, BOOL FromRead );
void DoBreakException       ( BOOL DelaySlot );
void _fastcall DoCopUnusableException ( BOOL DelaySlot, int Coprocessor );
BOOL DoIntrException        ( BOOL DelaySlot );
void _fastcall DoTLBMiss              ( BOOL DelaySlot, DWORD BadVaddr );
void _fastcall DoSysCallException ( BOOL DelaySlot);

#ifdef __cplusplus
}
#endif

