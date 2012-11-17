#include "stdafx.h"

int   CRecompilerSettings::m_RefCount = 0; 

bool  CRecompilerSettings::m_bShowRecompMemSize;
bool  CRecompilerSettings::m_bSMM_StoreInstruc;  
bool  CRecompilerSettings::m_bSMM_Protect;  
bool  CRecompilerSettings::m_bSMM_ValidFunc;
bool  CRecompilerSettings::m_bSMM_PIDMA;  
bool  CRecompilerSettings::m_bSMM_TLB;    
bool  CRecompilerSettings::m_bProfiling;  
bool  CRecompilerSettings::m_bRomInMemory;
bool  CRecompilerSettings::m_bFastSP;
bool  CRecompilerSettings::m_b32Bit;
bool  CRecompilerSettings::m_RegCaching;
bool  CRecompilerSettings::m_bLinkBlocks;
DWORD CRecompilerSettings::m_LookUpMode; //FUNC_LOOKUP_METHOD

CRecompilerSettings::CRecompilerSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		g_Settings->RegisterChangeCB(Game_SMM_StoreInstruc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_SMM_Protect,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_SMM_ValidFunc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_SMM_PIDMA,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_SMM_TLB,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_RegCache,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_BlockLinking,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_FuncLookupMode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Debugger_ShowRecompMemSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_LoadRomToMemory,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_32Bit,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Game_FastSP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CRecompilerSettings::~CRecompilerSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		g_Settings->UnregisterChangeCB(Game_SMM_StoreInstruc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_SMM_Protect,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_SMM_ValidFunc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_SMM_PIDMA,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_SMM_TLB,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_RegCache,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_BlockLinking,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_FuncLookupMode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Debugger_ShowRecompMemSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_LoadRomToMemory,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_32Bit,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Game_FastSP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CRecompilerSettings::RefreshSettings()
{
	m_bSMM_StoreInstruc  = false /*g_Settings->LoadBool(Game_SMM_StoreInstruc)*/;
	m_bSMM_Protect       = g_Settings->LoadBool(Game_SMM_Protect);
	m_bSMM_ValidFunc     = g_Settings->LoadBool(Game_SMM_ValidFunc);
	m_bSMM_PIDMA         = g_Settings->LoadBool(Game_SMM_PIDMA);
	m_bSMM_TLB           = g_Settings->LoadBool(Game_SMM_TLB);
	m_bShowRecompMemSize = g_Settings->LoadBool(Debugger_ShowRecompMemSize);
	m_bProfiling         = g_Settings->LoadBool(Debugger_ProfileCode);
	m_bRomInMemory       = g_Settings->LoadBool(Game_LoadRomToMemory);
	m_bFastSP            = g_Settings->LoadBool(Game_FastSP);
	m_b32Bit             = g_Settings->LoadBool(Game_32Bit);

	m_RegCaching         = g_Settings->LoadBool(Game_RegCache);
	m_bLinkBlocks        = g_Settings->LoadBool(Game_BlockLinking);
	m_LookUpMode         = g_Settings->LoadDword(Game_FuncLookupMode);
}
