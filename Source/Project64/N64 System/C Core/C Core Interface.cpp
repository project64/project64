#include "stdafx.h"
#include "mempak.h"
#include "Plugin.h"
#include "Logging.h"

//settings
BOOL g_HaveDebugger = false, g_AudioSignal = false;
DWORD g_RomFileSize = 0, g_CountPerOp = 2, g_ViRefreshRate = 1500;
enum CPU_TYPE g_CPU_Type;
enum SAVE_CHIP_TYPE g_SaveUsing;

//Plugins
DWORD * _AudioIntrReg = NULL;
enum SystemType g_SystemType;

BOOL          g_IndvidualBlock, g_Profiling;

void CC_Core::SetSettings  ( )
{
	if (_Settings)
	{
		g_HaveDebugger        = _Settings->LoadBool(Debugger_Enabled);
		g_CPU_Type            = (CPU_TYPE)_Settings->LoadDword(Game_CpuType);
		g_SaveUsing           = (SAVE_CHIP_TYPE)_Settings->LoadDword(Game_SaveChip);
		g_AudioSignal         = _Settings->LoadBool(Game_RspAudioSignal);
		g_CountPerOp          = _Settings->LoadDword(Game_CounterFactor);
		g_ViRefreshRate       = _Settings->LoadDword(Game_ViRefreshRate);
	}
}

void CC_Core::PauseExecution ( void )
{
	_BaseSystem->Pause();
}

void CC_Core::RunRsp ( void )
{
	try
	{
		_System->RunRSP();
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
	_BaseSystem->m_Profile.GenerateLog();
}

void CC_Core::ResetTimer ( void )
{
	_System->m_Profile.ResetCounters();
}

DWORD CC_Core::StartTimer ( DWORD Address )
{
	return _System->m_Profile.StartTimer(Address);
}

DWORD CC_Core::StopTimer ( void )
{
	return _System->m_Profile.StopTimer();
}

void PauseExecution ( void )
{
	CC_Core::PauseExecution();
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

void RunRsp( void ) 
{
	CC_Core::RunRsp();
}

void SyncSystem (void)
{
	_BaseSystem->SyncCPU(_SyncSystem);
}

void ApplyGSButtonCheats ( void )
{
	CC_Core::ApplyGSButtonCheats(_BaseSystem);
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
		_BaseSystem->m_EndEmulation = true;
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
	bool Result = CC_Core::LoadState(_System);
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
	return CC_Core::SaveState(_BaseSystem);
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
	CC_Core::ApplyCheats(_BaseSystem);
}

void CloseSaveChips ( void )
{
	CloseMempak();
}
