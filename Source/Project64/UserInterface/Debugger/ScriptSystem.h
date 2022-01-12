#include <windows.h>
#include <fstream>
#include <map>

#include "ScriptTypes.h"
#include "debugger.h"

#pragma once

#define SCRIPTSYS_SCRIPTS_DIR "Scripts\\"
#define SCRIPTSYS_MODULES_DIR "Scripts\\modules\\"

class CScriptSystem
{
    typedef std::map<JSInstanceName, CScriptInstance*> JSInstanceMap;
    typedef std::vector<JSAppCallback> JSAppCallbackList;
    typedef std::map<JSInstanceName, JSInstanceStatus> JSInstanceStatusMap;
    typedef std::vector<JSSysCommand> JSSysCommandQueue;

    enum { JS_CPU_CB_RANGE_CACHE_SIZE = 256 };

    struct JSCpuCbListInfo
    {
        size_t   numCallbacks;
        uint32_t minAddrStart;
        uint32_t maxAddrEnd;
        size_t   numRangeCacheEntries;
        bool     bRangeCacheExceeded;
        struct {
            uint32_t addrStart;
            uint32_t addrEnd;
        } rangeCache[JS_CPU_CB_RANGE_CACHE_SIZE];
    };

    HANDLE              m_hThread;

    CriticalSection          m_CmdQueueCS;
    JSSysCommandQueue        m_CmdQueue;
    HANDLE                   m_hCmdEvent;
    std::set<JSInstanceName> m_StopsIssued;

    CriticalSection     m_InstancesCS;
    JSInstanceMap       m_Instances;
    JSAppCallbackList   m_AppCallbackHooks[JS_NUM_APP_HOOKS];
    JSAppCallbackID     m_NextAppCallbackId;
    
    volatile size_t   m_AppCallbackCount;

    volatile JSCpuCbListInfo m_CpuExecCbInfo;
    volatile JSCpuCbListInfo m_CpuReadCbInfo;
    volatile JSCpuCbListInfo m_CpuWriteCbInfo;
    
    CriticalSection     m_UIStateCS;
    JSInstanceStatusMap m_UIInstanceStatus;
    stdstr              m_UILog;

    CDebuggerUI*        m_Debugger;

    std::set<std::string> m_AutorunList;

    stdstr m_InstallDirFullPath;
    stdstr m_ScriptsDirFullPath;
    stdstr m_ModulesDirFullPath;

public:
    CScriptSystem(CDebuggerUI* debugger);
    ~CScriptSystem();

    CDebuggerUI* Debugger();

    void StartScript(const char* instanceName, const char* path);
    void StopScript(const char* instanceName);
    void Input(const char* instanceName, const char* code);

    stdstr InstallDirPath();
    stdstr ScriptsDirPath();
    stdstr ModulesDirPath();
    
    JSInstanceStatus GetStatus(const char* instanceName);
    void NotifyStatus(const char* instanceName, JSInstanceStatus status);
    void ConsoleLog(const char* format, ...);
    void ConsolePrint(const char* format, ...);
    void ConsoleClear();
    stdstr GetConsoleBuffer();

    void PostCMethodCall(const char* instanceName, void* dukThisHeapPtr, duk_c_function func,
        JSDukArgSetupFunc argSetupFunc = nullptr,
        void* argSetupParam = nullptr,
        size_t argSetupParamSize = 0);

    bool HaveAppCallbacks(JSAppHookID hookId);

    // Note: Unguarded for speed, shouldn't matter
    inline bool HaveAppCallbacks() { return m_AppCallbackCount != 0; }

    inline bool HaveCpuExecCallbacks(uint32_t address)
    {
        return HaveCpuCallbacks(m_CpuExecCbInfo, m_AppCallbackHooks[JS_HOOK_CPU_EXEC], address);
    }

    inline bool HaveCpuReadCallbacks(uint32_t address)
    {
        return HaveCpuCallbacks(m_CpuReadCbInfo, m_AppCallbackHooks[JS_HOOK_CPU_READ], address);
    }

    inline bool HaveCpuWriteCallbacks(uint32_t address)
    {
        return HaveCpuCallbacks(m_CpuWriteCbInfo, m_AppCallbackHooks[JS_HOOK_CPU_WRITE], address);
    }
    
    static void UpdateCpuCbListInfo(volatile JSCpuCbListInfo& info, JSAppCallbackList& callbacks);

    void DoMouseEvent(JSAppHookID hookId, int x, int y, DWORD uMsg = (DWORD)-1);
    void InvokeAppCallbacks(JSAppHookID hookId, void* env = nullptr);

    void ExecAutorunList();
    std::set<std::string>& AutorunList();
    void LoadAutorunList();
    void SaveAutorunList();

    JSAppCallbackID RawAddAppCallback(JSAppHookID hookId, JSAppCallback& callback);
    void RawRemoveAppCallback(JSAppHookID hookId, JSAppCallbackID callbackId);

private:
    inline bool HaveCpuCallbacks(volatile JSCpuCbListInfo& info, JSAppCallbackList& callbacks, uint32_t address)
    {
        if (info.numCallbacks == 0 || address < info.minAddrStart || address > info.maxAddrEnd)
        {
            return false;
        }

        if (!info.bRangeCacheExceeded)
        {
            for (size_t i = 0; i < info.numRangeCacheEntries; i++)
            {
                if (address >= info.rangeCache[i].addrStart &&
                    address <= info.rangeCache[i].addrEnd)
                {
                    return true;
                }
            }

            return false;
        }

        CGuard guard(m_InstancesCS);

        for (JSAppCallback& callback : callbacks)
        {
            if (address >= callback.m_Params.addrStart &&
                address <= callback.m_Params.addrEnd)
            {
                return true;
            }
        }

        return false;
    }

    void InitDirectories();

    void PostCommand(JSSysCommandID id, stdstr paramA = "", stdstr paramB = "", void* paramC = nullptr);

    static DWORD WINAPI ThreadProc(void* _this);
    void ThreadProc();

    bool ProcessCommand(JSSysCommand& cmd);
    bool ProcessCommandQueue(std::vector<JSSysCommand>& queue);
    void PullCommands(JSSysCommandID id, std::vector<JSSysCommand>& out);

    void OnStartScript(const char* name, const char* path);
    void OnStopScript(const char* name);
    void OnInput(const char* name, const char* code);
    void OnCMethodCall(const char* name, JSSysCMethodCall* methodCall);
    void OnSweep(bool bIfDone);

    bool RawRemoveInstance(const char* key);
    void RefreshCallbackMaps();

    static stdstr FixStringReturns(const char* str);
};
