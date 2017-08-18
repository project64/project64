#include <stdafx.h>

#include "ScriptHook.h"
#include "ScriptInstance.h"
#include "ScriptSystem.h"

int CScriptHook::Add(CScriptInstance* scriptInstance, void* heapptr, uint32_t param, uint32_t param2, bool bOnce)
{
	JSCALLBACK jsCallback;
	jsCallback.scriptInstance = scriptInstance;
	jsCallback.heapptr = heapptr;
	jsCallback.callbackId = m_ScriptSystem->GetNextCallbackId();
	jsCallback.param = param;
	jsCallback.param2 = param2;
	jsCallback.bOnce = bOnce;
	m_Callbacks.push_back(jsCallback);
	return jsCallback.callbackId;
}

void CScriptHook::InvokeById(int callbackId)
{
	int nCallbacks = m_Callbacks.size();
	for (int i = 0; i < nCallbacks; i++)
	{
		if (m_Callbacks[i].callbackId == callbackId)
		{
			m_Callbacks[i].scriptInstance->Invoke(m_Callbacks[i].heapptr);
			return;
		}
	}
}

void CScriptHook::InvokeByParam(uint32_t param)
{
	int nCallbacks = m_Callbacks.size();
	for (int i = 0; i < nCallbacks; i++)
	{
		if (m_Callbacks[i].param == param)
		{
			m_Callbacks[i].scriptInstance->Invoke(m_Callbacks[i].heapptr, param);
			return;
		}
	}
}

void CScriptHook::InvokeByParamInRange(uint32_t param)
{
	int nCallbacks = m_Callbacks.size();
	for (int i = 0; i < nCallbacks; i++)
	{
		if (param == m_Callbacks[i].param || (param >= m_Callbacks[i].param && param < m_Callbacks[i].param2))
		{
			m_Callbacks[i].scriptInstance->Invoke(m_Callbacks[i].heapptr, param);
			return;
		}
	}
}

void CScriptHook::InvokeAll()
{
	int nCallbacks = m_Callbacks.size();
	for (int i = 0; i < nCallbacks; i++)
	{
		m_Callbacks[i].scriptInstance->Invoke(m_Callbacks[i].heapptr);
	}
}

void CScriptHook::RemoveById(int callbackId)
{
	int nCallbacks = m_Callbacks.size();
	for (int i = 0; i < nCallbacks; i++)
	{
		if (m_Callbacks[i].callbackId == callbackId)
		{
			m_Callbacks.erase(m_Callbacks.begin() + i);
			return;
		}
	}
}

void CScriptHook::RemoveByParam(uint32_t param)
{
	int nCallbacks = m_Callbacks.size();
	for (int i = 0; i < nCallbacks; i++)
	{
		if (m_Callbacks[i].param == param)
		{
			m_Callbacks.erase(m_Callbacks.begin() + i);
			return;
		}
	}
}

void CScriptHook::RemoveByInstance(CScriptInstance* scriptInstance)
{
	int lastIndex = m_Callbacks.size() - 1;
	for (int i = lastIndex; i >= 0; i--)
	{
		if (m_Callbacks[i].scriptInstance == scriptInstance)
		{
			m_Callbacks.erase(m_Callbacks.begin() + i);
		}
	}
}

bool CScriptHook::HasContext(CScriptInstance* scriptInstance)
{
	for (int i = 0; i < m_Callbacks.size(); i++)
	{
		if (m_Callbacks[i].scriptInstance == scriptInstance)
		{
			return true;
		}
	}
	return false;
}

CScriptHook::CScriptHook(CScriptSystem* scriptSystem)
{
	m_ScriptSystem = scriptSystem;
}

CScriptHook::~CScriptHook()
{
	m_Callbacks.clear();
}