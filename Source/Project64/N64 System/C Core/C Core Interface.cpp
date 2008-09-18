#include "C Core.h"
#include "eeprom.h"
#include "mempak.h"
extern "C" {
#include "Plugin.h"
#include "Logging.h"
#include "Interpreter CPU.h"
#include "Recompiler CPU.h"
#include "CPU Log.h"
#include "sram.h"
#include "flashram.h"

enum STEP_TYPE NextInstruction, Main_NextInstruction, Sync_NextInstruction;

}

CN64System    * g_N64System  = NULL;
CN64System    * g_SyncSystem = NULL;
CRecompiler   * g_Recompiler = NULL;
CMipsMemory   * g_MMU        = NULL; //Memory of the n64 
CTLB          * g_TLB        = NULL; //TLB Unit
CRegisters    * g_Reg        = NULL; //Current Register Set attacted to the _MMU
CNotification * g_Notify     = NULL;   
CSettings     * g_Settings   = NULL;   
CPlugins      * g_Plugins    = NULL;
CN64Rom       * g_Rom        = NULL;      //The current rom that this system is executing.. it can only execute one file at the time
CAudio        * g_Audio      = NULL;

//registers 
MULTI_ACCESS_QWORD * g_GPR = NULL, * g_FPR = NULL, * g_HI = NULL, * g_LO = NULL;
DWORD              * g_PROGRAM_COUNTER = NULL, * g_CP0 = NULL, * g_RegMI = NULL, * g_LLBit = NULL,
		* g_LLAddr = NULL, * g_FPCR = NULL, * g_RegSI = NULL, * g_RegRI = NULL, * g_RegPI = NULL, 
		* g_RegAI = NULL, * g_RegVI = NULL, * g_RegDPC = NULL, * g_RegSP = NULL, * g_RegRDRAM = NULL;
double ** g_FPRDoubleLocation;
float  ** g_FPRFloatLocation;
enum TimerType * g_CurrentTimerType;
int * g_Timer = NULL;

//Register Name
const char ** g_Cop0_Name;

//settings
BOOL g_ShowUnhandledMemory = false, g_ShowCPUPer = false, g_ShowTLBMisses = false, g_UseTlb = true, 
	g_HaveDebugger = false, g_AudioSignal = false, g_ShowDListAListCount = false, 
	g_ShowPifRamErrors = false, g_GenerateLog = false, g_DelaySI = false, g_SPHack = false, 
	g_DisableRegCaching = false, g_ShowCompMem = false, g_UseLinking = false,
	g_FixedAudio = false, g_LogX86Code = false;
DWORD g_RomFileSize = 0, g_CountPerOp = 2;
enum CPU_TYPE g_CPU_Type;
enum SAVE_CHIP_TYPE g_SaveUsing;
enum CICChip g_CicChip;
enum FUNC_LOOKUP_METHOD g_LookUpMode;
char g_RomName [300];

//Plugins
DWORD * g_AudioIntrReg = NULL;
CONTROL * g_Controllers;
enum SystemType g_SystemType;

/******** All DLLs have this function **************/
void (__cdecl *GetDllInfo)             ( PLUGIN_INFO * PluginInfo );

/********** RSP DLL: Functions *********************/
//void (__cdecl *GetRspDebugInfo)    ( RSPDEBUG_INFO * DebugInfo );
//void (__cdecl *RSPCloseDLL)        ( void );
//void (__cdecl *RSPDllAbout)        ( HWND hWnd );
//void (__cdecl *RSPDllConfig)       ( HWND hWnd );
//void (__cdecl *RSPRomClosed)       ( void );
DWORD (__cdecl *DoRspCycles)       ( DWORD );
//void (__cdecl *InitiateRSP_1_0)    ( RSP_INFO_1_0 Rsp_Info, DWORD * Cycles);
//void (__cdecl *InitiateRSP_1_1)    ( RSP_INFO_1_1 Rsp_Info, DWORD * Cycles);
//void (__cdecl *InitiateRSPDebugger)( DEBUG_INFO DebugInfo);

