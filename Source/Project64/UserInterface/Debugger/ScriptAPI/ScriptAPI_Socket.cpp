#include <stdafx.h>
#include "ScriptAPI.h"
#include "JSSocketWorker.h"

#pragma warning(disable: 4702) // disable unreachable code warning

static CJSSocketWorker* GetThisSocket(duk_context* ctx);
static duk_ret_t RequireBufferDataOrString(duk_context* ctx, duk_idx_t idx, const char** data, duk_size_t* size);
static duk_int_t RegisterWriteCallback(duk_context* ctx, duk_idx_t idx);
static void RegisterWriteEndCallback(duk_context* ctx, duk_idx_t idx);

void ScriptAPI::Define_Socket(duk_context* ctx)
{
    const DukPropListEntry prototype[] = {
        { "connect",       DukCFunction(js_Socket_connect) },
        { "write",         DukCFunction(js_Socket_write) },
        { "end",           DukCFunction(js_Socket_end) },
        { "close",         DukCFunction(js_Socket_close) },
        { "on",            DukCFunction(js__Emitter_on) },
        { "off",           DukCFunction(js__Emitter_off) },
        { "localAddress",  DukGetter(js_Socket__get_localAddress) },
        { "localPort",     DukGetter(js_Socket__get_localPort) },
        { "remoteAddress", DukGetter(js_Socket__get_remoteAddress) },
        { "remotePort",    DukGetter(js_Socket__get_remotePort) },
        { "addressFamily", DukGetter(js_Socket__get_addressFamily) },
        { nullptr }
    };

    DefineGlobalClass(ctx, "Socket", js_Socket__constructor, prototype, nullptr);
}

duk_ret_t ScriptAPI::js_Socket__constructor(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_OptObject });

    if (!duk_is_constructor_call(ctx))
    {
        return DUK_RET_ERROR;
    }

    bool bAllowHalfOpen = false;

    CScriptInstance* inst = GetInstance(ctx);

    if (duk_is_object(ctx, 0))
    {
        if (duk_has_prop_string(ctx, 0, "allowHalfOpen"))
        {
            duk_get_prop_string(ctx, 0, "allowHalfOpen");
            bAllowHalfOpen = duk_get_boolean_default(ctx, -1, 0) != (duk_bool_t)false;
            duk_pop(ctx);
        }
    }

    duk_push_this(ctx);
    void* objectHeapPtr = duk_get_heapptr(ctx, -1);
    
    InitEmitter(ctx, -1, {
        "data",
        "end",
        "connect",
        "error",
        "close",
        "drain",
        "lookup"
    });

    duk_push_uint(ctx, 0);
    duk_put_prop_string(ctx, -2, HS_socketNextWriteCallbackId);

    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, HS_socketWriteCallbacks);

    duk_push_array(ctx);
    duk_put_prop_string(ctx, -2, HS_socketWriteEndCallbacks);

    CJSSocketWorker* socketWorker = new CJSSocketWorker(inst, objectHeapPtr, bAllowHalfOpen);

    duk_push_pointer(ctx, (void*)socketWorker);
    duk_put_prop_string(ctx, -2, HS_socketWorkerPtr);

    duk_push_c_function(ctx, js_Socket__finalizer, 1);
    duk_set_finalizer(ctx, -2);

    return 0;
}

duk_ret_t ScriptAPI::js_Socket__finalizer(duk_context* ctx)
{
    duk_get_prop_string(ctx, 0, HS_socketWorkerPtr);
    CJSSocketWorker* socketWorker = (CJSSocketWorker*)duk_get_pointer(ctx, -1);

    if (socketWorker == nullptr)
    {
        return 0;
    }

    UnrefObject(ctx, 0);
    delete socketWorker;

    return 0;
}

duk_ret_t ScriptAPI::js_Socket_connect(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Number, Arg_String, Arg_OptFunction });

    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    
    duk_idx_t nargs = duk_get_top(ctx);
    unsigned short port = (unsigned short)duk_get_uint(ctx, 0);
    const char* host = duk_get_string(ctx, 1);

    if (nargs == 3)
    {
        // add 'connect' event listener
        duk_push_c_function(ctx, js__Emitter_on, 2); // todo once
        duk_push_this(ctx);
        duk_push_string(ctx, "connect");
        duk_pull(ctx, 2);
        duk_pcall_method(ctx, 2);
        duk_pop(ctx);
    }

    duk_push_this(ctx);
    RefObject(ctx, -1);

    socketWorker->Init(host, port);
    socketWorker->StartWorkerProc();

    return 0;
}

duk_ret_t ScriptAPI::js_Socket_write(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);

    const char* data;
    duk_size_t size;
    duk_int_t callbackId = -1;
    
    duk_idx_t nargs = duk_get_top(ctx);
    RequireBufferDataOrString(ctx, 0, &data, &size);

    if (nargs == 2 && duk_is_function(ctx, 1))
    {
        callbackId = RegisterWriteCallback(ctx, 1);
    }

    socketWorker->Write((char*)data, size, callbackId, false);
    return 0;
}

