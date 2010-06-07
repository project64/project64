#include "stdafx.h"

CPlugins::CPlugins (const stdstr & PluginDir):
	m_PluginDir(PluginDir),
	m_Gfx(NULL), m_Audio(NULL), m_RSP(NULL), m_Control(NULL)
{
	CreatePlugins();
	_Settings->RegisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_Contr,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);

}

CPlugins::~CPlugins (void) {
	_Settings->UnregisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_Contr,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);
	ShutDownPlugins();
}

void CPlugins::PluginChanged ( CPlugins * _this )
{
	bool bGfxChange = stricmp(_this->m_GfxFile.c_str(),_Settings->LoadString(Game_Plugin_Gfx).c_str()) != 0;
	bool bAudioChange = stricmp(_this->m_AudioFile.c_str(),_Settings->LoadString(Game_Plugin_Audio).c_str()) != 0;
	bool bRspChange = stricmp(_this->m_RSPFile.c_str(),_Settings->LoadString(Game_Plugin_RSP).c_str()) != 0;
	bool bContChange = stricmp(_this->m_ControlFile.c_str(),_Settings->LoadString(Game_Plugin_Controller).c_str()) != 0;
	
	if ( bGfxChange || bAudioChange || bRspChange || bContChange )
	{
		if (_Settings->LoadBool(GameRunning_CPU_Running) != 0)  
		{
			if (_N64System) { _N64System->ExternalEvent(SysEvent_ChangePlugins); }
		} else {
			_this->Reset();
			_Notify->RefreshMenu();
		}
	}
}

void CPlugins::CreatePlugins( void ) {
	Reset(PLUGIN_TYPE_GFX);
	Reset(PLUGIN_TYPE_AUDIO);
	Reset(PLUGIN_TYPE_RSP);
	Reset(PLUGIN_TYPE_CONTROLLER);	

	if (_Settings->LoadBool(Debugger_Enabled))
	{
		Notify().RefreshMenu();
	}
}

void CPlugins::GameReset  ( void )
{
	if (m_Gfx)   {
		m_Gfx->GameReset();
	}
	if (m_Audio)   {
		m_Audio->GameReset();
	}
	if (m_RSP)   {
		m_RSP->GameReset();
	}
	if (m_Control) {
		m_Control->GameReset();
	}

}
void CPlugins::ShutDownPlugins( void ) {
	if (m_Gfx)   {
		WriteTrace(TraceGfxPlugin,"Close: Starting");
		m_Gfx->Close();
		WriteTrace(TraceGfxPlugin,"Close: Done");
		WriteTrace(TraceGfxPlugin,"deconstructor: Starting");
		delete m_Gfx;   
		WriteTrace(TraceGfxPlugin,"deconstructor: Done");
		m_Gfx = NULL;
//		_Settings->UnknownSetting_GFX = NULL;
	}
	if (m_Audio)   {
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 5");
		m_Audio->Close();
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 6");
		delete m_Audio;   
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 7");
		m_Audio = NULL;
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 8");
//		_Settings->UnknownSetting_AUDIO = NULL;
	}
	if (m_RSP)   {
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 9");
		m_RSP->Close();
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 10");
		delete m_RSP;   
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 11");
		m_RSP = NULL;
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 12");
//		_Settings->UnknownSetting_RSP = NULL;
	}
	if (m_Control) {
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 12");
		m_Control->Close();
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 13");
		delete m_Control;
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 14");
		m_Control = NULL;
		WriteTrace(TraceDebug,"CPlugins::ShutDownPlugins 15");
//		_Settings->UnknownSetting_CTRL = NULL;
	}
}

void CPlugins::SetRenderWindows( CMainGui * RenderWindow, CMainGui * DummyWindow ) {
	_RenderWindow = RenderWindow;
	_DummyWindow  = DummyWindow;
}

bool CPlugins::Initiate ( void ) 
{
	bool bResult = _RenderWindow->InitiatePlugins();
	if (bResult)
	{
		m_Gfx->RomOpened();
		m_Audio->RomOpened();
		m_Control->RomOpened();
	}
	return bResult;
}

bool CPlugins::InitiateMainThread( void )
{
	WriteTrace(TraceDebug,"CPlugins::Initiate 1");
	//Check to make sure we have the plugin avaliable to be used
	if (m_Gfx   == NULL) { return false; }
	if (m_Audio == NULL) { return false; }
	if (m_RSP   == NULL) { return false; }
	if (m_Control == NULL) { return false; }

	WriteTrace(TraceGfxPlugin,"Close: Starting");
	m_Gfx->Close();
	WriteTrace(TraceGfxPlugin,"Close: Done");
	WriteTrace(TraceDebug,"CPlugins::Initiate 3");
	m_Audio->Close();
	WriteTrace(TraceRSP,"Close: Starting");
	m_RSP->Close();
	WriteTrace(TraceRSP,"Close: Done");
	m_Control->Close();
	WriteTrace(TraceDebug,"CPlugins::Initiate 6");

	WriteTrace(TraceGfxPlugin,"Initiate: Starting");
	if (!m_Gfx->Initiate(_N64System,_RenderWindow))   { return false; }
	WriteTrace(TraceGfxPlugin,"Initiate: Done");
	WriteTrace(TraceDebug,"CPlugins::Initiate 7");
	if (!m_Audio->Initiate(_N64System,_RenderWindow)) { return false; }
	WriteTrace(TraceDebug,"CPlugins::Initiate 8");
	if (!m_Control->Initiate(_N64System,_RenderWindow)) { return false; }
	WriteTrace(TraceRSP	,"Initiate: Starting");
	if (!m_RSP->Initiate(this,_N64System))   { return false; }
	WriteTrace(TraceRSP,"Initiate: Done");
	WriteTrace(TraceDebug,"CPlugins::Initiate 10");
	
	return true;
}

