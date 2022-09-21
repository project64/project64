#include "stdafx.h"

#include "JSSocketWorker.h"
#include "ScriptAPI.h"

CJSSocketWorker::CJSSocketWorker(CScriptInstance * inst, void * objectHeapPtr, bool bAllowHalfOpen) :
    CScriptWorker(inst, objectHeapPtr),
    m_Socket(INVALID_SOCKET),
    m_bAllowHalfOpen(bAllowHalfOpen),
    m_bWinsockOK(false)
{
    WSADATA wsaData;
    m_bWinsockOK = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

CJSSocketWorker::~CJSSocketWorker()
{
    StopWorkerProc();

    if (m_bWinsockOK)
    {
        WSACleanup();
    }
}

bool CJSSocketWorker::Init(SOCKET sock)
{
    if (!m_bWinsockOK)
    {
        JSEmitError("WSAStartup() error");
        return false;
    }

    if (m_hThread != nullptr)
    {
        JSEmitError("invalid state - socket is already active");
        return false;
    }

    m_Socket = sock;

    UpdateAddresses();

    return true;
}

bool CJSSocketWorker::Init(const char * host, unsigned short port)
{
    if (!m_bWinsockOK)
    {
        JSEmitError("WSAStartup() error");
        return false;
    }

    if (m_hThread != nullptr)
    {
        JSEmitError("invalid state - socket is already active");
        return false;
    }

    m_Queue.connectHost = host;
    m_Queue.connectPort = port;
    m_Queue.bConnectPending = true;

    return true;
}

bool CJSSocketWorker::Write(const char * data, size_t length, duk_int_t callbackId, bool bEnd)
{
    CGuard guard(m_Queue.cs);

    if (m_Queue.bFullClosePending ||
        m_Queue.bSendClosePending ||
        m_Queue.bSendClosed)
    {
        return false;
    }

    if (bEnd)
    {
        m_Queue.bSendClosePending = true;
    }

    BufferedWrite bufferedWrite;
    bufferedWrite.offset = 0;
    bufferedWrite.callbackId = callbackId;

    if (length != 0)
    {
        bufferedWrite.data.resize(length);
        memcpy(&bufferedWrite.data[0], data, length);
    }

    m_Queue.writes.push_back(bufferedWrite);
    return true;
}

void CJSSocketWorker::WorkerProc()
{
    TIMEVAL timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT_MS * 1000;

    bool bHaveConnection = false;
    bool bConnectPending = false;
    bool bWritesPending = false;
    bool bRecvClosed = false;
    bool bSendClosed = false;

    {
        CGuard guard(m_Queue.cs);
        bConnectPending = m_Queue.bConnectPending;
    }

    if (!bConnectPending)
    {
        // assume it's already connected
        bHaveConnection = true;
    }

    if (bConnectPending && ProcConnect())
    {
        bConnectPending = false;
    }

    while (true)
    {
        if (StopRequested())
        {
            break;
        }

        {
            CGuard guard(m_Queue.cs);
            bWritesPending = m_Queue.writes.size() > 0;
            bRecvClosed = m_Queue.bRecvClosed;
            bSendClosed = m_Queue.bSendClosed;

            if (m_Queue.bFullClosePending && !bWritesPending)
            {
                break;
            }
        }

        if (bRecvClosed && !bWritesPending)
        {
            // nothing to do
            Sleep(TIMEOUT_MS);
            continue;
        }

        int numFds;
        fd_set readFds, writeFds;
        fd_set * pWriteFds = nullptr;
        fd_set * pReadFds = nullptr;

        if (!bRecvClosed)
        {
            pReadFds = &readFds;
            FD_ZERO(pReadFds);
            FD_SET(m_Socket, pReadFds);
        }

        if (bWritesPending || !bHaveConnection)
        {
            pWriteFds = &writeFds;
            FD_ZERO(pWriteFds);
            FD_SET(m_Socket, pWriteFds);
        }

        numFds = select(0, pReadFds, pWriteFds, nullptr, &timeout);
        if (numFds == SOCKET_ERROR)
        {
            JSEmitError("select() error");
            break;
        }

        if (numFds == 0)
        {
            continue;
        }

        bool bWritable = pWriteFds && FD_ISSET(m_Socket, pWriteFds);
        bool bReadable = pReadFds && FD_ISSET(m_Socket, pReadFds);

        if (bWritable)
        {
            if (!bHaveConnection)
            {
                bHaveConnection = true;
                JSEmitConnect();
            }

            if (!bSendClosed && bWritesPending)
            {
                ProcSendData();
            }
        }
        else if (pWriteFds && bHaveConnection)
        {
            JSEmitError("connection reset");
            break;
        }

        if (bReadable)
        {
            ProcRecvData();
        }
    }

    closesocket(m_Socket);
    JSEmitClose();
    ClearAddress();
    ClearQueue();
    m_Socket = INVALID_SOCKET;

    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__UnrefObject);
}

