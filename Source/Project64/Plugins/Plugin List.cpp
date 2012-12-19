/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

CPluginList::CPluginList(bool bAutoFill /* = true */) :
	m_PluginDir(g_Settings->LoadString(Directory_Plugin),"")
{
	if (bAutoFill)
	{
		LoadList();
	}
}

CPluginList::~CPluginList()
{
}

int CPluginList::GetPluginCount() const
{
	return m_PluginList.size();
}

const CPluginList::PLUGIN * CPluginList::GetPluginInfo  ( int indx ) const
{
	if (indx < 0 || indx >= (int)m_PluginList.size())
	{
		return NULL;
	}
	return &m_PluginList[indx];

}

bool CPluginList::LoadList()
{
	WriteTrace(TraceDebug,__FUNCTION__ ": Start");
	m_PluginList.clear();
	AddPluginFromDir(m_PluginDir);
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");
	return true;
}

void CPluginList::AddPluginFromDir ( CPath Dir)
{
	Dir.SetNameExtension("*.*");
	if (Dir.FindFirst(_A_SUBDIR))
	{
		do {
			AddPluginFromDir(Dir);
		} while (Dir.FindNext());
		Dir.UpDirectory();
	}

	Dir.SetNameExtension("*.dll");
	if (Dir.FindFirst())
	{
		HMODULE hLib = NULL;
		do {
			if (hLib)
			{
				FreeLibrary(hLib);
				hLib = NULL;
			}

			UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
			WriteTraceF(TraceDebug,__FUNCTION__ ": loading %s",(LPCSTR)Dir);
			hLib = LoadLibrary(Dir);		
			SetErrorMode(LastErrorMode);

			if (hLib == NULL) 
			{ 
				continue;
			}

			void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );
			GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( hLib, "GetDllInfo" );
			if (GetDllInfo == NULL) 
			{
				continue;
			}
			
			PLUGIN Plugin;
			memset(&Plugin.Info,0,sizeof(Plugin.Info));
			Plugin.Info.MemoryBswaped = true;
			GetDllInfo(&Plugin.Info);
			if (!ValidPluginVersion(Plugin.Info))
			{
				continue;
			}

			Plugin.FullPath = Dir;
			Plugin.FileName = ((stdstr &)Dir).substr(((stdstr &)m_PluginDir).length());

			if (GetProcAddress(hLib,"DllAbout") != NULL) 
			{
				Plugin.AboutFunction = true;
			}
			m_PluginList.push_back(Plugin);
		} while (Dir.FindNext());

		if (hLib)
		{
			FreeLibrary(hLib);
			hLib = NULL;
		}
	}
}

bool CPluginList::ValidPluginVersion ( PLUGIN_INFO & PluginInfo ) {
	switch (PluginInfo.Type) 
	{
	case PLUGIN_TYPE_RSP: 
		if (!PluginInfo.MemoryBswaped)	  { return false; }
		if (PluginInfo.Version == 0x0001) { return true;  }
		if (PluginInfo.Version == 0x0100) { return true;  }
		if (PluginInfo.Version == 0x0101) { return true;  }
		if (PluginInfo.Version == 0x0102) { return true;  }
		break;
	case PLUGIN_TYPE_GFX:
		if (!PluginInfo.MemoryBswaped)	  { return false; }
		if (PluginInfo.Version == 0x0102) { return true;  }
		if (PluginInfo.Version == 0x0103) { return true;  }
		if (PluginInfo.Version == 0x0104) { return true;  }
		break;
	case PLUGIN_TYPE_AUDIO:
		if (!PluginInfo.MemoryBswaped)	  { return false; }
		if (PluginInfo.Version == 0x0101) { return true;  }
		if (PluginInfo.Version == 0x0102) { return true;  }
		break;
	case PLUGIN_TYPE_CONTROLLER:
		if (PluginInfo.Version == 0x0100) { return true; }
		if (PluginInfo.Version == 0x0101) { return true; }
		if (PluginInfo.Version == 0x0102) { return true; }
		break;
	}
	return FALSE;
}


