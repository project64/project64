#include "stdafx.h"
#include "eeprom.h"
#include "mempak.h"
#include "Plugin.h"
#include "Logging.h"
#include "CPU Log.h"
#include "sram.h"
#include "flashram.h"

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
enum SystemType g_SystemType;

//Memory
DWORD g_RdramSize;

BOOL          g_IndvidualBlock, g_Profiling;

void CC_Core::SetSettings  ( )
{
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
	_N64System->m_EndEmulation = true;
	_Notify->BreakPoint(__FILE__,__LINE__);
//	g_Main_CPU_Action->DoSomething = true;
//	g_Main_CPU_Action->CloseCPU = true;
	return false;
}

void RunRsp( void ) 
{
	CC_Core::RunRsp();
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
		_N64System->m_EndEmulation = true;
	} else {
		//CC_Core::SetCurrentSystem(_N64System);
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
	//CC_Core::SetCurrentSystem(_N64System);
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

void CloseSaveChips ( void )
{
	CloseEeprom();
	CloseMempak();
	CloseSram();
	CloseFlashRam();
}

void SyncToPC (void) {
	//FixRandomReg();
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
