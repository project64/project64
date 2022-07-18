#include <stdafx.h>
#include "ScriptAPI.h"
#include "../Assembler.h"
#include <Project64-core/N64System/Mips/R4300iInstruction.h>

void ScriptAPI::Define_asm(duk_context *ctx)
{
    const DukPropListEntry props[] = {
        { "gprname", DukCFunction(js_asm_gprname) },
        { "encode", DukCFunction(js_asm_encode) },
        { "decode", DukCFunction(js_asm_decode) },
        { nullptr }
    };

    DefineGlobalInterface(ctx, "asm", props);
}

duk_ret_t ScriptAPI::js_asm_gprname(duk_context* ctx)
{
    const char* names[32] = {
        "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };

    CheckArgs(ctx, { Arg_Number });

    duk_uint_t idx = duk_get_uint(ctx, 0);

    if (idx < 32)
    {
        duk_push_string(ctx, names[idx]);
    }
    else
    {
        return DUK_RET_RANGE_ERROR;
    }

    return 1;
}

duk_ret_t ScriptAPI::js_asm_encode(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_String, Arg_OptNumber });

    const char *code = duk_get_string(ctx, 0);
    uint32_t address = duk_get_uint_default(ctx, 1, 0);

    // TODO: CAssembler's state is not thread safe.
    // CAssembler should be object-oriented

    uint32_t opcode;
    if (!CAssembler::AssembleLine(code, &opcode, address))
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "ASM syntax error");
        duk_throw(ctx);
    }

    duk_push_uint(ctx, opcode);
    return 1;
}

duk_ret_t ScriptAPI::js_asm_decode(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Number, Arg_OptNumber });

    uint32_t opcode = duk_get_uint(ctx, 0);
    uint32_t address = duk_get_uint_default(ctx, 1, 0);
    duk_push_string(ctx, R4300iInstruction(address, opcode).NameAndParam().c_str());
    return 1;
}
