#include "../ScriptTypes.h"
#include "../ScriptSystem.h"
#include "../ScriptInstance.h"

#pragma once

#define HS_gAppCallbacks             DUK_HIDDEN_SYMBOL("gAppCallbacks")
#define HS_gInstancePtr              DUK_HIDDEN_SYMBOL("gInstancePtr")
#define HS_gInputListener            DUK_HIDDEN_SYMBOL("gInputListener")
#define HS_gOpenFileDescriptors      DUK_HIDDEN_SYMBOL("gOpenFileDescriptors")
#define HS_gKeepAlive                DUK_HIDDEN_SYMBOL("gKeepAlive")
#define HS_gPrivateCallEnabled       DUK_HIDDEN_SYMBOL("gPrivateCallEnabled")
#define HS_gNativeModules            DUK_HIDDEN_SYMBOL("gNativeModules")
#define HS_gObjectRefs               DUK_HIDDEN_SYMBOL("gObjectRefs")
#define HS_gNextObjectRefId          DUK_HIDDEN_SYMBOL("gNextObjectRefId")
#define HS_gIntervals                DUK_HIDDEN_SYMBOL("gIntervals")
#define HS_gNextInvervalId           DUK_HIDDEN_SYMBOL("gNextIntervalId")

#define HS_objectRefId               DUK_HIDDEN_SYMBOL("objectRefId")
#define HS_emitterListeners          DUK_HIDDEN_SYMBOL("emitterListeners")
#define HS_emitterNextListenerId     DUK_HIDDEN_SYMBOL("emitterNextListenerId")
#define HS_socketWorkerPtr           DUK_HIDDEN_SYMBOL("socketWorkerPtr")
#define HS_socketNextWriteCallbackId DUK_HIDDEN_SYMBOL("nextWriteCallbackId")
#define HS_socketWriteCallbacks      DUK_HIDDEN_SYMBOL("writeCallbacks")
#define HS_socketWriteEndCallbacks   DUK_HIDDEN_SYMBOL("endCallbacks")
#define HS_serverWorkerPtr           DUK_HIDDEN_SYMBOL("serverWorkerPtr")
#define HS_renderWindowPtr           DUK_HIDDEN_SYMBOL("renderWindowPtr")
#define HS_n64ImagePtr               DUK_HIDDEN_SYMBOL("n64ImagePtr")

namespace ScriptAPI
{
    struct DukPropListEntry;

    enum MemType {
        U8, U16, U32, S8, S16, S32, F32, F64,
        U64, S64
    };

    enum ArgType {
        Arg_Any,
        Arg_Number,
        Arg_BufferData,
        Arg_String,
        Arg_Function,
        Arg_Object,
        Arg_Array,
        Arg_Boolean,

        ArgAttr_Optional = (1 << 31),
        Arg_OptAny = Arg_Any | ArgAttr_Optional,
        Arg_OptNumber = Arg_Number | ArgAttr_Optional,
        Arg_OptBufferData = Arg_BufferData | ArgAttr_Optional,
        Arg_OptString = Arg_String | ArgAttr_Optional,
        Arg_OptFunction = Arg_Function | ArgAttr_Optional,
        Arg_OptObject = Arg_Object | ArgAttr_Optional,
        Arg_OptArray = Arg_Array | ArgAttr_Optional,
        Arg_OptBoolean = Arg_Boolean | ArgAttr_Optional,

        ArgAttrs = ArgAttr_Optional
    };

    // ScriptAPI
    void InitEnvironment(duk_context* ctx, CScriptInstance* inst);
    void DefineGlobalConstants(duk_context* ctx);

    void DefineGlobalClass(duk_context* ctx, const char* className,
        duk_c_function constructorFunc,
        const DukPropListEntry* prototypeProps = nullptr,
        const DukPropListEntry* staticProps = nullptr);

    void DefineGlobalInterface(duk_context* ctx, const char* name, const DukPropListEntry* props);
    
