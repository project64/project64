#include <windows.h>

#include "Plugin.h"
#include "c core.h"
#include "C Global Variable.h"

#include "..\\Types.h"
#include "Interpreter CPU.h"
#include "Recompiler CPU.h"
#include "c memory.h"
#include "Registers.h"
#include "Exception.h"
#include "r4300i Commands.h"
#include "TLB.h"
#include "DMA.h"
#include "eeprom.h"
#include "sram.h"
#include "flashram.h"
#include "mempak.h"
#include "pif.h"
#include "Sync Cpu.h"

//extern int NextInstruction/*, ManualPaused*/;
extern DWORD JumpToLocation;
extern BOOL TestTimer;

/*#define MaxTimers	5
#define CompareTimer			0
#define SiTimer					1
#define PiTimer					2
#define ViTimer					3
#define RspTimer				4

typedef struct {
	int  NextTimer[MaxTimers];
	BOOL Active[MaxTimers];
	int  CurrentTimerType;
	int  Timer;
} SYSTEM_TIMERS;

extern SYSTEM_TIMERS Timers;*/
//extern DWORD MemoryStack;

/*extern BOOL HaveDebugger, AutoLoadMapFile,  
	AutoStart, 
	AutoSleep, UseIni, RomBrowser,
	IgnoreMove, Rercursion, ShowCPUPer, AutoZip, 
	AutoFullScreen, SystemABL, AlwaysOnTop, BasicMode, RememberCheats,AudioSignal;

void ChangeCompareTimer ( void );
void CheckTimer         ( void );
void TimerDone          ( void );
*/

void DoSomething        ( void );
int  DelaySlotEffectsCompare ( DWORD PC, DWORD Reg1, DWORD Reg2 );
int  DelaySlotEffectsJump (DWORD JumpPC);
void InPermLoop         ( void );
void DisplayFPS         ( void );
