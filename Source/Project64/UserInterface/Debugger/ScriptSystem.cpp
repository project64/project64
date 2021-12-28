#include <stdafx.h>
#include <sys/stat.h>
#include <sstream>
#include "ScriptTypes.h"
#include "ScriptSystem.h"
#include "ScriptInstance.h"
#include "ScriptAPI/ScriptAPI.h"
#include "Debugger.h"
#include "Project64\UserInterface\DiscordRPC.h"

CScriptSystem::CScriptSystem(CDebuggerUI *debugger) :
    m_Debugger(debugger),
    m_NextAppCallbackId(0),
    m_AppCallbackCount(0),
    m_CpuExecCbInfo({}),
    m_CpuReadCbInfo({}),
    m_CpuWriteCbInfo({})
{
    InitDirectories();

    m_hCmdEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_hThread = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
}

CScriptSystem::~CScriptSystem()
{
    PostCommand(JS_CMD_SHUTDOWN);
    WaitForSingleObject(m_hThread, INFINITE);
    CloseHandle(m_hThread);
    CloseHandle(m_hCmdEvent);
}

JSInstanceStatus CScriptSystem::GetStatus(const char* name)
{
    CGuard guard(m_UIStateCS);
    if (m_UIInstanceStatus.count(name) == 0)
    {
        return JS_STATUS_STOPPED;
    }
    else
    {
        return m_UIInstanceStatus[name];
    }
}

void CScriptSystem::NotifyStatus(const char* name, JSInstanceStatus status)
{
    CGuard guard(m_UIStateCS);
    if (status == JS_STATUS_STOPPED)
    {
        m_UIInstanceStatus.erase(name);
    }
    else
    {
        m_UIInstanceStatus[name] = status;
    }
    m_Debugger->Debug_RefreshScriptsWindow();
}

void CScriptSystem::ConsoleLog(const char* format, ...)
{
    CGuard guard(m_UIStateCS);

    va_list args;
    va_start(args, format);

    int size = vsnprintf(nullptr, 0, format, args) + 1;
    char* str = new char[size];
    vsnprintf(str, size, format, args);

    stdstr formattedMsg = FixStringReturns(str) + "\r\n";
    
    m_Debugger->Debug_LogScriptsWindow(formattedMsg.c_str());
    m_UILog += formattedMsg;

    delete[] str;
    va_end(args);
}

void CScriptSystem::ConsolePrint(const char* format, ...)
{
    CGuard guard(m_UIStateCS);

    va_list args;
    va_start(args, format);

    int size = vsnprintf(nullptr, 0, format, args) + 1;
    char* str = new char[size];
    vsnprintf(str, size, format, args);
    
    stdstr formattedMsg = FixStringReturns(str);
    
    m_Debugger->Debug_LogScriptsWindow(formattedMsg.c_str());
    m_UILog += formattedMsg;

    delete[] str;
    va_end(args);
}

void CScriptSystem::ConsoleClear()
{
    CGuard guard(m_UIStateCS);
    m_UILog.clear();
    m_Debugger->Debug_ClearScriptsWindow();
}

stdstr CScriptSystem::GetConsoleBuffer()
{
    CGuard guard(m_UIStateCS);
    return stdstr(m_UILog);
}

stdstr CScriptSystem::InstallDirPath()
{
    return m_InstallDirFullPath;
}

stdstr CScriptSystem::ScriptsDirPath()
{
    return m_ScriptsDirFullPath;
}

stdstr CScriptSystem::ModulesDirPath()
{
    return m_ModulesDirFullPath;
}

void CScriptSystem::InitDirectories()
{
    m_InstallDirFullPath = (std::string)CPath(CPath::MODULE_DIRECTORY);
    m_ScriptsDirFullPath = m_InstallDirFullPath + SCRIPTSYS_SCRIPTS_DIR;
    m_ModulesDirFullPath = m_InstallDirFullPath + SCRIPTSYS_MODULES_DIR;

    if (!PathFileExistsA(m_ScriptsDirFullPath.c_str()))
    {
        CreateDirectoryA(m_ScriptsDirFullPath.c_str(), nullptr);
    }

    if (!PathFileExistsA(m_ModulesDirFullPath.c_str()))
    {
        CreateDirectoryA(m_ModulesDirFullPath.c_str(), nullptr);
    }
}

void CScriptSystem::StartScript(const char *name, const char *path)
{
    PostCommand(JS_CMD_START_SCRIPT, name, path);
}

void CScriptSystem::StopScript(const char *name)
{
    PostCommand(JS_CMD_STOP_SCRIPT, name);
}

