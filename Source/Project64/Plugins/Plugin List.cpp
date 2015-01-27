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
		do
		{
			WriteTraceF(TraceDebug, __FUNCTION__ ": loading %s", (LPCSTR)Dir);

			CPlugin * plugin = CPlugin::InitPlugin((LPCTSTR)Dir);
			if (plugin == NULL)
				continue;

			PLUGIN Plugin = { 0 };
			memcpy(&Plugin.Info, plugin->GetInfo(), sizeof(PLUGIN_INFO));
			Plugin.FullPath = Dir;
			Plugin.FileName = ((stdstr &)Dir).substr(((stdstr &)m_PluginDir).length());

			if (plugin->DllAbout != NULL)
				Plugin.AboutFunction = true;

			m_PluginList.push_back(Plugin);
			delete plugin;
		} while (Dir.FindNext());
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