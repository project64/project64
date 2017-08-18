#include <stdafx.h>

#include "ScriptInstance.h"
#include "ScriptSystem.h"
#include "ScriptHook.h"
#include "Breakpoints.h"

static BOOL ConnectEx(SOCKET s, const SOCKADDR* name, int namelen, PVOID lpSendBuffer,
	DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped);

vector<CScriptInstance*> CScriptInstance::Cache;

void CScriptInstance::CacheInstance(CScriptInstance* _this)
{
	Cache.push_back(_this);
}

void CScriptInstance::UncacheInstance(CScriptInstance* _this)
{
	for (int i = 0; i < Cache.size(); i++)
	{
		if (Cache[i] == _this)
		{
			Cache.erase(Cache.begin() + i);
			break;
		}
	}
}

CScriptInstance* CScriptInstance::FetchInstance(duk_context* ctx)
{
	for (int i = 0; i < Cache.size(); i++)
	{
		if (Cache[i]->DukContext() == ctx)
		{
			return Cache[i];
		}
	}
}

CScriptInstance::CScriptInstance(CDebuggerUI* debugger)
{
	m_Debugger = debugger;
	m_Ctx = duk_create_heap_default();
	m_ScriptSystem = m_Debugger->ScriptSystem();
	m_NextListenerId = 0;
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	InitializeCriticalSection(&m_CriticalSection);
	CacheInstance(this);
}

CScriptInstance::~CScriptInstance()
{
	UncacheInstance(this);
	//TerminateThread(m_hThread, 0);
	//CloseHandle(m_hThread);
	DeleteCriticalSection(&m_CriticalSection);
	duk_destroy_heap(m_Ctx);
}

void CScriptInstance::Start(char* path)
{
	m_TempPath = path;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartThread, this, 0, NULL);
}

void CScriptInstance::ForceStop()
{
	// Close all files and delete all hooked callbacks
	EnterCriticalSection(&m_CriticalSection);
	m_ScriptSystem->ClearCallbacksForInstance(this);
	CloseAllFiles();
	LeaveCriticalSection(&m_CriticalSection);
	SetState(STATE_STOPPED);
}

duk_context* CScriptInstance::DukContext()
{
	return m_Ctx;
}

INSTANCE_STATE CScriptInstance::GetState()
{
	return m_State;
}

void CScriptInstance::SetState(INSTANCE_STATE state)
{
	m_State = state;
	StateChanged();
}

void CScriptInstance::StateChanged()
{
	// todo mutex might be needed here
	m_Debugger->Debug_RefreshScriptsWindow();
	m_ScriptSystem->DeleteStoppedInstances();
}

DWORD CALLBACK CScriptInstance::StartThread(CScriptInstance* _this)
{
	_this->StartScriptProc();
	return 0;
}

void CScriptInstance::StartScriptProc()
{
	m_hThread = GetCurrentThread();
	SetState(STATE_STARTED);
	
	bool bWasUnpaused = false;

	if (!g_Settings->LoadBool(GameRunning_CPU_Paused) && g_MMU)
	{
		// Pause CPU during startup phase
		bWasUnpaused = true;
		g_System->ExternalEvent(SysEvent_PauseCPU_AppLostFocus);
	}

	duk_context* ctx = m_Ctx;

	duk_push_object(ctx);
	duk_put_global_string(ctx, "_native");
	duk_get_global_string(ctx, "_native");
	duk_put_function_list(ctx, -1, NativeFunctions);
	duk_pop(ctx);
	
	const char* apiScript = m_ScriptSystem->APIScript();

	duk_int_t apiresult = duk_peval_string(ctx, apiScript);

	if (apiresult != 0)
	{
		MessageBox(NULL, duk_safe_to_string(ctx, -1), "API Script Error", MB_OK | MB_ICONERROR);
		return;
	}

	if (m_TempPath)
	{
		stdstr fullPath = stdstr_f("Scripts/%s", m_TempPath);
		duk_int_t scriptresult = duk_peval_file(ctx, fullPath.c_str());
		m_TempPath = NULL;

		if (scriptresult != 0)
		{
			const char* errorText = duk_safe_to_string(ctx, -1);
			//MessageBox(NULL, duk_safe_to_string(ctx, -1), "Script error", MB_OK | MB_ICONWARNING);
			m_Debugger->Debug_LogScriptsWindow(errorText);
			m_Debugger->Debug_LogScriptsWindow("\r\n");
			SetState(STATE_STOPPED);
			return;
		}
	}

	if (bWasUnpaused)
	{
		g_System->ExternalEvent(SysEvent_ResumeCPU_AppGainedFocus);
	}

	if (HaveEvents())
	{
		StartEventLoop();
	}
	else
	{
		SetState(STATE_STOPPED);
	}
}

