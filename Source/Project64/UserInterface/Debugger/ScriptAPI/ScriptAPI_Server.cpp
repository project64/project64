#include <stdafx.h>
#include "ScriptAPI.h"
#include "JSServerWorker.h"

static CJSServerWorker* GetThisServer(duk_context* ctx);

void ScriptAPI::Define_Server(duk_context* ctx)
{
    const DukPropListEntry prototype[] = {
        { "listen",  DukCFunction(js_Server_listen) },
        { "close",   DukCFunction(js_Server_close) },
        { "on",      DukCFunction(js__Emitter_on) },
        { "off",     DukCFunction(js__Emitter_off) },
        { "port",    DukGetter(js_Server__get_port) },
        { "address", DukGetter(js_Server__get_address) },
        { "addressFamily",  DukGetter(js_Server__get_addressFamily) },
        { nullptr }
    };
    
    DefineGlobalClass(ctx, "Server", js_Server__constructor, prototype);
}

duk_ret_t ScriptAPI::js_Server__constructor(duk_context* ctx)
{
    CheckArgs(ctx, {});

    if (!duk_is_constructor_call(ctx))
    {
        return DUK_RET_ERROR;
    }

    CScriptInstance* inst = GetInstance(ctx);

    duk_push_this(ctx);
    void* objectHeapPtr = duk_get_heapptr(ctx, -1);

    InitEmitter(ctx, -1, {
        "close",
        "connection",
        "error",
        "listening"
    });
    
    duk_push_c_function(ctx, js_Server__finalizer, 1);
    duk_set_finalizer(ctx, -2);

    CJSServerWorker* serverWorker = new CJSServerWorker(inst, objectHeapPtr);
    duk_push_pointer(ctx, serverWorker);
    duk_put_prop_string(ctx, -2, HS_serverWorkerPtr);

    return 0;
}

duk_ret_t ScriptAPI::js_Server__finalizer(duk_context* ctx)
{
    UnrefObject(ctx, 0);
    duk_get_prop_string(ctx, 0, HS_serverWorkerPtr);
    CJSServerWorker* serverWorker = (CJSServerWorker*)duk_get_pointer(ctx, -1);
    delete serverWorker;
    return 0;
}

duk_ret_t ScriptAPI::js_Server_listen(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Number, Arg_String, Arg_OptFunction });
    CJSServerWorker* serverWorker = GetThisServer(ctx);

    unsigned short port = (unsigned short)duk_get_int(ctx, 0);
    const char* address = duk_get_string(ctx, 1);

    // todo callback

    duk_push_this(ctx);
    RefObject(ctx, -1);

    serverWorker->Init(address, port);
    serverWorker->StartWorkerProc();

    return 0;
}

duk_ret_t ScriptAPI::js_Server_close(duk_context* ctx)
{
    CheckArgs(ctx, {});

    CJSServerWorker* serverWorker = GetThisServer(ctx);
    serverWorker->StopWorkerProc();
    return 0;
}

duk_ret_t ScriptAPI::js_Server__get_port(duk_context* ctx)
{
    CJSServerWorker* serverWorker = GetThisServer(ctx);
    duk_push_uint(ctx, serverWorker->GetPort());
    return 1;
}

duk_ret_t ScriptAPI::js_Server__get_address(duk_context* ctx)
{
    CJSServerWorker* serverWorker = GetThisServer(ctx);
    duk_push_string(ctx, serverWorker->GetAddress().c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_Server__get_addressFamily(duk_context* ctx)
{
    CJSServerWorker* serverWorker = GetThisServer(ctx);
    duk_push_string(ctx, serverWorker->GetFamily());
    return 1;
}

CJSServerWorker* GetThisServer(duk_context* ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HS_serverWorkerPtr);
    CJSServerWorker* serverWorker = (CJSServerWorker*)duk_get_pointer(ctx, -1);
    duk_pop_n(ctx, 2);
    return serverWorker;
}
