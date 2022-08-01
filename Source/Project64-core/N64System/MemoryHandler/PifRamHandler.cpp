#include "stdafx.h"
#include "PifRamHandler.h"
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\SystemGlobals.h>

PifRamHandler::PifRamHandler(CMipsMemoryVM & MMU, CRegisters & Reg) :
    m_MMU(MMU),
    m_PifRam(MMU.PifRam()),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool PifRamHandler::Read32(uint32_t Address, uint32_t & Value)
{
    Address &= 0x1FFFFFFF;
    if (Address < 0x1FC007C0)
    {
        //Value = swap32by8(*(uint32_t *)(&PifRom[PAddr - 0x1FC00000]));
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (Address < 0x1FC00800)
    {
        Value = *(uint32_t *)(&m_PifRam[Address - 0x1FC007C0]);
        Value = swap32by8(Value);
    }
    else
    {
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (GenerateLog() && LogPRDirectMemLoads() && Address >= 0x1FC007C0 && Address <= 0x1FC007FC)
    {
        LogMessage("%08X: read word from PIF RAM at 0x%X (%08X)", m_PC, Address - 0x1FC007C0, Value);
    }
    return true;
}

bool PifRamHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    Address &= 0x1FFFFFFF;
    if (GenerateLog() && LogPRDirectMemStores() && Address >= 0x1FC007C0 && Address <= 0x1FC007FC)
    {
        LogMessage("%08X: Writing 0x%08X to PIF RAM at 0x%X", m_PC, Value, Address - 0x1FC007C0);
    }

    if (Address < 0x1FC007C0)
    {
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (Address < 0x1FC00800)
    {
        uint32_t SwappedMask = swap32by8(Mask);
        Value = ((*(uint32_t *)(&m_PifRam[Address - 0x1FC007C0])) & ~SwappedMask) | (Value & SwappedMask);
        *(uint32_t *)(&m_PifRam[Address - 0x1FC007C0]) = Value;
        if (Address == 0x1FC007FC)
        {
            m_MMU.PifRamWrite();
        }
    }
    return true;
}

uint32_t PifRamHandler::swap32by8(uint32_t word)
{
    const uint32_t swapped =
#if defined(_MSC_VER)
        _byteswap_ulong(word)
#elif defined(__GNUC__)
        __builtin_bswap32(word)
#else
        (word & 0x000000FFul) << 24
        | (word & 0x0000FF00ul) << 8
        | (word & 0x00FF0000ul) >> 8
        | (word & 0xFF000000ul) >> 24
#endif
        ;
    return (swapped & 0xFFFFFFFFul);
}