void CScriptInstance::StartEventLoop()
{
	SetState(STATE_RUNNING);

	// Todo interrupt with an apc when an event is removed and event count is 0
	while (HaveEvents())
	{
		IOLISTENER* lpListener;
		EVENT_STATUS status = WaitForEvent(&lpListener);

		if (status == EVENT_STATUS_INTERRUPTED)
		{
			continue;
		}

		if (status == EVENT_STATUS_ERROR)
		{
			break;
		}

		InvokeListenerCallback(lpListener);
		RemoveListener(lpListener);
	}

	SetState(STATE_STOPPED);
}

CScriptInstance::EVENT_STATUS
CScriptInstance::WaitForEvent(IOLISTENER** lpListener)
{
	DWORD nBytesTransferred;
	ULONG_PTR completionKey;
	OVERLAPPED* lpUsedOverlap;

	BOOL status = GetQueuedCompletionStatus(
		m_hIOCompletionPort,
		&nBytesTransferred,
		&completionKey,
		&lpUsedOverlap,
		INFINITE
	);

	if (!status)
	{
		*lpListener = NULL;
		DWORD errorCode = GetLastError();
		if (errorCode == STATUS_USER_APC)
		{
			return EVENT_STATUS_INTERRUPTED;
		}
		return EVENT_STATUS_ERROR;
	}

	*lpListener = (IOLISTENER*) lpUsedOverlap;

	(*lpListener)->dataLen = nBytesTransferred;

	return EVENT_STATUS_OK;
}

bool CScriptInstance::HaveEvents()
{
	return
		(m_Listeners.size() > 0) ||
		m_ScriptSystem->HasCallbacksForInstance(this);
}

void CScriptInstance::AddFile(HANDLE fd, bool bSocket)
{
	IOFD iofd;
	iofd.fd = fd;
	iofd.iocp = CreateIoCompletionPort(fd, m_hIOCompletionPort, (ULONG_PTR)fd, 0);
	iofd.bSocket = bSocket;
	m_Files.push_back(iofd);
}

void CScriptInstance::CloseFile(HANDLE fd)
{
	// Causes EVT_READ with length 0
	// Not safe to remove listeners until then

	for (uint32_t i = 0; i < m_Files.size(); i++)
	{
		IOFD iofd = m_Files[i];
		if (iofd.fd != fd)
		{
			continue;
		}

		// Close file handle
		if (iofd.bSocket)
		{
			closesocket((SOCKET)iofd.fd);
		}
		else
		{
			CloseHandle(iofd.fd);
		}
		break;
	}
}

void CScriptInstance::CloseAllFiles()
{
	// Causes EVT_READ with length 0
	// Not safe to remove listeners until then

	for (uint32_t i = 0; i < m_Files.size(); i++)
	{
		IOFD iofd = m_Files[i];

		// Close file handle
		if (iofd.bSocket)
		{
			closesocket((SOCKET)iofd.fd);
		}
		else
		{
			CloseHandle(iofd.fd);
		}
	}
}

void CScriptInstance::RemoveFile(HANDLE fd)
{
	// Stop tracking an fd and remove all of its listeners
	for (uint32_t i = 0; i < m_Files.size(); i++)
	{
		IOFD iofd = m_Files[i];
		if (iofd.fd != fd)
		{
			continue;
		}

		m_Files.erase(m_Files.begin() + i);
		RemoveListenersByFd(fd);
		break;
	}
}

HANDLE CScriptInstance::CreateSocket()
{
	HANDLE fd = (HANDLE)WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	AddFile(fd, true);
	return fd;
}

CScriptInstance::IOLISTENER*
CScriptInstance::AddListener(HANDLE fd, IOEVENTTYPE evt, void* callback, void* data, int dataLen)
{
	IOLISTENER* lpListener = (IOLISTENER*) malloc(sizeof(IOLISTENER));
	memset(lpListener, 0x00, sizeof(IOLISTENER));

	lpListener->id = m_NextListenerId++;
	lpListener->eventType = evt;
	lpListener->fd = fd;
	lpListener->callback = callback;
	lpListener->data = data;
	lpListener->dataLen = dataLen;

	m_Listeners.push_back(lpListener);

	return lpListener;
}

void CScriptInstance::RemoveListenerByIndex(UINT index)
{
	IOLISTENER* lpListener = m_Listeners[index];

	if (lpListener->data != NULL)
	{
		free(lpListener->data);
	}

	CancelIoEx(lpListener->fd, (LPOVERLAPPED)lpListener);

	free(lpListener);

	m_Listeners.erase(m_Listeners.begin() + index);
}

// Free listener & its buffer, remove from list
void CScriptInstance::RemoveListener(IOLISTENER* lpListener)
{
	for (UINT i = 0; i < m_Listeners.size(); i++)
	{
		if (m_Listeners[i] == lpListener)
		{
			RemoveListenerByIndex(i);
			break;
		}
	}
}

