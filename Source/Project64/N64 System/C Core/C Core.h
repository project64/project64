#ifndef __C_CORE__H__
#define __C_CORE__H__

#include "C Core Interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "..\N64 Types.h"
#include "..\Mips\OpCode.h"
#include "Core Settings.h"
#include "C Memory.h"
#include "CPU Log.h"

void InitializeCPUCore  ( void );

//from exception.h
void _fastcall DoTLBMiss              ( BOOL DelaySlot, DWORD BadVaddr );

extern enum STEP_TYPE NextInstruction;

#ifdef __cplusplus
}
#endif

#endif 