void CPlugins::Reset ( void ) {
	ShutDownPlugins();
	CreatePlugins();
}

void CPlugins::Reset ( PLUGIN_TYPE Type ) 
{
	switch (Type)
	{
	case PLUGIN_TYPE_RSP:
		if (m_RSP)   
		{
			WriteTrace(TraceRSP,"Close: Starting");
			m_RSP->Close();
			WriteTrace(TraceRSP,"Close: Done");
			WriteTrace(TraceRSP,"deconstructor: Starting");
			delete m_RSP;   
			WriteTrace(TraceRSP,"deconstructor: Done");
			m_RSP = NULL;
		}
		{
			m_RSPFile = _Settings->LoadString(Plugin_RSP_Current);
			stdstr_f RspPluginFile("%s%s",m_PluginDir.c_str(),m_RSPFile.c_str());
			WriteTraceF(TraceRSP,"Loading (%s): Starting",RspPluginFile.c_str());
			m_RSP   = new CRSP_Plugin(RspPluginFile.c_str());
			WriteTrace(TraceRSP,"Loading Done");

		}
		WriteTraceF(TraceRSP,"Current Ver: %s",m_RSP->PluginName().c_str());
		_Settings->SaveString(Plugin_RSP_CurVer,m_RSP->PluginName().c_str());

		//Enable debugger
		if (m_RSP->EnableDebugging)
		{
			WriteTrace(TraceRSP,"EnableDebugging: starting");
			m_RSP->EnableDebugging(_Settings->LoadDword(Debugger_Enabled));
			WriteTrace(TraceRSP,"EnableDebugging: done");
		}
		break;
	case PLUGIN_TYPE_GFX:
		if (m_Gfx) 
		{
			WriteTrace(TraceGfxPlugin,"Close: Starting");
			m_Gfx->Close();
			WriteTrace(TraceGfxPlugin,"Close: Done");
			WriteTrace(TraceGfxPlugin,"deconstructor: Starting");
			delete m_Gfx;   
			WriteTrace(TraceGfxPlugin,"deconstructor: Done");
			m_Gfx = NULL;
		}
		{
			m_GfxFile = _Settings->LoadString(Game_Plugin_Gfx);
			stdstr_f GfxPluginFile("%s%s",m_PluginDir.c_str(),m_GfxFile.c_str());
			WriteTraceF(TraceGfxPlugin,"Loading (%s): Starting",GfxPluginFile.c_str());
			m_Gfx   = new CGfxPlugin(GfxPluginFile.c_str());
			WriteTrace(TraceGfxPlugin,"Loading Done");
		}
		WriteTraceF(TraceGfxPlugin,"Current Ver: %s",m_Gfx->PluginName().c_str());
		_Settings->SaveString(Plugin_GFX_CurVer,m_Gfx->PluginName().c_str());
		break;
	case PLUGIN_TYPE_AUDIO:
		if (m_Audio)   {
			WriteTrace(TraceDebug,"Audio Plugin: Close Starting");
			m_Audio->Close();
			WriteTrace(TraceDebug,"Audio Plugin: Close done");
			WriteTrace(TraceDebug,"Audio Plugin: Deconstructor Starting");
			delete m_Audio;   
			WriteTrace(TraceDebug,"Audio Plugin: Deconstructor Done");
			m_Audio = NULL;
		}
		{
			m_AudioFile = _Settings->LoadString(Game_Plugin_Audio);
			stdstr_f PluginFile("%s%s",m_PluginDir.c_str(),m_AudioFile.c_str());
			WriteTraceF(TraceDebug,"Loading (%s): Starting",PluginFile.c_str());
			m_Audio = new CAudioPlugin(PluginFile.c_str());
			WriteTrace(TraceDebug,"Loading Done");
			_Settings->SaveString(Plugin_AUDIO_CurVer,m_Audio->PluginName().c_str());
		}
		break;
	case PLUGIN_TYPE_CONTROLLER:
		if (m_Control)   {
			WriteTrace(TraceDebug,"Controller Plugin: Close Starting");
			m_Control->Close();
			WriteTrace(TraceDebug,"Controller Plugin: Close done");
			WriteTrace(TraceDebug,"Controller Plugin: Deconstructor Starting");
			delete m_Control;   
			WriteTrace(TraceDebug,"Controller Plugin: Deconstructor Done");
			m_Control = NULL;
		}
		{
			m_ControlFile = _Settings->LoadString(Game_Plugin_Controller);
			stdstr_f PluginFile("%s%s",m_PluginDir.c_str(),m_ControlFile.c_str());
			WriteTraceF(TraceDebug,"Loading (%s): Starting",PluginFile.c_str());
			m_Control = new CControl_Plugin(PluginFile.c_str());
			WriteTrace(TraceDebug,"Loading Done");
			_Settings->SaveString(Plugin_CONT_CurVer,m_Control->PluginName().c_str());
		}
		break;
	}
}