void CScriptInstance::RemoveListenersByFd(HANDLE fd)
{
	for (UINT i = 0; i < m_Listeners.size(); i++)
	{
		if (m_Listeners[i]->fd == fd)
		{
			RemoveListenerByIndex(i);
		}
	}
}

void CScriptInstance::InvokeListenerCallback(IOLISTENER* lpListener)
{
	if (lpListener->callback == NULL)
	{
		return;
	}

	EnterCriticalSection(&m_CriticalSection);

	duk_push_heapptr(m_Ctx, lpListener->callback);

	int nargs = 0;

	switch (lpListener->eventType)
	{
	case EVENT_READ:
		nargs = 1;
		if (lpListener->dataLen > 0)
		{
			void* data = duk_push_buffer(m_Ctx, lpListener->dataLen, false);
			memcpy(data, lpListener->data, lpListener->dataLen);
		}
		else
		{
			// handle must have closed, safe to untrack fd and remove all associated listeners
			RemoveFile(lpListener->fd);

			// pass null to callback
			duk_push_null(m_Ctx);
		}
		break;
	case EVENT_WRITE:
		nargs = 1;
		duk_push_uint(m_Ctx, lpListener->dataLen); // num bytes written
		break;
	case EVENT_ACCEPT:
		// pass client socket fd to callback
		nargs = 1;
		duk_push_uint(m_Ctx, (UINT)lpListener->childFd);
		break;
	case EVENT_CONNECT:
		nargs = 0;
		break;
	}

	duk_int_t status = duk_pcall(m_Ctx, nargs);

	if (status != DUK_EXEC_SUCCESS)
	{
		const char* msg = duk_safe_to_string(m_Ctx, -1);
		MessageBox(NULL, msg, "Script error", MB_OK | MB_ICONWARNING);
	}

	LeaveCriticalSection(&m_CriticalSection);
}

const char* CScriptInstance::Eval(const char* jsCode)
{
	int result = duk_peval_string(m_Ctx, jsCode);
	const char* msg = duk_safe_to_string(m_Ctx, -1);
	if (result != 0)
	{
		MessageBox(NULL, msg, "Script error", MB_OK | MB_ICONWARNING);
	}
	duk_pop(m_Ctx);
	return msg;
}

class PendingEval {
	char* m_CodeCopy;
	CScriptInstance* m_ScriptInstance;
public:
	PendingEval(CScriptInstance* instance, const char* jsCode)
	{
		m_CodeCopy = (char*)malloc(strlen(jsCode));
		m_ScriptInstance = instance;
		MessageBox(NULL, "pending eval created", "", MB_OK);
	}
	~PendingEval()
	{
		MessageBox(NULL, "pending eval deleted", "", MB_OK);
		free(m_CodeCopy);
	}
	void Run()
	{
		MessageBox(NULL, "pending eval invoked", "", MB_OK);
		m_ScriptInstance->Eval(m_CodeCopy);
	}
};

void CScriptInstance::EvalAsync(const char* jsCode)
{
	PendingEval* pendingEval = new PendingEval(this, jsCode);
	QueueAPC(EvalAsyncCallback, (ULONG_PTR)pendingEval);
}

void CALLBACK CScriptInstance::EvalAsyncCallback(ULONG_PTR _pendingEval)
{
	PendingEval* evalWait = (PendingEval*)_pendingEval;
	evalWait->Run();
	delete evalWait;
}

const char* CScriptInstance::EvalFile(const char* jsPath)
{
	int result = duk_peval_file(m_Ctx, jsPath);
	const char* msg = duk_safe_to_string(m_Ctx, -1);
	if (result != 0)
	{
		MessageBox(NULL, msg, jsPath, MB_OK | MB_ICONWARNING);
	}
	duk_pop(m_Ctx);
	return msg;
}

void CScriptInstance::Invoke(void* heapptr, uint32_t param)
{
	EnterCriticalSection(&m_CriticalSection);
	duk_push_heapptr(m_Ctx, heapptr);
	duk_push_uint(m_Ctx, param);
	
	duk_int_t status = duk_pcall(m_Ctx, 1);

	if (status != DUK_EXEC_SUCCESS)
	{
		const char* errorText = duk_safe_to_string(m_Ctx, -1);
		m_Debugger->Debug_LogScriptsWindow(errorText);
		m_Debugger->Debug_LogScriptsWindow("\r\n");
	}

	duk_pop(m_Ctx);
	LeaveCriticalSection(&m_CriticalSection);
}

void CScriptInstance::QueueAPC(PAPCFUNC userProc, ULONG_PTR param)
{
	if (m_hThread != NULL)
	{
		MessageBox(NULL, "apc queued", "", MB_OK);
		QueueUserAPC(userProc, m_hThread, param);
	}
}