duk_ret_t ScriptAPI::js_Socket_end(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);

    const char* data;
    duk_size_t size;
    duk_int_t callbackId = -1;

    duk_idx_t nargs = duk_get_top(ctx);
    RequireBufferDataOrString(ctx, 0, &data, &size);

    if (nargs == 2 && duk_is_function(ctx, 1))
    {
        RegisterWriteEndCallback(ctx, 1);
    }

    socketWorker->Write((char*)data, size, callbackId, true);
    return 0;
}

duk_ret_t ScriptAPI::js_Socket__invokeWriteCallback(duk_context* ctx)
{
    duk_int_t callbackId = duk_get_int(ctx, 0);

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HS_socketWriteCallbacks);
    duk_get_prop_index(ctx, -1, callbackId);
    duk_dup(ctx, -3);

    if (duk_pcall_method(ctx, 0) != 0)
    {
        duk_throw(ctx);
    }

    duk_pop(ctx);
    duk_del_prop_index(ctx, -1, callbackId);

    return 0;
}

duk_ret_t ScriptAPI::js_Socket__invokeWriteEndCallbacks(duk_context* ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HS_socketWriteEndCallbacks);
    duk_size_t numCallbacks = duk_get_length(ctx, -1);
    for (duk_size_t i = 0; i < numCallbacks; i++)
    {
        duk_get_prop_index(ctx, -1, i);
        duk_dup(ctx, -3);

        if (duk_pcall_method(ctx, 0) != 0)
        {
            duk_throw(ctx);
        }

        duk_pop(ctx);
    }

    duk_pop(ctx);

    // reset array
    duk_push_array(ctx); 
    duk_put_prop_string(ctx, -2, HS_socketWriteEndCallbacks);

    return 0;
}

duk_ret_t ScriptAPI::js_Socket_close(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    socketWorker->StopWorkerProc();
    return 0;
}

duk_ret_t ScriptAPI::js_Socket__get_localAddress(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    duk_push_string(ctx, socketWorker->GetLocalAddress().c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_Socket__get_localPort(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    duk_push_uint(ctx, socketWorker->GetLocalPort());
    return 1;
}

duk_ret_t ScriptAPI::js_Socket__get_remoteAddress(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    duk_push_string(ctx, socketWorker->GetRemoteAddress().c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_Socket__get_remotePort(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    duk_push_uint(ctx, socketWorker->GetRemotePort());
    return 1;
}

duk_ret_t ScriptAPI::js_Socket__get_addressFamily(duk_context* ctx)
{
    CJSSocketWorker* socketWorker = GetThisSocket(ctx);
    duk_push_string(ctx, socketWorker->GetFamily());
    return 1;
}

CJSSocketWorker* GetThisSocket(duk_context* ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HS_socketWorkerPtr);
    CJSSocketWorker* socketWorker = (CJSSocketWorker*)duk_get_pointer(ctx, -1);
    duk_pop_n(ctx, 2);

    if (socketWorker == nullptr)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "internal socket object is null");
        return duk_throw(ctx);
    }

    return socketWorker;
}

duk_ret_t RequireBufferDataOrString(duk_context* ctx, duk_idx_t idx, const char** data, duk_size_t* size)
{
    if (duk_is_buffer_data(ctx, idx))
    {
        *data = (const char*)duk_get_buffer_data(ctx, idx, size);
        return 0;
    }
    else if (duk_is_string(ctx, idx))
    {
        *data = duk_get_lstring(ctx, idx, size);
        return 0;
    }

    duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "arg %d must be buffer-like or string", idx);
    return duk_throw(ctx);
}

duk_int_t RegisterWriteCallback(duk_context* ctx, duk_idx_t idx)
{
    idx = duk_normalize_index(ctx, idx);

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HS_socketNextWriteCallbackId);
    duk_int_t callbackId = duk_get_int(ctx, -1);
    duk_pop(ctx);

    duk_get_prop_string(ctx, -1, HS_socketWriteCallbacks);
    duk_dup(ctx, idx);
    duk_put_prop_index(ctx, -2, callbackId);
    duk_pop(ctx);

    duk_push_int(ctx, callbackId + 1);
    duk_put_prop_string(ctx, -2, HS_socketNextWriteCallbackId);

    return callbackId;
}

void RegisterWriteEndCallback(duk_context* ctx, duk_idx_t idx)
{
    idx = duk_normalize_index(ctx, idx);

    duk_push_this(ctx);

    duk_get_prop_string(ctx, -1, HS_socketWriteEndCallbacks);
    duk_size_t callbackIdx = duk_get_length(ctx, -1);
    duk_dup(ctx, idx);
    duk_put_prop_index(ctx, -2, callbackIdx);
    duk_pop(ctx);
}