#ifdef toremove
CPluginList::CPluginList (CSettings * Settings) {
	g_Settings = Settings;
}

#include <windows.h>

void CPluginList::AddPluginFromDir (const char * PluginDir, const char * Dir, PluginList * Plugins) {
	WIN32_FIND_DATA FindData;
	char SearchDir[300];

	//Go through all directories
	strncpy(SearchDir,Dir,sizeof(SearchDir));
	strncat(SearchDir,"*.*",sizeof(SearchDir));
	
	HANDLE hFind = FindFirstFile(SearchDir, &FindData);
	while (hFind != INVALID_HANDLE_VALUE) {
		if (strcmp(FindData.cFileName,".") == 0 || strcmp(FindData.cFileName,"..") == 0){
			if (FindNextFile(hFind,&FindData) == 0) { 
				hFind = INVALID_HANDLE_VALUE; 
			}
			continue;
		}
		if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			char NewDir[300];
			
			strncpy(NewDir,Dir,sizeof(NewDir));
			strncat(NewDir,FindData.cFileName,sizeof(NewDir));
			strncat(NewDir,"\\",sizeof(NewDir));
			AddPluginFromDir(PluginDir,NewDir,Plugins);
		}
		if (FindNextFile(hFind,&FindData) == 0) { 
			hFind = INVALID_HANDLE_VALUE; 
		}
	}
	

	//Create the search path for all dll in directory
	strncpy(SearchDir,Dir,sizeof(SearchDir));
	strncat(SearchDir,"*.dll",sizeof(SearchDir));

	//Add all DLL's to the list of plugins
	hFind = FindFirstFile(SearchDir, &FindData);
	while (hFind != INVALID_HANDLE_VALUE) {
		PLUGIN Plugin;
		Plugin.FullPath = Dir;
		Plugin.FullPath += FindData.cFileName;
		Plugin.FileName = Plugin.FullPath.substr(strlen(PluginDir));
		Plugin.InfoFunction = false;
		
		//Load the plugin
		UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
		HMODULE hLib = LoadLibrary(Plugin.FullPath.c_str());		
		SetErrorMode(LastErrorMode);

		if (hLib != NULL) { 
			void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );

			GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( hLib, "GetDllInfo" );
			if (GetDllInfo != NULL) {
				if (GetProcAddress(hLib,"DllAbout") != NULL) {
					Plugin.InfoFunction = true;
				}
				GetDllInfo(&Plugin.info);
				if (ValidPluginVersion(&Plugin.info) &&
					!(Plugin.info.Type != PLUGIN_TYPE_CONTROLLER && Plugin.info.MemoryBswaped == FALSE))
				{
					Plugins->push_back(Plugin);
				}
			}
			FreeLibrary(hLib);
		}

		if (FindNextFile(hFind,&FindData) == 0) { 
			hFind = INVALID_HANDLE_VALUE; 
		}
	}
}

void CPluginList::DllAbout (void * hParent, const char * PluginFile ) {
	//Load the plugin
	UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
	HMODULE hLib = LoadLibrary(PluginFile);		
	SetErrorMode(LastErrorMode);
	
	//Get DLL about
	void (__cdecl *DllAbout) ( HWND hWnd );
	DllAbout = (void (__cdecl *)(HWND))GetProcAddress( hLib, "DllAbout" );
	
	//call the function from the dll
	DllAbout((HWND)hParent);

	FreeLibrary(hLib);
}

PluginList CPluginList::GetPluginList (void) {
	PluginList Plugins;

	//Create search path for plugins
	Notify().BreakPoint(__FILE__,__LINE__);
/*	char SearchDir[300] = "";
	g_Settings->LoadString(PluginDirectory,SearchDir,sizeof(SearchDir));

	//recursively scan search dir, and add files in that dir 
	AddPluginFromDir(SearchDir,SearchDir,&Plugins);
	*/
	return Plugins;
}

#endif