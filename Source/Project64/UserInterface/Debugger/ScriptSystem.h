#pragma once

#include <stdafx.h>
#include <3rdParty/duktape/duktape.h>

#include "ScriptInstance.h"

class CScriptHook;

class CScriptSystem
{
public:
    CScriptSystem(CDebuggerUI* debugger);
    ~CScriptSystem();
    // Run a script in its own context/thread
    void RunScript(const char * path);

    // Kill a script context/thread by its path
    void StopScript(const char * path);

    const char* APIScript();

private:
    typedef struct {
        const char* hookId;
        CScriptHook* cbList;
    } HOOKENTRY;

    typedef struct {
        char* path;
        CScriptInstance* scriptInstance;
    } INSTANCE_ENTRY;

    CDebuggerUI* m_Debugger;
    int m_NumCallbacks;
    char* m_APIScript;

    vector<HOOKENTRY> m_Hooks;
    vector<INSTANCE_ENTRY> m_RunningInstances;

    vector<std::string> m_LogData;

    CScriptHook* m_HookCPUExec;
    CScriptHook* m_HookCPURead;
    CScriptHook* m_HookCPUWrite;
    CScriptHook* m_HookCPUExecOpcode;
    CScriptHook* m_HookCPUGPRValue;
    CScriptHook* m_HookFrameDrawn;

    CriticalSection m_CS;

    void RegisterHook(const char* hookId, CScriptHook* cbList); // Associate string ID with callback list
    void UnregisterHooks();

    HDC m_ScreenDC;

    int m_NextCallbackId;

public:
    // Returns true if any of the script hooks have callbacks for scriptInstance

    void SetScreenDC(HDC hdc)
    {
        m_ScreenDC = hdc;
    }

    HDC GetScreenDC()
    {
        return m_ScreenDC;
    }

    inline void LogText(const char* text)
    {
        m_LogData.push_back(text);
        m_Debugger->Debug_RefreshScriptsWindow();
    }

    bool HasCallbacksForInstance(CScriptInstance* scriptInstance);

    // Remove all hooked callbacks for an instance
    void ClearCallbacksForInstance(CScriptInstance* scriptInstance);

    void RemoveCallbackById(int callbackId);

    CScriptHook* GetHook(const char* hookId);

    int GetNextCallbackId();
    void CallbackAdded();
    void CallbackRemoved();

    inline int HaveCallbacks()
    {
        return m_NumCallbacks != 0;
    }

    void DeleteStoppedInstances();
    INSTANCE_STATE GetInstanceState(const char* scriptName);
    CScriptInstance* GetInstance(const char* scriptName);

    CScriptHook* HookCPUExec()
    {
        return m_HookCPUExec;
    }

    CScriptHook* HookCPURead()
    {
        return m_HookCPURead;
    }

    CScriptHook* HookCPUWrite()
    {
        return m_HookCPUWrite;
    }

    CScriptHook* HookCPUExecOpcode()
    {
        return m_HookCPUExecOpcode;
    }

    CScriptHook* HookCPUGPRValue()
    {
        return m_HookCPUGPRValue;
    }

    CScriptHook* HookFrameDrawn()
    {
        return m_HookFrameDrawn;
    }
};