void CPlugins::ConfigPlugin ( DWORD hParent, PLUGIN_TYPE Type ) {
	switch (Type) {
	case PLUGIN_TYPE_RSP:
		if (m_RSP == NULL || m_RSP->Config == NULL) { break; }
		if (!m_RSP->Initilized()) {
			if (!m_RSP->Initiate(NULL,NULL)) {				
				break;
			}
		}
		m_RSP->Config(hParent);
		break;
	case PLUGIN_TYPE_GFX:
		if (m_Gfx == NULL || m_Gfx->Config == NULL) { break; }
		if (!m_Gfx->Initilized()) {
			if (!m_Gfx->Initiate(NULL,_DummyWindow)) {
				break;
			}
		}
		m_Gfx->Config(hParent);
		break;
	case PLUGIN_TYPE_AUDIO:
		if (m_Audio == NULL || m_Audio->Config == NULL) { break; }
		if (!m_Audio->Initilized()) {
			if (!m_Audio->Initiate(NULL,_DummyWindow)) {
				break;
			}
		}
		m_Audio->Config(hParent);
		break;
	case PLUGIN_TYPE_CONTROLLER:
		if (m_Control == NULL || m_Control->Config == NULL) { break; }
		if (!m_Control->Initilized()) {
			if (!m_Control->Initiate(NULL,_DummyWindow)) {
				break;
			}
		}
		m_Control->Config(hParent);
		break;

	}
}

void DummyCheckInterrupts ( void ) {
}

void DummyFunction (void) {
}

#include <windows.h>

void CPlugins::CreatePluginDir ( const stdstr & DstDir ) const {
   char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], 
	   fname[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath(DstDir.c_str(), drive, dir, fname, ext );			
	_makepath(path_buffer, drive, dir, "", "" );
	if (CreateDirectory(path_buffer,NULL) == 0 && GetLastError() == ERROR_PATH_NOT_FOUND) 
	{
		path_buffer[strlen(path_buffer) - 1] = 0;
		CreatePluginDir(stdstr(path_buffer));
		CreateDirectory(path_buffer,NULL);
	}
}

void CPlugins::CopyPlugins (  const stdstr & DstDir ) const {	
	//Copy GFX Plugin
	stdstr_f srcGfxPlugin("%s%s",m_PluginDir.c_str(),_Settings->LoadString(Plugin_GFX_Current).c_str());
	stdstr_f dstGfxPlugin("%s%s",DstDir.c_str(),_Settings->LoadString(Plugin_GFX_Current).c_str());
	
	if (CopyFile(srcGfxPlugin.c_str(),dstGfxPlugin.c_str(),false) == 0) 
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { CreatePluginDir(dstGfxPlugin); }
		CopyFile(srcGfxPlugin.c_str(),dstGfxPlugin.c_str(),false);
	}

	//Copy m_Audio Plugin
	stdstr_f srcAudioPlugin("%s%s",m_PluginDir.c_str(),_Settings->LoadString(Plugin_AUDIO_Current).c_str());
	stdstr_f dstAudioPlugin("%s%s",DstDir.c_str(), _Settings->LoadString(Plugin_AUDIO_Current).c_str());
	if (CopyFile(srcAudioPlugin.c_str(),dstAudioPlugin.c_str(),false) == 0) {
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { CreatePluginDir(dstAudioPlugin); }
		CopyFile(srcAudioPlugin.c_str(),dstAudioPlugin.c_str(),false);
	}

	//Copy m_RSP Plugin
	stdstr_f srcRSPPlugin("%s%s",m_PluginDir.c_str(), _Settings->LoadString(Plugin_RSP_Current).c_str());
	stdstr_f dstRSPPlugin("%s%s",DstDir.c_str(),_Settings->LoadString(Plugin_RSP_Current).c_str());
	if (CopyFile(srcRSPPlugin.c_str(),dstRSPPlugin.c_str(),false) == 0) {
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { CreatePluginDir(dstRSPPlugin); }
		CopyFile(srcRSPPlugin.c_str(),dstRSPPlugin.c_str(),false);
	}

	//Copy Controler Plugin
	stdstr_f srcContPlugin("%s%s",m_PluginDir.c_str(), _Settings->LoadString(Plugin_CONT_Current).c_str());
	stdstr_f dstContPlugin("%s%s",DstDir.c_str(),_Settings->LoadString(Plugin_CONT_Current).c_str());
	if (CopyFile(srcContPlugin.c_str(),dstContPlugin.c_str(),false) == 0) {
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { CreatePluginDir(dstContPlugin); }
		CopyFile(srcContPlugin.c_str(),dstContPlugin.c_str(),false);
	}
}
