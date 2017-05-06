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
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>
#ifndef _WIN32
#include <stdlib.h>
#endif
#ifdef __arm__
#include <Project64-core/N64System/Recompiler/Arm/ArmOpCode.h>
#endif

#if defined(__i386__) || defined(_M_IX86)

bool CMipsMemoryVM::FilterX86Exception(uint32_t MemAddress, X86_CONTEXT & context)
{
    WriteTrace(TraceExceptionHandler, TraceVerbose, "MemAddress: %X", MemAddress);
    uint32_t * Reg;

    if (g_MMU == NULL)
    {
        WriteTrace(TraceExceptionHandler, TraceError, "g_MMU == NULL");
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }

    if ((int32_t)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF)
    {
        WriteTrace(TraceExceptionHandler, TraceError, "Invalid memory adderess: %X", MemAddress);
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return false;
    }

    uint8_t * TypePos = (uint8_t *)*(context.Eip);
    WriteTrace(TraceExceptionHandler, TraceVerbose, "TypePos[0] = %02X TypePos[1] = %02X", TypePos[0], TypePos[2]);

    Reg = NULL;
    if (*TypePos == 0xF3 && (*(TypePos + 1) == 0xA4 || *(TypePos + 1) == 0xA5))
    {
        uint32_t Start = (*context.Edi - (uint32_t)g_MMU->m_RDRAM);
        uint32_t End = Start + *context.Ecx;
        if ((int32_t)Start < 0)
        {
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return false;
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
        if (End < g_MMU->RdramSize() && g_Recompiler)
        {
            for (uint32_t count = (Start & ~0xFFF); count < End; count += 0x1000)
            {
                g_Recompiler->ClearRecompCode_Phys(count, 0x1000, CRecompiler::Remove_ProtectedMem);
            }
            return true;
        }
        if (Start >= 0x04000000 && End < 0x04002000 && g_Recompiler)
        {
            g_Recompiler->ClearRecompCode_Phys(Start & ~0xFFF, 0x1000, CRecompiler::Remove_ProtectedMem);
            return true;
        }
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return false;
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
    case 0x00: Reg = context.Eax; break;
    case 0x08: Reg = context.Ecx; break;
    case 0x10: Reg = context.Edx; break;
    case 0x18: Reg = context.Ebx; break;
    case 0x20: Reg = context.Esp; break;
    case 0x28: Reg = context.Ebp; break;
    case 0x30: Reg = context.Esi; break;
    case 0x38: Reg = context.Edi; break;
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
        return false;
    }

    if (Reg == NULL)
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return false;
    }

    switch (*TypePos)
    {
    case 0x0F:
        switch (*(TypePos + 1))
        {
        case 0xB6:
            if (!g_MMU->LB_NonMemory(MemAddress, (uint32_t *)Reg, false))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load byte\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)ReadPos;
            return true;
        case 0xB7:
            if (!g_MMU->LH_NonMemory(MemAddress, (uint32_t *)Reg, false))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)ReadPos;
            return true;
        case 0xBE:
            if (!g_MMU->LB_NonMemory(MemAddress, (uint32_t *)Reg, true))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load byte\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)ReadPos;
            return true;
        case 0xBF:
            if (!g_MMU->LH_NonMemory(MemAddress, (uint32_t *)Reg, true))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)ReadPos;
            return true;
        default:
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return false;
        }
        break;
    case 0x66:
        switch (*(TypePos + 1))
        {
        case 0x8B:
            if (!g_MMU->LH_NonMemory(MemAddress, (uint32_t *)Reg, false))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)ReadPos;
            return true;
        case 0x89:
            if (!g_MMU->SH_NonMemory(MemAddress, *(uint16_t *)Reg))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress,
                        (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)ReadPos;
            return true;
        case 0xC7:
            if (Reg != context.Eax)
            {
                if (bHaveDebugger())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                return false;
            }
            if (!g_MMU->SH_NonMemory(MemAddress, *(uint16_t *)ReadPos))
            {
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
                }
            }
            *context.Eip = (uint32_t)(ReadPos + 2);
            return true;
        default:
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return false;
        }
        break;
    case 0x88:
        if (!g_MMU->SB_NonMemory(MemAddress, *(uint8_t *)Reg))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
            }
        }
        *context.Eip = (uint32_t)ReadPos;
        return true;
    case 0x8A:
        if (!g_MMU->LB_NonMemory(MemAddress, (uint32_t *)Reg, false))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load byte\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
            }
        }
        *context.Eip = (uint32_t)ReadPos;
        return true;
    case 0x8B:
        if (!g_MMU->LW_NonMemory(MemAddress, (uint32_t *)Reg))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
            }
        }
        *context.Eip = (uint32_t)ReadPos;
        return true;
    case 0x89:
        if (!g_MMU->SW_NonMemory(MemAddress, *(uint32_t *)Reg))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
            }
        }
        *context.Eip = (uint32_t)ReadPos;
        return true;
    case 0xC6:
        if (Reg != context.Eax)
        {
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return false;
        }
        if (!g_MMU->SB_NonMemory(MemAddress, *(uint8_t *)ReadPos))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
            }
        }
        *context.Eip = (uint32_t)(ReadPos + 1);
        return true;
    case 0xC7:
        if (Reg != context.Eax)
        {
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return false;
        }
        if (!g_MMU->SW_NonMemory(MemAddress, *(uint32_t *)ReadPos))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nX86 Address: %08X", MemAddress, (uint8_t *)*context.Eip).c_str());
            }
        }
        *context.Eip = (uint32_t)(ReadPos + 4);
        return true;
    }
    if (bHaveDebugger())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}
