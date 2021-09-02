#include <stdafx.h>
#include "ScriptAPI.h"
#include <Project64/UserInterface/DiscordRPC.h>
#include <Project64/UserInterface/MainWindow.h>

void ScriptAPI::Define_pj64(duk_context* ctx)
{
    const DukPropListEntry romInfoProps[] = {
        { "goodName",    DukGetter(js_pj64_romInfo__get_goodName) },
        { "fileName",    DukGetter(js_pj64_romInfo__get_fileName) },
        { "filePath",    DukGetter(js_pj64_romInfo__get_filePath) },
        { "crc1",        DukGetter(js_pj64_romInfo__get_headerCrc1) },
        { "crc2",        DukGetter(js_pj64_romInfo__get_headerCrc2) },
        { "name",        DukGetter(js_pj64_romInfo__get_headerName) },
        { "mediaFormat", DukGetter(js_pj64_romInfo__get_headerMediaFormat) },
        { "id",          DukGetter(js_pj64_romInfo__get_headerId) },
        { "countryCode", DukGetter(js_pj64_romInfo__get_headerCountryCode) },
        { "version",     DukGetter(js_pj64_romInfo__get_headerVersion) },
        { nullptr }
    };

    const DukPropListEntry props[] = {
        { "open",             DukCFunction(js_pj64_open) },
        { "close",            DukCFunction(js_pj64_close) },
        { "reset",            DukCFunction(js_pj64_reset) },
        { "pause",            DukCFunction(js_pj64_pause) },
        { "resume",           DukCFunction(js_pj64_resume) },
        { "limitfps",         DukCFunction(js_pj64_limitfps) },
        //{ "savestate",        DukCFunction(js_pj64_savestate) },
        //{ "loadstate",        DukCFunction(js_pj64_loadstate) },
        { "installDirectory", DukGetter(js_pj64__get_installDirectory) },
        { "scriptsDirectory", DukGetter(js_pj64__get_scriptsDirectory) },
        { "modulesDirectory", DukGetter(js_pj64__get_modulesDirectory) },
        { "romDirectory",     DukGetter(js_pj64__get_romDirectory) },
        { DUK_HIDDEN_SYMBOL("romInfo"), DukObject(romInfoProps) },
        { "romInfo", DukGetter(js_pj64__get_romInfo) },
        { nullptr }
    };

    DefineGlobalInterface(ctx, "pj64", props);
}

duk_ret_t ScriptAPI::js_pj64_open(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char* romPath = duk_get_string(ctx, 0);

    HWND hMainWindow = (HWND)g_Plugins->MainWindow()->GetWindowHandle();
    char* romPathCopy = new char[MAX_PATH]; // main window proc deletes
    strncpy(romPathCopy, romPath, MAX_PATH);
    PostMessage(hMainWindow, WM_JSAPI_ACTION, JSAPI_ACT_OPEN_ROM, (WPARAM)romPath);

    return 0;
}

duk_ret_t ScriptAPI::js_pj64_close(duk_context* /*ctx*/)
{
    HWND hMainWindow = (HWND)g_Plugins->MainWindow()->GetWindowHandle();
    PostMessage(hMainWindow, WM_JSAPI_ACTION, JSAPI_ACT_CLOSE_ROM, 0);
    return 0;
}

duk_ret_t ScriptAPI::js_pj64_reset(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_OptBoolean });
    bool bSoftReset = (bool)duk_get_boolean_default(ctx, 0, (duk_bool_t)false);

    HWND hMainWindow = (HWND)g_Plugins->MainWindow()->GetWindowHandle();
    PostMessage(hMainWindow, WM_JSAPI_ACTION, JSAPI_ACT_RESET, (WPARAM)bSoftReset);
    return 0;
}

duk_ret_t ScriptAPI::js_pj64_pause(duk_context* /*ctx*/)
{
    HWND hMainWindow = (HWND)g_Plugins->MainWindow()->GetWindowHandle();
    PostMessage(hMainWindow, WM_JSAPI_ACTION, JSAPI_ACT_PAUSE, 0);
    return 0;
}

duk_ret_t ScriptAPI::js_pj64_resume(duk_context* /*ctx*/)
{
    HWND hMainWindow = (HWND)g_Plugins->MainWindow()->GetWindowHandle();
    PostMessage(hMainWindow, WM_JSAPI_ACTION, JSAPI_ACT_RESUME, 0);
    return 0;
}

