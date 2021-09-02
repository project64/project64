#include <stdafx.h>
#include <windows.h>
#include "ScriptAPI.h"

void ScriptAPI::Define_alert(duk_context* ctx)
{
    DefineGlobalFunction(ctx, "alert", js_alert);
}

duk_ret_t ScriptAPI::js_alert(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Any, Arg_OptAny });
    duk_idx_t nargs = duk_get_top(ctx);

    const char* message = duk_safe_to_string(ctx, 0);
    const char* caption = (nargs == 2) ? duk_safe_to_string(ctx, 1) : "";

    HWND mainWindow = (HWND)g_Plugins->MainWindow()->GetWindowHandle();
    MessageBoxA(mainWindow, message, caption, MB_OK);
    return 0;
}
