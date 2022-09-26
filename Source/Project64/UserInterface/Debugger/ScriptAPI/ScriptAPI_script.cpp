#include <stdafx.h>

#include "ScriptAPI.h"

#pragma warning(disable : 4702) // disable unreachable code warning

void ScriptAPI::Define_script(duk_context * ctx)
{
    const DukPropListEntry props[] = {
        {"timeout", DukCFunction(js_script_timeout)},
        {"keepalive", DukCFunction(js_script_keepalive)},
        {"abort", DukCFunction(js_script_abort)},
        {nullptr},
    };

    DefineGlobalInterface(ctx, "script", props);
}

duk_ret_t ScriptAPI::js_script_timeout(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});
    CScriptInstance * inst = GetInstance(ctx);
    inst->SetExecTimeout((uint64_t)duk_get_number(ctx, 0));
    return 0;
}

duk_ret_t ScriptAPI::js_script_keepalive(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Boolean});
    CScriptInstance * inst = GetInstance(ctx);

    duk_bool_t bKeepAlive = duk_get_boolean(ctx, 0);

    duk_push_global_object(ctx);
    duk_bool_t bHaveProp = duk_has_prop_string(ctx, -1, HS_gKeepAlive);

    if (bKeepAlive && !bHaveProp)
    {
        duk_push_boolean(ctx, 1);
        duk_put_prop_string(ctx, -2, HS_gKeepAlive);
        inst->IncRefCount();
    }
    else if (!bKeepAlive && bHaveProp)
    {
        duk_del_prop_string(ctx, -1, HS_gKeepAlive);
        inst->DecRefCount();
    }

    duk_pop(ctx);

    return 0;
}

duk_ret_t ScriptAPI::js_script_abort(duk_context * ctx)
{
    CheckArgs(ctx, {});
    CScriptInstance * inst = GetInstance(ctx);
    if (inst->PrepareAbort())
    {
        return duk_fatal(ctx, "aborted");
    }
    return 0;
}
