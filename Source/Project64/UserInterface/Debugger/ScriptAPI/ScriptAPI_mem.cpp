#include "stdafx.h"

#include "ScriptAPI.h"
#include <Project64/UserInterface/Debugger/DebugMMU.h>
#include <Project64/UserInterface/Debugger/debugger.h>
#include <stdio.h>
#include <string>

#pragma warning(disable : 4702) // disable unreachable code warning

using namespace ScriptAPI;

static size_t MemTypeSize(MemType t);
static duk_ret_t ThrowMemoryError(duk_context * ctx, uint32_t address);

void ScriptAPI::Define_mem(duk_context * ctx)
{
#define MEM_PROXY_FUNCS(T) js_mem__get<T>, js_mem__set<T>

    const DukPropListEntry props[] = {
        {"getblock", DukCFunction(js_mem_getblock)},
        {"setblock", DukCFunction(js_mem_setblock)},
        {"getstring", DukCFunction(js_mem_getstring)},
        {"setstring", DukCFunction(js_mem_setblock)},
        {"bindvar", DukCFunction(js_mem_bindvar)},
        {"bindvars", DukCFunction(js_mem_bindvars)},
        {"bindstruct", DukCFunction(js_mem_bindstruct)},
        {"typedef", DukCFunction(js_mem_typedef)},
        {"ramSize", DukGetter(js_mem__get_ramsize)},
        {"romSize", DukGetter(js_mem__get_romsize)},
        {"ptr", DukGetter(js_mem__get_ptr)},
        {"u32", DukProxy(MEM_PROXY_FUNCS(uint32_t))},
        {"u16", DukProxy(MEM_PROXY_FUNCS(uint16_t))},
        {"u8", DukProxy(MEM_PROXY_FUNCS(uint8_t))},
        {"s32", DukProxy(MEM_PROXY_FUNCS(int32_t))},
        {"s16", DukProxy(MEM_PROXY_FUNCS(int16_t))},
        {"s8", DukProxy(MEM_PROXY_FUNCS(int8_t))},
        {"f64", DukProxy(MEM_PROXY_FUNCS(double))},
        {"f32", DukProxy(MEM_PROXY_FUNCS(float))},
        {nullptr},
    };

    DefineGlobalInterface(ctx, "mem", props);
}

template <class T>
duk_ret_t ScriptAPI::js_mem__get(duk_context * ctx)
{
    CScriptInstance * inst = GetInstance(ctx);

    uint32_t addr = (uint32_t)duk_to_number(ctx, 1);

    T value;
    if (inst->Debugger()->DebugLoad_VAddr<T>(addr, value))
    {
        duk_push_number(ctx, value);
        return 1;
    }

    return ThrowMemoryError(ctx, addr);
}

template <class T>
duk_ret_t ScriptAPI::js_mem__set(duk_context * ctx)
{
    CScriptInstance * inst = GetInstance(ctx);

    uint32_t addr = (uint32_t)duk_to_number(ctx, 1);
    T value = (T)duk_to_number(ctx, 2);

    if (inst->Debugger()->DebugStore_VAddr<T>(addr, value))
    {
        duk_push_true(ctx);
        return 1;
    }

    return ThrowMemoryError(ctx, addr);
}