bool CJSSocketWorker::ProcConnect()
{
    stdstr strPort, strHost;

    {
        CGuard guard(m_Queue.cs);
        strPort = stdstr_f("%d", m_Queue.connectPort);
        strHost = m_Queue.connectHost;
        m_Queue.bConnectPending = false;
    }

    struct addrinfo hints, *result, *ptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int rc;
    rc = getaddrinfo(m_Queue.connectHost.c_str(), strPort.c_str(), &hints, &result);

    if (rc != 0)
    {
        JSEmitError("getaddrinfo() error");
        JSEmitLookup(m_RemoteAddress); // port=0, err
        freeaddrinfo(result);
        return false;
    }

    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        m_Socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (m_Socket == INVALID_SOCKET)
        {
            JSEmitError("socket() error");
            freeaddrinfo(result);
            return false;
        }

        rc = connect(m_Socket, ptr->ai_addr, (int)ptr->ai_addrlen);

        if (rc != SOCKET_ERROR)
        {
            ULONG nonBlock = 1;
            rc = ioctlsocket(m_Socket, FIONBIO, &nonBlock);
            if (rc == SOCKET_ERROR)
            {
                JSEmitError("ioctlsocket() FIONBIO error");
                closesocket(m_Socket);
                freeaddrinfo(result);
                return false;
            }

            UpdateAddresses();
            JSEmitLookup(m_RemoteAddress);
            freeaddrinfo(result);
            return true;
        }

        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }

    return false;
}

void CJSSocketWorker::ProcSendData()
{
    CGuard guard(m_Queue.cs);
    BufferedWrite & bufferedWrite = m_Queue.writes.front();

    int avail = bufferedWrite.data.size() - bufferedWrite.offset;
    int numBytesSent = send(m_Socket, &bufferedWrite.data[bufferedWrite.offset], avail, 0);

    if (numBytesSent >= 0)
    {
        bufferedWrite.offset += numBytesSent;
        if (bufferedWrite.offset == bufferedWrite.data.size())
        {
            if (bufferedWrite.callbackId != -1)
            {
                m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js_Socket__invokeWriteCallback,
                                            CbArgs_Write, &bufferedWrite.callbackId, sizeof(bufferedWrite.callbackId));
            }
            m_Queue.writes.erase(m_Queue.writes.begin());
        }

        if (m_Queue.writes.size() == 0)
        {
            JSEmitDrain();
        }
    }
    else
    {
        JSEmitError("send() error");
    }

    if (m_Queue.writes.size() == 0 &&
        m_Queue.bSendClosePending)
    {
        shutdown(m_Socket, SD_SEND);
        m_Queue.bSendClosePending = false;
        m_Queue.bSendClosed = true;

        m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js_Socket__invokeWriteEndCallbacks);
    }
}

