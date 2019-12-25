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
#include <stdafx.h>

#include "ScriptHook.h"
#include "ScriptInstance.h"
#include "ScriptSystem.h"

int CScriptHook::Add(CScriptInstance* scriptInstance, void* heapptr, uint32_t param, uint32_t param2,
    uint32_t param3, uint32_t param4, bool bOnce)
{
    JSCALLBACK jsCallback;
    jsCallback.scriptInstance = scriptInstance;
    jsCallback.heapptr = heapptr;
    jsCallback.callbackId = m_ScriptSystem->GetNextCallbackId();
    jsCallback.param = param;
    jsCallback.param2 = param2;
    jsCallback.param3 = param3;
    jsCallback.param4 = param4;
    jsCallback.bOnce = bOnce;
    m_Callbacks.push_back(jsCallback);
    m_ScriptSystem->CallbackAdded();
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

void CScriptHook::InvokeByAddressInRange(uint32_t address)
{
    int nCallbacks = m_Callbacks.size();
    for (int i = 0; i < nCallbacks; i++)
    {
        if (address == m_Callbacks[i].param || (address >= m_Callbacks[i].param && address <= m_Callbacks[i].param2))
        {
            m_Callbacks[i].scriptInstance->Invoke(m_Callbacks[i].heapptr, address);
            return;
        }
    }
}

void CScriptHook::InvokeByAddressInRange_MaskedOpcode(uint32_t pc, uint32_t opcode)
{
    int nCallbacks = m_Callbacks.size();
    for (int i = 0; i < nCallbacks; i++)
    {
        if (pc == m_Callbacks[i].param || (pc >= m_Callbacks[i].param && pc <= m_Callbacks[i].param2))
        {
            if ((m_Callbacks[i].param3 & m_Callbacks[i].param4) == (opcode & m_Callbacks[i].param4))
            {
                m_Callbacks[i].scriptInstance->Invoke(m_Callbacks[i].heapptr, pc);
                return;
            }
        }
    }
}

void CScriptHook::InvokeByAddressInRange_GPRValue(uint32_t pc)
{
    int nCallbacks = m_Callbacks.size();

    for (int i = 0; i < nCallbacks; i++)
    {
        uint32_t startAddress = m_Callbacks[i].param;
        uint32_t endAddress = m_Callbacks[i].param2;
        uint32_t registers = m_Callbacks[i].param3;
        uint32_t value = m_Callbacks[i].param4;

        if (!(pc == startAddress || (pc >= startAddress && pc <= endAddress)))
        {
            continue;
        }

        for (int nReg = 0; nReg < 32; nReg++)
        {
            if (registers & (1 << nReg))
            {
                if (value == g_Reg->m_GPR[nReg].UW[0])
                {
                    m_Callbacks[i].scriptInstance->Invoke2(m_Callbacks[i].heapptr, pc, nReg);
                    break;
                }
            }
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
            m_ScriptSystem->CallbackRemoved();
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
            m_ScriptSystem->CallbackRemoved();
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
            m_ScriptSystem->CallbackRemoved();
        }
    }
}

bool CScriptHook::HasContext(CScriptInstance* scriptInstance)
{
    for (size_t i = 0; i < m_Callbacks.size(); i++)
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