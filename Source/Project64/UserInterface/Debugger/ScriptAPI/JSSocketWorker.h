#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "ScriptAPI.h"
#include "../ScriptWorker.h"
#pragma comment (lib, "Ws2_32.lib")
#pragma once

class CJSSocketWorker : public CScriptWorker
{
private:
    enum { TIMEOUT_MS = 1 };

    struct JSSocketAddrInfo
    {
        char address[INET6_ADDRSTRLEN];
        const char* family;
        unsigned short port;

        JSSocketAddrInfo() :
            address(""),
            family(""),
            port(0)
        {
        }
    };

    struct BufferedWrite
    {
        size_t offset;
        std::vector<char> data;
        duk_int_t callbackId;
    };

    struct JSEmitDataEnv {
        char* data;
        size_t size;
    };

    struct JSSocketQueue
    {
        CriticalSection cs;

        bool bConnectPending;
        bool bFullClosePending;
        bool bSendClosePending;
        bool bSendClosed;
        bool bRecvClosed;

        std::string connectHost;
        unsigned short connectPort;
        std::vector<BufferedWrite> writes;

        JSSocketQueue() :
            bConnectPending(false),
            bFullClosePending(false),
            bSendClosePending(false),
            bSendClosed(false),
            bRecvClosed(false),
            connectHost(""),
            connectPort(0)
        {
        }
    };

    SOCKET           m_Socket;
    bool             m_bWinsockOK;
    bool             m_bAllowHalfOpen;
    JSSocketAddrInfo m_LocalAddress;
    JSSocketAddrInfo m_RemoteAddress;
    JSSocketQueue    m_Queue;
    
public:
    CJSSocketWorker(CScriptInstance* inst, void* objectHeapPtr, bool bAllowHalfOpen);
    virtual ~CJSSocketWorker();

    bool Init(SOCKET sock);
    bool Init(const char* host, unsigned short port);

    void WorkerProc();

    bool Write(const char* data, size_t length, duk_int_t callbackId, bool bEnd = false);
    //bool GetAddress(JSSocketAddrInfo& address);
    std::string GetLocalAddress();
    unsigned short GetLocalPort();
    std::string GetRemoteAddress();
    unsigned short GetRemotePort();
    const char* GetFamily();

private:
    bool ProcConnect();
    void ProcSendData();
    void ProcRecvData();

    void UpdateAddresses();
    void ClearAddress();
    void ClearQueue();

    void JSEmitConnect();
    void JSEmitData(const char* data, size_t size);
    void JSEmitEnd();
    void JSEmitClose();
    void JSEmitDrain();
    void JSEmitLookup(JSSocketAddrInfo& addr);
    void JSEmitError(const char* errMessage);

    static duk_idx_t CbArgs_EmitConnect(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_EmitData(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_EmitEnd(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_EmitClose(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_EmitDrain(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_EmitLookup(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_EmitError(duk_context* ctx, void* _env);
    static duk_idx_t CbArgs_Write(duk_context* ctx, void* _env);
};