duk_ret_t ScriptAPI::js_mem_getblock(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_Number});
    CScriptInstance * inst = GetInstance(ctx);

    duk_uint_t addr = duk_to_uint(ctx, 0);
    duk_uint_t length = duk_to_uint(ctx, 1);

    uint8_t * data = (uint8_t *)duk_push_fixed_buffer(ctx, length);
    duk_push_buffer_object(ctx, -1, 0, length, DUK_BUFOBJ_NODEJS_BUFFER);

    uint32_t paddr;
    uint8_t * memsrc = nullptr;
    uint32_t offsetStart = 0;

    if (addr < 0x80000000 || addr >= 0xC0000000)
    {
        if (!g_MMU || !g_MMU->VAddrToPAddr(addr, paddr))
        {
            return ThrowMemoryError(ctx, addr);
        }
    }
    else
    {
        paddr = addr & 0x1FFFFFFF;
    }

    if (g_MMU && paddr >= 0x00000000 && (paddr + length) <= g_MMU->RdramSize())
    {
        memsrc = g_MMU->Rdram();
        offsetStart = paddr;
    }
    else if (g_Rom && paddr >= 0x10000000 && ((paddr - 0x10000000) + length) <= g_Rom->GetRomSize())
    {
        memsrc = g_Rom->GetRomAddress();
        offsetStart = paddr - 0x10000000;
    }

    if (memsrc != nullptr)
    {
        uint32_t offsetEnd = offsetStart + length;
        uint32_t alignedOffsetStart = (offsetStart + 15) & ~15;
        uint32_t alignedOffsetEnd = offsetEnd & ~15;
        uint32_t prefixLen = alignedOffsetStart - offsetStart;
        uint32_t middleLen = alignedOffsetEnd - alignedOffsetStart;
        uint32_t suffixLen = offsetEnd - alignedOffsetEnd;

        uint32_t * middleDst = (uint32_t *)&data[0 + prefixLen];
        uint32_t * middleDstEnd = (uint32_t *)&data[0 + prefixLen + middleLen];
        uint32_t * middleSrc = (uint32_t *)&memsrc[alignedOffsetStart];

        for (size_t i = 0; i < prefixLen; i++)
        {
            data[i] = memsrc[(offsetStart + i) ^ 3];
        }

        while (middleDst < middleDstEnd)
        {
            *middleDst++ = _byteswap_ulong(*middleSrc++);
            *middleDst++ = _byteswap_ulong(*middleSrc++);
            *middleDst++ = _byteswap_ulong(*middleSrc++);
            *middleDst++ = _byteswap_ulong(*middleSrc++);
        }

        for (size_t i = 0; i < suffixLen; i++)
        {
            data[(length - suffixLen) + i] = memsrc[(alignedOffsetEnd + i) ^ 3];
        }
    }
    else
    {
        for (size_t i = 0; i < length; i++)
        {
            uint8_t byte;
            if (inst->Debugger()->DebugLoad_VAddr<uint8_t>((uint32_t)((UINT_PTR)addr + i), byte))
            {
                data[i] = byte;
            }
            else
            {
                return ThrowMemoryError(ctx, (uint32_t)((UINT_PTR)addr + i));
            }
        }
    }

    return 1;
}

duk_ret_t ScriptAPI::js_mem_getstring(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_OptNumber});
    CScriptInstance * inst = GetInstance(ctx);

    duk_idx_t nargs = duk_get_top(ctx);

    duk_uint_t addr = duk_to_uint(ctx, 0);
    duk_uint_t maxLength = nargs > 1 ? duk_to_uint(ctx, 1) : 0xFFFFFFFF;
    size_t length = 0;

    for (size_t i = 0; i < maxLength; i++)
    {
        char c;
        if (!inst->Debugger()->DebugLoad_VAddr<char>((uint32_t)((UINT_PTR)addr + i), c))
        {
            return ThrowMemoryError(ctx, addr);
        }

        if (c == 0)
        {
            break;
        }

        length++;
    }

    char * str = new char[length + 1];
    str[length] = '\0';

    for (size_t i = 0; i < length; i++)
    {
        if (!inst->Debugger()->DebugLoad_VAddr<char>((uint32_t)((UINT_PTR)addr + i), str[i]))
        {
            delete[] str;
            return ThrowMemoryError(ctx, addr);
        }
    }

    duk_push_string(ctx, str);
    delete[] str;
    return 1;
}

duk_ret_t ScriptAPI::js_mem_setblock(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_Any, Arg_OptNumber});
    CScriptInstance * inst = GetInstance(ctx);
    CDebuggerUI * debugger = inst->Debugger();

    duk_idx_t nargs = duk_get_top(ctx);

    char * data;
    duk_size_t dataSize, length;

    uint32_t address = duk_get_uint(ctx, 0);

    if (duk_is_buffer_data(ctx, 1))
    {
        data = (char *)duk_get_buffer_data(ctx, 1, &dataSize);
    }
    else if (duk_is_string(ctx, 1))
    {
        data = (char *)duk_get_lstring(ctx, 1, &dataSize);
    }
    else
    {
        return ThrowInvalidArgsError(ctx);
    }

    if (nargs == 3)
    {
        duk_double_t l = duk_get_number(ctx, 2);

        if (l < 0 || l > dataSize)
        {
            return DUK_RET_RANGE_ERROR;
        }

        length = (duk_size_t)l;
    }
    else
    {
        length = dataSize;
    }

    for (size_t i = 0; i < length; i++)
    {
        if (!debugger->DebugStore_VAddr((uint32_t)((UINT_PTR)address + i), data[i]))
        {
            return ThrowMemoryError(ctx, (uint32_t)((UINT_PTR)address + i));
        }
    }

    return 0;
}

