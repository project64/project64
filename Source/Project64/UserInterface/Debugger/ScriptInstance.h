#include "ScriptTypes.h"
#include "ScriptSystem.h"
#include "ScriptWorker.h"

#pragma once

class CScriptInstance
{
private:
    JSInstanceName  m_InstanceName;
    CScriptSystem*  m_System;
    duk_context*    m_Ctx;
    size_t          m_RefCount;
    uint64_t        m_ExecTimeout;
    uint64_t        m_ExecStartTime;
    std::ifstream   m_SourceFile;
    char*           m_SourceCode;
    JSAppCallbackID m_CurExecCallbackId;
    std::vector<CScriptWorker*> m_Workers;
    bool            m_bStopping;

public:
    CScriptInstance(CScriptSystem* sys, const char* name);
    ~CScriptInstance();

    JSInstanceName& Name();
    CScriptSystem*  System();
    CDebuggerUI*    Debugger();
    JSAppCallbackID CallbackId();

    bool   Run(const char* path);
    void   SetExecTimeout(uint64_t timeout);
    bool   IsTimedOut();

    size_t GetRefCount();
    void   IncRefCount();
    void   DecRefCount();
    void   SetStopping(bool bStopping);
    bool   IsStopping();

    bool RegisterWorker(CScriptWorker* worker);
    void UnregisterWorker(CScriptWorker* worker);
    void StopRegisteredWorkers();

    void   RawInvokeAppCallback(JSAppCallback& cb, void *_hookEnv);

    void   RawConsoleInput(const char* code);

    void   RawCall(void* dukFuncHeapPtr, JSDukArgSetupFunc argSetupFunc, void* param = nullptr);

    void   RawCMethodCall(void* dukThisHeapPtr, duk_c_function func,
                          JSDukArgSetupFunc argSetupFunc = nullptr,
                          void* argSetupParam = nullptr);

    void   PostCMethodCall(void* dukThisHeapPtr, duk_c_function func,
                           JSDukArgSetupFunc argSetupFunc = nullptr,
                           void* argSetupParam = nullptr, size_t argSetupParamSize = 0);

private:
    static uint64_t Timestamp();
    void Cleanup();
};