/********** GFX DLL: Functions *********************/
void (__cdecl *CaptureScreen)      ( const char * );
void (__cdecl *ChangeWindow)       ( void );
//void (__cdecl *GetGfxDebugInfo)    ( GFXDEBUG_INFO * GFXDebugInfo );
//void (__cdecl *GFXCloseDLL)        ( void );
//void (__cdecl *GFXDllAbout)        ( HWND hParent );
//void (__cdecl *GFXDllConfig)       ( HWND hParent );
//void (__cdecl *GfxRomClosed)       ( void );
//void (__cdecl *GfxRomOpen)         ( void );
void (__cdecl *DrawScreen)         ( void );
//void (__cdecl *FrameBufferRead)    ( DWORD addr );
//void (__cdecl *FrameBufferWrite)   ( DWORD addr, DWORD Bytes );
//BOOL (__cdecl *InitiateGFX)        ( GFX_INFO Gfx_Info );
//void (__cdecl *InitiateGFXDebugger)( DEBUG_INFO DebugInfo);
void (__cdecl *MoveScreen)         ( int xpos, int ypos );
void (__cdecl *ProcessDList)       ( void );
void (__cdecl *ProcessRDPList)     ( void );
void (__cdecl *ShowCFB)			   ( void );
void (__cdecl *UpdateScreen)       ( void );
void (__cdecl *ViStatusChanged)    ( void );
void (__cdecl *ViWidthChanged)     ( void );

/************ Audio DLL: Functions *****************/
//void (__cdecl *AiCloseDLL)       ( void );
//void (__cdecl *AiDacrateChanged) ( int SystemType );
void (__cdecl *AiLenChanged)     ( void );
//void (__cdecl *AiDllAbout)       ( HWND hParent );
//void (__cdecl *AiDllConfig)      ( HWND hParent );
//void (__cdecl *AiDllTest)        ( HWND hParent );
DWORD (__cdecl *AiReadLength)    ( void );
//void (__cdecl *AiRomClosed)      ( void );
//void (__cdecl *AiUpdate)         ( BOOL Wait );
//BOOL (__cdecl *InitiateAudio)    ( AUDIO_INFO Audio_Info );
void (__cdecl *ProcessAList)     ( void );

/********** Controller DLL: Functions **************/
//void (__cdecl *ContCloseDLL)     ( void );
void (__cdecl *ControllerCommand)( int Control, BYTE * Command );
//void (__cdecl *ContDllAbout)     ( HWND hParent );
//void (__cdecl *ContConfig)       ( HWND hParent );
//void (__cdecl *InitiateControllers_1_0)( HWND hMainWindow, CONTROL Controls[4] );
//void (__cdecl *InitiateControllers_1_1)( CONTROL_INFO ControlInfo );
void (__cdecl *GetKeys)          ( int Control, BUTTONS * Keys );
void (__cdecl *ReadController)   ( int Control, BYTE * Command );
//void (__cdecl *ContRomOpen)      ( void );
//void (__cdecl *ContRomClosed)    ( void );
//void (__cdecl *WM_KeyDown)       ( WPARAM wParam, LPARAM lParam );
//void (__cdecl *WM_KeyUp)         ( WPARAM wParam, LPARAM lParam );
void (__cdecl *RumbleCommand)	 ( int Control, BOOL bRumble );

//Memory
DWORD * g_TLB_ReadMap, * g_TLB_WriteMap, g_RdramSize, * g_HalfLine, * g_MemoryStack;
BYTE * g_N64MEM, *g_RDRAM, *g_DMEM, *g_IMEM, *g_ROM, *g_PIF_Ram;

OPCODE        g_Opcode;
BOOL          g_IndvidualBlock, g_Profiling;
DWORD g_CurrentFrame;
QWORD g_Frequency, g_Frames[NoOfFrames], g_LastFrame;


CPU_ACTION * g_CPU_Action = NULL;
CPU_ACTION * g_Main_CPU_Action = NULL;
CPU_ACTION * g_Sync_CPU_Action = NULL;

