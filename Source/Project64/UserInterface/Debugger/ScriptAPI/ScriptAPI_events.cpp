#include <stdafx.h>
#include "ScriptAPI.h"
#include "../OpInfo.h"

#pragma warning(disable: 4702) // disable unreachable code warning

using namespace ScriptAPI;

static bool CbCond_PcBetween(JSAppCallback* cb, void* env);
static bool CbCond_ReadAddrBetween(JSAppCallback* cb, void* env);
static bool CbCond_WriteAddrBetween(JSAppCallback* cb, void* env);
static bool CbCond_PcBetween_OpcodeEquals(JSAppCallback* cb, void* env);
static bool CbCond_PcBetween_GprValueEquals(JSAppCallback* cb, void* env);

static duk_idx_t CbArgs_GenericEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_EmuStateChangeEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_ExecEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_ReadEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_WriteEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_OpcodeEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_RegValueEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_MouseEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_SPTaskEventObject(duk_context* ctx, void* env);
static duk_idx_t CbArgs_PIEventObject(duk_context* ctx, void* env);

static duk_ret_t RequireAddressOrAddressRange(duk_context* ctx, duk_idx_t idx, uint32_t* addrStart, uint32_t *addrEnd);
static duk_ret_t RequireInterpreterCPU(duk_context* ctx);

