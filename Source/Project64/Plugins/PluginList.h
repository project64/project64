/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <Project64-core/Settings/Settings.h>

class CPluginList
{
public:
    typedef struct
    {
        PLUGIN_INFO Info;
        bool        AboutFunction;
        CPath       FullPath;
        stdstr      FileName;
    } PLUGIN;

public:
    CPluginList(bool bAutoFill = true);
    ~CPluginList();

    bool     LoadList(void);
    int      GetPluginCount(void) const;
    const PLUGIN * GetPluginInfo(int indx) const;

private:
    typedef std::vector<PLUGIN>   PluginList;

    PluginList m_PluginList;
    CPath      m_PluginDir;

    void AddPluginFromDir(CPath Dir);
};