/****************************/

duk_ret_t CScriptInstance::js_ioSockConnect(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	HANDLE fd = (HANDLE)duk_get_uint(ctx, 0);
	const char* ipStr = duk_to_string(ctx, 1);
	USHORT port = (USHORT)duk_get_uint(ctx, 2);
	void* callback = duk_get_heapptr(ctx, 3);

	char ipBytes[sizeof(uint32_t)];
	sscanf(ipStr, "%hhu.%hhu.%hhu.%hhu", &ipBytes[0], &ipBytes[1], &ipBytes[2], &ipBytes[3]);

	SOCKADDR_IN serverAddr;
	serverAddr.sin_addr.s_addr = *(uint32_t*)ipBytes;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_family = AF_INET;

	SOCKADDR_IN clientAddr;
	clientAddr.sin_addr.s_addr = INADDR_ANY;
	clientAddr.sin_port = 0;
	clientAddr.sin_family = AF_INET;

	::bind((SOCKET)fd, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR));

	IOLISTENER* lpListener = _this->AddListener(fd, EVENT_CONNECT, callback);
	ConnectEx((SOCKET)fd, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR), NULL, 0, NULL, (LPOVERLAPPED)lpListener);

	duk_pop_n(ctx, 4);
	return 1;
}

duk_ret_t CScriptInstance::js_ioClose(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);
	HANDLE fd = (HANDLE)duk_get_uint(ctx, 0);
	duk_pop(ctx);
	_this->CloseFile(fd);
	return 1;
}

duk_ret_t CScriptInstance::js_ioSockCreate(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	HANDLE fd = _this->CreateSocket();
	duk_pop_n(ctx, duk_get_top(ctx));
	duk_push_int(ctx, (int)fd);
	return 1;
}

duk_ret_t CScriptInstance::js_ioSockListen(duk_context* ctx)
{
	HANDLE fd = (HANDLE)duk_get_uint(ctx, 0);
	USHORT port = (USHORT)duk_get_uint(ctx, 1);

	duk_pop_n(ctx, 2);

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (::bind((SOCKET)fd, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		duk_push_boolean(ctx, false);
		return 1;
	}

	bool listenOkay = listen((SOCKET)fd, 3) == 0;
	duk_push_boolean(ctx, listenOkay);

	return 1;
}

duk_ret_t CScriptInstance::js_ioSockAccept(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	HANDLE fd = (HANDLE)duk_get_uint(ctx, 0);
	void* jsCallback = duk_get_heapptr(ctx, 1);

	void* data = malloc(sizeof(SOCKADDR) * 4); // issue?

	IOLISTENER* lpListener = _this->AddListener(fd, EVENT_ACCEPT, jsCallback, data, 0);

	lpListener->childFd = _this->CreateSocket();

	AcceptEx(
		(SOCKET)fd,
		(SOCKET)lpListener->childFd,
		lpListener->data, // local and remote SOCKADDR
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		NULL,
		(LPOVERLAPPED)lpListener
	);

	duk_pop_n(ctx, 2);
	return 1;
}

duk_ret_t CScriptInstance::js_ioRead(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	// (fd, bufferSize, callback)
	HANDLE fd = (HANDLE)duk_get_uint(ctx, 0);
	size_t bufferSize = duk_get_uint(ctx, 1);
	void* jsCallback = duk_get_heapptr(ctx, 2);

	void* data = malloc(bufferSize); // freed after event is fired

	IOLISTENER* lpListener = _this->AddListener(fd, EVENT_READ, jsCallback, data, bufferSize);
	BOOL status = ReadFile(fd, lpListener->data, lpListener->dataLen, NULL, (LPOVERLAPPED)lpListener);

	if (status == false && GetLastError() != ERROR_IO_PENDING)
	{
		MessageBox(NULL, "readex error", "", MB_OK);
	}

	duk_pop_n(ctx, 3);
	return 1;
}

duk_ret_t CScriptInstance::js_ioWrite(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	HANDLE fd = (HANDLE)duk_get_uint(ctx, 0);
	duk_size_t dataLen;
	void* jsData = duk_to_buffer(ctx, 1, &dataLen);
	void* jsCallback = duk_get_heapptr(ctx, 2);

	char* data = (char*)malloc(dataLen + 1); // freed after event is fired
	memcpy(data, jsData, dataLen);
	data[dataLen] = '\0';

	IOLISTENER* lpListener = _this->AddListener(fd, EVENT_WRITE, jsCallback, data, dataLen);
	WriteFile(fd, lpListener->data, lpListener->dataLen, NULL, (LPOVERLAPPED)lpListener);

	duk_pop_n(ctx, 3);
	return 1;
}

duk_ret_t CScriptInstance::js_AddCallback(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	const char* hookId;
	void* heapptr;
	uint32_t param = 0;
	uint32_t param2 = 0;
	bool bOnce = false;

	int argc = duk_get_top(ctx);

	hookId = duk_get_string(ctx, 0);
	heapptr = duk_get_heapptr(ctx, 1);
	
	if (argc >= 3)
	{
		param = duk_get_uint(ctx, 2);
		if (argc > 3)
		{
			param2 = duk_get_uint(ctx, 3);
			if (argc > 4)
			{
				bOnce = duk_get_boolean(ctx, 4);
			}
		}
	}
	
	int callbackId = -1;

	CScriptHook* hook = _this->m_ScriptSystem->GetHook(hookId);
	if (hook != NULL)
	{
		callbackId = hook->Add(_this, heapptr, param, param2, bOnce);
	}

	duk_pop_n(ctx, argc);

	duk_push_int(ctx, callbackId);
	
	return 1;
}

duk_ret_t CScriptInstance::js_RemoveCallback(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);
	int callbackId = duk_get_int(ctx, 0);

	_this->m_ScriptSystem->RemoveCallbackById(callbackId);

	duk_pop(ctx);
	return 1;
}

