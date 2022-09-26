#include "stdafx.h"

#include "ScriptAPI.h"
#include <Project64-core/N64System/Mips/Register.h>

#pragma warning(disable : 4702) // disable unreachable code warning

static duk_ret_t GPRGetImpl(duk_context * ctx, bool bUpper);
static duk_ret_t GPRSetImpl(duk_context * ctx, bool bUpper);
static duk_ret_t FPRGetImpl(duk_context * ctx, bool bDouble);
static duk_ret_t FPRSetImpl(duk_context * ctx, bool bDouble);

static int FPRIndex(const char * regName);
static int GPRIndex(const char * regName);
static uint32_t * COP0RegPtr(const char * regName);
static uint32_t * CPURegPtr(const char * regName);

static duk_ret_t ThrowRegInvalidError(duk_context * ctx);
static duk_ret_t ThrowRegContextUnavailableError(duk_context * ctx);
static duk_ret_t ThrowRegAssignmentTypeError(duk_context * ctx);

void ScriptAPI::Define_cpu(duk_context * ctx)
{
    // todo cleanup

    #define REG_PROXY_FUNCTIONS(getter, setter) { \
        { "get", getter, 2 }, \
        { "set", setter, 3 }, \
        { nullptr, nullptr, 0 } \
    }

    const struct
    {
        const char * key;
        const duk_function_list_entry functions[3];
    } proxies[] = {
        {"gpr", REG_PROXY_FUNCTIONS(js_cpu_gpr_get, js_cpu_gpr_set)},
        {"ugpr", REG_PROXY_FUNCTIONS(js_cpu_ugpr_get, js_cpu_ugpr_set)},
        {"fpr", REG_PROXY_FUNCTIONS(js_cpu_fpr_get, js_cpu_fpr_set)},
        {"dfpr", REG_PROXY_FUNCTIONS(js_cpu_dfpr_get, js_cpu_dfpr_set)},
        {"cop0", REG_PROXY_FUNCTIONS(js_cpu_cop0_get, js_cpu_cop0_set)},
        {nullptr, nullptr},
    };

    const duk_function_list_entry cpufuncs[] = REG_PROXY_FUNCTIONS(js_cpu_get, js_cpu_set);

    duk_push_global_object(ctx);
    duk_push_object(ctx);

    for (size_t i = 0; proxies[i].key != nullptr; i++)
    {
        duk_push_string(ctx, proxies[i].key);
        duk_push_object(ctx);
        duk_push_object(ctx);
        duk_put_function_list(ctx, -1, proxies[i].functions);
        duk_push_proxy(ctx, 0);
        duk_freeze(ctx, -1);
        duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
    }

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, cpufuncs);
    duk_push_proxy(ctx, 0);
    duk_freeze(ctx, -1);

    duk_put_prop_string(ctx, -2, "cpu");
    duk_pop(ctx);
}

duk_ret_t ScriptAPI::js_cpu_get(duk_context * ctx)
{
    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    const char * key = duk_get_string(ctx, 1);
    uint32_t * pReg = CPURegPtr(key);

    if (pReg == nullptr)
    {
        duk_get_prop_string(ctx, 0, key);
        return 1;
    }

    duk_push_uint(ctx, *pReg);
    return 1;
}

duk_ret_t ScriptAPI::js_cpu_set(duk_context * ctx)
{
    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    uint32_t * pReg = CPURegPtr(duk_get_string(ctx, 1));

    if (!duk_is_number(ctx, 2) || pReg == nullptr)
    {
        return ThrowRegAssignmentTypeError(ctx);
    }

    *pReg = duk_get_uint(ctx, 2);
    duk_push_true(ctx);
    return 1;
}

duk_ret_t ScriptAPI::js_cpu_gpr_get(duk_context * ctx)
{
    return GPRGetImpl(ctx, false);
}

duk_ret_t ScriptAPI::js_cpu_gpr_set(duk_context * ctx)
{
    return GPRSetImpl(ctx, false);
}

duk_ret_t ScriptAPI::js_cpu_ugpr_get(duk_context * ctx)
{
    return GPRGetImpl(ctx, true);
}

duk_ret_t ScriptAPI::js_cpu_ugpr_set(duk_context * ctx)
{
    return GPRSetImpl(ctx, true);
}

duk_ret_t ScriptAPI::js_cpu_fpr_get(duk_context * ctx)
{
    return FPRGetImpl(ctx, false);
}

duk_ret_t ScriptAPI::js_cpu_fpr_set(duk_context * ctx)
{
    return FPRSetImpl(ctx, false);
}

duk_ret_t ScriptAPI::js_cpu_dfpr_get(duk_context * ctx)
{
    return FPRGetImpl(ctx, true);
}

duk_ret_t ScriptAPI::js_cpu_dfpr_set(duk_context * ctx)
{
    return FPRSetImpl(ctx, true);
}