#endif

#ifdef __arm__
bool CMipsMemoryVM::DumpArmExceptionInfo(uint32_t MemAddress, mcontext_t & context)
{
    ArmThumbOpcode * OpCode = (ArmThumbOpcode *)context.arm_pc;
    Arm32Opcode * OpCode32 = (Arm32Opcode *)context.arm_pc;
    WriteTrace(TraceExceptionHandler, TraceError, "Program Counter 0x%lx", g_Reg->m_PROGRAM_COUNTER);
    for (int i = 0, n = (sizeof(g_BaseSystem->m_LastSuccessSyncPC) / sizeof(g_BaseSystem->m_LastSuccessSyncPC[0])); i < n; i++)
    {
        WriteTrace(TraceExceptionHandler, TraceError, "m_LastSuccessSyncPC[%d] = 0x%lx", i, g_BaseSystem->m_LastSuccessSyncPC[i]);
    }
    WriteTrace(TraceExceptionHandler, TraceError, "MemAddress = 0x%lx", MemAddress);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r0 = 0x%lx", context.arm_r0);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r1 = 0x%lx", context.arm_r1);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r2 = 0x%lx", context.arm_r2);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r3 = 0x%lx", context.arm_r3);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r4 = 0x%lx", context.arm_r4);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r5 = 0x%lx", context.arm_r5);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r6 = 0x%lx", context.arm_r6);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r7 = 0x%lx", context.arm_r7);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r8 = 0x%lx", context.arm_r8);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r9 = 0x%lx", context.arm_r9);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_r10 = 0x%lx", context.arm_r10);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_fp = 0x%lx", context.arm_fp);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_ip = 0x%lx", context.arm_ip);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_sp = 0x%lx", context.arm_sp);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_lr = 0x%lx", context.arm_lr);
    WriteTrace(TraceExceptionHandler, TraceError, "uc->uc_mcontext.arm_pc = 0x%lx", context.arm_pc);

    uint8_t * TypePos = (uint8_t *)context.arm_pc;
    WriteTrace(TraceExceptionHandler, TraceError, "TypePos: %02X %02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0], TypePos[1], TypePos[2], TypePos[3], TypePos[4], TypePos[5], TypePos[6], TypePos[7], TypePos[8]);

    TypePos = (uint8_t *)context.arm_pc - 0x40;
    WriteTrace(TraceExceptionHandler, TraceError, "code:");
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x00], TypePos[0x01], TypePos[0x02], TypePos[0x03], TypePos[0x04], TypePos[0x05], TypePos[0x06], TypePos[0x07]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x08], TypePos[0x09], TypePos[0x0A], TypePos[0x0B], TypePos[0x0C], TypePos[0x0D], TypePos[0x0E], TypePos[0x0F]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x10], TypePos[0x11], TypePos[0x12], TypePos[0x13], TypePos[0x14], TypePos[0x15], TypePos[0x16], TypePos[0x17]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x18], TypePos[0x19], TypePos[0x1A], TypePos[0x1B], TypePos[0x1C], TypePos[0x1D], TypePos[0x1E], TypePos[0x1F]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x20], TypePos[0x21], TypePos[0x22], TypePos[0x23], TypePos[0x24], TypePos[0x25], TypePos[0x26], TypePos[0x27]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x28], TypePos[0x29], TypePos[0x2A], TypePos[0x2B], TypePos[0x2C], TypePos[0x2D], TypePos[0x2E], TypePos[0x2F]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x30], TypePos[0x31], TypePos[0x32], TypePos[0x33], TypePos[0x34], TypePos[0x35], TypePos[0x36], TypePos[0x37]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x38], TypePos[0x39], TypePos[0x3A], TypePos[0x3B], TypePos[0x3C], TypePos[0x3D], TypePos[0x3E], TypePos[0x3F]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x40], TypePos[0x41], TypePos[0x42], TypePos[0x43], TypePos[0x44], TypePos[0x45], TypePos[0x46], TypePos[0x47]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x48], TypePos[0x49], TypePos[0x4A], TypePos[0x4B], TypePos[0x4C], TypePos[0x4D], TypePos[0x4E], TypePos[0x4F]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x50], TypePos[0x51], TypePos[0x52], TypePos[0x53], TypePos[0x54], TypePos[0x55], TypePos[0x56], TypePos[0x57]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x58], TypePos[0x59], TypePos[0x5A], TypePos[0x5B], TypePos[0x5C], TypePos[0x5D], TypePos[0x5E], TypePos[0x5F]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x60], TypePos[0x61], TypePos[0x62], TypePos[0x63], TypePos[0x64], TypePos[0x65], TypePos[0x66], TypePos[0x67]);
    WriteTrace(TraceExceptionHandler, TraceError, "%02X %02X %02X %02X %02X %02X %02X %02X", TypePos[0x68], TypePos[0x69], TypePos[0x6A], TypePos[0x6B], TypePos[0x6C], TypePos[0x6D], TypePos[0x6E], TypePos[0x6F]);

    WriteTrace(TraceExceptionHandler, TraceError, "OpCode.Hex: %X", OpCode->Hex);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode.opcode: %X", OpCode->Reg.opcode);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode.rm: %X", OpCode->Reg.rm);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode.rn: %X", OpCode->Reg.rn);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode.rt: %X", OpCode->Reg.rt);

    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32.Hex: %X", OpCode32->Hex);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint16.opcode: %X", OpCode32->uint16.opcode);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint16.rm: %X", OpCode32->uint16.rm);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint16.rn: %X", OpCode32->uint16.rn);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint16.rt: %X", OpCode32->uint16.rt);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint16.imm2: %X", OpCode32->uint16.imm2);

    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint32.opcode: %X", OpCode32->uint32.opcode);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint32.rn: %X", OpCode32->uint32.rn);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint32.rt: %X", OpCode32->uint32.rt);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint32.opcode2: %X", OpCode32->uint32.opcode2);
    WriteTrace(TraceExceptionHandler, TraceError, "OpCode32->uint32.rm: %X", OpCode32->uint32.rm);

    for (int count = 0; count < 32; count++)
    {
        WriteTrace(TraceExceptionHandler, TraceError, "GPR[%s] 0x%08X%08X", CRegName::GPR[count], g_Reg->m_GPR[count].W[1], g_Reg->m_GPR[count].W[0]);
    }
    Flush_Recompiler_Log();
    TraceFlushLog();
}