duk_ret_t ScriptAPI::js_mem__boundget(duk_context * ctx)
{
    CDebuggerUI * debugger = GetInstance(ctx)->Debugger();

    uint32_t addr = duk_get_uint(ctx, 0);
    duk_int_t type = duk_get_int(ctx, 1);

    union
    {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        int8_t s8;
        int16_t s16;
        int32_t s32;
        float f32;
        double f64;
    } retval;

#define MEM_BOUNDGET_TRY(addr, T, result, dukpush)  \
    if (debugger->DebugLoad_VAddr<T>(addr, result)) \
    {                                               \
        dukpush(ctx, result);                       \
    }                                               \
    else                                            \
    {                                               \
        goto memory_error;                          \
    }

    switch (type)
    {
    case U8:
        MEM_BOUNDGET_TRY(addr, uint8_t, retval.u8, duk_push_uint);
        return 1;
    case U16:
        MEM_BOUNDGET_TRY(addr, uint16_t, retval.u16, duk_push_uint);
        return 1;
    case U32:
        MEM_BOUNDGET_TRY(addr, uint32_t, retval.u32, duk_push_uint);
        return 1;
    case S8:
        MEM_BOUNDGET_TRY(addr, int8_t, retval.s8, duk_push_int);
        return 1;
    case S16:
        MEM_BOUNDGET_TRY(addr, int16_t, retval.s16, duk_push_int);
        return 1;
    case S32:
        MEM_BOUNDGET_TRY(addr, int32_t, retval.s32, duk_push_int);
        return 1;
    case F32:
        MEM_BOUNDGET_TRY(addr, float, retval.f32, duk_push_number);
        return 1;
    case F64:
        MEM_BOUNDGET_TRY(addr, double, retval.f64, duk_push_number);
        return 1;
    }

memory_error:
    return ThrowMemoryError(ctx, addr);
}

duk_ret_t ScriptAPI::js_mem__boundset(duk_context * ctx)
{
    CDebuggerUI * debugger = GetInstance(ctx)->Debugger();

    uint32_t addr = duk_get_uint(ctx, 0);
    duk_int_t type = duk_get_int(ctx, 1);

#define MEM_BOUNDSET_TRY(addr, T, value)            \
    if (debugger->DebugStore_VAddr<T>(addr, value)) \
    {                                               \
        return 1;                                   \
    }                                               \
    else                                            \
    {                                               \
        goto memory_error;                          \
    }

    switch (type)
    {
    case U8:
        MEM_BOUNDSET_TRY(addr, uint8_t, duk_get_uint(ctx, 2) & 0xFF);
        break;
    case U16:
        MEM_BOUNDSET_TRY(addr, uint16_t, duk_get_uint(ctx, 2) & 0xFFFF);
        break;
    case U32:
        MEM_BOUNDSET_TRY(addr, uint32_t, duk_get_uint(ctx, 2));
        break;
    case S8:
        MEM_BOUNDSET_TRY(addr, int8_t, duk_get_int(ctx, 2) & 0xFF);
        break;
    case S16:
        MEM_BOUNDSET_TRY(addr, int16_t, duk_get_int(ctx, 2) & 0xFFFF);
        break;
    case S32:
        MEM_BOUNDSET_TRY(addr, int32_t, duk_get_int(ctx, 2));
        break;
    case F32:
        MEM_BOUNDSET_TRY(addr, float, (float)duk_get_number(ctx, 2));
        break;
    case F64:
        MEM_BOUNDSET_TRY(addr, double, duk_get_number(ctx, 2));
        break;
    }

    return 0;

memory_error:
    return ThrowMemoryError(ctx, addr);
}

duk_ret_t ScriptAPI::js_mem_bindvar(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Object, Arg_Number, Arg_String, Arg_Number});

    duk_uint_t addr = duk_get_uint(ctx, 1);
    const char * key = duk_get_string(ctx, 2);
    duk_int_t type = duk_get_int(ctx, 3);

    duk_push_string(ctx, key);

    // [ ... js_mem__boundget ] -> [ ... js_mem__boundget.bind(obj, addr, type) ]
    duk_push_c_function(ctx, js_mem__boundget, DUK_VARARGS);
    duk_push_string(ctx, "bind");
    duk_dup(ctx, 0);
    duk_push_uint(ctx, addr);
    duk_push_int(ctx, type);
    duk_call_prop(ctx, -5, 3);
    duk_remove(ctx, -2);

    duk_push_c_function(ctx, js_mem__boundset, DUK_VARARGS);
    duk_push_string(ctx, "bind");
    duk_dup(ctx, 0);
    duk_push_uint(ctx, addr);
    duk_push_int(ctx, type);
    duk_call_prop(ctx, -5, 3);
    duk_remove(ctx, -2);

    duk_def_prop(ctx, 0, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

    return 0;
}