duk_ret_t CScriptInstance::js_GetPCVal(duk_context* ctx)
{
	duk_push_uint(ctx, g_Reg->m_PROGRAM_COUNTER);
	return 1;
}

duk_ret_t CScriptInstance::js_SetPCVal(duk_context* ctx)
{
	uint32_t val = duk_to_uint32(ctx, 0);
	g_Reg->m_PROGRAM_COUNTER = val;
	duk_pop_n(ctx, 1);
	return 1;
}

duk_ret_t CScriptInstance::js_GetLOVal(duk_context* ctx)
{
	bool bUpper = duk_get_boolean(ctx, 0);
	duk_pop_n(ctx, 1);

	if (!bUpper)
	{
		duk_push_uint(ctx, g_Reg->m_LO.UW[0]);
	}
	else
	{
		duk_push_uint(ctx, g_Reg->m_LO.UW[1]);
	}
	
	return 1;
}

duk_ret_t CScriptInstance::js_SetLOVal(duk_context* ctx)
{
	bool bUpper = duk_get_boolean(ctx, 0);
	uint32_t val = duk_to_uint32(ctx, 1);

	duk_pop_n(ctx, 2);

	if (!bUpper)
	{
		g_Reg->m_LO.UW[0] = val;
	}
	else
	{
		g_Reg->m_LO.UW[1] = val;
	}
	
	return 1;
}

duk_ret_t CScriptInstance::js_GetHIVal(duk_context* ctx)
{
	bool bUpper = duk_get_boolean(ctx, 0);
	duk_pop_n(ctx, 1);

	if (!bUpper)
	{
		duk_push_uint(ctx, g_Reg->m_HI.UW[0]);
	}
	else
	{
		duk_push_uint(ctx, g_Reg->m_HI.UW[1]);
	}
	
	return 1;
}

duk_ret_t CScriptInstance::js_SetHIVal(duk_context* ctx)
{
	bool bUpper = duk_get_boolean(ctx, 0);
	uint32_t val = duk_to_uint32(ctx, 1);
	duk_pop_n(ctx, 2);

	if (!bUpper)
	{
		g_Reg->m_HI.UW[0] = val;
	}
	else
	{
		g_Reg->m_HI.UW[1] = val;
	}
	
	return 1;
}

duk_ret_t CScriptInstance::js_GetGPRVal(duk_context* ctx)
{
	int nargs = duk_get_top(ctx);
	int regnum = duk_to_int(ctx, 0);
	bool bUpper = false;

	if (nargs > 1)
	{
		bUpper = duk_to_boolean(ctx, 1);
	}

	duk_pop_n(ctx, nargs);

	if (!bUpper)
	{
		duk_push_uint(ctx, g_Reg->m_GPR[regnum].UW[0]);
	}
	else
	{
		duk_push_uint(ctx, g_Reg->m_GPR[regnum].UW[1]);
	}
	
	return 1;
}

duk_ret_t CScriptInstance::js_SetGPRVal(duk_context* ctx)
{
	int nargs = duk_get_top(ctx);
	int regnum = duk_to_int(ctx, 0);
	bool bUpper = duk_to_boolean(ctx, 1);
	uint32_t val = duk_to_uint32(ctx, 2);

	if (!bUpper)
	{
		g_Reg->m_GPR[regnum].UW[0] = val;
	}
	else
	{
		g_Reg->m_GPR[regnum].UW[1] = val;
	}
	
	duk_pop_n(ctx, 3);
	return 1;
}