    void DefineGlobalFunction(duk_context* ctx, const char* name, duk_c_function func);

    CScriptInstance* GetInstance(duk_context* ctx);

    JSAppCallbackID AddAppCallback(duk_context* ctx, duk_idx_t callbackFuncIdx,
        JSAppHookID hookId,
        JSDukArgSetupFunc argSetupFunc = nullptr,
        JSAppCallbackCondFunc conditionFunc = nullptr,
        JSAppCallbackCleanupFunc cleanupFunc = nullptr);

    JSAppCallbackID AddAppCallback(duk_context* ctx, JSAppHookID hookId, JSAppCallback& callback);

    bool RemoveAppCallback(duk_context* ctx, JSAppCallbackID callbackId);
    duk_ret_t js__AppCallbackFinalizer(duk_context* ctx);

    void RefObject(duk_context* ctx, duk_idx_t idx);
    void UnrefObject(duk_context* ctx, duk_idx_t idx);
    duk_ret_t js__UnrefObject(duk_context* ctx);

    void InitEmitter(duk_context* ctx, duk_idx_t obj_idx, const std::vector<std::string>& eventNames);
    duk_ret_t js__Emitter_emit(duk_context* ctx);
    duk_ret_t js__Emitter_on(duk_context* ctx);
    duk_ret_t js__Emitter_off(duk_context* ctx);

    duk_ret_t js_Duktape_modSearch(duk_context* ctx); // require()
    void RegisterNativeModule(duk_context* ctx, HMODULE hModule);
    duk_ret_t js__NativeModuleFinalizer(duk_context* ctx);

    void AllowPrivateCall(duk_context* ctx, bool bAllow);
    bool PrivateCallAllowed(duk_context* ctx);

    void PushNewDummyConstructor(duk_context* ctx, bool bFrozen = true);
    void DefineGlobalDummyConstructors(duk_context* ctx, const char* constructorNames[], bool bFreeze = true);
    void SetDummyConstructor(duk_context* ctx, duk_idx_t obj_idx, const char* globalConstructorName);
    duk_ret_t js_DummyConstructor(duk_context* ctx);

    const char* ArgTypeName(ArgType argType);
    duk_bool_t ArgTypeMatches(duk_context* ctx, duk_idx_t idx, ArgType wantType);
    duk_ret_t CheckArgs(duk_context* ctx, const std::vector<ArgType>& argTypes);
    duk_ret_t CheckSetterAssignment(duk_context* ctx, ArgType wantType);

    duk_ret_t ThrowInvalidArgsError(duk_context* ctx);
    duk_ret_t ThrowInvalidArgError(duk_context* ctx, duk_idx_t idx, ArgType wantType);
    duk_ret_t ThrowTooManyArgsError(duk_context* ctx);
    duk_ret_t ThrowInvalidAssignmentError(duk_context* ctx, ArgType wantType);
    duk_ret_t ThrowNotCallableError(duk_context* ctx);

    void DebugStack(duk_context* ctx, const char* file, int line);

    // ScriptAPI_events
    void Define_events(duk_context* ctx);
    duk_ret_t js_events_onstatechange(duk_context* ctx);
    duk_ret_t js_events_onexec(duk_context* ctx);
    duk_ret_t js_events_onread(duk_context* ctx);
    duk_ret_t js_events_onwrite(duk_context* ctx);
    duk_ret_t js_events_onopcode(duk_context* ctx);
    duk_ret_t js_events_ongprvalue(duk_context* ctx);
    duk_ret_t js_events_ondraw(duk_context* ctx);
    duk_ret_t js_events_onpifread(duk_context* ctx);
    duk_ret_t js_events_onsptask(duk_context* ctx);
    duk_ret_t js_events_onpidma(duk_context* ctx);
    duk_ret_t js_events_onmouseup(duk_context* ctx);
    duk_ret_t js_events_onmousedown(duk_context* ctx);
    duk_ret_t js_events_onmousemove(duk_context* ctx);
    duk_ret_t js_events_remove(duk_context* ctx);

