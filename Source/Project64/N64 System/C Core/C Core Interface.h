#pragma once

#ifdef __cplusplus
#include "..\\..\\N64 System.h"
#include "..\\..\\User Interface.h"
#include "..\\..\\Multilanguage.h"
#include "..\\..\\Plugin.h"

class CC_Core
{
public:
	static void SetSettings  ( );
};

#endif
#include <common/memtest.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "..\\Types.h"

const char * GetAppName ( void );
void CloseSaveChips     ( void );

//settings
extern BOOL g_HaveDebugger, g_AudioSignal;
extern DWORD g_RomFileSize, g_CountPerOp;
extern enum CPU_TYPE g_CPU_Type;
extern enum SAVE_CHIP_TYPE g_SaveUsing;

//Plugins
extern enum SystemType g_SystemType;

#ifdef __cplusplus
}
#endif