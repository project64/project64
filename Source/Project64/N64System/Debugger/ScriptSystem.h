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

#pragma once

#include <stdafx.h>
#include <3rdParty/duktape/duktape.h>

#include "ScriptInstance.h"

class CScriptHook;

class CScriptSystem
{
public:
	CScriptSystem(CDebuggerUI* debugger);
	~CScriptSystem();
	// Run a script in its own context/thread
	void RunScript(char* path);

	// Kill a script context/thread by its path
	void StopScript(char* path);

	const char* APIScript();

private:
	typedef struct {
		const char* hookId;
		CScriptHook* cbList;
	} HOOKENTRY;

	typedef struct {
		char* path;
		CScriptInstance* scriptInstance;
	} INSTANCE_ENTRY;

	CDebuggerUI* m_Debugger;

	char* m_APIScript;

	vector<HOOKENTRY> m_Hooks;
	vector<INSTANCE_ENTRY> m_RunningInstances;

	vector<char*> m_LogData;

	CScriptHook* m_HookCPUExec;
	CScriptHook* m_HookCPURead;
	CScriptHook* m_HookCPUWrite;

	CScriptHook* m_HookFrameDrawn;

	void RegisterHook(const char* hookId, CScriptHook* cbList); // associate string id with callback list
	void UnregisterHooks();

	HDC m_ScreenDC;

	int m_NextCallbackId;

public:
	// Returns true if any of the script hooks have callbacks for scriptInstance


	void SetScreenDC(HDC hdc)
	{
		m_ScreenDC = hdc;
	}

	HDC GetScreenDC()
	{
		return m_ScreenDC;
	}

	inline vector<char*>* LogData()
	{
		return &m_LogData;
	}
	
	inline void LogText(const char* text)
	{
		char* newStr = (char*)malloc(strlen(text));
		strcpy(newStr, text);
		m_LogData.push_back(newStr);
		m_Debugger->Debug_RefreshScriptsWindow();
	}
	
	bool HasCallbacksForInstance(CScriptInstance* scriptInstance);

	// Remove all hooked callbacks for an instance
	void ClearCallbacksForInstance(CScriptInstance* scriptInstance);

	void RemoveCallbackById(int callbackId);

	CScriptHook* GetHook(const char* hookId);

	int GetNextCallbackId();
	
	void DeleteStoppedInstances();
	INSTANCE_STATE GetInstanceState(char* scriptName);
	CScriptInstance* GetInstance(char* scriptName);

	CScriptHook* HookCPUExec()
	{
		return m_HookCPUExec;
	}

	CScriptHook* HookCPURead()
	{
		return m_HookCPURead;
	}

	CScriptHook* HookCPUWrite()
	{
		return m_HookCPUWrite;
	}

	CScriptHook* HookFrameDrawn()
	{
		return m_HookFrameDrawn;
	}
};