void CScriptSystem::Input(const char *name, const char *code)
{
    PostCommand(JS_CMD_INPUT, name, code);
}

bool CScriptSystem::HaveAppCallbacks(JSAppHookID hookId)
{
    CGuard guard(m_InstancesCS);

    return (hookId < JS_NUM_APP_HOOKS &&
            m_AppCallbackHooks[hookId].size() > 0);
}

void CScriptSystem::InvokeAppCallbacks(JSAppHookID hookId, void* env)
{
    CGuard guard(m_InstancesCS);
    RefreshCallbackMaps();

    JSAppCallbackList& callbacks = m_AppCallbackHooks[hookId];

    bool bNeedSweep = false;

    for (JSAppCallback& callback : callbacks)
    {
        if (callback.m_bDisabled || !callback.m_ConditionFunc(&callback, env))
        {
            continue;
        }

        CScriptInstance* instance = callback.m_Instance;

        if (!instance->IsStopping())
        {
            instance->RawInvokeAppCallback(callback, env);

            if (instance->GetRefCount() == 0)
            {
                bNeedSweep = true;
            }
        }
    }

    if (bNeedSweep)
    {
        PostCommand(JS_CMD_SWEEP);
    }
}

void CScriptSystem::UpdateCpuCbListInfo(volatile JSCpuCbListInfo& info, JSAppCallbackList& callbacks)
{
    uint32_t minAddrStart = 0;
    uint32_t maxAddrEnd = 0;
    int numCacheEntries = 0;
    bool bCacheExceeded = false;

    for (JSAppCallback& callback : callbacks)
    {
        size_t i;
        for (i = 0; i < numCacheEntries; i++)
        {
            // combine adjacent/overlapping ranges
            if ((callback.m_Params.addrStart >= info.rangeCache[i].addrStart &&
                 callback.m_Params.addrStart <= info.rangeCache[i].addrEnd + 1) ||
                (callback.m_Params.addrEnd >= info.rangeCache[i].addrStart - 1 &&
                 callback.m_Params.addrEnd <= info.rangeCache[i].addrEnd))
            {
                info.rangeCache[i].addrStart = min(info.rangeCache[i].addrStart, callback.m_Params.addrStart);
                info.rangeCache[i].addrEnd = max(info.rangeCache[i].addrEnd, callback.m_Params.addrEnd);
                break;
            }
        }

        if (i == numCacheEntries)
        {
            if (i == JS_CPU_CB_RANGE_CACHE_SIZE)
            {
                bCacheExceeded = true;
            }
            else
            {
                info.rangeCache[i].addrStart = callback.m_Params.addrStart;
                info.rangeCache[i].addrEnd = callback.m_Params.addrEnd;
                numCacheEntries++;
            }
        }
        
        if (callback.m_Params.addrStart < minAddrStart)
        {
            minAddrStart = callback.m_Params.addrStart;
        }

        if (callback.m_Params.addrEnd > maxAddrEnd)
        {
            maxAddrEnd = callback.m_Params.addrEnd;
        }
    }

    info.numRangeCacheEntries = numCacheEntries;
    info.bRangeCacheExceeded = bCacheExceeded;
    info.numCallbacks = callbacks.size();
    info.minAddrStart = minAddrStart;
    info.maxAddrEnd = maxAddrEnd;
}