    // ScriptAPI_console
    void Define_console(duk_context* ctx);
    duk_ret_t js_console_print(duk_context* ctx);
    duk_ret_t js_console_log(duk_context* ctx);
    duk_ret_t js_console_error(duk_context* ctx);
    duk_ret_t js_console_clear(duk_context* ctx);
    duk_ret_t js_console_listen(duk_context* ctx);

    // ScriptAPI_mem
    void Define_mem(duk_context* ctx);
    template <class T> duk_ret_t js_mem__get(duk_context* ctx);
    template <class T> duk_ret_t js_mem__set(duk_context* ctx);
    duk_ret_t js_mem__boundget(duk_context* ctx);
    duk_ret_t js_mem__boundset(duk_context* ctx);
    duk_ret_t js_mem__type_constructor(duk_context* ctx);
    duk_ret_t js_mem__get_ramsize(duk_context* ctx);
    duk_ret_t js_mem__get_romsize(duk_context* ctx);
    duk_ret_t js_mem__get_ptr(duk_context* ctx);
    duk_ret_t js_mem_getblock(duk_context* ctx);
    duk_ret_t js_mem_getstring(duk_context* ctx);
    duk_ret_t js_mem_setblock(duk_context* ctx);
    duk_ret_t js_mem_bindvar(duk_context* ctx);
    duk_ret_t js_mem_bindvars(duk_context* ctx);
    duk_ret_t js_mem_bindstruct(duk_context* ctx);
    duk_ret_t js_mem_typedef(duk_context* ctx);

    // ScriptAPI_Server
    void Define_Server(duk_context* ctx);
    duk_ret_t js_Server__constructor(duk_context* ctx);
    duk_ret_t js_Server__finalizer(duk_context* ctx);
    duk_ret_t js_Server__get_port(duk_context* ctx);
    duk_ret_t js_Server__get_address(duk_context* ctx);
    duk_ret_t js_Server__get_addressFamily(duk_context* ctx);
    duk_ret_t js_Server_listen(duk_context* ctx);
    duk_ret_t js_Server_close(duk_context* ctx);

    // ScriptAPI_Socket
    void Define_Socket(duk_context* ctx);
    duk_ret_t js_Socket__constructor(duk_context* ctx);
    duk_ret_t js_Socket__finalizer(duk_context* ctx);
    duk_ret_t js_Socket__invokeWriteCallback(duk_context* ctx);
    duk_ret_t js_Socket__invokeWriteEndCallbacks(duk_context* ctx);
    duk_ret_t js_Socket_connect(duk_context* ctx);
    duk_ret_t js_Socket_write(duk_context* ctx);
    duk_ret_t js_Socket_end(duk_context* ctx);
    duk_ret_t js_Socket_close(duk_context* ctx);
    duk_ret_t js_Socket__get_localAddress(duk_context* ctx);
    duk_ret_t js_Socket__get_localPort(duk_context* ctx);
    duk_ret_t js_Socket__get_remoteAddress(duk_context* ctx);
    duk_ret_t js_Socket__get_remotePort(duk_context* ctx);
    duk_ret_t js_Socket__get_addressFamily(duk_context* ctx);
    
    // ScriptAPI_script
    void Define_script(duk_context* ctx);
    duk_ret_t js_script_keepalive(duk_context* ctx);
    duk_ret_t js_script_timeout(duk_context* ctx);