void CJSSocketWorker::ProcRecvData()
{
    char recvBuffer[8192];
    int numBytesReceived = recv(m_Socket, recvBuffer, sizeof(recvBuffer), 0);

    if (numBytesReceived > 0)
    {
        JSEmitData(recvBuffer, numBytesReceived);
    }
    else if (numBytesReceived == 0)
    {
        JSEmitEnd();

        CGuard guard(m_Queue.cs);
        if (!m_bAllowHalfOpen)
        {
            m_Queue.bFullClosePending = true;
        }
        m_Queue.bRecvClosed = true;
    }
    else
    {
        JSEmitError("recv() error");
    }
}

void CJSSocketWorker::UpdateAddresses()
{
    CGuard guard(m_CS);

    WSAPROTOCOL_INFO protocolInfo;
    int protocolInfoSize = sizeof(protocolInfo);

    if (getsockopt(m_Socket, SOL_SOCKET, SO_PROTOCOL_INFO, (char *)&protocolInfo, &protocolInfoSize) != 0)
    {
        return;
    }

    int & family = protocolInfo.iAddressFamily;
    sockaddr * pLocalAddr = nullptr;
    sockaddr * pRemoteAddr = nullptr;

    union
    {
        sockaddr_in ipv4;
        sockaddr_in6 ipv6;
    } localAddr;

    union
    {
        sockaddr_in ipv4;
        sockaddr_in6 ipv6;
    } remoteAddr;

    int addrSize = 0;

    if (family == AF_INET)
    {
        pLocalAddr = (sockaddr *)&localAddr.ipv4;
        pRemoteAddr = (sockaddr *)&remoteAddr.ipv4;
        addrSize = sizeof(sockaddr_in);
    }
    else if (family == AF_INET6)
    {
        pLocalAddr = (sockaddr *)&localAddr.ipv6;
        pRemoteAddr = (sockaddr *)&remoteAddr.ipv6;
        addrSize = sizeof(sockaddr_in6);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    getsockname(m_Socket, pLocalAddr, &addrSize);
    getpeername(m_Socket, pRemoteAddr, &addrSize);

    getnameinfo(pLocalAddr, addrSize,
                m_LocalAddress.address, sizeof(m_LocalAddress.address),
                0, 0, NI_NUMERICHOST);

    getnameinfo(pRemoteAddr, addrSize,
                m_RemoteAddress.address, sizeof(m_RemoteAddress.address),
                0, 0, NI_NUMERICHOST);

    if (family == AF_INET)
    {
        m_LocalAddress.port = ntohs(localAddr.ipv4.sin_port);
        m_LocalAddress.family = "IPv4";

        m_RemoteAddress.port = ntohs(remoteAddr.ipv4.sin_port);
        m_RemoteAddress.family = "IPv4";
    }
    else
    {
        m_LocalAddress.port = ntohs(localAddr.ipv6.sin6_port);
        m_LocalAddress.family = "IPv6";

        m_RemoteAddress.port = ntohs(remoteAddr.ipv6.sin6_port);
        m_RemoteAddress.family = "IPv6";
    }
}

void CJSSocketWorker::JSEmitConnect()
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitConnect);
}

void CJSSocketWorker::JSEmitData(const char * data, size_t size)
{
    JSEmitDataEnv env;
    env.data = new char[size];
    env.size = size;

    memcpy(env.data, data, size);

    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitData, (void *)&env, sizeof(env));
}

void CJSSocketWorker::JSEmitEnd()
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitEnd);
}

void CJSSocketWorker::JSEmitClose()
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitClose);
}

void CJSSocketWorker::JSEmitDrain()
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitDrain);
}

void CJSSocketWorker::JSEmitLookup(JSSocketAddrInfo & addr)
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitLookup, (void *)&addr, sizeof(addr));
}

void CJSSocketWorker::JSEmitError(const char * errMessage)
{
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr,
                                ScriptAPI::js__Emitter_emit, CbArgs_EmitError, (void *)errMessage, strlen(errMessage) + 1);
}

duk_idx_t CJSSocketWorker::CbArgs_EmitConnect(duk_context * ctx, void * /*_env*/)
{
    duk_push_string(ctx, "connect");
    return 1;
}

