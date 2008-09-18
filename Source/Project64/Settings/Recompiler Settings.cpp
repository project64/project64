#pragma once
#include "..\Settings.h"

bool CRecompilerSettings::bShowRecompMemSize; //= _Settings->LoadDword(ShowRecompMemSize) != 0;
bool CRecompilerSettings::bSMM_Protect;       //= _Settings->LoadDword(SMM_Protect) != 0;
bool CRecompilerSettings::bSMM_ValidFunc;     //= _Settings->LoadDword(SMM_ValidFunc) != 0;
bool CRecompilerSettings::bSMM_PIDMA;         //= _Settings->LoadDword(SMM_PIDMA) != 0;
bool CRecompilerSettings::bSMM_TLB;           //= _Settings->LoadDword(SMM_TLB) != 0;
bool CRecompilerSettings::bProfiling;         //= _Settings->LoadDword(ProfileCode) != 0;
bool CRecompilerSettings::bRomInMemory;        //= _Settings->LoadDword(ProfileCode) != 0;

CRecompilerSettings::CRecompilerSettings()
{
	bShowRecompMemSize = _Settings->LoadDword(ShowRecompMemSize) != 0;
	bSMM_Protect       = _Settings->LoadDword(SMM_Protect) != 0;
	bSMM_ValidFunc     = _Settings->LoadDword(SMM_ValidFunc) != 0;
	bSMM_PIDMA         = _Settings->LoadDword(SMM_PIDMA) != 0;
	bSMM_TLB           = _Settings->LoadDword(SMM_TLB) != 0;
	bProfiling         = _Settings->LoadDword(ProfileCode) != 0;
	bRomInMemory       = _Settings->LoadDword(RomInMemory) != 0;
	_Settings->RegisterChangeCB(ShowRecompMemSize,this,(CSettings::SettingChangedFunc)ShowRecompMemSizeChanged);
	_Settings->RegisterChangeCB(ProfileCode,this,(CSettings::SettingChangedFunc)ProfilingChanged);
	_Settings->RegisterChangeCB(RomInMemory,this,(CSettings::SettingChangedFunc)RomInMemoryChanged);
}

CRecompilerSettings::~CRecompilerSettings()
{
	_Settings->UnregisterChangeCB(ShowRecompMemSize,this,(CSettings::SettingChangedFunc)ShowRecompMemSizeChanged);
	_Settings->UnregisterChangeCB(ProfileCode,this,(CSettings::SettingChangedFunc)ProfilingChanged);
	_Settings->UnregisterChangeCB(RomInMemory,this,(CSettings::SettingChangedFunc)RomInMemoryChanged);
}

void CRecompilerSettings::ShowRecompMemSizeChanged (CRecompilerSettings * _this)
{
	_this->bShowRecompMemSize = _Settings->LoadDword(ShowRecompMemSize) != 0;
}


void CRecompilerSettings::ProfilingChanged (CRecompilerSettings * _this)
{
	_this->bProfiling = _Settings->LoadDword(ProfileCode) != 0;
}

void CRecompilerSettings::RomInMemoryChanged (CRecompilerSettings * _this)
{
	_this->bRomInMemory = _Settings->LoadDword(RomInMemory) != 0;
}
