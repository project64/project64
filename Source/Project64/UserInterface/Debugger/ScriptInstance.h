/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include "stdafx.h"
#include <3rdParty/duktape/duktape.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

class CScriptSystem;

typedef enum {
    STATE_STARTED, // initial evaluation & execution
    STATE_RUNNING, // event loop running with pending events
    STATE_STOPPED,  // no pending events
    STATE_INVALID
} INSTANCE_STATE;

class CScriptInstance
{
    typedef enum {
        EVENT_READ,
        EVENT_WRITE,
        EVENT_ACCEPT,
        EVENT_CONNECT
    } IOEVENTTYPE;

    typedef struct {
        OVERLAPPED  ovl;
        IOEVENTTYPE eventType;
        HANDLE      fd;
        HANDLE      childFd; // accepted socket
        bool        bSocket;
        UINT        id;
        void*       data;
        DWORD       dataLen; // changed to bytes transferred after event is fired
        void*       callback;
    } IOLISTENER;

    // Wrapper for file/socket descriptor and completion port
    typedef struct {
        HANDLE fd;
        HANDLE iocp;
        bool bSocket;
    } IOFD;

    typedef enum {
        EVENT_STATUS_OK,
        EVENT_STATUS_INTERRUPTED,
        EVENT_STATUS_ERROR
    } EVENT_STATUS;

    // For synchronous file operations
    typedef struct
    {
        FILE* fp;
        int fd;
    } FILE_FD;

    typedef BOOL(__stdcall *Dynamic_CancelIoEx)(HANDLE, LPOVERLAPPED);
public:

    CScriptInstance(CDebuggerUI* debugger);
    ~CScriptInstance();

    void Start(char* path);
    void ForceStop();
    void Invoke(void* heapptr, uint32_t param = 0);
    void Invoke2(void* heapptr, uint32_t param = 0, uint32_t param2 = 0);
    INSTANCE_STATE GetState();

    friend class PendingEval;
    const char* Eval(const char* jsCode);

private:
    duk_context*        m_Ctx;
    duk_context*        DukContext();
    char*               m_TempPath;

    HANDLE              m_hThread;
    HANDLE              m_hIOCompletionPort;
    CriticalSection     m_CS;

    vector<IOFD>        m_AsyncFiles;
    vector<IOLISTENER*> m_Listeners;
    UINT                m_NextListenerId;

    vector<FILE_FD>     m_Files;

    CDebuggerUI*        m_Debugger;

    CScriptSystem*      m_ScriptSystem;

    INSTANCE_STATE      m_State;

    static DWORD CALLBACK StartThread(CScriptInstance* _this);
    void StartScriptProc();
    void StartEventLoop();
    bool HaveEvents();
    EVENT_STATUS WaitForEvent(IOLISTENER** lpListener);

    void SetState(INSTANCE_STATE state);
    void StateChanged();
    void CleanUp();

    void QueueAPC(PAPCFUNC userProc, ULONG_PTR param = 0);

    void AddAsyncFile(HANDLE fd, bool bSocket = false);
    void CloseAsyncFile(HANDLE fd);
    void CloseAllAsyncFiles();
    void RemoveAsyncFile(HANDLE fd);
    HANDLE CreateSocket();

    IOLISTENER* AddListener(HANDLE fd, IOEVENTTYPE evt, void* jsCallback, void* data = NULL, int dataLen = 0);
    void RemoveListener(IOLISTENER* lpListener);
    void RemoveListenerByIndex(UINT index);
    void RemoveListenersByFd(HANDLE fd);
    void InvokeListenerCallback(IOLISTENER* lpListener);

    //static void CALLBACK EvalAsyncCallback(ULONG_PTR evalWait);

    bool AddFile(const char* path, const char* mode, int* fd); // return fd
    void CloseFile(int fd);
    FILE* GetFilePointer(int fd);
    void CloseAllFiles();

    const char* EvalFile(const char* jsPath);

    // Handle to to dynamically load CancelIoEx for Windows XP compatibility
    HMODULE m_hKernel;
    Dynamic_CancelIoEx m_CancelIoEx;