bool CMipsMemoryVM::FilterArmException(uint32_t MemAddress, mcontext_t & context)
{
    if ((int32_t)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF)
    {
        WriteTrace(TraceExceptionHandler, TraceError, "Invalid memory adderess: %X", MemAddress);
        DumpArmExceptionInfo(MemAddress, context);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }

    uint32_t * ArmRegisters[16] =
    {
        (uint32_t*)&context.arm_r0, (uint32_t*)&context.arm_r1, (uint32_t*)&context.arm_r2, (uint32_t*)&context.arm_r3,
        (uint32_t*)&context.arm_r4, (uint32_t*)&context.arm_r5, (uint32_t*)&context.arm_r6, (uint32_t*)&context.arm_r7,
        (uint32_t*)&context.arm_r8, (uint32_t*)&context.arm_r9, (uint32_t*)&context.arm_r10,(uint32_t*)&context.arm_fp,
        (uint32_t*)&context.arm_ip, (uint32_t*)&context.arm_sp, (uint32_t*)&context.arm_lr, (uint32_t*)&context.arm_pc,
    };

    ArmThumbOpcode * OpCode = (ArmThumbOpcode *)context.arm_pc;
    Arm32Opcode * OpCode32 = (Arm32Opcode *)context.arm_pc;
    if (OpCode->Reg.opcode == ArmLDR_Reg)
    {
        if (!g_MMU->LW_NonMemory(MemAddress, ArmRegisters[OpCode->Reg.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }
    if (OpCode->Reg.opcode == ArmSTR_Reg)
    {
        if (!g_MMU->SW_NonMemory(MemAddress, *ArmRegisters[OpCode->Reg.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode32->imm2.opcode == 0xF84 && OpCode32->imm2.Opcode2 == 0)
    {
        //42 f8 03 c0 str.w	ip, [r2, r3]
        if (!g_MMU->SW_NonMemory(MemAddress, *ArmRegisters[OpCode32->imm2.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->imm12.opcode == 0xF8C)
    {
        //c9 f8 00 b0 str.w	r11, [r9]
        if (!g_MMU->SW_NonMemory(MemAddress, *ArmRegisters[OpCode32->imm2.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->imm12.opcode == 0xF8D)
    {
        //dc f8 70 70 ldr.w	r7, [ip, #112]
        if (!g_MMU->LW_NonMemory(MemAddress, ArmRegisters[OpCode32->imm12.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->reg_cond_imm5.opcode == 3 && OpCode32->reg_cond_imm5.opcode1 == 0 && OpCode32->reg_cond_imm5.opcode2 == 0 && OpCode32->reg_cond_imm5.opcode3 == 0)
    {
        //17847001 	strne	r7, [r4, r1]
        //e789300c 	str	r3, [r9, ip]
        if (!g_MMU->SW_NonMemory(MemAddress, *ArmRegisters[OpCode32->reg_cond_imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode->Reg.opcode == 0x2A) //STRB
    {
        if (!g_MMU->SB_NonMemory(MemAddress, *ArmRegisters[OpCode->Reg.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode32->reg_cond_imm5.opcode == 3 && OpCode32->reg_cond_imm5.opcode1 == 1 && OpCode32->reg_cond_imm5.opcode2 == 0 && OpCode32->reg_cond_imm5.opcode3 == 0)
    {
        //17c32001 	strbne	r2, [r3, r1]
        if (!g_MMU->SB_NonMemory(MemAddress, *ArmRegisters[OpCode32->reg_cond_imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->reg_cond_imm8.opcode == 0 && OpCode32->reg_cond_imm8.opcode1 == 1 && OpCode32->reg_cond_imm8.opcode2 == 0 && OpCode32->reg_cond_imm8.opcode3 == 0xB)
    {
        //11c020b0 	strhne	r2, [r0]
        if (!g_MMU->SH_NonMemory(MemAddress, *ArmRegisters[OpCode32->reg_cond_imm8.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode->Imm5.opcode == 0x10)
    {
        // 00 80 strh	r0, [r0, #0]
        if (!g_MMU->SH_NonMemory(MemAddress, *ArmRegisters[OpCode->Imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode->Reg.opcode == 0x29)
    {
        // 14 52 strh	r4, [r2, r0]
        if (!g_MMU->SH_NonMemory(MemAddress, *ArmRegisters[OpCode->Reg.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode->Imm5.opcode == 0xC)
    {
        //2e 60 str	r6, [r5, #0]
        if (!g_MMU->SW_NonMemory(MemAddress, *ArmRegisters[OpCode->Imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode->Imm5.opcode == 0xD)
    {
        //3F 68 ldr	r7, [r7, #0]
        if (!g_MMU->LW_NonMemory(MemAddress, ArmRegisters[OpCode->Imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode->Imm5.opcode == 0xE)
    {
        //b8 70 strb	r0, [r7, #2]
        if (!g_MMU->SB_NonMemory(MemAddress, *ArmRegisters[OpCode->Imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store byte\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 2;
        return true;
    }

    if (OpCode32->reg_cond.opcode == 0 && OpCode32->reg_cond.opcode1 == 0 && OpCode32->reg_cond.opcode2 == 0 && OpCode32->reg_cond.opcode3 == 0xB)
    {
        //118320b1 	strhne	r2, [r3, r1]
        if (!g_MMU->SH_NonMemory(MemAddress, *ArmRegisters[OpCode32->reg_cond.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->reg_cond_imm12.opcode == 2 && OpCode32->reg_cond_imm12.opcode1 == 0 && OpCode32->reg_cond_imm12.opcode2 == 0)
    {
        //e48a1004 	str	r1, [sl], #4
        if (!g_MMU->SW_NonMemory(MemAddress, *ArmRegisters[OpCode32->reg_cond_imm12.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to store word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->uint16.opcode == ArmLDRH_W)
    {
        //f833 c001 ldrh.w	ip, [r3, r1]
        if (!g_MMU->LH_NonMemory(MemAddress, ArmRegisters[OpCode32->uint16.rt], false))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->uint32.opcode == ArmLDRH_Reg && OpCode32->uint32.opcode2 == 0xB)
    {
        //e19a20b2 ldrh	r2, [sl, r2]
        if (!g_MMU->LH_NonMemory(MemAddress, ArmRegisters[OpCode32->uint32.rt], false))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->reg_cond.opcode == 0 && OpCode32->reg_cond.opcode1 == 0 && OpCode32->reg_cond.opcode2 == 1 && OpCode32->reg_cond.opcode3 == 0xB)
    {
        //119330b1 	ldrhne	r3, [r3, r1]
        if (!g_MMU->LH_NonMemory(MemAddress, ArmRegisters[OpCode32->reg_cond.rt], false))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load half word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->reg_cond_imm5.opcode == 3 && OpCode32->reg_cond_imm5.opcode1 == 0 && OpCode32->reg_cond_imm5.opcode2 == 1 && OpCode32->reg_cond_imm5.opcode3 == 0)
    {
        //1790a001 	ldrne	sl, [r0, r1]
        if (!g_MMU->LW_NonMemory(MemAddress, ArmRegisters[OpCode32->reg_cond_imm5.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    if (OpCode32->imm2.opcode == 0xF85 && OpCode32->imm2.Opcode2 == 0)
    {
        //52 f8 21 30 ldr.w	r3, [r2, r1, lsl #2]
        if (!g_MMU->LW_NonMemory(MemAddress, ArmRegisters[OpCode32->imm2.rt]))
        {
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("Failed to load word\n\nMIPS Address: %08X\nPC Address: %08X", MemAddress, context.arm_pc).c_str());
            }
        }
        context.arm_pc = context.arm_pc + 4;
        return true;
    }

    DumpArmExceptionInfo(MemAddress, context);
    return false;
}
#endif

#ifndef _WIN32
bool CMipsMemoryVM::SetupSegvHandler(void)
{
    struct sigaction sig_act;
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART;
    sig_act.sa_sigaction = segv_handler;
    return sigaction(SIGSEGV, &sig_act, NULL) == 0;
}

void CMipsMemoryVM::segv_handler(int signal, siginfo_t *siginfo, void *sigcontext)
{
    ucontext_t *ucontext = (ucontext_t*)sigcontext;

    WriteTrace(TraceExceptionHandler, TraceNotice, "Segmentation Fault!");
    WriteTrace(TraceExceptionHandler, TraceNotice, "info.si_signo = %d", signal);
    WriteTrace(TraceExceptionHandler, TraceNotice, "info.si_errno = %d", siginfo->si_errno);
    WriteTrace(TraceExceptionHandler, TraceNotice, "info.si_code  = %d", siginfo->si_code);
    WriteTrace(TraceExceptionHandler, TraceNotice, "info.si_addr  = %p", siginfo->si_addr);

    WriteTrace(TraceExceptionHandler, TraceNotice, "%s: si_addr: %p", __FUNCTION__, siginfo->si_addr);

    uint32_t MemAddress = (char *)siginfo->si_addr - (char *)g_MMU->Rdram();
    WriteTrace(TraceExceptionHandler, TraceNotice, "MemAddress = %X", MemAddress);
#ifdef __i386__
    for (int i = 0; i < NGREG; i++)
    {
        WriteTrace(TraceExceptionHandler, TraceNotice, "reg[%02d] = 0x%08x", i, ucontext->uc_mcontext.gregs[i]);
    }
    WriteTrace(TraceExceptionHandler, TraceNotice, "REG_EIP  = %X", ucontext->uc_mcontext.gregs[REG_EIP]);

    WriteTrace(TraceExceptionHandler, TraceNotice, "Data:");
    for (uint8_t * TypePos = (uint8_t *)ucontext->uc_mcontext.gregs[REG_EIP] - 0x200; TypePos < (uint8_t *)ucontext->uc_mcontext.gregs[REG_EIP] + 0x30; TypePos += 8)
    {
        WriteTrace(TraceExceptionHandler, TraceNotice, "%X: %02X %02X %02X %02X %02X %02X %02X %02X", (uint32_t)TypePos, TypePos[0], TypePos[1], TypePos[2], TypePos[3], TypePos[4], TypePos[5], TypePos[6], TypePos[7]);
    }

    X86_CONTEXT context;
    context.Edi = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_EDI];
    context.Esi = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_ESI];
    context.Ebx = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_EBX];
    context.Edx = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_EDX];
    context.Ecx = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_ECX];
    context.Eax = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_EAX];
    context.Eip = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_EIP];
    context.Esp = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_ESP];
    context.Ebp = (uint32_t*)&ucontext->uc_mcontext.gregs[REG_EBP];

    if (FilterX86Exception(MemAddress, context))
    {
        WriteTrace(TraceExceptionHandler, TraceNotice, "Success!");
        WriteTrace(TraceExceptionHandler, TraceNotice, "REG_EIP  = %X", ucontext->uc_mcontext.gregs[REG_EIP]);
        return;
    }
#elif defined(__arm__)
    if (FilterArmException(MemAddress, ucontext->uc_mcontext))
    {
        WriteTrace(TraceExceptionHandler, TraceNotice, "Success!");
        return;
    }
#endif
    WriteTrace(TraceExceptionHandler, TraceError, "Failed quiting now");
    exit(0); //can't return to main, it's where the segfault occured.
}

#else
int32_t CMipsMemoryVM::MemoryFilter(uint32_t dwExptCode, void * lpExceptionPointer)
{
#if defined(_M_IX86) && defined(_WIN32)
    if (dwExptCode != EXCEPTION_ACCESS_VIOLATION || g_MMU == NULL)
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    //convert the pointer since we are not having win32 structures in headers
    LPEXCEPTION_POINTERS lpEP = (LPEXCEPTION_POINTERS)lpExceptionPointer;
    uint32_t MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)g_MMU->Rdram();

    X86_CONTEXT context;
    context.Edi = (uint32_t*)&lpEP->ContextRecord->Edi;
    context.Esi = (uint32_t*)&lpEP->ContextRecord->Esi;
    context.Ebx = (uint32_t*)&lpEP->ContextRecord->Ebx;
    context.Edx = (uint32_t*)&lpEP->ContextRecord->Edx;
    context.Ecx = (uint32_t*)&lpEP->ContextRecord->Ecx;
    context.Eax = (uint32_t*)&lpEP->ContextRecord->Eax;
    context.Eip = (uint32_t*)&lpEP->ContextRecord->Eip;
    context.Esp = (uint32_t*)&lpEP->ContextRecord->Esp;
    context.Ebp = (uint32_t*)&lpEP->ContextRecord->Ebp;

    if (FilterX86Exception(MemAddress, context))
    {
        WriteTrace(TraceExceptionHandler, TraceNotice, "Success!");
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_EXECUTE_HANDLER;
#else
    return EXCEPTION_EXECUTE_HANDLER;
#endif
}
#endif