#include "stdafx.h"

#include "ScriptAPI.h"

#pragma warning(disable : 4702) // disable unreachable code warning

static void GetRange(duk_context * ctx, duk_idx_t idx, uint32_t * start, uint32_t * end);

void ScriptAPI::Define_AddressRange(duk_context * ctx)
{
    const DukPropListEntry prototype[] = {
        {"size", DukCFunction(js_AddressRange_size)},
        {"includes", DukCFunction(js_AddressRange_includes)},
        {"offset", DukCFunction(js_AddressRange_offset)},
        {"address", DukCFunction(js_AddressRange_address)},
        {nullptr},
    };

    DefineGlobalClass(ctx, "AddressRange", js_AddressRange__constructor, prototype);

    struct
    {
        const char * key;
        uint32_t start, end;
    } ranges[] = {
        {"ADDR_ANY", 0x00000000, 0xFFFFFFFF},
        {"ADDR_ANY_KUSEG", 0x00000000, 0x7FFFFFFF},
        {"ADDR_ANY_KSEG0", 0x80000000, 0x9FFFFFFF},
        {"ADDR_ANY_KSEG1", 0xA0000000, 0xBFFFFFFF},
        {"ADDR_ANY_KSEG2", 0xC0000000, 0xFFFFFFFF},
        {"ADDR_ANY_RDRAM", 0x80000000, 0x807FFFFF},
        {"ADDR_ANY_RDRAM_UNC", 0xA0000000, 0xA07FFFFF},
        {"ADDR_ANY_CART_ROM", 0x90000000, 0x95FFFFFF},
        {"ADDR_ANY_CART_ROM_UNC", 0xB0000000, 0xB5FFFFFF},
        {nullptr, 0, 0},
    };

    duk_push_global_object(ctx);

    for (int i = 0; ranges[i].key != nullptr; i++)
    {
        duk_push_string(ctx, ranges[i].key);
        duk_get_global_string(ctx, "AddressRange");
        duk_push_uint(ctx, ranges[i].start);
        duk_push_uint(ctx, ranges[i].end);
        duk_new(ctx, 2);
        duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
    }

    duk_pop(ctx);
}

duk_ret_t ScriptAPI::js_AddressRange__constructor(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_Number});

    duk_to_uint32(ctx, 0);
    duk_to_uint32(ctx, 1);

    duk_uint_t start = duk_get_uint(ctx, 0);
    duk_uint_t end = duk_get_uint(ctx, 1);

    if (end < start)
    {
        duk_push_error_object(ctx, DUK_ERR_RANGE_ERROR, "invalid range; end cannot be less than start");
        return duk_throw(ctx);
    }

    duk_push_this(ctx);
    duk_push_uint(ctx, start);
    duk_put_prop_string(ctx, -2, "start");
    duk_push_uint(ctx, end);
    duk_put_prop_string(ctx, -2, "end");
    duk_freeze(ctx, -1);
    duk_pop(ctx);

    return 0;
}

duk_ret_t ScriptAPI::js_AddressRange_size(duk_context * ctx)
{
    CheckArgs(ctx, {});
    duk_uint_t start, end;

    duk_push_this(ctx);
    GetRange(ctx, -1, &start, &end);

    duk_push_uint(ctx, end - start + 1);
    return 1;
}

duk_ret_t ScriptAPI::js_AddressRange_includes(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});
    duk_to_uint32(ctx, 0);
    duk_uint_t address = duk_get_uint(ctx, 0);
    duk_uint_t start, end;

    duk_push_this(ctx);
    GetRange(ctx, -1, &start, &end);

    duk_push_boolean(ctx, static_cast<duk_bool_t>(address >= start && address <= end));
    return 1;
}

duk_ret_t ScriptAPI::js_AddressRange_offset(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});
    duk_to_uint32(ctx, 0);
    duk_uint_t address = duk_get_uint(ctx, 0);
    duk_uint_t start, end;

    duk_push_this(ctx);
    GetRange(ctx, -1, &start, &end);

    if (address < start || address > end)
    {
        duk_push_error_object(ctx, DUK_ERR_RANGE_ERROR, "address out of bounds");
        return duk_throw(ctx);
    }

    duk_push_uint(ctx, address - start);
    return 1;
}

duk_ret_t ScriptAPI::js_AddressRange_address(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});

    duk_int_t offset = duk_get_int(ctx, 0);
    duk_uint_t start, end;

    duk_push_this(ctx);
    GetRange(ctx, -1, &start, &end);

    duk_uint_t address = start + offset;

    if (address < start || address > end)
    {
        duk_push_error_object(ctx, DUK_ERR_RANGE_ERROR, "offset out of bounds");
        return duk_throw(ctx);
    }

    duk_push_uint(ctx, address);
    return 1;
}

static void GetRange(duk_context * ctx, duk_idx_t idx, uint32_t * start, uint32_t * end)
{
    idx = duk_normalize_index(ctx, idx);
    duk_get_prop_string(ctx, idx, "start");
    *start = duk_get_uint(ctx, -1);
    duk_get_prop_string(ctx, idx, "end");
    *end = duk_get_uint(ctx, -1);
    duk_pop_n(ctx, 2);
}
