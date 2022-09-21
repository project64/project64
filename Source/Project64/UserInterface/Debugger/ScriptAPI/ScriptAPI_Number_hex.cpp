#include "stdafx.h"

#include "ScriptAPI.h"

void ScriptAPI::Define_Number_prototype_hex(duk_context * ctx)
{
    duk_get_global_string(ctx, "Number");
    duk_get_prop_string(ctx, -1, "prototype");
    duk_push_c_function(ctx, js_Number_prototype_hex, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "hex");
    duk_pop_n(ctx, 2);
}

duk_ret_t ScriptAPI::js_Number_prototype_hex(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_OptNumber});

    duk_uint_t value;
    duk_uint_t length = 8;
    char hexString[64];

    length = duk_get_uint_default(ctx, 0, 8);

    duk_push_this(ctx);
    value = (duk_uint_t)duk_get_number(ctx, -1);
    duk_pop(ctx);

    snprintf(hexString, sizeof(hexString), "%0*X", length, value);

    duk_push_string(ctx, hexString);
    return 1;
}
