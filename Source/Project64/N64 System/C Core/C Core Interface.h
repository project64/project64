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
	static void RunRsp (void );
	static void ApplyCheats (CN64System * System );
	static void ApplyGSButtonCheats (CN64System * System );
	static void PauseExecution ( void );
	static bool LoadState (CN64System * System ) { return System->LoadState(); }
	static bool SaveState (CN64System * System ) { return System->SaveState(); }
	static void GenerateProfileLog ( void );
	static void ResetTimer ( void );
	static DWORD StartTimer ( DWORD Address );
	static DWORD StopTimer  ( void );
};

#endif
#include <common/memtest.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "..\\Types.h"

const char * GetAppName ( void );

void PauseExecution     ( void );
void GetAutoSaveDir     ( char * Directory );
void GetInstantSaveDir  ( char * Directory );
void SetFpuLocations    ( void ); 
BOOL Limit_FPS          ( void );
void ChangePluginFunc    ( void );
void ApplyGSButtonCheats ( void );
void ChangeFullScreenFunc ( void ); 
void StartInterpreterCPU      ( void );
void SyncSystem         ( void );
BOOL Machine_LoadState  ( void );
BOOL Machine_SaveState  ( void );
void BreakPoint         ( LPCSTR FileName, int LineNumber );
void ApplyCheats    ( void );
void RunRsp	            ( void );
void RefreshScreen      ( void );
void CloseSaveChips     ( void );

//Timer functions
void ResetTimer ( void );
void GenerateProfileLog ( void );
DWORD StartTimer ( DWORD Address );
DWORD StopTimer  ( void );

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