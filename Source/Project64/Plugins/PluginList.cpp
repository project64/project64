#include "stdafx.h"

#include "PluginList.h"
#include <Project64-core/Plugins/PluginBase.h>
#include <io.h>

CPluginList::CPluginList(bool bAutoFill /* = true */) :
    m_PluginDir(g_Settings->LoadStringVal(Directory_Plugin), "")
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
    return (int)((INT_PTR)m_PluginList.size());
}

const CPluginList::PLUGIN * CPluginList::GetPluginInfo(int indx) const
{
    if (indx < 0 || indx >= (int)m_PluginList.size())
    {
        return nullptr;
    }
    return &m_PluginList[indx];
}

bool CPluginList::LoadList()
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    m_PluginList.clear();
    AddPluginFromDir(m_PluginDir);
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
    return true;
}

void CPluginList::AddPluginFromDir(CPath Dir)
{
    Dir.SetNameExtension("*");
    if (Dir.FindFirst(CPath::FIND_ATTRIBUTE_SUBDIR))
    {
        do
        {
            AddPluginFromDir(Dir);
        } while (Dir.FindNext());
        Dir.UpDirectory();
    }

    Dir.SetNameExtension("*.dll");
    if (Dir.FindFirst())
    {
        HMODULE hLib = nullptr;
        do
        {
            if (hLib)
            {
                FreeLibrary(hLib);
                hLib = nullptr;
            }

            //UINT LastErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
            WriteTrace(TraceUserInterface, TraceDebug, "Loading %s", (LPCSTR)Dir);
            hLib = LoadLibrary(stdstr((LPCSTR)Dir).ToUTF16().c_str());
            //SetErrorMode(LastErrorMode);

            if (hLib == nullptr)
            {
                DWORD LoadError = GetLastError();
                WriteTrace(TraceUserInterface, TraceDebug, "Failed to load %s (error: %d)", (LPCSTR)Dir, LoadError);
                continue;
            }

            void(CALL * GetDllInfo)(PLUGIN_INFO * PluginInfo);
            GetDllInfo = (void(CALL *)(PLUGIN_INFO *))GetProcAddress(hLib, "GetDllInfo");
            if (GetDllInfo == nullptr)
            {
                continue;
            }

            PLUGIN Plugin = {0};
            Plugin.Info.Reserved2 = true;
            GetDllInfo(&Plugin.Info);
            if (!CPlugin::ValidPluginVersion(Plugin.Info))
            {
                continue;
            }

            Plugin.FullPath = Dir;
            Plugin.FileName = stdstr((const char *)Dir).substr(strlen(m_PluginDir));

            if (GetProcAddress(hLib, "DllAbout") != nullptr)
            {
                Plugin.AboutFunction = true;
            }
            m_PluginList.push_back(Plugin);
        } while (Dir.FindNext());

        if (hLib)
        {
            FreeLibrary(hLib);
            hLib = nullptr;
        }
    }
}