duk_ret_t CScriptInstance::js_GetFPRVal(duk_context* ctx)
{
	int nargs = duk_get_top(ctx);
	int regnum = duk_to_int(ctx, 0);
	bool bDouble = false;

	if (nargs > 1)
	{
		bDouble = duk_get_boolean(ctx, 1);
	}
	
	duk_pop_n(ctx, nargs);

	if (!bDouble)
	{
		duk_push_number(ctx, (duk_double_t)*g_Reg->m_FPR_S[regnum]);
	}
	else
	{
		duk_push_number(ctx, (duk_double_t)*g_Reg->m_FPR_D[regnum & 0x1E]);
	}
	return 1;
}

duk_ret_t CScriptInstance::js_SetFPRVal(duk_context* ctx)
{
	int nargs = duk_get_top(ctx);
	int regnum = duk_to_int(ctx, 0);
	bool bDouble = duk_to_boolean(ctx, 1);
	duk_double_t val = duk_to_number(ctx, 2);
	
	if (!bDouble)
	{
		*g_Reg->m_FPR_S[regnum] = (float)val;
	}
	else
	{
		*g_Reg->m_FPR_D[regnum & 0x1E] = val;
	}
	
	duk_pop_n(ctx, 3);
	return 1;
}

duk_ret_t CScriptInstance::js_GetROMInt(duk_context* ctx)
{
	uint32_t address = duk_to_uint32(ctx, 0) & 0x0FFFFFFF;
	int bitwidth = duk_to_int(ctx, 1);
	duk_bool_t bSigned = duk_to_boolean(ctx, 2);

	duk_pop_n(ctx, 3);

	if (g_Rom == NULL)
	{
		goto return_err;
	}

	uint8_t* rom = g_Rom->GetRomAddress(); // little endian
	uint32_t romSize = g_Rom->GetRomSize();

	if (address > romSize)
	{
		goto return_err;
	}

	DWORD retval;

	switch (bitwidth)
	{
	case 8:
	{
		uint8_t val = rom[address ^ 0b11];
		retval = bSigned ? (char)val : val;
		break;
	}
	case 16:
	{
		uint16_t val = *(uint16_t*)&rom[address ^ 0b10];
		retval = bSigned ? (short)val : val;
		break;
	}
	case 32:
	{
		uint32_t val = *(uint32_t*)&rom[address];
		retval = bSigned ? (int)val : val;
		break;
	}
	default:
		goto return_err;
	}

	if (bSigned)
	{
		duk_push_int(ctx, retval);
	}
	else
	{
		duk_push_uint(ctx, retval);
	}

	return 1;

return_err:
	duk_push_boolean(ctx, false);
	return 1;
}

duk_ret_t CScriptInstance::js_GetROMFloat(duk_context* ctx)
{
	int argc = duk_get_top(ctx);

	uint32_t address = duk_to_uint32(ctx, 0);
	duk_bool_t bDouble = false;

	if (argc > 1)
	{
		bDouble = duk_get_boolean(ctx, 1);
	}

	duk_pop_n(ctx, argc);

	if (g_Rom == NULL)
	{
		goto return_err;
	}

	uint8_t* rom = g_Rom->GetRomAddress(); // little endian
	uint32_t romSize = g_Rom->GetRomSize();

	if (address > romSize)
	{
		goto return_err;
	}

	if (!bDouble)
	{
		float value = *(float*)&rom[address];
		duk_push_number(ctx, (double)value);
		return 1;
	}
	
	uint8_t raw[8];
	
	memcpy(&raw[0], &rom[address + 4], 4);
	memcpy(&raw[4], &rom[address], 4);
	
	double value;
	memcpy(&value, raw, 8);
	
	duk_push_number(ctx, value);
	return 1;

return_err:
	duk_push_boolean(ctx, false);
	return 1;
}

duk_ret_t CScriptInstance::js_GetRDRAMInt(duk_context* ctx)
{
	uint32_t address = duk_to_uint32(ctx, 0);
	int bitwidth = duk_to_int(ctx, 1);
	duk_bool_t bSigned = duk_to_boolean(ctx, 2);

	duk_pop_n(ctx, 3);

	if (g_MMU == NULL)
	{
		goto return_err;
	}

	DWORD retval;

	switch (bitwidth)
	{
	case 8:
	{
		uint8_t val;
		if (!g_MMU->LB_VAddr(address, val))
		{
			goto return_err;
		}
		retval = bSigned ? (char)val : val;
		break;
	}
	case 16:
	{
		uint16_t val;
		if (!g_MMU->LH_VAddr(address, val))
		{
			goto return_err;
		}
		retval = bSigned ? (short)val : val;
		break;
	}
	case 32:
	{
		uint32_t val;
		if (!g_MMU->LW_VAddr(address, val))
		{
			goto return_err;
		}
		retval = bSigned ? (int)val : val;
		break;
	}
	default:
		goto return_err;
	}

	if (bSigned)
	{
		duk_push_int(ctx, retval);
	}
	else
	{
		duk_push_uint(ctx, retval);
	}

	return 1;

return_err:
	duk_push_boolean(ctx, false);
	return 1;
}