duk_idx_t CJSSocketWorker::CbArgs_EmitData(duk_context * ctx, void * _env)
{
    JSEmitDataEnv * env = (JSEmitDataEnv *)_env;
    duk_push_string(ctx, "data");
    char * buffer = (char *)duk_push_fixed_buffer(ctx, env->size);
    memcpy(buffer, env->data, env->size);

    delete[] env->data;

    duk_push_buffer_object(ctx, -1, 0, env->size, DUK_BUFOBJ_NODEJS_BUFFER);
    duk_remove(ctx, -2);
    return 2;
}

duk_idx_t CJSSocketWorker::CbArgs_EmitEnd(duk_context * ctx, void * /*_env*/)
{
    duk_push_string(ctx, "end");
    return 1;
}

duk_idx_t CJSSocketWorker::CbArgs_EmitClose(duk_context * ctx, void * /*_env*/)
{
    duk_push_string(ctx, "close");
    return 1;
}

duk_idx_t CJSSocketWorker::CbArgs_EmitDrain(duk_context * ctx, void * /*_env*/)
{
    duk_push_string(ctx, "drain");
    return 1;
}

duk_idx_t CJSSocketWorker::CbArgs_EmitLookup(duk_context * ctx, void * _env)
{
    JSSocketAddrInfo * addr = (JSSocketAddrInfo *)_env;

    duk_push_string(ctx, "lookup");
    duk_push_object(ctx);

    if (addr->port != 0)
    {
        duk_push_null(ctx);
    }
    else
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "dns lookup error");
    }

    duk_put_prop_string(ctx, -2, "err");
    duk_push_string(ctx, addr->address);
    duk_put_prop_string(ctx, -2, "address");
    duk_push_uint(ctx, addr->port);
    duk_put_prop_string(ctx, -2, "port");
    duk_push_string(ctx, addr->family);
    duk_put_prop_string(ctx, -2, "family");
    return 2;
}

duk_idx_t CJSSocketWorker::CbArgs_EmitError(duk_context * ctx, void * _env)
{
    const char * errMessage = (const char *)_env;
    duk_push_string(ctx, "error");
    duk_push_error_object(ctx, DUK_ERR_ERROR, errMessage);
    return 2;
}

duk_idx_t CJSSocketWorker::CbArgs_Write(duk_context * ctx, void * _env)
{
    duk_int_t callbackId = *(duk_int_t *)_env;
    duk_push_int(ctx, callbackId);
    return 1;
}

std::string CJSSocketWorker::GetLocalAddress()
{
    CGuard guard(m_CS);
    return m_LocalAddress.address;
}

unsigned short CJSSocketWorker::GetLocalPort()
{
    CGuard guard(m_CS);
    return m_LocalAddress.port;
}

std::string CJSSocketWorker::GetRemoteAddress()
{
    CGuard guard(m_CS);
    return m_RemoteAddress.address;
}

unsigned short CJSSocketWorker::GetRemotePort()
{
    CGuard guard(m_CS);
    return m_RemoteAddress.port;
}

const char * CJSSocketWorker::GetFamily()
{
    CGuard guard(m_CS);
    return m_LocalAddress.family;
}

void CJSSocketWorker::ClearAddress()
{
    CGuard guard(m_CS);

    m_LocalAddress.port = 0;
    memset(m_LocalAddress.address, 0, sizeof(m_LocalAddress.address));
    m_LocalAddress.family = "";

    m_RemoteAddress.port = 0;
    memset(m_RemoteAddress.address, 0, sizeof(m_RemoteAddress.address));
    m_RemoteAddress.family = "";
}

void CJSSocketWorker::ClearQueue()
{
    CGuard guard(m_Queue.cs);

    m_Queue.bSendClosePending = false;
    m_Queue.bSendClosed = false;
    m_Queue.bFullClosePending = false;
    m_Queue.bConnectPending = false;
    m_Queue.connectHost = "";
    m_Queue.connectPort = 0;
    m_Queue.writes.clear();
}