    // Lookup list of CScriptInstance instances for static js_* functions
    static vector<CScriptInstance*> Cache;
    static void CacheInstance(CScriptInstance* _this);
    static void UncacheInstance(CScriptInstance* _this);
    static CScriptInstance* FetchInstance(duk_context* ctx);

    // Bound functions (_native object)
    static duk_ret_t js_ioSockCreate(duk_context*);
    static duk_ret_t js_ioSockListen(duk_context*);
    static duk_ret_t js_ioSockAccept(duk_context*); // async
    static duk_ret_t js_ioSockConnect(duk_context*); // async
    static duk_ret_t js_ioRead(duk_context*); // async
    static duk_ret_t js_ioWrite(duk_context*); // async
    static duk_ret_t js_ioClose(duk_context*); // (fd) ; file or socket
    static duk_ret_t js_MsgBox(duk_context*); // (message, caption)
    static duk_ret_t js_AddCallback(duk_context*); // (hookId, callback, tag) ; external events
    static duk_ret_t js_RemoveCallback(duk_context*); // (callbackId)
    static duk_ret_t js_GetPCVal(duk_context*); // ()
    static duk_ret_t js_SetPCVal(duk_context*); // (value)
    static duk_ret_t js_GetHIVal(duk_context*); // (bUpper)
    static duk_ret_t js_SetHIVal(duk_context*); // (bUpper, value)
    static duk_ret_t js_GetLOVal(duk_context*); // (bUpper)
    static duk_ret_t js_SetLOVal(duk_context*); // (bUpper, value)
    static duk_ret_t js_GetGPRVal(duk_context*); // (regNum, bUpper)
    static duk_ret_t js_SetGPRVal(duk_context*); // (regNum, bUpper, value)
    static duk_ret_t js_GetFPRVal(duk_context*); // (regNum, bDouble)
    static duk_ret_t js_SetFPRVal(duk_context*); // (regNum, bDouble, value)
    static duk_ret_t js_GetCauseVal(duk_context*); // ()
    static duk_ret_t js_SetCauseVal(duk_context*); // (value)
    static duk_ret_t js_GetROMInt(duk_context*); // (address, bitwidth, signed)
    static duk_ret_t js_GetROMFloat(duk_context*); // (address, bDouble)
    static duk_ret_t js_GetROMBlock(duk_context*); // (address, nBytes) ; returns Buffer
    static duk_ret_t js_GetROMString(duk_context*); // (address[, maxLen]) ; fetch zero terminated string from memory

    static duk_ret_t js_GetRDRAMInt(duk_context*); // (address, bitwidth, signed)
    static duk_ret_t js_SetRDRAMInt(duk_context*); // (address, bitwidth, signed, newValue)
    static duk_ret_t js_GetRDRAMFloat(duk_context*); // (address, bDouble)
    static duk_ret_t js_SetRDRAMFloat(duk_context*); // (address, bDouble, newValue)
    static duk_ret_t js_GetRDRAMBlock(duk_context*); // (address, nBytes) ; returns Buffer
    static duk_ret_t js_GetRDRAMString(duk_context*); // (address[, maxLen]) ; fetch zero terminated string from memory
    static duk_ret_t js_ConsolePrint(duk_context*);
    static duk_ret_t js_ConsoleClear(duk_context*);

    static duk_ret_t js_BreakHere(duk_context*);
    static duk_ret_t js_Pause(duk_context*); // () ; pauses emulation
    static duk_ret_t js_ShowCommands(duk_context*); // ([address]) ; shows commands window

    static duk_ret_t js_ScreenPrint(duk_context*); // (x, y, text)

    static duk_ret_t js_FSOpen(duk_context*); // (path, flags) ; returns fd
    static duk_ret_t js_FSClose(duk_context*); // (fd)
    static duk_ret_t js_FSWrite(duk_context*); // (fd, buffer[, offset[, length[, position]]])
    static duk_ret_t js_FSRead(duk_context*); // (fd, buffer, offset, length, position)
    static duk_ret_t js_FSFStat(duk_context*); // (fd)
    static duk_ret_t js_FSStat(duk_context*); // (path)
    static duk_ret_t js_FSMkDir(duk_context*); // (path)
    static duk_ret_t js_FSRmDir(duk_context*); // (path)
    static duk_ret_t js_FSUnlink(duk_context*); // (path)
    static duk_ret_t js_FSReadDir(duk_context*); // (path)

