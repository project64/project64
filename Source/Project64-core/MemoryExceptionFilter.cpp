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
#include "stdafx.h"
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <Project64-core/N64System/SystemGlobals.h>

#ifdef _WIN32
int32_t CMipsMemoryVM::MemoryFilter(uint32_t dwExptCode, void * lpExceptionPointer)
{
#if defined(_M_IX86) && defined(_WIN32)
    // to do:  Remove the _M_IX86 criteria.  This can compile on 64-bit Windows.
#ifndef _WIN64
    DWORD * Reg;
    // We need this to fix 32-bit Windows builds,
    // because Project64 currently uses uint32_t all the time instead of int32_t.
#else
    size_t * Reg;
#endif

    if (dwExptCode != EXCEPTION_ACCESS_VIOLATION || g_MMU == NULL)
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    //convert the pointer since we are not having win32 structures in headers
    LPEXCEPTION_POINTERS lpEP = (LPEXCEPTION_POINTERS)lpExceptionPointer;

    uint32_t MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)g_MMU->Rdram();
    if ((int32_t)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF)
    {
        //		if (bHaveDebugger())
        //		{
        //			g_Notify->BreakPoint(__FILE__, __LINE__);
        //		}
        return EXCEPTION_EXECUTE_HANDLER;
    }

    uint8_t * TypePos = (uint8_t *)lpEP->ContextRecord->Eip;
    EXCEPTION_RECORD exRec = *lpEP->ExceptionRecord;

    Reg = NULL;
    if (*TypePos == 0xF3 && (*(TypePos + 1) == 0xA4 || *(TypePos + 1) == 0xA5))
    {
        uint32_t Start = (lpEP->ContextRecord->Edi - (uint32_t)m_RDRAM);
        uint32_t End = Start + lpEP->ContextRecord->Ecx;
        if ((int32_t)Start < 0)
        {
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }
#ifdef CFB_READ
        uint32_t count, OldProtect;
        if (Start >= CFBStart && End < CFBEnd)
        {
            for (count = Start; count < End; count += 0x1000)
            {
                VirtualProtect(m_RDRAM + count, 4, PAGE_READONLY, &OldProtect);
                if (FrameBufferRead)
                {
                    FrameBufferRead(count & ~0xFFF);
                }
            }
            return EXCEPTION_CONTINUE_EXECUTION;
        }
#endif
        if (End < RdramSize())
        {
            for (uint32_t count = (Start & ~0xFFF); count < End; count += 0x1000)
            {
                g_Recompiler->ClearRecompCode_Phys(count, 0x1000, CRecompiler::Remove_ProtectedMem);
            }
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (Start >= 0x04000000 && End < 0x04002000)
        {
            g_Recompiler->ClearRecompCode_Phys(Start & ~0xFFF, 0x1000, CRecompiler::Remove_ProtectedMem);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    uint8_t * ReadPos;
    if (*TypePos == 0x0F && *(TypePos + 1) == 0xB6)
    {
        ReadPos = TypePos + 2;
    }
    else if (*TypePos == 0x0F && *(TypePos + 1) == 0xB7)
    {
        ReadPos = TypePos + 2;
    }
    else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBE)
    {
        ReadPos = TypePos + 2;
    }
    else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBF)
    {
        ReadPos = TypePos + 2;
    }
    else if (*TypePos == 0x66)
    {
        ReadPos = TypePos + 2;
    }
    else
    {
        ReadPos = TypePos + 1;
    }

    switch (*ReadPos & 0x38)
    {
    case 0x00: Reg = &(lpEP->ContextRecord->Eax); break;
    case 0x08: Reg = &(lpEP->ContextRecord->Ecx); break;
    case 0x10: Reg = &(lpEP->ContextRecord->Edx); break;
    case 0x18: Reg = &(lpEP->ContextRecord->Ebx); break;
    case 0x20: Reg = &(lpEP->ContextRecord->Esp); break;
    case 0x28: Reg = &(lpEP->ContextRecord->Ebp); break;
    case 0x30: Reg = &(lpEP->ContextRecord->Esi); break;
    case 0x38: Reg = &(lpEP->ContextRecord->Edi); break;
    }

    switch ((*ReadPos & 0xC7))
    {
    case 0: ReadPos += 1; break;
    case 1: ReadPos += 1; break;
    case 2: ReadPos += 1; break;
    case 3: ReadPos += 1; break;
    case 4:
        ReadPos += 1;
        switch ((*ReadPos & 0xC7))
        {
        case 0: ReadPos += 1; break;
        case 1: ReadPos += 1; break;
        case 2: ReadPos += 1; break;
        case 3: ReadPos += 1; break;
        case 6: ReadPos += 1; break;
        case 7: ReadPos += 1; break;
        case 0x80: ReadPos += 1; break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case 5: ReadPos += 5; break;
    case 6: ReadPos += 1; break;
    case 7: ReadPos += 1; break;
    case 0x40: ReadPos += 2; break;
    case 0x41: ReadPos += 2; break;
    case 0x42: ReadPos += 2; break;
    case 0x43: ReadPos += 2; break;
    case 0x44: ReadPos += 3; break;
    case 0x46: ReadPos += 2; break;
    case 0x47: ReadPos += 2; break;
    case 0x80: ReadPos += 5; break;
    case 0x81: ReadPos += 5; break;
    case 0x82: ReadPos += 5; break;
    case 0x83: ReadPos += 5; break;
    case 0x86: ReadPos += 5; break;
    case 0x87: ReadPos += 5; break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    if (Reg == NULL)
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    switch (*TypePos)
    {
    case 0x0F:
        switch (*(TypePos + 1))
        {
        case 0xB6:
            if (!LB_NonMemory(MemAddress, (uint32_t *)Reg, false))
            {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load byte\n\nMIPS Address: %08X\nX86 Address: %08X",
                        (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
            return EXCEPTION_CONTINUE_EXECUTION;
        case 0xB7:
            if (!LH_NonMemory(MemAddress, (uint32_t *)Reg, false))
            {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nX86 Address: %08X",
                        (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
            return EXCEPTION_CONTINUE_EXECUTION;
        case 0xBE:
            if (!LB_NonMemory(MemAddress, (uint32_t *)Reg, true))
            {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load byte\n\nMIPS Address: %08X\nX86 Address: %08X",
                        (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
            return EXCEPTION_CONTINUE_EXECUTION;
        case 0xBF:
            if (!LH_NonMemory(MemAddress, (uint32_t *)Reg, true))
            {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nX86 Address: %08X",
                        (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
            return EXCEPTION_CONTINUE_EXECUTION;
        default:
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }
        break;
    case 0x66:
        switch (*(TypePos + 1))
        {
        case 0x8B:
            if (!LH_NonMemory(MemAddress, (uint32_t *)Reg, false))
            {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to half word\n\nMIPS Address: %08X\nX86 Address: %08X",
                        (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
            return EXCEPTION_CONTINUE_EXECUTION;
        case 0x89:
            if (!SH_NonMemory(MemAddress, *(uint16_t *)Reg))
            {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
            return EXCEPTION_CONTINUE_EXECUTION;
        case 0xC7:
            if (Reg != &lpEP->ContextRecord->Eax)
            {
                if (bHaveDebugger())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                return EXCEPTION_EXECUTE_HANDLER;
            }
            if (!SH_NonMemory(MemAddress, *(uint16_t *)ReadPos)) {
                if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress,
                        (uint8_t *)lpEP->ContextRecord->Eip).c_str());
                }
            }
            lpEP->ContextRecord->Eip = (uint32_t)(ReadPos + 2);
            return EXCEPTION_CONTINUE_EXECUTION;
        default:
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }
        break;
    case 0x88:
        if (!SB_NonMemory(MemAddress, *(uint8_t *)Reg))
        {
            if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nX86 Address: %08X",
                    (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                    (uint8_t *)lpEP->ContextRecord->Eip).c_str());
            }
        }
        lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
        return EXCEPTION_CONTINUE_EXECUTION;
    case 0x8A:
        if (!LB_NonMemory(MemAddress, (uint32_t *)Reg, false))
        {
            if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load byte\n\nMIPS Address: %08X\nX86 Address: %08X",
                    (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                    (uint8_t *)lpEP->ContextRecord->Eip).c_str());
            }
        }
        lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
        return EXCEPTION_CONTINUE_EXECUTION;
    case 0x8B:
        if (!LW_NonMemory(MemAddress, (uint32_t *)Reg))
        {
            if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nX86 Address: %08X",
                    (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
                    (uint8_t *)lpEP->ContextRecord->Eip).c_str());
            }
        }
        lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
        return EXCEPTION_CONTINUE_EXECUTION;
    case 0x89:
        if (!SW_NonMemory(MemAddress, *(uint32_t *)Reg))
        {
            if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress,
                    (uint8_t *)lpEP->ContextRecord->Eip).c_str());
            }
        }
        lpEP->ContextRecord->Eip = (uint32_t)ReadPos;
        return EXCEPTION_CONTINUE_EXECUTION;
    case 0xC6:
        if (Reg != &lpEP->ContextRecord->Eax)
        {
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }
        if (!SB_NonMemory(MemAddress, *(uint8_t *)ReadPos))
        {
            if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress,
                    (uint8_t *)lpEP->ContextRecord->Eip).c_str());
            }
        }
        lpEP->ContextRecord->Eip = (uint32_t)(ReadPos + 1);
        return EXCEPTION_CONTINUE_EXECUTION;
    case 0xC7:
        if (Reg != &lpEP->ContextRecord->Eax)
        {
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }
        if (!SW_NonMemory(MemAddress, *(uint32_t *)ReadPos))
        {
            if (g_Settings->LoadDword(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress,
                    (uint8_t *)lpEP->ContextRecord->Eip).c_str());
            }
        }
        lpEP->ContextRecord->Eip = (uint32_t)(ReadPos + 4);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (bHaveDebugger())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#else
    g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif