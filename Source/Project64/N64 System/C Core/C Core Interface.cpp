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

#ifdef toremove
CN64System    * _N64System  = NULL;
CN64System    * _SyncSystem = NULL;
CRecompiler   * _Recompiler = NULL;
CMipsMemoryVM * _MMU        = NULL; //Memory of the n64 
CTLB          * _TLB        = NULL; //TLB Unit
CRegisters    * _Reg        = NULL; //Current Register Set attacted to the _MMU
CNotification * _Notify     = NULL;   
CSettings     * _Settings   = NULL;   
CPlugins      * _Plugins    = NULL;
CN64Rom       * _Rom        = NULL;      //The current rom that this system is executing.. it can only execute one file at the time

//registers 
MIPS_DWORD * _GPR = NULL, * _FPR = NULL, * g_HI = NULL, * g_LO = NULL;
DWORD              * _PROGRAM_COUNTER = NULL, * _CP0 = NULL, * _RegMI = NULL, * _LLBit = NULL,
		* _LLAddr = NULL, * _FPCR = NULL, * _RegSI = NULL, * _RegRI = NULL, * _RegPI = NULL, 
		* _RegAI = NULL, * _RegVI = NULL, * _RegDPC = NULL, * _RegSP = NULL, * _RegRDRAM = NULL;
double ** _FPRDoubleLocation;
float  ** _FPRFloatLocation;
int * _Timer = NULL;
#endif

//settings
BOOL g_ShowUnhandledMemory = false, g_ShowCPUPer = false, g_ShowTLBMisses = false, g_UseTlb = true, 
	g_HaveDebugger = false, g_AudioSignal = false, g_ShowDListAListCount = false, 
	g_ShowPifRamErrors = false, g_GenerateLog = false, g_DelaySI = false, 
	g_DisableRegCaching = false, g_ShowCompMem = false, g_UseLinking = false,
	g_FixedAudio = false, g_LogX86Code = false;
DWORD g_RomFileSize = 0, g_CountPerOp = 2, g_ViRefreshRate = 1500;
enum CPU_TYPE g_CPU_Type;
enum SAVE_CHIP_TYPE g_SaveUsing;
enum CICChip g_CicChip;
enum FUNC_LOOKUP_METHOD g_LookUpMode;
char g_RomName [300];

//Plugins
DWORD * _AudioIntrReg = NULL;
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
DWORD * g_TLB_ReadMap, * g_TLB_WriteMap, g_RdramSize;

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
		g_RomFileSize = _Rom->GetRomSize();
		g_CicChip     = _Rom->CicChipID();
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
	_SyncSystem = System;
}

void CC_Core::SetSettings  ( )
{
	_Settings  = _Settings;
	if (_Settings)
	{
		g_HaveDebugger        = _Settings->LoadBool(Debugger_Enabled);
		if (g_HaveDebugger)
		{
			g_ShowUnhandledMemory = _Settings->LoadBool(Debugger_ShowUnhandledMemory);
			g_ShowDListAListCount = _Settings->LoadBool(Debugger_ShowDListAListCount);
		} else {
			g_ShowUnhandledMemory = false; 
			g_ShowUnhandledMemory = false;

		}
		g_ShowCPUPer          = _Settings->LoadBool(UserInterface_ShowCPUPer);
		g_ShowTLBMisses       = false;
		g_UseTlb              = _Settings->LoadBool(Game_UseTlb);
		g_CPU_Type            = (CPU_TYPE)_Settings->LoadDword(Game_CpuType);
		g_SaveUsing           = (SAVE_CHIP_TYPE)_Settings->LoadDword(Game_SaveChip);
		g_AudioSignal         = _Settings->LoadBool(Game_RspAudioSignal);
		g_RdramSize           = _Settings->LoadDword(Game_RDRamSize);
		g_ShowPifRamErrors    = _Settings->LoadDword(Debugger_ShowPifErrors);
		g_CountPerOp          = _Settings->LoadDword(Game_CounterFactor);
		g_GenerateLog         = _Settings->LoadDword(Debugger_GenerateDebugLog);
		g_DelaySI             = _Settings->LoadBool(Game_DelaySI);
		g_FixedAudio          = _Settings->LoadBool(Game_FixedAudio);
		g_LogX86Code          = _Settings->LoadBool(Debugger_GenerateLogFiles);
		g_LookUpMode          = (FUNC_LOOKUP_METHOD)_Settings->LoadDword(Game_FuncLookupMode);
		g_DisableRegCaching   = !_Settings->LoadBool(Game_RegCache);
		g_UseLinking          = _Settings->LoadBool(Game_BlockLinking);
		g_ShowCompMem         = false;
		g_ViRefreshRate       = _Settings->LoadDword(Game_ViRefreshRate);
		strcpy(g_RomName, _Settings->LoadString(Game_GameName).c_str());
	}
}

void CC_Core::SetCurrentSystem (CN64System * System )
{
	_MMU        = NULL;
	_Reg        = NULL;
	_TLB        = NULL;
	_Audio      = NULL;
	_Recompiler = NULL;

	_N64System = System;

	if (_SyncSystem == System)
	{ 
		Main_NextInstruction = NextInstruction;
		g_CPU_Action = g_Sync_CPU_Action;
		NextInstruction = Sync_NextInstruction;
	} else {
		Sync_NextInstruction = NextInstruction;
		g_CPU_Action = g_Main_CPU_Action;
		NextInstruction = Main_NextInstruction;
	}
	if (_N64System) 
	{ 
		_Recompiler = System->m_Recomp;
		_MMU        = &System->m_MMU_VM; 
		_TLB        = &System->m_TLB;
		_Plugins    = System->m_Plugins;
		_Audio      = &System->m_Audio;
		_Reg        = &System->m_Reg;
	}
	if (_Reg)
	{
		_GPR             = _Reg->m_GPR;
		_CP0             = _Reg->m_CP0;
		_FPR             = _Reg->m_FPR;
		_FPCR            = _Reg->m_FPCR;
		_FPRFloatLocation  = _Reg->m_FPR_S;
		_FPRDoubleLocation = _Reg->m_FPR_D;
		_RegHI              = &_Reg->m_HI;
		_RegLO              = &_Reg->m_LO;
		_LLBit           = &_Reg->m_LLBit;
		_LLAddr          = &_Reg->m_LLAddr;
		_RegRI           = _Reg->m_RDRAM_Interface;
		_RegRDRAM        = _Reg->m_RDRAM_Registers;
		_RegMI           = _Reg->m_Mips_Interface;
		_RegVI           = _Reg->m_Video_Interface;
		_RegDPC          = _Reg->m_Display_ControlReg;
		_RegAI           = _Reg->m_Audio_Interface;
		_RegSP           = _Reg->m_SigProcessor_Interface;
		_RegPI           = _Reg->m_Peripheral_Interface;
		_RegSI           = _Reg->m_SerialInterface;
		_AudioIntrReg    = &_Reg->m_AudioIntrReg;
		_PROGRAM_COUNTER = &_Reg->m_PROGRAM_COUNTER;
		_NextTimer       = &_N64System->m_NextTimer;
	}

	CaptureScreen       = _Plugins->Gfx()->CaptureScreen;
	ChangeWindow        = _Plugins->Gfx()->ChangeWindow;
	DrawScreen          = _Plugins->Gfx()->DrawScreen;
	MoveScreen          = _Plugins->Gfx()->MoveScreen;
	ProcessDList        = _Plugins->Gfx()->ProcessDList;
	ProcessRDPList      = _Plugins->Gfx()->ProcessRDPList;
	ShowCFB             = _Plugins->Gfx()->ShowCFB;
	UpdateScreen        = _Plugins->Gfx()->UpdateScreen;
	ViStatusChanged     = _Plugins->Gfx()->ViStatusChanged;
	ViWidthChanged      = _Plugins->Gfx()->ViWidthChanged;
#ifdef tofix
//	GetGfxDebugInfo     = _Plugins->Gfx()->GetGfxDebugInfo;
//	GFXCloseDLL         = _Plugins->Gfx()->GFXCloseDLL;
//	GFXDllAbout         = _Plugins->Gfx()->GFXDllAbout;
//	GFXDllConfig        = _Plugins->Gfx()->GFXDllConfig;
//	GfxRomClosed        = _Plugins->Gfx()->GfxRomClosed;
//	GfxRomOpen          = _Plugins->Gfx()->GfxRomOpen;
//	FrameBufferRead     = _Plugins->Gfx()->FrameBufferRead;
//	FrameBufferWrite    = _Plugins->Gfx()->FrameBufferWrite;
//	InitiateGFX         = _Plugins->Gfx()->InitiateGFX;
//	InitiateGFXDebugger = _Plugins->Gfx()->InitiateGFXDebugger;
#endif

	ControllerCommand   = _Plugins->Control()->ControllerCommand;
	GetKeys             = _Plugins->Control()->GetKeys;
	ReadController      = _Plugins->Control()->ReadController;
	RumbleCommand       = _Plugins->Control()->RumbleCommand;
	g_Controllers       = _Plugins->Control()->m_PluginControllers;
#ifdef tofix
//	ContCloseDLL        = _Plugins->Control()->ContCloseDLL;
//	ContDllAbout        = _Plugins->Control()->ContDllAbout;
//	ContConfig          = _Plugins->Control()->ContConfig;
//	InitiateControllers_1_0= _Plugins->Control()->InitiateControllers_1_0;
//	InitiateControllers_1_1= _Plugins->Control()->InitiateControllers_1_1;
//	ContRomOpen         = _Plugins->Control()->ContRomOpen;
//	ContRomClosed       = _Plugins->Control()->ContRomClosed;
//	WM_KeyDown          = _Plugins->Control()->WM_KeyDown;
//	WM_KeyUp            = _Plugins->Control()->WM_KeyUp;
#endif

	DoRspCycles         = _Plugins->RSP()->DoRspCycles;
#ifdef tofix
//	GetRspDebugInfo     = _Plugins->RSP()->GetRspDebugInfo;
//	RSPCloseDLL         = _Plugins->RSP()->RSPCloseDLL;
//	RSPDllAbout         = _Plugins->RSP()->RSPDllAbout;
//	RSPDllConfig        = _Plugins->RSP()->RSPDllConfig;
//	RSPRomClosed        = _Plugins->RSP()->RSPRomClosed;
//	InitiateRSP_1_0     = _Plugins->RSP()->InitiateRSP_1_0;
//	InitiateRSP_1_1     = _Plugins->RSP()->InitiateRSP_1_1;
//	InitiateRSPDebugger = _Plugins->RSP()->InitiateRSPDebugger;
#endif
	
	AiLenChanged        = _Plugins->Audio()->LenChanged;
	AiReadLength        = _Plugins->Audio()->ReadLength;
	ProcessAList        = _Plugins->Audio()->ProcessAList;
#ifdef tofix
//	AiCloseDLL          = _Plugins->Audio()->AiCloseDLL;
//	AiDacrateChanged    = _Plugins->Audio()->AiDacrateChanged;
//	AiDllAbout          = _Plugins->Audio()->AiDllAbout;
//	AiDllConfig         = _Plugins->Audio()->AiDllConfig;
//	AiDllTest           = _Plugins->Audio()->AiDllTest;
//	AiRomClosed         = _Plugins->Audio()->AiRomClosed;
//	AiUpdate            = _Plugins->Audio()->Update;
//	InitiateAudio       = _Plugins->Audio()->InitiateAudio;
#endif
	g_TLB_ReadMap       = NULL; //System->m_TLB.TLB_ReadMap;
	g_TLB_WriteMap      = NULL; //System->m_TLB.TLB_WriteMap;
#ifdef tofix
	g_MemorStack       = &_MMU->m_MemoryStack;
#endif
}

