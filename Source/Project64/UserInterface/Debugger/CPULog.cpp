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
#include "stdafx.h"
#include "CPULog.h"
#include <Project64-core/N64System/Mips/OpCodeName.h>

CCPULog::CCPULog(size_t size) :
    m_bMaxed(false),
    m_Index(0)
{
    if (size != 0)
    {
        m_Size = size;
    }
    else if (!g_Settings->LoadBool(Debugger_CPULoggingEnabled))
    {
        m_Size = 0;
    }
    else
    {
        m_Size = g_Settings->LoadDword(Debugger_CPULogBufferSize);
    }

    if (m_Size == 0)
    {
        m_Array = NULL;
        return;
    }
    
    m_Array = new CPUState[m_Size];
}

CCPULog::~CCPULog(void)
{
    if (m_Array != NULL)
    {
        delete[] m_Array;
        m_Array = NULL;
    }
}

void CCPULog::PushState()
{
    if (m_Array == NULL)
    {
        return;
    }

    if (m_Index == m_Size)
    {
        m_Index = 0;
        m_bMaxed = true;
    }

    CPUState* state = &m_Array[m_Index++];

    state->pc = g_Reg->m_PROGRAM_COUNTER;
    state->opcode = R4300iOp::m_Opcode;

    memcpy(state->gpr, g_Reg->m_GPR, sizeof(g_Reg->m_GPR));
    state->gprHi = g_Reg->m_HI;
    state->gprLo = g_Reg->m_LO;

    for (int i = 0; i < 32; i++)
    {
        state->fpr[i] = *g_Reg->m_FPR_S[i];
    }

    state->fpcr = g_Reg->m_FPCR[31];
}

size_t CCPULog::GetCount(void)
{
    if (m_bMaxed)
    {
        return m_Size;
    }
    return m_Index;
}

size_t CCPULog::GetSize(void)
{
    return m_Size;
}

CPUState* CCPULog::GetEntry(size_t index)
{
    if (m_Array == NULL)
    {
        return NULL;
    }

    if (m_bMaxed)
    {
        if (index >= m_Size)
        {
            return NULL;
        }
        return &m_Array[(m_Index + index) % m_Size];
    }

    if (index >= m_Index)
    {
        return NULL;
    }

    return &m_Array[index];
}

void CCPULog::Reset()
{
    size_t newSize;
    
    if (!g_Settings->LoadBool(Debugger_CPULoggingEnabled))
    {
        newSize = 0;
    }
    else
    {
        newSize = g_Settings->LoadDword(Debugger_CPULogBufferSize);
    }

    m_Index = 0;
    m_bMaxed = false;

    if (m_Size != newSize)
    {
        m_Size = newSize;

        if (m_Array != NULL)
        {
            delete[] m_Array;
            m_Array = NULL;
        }
        
        if (m_Size != 0)
        {
            m_Array = new CPUState[m_Size];
        }
    }
}

CCPULog* CCPULog::Clone(void)
{
    if (m_Array == NULL)
    {
        return NULL;
    }

    CCPULog *clone = new CCPULog(m_Size);
    clone->m_bMaxed = m_bMaxed;
    clone->m_Index = m_Index;
    memcpy(clone->m_Array, m_Array, sizeof(CPUState) * m_Size);
    return clone;
}

void CCPULog::DumpToFile(const char* path)
{
    FILE* fp = fopen(path, "wb");

    if (fp == NULL)
    {
        return;
    }

    size_t count = GetCount();

    for (size_t i = 0; i < count; i++)
    {
        CPUState* state = GetEntry(i);

        char *tokctx;

        char* szCommand = (char*)R4300iOpcodeName(state->opcode.Hex, state->pc);
        char* szCmdName = strtok_s((char*)szCommand, "\t", &tokctx);
        char* szCmdArgs = strtok_s(NULL, "\t", &tokctx);

        if (szCmdArgs == NULL)
        {
            szCmdArgs = "";
        }

        fprintf(fp, "%08X: %08X %s %s\n", state->pc, state->opcode.Hex, szCmdName, szCmdArgs);
    }

    fclose(fp);
}