void CScriptSystem::RefreshCallbackMaps()
{
    for (JSAppCallbackList& callbacks : m_AppCallbackHooks)
    {
        JSAppCallbackList::iterator it = callbacks.begin();

        while (it != callbacks.end())
        {
            if (it->m_bDisabled)
            {
                callbacks.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    UpdateCpuCbListInfo(m_CpuExecCbInfo, m_AppCallbackHooks[JS_HOOK_CPU_EXEC]);
    UpdateCpuCbListInfo(m_CpuReadCbInfo, m_AppCallbackHooks[JS_HOOK_CPU_READ]);
    UpdateCpuCbListInfo(m_CpuWriteCbInfo, m_AppCallbackHooks[JS_HOOK_CPU_WRITE]);
}

void CScriptSystem::DoMouseEvent(JSAppHookID hookId, int x, int y, DWORD uMsg)
{
    int button = -1;

    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        button = 0;
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        button = 1;
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        button = 2;
        break;
    }

    JSHookMouseEnv env = { x, y, button };
    InvokeAppCallbacks(hookId, (void*)&env);
}

void CScriptSystem::PostCMethodCall(const char* name, void *dukThisHeapPtr, duk_c_function func,
    JSDukArgSetupFunc argSetupFunc, void *argSetupParam, size_t argSetupParamSize)
{
    CGuard guard(m_CmdQueueCS);
    // Will be deleted by command handler
    JSSysCMethodCall* methodCall = new JSSysCMethodCall(dukThisHeapPtr, func, argSetupFunc, argSetupParam, argSetupParamSize);
    PostCommand(JS_CMD_C_METHOD_CALL, name, "", (void*)methodCall);
}

void CScriptSystem::PostCommand(JSSysCommandID id, stdstr paramA, stdstr paramB, void* paramC)
{
    CGuard guard(m_CmdQueueCS);
    JSSysCommand cmd = { id, paramA, paramB, paramC };
    m_CmdQueue.push_back(cmd);
    SetEvent(m_hCmdEvent);
}

DWORD CScriptSystem::ThreadProc(void *_this)
{
    ((CScriptSystem *)_this)->ThreadProc();
    return 0;
}

void CScriptSystem::ThreadProc()
{
    std::vector<JSSysCommand> queue;

    while(true)
    {
        WaitForSingleObject(m_hCmdEvent, INFINITE);

        {
            CGuard guard(m_CmdQueueCS);
            queue = m_CmdQueue;
            m_CmdQueue.clear();
        }

        if (!ProcessCommandQueue(queue))
        {
            break;
        }
    }
}

void CScriptSystem::OnStartScript(const char *name, const char *path)
{
    if (m_Instances.count(name) != 0)
    {
        ConsoleLog("[SCRIPTSYS]: error: START_SCRIPT aborted; '%s' is already instanced", name);
    }

    CScriptInstance *inst = new CScriptInstance(this, name);
    
    NotifyStatus(name, JS_STATUS_STARTING);

    if(inst->Run(path) && inst->GetRefCount() > 0)
    {
        m_Instances[name] = inst;
        NotifyStatus(name, JS_STATUS_STARTED);
    }
    else
    {
        NotifyStatus(name, JS_STATUS_STOPPED);
        delete inst;
    }
}

void CScriptSystem::OnStopScript(const char *name)
{
    if (m_Instances.count(name) == 0)
    {
        ConsoleLog("[SCRIPTSYS]: error: STOP_SCRIPT aborted; instance '%s' does not exist", name);
        return;
    }

    RawRemoveInstance(name);
    NotifyStatus(name, JS_STATUS_STOPPED);
}

void CScriptSystem::OnInput(const char *name, const char *code)
{
    if(m_Instances.count(name) == 0)
    {
        ConsoleLog("[SCRIPTSYS]: error: INPUT aborted; instance '%s' does not exist", name);
        return;
    }

    CScriptInstance* inst = m_Instances[name];

    inst->RawConsoleInput(code);

    if(!inst->IsStopping() && inst->GetRefCount() == 0)
    {
        NotifyStatus(name, JS_STATUS_STOPPED);
        RawRemoveInstance(name);
    }
}

void CScriptSystem::OnCMethodCall(const char *name, JSSysCMethodCall* methodCall)
{
    if (m_Instances.count(name) == 0)
    {
        ConsoleLog("[SCRIPTSYS]: error: method call aborted; instance '%s' doesn't exist", name);
    }

    if (m_Instances.count(name) == 0)
    {
        return;
    }

    CScriptInstance* inst = m_Instances[name];

    inst->RawCMethodCall(methodCall->m_DukThisHeapPtr, methodCall->m_Func, methodCall->m_ArgSetupFunc, methodCall->m_ArgSetupParam);

    if (!inst->IsStopping() && inst->GetRefCount() == 0)
    {
        NotifyStatus(name, JS_STATUS_STOPPED);
        RawRemoveInstance(name);
    }
}

void CScriptSystem::OnSweep(bool bIfDone)
{
    JSInstanceMap::iterator it = m_Instances.begin();
    while(it != m_Instances.end())
    {
        CScriptInstance*& inst = it->second;
        if(!bIfDone || inst->GetRefCount() == 0)
        {
            NotifyStatus(inst->Name().c_str(), JS_STATUS_STOPPED);
            delete inst;
            m_Instances.erase(it);
        }
        else
        {
            it++;
        }
    }
}

bool CScriptSystem::RawRemoveInstance(const char *name)
{
    if(m_Instances.count(name) == 0)
    {
        return false;
    }

    CScriptInstance*& inst = m_Instances[name];

    if (inst->IsStopping())
    {
        return false;
    }

    inst->SetStopping(true);
    inst->StopRegisteredWorkers();
    
    std::vector<JSSysCommand> pendingCalls;
    PullCommands(JS_CMD_C_METHOD_CALL, pendingCalls);
    ProcessCommandQueue(pendingCalls);

    delete inst;
    m_Instances.erase(name);
    return true;
}

JSAppCallbackID CScriptSystem::RawAddAppCallback(JSAppHookID hookId, JSAppCallback& callback)
{
    if(hookId >= JS_NUM_APP_HOOKS)
    {
        return JS_INVALID_CALLBACK;
    }

    callback.m_Instance->IncRefCount();
    callback.m_CallbackId = m_NextAppCallbackId++;
    m_AppCallbackHooks[hookId].push_back(callback);
    m_AppCallbackCount++;
    return callback.m_CallbackId;
}

void CScriptSystem::RawRemoveAppCallback(JSAppHookID hookId, JSAppCallbackID callbackId)
{
    JSAppCallbackList::iterator it = m_AppCallbackHooks[hookId].begin();
    while (it != m_AppCallbackHooks[hookId].end())
    {
        if (it->m_CallbackId == callbackId)
        {
            m_AppCallbackCount--;
            it->m_Instance->DecRefCount();
            it->m_bDisabled = true;
            return;
        }

        it++;
    }
}

void CScriptSystem::ExecAutorunList()
{
    LoadAutorunList();

    std::istringstream joinedNames(g_Settings->LoadStringVal(Debugger_AutorunScripts));
    std::string scriptName;
    while (std::getline(joinedNames, scriptName, '|'))
    {
        std::string scriptPath = m_ScriptsDirFullPath + scriptName;
        ConsoleLog("[SCRIPTSYS]: autorun '%s'", scriptName.c_str());
        StartScript(scriptName.c_str(), scriptPath.c_str());
    }
}

std::set<std::string>& CScriptSystem::AutorunList()
{
    return m_AutorunList;
}

void CScriptSystem::LoadAutorunList()
{
    m_AutorunList.clear();

    std::istringstream joinedNames(g_Settings->LoadStringVal(Debugger_AutorunScripts));
    std::string scriptName;

    while (std::getline(joinedNames, scriptName, '|'))
    {
        m_AutorunList.insert(scriptName);
    }
}

void CScriptSystem::SaveAutorunList()
{
    std::string joinedNames = "";

    std::set<std::string>::iterator it;
    for (it = m_AutorunList.begin(); it != m_AutorunList.end(); it++)
    {
        if (it != m_AutorunList.begin())
        {
            joinedNames += "|";
        }
        joinedNames += *it;
    }

    g_Settings->SaveString(Debugger_AutorunScripts, joinedNames.c_str());
}

CDebuggerUI* CScriptSystem::Debugger()
{
    return m_Debugger;
}

stdstr CScriptSystem::FixStringReturns(const char* str)
{
    stdstr fstr = str;
    size_t pos = 0;
    while ((pos = fstr.find("\n", pos)) != stdstr::npos)
    {
        fstr.replace(pos, 1, "\r\n");
        pos += 2;
    }
    return fstr;
}

bool CScriptSystem::ProcessCommandQueue(std::vector<JSSysCommand>& queue)
{
    std::vector<JSSysCommand>::iterator it;
    for (it = queue.begin(); it != queue.end(); it++)
    {
        if (!ProcessCommand(*it))
        {
            return false;
        }
    }

    return true;
}

bool CScriptSystem::ProcessCommand(JSSysCommand& cmd)
{
    CGuard guard(m_InstancesCS);

    switch (cmd.id)
    {
    case JS_CMD_START_SCRIPT:
        OnStartScript(cmd.paramA.c_str(), cmd.paramB.c_str());
        break;
    case JS_CMD_STOP_SCRIPT:
        OnStopScript(cmd.paramA.c_str());
        break;
    case JS_CMD_INPUT:
        OnInput(cmd.paramA.c_str(), cmd.paramB.c_str());
        break;
    case JS_CMD_C_METHOD_CALL:
        {
            JSSysCMethodCall* methodCall = (JSSysCMethodCall*)cmd.paramC;
            OnCMethodCall(cmd.paramA.c_str(), methodCall);
            delete methodCall;
        }
        break;
    case JS_CMD_SWEEP:
        OnSweep(true);
        break;
    case JS_CMD_SHUTDOWN:
        OnSweep(false);
        return false;
    }

    RefreshCallbackMaps();
    return true;
}

void CScriptSystem::PullCommands(JSSysCommandID id, std::vector<JSSysCommand>& out)
{
    CGuard guard(m_CmdQueueCS);

    std::vector<JSSysCommand>::iterator it;
    for (it = m_CmdQueue.begin(); it != m_CmdQueue.end();)
    {
        if ((*it).id == id)
        {
            out.push_back(*it);
            it = m_CmdQueue.erase(it);
        }
        else
        {
            it++;
        }
    }
}
