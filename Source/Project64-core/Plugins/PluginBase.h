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

#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/TraceModulesProject64.h>
#include "PluginClass.h"

#if defined(_WIN32)
#define CALL        __cdecl
#else
#define CALL
#endif

class CPlugin :
    private CDebugSettings
{
public:
    CPlugin();
    virtual ~CPlugin();
    inline const char * PluginName() const { return m_PluginInfo.Name; }
    inline bool Initialized() { return m_Initialized; }

    virtual int32_t GetDefaultSettingStartRange() const = 0;
    virtual int32_t GetSettingStartRange() const = 0;

    bool Load(const char * FileName);

    void RomOpened();
    void RomClose();
    void GameReset();
    void Close();

    void(CALL *DllAbout)  (void * hWnd);
    void(CALL *DllConfig) (void * hParent);

    static bool ValidPluginVersion(PLUGIN_INFO & PluginInfo);

protected:
    void UnloadPlugin();
    const char * PluginType() const;
    TraceModuleProject64 PluginTraceType() const;
    virtual void UnloadPluginDetails() = 0;
    virtual PLUGIN_TYPE type() = 0;
    virtual bool LoadFunctions(void) = 0;

    void(CALL *CloseDLL)            (void);
    void(CALL *RomOpen)             (void);
    void(CALL *RomClosed)           (void);
    void(CALL *PluginOpened)(void);
    void(CALL *SetSettingInfo)      (PLUGIN_SETTINGS  *);
    void(CALL *SetSettingInfo2)     (PLUGIN_SETTINGS2 *);
    void(CALL *SetSettingInfo3)     (PLUGIN_SETTINGS3 *);

    void * m_hDll;
    bool   m_Initialized, m_RomOpen;
    PLUGIN_INFO m_PluginInfo;

    // Loads a function pointer from the currently loaded DLL
    template <typename T>
    void _LoadFunction(const char * szFunctionName, T & functionPointer) {
        functionPointer = (T)GetProcAddress((HMODULE)m_hDll, szFunctionName);
    }

    // Simple wrapper around _LoadFunction() to avoid having to specify the same two arguments
    // i.e. _LoadFunction("CloseDLL", CloseDLL);
#define LoadFunction(functionName) _LoadFunction(#functionName, functionName)
};
