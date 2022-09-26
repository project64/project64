#include "stdafx.h"

#include "ScriptAPI.h"
#include <Project64-core/Settings/DebugSettings.h>

void ScriptAPI::Define_debug(duk_context * ctx)
{
    const DukPropListEntry props[] = {
        {"breakhere", DukCFunction(js_debug_breakhere)},
        {"step", DukCFunction(js_debug_step)},
        {"skip", DukCFunction(js_debug_skip)},
        {"resume", DukCFunction(js_debug_resume)},
        {"showmemory", DukCFunction(js_debug_showmemory)},
        {"showcommands", DukCFunction(js_debug_showcommands)},
        {"paused", DukGetter(js_debug__get_paused)},
        {nullptr},
    };

    DefineGlobalInterface(ctx, "debug", props);
}

duk_ret_t ScriptAPI::js_debug_breakhere(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_OptBoolean});

    if (duk_get_boolean_default(ctx, 0, (duk_bool_t) false) == 1)
    {
        g_Settings->SaveBool(Debugger_SilentBreak, true);
    }
    g_Settings->SaveBool(Debugger_SteppingOps, true);
    return 0;
}

duk_ret_t ScriptAPI::js_debug_step(duk_context * ctx)
{
    CheckArgs(ctx, {});

    if (g_Settings->LoadBool(Debugger_SteppingOps) && CDebugSettings::WaitingForStep())
    {
        g_Settings->SaveBool(Debugger_SilentBreak, true);
        GetInstance(ctx)->Debugger()->StepEvent().Trigger();
    }
    return 0;
}

duk_ret_t ScriptAPI::js_debug_skip(duk_context * ctx)
{
    CheckArgs(ctx, {});

    g_Settings->SaveBool(Debugger_SkipOp, true);

    if (g_Settings->LoadBool(Debugger_SteppingOps) && CDebugSettings::WaitingForStep())
    {
        GetInstance(ctx)->Debugger()->StepEvent().Trigger();
    }
    return 0;
}

duk_ret_t ScriptAPI::js_debug_showmemory(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_OptBoolean});

    uint32_t address = duk_get_uint(ctx, 0);
    bool bPhysical = duk_get_boolean_default(ctx, 1, 0) ? true : false;

    GetInstance(ctx)->Debugger()->Debug_ShowMemoryLocation(address, !bPhysical);
    return 0;
}

duk_ret_t ScriptAPI::js_debug_showcommands(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});
    uint32_t address = duk_get_uint(ctx, 0);
    GetInstance(ctx)->Debugger()->Debug_ShowCommandsLocation(address, true);
    return 0;
}

duk_ret_t ScriptAPI::js_debug_resume(duk_context * ctx)
{
    CheckArgs(ctx, {});

    g_Settings->SaveBool(Debugger_SteppingOps, false);

    if (CDebugSettings::WaitingForStep())
    {
        GetInstance(ctx)->Debugger()->StepEvent().Trigger();
    }
    return 0;
}

duk_ret_t ScriptAPI::js_debug__get_paused(duk_context * ctx)
{
    duk_push_boolean(ctx, CDebugSettings::WaitingForStep() && g_Settings->LoadBool(Debugger_SteppingOps));
    return 1;
}