void ScriptAPI::Define_events(duk_context* ctx)
{
    const DukPropListEntry props[] = {
        { "onstatechange", DukCFunction(js_events_onstatechange) },
        { "onexec",        DukCFunction(js_events_onexec) },
        { "onread",        DukCFunction(js_events_onread) },
        { "onwrite",       DukCFunction(js_events_onwrite) },
        { "ongprvalue",    DukCFunction(js_events_ongprvalue) },
        { "onopcode",      DukCFunction(js_events_onopcode) },
        { "onpifread",     DukCFunction(js_events_onpifread) },
        { "onsptask",      DukCFunction(js_events_onsptask) },
        { "onpidma",       DukCFunction(js_events_onpidma) },
        { "onmouseup",     DukCFunction(js_events_onmouseup) },
        { "onmousedown",   DukCFunction(js_events_onmousedown) },
        { "onmousemove",   DukCFunction(js_events_onmousemove) },
        { "remove",        DukCFunction(js_events_remove) },
        { nullptr }
    };

    DefineGlobalInterface(ctx, "events", props);

    DefineGlobalClass(ctx, "GenericEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "EmuStateChangeEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "CPUExecEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "CPUReadWriteEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "CPUOpcodeEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "CPURegValueEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "SPTaskEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "PIEvent", js_DummyConstructor);
    DefineGlobalClass(ctx, "DrawEvent", js_DummyConstructor);

    const DukPropListEntry mouseEventStaticProps[] = {
        { "NONE",   DukNumber(-1) },
        { "LEFT",   DukNumber(0) },
        { "MIDDLE", DukNumber(1) },
        { "RIGHT",  DukNumber(2) },
        {nullptr}
    };

    DefineGlobalClass(ctx, "MouseEvent", js_DummyConstructor, nullptr, mouseEventStaticProps);
}

duk_ret_t ScriptAPI::js_events_onstatechange(duk_context * ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_EMUSTATECHANGE,
        CbArgs_EmuStateChangeEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onexec(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Any, Arg_Function });

    uint32_t addrStart, addrEnd;
    RequireAddressOrAddressRange(ctx, 0, &addrStart, &addrEnd);

    RequireInterpreterCPU(ctx);

    JSAppCallback cb(GetInstance(ctx), duk_get_heapptr(ctx, 1),
        CbCond_PcBetween, CbArgs_ExecEventObject);
    cb.m_Params.addrStart = addrStart;
    cb.m_Params.addrEnd = addrEnd;

    JSAppCallbackID callbackId = AddAppCallback(ctx, JS_HOOK_CPU_EXEC, cb);

    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onread(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Any, Arg_Function });

    uint32_t addrStart, addrEnd;
    RequireAddressOrAddressRange(ctx, 0, &addrStart, &addrEnd);

    RequireInterpreterCPU(ctx);

    JSAppCallback cb(GetInstance(ctx), duk_get_heapptr(ctx, 1),
        CbCond_ReadAddrBetween, CbArgs_ReadEventObject);
    cb.m_Params.addrStart = addrStart;
    cb.m_Params.addrEnd = addrEnd;

    JSAppCallbackID callbackId = AddAppCallback(ctx, JS_HOOK_CPU_READ, cb);

    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onwrite(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Any, Arg_Function });

    uint32_t addrStart, addrEnd;
    RequireAddressOrAddressRange(ctx, 0, &addrStart, &addrEnd);

    RequireInterpreterCPU(ctx);

    JSAppCallback cb(GetInstance(ctx), duk_get_heapptr(ctx, 1),
        CbCond_WriteAddrBetween, CbArgs_WriteEventObject);
    cb.m_Params.addrStart = addrStart;
    cb.m_Params.addrEnd = addrEnd;

    JSAppCallbackID callbackId = AddAppCallback(ctx, JS_HOOK_CPU_WRITE, cb);

    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onopcode(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Any, Arg_Number, Arg_Number, Arg_Function });

    uint32_t addrStart, addrEnd;
    RequireAddressOrAddressRange(ctx, 0, &addrStart, &addrEnd);

    RequireInterpreterCPU(ctx);

    uint32_t opcode = duk_get_uint(ctx, 1);
    uint32_t mask = duk_get_uint(ctx, 2);

    JSAppCallback cb(GetInstance(ctx), duk_get_heapptr(ctx, 3),
        CbCond_PcBetween_OpcodeEquals, CbArgs_OpcodeEventObject);
    cb.m_Params.addrStart = addrStart;
    cb.m_Params.addrEnd = addrEnd;
    cb.m_Params.opcode = opcode;
    cb.m_Params.opcodeMask = mask;

    JSAppCallbackID callbackId = AddAppCallback(ctx, JS_HOOK_CPU_EXEC, cb);

    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_ongprvalue(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Any, Arg_Number, Arg_Number, Arg_Function });

    uint32_t addrStart, addrEnd;
    RequireAddressOrAddressRange(ctx, 0, &addrStart, &addrEnd);

    RequireInterpreterCPU(ctx);

    JSAppCallback cb(GetInstance(ctx), duk_get_heapptr(ctx, 3),
        CbCond_PcBetween_GprValueEquals, CbArgs_RegValueEventObject);
    cb.m_Params.addrStart = addrStart;
    cb.m_Params.addrEnd = addrEnd;
    cb.m_Params.regIndices = duk_get_uint(ctx, 1);
    cb.m_Params.regValue = duk_get_uint(ctx, 2);

    JSAppCallbackID callbackId = AddAppCallback(ctx, JS_HOOK_CPU_EXEC, cb);

    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onpifread(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_PIFREAD, CbArgs_GenericEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onsptask(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_RSPTASK, CbArgs_SPTaskEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onpidma(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_PIDMA, CbArgs_PIEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onmouseup(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_MOUSEUP, CbArgs_MouseEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onmousedown(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_MOUSEDOWN, CbArgs_MouseEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_onmousemove(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Function });

    JSAppCallbackID callbackId = AddAppCallback(ctx, 0, JS_HOOK_MOUSEMOVE, CbArgs_MouseEventObject);
    duk_push_uint(ctx, callbackId);
    return 1;
}

duk_ret_t ScriptAPI::js_events_remove(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Number });

    JSAppCallbackID callbackId = (JSAppCallbackID)duk_get_uint(ctx, 0);

    if (!RemoveAppCallback(ctx, callbackId))
    {
        duk_push_error_object(ctx, DUK_ERR_REFERENCE_ERROR, "invalid callback ID");
        return duk_throw(ctx);
    }

    return 0;
}

bool CbCond_ReadAddrBetween(JSAppCallback* cb, void* _env)
{
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;

    if (!env->opInfo.IsLoadCommand())
    {
        return false;
    }

    uint32_t addr = env->opInfo.GetLoadStoreAddress();

    return (addr >= cb->m_Params.addrStart &&
            addr <= cb->m_Params.addrEnd);
}

bool CbCond_WriteAddrBetween(JSAppCallback* cb, void* _env)
{
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;

    if (!env->opInfo.IsStoreCommand())
    {
        return false;
    }

    uint32_t addr = env->opInfo.GetLoadStoreAddress();

    return (addr >= cb->m_Params.addrStart &&
            addr <= cb->m_Params.addrEnd);
}

bool CbCond_PcBetween(JSAppCallback* cb, void* _env)
{
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;
    return (env->pc >= cb->m_Params.addrStart &&
            env->pc <= cb->m_Params.addrEnd);
}

bool CbCond_PcBetween_OpcodeEquals(JSAppCallback* cb, void* _env)
{
    if (!CbCond_PcBetween(cb, _env))
    {
        return false;
    }

    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;
    return cb->m_Params.opcode == (env->opInfo.m_OpCode.Value & cb->m_Params.opcodeMask);
}

static bool CbCond_PcBetween_GprValueEquals(JSAppCallback* cb, void* _env)
{
    if (!CbCond_PcBetween(cb, _env))
    {
        return false;
    }

    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;

    for(int i = 0; i < 32; i++)
    {
        if(cb->m_Params.regIndices & (1 << i))
        {
            if(g_Reg->m_GPR[i].UW[0] == cb->m_Params.regValue)
            {
                env->outAffectedRegIndex = i;
                return true;
            }
        }
    }

    return false;
}

duk_idx_t CbArgs_EmuStateChangeEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookEmuStateChangeEnv* env = (JSHookEmuStateChangeEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "EmuStateChangeEvent");

    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "state",      DukUInt(env->state) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

duk_idx_t CbArgs_GenericEventObject(duk_context* ctx, void* /*_env*/)
{
    CScriptInstance* inst = GetInstance(ctx);
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "GenericEvent");
    
    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

duk_idx_t CbArgs_ExecEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "CPUExecEvent");
    
    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "pc", DukUInt(env->pc) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