duk_ret_t ScriptAPI::js_mem_bindvars(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Object, Arg_Array});

    duk_size_t length = duk_get_length(ctx, 1);

    for (duk_uarridx_t i = 0; i < length; i++)
    {
        duk_get_prop_index(ctx, 1, i);
        if (!duk_is_array(ctx, -1) || duk_get_length(ctx, -1) != 3)
        {
            return DUK_RET_TYPE_ERROR;
        }

        duk_push_c_function(ctx, js_mem_bindvar, 4);
        duk_dup(ctx, 0);
        duk_get_prop_index(ctx, -3, 0);
        duk_get_prop_index(ctx, -4, 1);
        duk_get_prop_index(ctx, -5, 2);

        if (duk_pcall(ctx, 4) != DUK_EXEC_SUCCESS)
        {
            return duk_throw(ctx);
        }

        duk_pop_n(ctx, 1);
    }

    duk_dup(ctx, 0);
    return 1;
}

duk_ret_t ScriptAPI::js_mem_bindstruct(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Object, Arg_Number, Arg_Object});

    uint32_t curAddr = duk_get_uint(ctx, 1);

    duk_enum(ctx, 2, DUK_ENUM_OWN_PROPERTIES_ONLY);

    while (duk_next(ctx, -1, 1))
    {
        MemType type = (MemType)duk_get_int(ctx, -1);

        duk_push_c_function(ctx, js_mem_bindvar, 4);
        duk_dup(ctx, 0);
        duk_push_uint(ctx, curAddr);
        duk_pull(ctx, -5);
        duk_pull(ctx, -5);

        if (duk_pcall(ctx, 4) != DUK_EXEC_SUCCESS)
        {
            return duk_throw(ctx);
        }

        duk_pop(ctx);

        curAddr += (uint32_t)((UINT_PTR)MemTypeSize(type));
    }

    duk_dup(ctx, 0);

    return 1;
}

duk_ret_t ScriptAPI::js_mem__type_constructor(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Object, Arg_Number});

    duk_push_c_function(ctx, js_mem_bindstruct, 3);
    duk_push_this(ctx);
    duk_pull(ctx, 1);
    duk_pull(ctx, 0);

    if (duk_pcall(ctx, 3) != DUK_EXEC_SUCCESS)
    {
        return duk_throw(ctx);
    }

    return 0;
}

duk_ret_t ScriptAPI::js_mem_typedef(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Object});

    duk_push_c_function(ctx, js_mem__type_constructor, DUK_VARARGS);
    duk_push_string(ctx, "bind");
    duk_push_null(ctx);
    duk_dup(ctx, 0);
    duk_call_prop(ctx, -4, 2);
    return 1;
}

duk_ret_t ScriptAPI::js_mem__get_ramsize(duk_context * ctx)
{
    duk_push_number(ctx, g_MMU ? g_MMU->RdramSize() : 0);
    return 1;
}

duk_ret_t ScriptAPI::js_mem__get_romsize(duk_context * ctx)
{
    duk_push_number(ctx, g_Rom ? g_Rom->GetRomSize() : 0);
    return 1;
}

duk_ret_t ScriptAPI::js_mem__get_ptr(duk_context * ctx)
{
    duk_push_pointer(ctx, g_MMU ? g_MMU->Rdram() : nullptr);
    return 1;
}

size_t MemTypeSize(MemType t)
{
    switch (t)
    {
    case U8:
    case S8:
        return 1;
    case U16:
    case S16:
        return 2;
    case U32:
    case S32:
    case F32:
        return 4;
    case F64:
        return 8;
    }
    return 0;
}

duk_ret_t ThrowMemoryError(duk_context * ctx, uint32_t address)
{
    duk_push_error_object(ctx, DUK_ERR_ERROR, "memory error (can't access 0x%08X)", address);
    return duk_throw(ctx);
}
