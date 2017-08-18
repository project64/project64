/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include <stdafx.h>
#include "ScriptSystem.h"
#include "Debugger-Scripts.h"

#include "ScriptInstance.h"
#include "ScriptHook.h"

CScriptSystem::CScriptSystem(CDebuggerUI* debugger)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_NextCallbackId = 0;

	m_Debugger = debugger;

	m_HookCPUExec = new CScriptHook(this);
	m_HookCPURead = new CScriptHook(this);
	m_HookCPUWrite = new CScriptHook(this);
	m_HookFrameDrawn = new CScriptHook(this);

	RegisterHook("exec", m_HookCPUExec);
	RegisterHook("read", m_HookCPURead);
	RegisterHook("write", m_HookCPUWrite);
	RegisterHook("draw", m_HookFrameDrawn);

	HMODULE hInst = GetModuleHandle(NULL);
	HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(IDR_JSAPI_TEXT), "TEXT");
	
	HGLOBAL hGlob = LoadResource(hInst, hRes);
	DWORD resSize = SizeofResource(hInst, hRes);
	m_APIScript = (char*) malloc(resSize + 1);
	
	void* resData = LockResource(hGlob);
	memcpy(m_APIScript, resData, resSize);
	m_APIScript[resSize] = '\0';
	FreeResource(hGlob);
}

CScriptSystem::~CScriptSystem()
{
	for (int i = 0; i < m_Hooks.size(); i++)
	{
		delete m_Hooks[i].cbList;
	}

	UnregisterHooks();
	free(m_APIScript);
}

const char* CScriptSystem::APIScript()
{
	return m_APIScript;
}

void CScriptSystem::RunScript(char* path)
{
	CScriptInstance* scriptInstance = new CScriptInstance(m_Debugger);
	char* pathSaved = (char*)malloc(strlen(path)); // freed via DeleteStoppedInstances
	strcpy(pathSaved, path);
	m_RunningInstances.push_back({ pathSaved, scriptInstance });
	scriptInstance->Start(pathSaved);
}

void CScriptSystem::StopScript(char* path)
{
	int nInstances = m_RunningInstances.size();

	CScriptInstance* scriptInstance = GetInstance(path);

	if (scriptInstance == NULL)
	{
		return;
	}
	
	scriptInstance->ForceStop();
}

void CScriptSystem::DeleteStoppedInstances()
{
	int lastIndex = m_RunningInstances.size() - 1;
	for (int i = lastIndex; i >= 0; i--)
	{
		if (m_RunningInstances[i].scriptInstance->GetState() == STATE_STOPPED)
		{
			free(m_RunningInstances[i].path);
			CScriptInstance* instance = m_RunningInstances[i].scriptInstance;
			delete instance;
			m_RunningInstances.erase(m_RunningInstances.begin() + i);
		}
	}
}

INSTANCE_STATE CScriptSystem::GetInstanceState(char* path)
{
	for (int i = 0; i < m_RunningInstances.size(); i++)
	{
		if (strcmp(m_RunningInstances[i].path, path) == 0)
		{
			return m_RunningInstances[i].scriptInstance->GetState();
		}
	}
	return STATE_INVALID;
}

CScriptInstance* CScriptSystem::GetInstance(char* path)
{
	for (int i = 0; i < m_RunningInstances.size(); i++)
	{
		if (strcmp(m_RunningInstances[i].path, path) == 0)
		{
			return m_RunningInstances[i].scriptInstance;
		}
	}
	return NULL;
}

bool CScriptSystem::HasCallbacksForInstance(CScriptInstance* scriptInstance)
{
	for (int i = 0; i < m_Hooks.size(); i++)
	{
		if (m_Hooks[i].cbList->HasContext(scriptInstance))
		{
			return true;
		}
	}
	return false;
}

void CScriptSystem::ClearCallbacksForInstance(CScriptInstance* scriptInstance)
{
	for (int i = 0; i < m_Hooks.size(); i++)
	{
		m_Hooks[i].cbList->RemoveByInstance(scriptInstance);
	}
}

void CScriptSystem::RemoveCallbackById(int callbackId)
{
	for (int i = 0; i < m_Hooks.size(); i++)
	{
		m_Hooks[i].cbList->RemoveById(callbackId);
	}
}

void CScriptSystem::RegisterHook(const char* hookId, CScriptHook* cbList)
{
	HOOKENTRY hook = { hookId, cbList };
	m_Hooks.push_back(hook);
}

void CScriptSystem::UnregisterHooks()
{
	m_Hooks.clear();
}

CScriptHook* CScriptSystem::GetHook(const char* hookId)
{
	int size = m_Hooks.size();
	for (int i = 0; i < size; i++)
	{
		if (strcmp(m_Hooks[i].hookId, hookId) == 0)
		{
			return m_Hooks[i].cbList;
		}
	}
	return NULL;
}

int CScriptSystem::GetNextCallbackId()
{
	return m_NextCallbackId++;
}