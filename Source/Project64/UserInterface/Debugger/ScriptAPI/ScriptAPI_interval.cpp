#include "stdafx.h"

#include "JSIntervalWorker.h"
#include "ScriptAPI.h"

using namespace ScriptAPI;

static int AddIntervalContext(duk_context * ctx, duk_idx_t func_idx, int delayMS, bool bOnce);
static void RemoveIntervalContext(duk_context * ctx, int intervalId);

void ScriptAPI::Define_interval(duk_context * ctx)
{
    DefineGlobalFunction(ctx, "setInterval", js_setInterval);
    DefineGlobalFunction(ctx, "clearInterval", js_clearInterval);
    DefineGlobalFunction(ctx, "setTimeout", js_setTimeout);
    DefineGlobalFunction(ctx, "clearTimeout", js_clearTimeout);
}

duk_ret_t ScriptAPI::js_setInterval(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Function, Arg_Number});
    int delayMS = duk_get_int(ctx, 1);
    int intervalId = AddIntervalContext(ctx, 0, delayMS, false);
    duk_push_int(ctx, intervalId);
    return 1;
}

duk_ret_t ScriptAPI::js_clearInterval(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});
    int intervalId = duk_get_int(ctx, 0);
    RemoveIntervalContext(ctx, intervalId);
    return 0;
}

duk_ret_t ScriptAPI::js_setTimeout(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Function, Arg_Number});
    int delayMS = duk_get_int(ctx, 1);
    int intervalId = AddIntervalContext(ctx, 0, delayMS, true);
    duk_push_int(ctx, intervalId);
    return 1;
}

duk_ret_t ScriptAPI::js_clearTimeout(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});
    int intervalId = duk_get_int(ctx, 0);
    RemoveIntervalContext(ctx, intervalId);
    return 0;
}

duk_ret_t ScriptAPI::js__IntervalContext_invokeFunc(duk_context * ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "func");
    duk_pcall(ctx, 0);
    return 0;
}

duk_ret_t ScriptAPI::js__IntervalContext_remove(duk_context * ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "id");
    int intervalId = duk_get_int(ctx, -1);
    RemoveIntervalContext(ctx, intervalId);
    return 0;
}

duk_ret_t ScriptAPI::js__IntervalContext_finalizer(duk_context * ctx)
{
    duk_get_prop_string(ctx, 0, "worker");
    CJSIntervalWorker * intervalWorker = (CJSIntervalWorker *)duk_get_pointer(ctx, -1);

    if (intervalWorker != nullptr)
    {
        delete intervalWorker;
    }

    return 0;
}

int AddIntervalContext(duk_context * ctx, duk_idx_t func_idx, int delayMS, bool bOnce)
{
    func_idx = duk_normalize_index(ctx, func_idx);

    CScriptInstance * inst = GetInstance(ctx);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, HS_gNextInvervalId);
    int intervalId = duk_get_int(ctx, -1);
    duk_pop(ctx);
    duk_push_int(ctx, intervalId + 1);
    duk_put_prop_string(ctx, -2, HS_gNextInvervalId);

    duk_get_prop_string(ctx, -1, HS_gIntervals);

    duk_push_object(ctx);
    RefObject(ctx, -1);

    void * objectHeapPtr = duk_get_heapptr(ctx, -1);
    CJSIntervalWorker * intervalWorker = new CJSIntervalWorker(inst, objectHeapPtr, delayMS, bOnce);

    duk_dup(ctx, func_idx);
    duk_put_prop_string(ctx, -2, "func");

    duk_push_pointer(ctx, intervalWorker);
    duk_put_prop_string(ctx, -2, "worker");

    duk_push_int(ctx, intervalId);
    duk_put_prop_string(ctx, -2, "id");

    duk_push_c_function(ctx, js__IntervalContext_finalizer, 1);
    duk_set_finalizer(ctx, -2);

    duk_put_prop_index(ctx, -2, intervalId);
    duk_pop_n(ctx, 2);

    intervalWorker->StartWorkerProc();

    return intervalId;
}

void RemoveIntervalContext(duk_context * ctx, int intervalId)
{
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, HS_gIntervals);

    if (!duk_has_prop_index(ctx, -1, intervalId))
    {
        return;
    }

    duk_get_prop_index(ctx, -1, intervalId);
    UnrefObject(ctx, -1);

    duk_get_prop_string(ctx, -1, "worker");

    CJSIntervalWorker * intervalWorker = (CJSIntervalWorker *)duk_get_pointer(ctx, -1);
    intervalWorker->StopWorkerProc();
    delete intervalWorker;

    duk_pop(ctx);
    duk_push_pointer(ctx, nullptr);
    duk_put_prop_string(ctx, -2, "worker");
    duk_pop(ctx);
    duk_del_prop_index(ctx, -1, intervalId);

    duk_pop_n(ctx, 2);
}
