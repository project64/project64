#pragma once

#include "ScriptAPI.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

class CJSServerWorker : public CScriptWorker
{
private:
    enum
    {
        TIMEOUT_MS = 1
    };

    struct ServerQueue
    {
        CriticalSection cs;
        std::string listenAddress;
        unsigned short listenPort;
        bool bClosePending;
        ServerQueue() :
            listenAddress(""), listenPort(0), bClosePending(false)
        {
        }
    } m_Queue;

    struct JSServerAddrInfo
    {
        char address[INET6_ADDRSTRLEN];
        const char * family;
        unsigned short port;

        JSServerAddrInfo() :
            address(""),
            family(""),
            port(0)
        {
        }
    };

    bool m_bWinsockOK;
    SOCKET m_ServerSocket;
    JSServerAddrInfo m_Address;

public:
    CJSServerWorker(CScriptInstance * instance, void * dukObjectHeapPtr);
    virtual ~CJSServerWorker();

    void Init(const char * address, unsigned short port);
    void WorkerProc();

    std::string GetAddress();
    unsigned short GetPort();
    const char * GetFamily();

private:
    void JSEmitConnection(SOCKET c);
    void JSEmitClose();
    void JSEmitListening();
    void JSEmitError(const char * errMessage);

    static duk_idx_t CbArgs_EmitConnection(duk_context * ctx, void * _env);
    static duk_idx_t CbArgs_EmitClose(duk_context * ctx, void * _env);
    static duk_idx_t CbArgs_EmitListening(duk_context * ctx, void * _env);
    static duk_idx_t CbArgs_EmitError(duk_context * ctx, void * _env);
};