duk_ret_t ScriptAPI::js_cpu_cop0_get(duk_context * ctx)
{
    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    if (!duk_is_string(ctx, 1))
    {
        return ThrowRegInvalidError(ctx);
    }

    const char * name = duk_get_string(ctx, 1);

    if (strcmp(name, "cause") == 0)
    {
        duk_push_uint(ctx, (uint32_t)(g_Reg->FAKE_CAUSE_REGISTER | g_Reg->CAUSE_REGISTER));
        return 1;
    }

    uint32_t * reg = COP0RegPtr(name);

    if (reg == nullptr)
    {
        return ThrowRegInvalidError(ctx);
    }

    duk_push_uint(ctx, *reg);
    return 1;
}

duk_ret_t ScriptAPI::js_cpu_cop0_set(duk_context * ctx)
{
    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    if (!duk_is_string(ctx, 1))
    {
        return ThrowRegInvalidError(ctx);
    }

    const char * name = duk_get_string(ctx, 1);

    if (!duk_is_number(ctx, 2))
    {
        return ThrowRegAssignmentTypeError(ctx);
    }

    if (strcmp(name, "cause") == 0)
    {
        uint32_t value = duk_get_uint(ctx, 2);
        g_Reg->FAKE_CAUSE_REGISTER = value;
        g_Reg->CAUSE_REGISTER = value;
        g_Reg->CheckInterrupts();

        duk_push_true(ctx);
        return 1;
    }

    uint32_t * reg = COP0RegPtr(name);

    if (reg == nullptr)
    {
        return ThrowRegInvalidError(ctx);
    }

    *reg = duk_get_uint(ctx, 2);
    duk_push_true(ctx);
    return 1;
}

static duk_ret_t GPRGetImpl(duk_context * ctx, bool bUpper)
{
    int regIndex = -1;

    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    if (duk_is_number(ctx, 1))
    {
        regIndex = duk_get_int(ctx, 1);
    }
    else if (duk_is_string(ctx, 1))
    {
        regIndex = GPRIndex(duk_get_string(ctx, 1));
    }

    if (regIndex < 0 || regIndex > 31)
    {
        return ThrowRegInvalidError(ctx);
    }

    duk_push_uint(ctx, g_Reg->m_GPR[regIndex].UW[bUpper ? 1 : 0]);
    return 1;
}

static duk_ret_t GPRSetImpl(duk_context * ctx, bool bUpper)
{
    int regIndex = -1;

    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    if (!duk_is_number(ctx, 2))
    {
        return ThrowRegAssignmentTypeError(ctx);
    }

    uint32_t value = duk_get_uint(ctx, 2);

    if (duk_is_number(ctx, 1))
    {
        regIndex = duk_get_int(ctx, 1);
    }
    else if (duk_is_string(ctx, 1))
    {
        regIndex = GPRIndex(duk_get_string(ctx, 1));
    }

    if (regIndex == 0)
    {
        duk_push_true(ctx);
        return 1;
    }

    if (regIndex < 0 || regIndex > 31)
    {
        return ThrowRegInvalidError(ctx);
    }

    g_Reg->m_GPR[regIndex].UW[bUpper ? 1 : 0] = value;
    duk_push_true(ctx);
    return 1;
}

static duk_ret_t FPRGetImpl(duk_context * ctx, bool bDouble)
{
    int regIndex = -1;

    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    if (duk_is_number(ctx, 1))
    {
        regIndex = duk_get_int(ctx, 1);
    }
    else if (duk_is_string(ctx, 1))
    {
        regIndex = FPRIndex(duk_get_string(ctx, 1));
    }

    if (regIndex < 0 || regIndex > 31)
    {
        return ThrowRegInvalidError(ctx);
    }

    if (bDouble)
    {
        duk_push_number(ctx, (duk_double_t)*g_Reg->m_FPR_D[regIndex & 0x1E]);
    }
    else
    {
        duk_push_number(ctx, (duk_double_t)*g_Reg->m_FPR_S[regIndex]);
    }

    return 1;
}

static duk_ret_t FPRSetImpl(duk_context * ctx, bool bDouble)
{
    int regIndex = -1;

    if (g_Reg == nullptr)
    {
        return ThrowRegContextUnavailableError(ctx);
    }

    if (!duk_is_number(ctx, 2))
    {
        return ThrowRegAssignmentTypeError(ctx);
    }

    if (duk_is_number(ctx, 1))
    {
        regIndex = duk_get_int(ctx, 1);
    }
    else if (duk_is_string(ctx, 1))
    {
        regIndex = FPRIndex(duk_get_string(ctx, 1));
    }

    if (regIndex < 0 || regIndex > 31)
    {
        return ThrowRegInvalidError(ctx);
    }

    duk_double_t value = duk_get_number(ctx, 2);

    if (bDouble)
    {
        *g_Reg->m_FPR_D[regIndex & 0x1E] = value;
    }
    else
    {
        *g_Reg->m_FPR_S[regIndex] = (float)value;
    }

    duk_push_true(ctx);
    return 1;
}

