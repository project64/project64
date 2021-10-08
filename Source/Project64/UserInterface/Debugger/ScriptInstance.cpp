#include <stdafx.h>
#include "ScriptTypes.h"
#include "ScriptInstance.h"
#include "ScriptAPI/ScriptAPI.h"

#include <sys/stat.h>

extern "C" {
    int DukTimeoutCheck(void* udata)
    {
        CScriptInstance* inst = (CScriptInstance*)udata;
        return (int)inst->IsTimedOut();
    }
}

CScriptInstance::CScriptInstance(CScriptSystem* sys, const char* name) :
    m_System(sys),
    m_InstanceName(name),
    m_Ctx(nullptr),
    m_RefCount(0),
    m_ExecTimeout(JS_EXEC_TIMEOUT),
    m_ExecStartTime(0),
    m_SourceCode(nullptr),
    m_CurExecCallbackId(JS_INVALID_CALLBACK),
    m_bStopping(false)
{
}

CScriptInstance::~CScriptInstance()
{
    Cleanup();
}

std::string& CScriptInstance::Name()
{
    return m_InstanceName;
}

CScriptSystem* CScriptInstance::System()
{
    return m_System;
}

CDebuggerUI* CScriptInstance::Debugger()
{
    return m_System->Debugger();
}

JSAppCallbackID CScriptInstance::CallbackId()
{
    return m_CurExecCallbackId;
}

bool CScriptInstance::Run(const char* path)
{
    if(m_Ctx != nullptr)
    {
        return false;
    }

    m_Ctx = duk_create_heap(nullptr, nullptr, nullptr, this, nullptr);

    if(m_Ctx == nullptr)
    {
        goto error_cleanup;
    }

    struct stat statBuf;
    if(stat(path, &statBuf) != 0)
    {
        m_System->ConsoleLog("[SCRIPTSYS]: error: could not stat '%s'", path);
        goto error_cleanup;
    }

    m_SourceCode = new char[statBuf.st_size + 1];
    m_SourceCode[statBuf.st_size] = '\0';

    m_SourceFile.open(path, std::ios::in | std::ios::binary);
    if(!m_SourceFile.is_open())
    {
        m_System->ConsoleLog("[SCRIPTSYS]: error: could not open '%s'", path);
        goto error_cleanup;
    }

    m_SourceFile.read(m_SourceCode, statBuf.st_size);

    if((size_t)m_SourceFile.tellg() != (size_t)statBuf.st_size)
    {
        m_System->ConsoleLog("[SCRIPTSYS]: error: could not read '%s'", path);
        goto error_cleanup;
    }

    m_ExecStartTime = Timestamp();

    ScriptAPI::InitEnvironment(m_Ctx, this);

    duk_push_string(m_Ctx, m_InstanceName.c_str());
    if(duk_pcompile_string_filename(m_Ctx, DUK_COMPILE_STRICT, m_SourceCode) != 0 ||
       duk_pcall(m_Ctx, 0) == DUK_EXEC_ERROR)
    {
        duk_get_prop_string(m_Ctx, -1, "stack");
        m_System->ConsoleLog("%s", duk_safe_to_string(m_Ctx, -1));
        duk_pop_n(m_Ctx, 2);
        goto error_cleanup;
    }

    duk_pop(m_Ctx);
    return true;

error_cleanup:
    Cleanup();
    return false;
}

void CScriptInstance::IncRefCount()
{
    m_RefCount++;
}

void CScriptInstance::DecRefCount()
{
    if(m_RefCount > 0)
    {
        m_RefCount--;
    }
}

void CScriptInstance::SetStopping(bool bStopping)
{
    m_bStopping = bStopping;
}

void CScriptInstance::RawCMethodCall(void* dukThisHeapPtr, duk_c_function func, JSDukArgSetupFunc argSetupFunc, void *argSetupParam)
{
    m_ExecStartTime = Timestamp();
    duk_push_c_function(m_Ctx, func, DUK_VARARGS);
    duk_push_heapptr(m_Ctx, dukThisHeapPtr);
    duk_idx_t nargs = argSetupFunc ? argSetupFunc(m_Ctx, argSetupParam) : 0;

    if (duk_pcall_method(m_Ctx, nargs) == DUK_EXEC_ERROR)
    {
        duk_get_prop_string(m_Ctx, -1, "stack");
        m_System->ConsoleLog("%s", duk_safe_to_string(m_Ctx, -1));
        duk_pop(m_Ctx);
    }

    duk_pop(m_Ctx);
}

