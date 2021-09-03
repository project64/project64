#include <stdafx.h>
#include "ScriptAPI.h"

void ScriptAPI::Define_AddressRange(duk_context* ctx)
{
    DefineGlobalClass(ctx, "AddressRange", js_AddressRange__constructor);

    struct { const char* key; uint32_t start, end; } ranges[] = {
        { "ADDR_ANY",              0x00000000, 0xFFFFFFFF },
        { "ADDR_ANY_KUSEG",        0x00000000, 0x7FFFFFFF },
        { "ADDR_ANY_KSEG0",        0x80000000, 0x9FFFFFFF },
        { "ADDR_ANY_KSEG1",        0xA0000000, 0xBFFFFFFF },
        { "ADDR_ANY_KSEG2",        0xC0000000, 0xFFFFFFFF },
        { "ADDR_ANY_RDRAM",        0x80000000, 0x807FFFFF },
        { "ADDR_ANY_RDRAM_UNC",    0xA0000000, 0xA07FFFFF },
        { "ADDR_ANY_CART_ROM",     0x90000000, 0x95FFFFFF },
        { "ADDR_ANY_CART_ROM_UNC", 0xB0000000, 0xB5FFFFFF },
        { nullptr, 0, 0 }
    };

    duk_push_global_object(ctx);
    duk_push_c_function(ctx, js_AddressRange__constructor, DUK_VARARGS);
    for(int i = 0; ranges[i].key != nullptr; i++)
    {
        duk_push_string(ctx, ranges[i].key);
        duk_dup(ctx, -2);
        duk_push_uint(ctx, ranges[i].start);
        duk_push_uint(ctx, ranges[i].end);
        duk_new(ctx, 2);
        duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
    }

    duk_pop_n(ctx, 2);
}

duk_ret_t ScriptAPI::js_AddressRange__constructor(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Number, Arg_Number });

    duk_to_uint32(ctx, 0);
    duk_to_uint32(ctx, 1);

    duk_push_this(ctx);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "start");
    duk_dup(ctx, 1);
    duk_put_prop_string(ctx, -2, "end");
    duk_freeze(ctx, -1);
    duk_pop(ctx);

    return 0;
}