    // ScriptAPI_fs
    void Define_fs(duk_context* ctx);
    duk_ret_t js_fs_open(duk_context* ctx);
    duk_ret_t js_fs_close(duk_context* ctx);
    duk_ret_t js_fs_write(duk_context* ctx);
    duk_ret_t js_fs_writefile(duk_context* ctx);
    duk_ret_t js_fs_read(duk_context* ctx);
    duk_ret_t js_fs_readfile(duk_context* ctx);
    duk_ret_t js_fs_fstat(duk_context* ctx);
    duk_ret_t js_fs_stat(duk_context* ctx);
    duk_ret_t js_fs_unlink(duk_context* ctx);
    duk_ret_t js_fs_mkdir(duk_context* ctx);
    duk_ret_t js_fs_rmdir(duk_context* ctx);
    duk_ret_t js_fs_readdir(duk_context* ctx);
    duk_ret_t js_fs_Stats__constructor(duk_context* ctx);
    duk_ret_t js_fs_Stats_isDirectory(duk_context* ctx);
    duk_ret_t js_fs_Stats_isFile(duk_context* ctx);

    // ScriptAPI_debug
    void Define_debug(duk_context* ctx);
    duk_ret_t js_debug__get_paused(duk_context* ctx);
    duk_ret_t js_debug_breakhere(duk_context* ctx);
    duk_ret_t js_debug_step(duk_context* ctx);
    duk_ret_t js_debug_skip(duk_context* ctx);
    duk_ret_t js_debug_resume(duk_context* ctx);
    duk_ret_t js_debug_showmemory(duk_context* ctx);
    duk_ret_t js_debug_showcommands(duk_context* ctx);

    // ScriptAPI_asm
    void Define_asm(duk_context* ctx);
    duk_ret_t js_asm_gprname(duk_context* ctx);
    duk_ret_t js_asm_encode(duk_context* ctx);
    duk_ret_t js_asm_decode(duk_context* ctx);

    // ScriptAPI_cpu
    void Define_cpu(duk_context* ctx);
    duk_ret_t js_cpu_get(duk_context* ctx);
    duk_ret_t js_cpu_set(duk_context* ctx);
    duk_ret_t js_cpu_gpr_get(duk_context* ctx);
    duk_ret_t js_cpu_gpr_set(duk_context* ctx);
    duk_ret_t js_cpu_ugpr_get(duk_context* ctx);
    duk_ret_t js_cpu_ugpr_set(duk_context* ctx);
    duk_ret_t js_cpu_fpr_get(duk_context* ctx);
    duk_ret_t js_cpu_fpr_set(duk_context* ctx);
    duk_ret_t js_cpu_dfpr_get(duk_context* ctx);
    duk_ret_t js_cpu_dfpr_set(duk_context* ctx);
    duk_ret_t js_cpu_cop0_get(duk_context* ctx);
    duk_ret_t js_cpu_cop0_set(duk_context* ctx);

