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

    inline size_t GetRefCount() { return m_RefCount; }
    void   IncRefCount();
    void   DecRefCount();
    void   SetStopping(bool bStopping);
    inline bool IsStopping() { return m_bStopping; }

    bool RegisterWorker(CScriptWorker* worker);
    void UnregisterWorker(CScriptWorker* worker);
    void StopRegisteredWorkers();

    inline void RawInvokeAppCallback(JSAppCallback& cb, void* _hookEnv)
    {
        m_CurExecCallbackId = cb.m_CallbackId;

        RawCall(cb.m_DukFuncHeapPtr, cb.m_DukArgSetupFunc, _hookEnv);

        if (cb.m_CleanupFunc != nullptr)
        {
            cb.m_CleanupFunc(m_Ctx, _hookEnv);
        }

        m_CurExecCallbackId = JS_INVALID_CALLBACK;
    }

    inline void RawCall(void *dukFuncHeapPtr, JSDukArgSetupFunc argSetupFunc, void *param = nullptr)
    {
        m_ExecStartTime = Timestamp();
        duk_push_heapptr(m_Ctx, dukFuncHeapPtr);
        duk_idx_t nargs = argSetupFunc ? argSetupFunc(m_Ctx, param) : 0;

        if (duk_pcall(m_Ctx, nargs) == DUK_EXEC_ERROR)
        {
            duk_get_prop_string(m_Ctx, -1, "stack");
            m_System->ConsoleLog("%s", duk_safe_to_string(m_Ctx, -1));
            duk_pop(m_Ctx);
        }

        duk_pop(m_Ctx);
    }

    void   RawCMethodCall(void* dukThisHeapPtr, duk_c_function func,
                          JSDukArgSetupFunc argSetupFunc = nullptr,
                          void* argSetupParam = nullptr);

    void   PostCMethodCall(void* dukThisHeapPtr, duk_c_function func,
                           JSDukArgSetupFunc argSetupFunc = nullptr,
                           void* argSetupParam = nullptr, size_t argSetupParamSize = 0);

    void   RawConsoleInput(const char* code);

private:
    static uint64_t Timestamp();
    void Cleanup();
};
