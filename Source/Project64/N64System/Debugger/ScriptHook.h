#pragma once

#include <stdafx.h>

class CScriptInstance;
class CScriptSystem;

class CScriptHook
{
private:
	typedef struct {
		CScriptInstance* scriptInstance;
		void* heapptr;
		uint32_t param;
		uint32_t param2;
		int callbackId;
		bool bOnce;
	} JSCALLBACK;

	CScriptSystem* m_ScriptSystem;

	//int m_NextCallbackId;
	vector<JSCALLBACK> m_Callbacks;

public:
	CScriptHook(CScriptSystem* scriptSystem);
	~CScriptHook();
	int Add(CScriptInstance* scriptInstance, void* heapptr, uint32_t param = 0, uint32_t param2 = 0, bool bOnce = false);
	void InvokeAll();
	void InvokeById(int callbackId);
	void InvokeByParam(uint32_t param);
	/* invoke if param >= cb.param && param < cb.param2*/
	void InvokeByParamInRange(uint32_t param);
	void RemoveById(int callbackId);
	void RemoveByParam(uint32_t tag);
	void RemoveByInstance(CScriptInstance* scriptInstance);
	bool HasContext(CScriptInstance* scriptInstance);
	//bool HasTag(uint32_t tag);
};