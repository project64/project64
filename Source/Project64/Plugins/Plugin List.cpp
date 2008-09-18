#include "..\Plugin.h"

CPluginList::CPluginList (CSettings * Settings) {
	_Settings = Settings;
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
	char SearchDir[300] = "";
	_Settings->LoadString(PluginDirectory,SearchDir,sizeof(SearchDir));

	//recursively scan search dir, and add files in that dir 
	AddPluginFromDir(SearchDir,SearchDir,&Plugins);
	
	return Plugins;
}

bool CPluginList::ValidPluginVersion ( PLUGIN_INFO * PluginInfo ) {
	switch (PluginInfo->Type) {
	case PLUGIN_TYPE_RSP: 
		if (PluginInfo->Version == 0x0001) { return TRUE; }
		if (PluginInfo->Version == 0x0100) { return TRUE; }
		if (PluginInfo->Version == 0x0101) { return TRUE; }
		if (PluginInfo->Version == 0x0102) { return TRUE; }
		break;
	case PLUGIN_TYPE_GFX:
		if (PluginInfo->Version == 0x0102) { return TRUE; }
		if (PluginInfo->Version == 0x0103) { return TRUE; }
		if (PluginInfo->Version == 0x0104) { return TRUE; }
		break;
	case PLUGIN_TYPE_AUDIO:
		if (PluginInfo->Version == 0x0101) { return TRUE; }
		if (PluginInfo->Version == 0x0102) { return TRUE; }
		break;
	case PLUGIN_TYPE_CONTROLLER:
		if (PluginInfo->Version == 0x0100) { return TRUE; }
		if (PluginInfo->Version == 0x0101) { return TRUE; }
		if (PluginInfo->Version == 0x0102) { return TRUE; }
		break;
	}
	return FALSE;
}
