#include "stdafx.h"

#include "ScriptAPI.h"
#include <windows.h>

static void ConcatArgs(duk_context * ctx, stdstr & out);

void ScriptAPI::Define_console(duk_context * ctx)
{
    const DukPropListEntry props[] = {
        {"print", DukCFunction(js_console_print)},
        {"log", DukCFunction(js_console_log)},
        {"error", DukCFunction(js_console_error)},
        {"clear", DukCFunction(js_console_clear)},
        {"listen", DukCFunction(js_console_listen)},
        { nullptr }
    };

    DefineGlobalInterface(ctx, "console", props);
}

duk_ret_t ScriptAPI::js_console_print(duk_context * ctx)
{
    stdstr out;
    ConcatArgs(ctx, out);
    GetInstance(ctx)->System()->ConsolePrint("%s", out.c_str());
    return 0;
}

duk_ret_t ScriptAPI::js_console_log(duk_context * ctx)
{
    stdstr out;
    ConcatArgs(ctx, out);
    GetInstance(ctx)->System()->ConsoleLog("%s", out.c_str());
    return 0;
}

duk_ret_t ScriptAPI::js_console_error(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_OptAny});

    if (duk_is_error(ctx, 0))
    {
        duk_get_prop_string(ctx, 0, "stack");
        const char * message = duk_get_string(ctx, -1);
        GetInstance(ctx)->System()->ConsoleLog("%s", message);
        return 0;
    }

    GetInstance(ctx)->System()->ConsoleLog("%s", duk_safe_to_string(ctx, 0));
    return 0;
}

duk_ret_t ScriptAPI::js_console_clear(duk_context * ctx)
{
    CheckArgs(ctx, {});
    GetInstance(ctx)->System()->ConsoleClear();
    return 0;
}

duk_ret_t ScriptAPI::js_console_listen(duk_context * ctx)
{
    CScriptInstance * inst = GetInstance(ctx);

    duk_push_global_object(ctx);
    duk_bool_t haveListener = duk_has_prop_string(ctx, -1, HS_gInputListener);

    if (duk_is_function(ctx, 0))
    {
        duk_pull(ctx, 0);
        duk_put_prop_string(ctx, -2, HS_gInputListener);
        if (!haveListener)
        {
            inst->IncRefCount();
        }
        return 0;
    }
    else if (duk_is_null(ctx, 0))
    {
        if (haveListener)
        {
            duk_del_prop_string(ctx, -1, HS_gInputListener);
            inst->DecRefCount();
        }
        return 0;
    }

    return ThrowInvalidArgsError(ctx);
}

void ConcatArgs(duk_context * ctx, stdstr & out)
{
    out = "";
    duk_idx_t nargs = duk_get_top(ctx);

    // note: global JSON.stringify must be intact

    duk_get_global_string(ctx, "JSON");
    duk_get_prop_string(ctx, -1, "stringify");
    duk_remove(ctx, -2);

    for (duk_idx_t n = 0; n < nargs; n++)
    {
        if (n != 0)
        {
            out += " ";
        }

        if (duk_is_object(ctx, n))
        {
            duk_dup(ctx, n);
            out += duk_safe_to_string(ctx, -1);
            out += " ";
            duk_pop(ctx);
            duk_dup(ctx, -1);
            duk_dup(ctx, n);
            duk_push_null(ctx);
            duk_push_int(ctx, 2);
            duk_pcall(ctx, 3);
            out += duk_safe_to_string(ctx, -1);
            duk_pop(ctx);
        }
        else
        {
            out += duk_safe_to_string(ctx, n);
        }
    }
}
