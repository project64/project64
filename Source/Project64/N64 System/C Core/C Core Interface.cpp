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

const char * GetAppName ( void )
{
	static stdstr szAppName = _Settings->LoadString(Setting_ApplicationName);
	return szAppName.c_str();
}

void CloseSaveChips ( void )
{
	CloseMempak();
}
