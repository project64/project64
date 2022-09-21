#include "stdafx.h"

#include "JSServerWorker.h"
#include "JSSocketWorker.h"

CJSServerWorker::CJSServerWorker(CScriptInstance* instance, void* dukObjectHeapPtr) :
    CScriptWorker(instance, dukObjectHeapPtr),
    m_bWinsockOK(false),
    m_ServerSocket(INVALID_SOCKET)
{
    WSADATA wsaData;
    m_bWinsockOK = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

CJSServerWorker::~CJSServerWorker()
{
    StopWorkerProc();

    if (m_bWinsockOK)
    {
        WSACleanup();
    }
}

void CJSServerWorker::Init(const char* address, unsigned short port)
{
    m_Queue.listenAddress = address;
    m_Queue.listenPort = port;
}

void CJSServerWorker::WorkerProc()
{
    int rc;

    union {
        SOCKADDR     service;
        SOCKADDR_IN  service_ipv4;
        SOCKADDR_IN6 service_ipv6;
    };

    if (m_ServerSocket != INVALID_SOCKET)
    {
        return;
    }

    if (inet_pton(AF_INET, m_Queue.listenAddress.c_str(), &service_ipv4.sin_addr) == 1)
    {
        service_ipv4.sin_family = AF_INET;
        service_ipv4.sin_port = htons(m_Queue.listenPort);
    }
    else if (inet_pton(AF_INET6, m_Queue.listenAddress.c_str(), &service_ipv6.sin6_addr) == 1)
    {
        service_ipv6.sin6_family = AF_INET6;
        service_ipv6.sin6_port = htons(m_Queue.listenPort);
    }
    else
    {
        JSEmitError("invalid address");
        goto stop_cleanup;
    }

    m_ServerSocket = socket(service.sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (m_ServerSocket == INVALID_SOCKET)
    {
        JSEmitError("failed to initialize server socket");
        goto stop_cleanup;
    }

    ULONG nonBlock = 1;
    rc = ioctlsocket(m_ServerSocket, FIONBIO, &nonBlock);
    if (rc == SOCKET_ERROR)
    {
        JSEmitError("ioctlsocket() FIONBIO error");
        goto stop_cleanup;
    }

    rc = ::bind(m_ServerSocket, (const SOCKADDR*)&service,
        service.sa_family == AF_INET ? sizeof(service_ipv4) : sizeof(service_ipv6));

    if (rc == SOCKET_ERROR)
    {
        JSEmitError(stdstr_f("bind() error (%u)", WSAGetLastError()).c_str());
        goto stop_cleanup;
    }

    rc = listen(m_ServerSocket, SOMAXCONN);

    if (rc == SOCKET_ERROR)
    {
        JSEmitError("listen() error");
        goto stop_cleanup;
    }

    {
        CGuard guard(m_CS);
        strncpy(m_Address.address, m_Queue.listenAddress.c_str(), sizeof(m_Address.address));
        m_Address.port = m_Queue.listenPort;
        m_Address.family = service.sa_family == AF_INET ? "IPv4" : "IPv6";
    }

    JSEmitListening();

    TIMEVAL timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT_MS * 1000;

    while (true)
    {
        if (StopRequested())
        {
            goto stop_cleanup;
        }

        {
            CGuard guard(m_Queue.cs);
            if (m_Queue.bClosePending)
            {
                goto stop_cleanup;
            }
        }

        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(m_ServerSocket, &readFds);

        int numFds = select(0, &readFds, nullptr, nullptr, &timeout);

        if (numFds == 0)
        {
            continue;
        }

        if (numFds == SOCKET_ERROR)
        {
            goto stop_cleanup;
        }

        if (numFds > 0 && FD_ISSET(m_ServerSocket, &readFds))
        {
            SOCKET clientSocket = accept(m_ServerSocket, nullptr, nullptr);
            JSEmitConnection(clientSocket);
        }
    }

stop_cleanup:
    {
        CGuard guard(m_CS);
        strncpy(m_Address.address, "", sizeof(m_Address.address));
        m_Address.port = 0;
        m_Address.family = "";
    }

    if (m_ServerSocket != INVALID_SOCKET)
    {
        closesocket(m_ServerSocket);
        m_ServerSocket = INVALID_SOCKET;
        JSEmitClose();
    }

    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__UnrefObject);
}

std::string CJSServerWorker::GetAddress()
{
    CGuard guard(m_CS);
    return m_Address.address;
}

unsigned short CJSServerWorker::GetPort()
{
    CGuard guard(m_CS);
    return m_Address.port;
}

const char * CJSServerWorker::GetFamily()
{
    CGuard guard(m_CS);
    return m_Address.family;
}

void CJSServerWorker::JSEmitConnection(SOCKET c)
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__Emitter_emit,
                                CbArgs_EmitConnection, (void *)&c, sizeof(c));
}

void CJSServerWorker::JSEmitClose()
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__Emitter_emit,
                                CbArgs_EmitClose);
}

void CJSServerWorker::JSEmitListening()
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__Emitter_emit,
                                CbArgs_EmitListening);
}

void CJSServerWorker::JSEmitError(const char * errMessage)
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__Emitter_emit,
                                CbArgs_EmitError, (void *)errMessage, strlen(errMessage) + 1);
}

duk_idx_t CJSServerWorker::CbArgs_EmitConnection(duk_context * ctx, void * _env)
{
    duk_push_string(ctx, "connection");

    SOCKET client = *(SOCKET *)_env;
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "Socket");
    duk_remove(ctx, -2);
    duk_pnew(ctx, 0);

    ScriptAPI::RefObject(ctx, -1);

    duk_get_prop_string(ctx, -1, HS_socketWorkerPtr);
    CJSSocketWorker * socketWorker = (CJSSocketWorker *)duk_get_pointer(ctx, -1);
    duk_pop(ctx);

    socketWorker->Init(client);
    socketWorker->StartWorkerProc();

    return 2;
}

duk_idx_t CJSServerWorker::CbArgs_EmitClose(duk_context * ctx, void * /*_env*/)
{
    duk_push_string(ctx, "close");
    return 1;
}

duk_idx_t CJSServerWorker::CbArgs_EmitListening(duk_context * ctx, void * /*_env*/)
{
    duk_push_string(ctx, "listening");
    return 1;
}

duk_idx_t CJSServerWorker::CbArgs_EmitError(duk_context * ctx, void * _env)
{
    const char * errMessage = (const char *)_env;
    duk_push_string(ctx, "error");
    duk_push_error_object(ctx, DUK_ERR_ERROR, errMessage);
    return 2;
}