void CC_Core::SetN64System (CN64System * N64System)
{
	if (g_Main_CPU_Action)
	{
		delete g_Main_CPU_Action;
		g_Main_CPU_Action  = NULL;
	}
	Main_NextInstruction = NORMAL;
	g_Main_CPU_Action = new CPU_ACTION;
	memset(g_Main_CPU_Action,0,sizeof(CPU_ACTION));

	if (N64System)
	{
		g_RomFileSize = N64System->_Rom->GetRomSize();
		g_CicChip            = N64System->_Rom->CicChipID();
	}
	g_CurrentFrame       = 0;
	SetCurrentSystem(N64System);
}

void CC_Core::SetSyncCpu   ( CN64System * System )
{
	if (g_Sync_CPU_Action)
	{
		delete g_Sync_CPU_Action;
		g_Sync_CPU_Action = NULL;
	}
	Sync_NextInstruction = NORMAL;
	if (System)
	{
		g_Sync_CPU_Action = new CPU_ACTION;
		memset(g_Sync_CPU_Action,0,sizeof(CPU_ACTION));
	}
	g_SyncSystem = System;
}

void CC_Core::SetSettings  ( )
{
	g_Settings  = _Settings;
	if (g_Settings)
	{
		g_HaveDebugger        = g_Settings->LoadDword(Debugger);
		if (g_HaveDebugger)
		{
			g_ShowUnhandledMemory = g_Settings->LoadDword(ShowUnhandledMemory);
			g_ShowDListAListCount = g_Settings->LoadDword(ShowDListAListCount);
		} else {
			g_ShowUnhandledMemory = false; 
			g_ShowUnhandledMemory = false;

		}
		g_ShowCPUPer          = g_Settings->LoadDword(ShowCPUPer);
		g_ShowTLBMisses       = false;
		g_UseTlb              = g_Settings->LoadDword(UseTLB);
		g_CPU_Type            = (CPU_TYPE)g_Settings->LoadDword(CPUType);
		g_SaveUsing           = (SAVE_CHIP_TYPE)g_Settings->LoadDword(SaveChipType);
		g_AudioSignal         = g_Settings->LoadDword(AudioSignal);
		g_RdramSize           = g_Settings->LoadDword(RamSize);
		g_ShowPifRamErrors    = g_Settings->LoadDword(ShowPifRamErrors);
		g_CountPerOp          = g_Settings->LoadDword(CounterFactor);
		g_GenerateLog         = g_Settings->LoadDword(GenerateDebugLog);
		g_DelaySI             = g_Settings->LoadDword(DelaySI);
		g_SPHack              = g_Settings->LoadDword(ROM_SPHack);
		g_FixedAudio          = g_Settings->LoadDword(ROM_FixedAudio);
		g_LogX86Code          = g_Settings->LoadDword(GenerateLogFiles);
		g_LookUpMode          = (FUNC_LOOKUP_METHOD)g_Settings->LoadDword(FuncLookupMode);
		g_DisableRegCaching   = !g_Settings->LoadDword(ROM_RegCache);
		g_UseLinking          = g_Settings->LoadDword(BlockLinking);
		g_ShowCompMem         = false;
		strcpy(g_RomName, g_Settings->LoadString(ROM_NAME).c_str());
	}
}