    // ScriptAPI_pj64
    void Define_pj64(duk_context* ctx);
    duk_ret_t js_pj64_open(duk_context* ctx);
    duk_ret_t js_pj64_close(duk_context* ctx);
    duk_ret_t js_pj64_reset(duk_context* ctx);
    duk_ret_t js_pj64_pause(duk_context* ctx);
    duk_ret_t js_pj64_resume(duk_context* ctx);
    duk_ret_t js_pj64_limitfps(duk_context* ctx);
    duk_ret_t js_pj64__get_installDirectory(duk_context* ctx);
    duk_ret_t js_pj64__get_scriptsDirectory(duk_context* ctx);
    duk_ret_t js_pj64__get_modulesDirectory(duk_context* ctx);
    duk_ret_t js_pj64__get_romDirectory(duk_context* ctx);
    duk_ret_t js_pj64__get_romInfo(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_goodName(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_fileName(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_filePath(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerCrc1(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerCrc2(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerName(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerMediaFormat(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerId(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerCountryCode(duk_context* ctx);
    duk_ret_t js_pj64_romInfo__get_headerVersion(duk_context* ctx);

    // ScriptAPI_AddressRange
    void Define_AddressRange(duk_context* ctx);
    duk_ret_t js_AddressRange__constructor(duk_context* ctx);

    // ScriptAPI_Number_hex
    void Define_Number_prototype_hex(duk_context* ctx);
    duk_ret_t js_Number_prototype_hex(duk_context* ctx);

    // ScriptAPI_N64Image
    void Define_N64Image(duk_context* ctx);
    duk_ret_t js_N64Image__constructor(duk_context* ctx);
    duk_ret_t js_N64Image__finalizer(duk_context* ctx);
    duk_ret_t js_N64Image_static_fromPNG(duk_context* ctx);
    duk_ret_t js_N64Image_static_format(duk_context* ctx);
    duk_ret_t js_N64Image_static_bpp(duk_context* ctx);
    duk_ret_t js_N64Image_toPNG(duk_context* ctx);
    duk_ret_t js_N64Image_update(duk_context* ctx);

    // ScriptAPI_exec
    void Define_exec(duk_context* ctx);
    duk_ret_t js_exec(duk_context* ctx);

    // ScriptAPI_alert
    void Define_alert(duk_context* ctx);
    duk_ret_t js_alert(duk_context* ctx);

    // ScriptAPI_interval
    void Define_interval(duk_context* ctx);
    duk_ret_t js_setInterval(duk_context* ctx);
    duk_ret_t js_clearInterval(duk_context* ctx);
    duk_ret_t js_setTimeout(duk_context* ctx);
    duk_ret_t js_clearTimeout(duk_context* ctx);
    duk_ret_t js__IntervalContext_invokeFunc(duk_context* ctx);
    duk_ret_t js__IntervalContext_remove(duk_context* ctx);
    duk_ret_t js__IntervalContext_finalizer(duk_context* ctx);

    enum {
        R0, AT, V0, V1, A0, A1, A2, A3,
        T0, T1, T2, T3, T4, T5, T6, T7,
        S0, S1, S2, S3, S4, S5, S6, S7,
        T8, T9, K0, K1, GP, SP, FP, RA,
        //S8 = FP
    };

    enum {
        GPR_R0 = (1 << R0),
        GPR_AT = (1 << AT),
        GPR_V0 = (1 << V0),
        GPR_V1 = (1 << V1),
        GPR_A0 = (1 << A0),
        GPR_A1 = (1 << A1),
        GPR_A2 = (1 << A2),
        GPR_A3 = (1 << A3),
        GPR_T0 = (1 << T0),
        GPR_T1 = (1 << T1),
        GPR_T2 = (1 << T2),
        GPR_T3 = (1 << T3),
        GPR_T4 = (1 << T4),
        GPR_T5 = (1 << T5),
        GPR_T6 = (1 << T6),
        GPR_T7 = (1 << T7),
        GPR_S0 = (1 << S0),
        GPR_S1 = (1 << S1),
        GPR_S2 = (1 << S2),
        GPR_S3 = (1 << S3),
        GPR_S4 = (1 << S4),
        GPR_S5 = (1 << S5),
        GPR_S6 = (1 << S6),
        GPR_S7 = (1 << S7),
        GPR_T8 = (1 << T8),
        GPR_T9 = (1 << T9),
        GPR_K0 = (1 << K0),
        GPR_K1 = (1 << K1),
        GPR_GP = (1 << GP),
        GPR_SP = (1 << SP),
        GPR_FP = (1 << FP),
        GPR_RA = (1 << RA),
        //GPR_S8 = GPR_FP,
        GPR_ANY = 0xFFFFFFFF
    };

#define DUK_TYPE_ID(id) \
static const DukPropTypeID _TYPE = DukPropTypeID::Type_ ## id;

#define DUK_DECLVAL_MEMBER(structName, memberName) \
structName memberName; Value(structName v) : memberName(v) {}

#define DUK_SCALARTYPE_IMPL(structName, primitiveType) \
DUK_TYPE_ID(structName) primitiveType value; structName(primitiveType value) : value(value){}

    enum DukPropTypeID
    {
        Type_DukInt,
        Type_DukUInt,
        Type_DukNumber,
        Type_DukString,
        Type_DukBoolean,
        Type_DukPointer,
        Type_DukCFunction,
        Type_DukDupIndex,
        Type_DukGetter,
        Type_DukGetterSetter,
        Type_DukObject,
        Type_DukProxy
    };

    struct DukInt { DUK_SCALARTYPE_IMPL(DukInt, duk_int_t) };
    struct DukUInt { DUK_SCALARTYPE_IMPL(DukUInt, duk_uint_t) };
    struct DukNumber { DUK_SCALARTYPE_IMPL(DukNumber, duk_double_t) };
    struct DukBoolean { DUK_SCALARTYPE_IMPL(DukBoolean, duk_bool_t) };
    struct DukString { DUK_SCALARTYPE_IMPL(DukString, const char*) };
    struct DukPointer { DUK_SCALARTYPE_IMPL(DukPointer, void*) };
    struct DukDupIndex { DUK_SCALARTYPE_IMPL(DukDupIndex, duk_idx_t) };
    //struct DukObject { DUK_SCALARTYPE_IMPL(DukObject, duk_idx_t) };

    struct DukCFunction {
        DUK_TYPE_ID(DukCFunction)
        duk_c_function func;
        duk_int_t nargs;
        DukCFunction(duk_c_function func, duk_int_t nargs = DUK_VARARGS) :
            func(func), nargs(nargs) {}
    };

    struct DukObject {
        DUK_TYPE_ID(DukObject)
        const DukPropListEntry* props;
        DukObject(const DukPropListEntry* props = nullptr) :
            props(props) {}
    };

    struct DukGetter {
        DUK_SCALARTYPE_IMPL(DukGetter, duk_c_function)
    };

    struct DukGetterSetter {
        DUK_TYPE_ID(DukGetterSetter)
        duk_c_function getter, setter;
        DukGetterSetter(duk_c_function getter, duk_c_function setter) :
            getter(getter), setter(setter) {}
    };

    struct DukProxy {
        DUK_TYPE_ID(DukProxy)
        duk_c_function getter, setter;
        DukProxy(duk_c_function getter, duk_c_function setter) :
            getter(getter), setter(setter) {}
    };

    struct DukPropListEntry
    {
        const char* key;
        DukPropTypeID typeId;
        bool writable, enumerable;

        union Value {
            DUK_DECLVAL_MEMBER(DukInt, dukInt)
            DUK_DECLVAL_MEMBER(DukUInt, dukUInt)
            DUK_DECLVAL_MEMBER(DukNumber, dukNumber)
            DUK_DECLVAL_MEMBER(DukBoolean, dukBoolean)
            DUK_DECLVAL_MEMBER(DukString, dukString)
            DUK_DECLVAL_MEMBER(DukPointer, dukPointer)
            DUK_DECLVAL_MEMBER(DukCFunction, dukCFunction)
            DUK_DECLVAL_MEMBER(DukGetter, dukGetter)
            DUK_DECLVAL_MEMBER(DukGetterSetter, dukGetterSetter)
            DUK_DECLVAL_MEMBER(DukDupIndex, dukDupIndex)
            DUK_DECLVAL_MEMBER(DukObject, dukObject)
            DUK_DECLVAL_MEMBER(DukProxy, dukProxy)
        } value;

        template<class T>
        DukPropListEntry(const char* key, T value, bool writable = false, bool enumerable = true) :
            key(key), typeId(T::_TYPE), writable(writable), enumerable(enumerable), value(value) {}

        DukPropListEntry(nullptr_t) :
            key(nullptr), value(DukInt(0)) {}

        DukPropListEntry(const char* key) :
            key(key), typeId(Type_DukInt), writable(false), enumerable(false), value(DukInt(0)) {}
    };

    // todo proxy object
    // todo DukObjectConfig_DummyConstructor
    // todo DukObjectConfig_Finalizer
    // todo DukObjectConfig_Freeze

    void DukPutPropList(duk_context* ctx, duk_idx_t obj_idx, const DukPropListEntry* props);
};