duk_idx_t CbArgs_ReadEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    CDebuggerUI* debugger = inst->Debugger();
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;
    
    uint32_t address = env->opInfo.GetLoadStoreAddress();

    uint8_t op = env->opInfo.m_OpCode.op;
    uint8_t rt = env->opInfo.m_OpCode.rt;
    bool bFPU = (op == R4300i_LWC1 || op == R4300i_LDC1);

    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "CPUReadWriteEvent");

    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "pc", DukUInt(env->pc) },
        { "address", DukUInt(address) },
        { "reg", DukUInt(rt) },
        { "fpu", DukBoolean(bFPU) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);

    union {
        uint8_t u8;
        int8_t s8;
        uint16_t u16;
        int16_t s16;
        uint32_t u32;
        int32_t s32;
        float f32;
        double f64;
        uint64_t u64;
    } value = {0};
    
    bool bNeedUpper32 = false;
    
    switch (env->opInfo.m_OpCode.op)
    {
    case R4300i_LB:
        debugger->DebugLoad_VAddr(address, value.s8);
        duk_push_int(ctx, value.s8);
        duk_push_int(ctx, S8);
        break;
    case R4300i_LBU:
        debugger->DebugLoad_VAddr(address, value.u8);
        duk_push_uint(ctx, value.u8);
        duk_push_int(ctx, U8);
        break;
    case R4300i_LH:
        debugger->DebugLoad_VAddr(address, value.s16);
        duk_push_int(ctx, value.s16);
        duk_push_int(ctx, S16);
        break;
    case R4300i_LHU:
        debugger->DebugLoad_VAddr(address, value.u16);
        duk_push_uint(ctx, value.u16);
        duk_push_int(ctx, U16);
        break;
    case R4300i_LL:
    case R4300i_LW:
        debugger->DebugLoad_VAddr(address, value.s32);
        duk_push_int(ctx, value.s32);
        duk_push_int(ctx, S32);
        break;
    case R4300i_LWU:
        debugger->DebugLoad_VAddr(address, value.u32);
        duk_push_uint(ctx, value.u32);
        duk_push_int(ctx, U32);
        break;
    case R4300i_LWC1:
        debugger->DebugLoad_VAddr(address, value.f32);
        duk_push_number(ctx, value.f32);
        duk_push_int(ctx, F32);
        break;
    case R4300i_LDC1:
        debugger->DebugLoad_VAddr(address, value.f64);
        duk_push_number(ctx, value.f64);
        duk_push_int(ctx, F64);
        break;
    case R4300i_LD:
        debugger->DebugLoad_VAddr(address, value.u64);
        duk_push_number(ctx, (duk_double_t)(value.u64 & 0xFFFFFFFF));
        duk_push_int(ctx, U64);
        bNeedUpper32 = true;
        break;
    case R4300i_LDL:
        {
        int shift = (address & 7) * 8;
        uint64_t mask = ~(((uint64_t)-1) << shift);
        debugger->DebugLoad_VAddr(address & ~7, value.u64);
        value.u64 = (g_Reg->m_GPR[rt].DW & mask) + (value.u64 << shift);
        duk_push_number(ctx, (duk_double_t)(value.u64 & 0xFFFFFFFF));
        duk_push_int(ctx, U64);
        bNeedUpper32 = true;
        }
        break;
    case R4300i_LDR:
        {
        int shift = 56 - ((address & 7) * 8);
        uint64_t mask = ~(((uint64_t)-1) >> shift);
        debugger->DebugLoad_VAddr(address & ~7, value.u64);
        value.u64 = (g_Reg->m_GPR[rt].DW & mask) + (value.u64 >> shift);
        duk_push_number(ctx, (duk_double_t)(value.u64 & 0xFFFFFFFF));
        duk_push_int(ctx, U64);
        bNeedUpper32 = true;
        }
        break;
    case R4300i_LWL:
        {
        int shift = (address & 3) * 8;
        uint32_t mask = ~(((uint32_t)-1) << shift);
        debugger->DebugLoad_VAddr(address & ~3, value.s32);
        value.s32 = (g_Reg->m_GPR[rt].W[0] & mask) + (value.s32 << shift);
        duk_push_number(ctx, value.s32);
        duk_push_int(ctx, S32);
        }
        break;
    case R4300i_LWR:
        {
        int shift = 24 - ((address & 3) * 8);
        uint32_t mask = ~(((uint32_t)-1) >> shift);
        debugger->DebugLoad_VAddr(address & ~3, value.s32);
        value.s32 = (g_Reg->m_GPR[rt].W[0] & mask) + (value.s32 >> shift);
        duk_push_number(ctx, value.s32);
        duk_push_int(ctx, S32);
        }
        break;
    default:
        duk_push_number(ctx, 0);
        duk_push_number(ctx, 0);
        break;
    }

    duk_put_prop_string(ctx, -3, "valueType");
    duk_put_prop_string(ctx, -2, "value");

    if (bNeedUpper32)
    {
        duk_push_number(ctx, (duk_double_t)(value.u64 >> 32));
        duk_put_prop_string(ctx, -2, "valueHi");
    }

    duk_freeze(ctx, -1);
    return 1;
}

