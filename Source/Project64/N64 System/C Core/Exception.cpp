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
#include <windows.h>
#include "main.h"
#include "cpu.h"
#include "plugin.h"
#include "debugger.h"

void __cdecl AiCheckInterrupts ( void ) {	
	CPU_Action.CheckInterrupts = TRUE;
	CPU_Action.DoSomething = TRUE;
}

void __cdecl CheckInterrupts ( void ) {	

	if (!g_FixedAudio && CPU_Type != CPU_SyncCores) {
		MI_INTR_REG &= ~MI_INTR_AI;
		MI_INTR_REG |= (_Reg->AudioIntrReg & MI_INTR_AI);
	}
	if ((MI_INTR_MASK_REG & MI_INTR_REG) != 0) {
		FAKE_CAUSE_REGISTER |= CAUSE_IP2;
	} else  {
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP2;
	}

	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }

	if (( STATUS_REGISTER & FAKE_CAUSE_REGISTER & 0xFF00) != 0) {
		if (!CPU_Action.DoInterrupt) {
			CPU_Action.DoSomething = TRUE;
			CPU_Action.DoInterrupt = TRUE;
		}
	}
}

void DoAddressError ( BOOL DelaySlot, DWORD BadVaddr, BOOL FromRead) {
#ifndef EXTERNAL_RELEASE
	DisplayError("AddressError");
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in AddressError Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in AddressError Exception");
	}
#endif
	if (FromRead) {
		CAUSE_REGISTER = EXC_RADE;
	} else {
		CAUSE_REGISTER = EXC_WADE;
	}
	BAD_VADDR_REGISTER = BadVaddr;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = (*_PROGRAM_COUNTER) - 4;
	} else {
		EPC_REGISTER = (*_PROGRAM_COUNTER);
	}
	STATUS_REGISTER |= STATUS_EXL;
	(*_PROGRAM_COUNTER) = 0x80000180;
}

void DoBreakException ( BOOL DelaySlot) {
#ifndef EXTERNAL_RELEASE
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in Break Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in Break Exception");
	}
#endif

	CAUSE_REGISTER = EXC_BREAK;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = (*_PROGRAM_COUNTER) - 4;
	} else {
		EPC_REGISTER = (*_PROGRAM_COUNTER);
	}
	STATUS_REGISTER |= STATUS_EXL;
	(*_PROGRAM_COUNTER) = 0x80000180;
}

void _fastcall DoCopUnusableException ( BOOL DelaySlot, int Coprocessor ) {
#ifndef EXTERNAL_RELEASE
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in Break Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in Break Exception");
	}
#endif

	CAUSE_REGISTER = EXC_CPU;
	if (Coprocessor == 1) { CAUSE_REGISTER |= 0x10000000; }
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = (*_PROGRAM_COUNTER) - 4;
	} else {
		EPC_REGISTER = (*_PROGRAM_COUNTER);
	}
	STATUS_REGISTER |= STATUS_EXL;
	(*_PROGRAM_COUNTER) = 0x80000180;
}

BOOL DoIntrException ( BOOL DelaySlot ) {
	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return FALSE; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return FALSE; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return FALSE; }
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.GenerateLog && LogOptions.LogExceptions && !LogOptions.NoInterrupts) {
		LogMessage("%08X: Interupt Generated", (*_PROGRAM_COUNTER) );
	}
#endif
	CAUSE_REGISTER = FAKE_CAUSE_REGISTER;
	CAUSE_REGISTER |= EXC_INT;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = (*_PROGRAM_COUNTER) - 4;
	} else {
		EPC_REGISTER = (*_PROGRAM_COUNTER);
	}
	STATUS_REGISTER |= STATUS_EXL;
	(*_PROGRAM_COUNTER) = 0x80000180;
	return TRUE;
}

void _fastcall DoTLBMiss ( BOOL DelaySlot, DWORD BadVaddr ) {
	CAUSE_REGISTER = EXC_RMISS;
	BAD_VADDR_REGISTER = BadVaddr;
	CONTEXT_REGISTER &= 0xFF80000F;
	CONTEXT_REGISTER |= (BadVaddr >> 9) & 0x007FFFF0;
	ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
	if ((STATUS_REGISTER & STATUS_EXL) == 0) {
		if (DelaySlot) {
			CAUSE_REGISTER |= CAUSE_BD;
			EPC_REGISTER = (*_PROGRAM_COUNTER) - 4;
		} else {
			EPC_REGISTER = (*_PROGRAM_COUNTER);
		}
		if (_TLB->AddressDefined(BadVaddr)) 
		{
			(*_PROGRAM_COUNTER) = 0x80000180;
		} else {
			(*_PROGRAM_COUNTER) = 0x80000000;
		}
		STATUS_REGISTER |= STATUS_EXL;
	} else {
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
#ifndef EXTERNAL_RELEASE
		DisplayError("TLBMiss - EXL Set\nBadVaddr = %X\nAddress Defined: %s",BadVaddr,_TLB->TLB_AddressDefined(BadVaddr)?"TRUE":"FALSE");
#endif
#endif
		(*_PROGRAM_COUNTER) = 0x80000180;
	}
}

void _fastcall DoSysCallException ( BOOL DelaySlot) {
#ifndef EXTERNAL_RELEASE
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in SysCall Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in SysCall Exception");
	}
#endif

	CAUSE_REGISTER = EXC_SYSCALL;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = (*_PROGRAM_COUNTER) - 4;
	} else {
		EPC_REGISTER = (*_PROGRAM_COUNTER);
	}
	STATUS_REGISTER |= STATUS_EXL;
	(*_PROGRAM_COUNTER) = 0x80000180;
}
