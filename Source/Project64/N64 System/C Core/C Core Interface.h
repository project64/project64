#pragma once

#ifdef __cplusplus
#include "..\\..\\N64 System.h"
#include "..\\..\\User Interface.h"
#include "..\\..\\Multilanguage.h"
#include "..\\..\\Plugin.h"

#ifdef toremove
extern CN64System    * g_N64System;
extern CN64System    * g_SyncSystem;
extern CRecompiler   * g_Recompiler;
extern CMipsMemoryVM * g_MMU; //Memory of the n64 
extern CTLB          * g_TLB; //TLB Unit
extern CRegisters    * g_Reg; //Current Register Set attacted to the _MMU
extern CNotification * g_Notify;   
extern CSettings     * g_Settings;   
extern CPlugins      * g_Plugins;
extern CN64Rom       * g_Rom;      //The current rom that this system is executing.. it can only execute one file at the time
extern CAudio        * g_Audio;
#endif

class CC_Core
{
public:
	static void SetSettings  ( );
	static void SetN64System ( CN64System * System );
	static void SetSyncCpu   ( CN64System * System );
	static void SetCurrentSystem (CN64System * System );
	static void RunRsp (void );
	static void RefreshScreen (void );
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
void DisplayError       ( const char * Message, ... );
void DisplayMessage     ( int DisplayTime, const char * Message, ... );
void DisplayMessage2    ( const char * Message, ... );
void GetAutoSaveDir     ( char * Directory );
void GetInstantSaveDir  ( char * Directory );
void SetFpuLocations    ( void ); 
BOOL Limit_FPS          ( void );
void DacrateChanged     ( enum SystemType Type );
void ChangePluginFunc    ( void );
void ApplyGSButtonCheats ( void );
void ChangeFullScreenFunc ( void ); 
BOOL Close_C_CPU        ( void );
void StopEmulation      ( void ); 
void __stdcall UpdateSyncCPU      ( DWORD const Cycles );
void StartInterpreterCPU      ( void );
void ExecuteCycles      ( DWORD Cycles );
void SyncSystem         ( void );
BOOL Machine_LoadState  ( void );
BOOL Machine_SaveState  ( void );
void BreakPoint         ( LPCSTR FileName, int LineNumber );
void CleanCMemory       ( void );
void ApplyCheats    ( void );
void RunRsp	            ( void );
void RefreshScreen      ( void );
void ResetX86Logs       ( void );
void SyncToPC           ( void );
BOOL EndEmulation       ( void );
void SetEndEmulation    ( BOOL End );
void CloseSaveChips     ( void );


//TLB Functions
BOOL TranslateVaddr     ( DWORD VAddr, DWORD  * PAddr); 
void TLB_ReadEntry      ( void );
void TLB_WriteEntry     ( int index, BOOL Random );

BOOL ClearRecompCodeInitialCode ( void );
BOOL ClearRecompCodeProtectMem  ( DWORD PhysicalAddress, int length );


//Timer functions
void ResetTimer ( void );
void GenerateProfileLog ( void );
DWORD StartTimer ( DWORD Address );
DWORD StopTimer  ( void );


//registers 
#ifdef toremove
extern MIPS_DWORD * g_GPR, * g_FPR, * g_HI, * g_LO;
extern DWORD              * g_PROGRAM_COUNTER, * g_CP0, * g_RegMI, * g_LLBit, 
		 * g_LLAddr, * g_FPCR, * g_RegSI, * g_RegRI, * g_RegPI, * g_RegAI,
		* g_RegVI, * g_RegDPC, * g_RegSP, * g_RegRDRAM;
extern double ** g_FPRDoubleLocation;
extern float  ** g_FPRFloatLocation;
extern int * g_Timer;
#endif

//settings
extern BOOL g_ShowUnhandledMemory, g_ShowCPUPer, g_ShowTLBMisses, g_UseTlb, 
	g_HaveDebugger, g_AudioSignal, g_ShowDListAListCount, g_ShowPifRamErrors,
	g_GenerateLog, g_DelaySI, g_DisableRegCaching, g_ShowCompMem,
	g_UseLinking, g_FixedAudio, g_LogX86Code;
extern DWORD g_RomFileSize, g_CountPerOp;
extern enum CPU_TYPE g_CPU_Type;
extern enum SAVE_CHIP_TYPE g_SaveUsing;
extern enum CICChip g_CicChip;
extern enum FUNC_LOOKUP_METHOD g_LookUpMode;
extern char g_RomName [300];

//Plugins
extern DWORD * g_AudioIntrReg;
extern CONTROL * g_Controllers;
extern enum SystemType g_SystemType;

//Memory
extern DWORD * g_TLB_ReadMap, * g_TLB_WriteMap, g_RdramSize, g_SystemRdramSize;

//Misc
enum { NoOfFrames = 7 };
extern DWORD g_CurrentFrame;
extern QWORD g_Frequency, g_Frames[NoOfFrames], g_LastFrame;

typedef struct {
	HANDLE hStepping;

	int  InterruptFlag;
	BOOL SoftReset;
	BOOL GenerateInterrupt;
	BOOL DoSomething;
	BOOL CloseCPU;
	BOOL ChangeWindow;
	BOOL GSButton;
	BOOL CheckInterrupts;
	BOOL Pause;
	BOOL SaveState;
	BOOL RestoreState;
	BOOL DoInterrupt;
	BOOL InterruptExecuted;
	BOOL Stepping;
	BOOL ChangePlugin;
	BOOL ProfileStartStop;
	BOOL ProfileResetStats;
	BOOL ProfileGenerateLogs;
	BOOL GameSpeed_Increase;
	BOOL GameSpeed_Decrease;
	BOOL DMAUsed;
} CPU_ACTION;

extern CPU_ACTION  *  g_CPU_Action;
extern CPU_ACTION  *  g_Main_CPU_Action;

#ifdef __cplusplus
}
#endif