duk_idx_t CbArgs_WriteEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    CDebuggerUI* debugger = inst->Debugger();
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;

    uint32_t address = env->opInfo.GetLoadStoreAddress();

    uint8_t op = env->opInfo.m_OpCode.op;
    uint8_t rt = env->opInfo.m_OpCode.rt;
    bool bFPU = (op == R4300i_SWC1 || op == R4300i_SDC1);

    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "CPUReadWriteEvent");

    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "pc", DukUInt(env->pc) },
        { "address", DukUInt(address) },
        { "reg", DukUInt(rt) },
        { "fpu", DukBoolean(bFPU) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);

    bool bNeedUpper32 = false;
    uint64_t value64 = 0;

    switch (env->opInfo.m_OpCode.op)
    {
    case R4300i_SB:
        duk_push_int(ctx, g_Reg->m_GPR[rt].B[0]);
        duk_push_int(ctx, S8);
        break;
    case R4300i_SH:
        duk_push_int(ctx, g_Reg->m_GPR[rt].HW[0]);
        duk_push_int(ctx, S16);
        break;
    case R4300i_SW:
        duk_push_int(ctx, g_Reg->m_GPR[rt].W[0]);
        duk_push_int(ctx, S32);
        break;
    case R4300i_SWC1:
        duk_push_number(ctx, *g_Reg->m_FPR_S[rt]);
        duk_push_int(ctx, F32);
        break;
    case R4300i_SDC1:
        duk_push_number(ctx, *g_Reg->m_FPR_D[rt]);
        duk_push_int(ctx, F64);
        break;
    case R4300i_SD:
        duk_push_number(ctx, g_Reg->m_GPR[rt].UW[0]);
        duk_push_int(ctx, U64);
        bNeedUpper32 = true;
        break;
    case R4300i_SWL:
        {
        int shift = (address & 3) * 8;
        uint32_t mask = ~(((uint32_t)-1) >> shift);
        uint32_t value;
        debugger->DebugLoad_VAddr(address & ~3, value);
        value = (value & mask) + (g_Reg->m_GPR[rt].UW[0] >> shift);
        duk_push_number(ctx, value);
        duk_push_int(ctx, S32);
        }
        break;
    case R4300i_SWR:
        {
        int shift = 24 - ((address & 3) * 8);
        uint32_t mask = ~(((uint32_t)-1) << shift);
        uint32_t value;
        debugger->DebugLoad_VAddr(address & ~3, value);
        value = (value & mask) + (g_Reg->m_GPR[rt].UW[0] >> shift);
        duk_push_number(ctx, value);
        duk_push_int(ctx, S32);
        }
        break;
    case R4300i_SDL:
        {
        int shift = (address & 7) * 8;
        uint64_t mask = ~(((uint64_t)-1) >> shift);
        debugger->DebugLoad_VAddr(address & ~7, value64);
        value64 = (value64 & mask) + (g_Reg->m_GPR[rt].UDW >> shift);
        duk_push_number(ctx, (duk_double_t)(value64 & 0xFFFFFFFF));
        duk_push_int(ctx, U64);
        }
    case R4300i_SDR:
    {
        int shift = 56 - ((address & 7) * 8);
        uint64_t mask = ~(((uint64_t)-1) << shift);
        debugger->DebugLoad_VAddr(address & ~7, value64);
        value64 = (value64 & mask) + (g_Reg->m_GPR[rt].UDW >> shift);
        duk_push_number(ctx, (duk_double_t)(value64 & 0xFFFFFFFF));
        duk_push_int(ctx, U64);
    }
    default:
        duk_push_number(ctx, 0);
        duk_push_number(ctx, 0);
        break;
    }

    duk_put_prop_string(ctx, -3, "valueType");
    duk_put_prop_string(ctx, -2, "value");

    if (bNeedUpper32)
    {
        duk_push_number(ctx, (duk_double_t)(value64 >> 32));
        duk_put_prop_string(ctx, -2, "valueHi");
    }

    duk_freeze(ctx, -1);
    
    return 1;
}

