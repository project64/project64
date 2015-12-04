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

    void(__cdecl *DllAbout)  (void * hWnd);
    void(__cdecl *DllConfig) (void * hParent);

    static bool ValidPluginVersion ( PLUGIN_INFO & PluginInfo );

protected:
    void UnloadPlugin();
    const char * PluginType() const;
    TraceType PluginTraceType() const;
    virtual void UnloadPluginDetails() = 0;
    virtual PLUGIN_TYPE type() = 0;
    virtual bool LoadFunctions(void) = 0;

    void(__cdecl *CloseDLL)		(void);
    void(__cdecl *RomOpen)			(void);
    void(__cdecl *RomClosed)	    (void);
    void(__cdecl *PluginOpened)(void);
    void(__cdecl *SetSettingInfo)	(PLUGIN_SETTINGS  *);
    void(__cdecl *SetSettingInfo2)	(PLUGIN_SETTINGS2 *);
    void(__cdecl *SetSettingInfo3)	(PLUGIN_SETTINGS3 *);

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
