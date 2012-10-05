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
		_Settings->RegisterChangeCB(Game_SMM_StoreInstruc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_SMM_Protect,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_SMM_ValidFunc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_SMM_PIDMA,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_SMM_TLB,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_RegCache,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_BlockLinking,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_FuncLookupMode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Debugger_ShowRecompMemSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_LoadRomToMemory,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_32Bit,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->RegisterChangeCB(Game_FastSP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CRecompilerSettings::~CRecompilerSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		_Settings->UnregisterChangeCB(Game_SMM_StoreInstruc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_SMM_Protect,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_SMM_ValidFunc,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_SMM_PIDMA,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_SMM_TLB,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_RegCache,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_BlockLinking,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_FuncLookupMode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Debugger_ShowRecompMemSize,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Debugger_ProfileCode,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_LoadRomToMemory,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_32Bit,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		_Settings->UnregisterChangeCB(Game_FastSP,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CRecompilerSettings::RefreshSettings()
{
	m_bSMM_StoreInstruc  = false /*_Settings->LoadBool(Game_SMM_StoreInstruc)*/;
	m_bSMM_Protect       = _Settings->LoadBool(Game_SMM_Protect);
	m_bSMM_ValidFunc     = _Settings->LoadBool(Game_SMM_ValidFunc);
	m_bSMM_PIDMA         = _Settings->LoadBool(Game_SMM_PIDMA);
	m_bSMM_TLB           = _Settings->LoadBool(Game_SMM_TLB);
	m_bShowRecompMemSize = _Settings->LoadBool(Debugger_ShowRecompMemSize);
	m_bProfiling         = _Settings->LoadBool(Debugger_ProfileCode);
	m_bRomInMemory       = _Settings->LoadBool(Game_LoadRomToMemory);
	m_bFastSP            = _Settings->LoadBool(Game_FastSP);
	m_b32Bit             = _Settings->LoadBool(Game_32Bit);

	m_RegCaching         = _Settings->LoadBool(Game_RegCache);
	m_bLinkBlocks        = _Settings->LoadBool(Game_BlockLinking);
	m_LookUpMode         = _Settings->LoadDword(Game_FuncLookupMode);
}