void CScriptInstance::PostCMethodCall(void* dukThisHeapPtr, duk_c_function func, JSDukArgSetupFunc argSetupFunc,
    void* argSetupParam, size_t argSetupParamSize)
{
    m_System->PostCMethodCall(m_InstanceName.c_str(), dukThisHeapPtr, func, argSetupFunc, argSetupParam, argSetupParamSize);
}

void CScriptInstance::RawConsoleInput(const char* code)
{
    m_System->ConsoleLog("> %s", code);

    duk_get_global_string(m_Ctx, HS_gInputListener);

    if (duk_is_function(m_Ctx, -1))
    {
        m_ExecStartTime = Timestamp();
        duk_push_string(m_Ctx, code);
        if (duk_pcall(m_Ctx, 1) != DUK_EXEC_SUCCESS)
        {
            duk_get_prop_string(m_Ctx, -1, "stack");
            m_System->ConsoleLog("%s", duk_safe_to_string(m_Ctx, -1));
            duk_pop_n(m_Ctx, 2);
            return;
        }
        else
        {
            duk_pop(m_Ctx);
            return;
        }
    }
    duk_pop(m_Ctx);

    m_ExecStartTime = Timestamp();
    
    duk_push_string(m_Ctx, stdstr_f("<input:%s>", m_InstanceName.c_str()).c_str());
    if (duk_pcompile_string_filename(m_Ctx, DUK_COMPILE_STRICT, code) != 0 ||
        duk_pcall(m_Ctx, 0) == DUK_EXEC_ERROR)
    {
        duk_get_prop_string(m_Ctx, -1, "stack");
        m_System->ConsoleLog("%s", duk_safe_to_string(m_Ctx, -1));
        duk_pop(m_Ctx);
    }
    else
    {
        if (duk_is_string(m_Ctx, -1))
        {
            m_System->ConsoleLog("\"%s\"", duk_get_string(m_Ctx, -1));
        }
        else if(duk_is_object(m_Ctx, -1))
        {
            duk_dup(m_Ctx, -1);
            duk_get_global_string(m_Ctx, "JSON");
            duk_get_prop_string(m_Ctx, -1, "stringify");
            duk_remove(m_Ctx, -2);
            duk_pull(m_Ctx, -3);
            duk_push_null(m_Ctx);
            duk_push_int(m_Ctx, 2);
            duk_pcall(m_Ctx, 3);

            const char* str = duk_safe_to_string(m_Ctx, -2);
            const char* res = duk_get_string(m_Ctx, -1);

            m_System->ConsoleLog("%s %s", str, res);
            duk_pop(m_Ctx);
        }
        else
        {
            m_System->ConsoleLog("%s", duk_safe_to_string(m_Ctx, -1));
        }
    }
    duk_pop(m_Ctx);
}

void CScriptInstance::SetExecTimeout(uint64_t timeout)
{
    m_ExecTimeout = timeout;
}

bool CScriptInstance::IsTimedOut()
{
    if(m_ExecStartTime == 0 || m_ExecTimeout == 0)
    {
        return false;
    }

    uint64_t timeElapsed = Timestamp() - m_ExecStartTime;
    return (timeElapsed >= m_ExecTimeout);
}

uint64_t CScriptInstance::Timestamp()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    return li.QuadPart / 10000;
}

void CScriptInstance::Cleanup()
{
    if(m_Ctx != nullptr)
    {
        duk_destroy_heap(m_Ctx);
        m_Ctx = nullptr;
    }
    if(m_SourceCode != nullptr)
    {
        delete[] m_SourceCode;
        m_SourceCode = nullptr;
    }
    if(m_SourceFile.is_open())
    {
        m_SourceFile.close();
    }
}

bool CScriptInstance::RegisterWorker(CScriptWorker* worker)
{
    if (IsStopping())
    {
        return false;
    }

    m_Workers.push_back(worker);
    return true;
}

void CScriptInstance::UnregisterWorker(CScriptWorker* worker)
{
    std::vector<CScriptWorker*>::iterator it;
    for (it = m_Workers.begin(); it != m_Workers.end(); it++)
    {
        if (*it == worker)
        {
            m_Workers.erase(it);
            return;
        }
    }
}

void CScriptInstance::StopRegisteredWorkers()
{
    std::vector<CScriptWorker*>::iterator it;
    for (it = m_Workers.begin(); it != m_Workers.end(); it++)
    {
        CScriptWorker* worker = *it;
        worker->StopWorkerProc();
    }
}