void CC_Core::SetCurrentSystem (CN64System * System )
{
	g_MMU        = NULL;
	g_Reg        = NULL;
	g_TLB        = NULL;
	g_Audio      = NULL;
	g_Recompiler = NULL;

	g_N64System = System;
	g_Notify    = System->_Notify;

	if (g_SyncSystem == System)
	{ 
		Main_NextInstruction = NextInstruction;
		g_CPU_Action = g_Sync_CPU_Action;
		NextInstruction = Sync_NextInstruction;
	} else {
		Sync_NextInstruction = NextInstruction;
		g_CPU_Action = g_Main_CPU_Action;
		NextInstruction = Main_NextInstruction;
	}
	if (g_N64System) 
	{ 
		g_Recompiler = System->_Recomp;
		g_MMU        = System->_MMU; 
		g_TLB        = System->_MMU;
		g_Plugins    = System->_Plugins;
		g_Rom        = System->_Rom;
		g_Audio      = System->_Audio;
	}
	if (g_MMU)       { g_Reg = g_MMU->SystemRegisters(); }
	if (g_Reg)
	{
		g_GPR             = g_Reg->GPR;
		g_CP0             = g_Reg->CP0;
		g_FPR             = g_Reg->FPR;
		g_FPCR            = g_Reg->FPCR;
		g_FPRFloatLocation  = g_Reg->FPR_S;
		g_FPRDoubleLocation = g_Reg->FPR_D;
		g_HI              = &g_Reg->HI;
		g_LO              = &g_Reg->LO;
		g_LLBit           = &g_Reg->LLBit;
		g_LLAddr          = &g_Reg->LLAddr;
		g_RegRI           = g_Reg->RDRAM_Interface;
		g_RegRDRAM        = g_Reg->RDRAM_Registers;
		g_RegMI           = g_Reg->Mips_Interface;
		g_RegVI           = g_Reg->Video_Interface;
		g_RegDPC          = g_Reg->Display_ControlReg;
		g_RegAI           = g_Reg->Audio_Interface;
		g_RegSP           = g_Reg->SigProcessor_Interface;
		g_RegPI           = g_Reg->Peripheral_Interface;
		g_RegSI           = g_Reg->SerialInterface;
		g_AudioIntrReg    = &g_Reg->AudioIntrReg;
		g_PROGRAM_COUNTER = &g_Reg->PROGRAM_COUNTER;
		g_Cop0_Name       = g_Reg->Cop0_Name;
		g_Timer           = &g_Reg->Timer;
		g_CurrentTimerType = &g_Reg->CurrentTimerType;
	}

	CaptureScreen       = g_Plugins->Gfx()->CaptureScreen;
	ChangeWindow        = g_Plugins->Gfx()->ChangeWindow;
//	GetGfxDebugInfo     = g_Plugins->Gfx()->GetGfxDebugInfo;
//	GFXCloseDLL         = g_Plugins->Gfx()->GFXCloseDLL;
//	GFXDllAbout         = g_Plugins->Gfx()->GFXDllAbout;
//	GFXDllConfig        = g_Plugins->Gfx()->GFXDllConfig;
//	GfxRomClosed        = g_Plugins->Gfx()->GfxRomClosed;
//	GfxRomOpen          = g_Plugins->Gfx()->GfxRomOpen;
	DrawScreen          = g_Plugins->Gfx()->DrawScreen;
//	FrameBufferRead     = g_Plugins->Gfx()->FrameBufferRead;
//	FrameBufferWrite    = g_Plugins->Gfx()->FrameBufferWrite;
//	InitiateGFX         = g_Plugins->Gfx()->InitiateGFX;
//	InitiateGFXDebugger = g_Plugins->Gfx()->InitiateGFXDebugger;
	MoveScreen          = g_Plugins->Gfx()->MoveScreen;
	ProcessDList        = g_Plugins->Gfx()->ProcessDList;
	ProcessRDPList      = g_Plugins->Gfx()->ProcessRDPList;
	ShowCFB             = g_Plugins->Gfx()->ShowCFB;
	UpdateScreen        = g_Plugins->Gfx()->UpdateScreen;
	ViStatusChanged     = g_Plugins->Gfx()->ViStatusChanged;
	ViWidthChanged      = g_Plugins->Gfx()->ViWidthChanged;

//	ContCloseDLL        = g_Plugins->Control()->ContCloseDLL;
	ControllerCommand   = g_Plugins->Control()->ControllerCommand;
//	ContDllAbout        = g_Plugins->Control()->ContDllAbout;
//	ContConfig          = g_Plugins->Control()->ContConfig;
//	InitiateControllers_1_0= g_Plugins->Control()->InitiateControllers_1_0;
//	InitiateControllers_1_1= g_Plugins->Control()->InitiateControllers_1_1;
	GetKeys             = g_Plugins->Control()->GetKeys;
	ReadController      = g_Plugins->Control()->ReadController;
//	ContRomOpen         = g_Plugins->Control()->ContRomOpen;
//	ContRomClosed       = g_Plugins->Control()->ContRomClosed;
//	WM_KeyDown          = g_Plugins->Control()->WM_KeyDown;
//	WM_KeyUp            = g_Plugins->Control()->WM_KeyUp;
	RumbleCommand       = g_Plugins->Control()->RumbleCommand;
	g_Controllers       = g_Plugins->Control()->m_PluginControllers;

//	GetRspDebugInfo     = g_Plugins->RSP()->GetRspDebugInfo;
//	RSPCloseDLL         = g_Plugins->RSP()->RSPCloseDLL;
//	RSPDllAbout         = g_Plugins->RSP()->RSPDllAbout;
//	RSPDllConfig        = g_Plugins->RSP()->RSPDllConfig;
//	RSPRomClosed        = g_Plugins->RSP()->RSPRomClosed;
	DoRspCycles         = g_Plugins->RSP()->DoRspCycles;
//	InitiateRSP_1_0     = g_Plugins->RSP()->InitiateRSP_1_0;
//	InitiateRSP_1_1     = g_Plugins->RSP()->InitiateRSP_1_1;
//	InitiateRSPDebugger = g_Plugins->RSP()->InitiateRSPDebugger;
	
//	AiCloseDLL          = g_Plugins->Audio()->AiCloseDLL;
//	AiDacrateChanged    = g_Plugins->Audio()->AiDacrateChanged;
	AiLenChanged        = g_Plugins->Audio()->LenChanged;
//	AiDllAbout          = g_Plugins->Audio()->AiDllAbout;
//	AiDllConfig         = g_Plugins->Audio()->AiDllConfig;
//	AiDllTest           = g_Plugins->Audio()->AiDllTest;
	AiReadLength        = g_Plugins->Audio()->ReadLength;
//	AiRomClosed         = g_Plugins->Audio()->AiRomClosed;
//	AiUpdate            = g_Plugins->Audio()->Update;
//	InitiateAudio       = g_Plugins->Audio()->InitiateAudio;
	ProcessAList        = g_Plugins->Audio()->ProcessAList;

	g_N64MEM            = System->_MMU->RDRAM;
	g_RDRAM             = System->_MMU->RDRAM;
	g_DMEM              = System->_MMU->DMEM;
	g_IMEM              = System->_MMU->IMEM;
	g_ROM               = System->_MMU->ROM;
	g_PIF_Ram           = System->_MMU->PIF_Ram;
	g_TLB_ReadMap       = System->_MMU->TLB_ReadMap;
	g_TLB_WriteMap      = System->_MMU->TLB_WriteMap;
	g_HalfLine          = &System->_MMU->m_HalfLine;
	g_MemoryStack       = &System->_MMU->m_MemoryStack;
}

