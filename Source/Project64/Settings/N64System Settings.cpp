#include "..\support.h"
#include "..\Settings.h"

bool CN64SystemSettings::bShowCPUPer;  //= _Settings->LoadDword(ShowCPUPer) != 0;
bool CN64SystemSettings::bProfiling;   //= _Settings->LoadDword(ProfileCode) != 0;
bool CN64SystemSettings::bBasicMode;   //= _Settings->LoadDword(BasicMode) != 0;
bool CN64SystemSettings::bLimitFPS;    //= _Settings->LoadDword(LimitFPS) != 0;
bool CN64SystemSettings::bShowDListAListCount;    //= _Settings->LoadDword(ShowDListAListCount) != 0;
bool CN64SystemSettings::bFixedAudio;  //= _Settings->LoadDword(ROM_FixedAudio) != 0;
bool CN64SystemSettings::bSyncToAudio; //7= _Settings->LoadDword(SyncViaAudio) != 0;
bool CN64SystemSettings::bDisplayFrameRate; //7= _Settings->LoadDword(SyncViaAudio) != 0;
bool CN64SystemSettings::bCleanFrameBox = false; //7= _Settings->LoadDword(SyncViaAudio) != 0;

CN64SystemSettings::CN64SystemSettings()
{
	_Settings->RegisterChangeCB(ShowCPUPer,this,(CSettings::SettingChangedFunc)ShowCPUPerChanged);
	_Settings->RegisterChangeCB(ProfileCode,this,(CSettings::SettingChangedFunc)ProfilingChanged);
	_Settings->RegisterChangeCB(BasicMode,this,(CSettings::SettingChangedFunc)BasicModeChanged);
	_Settings->RegisterChangeCB(LimitFPS,this,(CSettings::SettingChangedFunc)LimitFPSChanged);
	_Settings->RegisterChangeCB(ShowDListAListCount,this,(CSettings::SettingChangedFunc)ShowDListAListCountChanged);
	_Settings->RegisterChangeCB(DisplayFrameRate,this,(CSettings::SettingChangedFunc)DisplayFrameRateChanged);
	RefreshSettings();
}

CN64SystemSettings::~CN64SystemSettings()
{
	_Settings->UnregisterChangeCB(ShowCPUPer,this,(CSettings::SettingChangedFunc)ShowCPUPerChanged);
	_Settings->UnregisterChangeCB(ProfileCode,this,(CSettings::SettingChangedFunc)ProfilingChanged);
	_Settings->UnregisterChangeCB(BasicMode,this,(CSettings::SettingChangedFunc)BasicModeChanged);
	_Settings->UnregisterChangeCB(LimitFPS,this,(CSettings::SettingChangedFunc)LimitFPSChanged);
	_Settings->UnregisterChangeCB(ShowDListAListCount,this,(CSettings::SettingChangedFunc)ShowDListAListCountChanged);
	_Settings->UnregisterChangeCB(DisplayFrameRate,this,(CSettings::SettingChangedFunc)DisplayFrameRateChanged);
}

void CN64SystemSettings::RefreshSettings()
{
	bShowCPUPer  = _Settings->LoadDword(ShowCPUPer) != 0;
	bProfiling   = _Settings->LoadDword(ProfileCode) != 0;
	bBasicMode   = _Settings->LoadDword(BasicMode) != 0;
	bLimitFPS    = _Settings->LoadDword(LimitFPS) != 0;
	bShowDListAListCount = _Settings->LoadDword(ShowDListAListCount) != 0;
	bFixedAudio  = _Settings->LoadDword(ROM_FixedAudio) != 0;
	bSyncToAudio = bFixedAudio ? _Settings->LoadDword(SyncViaAudio) != 0 : false;
	bDisplayFrameRate = _Settings->LoadDword(DisplayFrameRate) != 0;
}

void CN64SystemSettings::ShowCPUPerChanged (CN64SystemSettings * _this)
{
	_this->bShowCPUPer  = _Settings->LoadDword(ShowCPUPer) != 0;
}

void CN64SystemSettings::ProfilingChanged (CN64SystemSettings * _this)
{
	_this->bProfiling  = _Settings->LoadDword(ProfileCode) != 0;
}

void CN64SystemSettings::BasicModeChanged (CN64SystemSettings * _this)
{
	_this->bBasicMode   = _Settings->LoadDword(BasicMode) != 0;
}

void CN64SystemSettings::LimitFPSChanged (CN64SystemSettings * _this)
{
	_this->bLimitFPS    = _Settings->LoadDword(LimitFPS) != 0;
}

void CN64SystemSettings::ShowDListAListCountChanged (CN64SystemSettings * _this)
{
	_this->bShowDListAListCount    = _Settings->LoadDword(ShowDListAListCount) != 0;
}

void CN64SystemSettings::DisplayFrameRateChanged (CN64SystemSettings * _this)
{
	_this->bDisplayFrameRate    = _Settings->LoadDword(DisplayFrameRate) != 0;
}