duk_ret_t CScriptInstance::js_SetRDRAMInt(duk_context* ctx)
{
	uint32_t address = duk_to_uint32(ctx, 0);
	int bitwidth = duk_to_int(ctx, 1);
	DWORD newValue = duk_to_uint32(ctx, 2);

	duk_pop_n(ctx, 3);

	if (g_MMU == NULL)
	{
		goto return_err;
	}

	switch (bitwidth)
	{
	case 8:
		if (!g_MMU->SB_VAddr(address, (uint8_t)newValue))
		{
			goto return_err;
		}
		goto return_ok;
	case 16:
		if (!g_MMU->SH_VAddr(address, (uint16_t)newValue))
		{
			goto return_err;
		}
		goto return_ok;
	case 32:
		if (!g_MMU->SW_VAddr(address, (uint32_t)newValue))
		{
			goto return_err;
		}
		goto return_ok;
	default:
		goto return_err;
	}

return_ok:
	duk_push_boolean(ctx, true);
	return 1;

return_err:
	duk_push_boolean(ctx, false);
	return 1;
}

duk_ret_t CScriptInstance::js_GetRDRAMFloat(duk_context* ctx)
{
	int argc = duk_get_top(ctx);

	uint32_t address = duk_to_uint32(ctx, 0);
	duk_bool_t bDouble = false;

	if (argc > 1)
	{
		bDouble = duk_get_boolean(ctx, 1);
	}

	duk_pop_n(ctx, argc);

	if (g_MMU == NULL)
	{
		goto return_err;
	}

	if (!bDouble)
	{
		uint32_t rawValue;
		if (!g_MMU->LW_VAddr(address, rawValue))
		{
			goto return_err;
		}

		float floatValue;
		memcpy(&floatValue, &rawValue, sizeof(float));

		duk_push_number(ctx, (double)floatValue);
		return 1;
	}

	uint64_t rawValue;
	if (!g_MMU->LD_VAddr(address, rawValue))
	{
		goto return_err;
	}

	double floatValue;
	memcpy(&floatValue, &rawValue, sizeof(double));

	duk_push_number(ctx, floatValue);
	return 1;

return_err:
	duk_push_boolean(ctx, false);
	return 1;
}

duk_ret_t CScriptInstance::js_SetRDRAMFloat(duk_context* ctx)
{
	int argc = duk_get_top(ctx);

	uint32_t address = duk_to_uint32(ctx, 0);
	double value = duk_get_number(ctx, 1);
	duk_bool_t bDouble = false;

	if (argc > 2)
	{
		bDouble = duk_get_boolean(ctx, 2);
	}

	duk_pop_n(ctx, argc);

	if (g_MMU == NULL)
	{
		goto return_err;
	}

	if (!bDouble)
	{
		float floatValue = (float)value;
		uint32_t rawValue;
		memcpy(&rawValue, &floatValue, sizeof(float));
		if (!g_MMU->SW_VAddr(address, rawValue))
		{
			goto return_err;
		}
	}
	else
	{
		double floatValue = (double)value;
		uint64_t rawValue;
		memcpy(&rawValue, &floatValue, sizeof(double));
		if (!g_MMU->SD_VAddr(address, rawValue))
		{
			goto return_err;
		}
	}

	duk_push_boolean(ctx, true);
	return 1;

return_err:
	duk_push_boolean(ctx, false);
	return 1;
}

duk_ret_t CScriptInstance::js_GetRDRAMBlock(duk_context* ctx)
{
	uint32_t address = duk_get_uint(ctx, 0);
	uint32_t size = duk_get_uint(ctx, 1);

	duk_pop_n(ctx, 2);

	uint8_t* block = (uint8_t*)duk_push_buffer(ctx, size, false);

	for (uint32_t i = 0; i < size; i++)
	{
		g_MMU->LB_VAddr(address + i, block[i]);
	}

	return 1;
}

duk_ret_t CScriptInstance::js_MsgBox(duk_context* ctx)
{
	int argc = duk_get_top(ctx);

	const char* msg = duk_to_string(ctx, 0);
	const char* caption = "";

	if (argc > 1)
	{
		caption = duk_to_string(ctx, 1);
	}

	MessageBox(NULL, msg, caption, MB_OK);

	duk_pop_n(ctx, argc);
	duk_push_boolean(ctx, 1);
	return 1;
}