static int GPRIndex(const char * regName)
{
    const char * names[] = {
        "r0",
        "at",
        "v0",
        "v1",
        "a0",
        "a1",
        "a2",
        "a3",
        "t0",
        "t1",
        "t2",
        "t3",
        "t4",
        "t5",
        "t6",
        "t7",
        "s0",
        "s1",
        "s2",
        "s3",
        "s4",
        "s5",
        "s6",
        "s7",
        "t8",
        "t9",
        "k0",
        "k1",
        "gp",
        "sp",
        "fp",
        "ra",
    };

    for (int i = 0; i < 32; i++)
    {
        if (strcmp(names[i], regName) == 0)
        {
            return i;
        }
    }

    return -1;
}

static int FPRIndex(const char * regName)
{
    const char * names[32] = {
        "f0",
        "f1",
        "f2",
        "f3",
        "f4",
        "f5",
        "f6",
        "f7",
        "f8",
        "f9",
        "f10",
        "f11",
        "f12",
        "f13",
        "f14",
        "f15",
        "f16",
        "f17",
        "f18",
        "f19",
        "f20",
        "f21",
        "f22",
        "f23",
        "f24",
        "f25",
        "f26",
        "f27",
        "f28",
        "f29",
        "f30",
        "f31",
    };

    for (int i = 0; i < 32; i++)
    {
        if (strcmp(names[i], regName) == 0)
        {
            return i;
        }
    }

    return -1;
}

static uint32_t * COP0RegPtr(const char * regName)
{
    if (g_Reg == nullptr)
    {
        return nullptr;
    }

    struct
    {
        const char * name;
        uint32_t * ptr;
    } names[] = {
        {"index", (uint32_t *)&g_Reg->INDEX_REGISTER},
        {"random", (uint32_t *)&g_Reg->RANDOM_REGISTER},
        {"entrylo0", (uint32_t *)&g_Reg->ENTRYLO0_REGISTER},
        {"entrylo1", (uint32_t *)&g_Reg->ENTRYLO1_REGISTER},
        {"context", (uint32_t *)&g_Reg->CONTEXT_REGISTER},
        {"pagemask", (uint32_t *)&g_Reg->PAGE_MASK_REGISTER},
        {"wired", (uint32_t *)&g_Reg->WIRED_REGISTER},
        {"badvaddr", (uint32_t *)&g_Reg->BAD_VADDR_REGISTER},
        {"count", (uint32_t *)&g_Reg->COUNT_REGISTER},
        {"entryhi", (uint32_t *)&g_Reg->ENTRYHI_REGISTER},
        {"compare", (uint32_t *)&g_Reg->COMPARE_REGISTER},
        {"status", (uint32_t *)&g_Reg->STATUS_REGISTER},
        //{ "cause", (uint32_t*)&g_Reg->CAUSE_REGISTER },
        {"epc", (uint32_t *)&g_Reg->EPC_REGISTER},
        {"config", (uint32_t *)&g_Reg->CONFIG_REGISTER},
        {"taglo", (uint32_t *)&g_Reg->TAGLO_REGISTER},
        {"taghi", (uint32_t *)&g_Reg->TAGHI_REGISTER},
        {"errorepc", (uint32_t *)&g_Reg->ERROREPC_REGISTER},
        {nullptr, nullptr},
    };

    for (int i = 0; names[i].name != nullptr; i++)
    {
        if (strcmp(regName, names[i].name) == 0)
        {
            return names[i].ptr;
        }
    }

    return nullptr;
}

static uint32_t * CPURegPtr(const char * key)
{
    if (g_Reg == nullptr)
    {
        return nullptr;
    }

    if (strcmp(key, "pc") == 0)
    {
        return &g_Reg->m_PROGRAM_COUNTER;
    }
    else if (strcmp(key, "hi") == 0)
    {
        return &g_Reg->m_HI.UW[0];
    }
    else if (strcmp(key, "uhi") == 0)
    {
        return &g_Reg->m_HI.UW[1];
    }
    else if (strcmp(key, "lo") == 0)
    {
        return &g_Reg->m_LO.UW[0];
    }
    else if (strcmp(key, "ulo") == 0)
    {
        return &g_Reg->m_LO.UW[1];
    }
    else if (strcmp(key, "fcr31") == 0)
    {
        return &g_Reg->m_FPCR[31];
    }

    return nullptr;
}

static duk_ret_t ThrowRegInvalidError(duk_context * ctx)
{
    duk_push_error_object(ctx, DUK_ERR_REFERENCE_ERROR, "invalid register name or number");
    return duk_throw(ctx);
}

static duk_ret_t ThrowRegContextUnavailableError(duk_context * ctx)
{
    duk_push_error_object(ctx, DUK_ERR_ERROR, "CPU register context is unavailable");
    return duk_throw(ctx);
}

static duk_ret_t ThrowRegAssignmentTypeError(duk_context * ctx)
{
    duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "invalid register value assignment");
    return duk_throw(ctx);
}