duk_ret_t ScriptAPI::js_pj64_limitfps(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_Boolean });
    bool bLimitFps = duk_get_boolean(ctx, 0);
    g_Settings->SaveBool(GameRunning_LimitFPS, bLimitFps);
    return 0;
}

/*
duk_ret_t ScriptAPI::js_pj64_savestate(duk_context* ctx)
{
    // todo: adjust path
    CheckArgs(ctx, { Arg_OptString });
    const char* path = duk_get_string_default(ctx, 0, "");

    g_Settings->SaveString(GameRunning_InstantSaveFile, path);
    g_System->ExternalEvent(SysEvent_SaveMachineState);
    return 0;
}

duk_ret_t ScriptAPI::js_pj64_loadstate(duk_context* ctx)
{
    // todo: adjust path
    CheckArgs(ctx, { Arg_OptString });
    const char* path = duk_get_string_default(ctx, 0, "");

    g_Settings->SaveString(GameRunning_InstantSaveFile, path);
    g_System->ExternalEvent(SysEvent_LoadMachineState);
    return 0;
}
*/

duk_ret_t ScriptAPI::js_pj64__get_installDirectory(duk_context* ctx)
{
    stdstr dirPath = GetInstance(ctx)->System()->InstallDirPath();
    duk_push_string(ctx, dirPath.c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64__get_scriptsDirectory(duk_context* ctx)
{
    stdstr dirPath = GetInstance(ctx)->System()->ScriptsDirPath();
    duk_push_string(ctx, dirPath.c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64__get_modulesDirectory(duk_context* ctx)
{
    stdstr dirPath = GetInstance(ctx)->System()->ModulesDirPath();
    duk_push_string(ctx, dirPath.c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64__get_romDirectory(duk_context * ctx)
{
    duk_push_string(ctx, g_Settings->LoadStringVal(RomList_GameDir).c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64__get_romInfo(duk_context* ctx)
{
    if (g_Settings->LoadStringVal(Game_GameName) != "")
    {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("romInfo"));
    }
    else
    {
        duk_push_null(ctx);
    }
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_goodName(duk_context* ctx)
{
    duk_push_string(ctx, g_Settings->LoadStringVal(Rdb_GoodName).c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_fileName(duk_context* ctx)
{
    duk_push_string(ctx, CPath(g_Settings->LoadStringVal(Game_File)).GetNameExtension().c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_filePath(duk_context* ctx)
{
    duk_push_string(ctx, g_Settings->LoadStringVal(Game_File).c_str());
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerCrc1(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    uint32_t crc1 = 0;
    debugger->DebugLoad_VAddr<uint32_t>(0xB0000010, crc1);
    duk_push_uint(ctx, crc1);
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerCrc2(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    uint32_t crc2 = 0;
    debugger->DebugLoad_VAddr<uint32_t>(0xB0000014, crc2);
    duk_push_uint(ctx, crc2);
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerName(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    char headerName[0x15] = "";
    for (size_t i = 0; i < 0x14; i++)
    {
        debugger->DebugLoad_VAddr<char>(0xB0000020 + i, headerName[i]);
    }
    duk_push_string(ctx, headerName);
    duk_trim(ctx, -1);
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerMediaFormat(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    uint32_t mediaFormat = 0;
    debugger->DebugLoad_VAddr<uint32_t>(0xB0000038, mediaFormat);
    duk_push_uint(ctx, mediaFormat);
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerId(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    char headerId[3] = "";
    debugger->DebugLoad_VAddr<char>(0xB000003C, headerId[0]);
    debugger->DebugLoad_VAddr<char>(0xB000003D, headerId[1]);
    duk_push_string(ctx, headerId);
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerCountryCode(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    char countryCode[2] = "";
    debugger->DebugLoad_VAddr<char>(0xB000003E, countryCode[0]);
    duk_push_string(ctx, countryCode);
    return 1;
}

duk_ret_t ScriptAPI::js_pj64_romInfo__get_headerVersion(duk_context* ctx)
{
    CDebuggerUI* debugger = GetInstance(ctx)->Debugger();
    uint8_t headerVersion = 0;
    debugger->DebugLoad_VAddr<uint8_t>(0xB000003F, headerVersion);
    duk_push_uint(ctx, headerVersion);
    return 1;
}