    static constexpr duk_function_list_entry NativeFunctions[] =
    {
        { "addCallback",    js_AddCallback,    DUK_VARARGS },
        { "removeCallback", js_RemoveCallback, DUK_VARARGS },

        { "setPCVal",       js_SetPCVal,       DUK_VARARGS },
        { "getPCVal",       js_GetPCVal,       DUK_VARARGS },
        { "setHIVal",       js_SetHIVal,       DUK_VARARGS },
        { "getHIVal",       js_GetHIVal,       DUK_VARARGS },
        { "setLOVal",       js_SetLOVal,       DUK_VARARGS },
        { "getLOVal",       js_GetLOVal,       DUK_VARARGS },
        { "setGPRVal",      js_SetGPRVal,      DUK_VARARGS },
        { "getGPRVal",      js_GetGPRVal,      DUK_VARARGS },
        { "setFPRVal",      js_SetFPRVal,      DUK_VARARGS },
        { "getFPRVal",      js_GetFPRVal,      DUK_VARARGS },
        { "setCauseVal",    js_SetCauseVal,    DUK_VARARGS },
        { "getCauseVal",    js_GetCauseVal,    DUK_VARARGS },

        { "getROMInt",      js_GetROMInt,      DUK_VARARGS },
        { "getROMFloat",    js_GetROMFloat,    DUK_VARARGS },
        { "getROMString",   js_GetROMString,   DUK_VARARGS },
        { "getROMBlock",    js_GetROMBlock,    DUK_VARARGS },

        { "getRDRAMInt",    js_GetRDRAMInt,    DUK_VARARGS },
        { "setRDRAMInt",    js_SetRDRAMInt,    DUK_VARARGS },
        { "getRDRAMFloat",  js_GetRDRAMFloat,  DUK_VARARGS },
        { "setRDRAMFloat",  js_SetRDRAMFloat,  DUK_VARARGS },
        { "getRDRAMBlock",  js_GetRDRAMBlock,  DUK_VARARGS },
        { "getRDRAMString", js_GetRDRAMString, DUK_VARARGS },

        { "sockCreate",     js_ioSockCreate,   DUK_VARARGS },
        { "sockListen",     js_ioSockListen,   DUK_VARARGS },
        { "sockAccept",     js_ioSockAccept,   DUK_VARARGS },
        { "sockConnect",    js_ioSockConnect,  DUK_VARARGS },
        { "close",          js_ioClose,        DUK_VARARGS },
        { "write",          js_ioWrite,        DUK_VARARGS },
        { "read",           js_ioRead,         DUK_VARARGS },
        { "msgBox",         js_MsgBox,         DUK_VARARGS },
        { "consolePrint",   js_ConsolePrint,   DUK_VARARGS },
        { "consoleClear",   js_ConsoleClear,   DUK_VARARGS },
        { "pause",          js_Pause,          DUK_VARARGS },
        { "showCommands",   js_ShowCommands,   DUK_VARARGS },

        { "breakHere",      js_BreakHere,      DUK_VARARGS },

        { "screenPrint",    js_ScreenPrint,    DUK_VARARGS },

        { "fsOpen",         js_FSOpen,         DUK_VARARGS },
        { "fsClose",        js_FSClose,        DUK_VARARGS },
        { "fsWrite",        js_FSWrite,        DUK_VARARGS },
        { "fsRead",         js_FSRead,         DUK_VARARGS },
        { "fsFStat",        js_FSFStat,        DUK_VARARGS },
        { "fsStat",         js_FSStat,         DUK_VARARGS },
        { "fsUnlink",       js_FSUnlink,       DUK_VARARGS },
        { "fsMkDir",        js_FSMkDir,        DUK_VARARGS },
        { "fsRmDir",        js_FSRmDir,        DUK_VARARGS },
        { "fsReadDir",      js_FSReadDir,      DUK_VARARGS },
        { NULL, NULL, 0 }
    };
};