#include <3rdparty/duktape/duktape.h>
#include <3rdparty/duktape/duk_module_duktape.h>
#include <cstdint>
#include <string>

#include "OpInfo.h"

#pragma once

enum
{
    JS_EXEC_TIMEOUT = 500
};

enum JSAppHookID
{
    JS_HOOK_CPU_EXEC,
    JS_HOOK_CPU_READ,
    JS_HOOK_CPU_WRITE,
    JS_HOOK_PIFREAD,
    JS_HOOK_PIDMA,
    JS_HOOK_GFXUPDATE,
    JS_HOOK_RSPTASK,
    JS_HOOK_EMUSTATECHANGE,
    JS_HOOK_MOUSEUP,
    JS_HOOK_MOUSEDOWN,
    JS_HOOK_MOUSEMOVE,
    JS_NUM_APP_HOOKS
};

enum JSInstanceStatus
{
    JS_STATUS_STOPPED,
    JS_STATUS_STARTING,
    JS_STATUS_STARTED
};

enum JSSysCommandID
{
    JS_CMD_IDLE,
    JS_CMD_START_SCRIPT,
    JS_CMD_STOP_SCRIPT,
    JS_CMD_SWEEP,
    JS_CMD_INPUT,
    JS_CMD_SHUTDOWN,
    JS_CMD_C_METHOD_CALL
};

enum JSEmuState
{
    JS_EMU_STARTED,
    JS_EMU_STOPPED,
    JS_EMU_RESETTING,
    JS_EMU_RESET,
    JS_EMU_PAUSED,
    JS_EMU_RESUMED,
    JS_EMU_LOADED_ROM,
    JS_EMU_LOADED_STATE, // todo
    JS_EMU_DEBUG_PAUSED,
    JS_EMU_DEBUG_RESUMED
};

class CScriptSystem;
class CScriptInstance;

typedef std::string JSInstanceName;

struct JSAppCallback;
typedef duk_idx_t (*JSDukArgSetupFunc)(duk_context *ctx, void *argSetupParam);
typedef bool (*JSAppCallbackCondFunc)(JSAppCallback* cb, void* hookEnv);
typedef void (*JSAppCallbackCleanupFunc)(duk_context* ctx, void *hookEnv);

typedef size_t JSAppCallbackID;
#define JS_INVALID_CALLBACK ((JSAppCallbackID)(-1))

struct JSAppCallback
{
    // assigned by scriptsys when this is added to a callback map
    JSAppCallbackID          m_CallbackId;
    bool                     m_bDisabled;

    CScriptInstance         *m_Instance;
    void                    *m_DukFuncHeapPtr;
    JSAppCallbackCondFunc    m_ConditionFunc;
    JSDukArgSetupFunc        m_DukArgSetupFunc;
    JSAppCallbackCleanupFunc m_CleanupFunc;

    struct {
        uint32_t addrStart, addrEnd;
        union {
            struct { uint32_t opcode, opcodeMask; };
            struct { uint32_t regIndices, regValue; };
        };
    } m_Params;

    static bool CbCondTrue(JSAppCallback*, void*) { return true; }

    JSAppCallback(CScriptInstance* instance, void* dukFuncHeapPtr,
                  JSAppCallbackCondFunc condFunc = nullptr,
                  JSDukArgSetupFunc argSetupFunc = nullptr,
                  JSAppCallbackCleanupFunc cleanupFunc = nullptr) :
        m_CallbackId(JS_INVALID_CALLBACK),
        m_bDisabled(false),
        m_Instance(instance),
        m_DukFuncHeapPtr(dukFuncHeapPtr),
        m_ConditionFunc(condFunc),
        m_DukArgSetupFunc(argSetupFunc),
        m_CleanupFunc(cleanupFunc)
    {
        if (m_ConditionFunc == nullptr)
        {
            m_ConditionFunc = CbCondTrue;
        }

        m_Params = {};
    }

    JSAppCallback() :
        m_CallbackId(JS_INVALID_CALLBACK),
        m_bDisabled(false),
        m_Instance(nullptr),
        m_DukFuncHeapPtr(nullptr),
        m_ConditionFunc(nullptr),
        m_DukArgSetupFunc(nullptr),
        m_CleanupFunc(nullptr)
    {
        if (m_ConditionFunc == nullptr)
        {
            m_ConditionFunc = CbCondTrue;
        }

        m_Params = {};
    }
};

struct JSHookCpuStepEnv
{
    uint32_t  pc;
    COpInfo   opInfo;
    int       outAffectedRegIndex; // set by the condition function
};

struct JSHookMouseEnv
{
    int x, y;
    int button; // 0=left,1=middle,2=right
};

struct JSHookSpTaskEnv
{
    uint32_t taskType;
    uint32_t taskFlags;
    uint32_t ucodeBootAddress;
    uint32_t ucodeBootSize;
    uint32_t ucodeAddress;
    uint32_t ucodeSize;
    uint32_t ucodeDataAddress;
    uint32_t ucodeDataSize;
    uint32_t dramStackAddress;
    uint32_t dramStackSize;
    uint32_t outputBuffAddress;
    uint32_t outputBuffSize;
    uint32_t dataAddress;
    uint32_t dataSize;
    uint32_t yieldDataAddress;
    uint32_t yieldDataSize;
};

struct JSHookPiDmaEnv
{
    int      direction;
    uint32_t dramAddress;
    uint32_t cartAddress;
    uint32_t length;
};

struct JSHookEmuStateChangeEnv
{
    JSEmuState state;
};

struct JSSysCommand
{
    JSSysCommandID id;
    stdstr         paramA;
    stdstr         paramB;
    void*          paramC;
};

struct JSSysCMethodCall
{
    void*                 m_DukThisHeapPtr;
    duk_c_function        m_Func;
    JSDukArgSetupFunc     m_ArgSetupFunc;
    void*                 m_ArgSetupParam;

    JSSysCMethodCall(void* dukThisHeapPtr, duk_c_function func,
                     JSDukArgSetupFunc argSetupFunc = nullptr,
                     void* argSetupParam = nullptr,
                     size_t argSetupParamSize = 0) :
        m_DukThisHeapPtr(dukThisHeapPtr),
        m_Func(func),
        m_ArgSetupFunc(argSetupFunc),
        m_ArgSetupParam(nullptr)
    {
        if (argSetupParam != nullptr && argSetupParamSize != 0)
        {
            m_ArgSetupParam = (void*)(new char[argSetupParamSize]);
            memcpy(m_ArgSetupParam, argSetupParam, argSetupParamSize);
        }
    }

    ~JSSysCMethodCall()
    {
        if (m_ArgSetupParam != nullptr)
        {
            delete[](char*)m_ArgSetupParam;
        }
    }
};