void CC_Core::PauseExecution ( void )
{
	_N64System->Pause();
}

void CC_Core::RunRsp ( void )
{
	try
	{
		_N64System->RunRSP();
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
		_N64System->RefreshScreen();
	} 
	catch (...)
	{
		WriteTraceF(TraceError,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
	}
}

void CC_Core::GenerateProfileLog ( void )
{
	_N64System->m_Profile.GenerateLog();
}

void CC_Core::ResetTimer ( void )
{
	_N64System->m_Profile.ResetCounters();
}

DWORD CC_Core::StartTimer ( DWORD Address )
{
	return _N64System->m_Profile.StartTimer(Address);
}

DWORD CC_Core::StopTimer ( void )
{
	return _N64System->m_Profile.StopTimer();
}

void PauseExecution ( void )
{
	CC_Core::PauseExecution();
}

void DisplayError ( const char * Message, ... )
{
	if (_Notify == NULL) { return; }

	va_list ap;
	va_start( ap, Message );
	_Notify->DisplayError(Message,ap);
}

void DisplayMessage  ( int DisplayTime, const char * Message, ... )
{
	if (_Notify == NULL) { return; }
	
	va_list ap;
	va_start( ap, Message );
	_Notify->DisplayMessage(DisplayTime, Message,ap);
}

void DisplayMessage2 ( const char * Message, ... )
{
	if (_Notify == NULL) { return; }

	va_list ap;
	va_start( ap, Message );
	_Notify->DisplayMessage2(Message,ap);
}

const char * GetAppName ( void )
{
	static stdstr szAppName = _Settings->LoadString(Setting_ApplicationName);
	return szAppName.c_str();
}

void GetAutoSaveDir( char * Directory ) 
{
	strcpy(Directory,_Settings->LoadString(Directory_NativeSave).c_str());
}

void GetInstantSaveDir( char * Directory ) 
{
	strcpy(Directory,_Settings->LoadString(Directory_InstantSave).c_str());
}

void SetFpuLocations( void ) 
{
	_Reg->FixFpuLocations();
}

BOOL Limit_FPS ( void )
{
	return _Settings->LoadDword(GameRunning_LimitFPS);
}

void DacrateChanged ( enum SystemType Type )
{
	_Plugins->Audio()->DacrateChanged(Type);
}

BOOL Close_C_CPU ( void )
{
	if (_Settings == NULL || !_Settings->LoadBool(GameRunning_CPU_Running))
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
	_N64System->CloseCpu();
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
	_N64System->UpdateSyncCPU(_SyncSystem,Cycles);
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
	_Notify->BreakPoint(__FILE__,__LINE__);
	//ExecuteInterpreterOps(Cycles);
}

void SyncSystem (void)
{
	_N64System->SyncCPU(_SyncSystem);
}

void ApplyGSButtonCheats ( void )
{
	CC_Core::ApplyGSButtonCheats(_N64System);
}

void ChangePluginFunc ( void )
{
	_Notify->DisplayMessage(0,MSG_PLUGIN_INIT);
	if (_Settings->LoadBool(Plugin_GFX_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_GFX);
	}
	if (_Settings->LoadBool(Plugin_AUDIO_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_AUDIO);
	}	
	if (_Settings->LoadBool(Plugin_CONT_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_CONTROLLER);
	}	
	if (_Settings->LoadBool(Plugin_RSP_Changed) || 
		_Settings->LoadBool(Plugin_AUDIO_Changed) || 
		_Settings->LoadBool(Plugin_GFX_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_RSP);
	}
	_Settings->SaveBool(Plugin_RSP_Changed,  false);
	_Settings->SaveBool(Plugin_AUDIO_Changed,false);
	_Settings->SaveBool(Plugin_GFX_Changed,  false);
	_Settings->SaveBool(Plugin_CONT_Changed, false);
	_Notify->RefreshMenu();
	if (!_Plugins->Initiate()) 
	{
		_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
		SetEndEmulation(true);
	} else {
		CC_Core::SetCurrentSystem(_N64System);
	}
	_Recompiler->ResetRecompCode();
}

void ChangeFullScreenFunc ( void )
{
	_Notify->ChangeFullScreen();
}

BOOL Machine_LoadState ( void )
{
	bool Result = CC_Core::LoadState(_N64System);
	CC_Core::SetCurrentSystem(_N64System);
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
	return CC_Core::SaveState(_N64System);
}

void BreakPoint(LPCSTR FileName, int LineNumber )
{
	if (_Notify)
	{
		_Notify->BreakPoint(FileName,LineNumber);
	}
}


void CC_Core::ApplyGSButtonCheats (CN64System * System)
{
	if (System == NULL)
	{
		return;
	}
	if (System->m_Cheats.CheatsSlectionChanged())
	{
		System->m_Cheats.LoadCheats(false);
	}
	System->m_Cheats.ApplyGSButton(_MMU);
}

void CC_Core::ApplyCheats (CN64System * System)
{
	if (System == NULL)
	{
		return;
	}
	if (System->m_Cheats.CheatsSlectionChanged())
	{
		System->m_Cheats.LoadCheats(false);
	}
	System->m_Cheats.ApplyCheats(_MMU);
}

void ApplyCheats (void)
{
	CC_Core::ApplyCheats(_N64System);
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
	return _N64System->m_EndEmulation;
}

void SetEndEmulation    ( BOOL End )
{
	_N64System->m_EndEmulation = End != 0;
}

void CloseSaveChips ( void )
{
	CloseEeprom();
	CloseMempak();
	CloseSram();
	CloseFlashRam();
}

void TLB_ReadEntry      ( void )
{
	_TLB->ReadEntry();
}

void TLB_WriteEntry( int index, BOOL Random )
{
	if (index > 31)
	{
		BreakPoint(__FILE__,__LINE__);
	}
	_TLB->WriteEntry(index,Random != 0);
}

void TLB_Probe()
{
	_TLB->Probe();
}

void SyncToPC (void) {
	FixRandomReg();
	SyncSystem ();
}

BOOL ClearRecompCodeProtectMem ( DWORD Address, int length )
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (_Recompiler)
	{
		return _Recompiler->ClearRecompCode_Phys(Address,length,CRecompiler::Remove_ProtectedMem);
	}
#endif
	return false;
}

BOOL ClearRecompCodeInitialCode ( void )
{
	if (_Recompiler)
	{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		return _Recompiler->ClearRecompCode_Virt(0x80000000,0x200,CRecompiler::Remove_InitialCode);
#endif
	}
	return false;
}