duk_idx_t CbArgs_OpcodeEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "CPUOpcodeEvent");

    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "pc", DukUInt(env->pc) },
        { "opcode", DukUInt(env->opInfo.m_OpCode.Value) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

duk_idx_t CbArgs_RegValueEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookCpuStepEnv* env = (JSHookCpuStepEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "CPURegValueEvent");

    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "pc", DukUInt(env->pc) },
        { "value", DukUInt(g_Reg->m_GPR[env->outAffectedRegIndex].UW[0]) },
        { "reg", DukUInt(env->outAffectedRegIndex) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

static duk_idx_t CbArgs_MouseEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookMouseEnv* env = (JSHookMouseEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "MouseEvent");

    const DukPropListEntry props[] = {
        { "callbackId", DukUInt(inst->CallbackId()) },
        { "button", DukInt(env->button) },
        { "x", DukInt(env->x) },
        { "y", DukInt(env->y) },
        { nullptr }
    };
    
    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

static duk_idx_t CbArgs_SPTaskEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookSpTaskEnv* env = (JSHookSpTaskEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "SPTaskEvent");

    const DukPropListEntry props[] = {
        { "callbackId",        DukUInt(inst->CallbackId()) },
        { "taskType",          DukUInt(env->taskType) },
        { "taskFlags",         DukUInt(env->taskFlags) },
        { "ucodeBootAddress",  DukUInt(env->ucodeBootAddress | 0x80000000) },
        { "ucodeBootSize",     DukUInt(env->ucodeBootSize) },
        { "ucodeAddress",      DukUInt(env->ucodeAddress | 0x80000000) },
        { "ucodeSize",         DukUInt(env->ucodeSize) },
        { "ucodeDataAddress",  DukUInt(env->ucodeDataAddress | 0x80000000) },
        { "ucodeDataSize",     DukUInt(env->ucodeDataSize) },
        { "dramStackAddress",  DukUInt(env->dramStackAddress | 0x80000000) },
        { "dramStackSize",     DukUInt(env->dramStackSize) },
        { "outputBuffAddress", DukUInt(env->outputBuffAddress | 0x80000000) },
        { "outputBuffSize",    DukUInt(env->outputBuffSize) },
        { "dataAddress",       DukUInt(env->dataAddress | 0x80000000) },
        { "dataSize",          DukUInt(env->dataSize) },
        { "yieldDataAddress",  DukUInt(env->yieldDataAddress | 0x80000000) },
        { "yieldDataSize",     DukUInt(env->yieldDataSize) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

static duk_idx_t CbArgs_PIEventObject(duk_context* ctx, void* _env)
{
    CScriptInstance* inst = GetInstance(ctx);
    JSHookPiDmaEnv* env = (JSHookPiDmaEnv*)_env;
    duk_push_object(ctx);
    SetDummyConstructor(ctx, -1, "PIEvent");

    const DukPropListEntry props[] = {
        { "callbackId",  DukUInt(inst->CallbackId()) },
        { "direction",   DukUInt(env->direction) },
        { "dramAddress", DukUInt(env->dramAddress | 0x80000000) },
        { "cartAddress", DukUInt(env->cartAddress | 0xA0000000) },
        { "length",      DukUInt(env->length + 1) },
        { nullptr }
    };

    DukPutPropList(ctx, -1, props);
    duk_freeze(ctx, -1);
    return 1;
}

duk_ret_t RequireAddressOrAddressRange(duk_context* ctx, duk_idx_t idx, uint32_t* addrStart, uint32_t* addrEnd)
{
    if(duk_is_number(ctx, idx))
    {
        if (abs(duk_get_number(ctx, idx)) > 0xFFFFFFFF)
        {
            duk_push_error_object(ctx, DUK_ERR_RANGE_ERROR,
                "address is out of range");
            return duk_throw(ctx);
        }

        uint32_t addr = duk_get_uint(ctx, idx);
        *addrStart = addr;
        *addrEnd = addr;
        return 0;
    }

    if(duk_is_object(ctx, idx))
    {
        if(!duk_has_prop_string(ctx, idx, "start") ||
           !duk_has_prop_string(ctx, idx, "end"))
        {
            duk_push_error_object(ctx, DUK_ERR_REFERENCE_ERROR,
                "object is missing 'start' or 'end' property");
            return duk_throw(ctx);
        }

        duk_get_prop_string(ctx, idx, "start");
        duk_get_prop_string(ctx, idx, "end");

        if(!duk_is_number(ctx, -2) ||
           !duk_is_number(ctx, -1))
        {
            duk_pop_n(ctx, 2);
            duk_push_error_object(ctx, DUK_ERR_REFERENCE_ERROR,
                "'start' and 'end' properties must be numbers");
            return duk_throw(ctx);
        }

        if (abs(duk_get_number(ctx, -2)) > 0xFFFFFFFF ||
            abs(duk_get_number(ctx, -1)) > 0xFFFFFFFF)
        {
            duk_push_error_object(ctx, DUK_ERR_RANGE_ERROR,
                "'start' or 'end' property out of range");
            return duk_throw(ctx);
        }

        *addrStart = duk_get_uint(ctx, -2);
        *addrEnd = duk_get_uint(ctx, -1);
        duk_pop_n(ctx, 2);
        return 0;
    }

    duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR,
        "argument %d invalid; expected number or object", idx);
    return duk_throw(ctx);
}

duk_ret_t RequireInterpreterCPU(duk_context* ctx)
{
    if (!g_Settings->LoadBool(Setting_ForceInterpreterCPU) &&
        (CPU_TYPE)g_Settings->LoadDword(Game_CpuType) != CPU_Interpreter)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR,
            "this feature requires the interpreter core");
        return duk_throw(ctx);
    }

    return 0;
}