// Return zero-terminated string from ram
duk_ret_t CScriptInstance::js_GetRDRAMString(duk_context* ctx)
{
	// (address[, maxLen])
	int nargs = duk_get_top(ctx);
	uint32_t address = duk_get_uint(ctx, 0);
	int maxLen = INT_MAX;
	
	if (nargs > 1)
	{
		maxLen = duk_get_int(ctx, 1);
		if (maxLen == 0)
		{
			maxLen = INT_MAX;
		}
	}

	uint8_t test = 0xFF;
	int len = 0;

	// determine length of string
	while (len < maxLen && g_MMU->LB_VAddr(address + len, test) && test != 0) // todo protect from ram overrun
	{
		if ((address & 0xFFFFFF) + len >= g_MMU->RdramSize())
		{
			break;
		}
		len++;
	}

	uint8_t* str = (uint8_t*)malloc(len + 1);
	
	for (int i = 0; i < len; i++)
	{
		g_MMU->LB_VAddr(address + i, str[i]);
	}

	str[len] = '\0';

	duk_pop_n(ctx, nargs);
	duk_push_string(ctx, (char*)str);
	free(str); // duk creates internal copy
	return 1;
}

// Return zero-terminated string from rom
duk_ret_t CScriptInstance::js_GetROMString(duk_context* ctx)
{
	// (address[, maxLen])
	int nargs = duk_get_top(ctx);
	uint32_t address = duk_get_uint(ctx, 0);
	int maxLen = INT_MAX;

	if (nargs > 1)
	{
		maxLen = duk_get_int(ctx, 1);
		if (maxLen == 0)
		{
			maxLen = INT_MAX;
		}
	}
	
	char* rom = (char*) g_Rom->GetRomAddress();
	uint32_t romSize = g_Rom->GetRomSize();

	int len = 0;

	while (len < maxLen)
	{
		if (address + len >= romSize)
		{
			break;
		}
		if (rom[address + len] == 0)
		{
			break;
		}

		len++;
	}

	char* str = (char*) malloc(len + 1);

	for (int i = 0; i < len; i++)
	{
		str[i] = rom[(address + i) ^ 3];
	}
	str[len] = '\0';
	
	duk_pop(ctx);
	duk_push_string(ctx, str);
	free(str); // duk creates internal copy
	return 1;
}

// Return zero-terminated string from rom
duk_ret_t CScriptInstance::js_GetROMBlock(duk_context* ctx)
{
	uint32_t address = duk_get_uint(ctx, 0);
	uint32_t size = duk_get_uint(ctx, 1);

	duk_pop_n(ctx, 2);

	uint8_t* block = (uint8_t*)duk_push_buffer(ctx, size, false);
	uint8_t* rom = g_Rom->GetRomAddress();
	memcpy(block, &rom[address], size);
	
	return 1;
}

duk_ret_t CScriptInstance::js_ConsolePrint(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);
	const char* text = duk_to_string(ctx, 0);
	
	_this->m_Debugger->Debug_LogScriptsWindow(text);

	duk_pop_n(ctx, 1);

	return 1;
}

duk_ret_t CScriptInstance::js_ConsoleClear(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);
	_this->m_Debugger->Debug_ClearScriptsWindow();
	return 1;
}

duk_ret_t CScriptInstance::js_Pause(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);
	CBreakpoints* breakpoints = _this->m_Debugger->Breakpoints();
	breakpoints->Pause();
	return 1;
}

duk_ret_t CScriptInstance::js_ShowCommands(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	int nargs = duk_get_top(ctx);

	uint32_t address = 0x80000000;

	if (nargs == 1)
	{
		address = duk_get_uint(ctx, 0);
	}

	duk_pop_n(ctx, nargs);

	_this->m_Debugger->Debug_ShowCommandsLocation(address, false);
	return 1;
}

duk_ret_t CScriptInstance::js_ScreenPrint(duk_context* ctx)
{
	CScriptInstance* _this = FetchInstance(ctx);

	HDC hdc = _this->m_ScriptSystem->GetScreenDC();

	int nargs = duk_get_top(ctx);

	if (nargs != 3)
	{
		duk_pop_n(ctx, nargs);
		return 1;
	}

	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);

	const char* text = duk_to_string(ctx, 2);

	int nChars = strlen(text);

	TextOut(hdc, x, y, text, nChars);

	duk_pop_n(ctx, nargs);
	return 1;
}

////////////


static BOOL ConnectEx(SOCKET s, const SOCKADDR* name, int namelen, PVOID lpSendBuffer,
	DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped)
{
	LPFN_CONNECTEX ConnectExPtr = NULL;
	DWORD nBytes;
	GUID guid = WSAID_CONNECTEX;
	int fetched = WSAIoctl(
		s,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		(void*)&guid,
		sizeof(GUID),
		&ConnectExPtr,
		sizeof(LPFN_CONNECTEX),
		&nBytes,
		NULL,
		NULL
	);

	if (fetched == 0 && ConnectExPtr != NULL)
	{
		ConnectExPtr(s, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
	}

	return false;
}