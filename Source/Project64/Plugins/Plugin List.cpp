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

			//UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
			WriteTraceF(TraceDebug,__FUNCTION__ ": loading %s",(LPCSTR)Dir);
			hLib = LoadLibrary(Dir);
			//SetErrorMode(LastErrorMode);

			if (hLib == NULL) 
			{ 
				DWORD LoadError = GetLastError();
				WriteTraceF(TraceDebug, __FUNCTION__ ": failed to loadi %s (error: %d)", (LPCSTR)Dir, LoadError);
				continue;
			}

			void (__cdecl *GetDllInfo) ( PLUGIN_INFO * PluginInfo );
			GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( hLib, "GetDllInfo" );
			if (GetDllInfo == NULL) 
			{
				continue;
			}
			
			PLUGIN Plugin = { 0 };
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
