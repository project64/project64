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
    typedef std::map<JSAppCallbackID, JSAppCallback> JSAppCallbackMap;
    typedef std::map<JSAppHookID, JSAppCallbackMap> JSAppHookMap;
    typedef std::map<JSInstanceName, JSInstanceStatus> JSInstanceStatusMap;
    typedef std::vector<JSSysCommand> JSSysCommandQueue;

    HANDLE              m_hThread;

    CriticalSection     m_CmdQueueCS;
    JSSysCommandQueue   m_CmdQueue;
    HANDLE              m_hCmdEvent;

    CriticalSection     m_InstancesCS;
    JSInstanceMap       m_Instances;
    JSAppHookMap        m_AppCallbackHooks;
    JSAppCallbackID     m_NextAppCallbackId;
    size_t              m_AppCallbackCount;

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
    void InvokeAppCallbacks(JSAppHookID hookId, void* env = nullptr);
    void DoMouseEvent(JSAppHookID hookId, int x, int y, DWORD uMsg = (DWORD)-1);
    JSAppCallbackID RawAddAppCallback(JSAppHookID hookId, JSAppCallback& callback);
    bool RawRemoveAppCallback(JSAppHookID hookId, JSAppCallbackID callbackId);

    void ExecAutorunList();
    std::set<std::string>& AutorunList();
    void LoadAutorunList();
    void SaveAutorunList();
    
private:
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

    static stdstr FixStringReturns(const char* str);
};