void CC_Core::PauseExecution ( void )
{
	g_N64System->Pause();
}

void CC_Core::RunRsp ( void )
{
	try
	{
		g_N64System->RunRSP();
	} 
	catch (...)
	{
		char Message[600];
		sprintf(Message,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
		MessageBox(NULL,Message,"Exception",MB_OK);
	}
}

void CC_Core::RefreshScreen(void)
{
	try
	{
		g_N64System->RefreshScreen();
	} 
	catch (...)
	{
		WriteTraceF(TraceError,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
	}
}

void CC_Core::GenerateProfileLog ( void )
{
	g_N64System->m_Profile.GenerateLog();
}

void CC_Core::ResetTimer ( void )
{
	g_N64System->m_Profile.ResetCounters();
}

DWORD CC_Core::StartTimer ( DWORD Address )
{
	return g_N64System->m_Profile.StartTimer(Address);
}

DWORD CC_Core::StopTimer ( void )
{
	return g_N64System->m_Profile.StopTimer();
}

void PauseExecution ( void )
{
	CC_Core::PauseExecution();
}

void DisplayError ( const char * Message, ... )
{
	if (g_Notify == NULL) { return; }

	va_list ap;
	va_start( ap, Message );
	g_Notify->DisplayError(Message,ap);
}

void DisplayMessage  ( int DisplayTime, const char * Message, ... )
{
	if (g_Notify == NULL) { return; }
	
	va_list ap;
	va_start( ap, Message );
	g_Notify->DisplayMessage(DisplayTime, Message,ap);
}

void DisplayMessage2 ( const char * Message, ... )
{
	if (g_Notify == NULL) { return; }

	va_list ap;
	va_start( ap, Message );
	g_Notify->DisplayMessage2(Message,ap);
}

const char * GetAppName ( void )
{
	static stdstr szAppName = g_Settings->LoadString(ApplicationName);
	return szAppName.c_str();
}

void GetAutoSaveDir( char * Directory ) 
{
	SettingID Dir = g_Settings->LoadDword(UseSaveDirSelected) ? SelectedSaveDirectory : InitialSaveDirectory ;
	strcpy(Directory,g_Settings->LoadString(Dir).c_str());
}

void GetInstantSaveDir( char * Directory ) 
{
	SettingID Dir = g_Settings->LoadDword(UseInstantDirSelected) ? SelectedInstantSaveDirectory : InitialInstantSaveDirectory ;
	strcpy(Directory,g_Settings->LoadString(Dir).c_str());
}

void SetFpuLocations( void ) 
{
	g_Reg->FixFpuLocations();
}

BOOL Limit_FPS ( void )
{
	return g_Settings->LoadDword(LimitFPS);
}

void DacrateChanged ( enum SystemType Type )
{
	g_Plugins->Audio()->DacrateChanged(Type);
}

BOOL Close_C_CPU ( void )
{
	if (g_Settings == NULL || !g_Settings->LoadDword(CPU_Running))
	{
		return true;
	}
	SetEndEmulation(true);
	g_Main_CPU_Action->DoSomething = true;
	g_Main_CPU_Action->CloseCPU = true;
	return false;
}

void StopEmulation ( void )
{
	g_N64System->CloseCpu();
}

void CleanCMemory ( void )
{
	if ( g_Main_CPU_Action )
	{
		delete g_Main_CPU_Action;
		g_Main_CPU_Action = NULL;
	}
	if (g_Sync_CPU_Action)
	{
		delete g_Sync_CPU_Action;
		g_Sync_CPU_Action = NULL;
	}
}

void __stdcall UpdateSyncCPU      ( DWORD const Cycles )
{
	g_N64System->UpdateSyncCPU(g_SyncSystem,Cycles);
}

void RunRsp( void ) 
{
	CC_Core::RunRsp();
}

void RefreshScreen( void ) 
{
	CC_Core::RefreshScreen();
}

void ExecuteCycles(DWORD Cycles)
{
	ExecuteInterpreterOps(Cycles);
}

void SyncSystem (void)
{
	g_N64System->SyncCPU(g_SyncSystem);
}

void ChangeTimer        ( enum TimerType Type, int Value )
{
	if (Value == 0)
	{
		g_Reg->DeactiateTimer(Type);
	} else 
	{
		g_Reg->ChangeTimerFixed(Type,Value); 
	}
}

void ChangeTimerRelative ( enum TimerType Type, int Value )
{
	g_Reg->ChangeTimerRelative(Type,Value); 
}

void ApplyGSButtonCheats ( void )
{
	CC_Core::ApplyGSButtonCheats(g_N64System);
}

void ChangePluginFunc ( void )
{
	g_Notify->DisplayMessage(0,MSG_PLUGIN_INIT);
	if (g_Settings->LoadDword(GFX_PluginChanged))
	{
		g_Plugins->Reset(PLUGIN_TYPE_GFX);
	}
	if (g_Settings->LoadDword(AUDIO_PluginChanged))
	{
		g_Plugins->Reset(PLUGIN_TYPE_AUDIO);
	}	
	if (g_Settings->LoadDword(CONT_PluginChanged))
	{
		g_Plugins->Reset(PLUGIN_TYPE_CONTROLLER);
	}	
	if (g_Settings->LoadDword(RSP_PluginChanged) || 
		g_Settings->LoadDword(AUDIO_PluginChanged) || 
		g_Settings->LoadDword(GFX_PluginChanged))
	{
		g_Plugins->Reset(PLUGIN_TYPE_RSP);
	}
	g_Settings->SaveDword(RSP_PluginChanged,  (DWORD)false);
	g_Settings->SaveDword(AUDIO_PluginChanged,(DWORD)false);
	g_Settings->SaveDword(GFX_PluginChanged,  (DWORD)false);
	g_Settings->SaveDword(CONT_PluginChanged, (DWORD)false);

	g_Notify->RefreshMenu();
	if (!g_Plugins->Initiate(g_N64System)) {
		g_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
		SetEndEmulation(true);
	} else {
		CC_Core::SetCurrentSystem(g_N64System);
	}
	g_Recompiler->ResetRecompCode();
}

void ChangeFullScreenFunc ( void )
{
	g_Notify->ChangeFullScreen();
}

BOOL Machine_LoadState ( void )
{
	bool Result = CC_Core::LoadState(g_N64System);
	CC_Core::SetCurrentSystem(g_N64System);
	return Result;
}

void ResetTimer ( void )
{
	CC_Core::ResetTimer();
}

void GenerateProfileLog ( void )
{
	CC_Core::GenerateProfileLog();
}

DWORD StartTimer ( DWORD Address )
{
	return CC_Core::StartTimer(Address);
}

DWORD StopTimer ( void )
{
	return CC_Core::StopTimer();
}

BOOL Machine_SaveState ( void )
{
	return CC_Core::SaveState(g_N64System);
}

void BreakPoint(LPCSTR FileName, int LineNumber )
{
	if (g_Notify)
	{
		g_Notify->BreakPoint(FileName,LineNumber);
	}
}

void UpdateCurrentHalfLine (void)
{
    if (*g_Timer < 0) { 
		*g_HalfLine = 0;
		return;
	}
	//DisplayError("Timer: %X",Timers.Timer);
	//HalfLine = (Timer / 1500) + VI_INTR_REG;
	*g_HalfLine = (DWORD)(*g_Timer / 1500);
	*g_HalfLine &= ~1;
//	*g_HalfLine += ViFieldNumber;
	//Timers.Timer -= 1500;
}

void CC_Core::ApplyGSButtonCheats (CN64System * System)
{
	if (System == NULL || System->_Cheats == NULL)
	{
		return;
	}
	if (System->_Cheats->CheatsSlectionChanged())
	{
		System->_Cheats->LoadCheats(false);
	}
	System->_Cheats->ApplyGSButton(System->_MMU);
}

void CC_Core::ApplyCheats (CN64System * System)
{
	if (System == NULL || System->_Cheats == NULL)
	{
		return;
	}
	if (System->_Cheats->CheatsSlectionChanged())
	{
		System->_Cheats->LoadCheats(false);
	}
	System->_Cheats->ApplyCheats(System->_MMU);
}

void ApplyCheats (void)
{
	CC_Core::ApplyCheats(g_N64System);
}

void ResetX86Logs ( void )
{
	if (g_LogX86Code)
	{
		Stop_x86_Log();
		Start_x86_Log();
	}
}

BOOL EndEmulation       ( void )
{
	return g_N64System->EndEmulation;
}

void SetEndEmulation    ( BOOL End )
{
	g_N64System->EndEmulation = End != 0;
}

void CloseSaveChips ( void )
{
	CloseEeprom();
	CloseMempak();
	CloseSram();
	CloseFlashRam();
}

BOOL TranslateVaddr( DWORD VAddr, DWORD * PAddr )
{
#ifdef _DEBUG
	if (PAddr == NULL)
	{
		BreakPoint(__FILE__,__LINE__);
	}
#endif
	return g_TLB->TranslateVaddr(VAddr,*PAddr);
}

BOOL AddressDefined     ( DWORD VAddr )
{
	return g_TLB->TLB_AddressDefined(VAddr);
}

void TLB_ReadEntry      ( void )
{
	g_TLB->TLB_ReadEntry();
}

void TLB_WriteEntry( int index, BOOL Random )
{
	if (index > 31)
	{
		BreakPoint(__FILE__,__LINE__);
	}
	g_TLB->TLB_WriteEntry(index,Random != 0);
}

void TLB_Probe()
{
	g_TLB->TLB_Probe();
}

void SyncToPC (void) {
	FixRandomReg();
	SyncSystem ();
}

BOOL ClearRecompCodeProtectMem ( DWORD Address, int length )
{
	if (g_Recompiler)
	{
		return g_Recompiler->ClearRecompCode_Phys(Address,length,CRecompiler::Remove_ProtectedMem);
	}
	return false;
}

BOOL ClearRecompCodeInitialCode ( void )
{
	if (g_Recompiler)
	{
		return g_Recompiler->ClearRecompCode_Virt(0x80000000,0x200,CRecompiler::Remove_InitialCode);
	}
	return false;
}