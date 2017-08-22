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
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64RomClass.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/ExceptionHandler.h>

#include <stdio.h>
#include <Common/MemoryManagement.h>

uint8_t * CMipsMemoryVM::m_Reserve1 = NULL;
uint8_t * CMipsMemoryVM::m_Reserve2 = NULL;
uint32_t CMipsMemoryVM::m_MemLookupAddress = 0;
MIPS_DWORD CMipsMemoryVM::m_MemLookupValue;
bool CMipsMemoryVM::m_MemLookupValid = true;
uint32_t CMipsMemoryVM::RegModValue;

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

CMipsMemoryVM::CMipsMemoryVM(bool SavesReadOnly) :
    CPifRam(SavesReadOnly),
    CFlashram(SavesReadOnly),
    CSram(SavesReadOnly),
    CDMA(*this, *this),
    m_RomMapped(false),
    m_Rom(NULL),
    m_RomSize(0),
    m_RomWrittenTo(false),
    m_RomWroteValue(0),
    m_HalfLine(0),
    m_HalfLineCheck(false),
    m_FieldSerration(0),
    m_TLB_ReadMap(NULL),
    m_TLB_WriteMap(NULL),
    m_RDRAM(NULL),
    m_DMEM(NULL),
    m_IMEM(NULL),
    m_DDRomMapped(false),
    m_DDRom(NULL),
    m_DDRomSize(0)
{
    g_Settings->RegisterChangeCB(Game_RDRamSize, this, (CSettings::SettingChangedFunc)RdramChanged);
}

uint32_t swap32by8(uint32_t word)
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

CMipsMemoryVM::~CMipsMemoryVM()
{
    g_Settings->UnregisterChangeCB(Game_RDRamSize, this, (CSettings::SettingChangedFunc)RdramChanged);
    FreeMemory();
}

void CMipsMemoryVM::Reset(bool /*EraseMemory*/)
{
    if (m_TLB_ReadMap)
    {
        size_t address;

        memset(m_TLB_ReadMap, 0, 0xFFFFF * sizeof(size_t));
        memset(m_TLB_WriteMap, 0, 0xFFFFF * sizeof(size_t));
        for (address = 0x80000000; address < 0xC0000000; address += 0x1000)
        {
            m_TLB_ReadMap[address >> 12] = ((size_t)m_RDRAM + (address & 0x1FFFFFFF)) - address;
            m_TLB_WriteMap[address >> 12] = ((size_t)m_RDRAM + (address & 0x1FFFFFFF)) - address;
        }

        if (g_Settings->LoadDword(Rdb_TLB_VAddrStart) != 0)
        {
            size_t Start = g_Settings->LoadDword(Rdb_TLB_VAddrStart); //0x7F000000;
            size_t Len = g_Settings->LoadDword(Rdb_TLB_VAddrLen);   //0x01000000;
            size_t PAddr = g_Settings->LoadDword(Rdb_TLB_PAddrStart); //0x10034b30;
            size_t End = Start + Len;
            for (address = Start; address < End; address += 0x1000)
            {
                m_TLB_ReadMap[address >> 12] = ((size_t)m_RDRAM + (address - Start + PAddr)) - address;
                m_TLB_WriteMap[address >> 12] = ((size_t)m_RDRAM + (address - Start + PAddr)) - address;
            }
        }
    }
}

void CMipsMemoryVM::ReserveMemory()
{
    m_Reserve1 = (uint8_t *)AllocateAddressSpace(0x20000000, (void *)g_Settings->LoadDword(Setting_FixedRdramAddress));
    m_Reserve2 = (uint8_t *)AllocateAddressSpace(0x04002000);
}

void CMipsMemoryVM::FreeReservedMemory()
{
    if (m_Reserve1)
    {
        FreeAddressSpace(m_Reserve1, 0x20000000);
        m_Reserve1 = NULL;
    }
    if (m_Reserve2)
    {
        FreeAddressSpace(m_Reserve2, 0x20000000);
        m_Reserve2 = NULL;
    }
}

bool CMipsMemoryVM::Initialize(bool SyncSystem)
{
    if (m_RDRAM != NULL)
    {
        return true;
    }

    if (!SyncSystem && m_RDRAM == NULL && m_Reserve1 != NULL)
    {
        m_RDRAM = m_Reserve1;
        m_Reserve1 = NULL;
    }
    if (SyncSystem && m_RDRAM == NULL && m_Reserve2 != NULL)
    {
        m_RDRAM = m_Reserve2;
        m_Reserve2 = NULL;
    }
    if (m_RDRAM == NULL)
    {
        m_RDRAM = (uint8_t *)AllocateAddressSpace(0x20000000);
    }
    if (m_RDRAM == NULL)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to Reserve RDRAM (Size: 0x%X)", 0x20000000);
        FreeMemory();
        return false;
    }

    m_AllocatedRdramSize = g_Settings->LoadDword(Game_RDRamSize);
    if (CommitMemory(m_RDRAM, m_AllocatedRdramSize, MEM_READWRITE) == NULL)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to Allocate RDRAM (Size: 0x%X)", m_AllocatedRdramSize);
        FreeMemory();
        return false;
    }

    if (CommitMemory(m_RDRAM + 0x04000000, 0x2000, MEM_READWRITE) == NULL)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to Allocate DMEM/IMEM (Size: 0x%X)", 0x2000);
        FreeMemory();
        return false;
    }

    m_DMEM = (uint8_t *)(m_RDRAM + 0x04000000);
    m_IMEM = (uint8_t *)(m_RDRAM + 0x04001000);

    if (g_Settings->LoadBool(Game_LoadRomToMemory))
    {
        m_RomMapped = true;
        m_Rom = m_RDRAM + 0x10000000;
        m_RomSize = g_Rom->GetRomSize();
        if (CommitMemory(m_Rom, g_Rom->GetRomSize(), MEM_READWRITE) == NULL)
        {
            WriteTrace(TraceN64System, TraceError, "Failed to Allocate Rom (Size: 0x%X)", g_Rom->GetRomSize());
            FreeMemory();
            return false;
        }
        memcpy(m_Rom, g_Rom->GetRomAddress(), g_Rom->GetRomSize());

        ::ProtectMemory(m_Rom, g_Rom->GetRomSize(), MEM_READONLY);
    }
    else
    {
        m_RomMapped = false;
        m_Rom = g_Rom->GetRomAddress();
        m_RomSize = g_Rom->GetRomSize();
    }

    //64DD IPL
    if (g_DDRom != NULL)
    {
        if (g_Settings->LoadBool(Game_LoadRomToMemory))
        {
            m_DDRomMapped = true;
            m_DDRom = m_RDRAM + 0x06000000;
            m_DDRomSize = g_DDRom->GetRomSize();
            if (CommitMemory(m_DDRom, g_DDRom->GetRomSize(), MEM_READWRITE) == NULL)
            {
                WriteTrace(TraceN64System, TraceError, "Failed to Allocate Rom (Size: 0x%X)", g_DDRom->GetRomSize());
                FreeMemory();
                return false;
            }
            memcpy(m_DDRom, g_DDRom->GetRomAddress(), g_DDRom->GetRomSize());

            ::ProtectMemory(m_DDRom, g_DDRom->GetRomSize(), MEM_READONLY);
        }
        else
        {
            m_DDRomMapped = false;
            m_DDRom = g_DDRom->GetRomAddress();
            m_DDRomSize = g_DDRom->GetRomSize();
        }
    }

    CPifRam::Reset();

    m_TLB_ReadMap = new size_t[0x100000];
    if (m_TLB_ReadMap == NULL)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to Allocate m_TLB_ReadMap (Size: 0x%X)", 0x100000 * sizeof(size_t));
        FreeMemory();
        return false;
    }

    m_TLB_WriteMap = new size_t[0x100000];
    if (m_TLB_WriteMap == NULL)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to Allocate m_TLB_WriteMap (Size: 0x%X)", 0xFFFFF * sizeof(size_t));
        FreeMemory();
        return false;
    }
    Reset(false);
    return true;
}

void CMipsMemoryVM::FreeMemory()
{
    if (m_RDRAM)
    {
        if (DecommitMemory(m_RDRAM, 0x20000000))
        {
            if (m_Reserve1 == NULL)
            {
                m_Reserve1 = m_RDRAM;
            }
            else if (m_Reserve2 == NULL)
            {
                m_Reserve2 = m_RDRAM;
            }
            else
            {
                FreeAddressSpace(m_RDRAM, 0x20000000);
            }
        }
        else
        {
            FreeAddressSpace(m_RDRAM, 0x20000000);
        }
        m_RDRAM = NULL;
        m_IMEM = NULL;
        m_DMEM = NULL;
    }
    if (m_TLB_ReadMap)
    {
        delete[] m_TLB_ReadMap;
        m_TLB_ReadMap = NULL;
    }
    if (m_TLB_WriteMap)
    {
        delete[] m_TLB_WriteMap;
        m_TLB_WriteMap = NULL;
    }
    CPifRam::Reset();
}

uint8_t * CMipsMemoryVM::Rdram()
{
    return m_RDRAM;
}

uint32_t CMipsMemoryVM::RdramSize()
{
    return m_AllocatedRdramSize;
}

uint8_t * CMipsMemoryVM::Dmem()
{
    return m_DMEM;
}

uint8_t * CMipsMemoryVM::Imem()
{
    return m_IMEM;
}

uint8_t * CMipsMemoryVM::PifRam()
{
    return m_PifRam;
}

bool CMipsMemoryVM::LB_VAddr(uint32_t VAddr, uint8_t& Value)
{
    if (m_TLB_ReadMap[VAddr >> 12] == 0)
    {
        return false;
    }

    Value = *(uint8_t*)(m_TLB_ReadMap[VAddr >> 12] + (VAddr ^ 3));
    return true;
}

bool CMipsMemoryVM::LH_VAddr(uint32_t VAddr, uint16_t& Value)
{
    if (m_TLB_ReadMap[VAddr >> 12] == 0)
    {
        return false;
    }

    Value = *(uint16_t*)(m_TLB_ReadMap[VAddr >> 12] + (VAddr ^ 2));
    return true;
}

bool CMipsMemoryVM::LW_VAddr(uint32_t VAddr, uint32_t& Value)
{
    if (VAddr >= 0xA3F00000 && VAddr < 0xC0000000)
    {
        if ((VAddr & 0xFFFFE000ul) != 0xA4000000ul) // !(A4000000 <= addr < A4002000)
        {
            VAddr &= 0x1FFFFFFF;
            LW_NonMemory(VAddr, &Value);
            return true;
        }
    }

    uint8_t* BaseAddress = (uint8_t*)m_TLB_ReadMap[VAddr >> 12];
    if (BaseAddress == NULL)
    {
        return false;
    }

    Value = *(uint32_t*)(BaseAddress + VAddr);

    //	if (LookUpMode == FuncFind_ChangeMemory)
    //	{
    //		g_Notify->BreakPoint(__FILE__, __LINE__);
    //		if ( (Command.Hex >> 16) == 0x7C7C)
    //		{
    //			Command.Hex = OrigMem[(Command.Hex & 0xFFFF)].OriginalValue;
    //		}
    //	}
    return true;
}

bool CMipsMemoryVM::LD_VAddr(uint32_t VAddr, uint64_t& Value)
{
    if (m_TLB_ReadMap[VAddr >> 12] == 0)
    {
        return false;
    }

    *((uint32_t*)(&Value) + 1) = *(uint32_t*)(m_TLB_ReadMap[VAddr >> 12] + VAddr);
    *((uint32_t*)(&Value) + 0) = *(uint32_t*)(m_TLB_ReadMap[VAddr >> 12] + VAddr + 4);
    return true;
}

bool CMipsMemoryVM::LB_PAddr(uint32_t PAddr, uint8_t& Value)
{
    if (PAddr < RdramSize())
    {
        Value = *(uint8_t*)(m_RDRAM + (PAddr ^ 3));
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::LH_PAddr(uint32_t PAddr, uint16_t& Value)
{
    if (PAddr < RdramSize())
    {
        Value = *(uint16_t*)(m_RDRAM + (PAddr ^ 2));
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::LW_PAddr(uint32_t PAddr, uint32_t& Value)
{
    if (PAddr < RdramSize())
    {
        Value = *(uint32_t*)(m_RDRAM + PAddr);
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::LD_PAddr(uint32_t PAddr, uint64_t& Value)
{
    if (PAddr < RdramSize())
    {
        *((uint32_t*)(&Value) + 1) = *(uint32_t*)(m_RDRAM + PAddr);
        *((uint32_t*)(&Value) + 0) = *(uint32_t*)(m_RDRAM + PAddr + 4);
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::SB_VAddr(uint32_t VAddr, uint8_t Value)
{
    if (m_TLB_WriteMap[VAddr >> 12] == 0)
    {
        return false;
    }

    *(uint8_t*)(m_TLB_WriteMap[VAddr >> 12] + (VAddr ^ 3)) = Value;
    return true;
}

bool CMipsMemoryVM::SH_VAddr(uint32_t VAddr, uint16_t Value)
{
    if (m_TLB_WriteMap[VAddr >> 12] == 0)
    {
        return false;
    }

    *(uint16_t*)(m_TLB_WriteMap[VAddr >> 12] + (VAddr ^ 2)) = Value;
    return true;
}

bool CMipsMemoryVM::SW_VAddr(uint32_t VAddr, uint32_t Value)
{
    if (VAddr >= 0xA3F00000 && VAddr < 0xC0000000)
    {
        if ((VAddr & 0xFFFFE000ul) != 0xA4000000ul) // !(A4000000 <= addr < A4002000)
        {
            VAddr &= 0x1FFFFFFF;
            SW_NonMemory(VAddr, Value);
            return true;
        }
    }

    if (m_TLB_WriteMap[VAddr >> 12] == 0)
    {
        return false;
    }

    *(uint32_t*)(m_TLB_WriteMap[VAddr >> 12] + VAddr) = Value;
    return true;
}

bool CMipsMemoryVM::SD_VAddr(uint32_t VAddr, uint64_t Value)
{
    if (m_TLB_WriteMap[VAddr >> 12] == 0)
    {
        return false;
    }

    *(uint32_t*)(m_TLB_WriteMap[VAddr >> 12] + VAddr + 0) = *((uint32_t*)(&Value) + 1);
    *(uint32_t*)(m_TLB_WriteMap[VAddr >> 12] + VAddr + 4) = *((uint32_t*)(&Value));
    return true;
}

bool CMipsMemoryVM::SB_PAddr(uint32_t PAddr, uint8_t Value)
{
    if (PAddr < RdramSize())
    {
        *(uint8_t*)(m_RDRAM + (PAddr ^ 3)) = Value;
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::SH_PAddr(uint32_t PAddr, uint16_t Value)
{
    if (PAddr < RdramSize())
    {
        *(uint16_t*)(m_RDRAM + (PAddr ^ 2)) = Value;
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::SW_PAddr(uint32_t PAddr, uint32_t Value)
{
    if (PAddr < RdramSize())
    {
        *(uint32_t*)(m_RDRAM + PAddr) = Value;
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::SD_PAddr(uint32_t PAddr, uint64_t Value)
{
    if (PAddr < RdramSize())
    {
        *(uint32_t*)(m_RDRAM + PAddr + 0) = *((uint32_t*)(&Value) + 1);
        *(uint32_t*)(m_RDRAM + PAddr + 4) = *((uint32_t*)(&Value));
        return true;
    }

    if (PAddr > 0x18000000)
    {
        return false;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CMipsMemoryVM::ValidVaddr(uint32_t VAddr) const
{
    return m_TLB_ReadMap[VAddr >> 12] != 0;
}

bool CMipsMemoryVM::VAddrToRealAddr(uint32_t VAddr, void * &RealAddress) const
{
    if (m_TLB_ReadMap[VAddr >> 12] == 0)
    {
        return false;
    }
    RealAddress = (uint8_t *)(m_TLB_ReadMap[VAddr >> 12] + VAddr);
    return true;
}

bool CMipsMemoryVM::TranslateVaddr(uint32_t VAddr, uint32_t &PAddr) const
{
    //Change the Virtual address to a Physical Address
    if (m_TLB_ReadMap[VAddr >> 12] == 0)
    {
        return false;
    }
    PAddr = (uint32_t)((uint8_t *)(m_TLB_ReadMap[VAddr >> 12] + VAddr) - m_RDRAM);
    return true;
}

bool CMipsMemoryVM::LB_NonMemory(uint32_t PAddr, uint32_t* Value, bool /*SignExtend*/)
{
    if (PAddr < 0x800000)
    {
        *Value = 0;
        return true;
    }

    if (PAddr >= 0x10000000 && PAddr < 0x16000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
        if (WrittenToRom)
        {
            return false;
        }

        if ((PAddr & 2) == 0)
        {
            PAddr = (PAddr + 4) ^ 2;
        }

        if ((PAddr - 0x10000000) < RomFileSize)
        {
            if (SignExtend)
            {
                *Value = (int32_t)((char)ROM[PAddr - 0x10000000]);
            }
            else
            {
                *Value = ROM[PAddr - 0x10000000];
            }

            return true;
        }
        else
        {
            *Value = 0;
            return false;
        }
#endif
    }
    //	switch (PAddr & 0xFFF00000)
    //{
    //	default:
    *Value = 0;
    //		return false;
    //		break;
    //	}
    return true;
}

bool CMipsMemoryVM::LH_NonMemory(uint32_t PAddr, uint32_t* Value, bool/* SignExtend*/)
{
    if (PAddr < 0x800000)
    {
        *Value = 0;
        return true;
    }

    if (PAddr >= 0x10000000 && PAddr < 0x16000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    //	switch (PAddr & 0xFFF00000)
    //	{
    //	default:
    *Value = 0;
    return false;
    //	}
    //	return true;
}

bool CMipsMemoryVM::LW_NonMemory(uint32_t PAddr, uint32_t* Value)
{
#ifdef CFB_READ
    if (PAddr >= CFBStart && PAddr < CFBEnd)
    {
        uint32_t OldProtect;
        VirtualProtect(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, PAGE_READONLY, &OldProtect);
        if (FrameBufferRead)
        {
            FrameBufferRead(PAddr & ~0xFFF);
        }
        *Value = *(uint32_t *)(m_RDRAM + PAddr);
        return true;
    }
#endif

    m_MemLookupAddress = PAddr;
    if (PAddr >= 0x10000000 && PAddr < 0x16000000)
    {
        Load32Rom();
    }
    else
    {
        switch (PAddr & 0xFFF00000)
        {
        case 0x03F00000: Load32RDRAMRegisters(); break;
        case 0x04000000: Load32SPRegisters(); break;
        case 0x04100000: Load32DPCommand(); break;
        case 0x04300000: Load32MIPSInterface(); break;
        case 0x04400000: Load32VideoInterface(); break;
        case 0x04500000: Load32AudioInterface(); break;
        case 0x04600000: Load32PeripheralInterface(); break;
        case 0x04700000: Load32RDRAMInterface(); break;
        case 0x04800000: Load32SerialInterface(); break;
        case 0x05000000: Load32CartridgeDomain2Address1(); break;
        case 0x06000000: Load32CartridgeDomain1Address1(); break;
        case 0x08000000: Load32CartridgeDomain2Address2(); break;
        case 0x1FC00000: Load32PifRam(); break;
        case 0x1FF00000: Load32CartridgeDomain1Address3(); break;
        default:
            m_MemLookupValue.UW[0] = PAddr & 0xFFFF;
            m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
        }
    }
    *Value = m_MemLookupValue.UW[0];
    return true;
}

bool CMipsMemoryVM::SB_NonMemory(uint32_t PAddr, uint8_t Value)
{
    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
#ifdef CFB_READ
        if (PAddr >= CFBStart && PAddr < CFBEnd)
        {
            uint32_t OldProtect;
            VirtualProtect(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, PAGE_READWRITE, &OldProtect);
            *(uint8_t *)(m_RDRAM + PAddr) = Value;
            VirtualProtect(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, OldProtect, &OldProtect);
            g_Notify->DisplayError("FrameBufferWrite");
            if (FrameBufferWrite) { FrameBufferWrite(PAddr, 1); }
            break;
        }
#endif
        if (PAddr < RdramSize())
        {
            g_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF, 0xFFC, CRecompiler::Remove_ProtectedMem);
            ::ProtectMemory(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, MEM_READWRITE);
            *(uint8_t *)(m_RDRAM + PAddr) = Value;
        }
        break;
    default:
        return false;
    }

    return true;
}

bool CMipsMemoryVM::SH_NonMemory(uint32_t PAddr, uint16_t Value)
{
    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
#ifdef CFB_READ
        if (PAddr >= CFBStart && PAddr < CFBEnd)
        {
            uint32_t OldProtect;
            VirtualProtect(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, PAGE_READWRITE, &OldProtect);
            *(uint16_t *)(m_RDRAM + PAddr) = Value;
            if (FrameBufferWrite) { FrameBufferWrite(PAddr & ~0xFFF, 2); }
            //*(uint16_t *)(m_RDRAM+PAddr) = 0xFFFF;
            //VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_NOACCESS, &OldProtect);
            g_Notify->DisplayError("PAddr = %x", PAddr);
            break;
        }
#endif
        if (PAddr < RdramSize())
        {
            g_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF, 0x1000, CRecompiler::Remove_ProtectedMem);
            ::ProtectMemory(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, MEM_READWRITE);
            *(uint16_t *)(m_RDRAM + PAddr) = Value;
        }
        break;
    default:
        return false;
    }

    return true;
}

bool CMipsMemoryVM::SW_NonMemory(uint32_t PAddr, uint32_t Value)
{
    m_MemLookupValue.UW[0] = Value;
    m_MemLookupAddress = PAddr;

    if (PAddr >= 0x10000000 && PAddr < 0x16000000)
    {
        if ((PAddr - 0x10000000) < g_Rom->GetRomSize())
        {
            m_RomWrittenTo = true;
            m_RomWroteValue = Value;
#ifdef ROM_IN_MAPSPACE
            {
                uint32_t OldProtect;
                VirtualProtect(ROM, RomFileSize, PAGE_NOACCESS, &OldProtect);
            }
#endif
            //LogMessage("%X: Wrote To Rom %08X from %08X",PROGRAM_COUNTER,Value,PAddr);
        }
        else
        {
            return false;
        }
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
#ifdef CFB_READ
        if (PAddr >= CFBStart && PAddr < CFBEnd)
        {
            uint32_t OldProtect;
            VirtualProtect(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, PAGE_READWRITE, &OldProtect);
            *(uint32_t *)(m_RDRAM + PAddr) = Value;
            VirtualProtect(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, OldProtect, &OldProtect);
            g_Notify->DisplayError("FrameBufferWrite %X", PAddr);
            if (FrameBufferWrite) { FrameBufferWrite(PAddr, 4); }
            break;
        }
#endif
        if (PAddr < RdramSize())
        {
            g_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF, 0x1000, CRecompiler::Remove_ProtectedMem);
            ::ProtectMemory(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, MEM_READWRITE);
            *(uint32_t *)(m_RDRAM + PAddr) = Value;
        }
        break;
    case 0x03F00000: Write32RDRAMRegisters(); break;
    case 0x04000000:
        if (PAddr < 0x04002000)
        {
            g_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF, 0xFFF, CRecompiler::Remove_ProtectedMem);
            *(uint32_t *)(m_RDRAM + PAddr) = Value;
        }
        else
        {
            Write32SPRegisters();
        }
        break;
    case 0x04100000: Write32DPCommandRegisters(); break;
    case 0x04300000: Write32MIPSInterface(); break;
    case 0x04400000: Write32VideoInterface(); break;
    case 0x04500000: Write32AudioInterface(); break;
    case 0x04600000: Write32PeripheralInterface(); break;
    case 0x04700000: Write32RDRAMInterface(); break;
    case 0x04800000: Write32SerialInterface(); break;
    case 0x05000000: Write32CartridgeDomain2Address1(); break;
    case 0x08000000: Write32CartridgeDomain2Address2(); break;
    case 0x1FC00000: Write32PifRam(); break;
    default:
        return false;
        break;
    }

    return true;
}

void CMipsMemoryVM::UpdateHalfLine()
{
    uint32_t NextViTimer = g_SystemTimer->GetTimer(CSystemTimer::ViTimer);

    if (*g_NextTimer < 0)
    {
        m_HalfLine = 0;
        return;
    }

    int32_t check_value = (int32_t)(m_HalfLineCheck - NextViTimer);
    if (check_value > 0 && check_value < 40)
    {
        *g_NextTimer -= g_System->ViRefreshRate();
        if (*g_NextTimer < 0)
        {
            *g_NextTimer = 0 - g_System->CountPerOp();
        }
        g_SystemTimer->UpdateTimers();
        NextViTimer = g_SystemTimer->GetTimer(CSystemTimer::ViTimer);
    }
    m_HalfLine = (uint32_t)(*g_NextTimer / g_System->ViRefreshRate());
    m_HalfLine &= ~1;
    m_HalfLine |= m_FieldSerration;
    g_Reg->VI_V_CURRENT_LINE_REG = m_HalfLine;
    m_HalfLineCheck = NextViTimer;
}

void CMipsMemoryVM::UpdateFieldSerration(uint32_t interlaced)
{
    m_FieldSerration ^= 1;
    m_FieldSerration &= interlaced;
}

void CMipsMemoryVM::ProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr)
{
    WriteTrace(TraceProtectedMem, TraceDebug, "StartVaddr: %08X EndVaddr: %08X", StartVaddr, EndVaddr);
    if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr))
    {
        return;
    }

    //Get Physical Addresses passed
    uint32_t StartPAddr, EndPAddr;
    if (!TranslateVaddr(StartVaddr, StartPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (!TranslateVaddr(EndVaddr, EndPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    //Get Length of memory being protected
    int32_t Length = ((EndPAddr + 3) - StartPAddr) & ~3;
    if (Length < 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    //Protect that memory address space
    uint8_t * MemLoc = Rdram() + StartPAddr;
    WriteTrace(TraceProtectedMem, TraceDebug, "Paddr: %08X Length: %X", StartPAddr, Length);

    ::ProtectMemory(MemLoc, Length, MEM_READONLY);
}

void CMipsMemoryVM::UnProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr)
{
    WriteTrace(TraceProtectedMem, TraceDebug, "StartVaddr: %08X EndVaddr: %08X", StartVaddr, EndVaddr);
    if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr)) { return; }

    //Get Physical Addresses passed
    uint32_t StartPAddr, EndPAddr;
    if (!TranslateVaddr(StartVaddr, StartPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (!TranslateVaddr(EndVaddr, EndPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    //Get Length of memory being protected
    int32_t Length = ((EndPAddr + 3) - StartPAddr) & ~3;
    if (Length < 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    //Protect that memory address space
    uint8_t * MemLoc = Rdram() + StartPAddr;
    ::ProtectMemory(MemLoc, Length, MEM_READWRITE);
}

const char * CMipsMemoryVM::LabelName(uint32_t Address) const
{
    //StringMap::iterator theIterator = m_LabelList.find(Address);
    //if (theIterator != m_LabelList.end())
    //{
    //	return (*theIterator).second;
    //}

    sprintf(m_strLabelName, "0x%08X", Address);
    return m_strLabelName;
}

void CMipsMemoryVM::TLB_Mapped(uint32_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly)
{
    size_t count, VEnd;

    VEnd = VAddr + Len;
    for (count = VAddr; count < VEnd; count += 0x1000)
    {
        size_t Index = count >> 12;
        m_TLB_ReadMap[Index] = ((size_t)m_RDRAM + (count - VAddr + PAddr)) - count;
        if (!bReadOnly)
        {
            m_TLB_WriteMap[Index] = ((size_t)m_RDRAM + (count - VAddr + PAddr)) - count;
        }
    }
}

void CMipsMemoryVM::TLB_Unmaped(uint32_t Vaddr, uint32_t Len)
{
    size_t count, End;

    End = Vaddr + Len;
    for (count = Vaddr; count < End; count += 0x1000)
    {
        size_t Index = count >> 12;
        m_TLB_ReadMap[Index] = 0;
        m_TLB_WriteMap[Index] = 0;
    }
}

void CMipsMemoryVM::RdramChanged(CMipsMemoryVM * _this)
{
    const size_t new_size = g_Settings->LoadDword(Game_RDRamSize);
    const size_t old_size = _this->m_AllocatedRdramSize;

    if (old_size == new_size)
    {
        return;
    }
    if (old_size > new_size)
    {
        DecommitMemory(_this->m_RDRAM + new_size, old_size - new_size);
    }
    else
    {
        void * result = CommitMemory(_this->m_RDRAM + old_size, new_size - old_size, MEM_READWRITE);
        if (result == NULL)
        {
            WriteTrace(TraceN64System, TraceError, "failed to allocate extended memory");
            g_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
        }
    }

    if (new_size > 0xFFFFFFFFul)
    { // should be unreachable because:  size_t new_size = g_Settings->(uint32_t)
        g_Notify->BreakPoint(__FILE__, __LINE__);
    } // ...However, FFFFFFFF also is a limit to RCP addressing, so we care.
    _this->m_AllocatedRdramSize = (uint32_t)new_size;
}

void CMipsMemoryVM::ChangeSpStatus()
{
    if ((RegModValue & SP_CLR_HALT) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_HALT;
    }
    if ((RegModValue & SP_SET_HALT) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_HALT;
    }
    if ((RegModValue & SP_CLR_BROKE) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_BROKE;
    }
    if ((RegModValue & SP_CLR_INTR) != 0)
    {
        g_Reg->MI_INTR_REG &= ~MI_INTR_SP;
        g_Reg->m_RspIntrReg &= ~MI_INTR_SP;
        g_Reg->CheckInterrupts();
    }
    if ((RegModValue & SP_SET_INTR) != 0 && bHaveDebugger())
    {
        g_Notify->DisplayError("SP_SET_INTR");
    }
    if ((RegModValue & SP_CLR_SSTEP) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SSTEP;
    }
    if ((RegModValue & SP_SET_SSTEP) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SSTEP;
    }
    if ((RegModValue & SP_CLR_INTR_BREAK) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK;
    }
    if ((RegModValue & SP_SET_INTR_BREAK) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_INTR_BREAK;
    }
    if ((RegModValue & SP_CLR_SIG0) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG0;
    }
    if ((RegModValue & SP_SET_SIG0) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG0;
    }
    if ((RegModValue & SP_CLR_SIG1) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG1;
    }
    if ((RegModValue & SP_SET_SIG1) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG1;
    }
    if ((RegModValue & SP_CLR_SIG2) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG2;
    }
    if ((RegModValue & SP_SET_SIG2) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG2;
    }
    if ((RegModValue & SP_CLR_SIG3) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG3;
    }
    if ((RegModValue & SP_SET_SIG3) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG3;
    }
    if ((RegModValue & SP_CLR_SIG4) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG4;
    }
    if ((RegModValue & SP_SET_SIG4) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG4;
    }
    if ((RegModValue & SP_CLR_SIG5) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG5;
    }
    if ((RegModValue & SP_SET_SIG5) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG5;
    }
    if ((RegModValue & SP_CLR_SIG6) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG6;
    }
    if ((RegModValue & SP_SET_SIG6) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG6;
    }
    if ((RegModValue & SP_CLR_SIG7) != 0)
    {
        g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG7;
    }
    if ((RegModValue & SP_SET_SIG7) != 0)
    {
        g_Reg->SP_STATUS_REG |= SP_STATUS_SIG7;
    }

    if ((RegModValue & SP_SET_SIG0) != 0 && g_System->RspAudioSignal())
    {
        g_Reg->MI_INTR_REG |= MI_INTR_SP;
        g_Reg->CheckInterrupts();
    }
    //if (*( uint32_t *)(DMEM + 0xFC0) == 1)
    //{
    //	ChangeTimer(RspTimer,0x40000);
    //}
    //else
    //{
    try
    {
        g_System->RunRSP();
    }
    catch (...)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    //}
}

void CMipsMemoryVM::ChangeMiIntrMask()
{
    if ((RegModValue & MI_INTR_MASK_CLR_SP) != 0)
    {
        g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP;
    }
    if ((RegModValue & MI_INTR_MASK_SET_SP) != 0)
    {
        g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SP;
    }
    if ((RegModValue & MI_INTR_MASK_CLR_SI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI;
    }
    if ((RegModValue & MI_INTR_MASK_SET_SI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SI;
    }
    if ((RegModValue & MI_INTR_MASK_CLR_AI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI;
    }
    if ((RegModValue & MI_INTR_MASK_SET_AI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_AI;
    }
    if ((RegModValue & MI_INTR_MASK_CLR_VI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI;
    }
    if ((RegModValue & MI_INTR_MASK_SET_VI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_VI;
    }
    if ((RegModValue & MI_INTR_MASK_CLR_PI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI;
    }
    if ((RegModValue & MI_INTR_MASK_SET_PI) != 0)
    {
        g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_PI;
    }
    if ((RegModValue & MI_INTR_MASK_CLR_DP) != 0)
    {
        g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP;
    }
    if ((RegModValue & MI_INTR_MASK_SET_DP) != 0)
    {
        g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_DP;
    }
}

void CMipsMemoryVM::Load32RDRAMRegisters(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x03F00000: m_MemLookupValue.UW[0] = g_Reg->RDRAM_CONFIG_REG; break;
    case 0x03F00004: m_MemLookupValue.UW[0] = g_Reg->RDRAM_DEVICE_ID_REG; break;
    case 0x03F00008: m_MemLookupValue.UW[0] = g_Reg->RDRAM_DELAY_REG; break;
    case 0x03F0000C: m_MemLookupValue.UW[0] = g_Reg->RDRAM_MODE_REG; break;
    case 0x03F00010: m_MemLookupValue.UW[0] = g_Reg->RDRAM_REF_INTERVAL_REG; break;
    case 0x03F00014: m_MemLookupValue.UW[0] = g_Reg->RDRAM_REF_ROW_REG; break;
    case 0x03F00018: m_MemLookupValue.UW[0] = g_Reg->RDRAM_RAS_INTERVAL_REG; break;
    case 0x03F0001C: m_MemLookupValue.UW[0] = g_Reg->RDRAM_MIN_INTERVAL_REG; break;
    case 0x03F00020: m_MemLookupValue.UW[0] = g_Reg->RDRAM_ADDR_SELECT_REG; break;
    case 0x03F00024: m_MemLookupValue.UW[0] = g_Reg->RDRAM_DEVICE_MANUF_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    m_MemLookupValid = true;
}

void CMipsMemoryVM::Load32SPRegisters(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04040010: m_MemLookupValue.UW[0] = g_Reg->SP_STATUS_REG; break;
    case 0x04040014: m_MemLookupValue.UW[0] = g_Reg->SP_DMA_FULL_REG; break;
    case 0x04040018: m_MemLookupValue.UW[0] = g_Reg->SP_DMA_BUSY_REG; break;
    case 0x0404001C:
        m_MemLookupValue.UW[0] = g_Reg->SP_SEMAPHORE_REG;
        g_Reg->SP_SEMAPHORE_REG = 1;
        break;
    case 0x04080000: m_MemLookupValue.UW[0] = g_Reg->SP_PC_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32DPCommand(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x0410000C: m_MemLookupValue.UW[0] = g_Reg->DPC_STATUS_REG; break;
    case 0x04100010: m_MemLookupValue.UW[0] = g_Reg->DPC_CLOCK_REG; break;
    case 0x04100014: m_MemLookupValue.UW[0] = g_Reg->DPC_BUFBUSY_REG; break;
    case 0x04100018: m_MemLookupValue.UW[0] = g_Reg->DPC_PIPEBUSY_REG; break;
    case 0x0410001C: m_MemLookupValue.UW[0] = g_Reg->DPC_TMEM_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32MIPSInterface(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04300000: m_MemLookupValue.UW[0] = g_Reg->MI_MODE_REG; break;
    case 0x04300004: m_MemLookupValue.UW[0] = g_Reg->MI_VERSION_REG; break;
    case 0x04300008: m_MemLookupValue.UW[0] = g_Reg->MI_INTR_REG; break;
    case 0x0430000C: m_MemLookupValue.UW[0] = g_Reg->MI_INTR_MASK_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32VideoInterface(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04400000: m_MemLookupValue.UW[0] = g_Reg->VI_STATUS_REG; break;
    case 0x04400004: m_MemLookupValue.UW[0] = g_Reg->VI_ORIGIN_REG; break;
    case 0x04400008: m_MemLookupValue.UW[0] = g_Reg->VI_WIDTH_REG; break;
    case 0x0440000C: m_MemLookupValue.UW[0] = g_Reg->VI_INTR_REG; break;
    case 0x04400010:
        g_MMU->UpdateHalfLine();
        m_MemLookupValue.UW[0] = g_MMU->m_HalfLine;
        break;
    case 0x04400014: m_MemLookupValue.UW[0] = g_Reg->VI_BURST_REG; break;
    case 0x04400018: m_MemLookupValue.UW[0] = g_Reg->VI_V_SYNC_REG; break;
    case 0x0440001C: m_MemLookupValue.UW[0] = g_Reg->VI_H_SYNC_REG; break;
    case 0x04400020: m_MemLookupValue.UW[0] = g_Reg->VI_LEAP_REG; break;
    case 0x04400024: m_MemLookupValue.UW[0] = g_Reg->VI_H_START_REG; break;
    case 0x04400028: m_MemLookupValue.UW[0] = g_Reg->VI_V_START_REG; break;
    case 0x0440002C: m_MemLookupValue.UW[0] = g_Reg->VI_V_BURST_REG; break;
    case 0x04400030: m_MemLookupValue.UW[0] = g_Reg->VI_X_SCALE_REG; break;
    case 0x04400034: m_MemLookupValue.UW[0] = g_Reg->VI_Y_SCALE_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32AudioInterface(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04500004:
        if (g_System->bFixedAudio())
        {
            m_MemLookupValue.UW[0] = g_Audio->GetLength();
        }
        else
        {
            if (g_Plugins->Audio()->AiReadLength != NULL)
            {
                m_MemLookupValue.UW[0] = g_Plugins->Audio()->AiReadLength();
            }
            else
            {
                m_MemLookupValue.UW[0] = 0;
            }
        }
        break;
    case 0x0450000C:
        if (g_System->bFixedAudio())
        {
            m_MemLookupValue.UW[0] = g_Audio->GetStatus();
        }
        else
        {
            m_MemLookupValue.UW[0] = g_Reg->AI_STATUS_REG;
        }
        break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32PeripheralInterface(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04600000: m_MemLookupValue.UW[0] = g_Reg->PI_DRAM_ADDR_REG; break;
    case 0x04600004: m_MemLookupValue.UW[0] = g_Reg->PI_CART_ADDR_REG; break;
    case 0x04600008: m_MemLookupValue.UW[0] = g_Reg->PI_RD_LEN_REG; break;
    case 0x0460000C: m_MemLookupValue.UW[0] = g_Reg->PI_WR_LEN_REG; break;
    case 0x04600010: m_MemLookupValue.UW[0] = g_Reg->PI_STATUS_REG; break;
    case 0x04600014: m_MemLookupValue.UW[0] = g_Reg->PI_DOMAIN1_REG; break;
    case 0x04600018: m_MemLookupValue.UW[0] = g_Reg->PI_BSD_DOM1_PWD_REG; break;
    case 0x0460001C: m_MemLookupValue.UW[0] = g_Reg->PI_BSD_DOM1_PGS_REG; break;
    case 0x04600020: m_MemLookupValue.UW[0] = g_Reg->PI_BSD_DOM1_RLS_REG; break;
    case 0x04600024: m_MemLookupValue.UW[0] = g_Reg->PI_DOMAIN2_REG; break;
    case 0x04600028: m_MemLookupValue.UW[0] = g_Reg->PI_BSD_DOM2_PWD_REG; break;
    case 0x0460002C: m_MemLookupValue.UW[0] = g_Reg->PI_BSD_DOM2_PGS_REG; break;
    case 0x04600030: m_MemLookupValue.UW[0] = g_Reg->PI_BSD_DOM2_RLS_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32RDRAMInterface(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04700000: m_MemLookupValue.UW[0] = g_Reg->RI_MODE_REG; break;
    case 0x04700004: m_MemLookupValue.UW[0] = g_Reg->RI_CONFIG_REG; break;
    case 0x04700008: m_MemLookupValue.UW[0] = g_Reg->RI_CURRENT_LOAD_REG; break;
    case 0x0470000C: m_MemLookupValue.UW[0] = g_Reg->RI_SELECT_REG; break;
    case 0x04700010: m_MemLookupValue.UW[0] = g_Reg->RI_REFRESH_REG; break;
    case 0x04700014: m_MemLookupValue.UW[0] = g_Reg->RI_LATENCY_REG; break;
    case 0x04700018: m_MemLookupValue.UW[0] = g_Reg->RI_RERROR_REG; break;
    case 0x0470001C: m_MemLookupValue.UW[0] = g_Reg->RI_WERROR_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32SerialInterface(void)
{
    switch (m_MemLookupAddress & 0x1FFFFFFF)
    {
    case 0x04800018: m_MemLookupValue.UW[0] = g_Reg->SI_STATUS_REG; break;
    default:
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32CartridgeDomain1Address1(void)
{
    //64DD IPL ROM
    if (g_DDRom != NULL && (m_MemLookupAddress & 0xFFFFFF) < g_MMU->m_DDRomSize)
    {
        m_MemLookupValue.UW[0] = *(uint32_t *)&g_MMU->m_DDRom[(m_MemLookupAddress & 0xFFFFFF)];
    }
    else
    {
        m_MemLookupValue.UW[0] = m_MemLookupAddress & 0xFFFF;
        m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
    }
}

void CMipsMemoryVM::Load32CartridgeDomain1Address3(void)
{
    m_MemLookupValue.UW[0] = m_MemLookupAddress & 0xFFFF;
    m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
}

void CMipsMemoryVM::Load32CartridgeDomain2Address1(void)
{
    //64DD REGISTERS
    if (g_Settings->LoadBool(Setting_EnableDisk))
    {
        switch (m_MemLookupAddress & 0x1FFFFFFF)
        {
        case 0x05000500: m_MemLookupValue.UW[0] = g_Reg->ASIC_DATA; break;
        case 0x05000504: m_MemLookupValue.UW[0] = g_Reg->ASIC_MISC_REG; break;
        case 0x05000508:
            m_MemLookupValue.UW[0] = g_Reg->ASIC_STATUS;
            DiskGapSectorCheck();
            break;
        case 0x0500050C: m_MemLookupValue.UW[0] = g_Reg->ASIC_CUR_TK; break;
        case 0x05000510: m_MemLookupValue.UW[0] = g_Reg->ASIC_BM_STATUS; break;
        case 0x05000514: m_MemLookupValue.UW[0] = g_Reg->ASIC_ERR_SECTOR; break;
        case 0x05000518: m_MemLookupValue.UW[0] = g_Reg->ASIC_SEQ_STATUS; break;
        case 0x0500051C: m_MemLookupValue.UW[0] = g_Reg->ASIC_CUR_SECTOR; break;
        case 0x05000520: m_MemLookupValue.UW[0] = g_Reg->ASIC_HARD_RESET; break;
        case 0x05000524: m_MemLookupValue.UW[0] = g_Reg->ASIC_C1_S0; break;
        case 0x05000528: m_MemLookupValue.UW[0] = g_Reg->ASIC_HOST_SECBYTE; break;
        case 0x0500052C: m_MemLookupValue.UW[0] = g_Reg->ASIC_C1_S2; break;
        case 0x05000530: m_MemLookupValue.UW[0] = g_Reg->ASIC_SEC_BYTE; break;
        case 0x05000534: m_MemLookupValue.UW[0] = g_Reg->ASIC_C1_S4; break;
        case 0x05000538: m_MemLookupValue.UW[0] = g_Reg->ASIC_C1_S6; break;
        case 0x0500053C: m_MemLookupValue.UW[0] = g_Reg->ASIC_CUR_ADDR; break;
        case 0x05000540: m_MemLookupValue.UW[0] = g_Reg->ASIC_ID_REG; break;
        case 0x05000544: m_MemLookupValue.UW[0] = g_Reg->ASIC_TEST_REG; break;
        case 0x05000548: m_MemLookupValue.UW[0] = g_Reg->ASIC_TEST_PIN_SEL; break;
        default:
            m_MemLookupValue.UW[0] = m_MemLookupAddress & 0xFFFF;
            m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    else
    {
        m_MemLookupValue.UW[0] = m_MemLookupAddress & 0xFFFF;
        m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
    }
}

void CMipsMemoryVM::Load32CartridgeDomain2Address2(void)
{
    if (g_System->m_SaveUsing == SaveChip_Auto)
    {
        g_System->m_SaveUsing = SaveChip_FlashRam;
    }
    if (g_System->m_SaveUsing == SaveChip_Sram)
    {
        //Load Sram
        uint8_t tmp[4] = "";
        g_MMU->DmaFromSram(tmp, (m_MemLookupAddress & 0x1FFFFFFF) - 0x08000000, 4);
        m_MemLookupValue.UW[0] = tmp[3] << 24 | tmp[2] << 16 | tmp[1] << 8 | tmp[0];
    }
    else if (g_System->m_SaveUsing != SaveChip_FlashRam)
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        m_MemLookupValue.UW[0] = m_MemLookupAddress & 0xFFFF;
        m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
    }
    else
    {
        m_MemLookupValue.UW[0] = g_MMU->ReadFromFlashStatus(m_MemLookupAddress & 0x1FFFFFFF);
    }
}

void CMipsMemoryVM::Load32PifRam(void)
{
    if ((m_MemLookupAddress & 0x1FFFFFFF) < 0x1FC007C0)
    {
        //m_MemLookupValue.UW[0] = swap32by8(*(uint32_t *)(&PifRom[PAddr - 0x1FC00000]));
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if ((m_MemLookupAddress & 0x1FFFFFFF) < 0x1FC00800)
    {
        uint8_t * PIF_Ram = g_MMU->PifRam();
        m_MemLookupValue.UW[0] = *(uint32_t *)(&PIF_Ram[(m_MemLookupAddress & 0x1FFFFFFF) - 0x1FC007C0]);
        m_MemLookupValue.UW[0] = swap32by8(m_MemLookupValue.UW[0]);
    }
    else
    {
        m_MemLookupValue.UW[0] = 0;
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Load32Rom(void)
{
    if (g_MMU->m_RomWrittenTo)
    {
        m_MemLookupValue.UW[0] = g_MMU->m_RomWroteValue;
        //LogMessage("%X: Read crap from Rom %08X from %08X",PROGRAM_COUNTER,*Value,PAddr);
        g_MMU->m_RomWrittenTo = false;
#ifdef ROM_IN_MAPSPACE
        {
            uint32_t OldProtect;
            VirtualProtect(ROM, RomFileSize, PAGE_READONLY, &OldProtect);
        }
#endif
    }
    else if ((m_MemLookupAddress & 0xFFFFFFF) < g_MMU->m_RomSize)
    {
        m_MemLookupValue.UW[0] = *(uint32_t *)&g_MMU->m_Rom[(m_MemLookupAddress & 0xFFFFFFF)];
    }
    else
    {
        m_MemLookupValue.UW[0] = m_MemLookupAddress & 0xFFFF;
        m_MemLookupValue.UW[0] = (m_MemLookupValue.UW[0] << 16) | m_MemLookupValue.UW[0];
    }
}

void CMipsMemoryVM::Write32RDRAMRegisters(void)
{
    switch ((m_MemLookupAddress & 0xFFFFFFF))
    {
    case 0x03F00000: g_Reg->RDRAM_CONFIG_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00004: g_Reg->RDRAM_DEVICE_ID_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00008: g_Reg->RDRAM_DELAY_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F0000C: g_Reg->RDRAM_MODE_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00010: g_Reg->RDRAM_REF_INTERVAL_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00014: g_Reg->RDRAM_REF_ROW_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00018: g_Reg->RDRAM_RAS_INTERVAL_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F0001C: g_Reg->RDRAM_MIN_INTERVAL_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00020: g_Reg->RDRAM_ADDR_SELECT_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F00024: g_Reg->RDRAM_DEVICE_MANUF_REG = m_MemLookupValue.UW[0]; break;
    case 0x03F04004: break;
    case 0x03F08004: break;
    case 0x03F80004: break;
    case 0x03F80008: break;
    case 0x03F8000C: break;
    case 0x03F80014: break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32SPRegisters(void)
{
    switch ((m_MemLookupAddress & 0xFFFFFFF))
    {
    case 0x04040000: g_Reg->SP_MEM_ADDR_REG = m_MemLookupValue.UW[0]; break;
    case 0x04040004: g_Reg->SP_DRAM_ADDR_REG = m_MemLookupValue.UW[0]; break;
    case 0x04040008:
        g_Reg->SP_RD_LEN_REG = m_MemLookupValue.UW[0];
        g_MMU->SP_DMA_READ();
        break;
    case 0x0404000C:
        g_Reg->SP_WR_LEN_REG = m_MemLookupValue.UW[0];
        g_MMU->SP_DMA_WRITE();
        break;
    case 0x04040010:
        if ((m_MemLookupValue.UW[0] & SP_CLR_HALT) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_HALT;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_HALT) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_HALT;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_BROKE) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_BROKE;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_INTR) != 0)
        {
            g_Reg->MI_INTR_REG &= ~MI_INTR_SP;
            g_Reg->m_RspIntrReg &= ~MI_INTR_SP;
            g_Reg->CheckInterrupts();
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_INTR) != 0)
        {
            g_Notify->DisplayError("SP_SET_INTR");
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SSTEP) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SSTEP;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SSTEP) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SSTEP;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_INTR_BREAK) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_INTR_BREAK) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_INTR_BREAK;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG0) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG0;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG0) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG0;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG1) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG1;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG1) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG1;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG2) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG2;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG2) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG2;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG3) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG3;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG3) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG3;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG4) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG4;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG4) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG4;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG5) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG5;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG5) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG5;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG6) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG6;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG6) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG6;
        }
        if ((m_MemLookupValue.UW[0] & SP_CLR_SIG7) != 0)
        {
            g_Reg->SP_STATUS_REG &= ~SP_STATUS_SIG7;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG7) != 0)
        {
            g_Reg->SP_STATUS_REG |= SP_STATUS_SIG7;
        }
        if ((m_MemLookupValue.UW[0] & SP_SET_SIG0) != 0 && g_System->RspAudioSignal())
        {
            g_Reg->MI_INTR_REG |= MI_INTR_SP;
            g_Reg->CheckInterrupts();
        }
        //if (*( uint32_t *)(DMEM + 0xFC0) == 1)
        //{
        //	ChangeTimer(RspTimer,0x30000);
        //}
        //else
        //{
        try
        {
            g_System->RunRSP();
        }
        catch (...)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        //}
        break;
    case 0x0404001C: g_Reg->SP_SEMAPHORE_REG = 0; break;
    case 0x04080000: g_Reg->SP_PC_REG = m_MemLookupValue.UW[0] & 0xFFC; break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32DPCommandRegisters(void)
{
    switch ((m_MemLookupAddress & 0xFFFFFFF))
    {
    case 0x04100000:
        g_Reg->DPC_START_REG = m_MemLookupValue.UW[0];
        g_Reg->DPC_CURRENT_REG = m_MemLookupValue.UW[0];
        break;
    case 0x04100004:
        g_Reg->DPC_END_REG = m_MemLookupValue.UW[0];
        if (g_Plugins->Gfx()->ProcessRDPList)
        {
            g_Plugins->Gfx()->ProcessRDPList();
        }
        break;
        //case 0x04100008: g_Reg->DPC_CURRENT_REG = Value; break;
    case 0x0410000C:
        if ((m_MemLookupValue.UW[0] & DPC_CLR_XBUS_DMEM_DMA) != 0)
        {
            g_Reg->DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((m_MemLookupValue.UW[0] & DPC_SET_XBUS_DMEM_DMA) != 0)
        {
            g_Reg->DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((m_MemLookupValue.UW[0] & DPC_CLR_FREEZE) != 0)
        {
            g_Reg->DPC_STATUS_REG &= ~DPC_STATUS_FREEZE;
        }
        if ((m_MemLookupValue.UW[0] & DPC_SET_FREEZE) != 0)
        {
            g_Reg->DPC_STATUS_REG |= DPC_STATUS_FREEZE;
        }
        if ((m_MemLookupValue.UW[0] & DPC_CLR_FLUSH) != 0)
        {
            g_Reg->DPC_STATUS_REG &= ~DPC_STATUS_FLUSH;
        }
        if ((m_MemLookupValue.UW[0] & DPC_SET_FLUSH) != 0)
        {
            g_Reg->DPC_STATUS_REG |= DPC_STATUS_FLUSH;
        }
        if ((m_MemLookupValue.UW[0] & DPC_CLR_FREEZE) != 0)
        {
            if ((g_Reg->SP_STATUS_REG & SP_STATUS_HALT) == 0)
            {
                if ((g_Reg->SP_STATUS_REG & SP_STATUS_BROKE) == 0)
                {
                    __except_try()
                    {
                        g_System->RunRSP();
                    }
                    __except_catch()
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                }
            }
        }
#ifdef legacycode
        if (ShowUnhandledMemory)
        {
            //if ( ( m_MemLookupValue.UW[0] & DPC_CLR_TMEM_CTR ) != 0)
            //{
            //	g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_TMEM_CTR");
            //}
            //if ( ( m_MemLookupValue.UW[0] & DPC_CLR_PIPE_CTR ) != 0)
            //{
            //	g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR");
            //}
            //if ( ( m_MemLookupValue.UW[0] & DPC_CLR_CMD_CTR ) != 0)
            //{
            //	g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR");
            //}
            //if ( ( m_MemLookupValue.UW[0] & DPC_CLR_CLOCK_CTR ) != 0)
            //{
            //	g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR");
            //}
        }
#endif
        break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32MIPSInterface(void)
{
    switch ((m_MemLookupAddress & 0xFFFFFFF))
    {
    case 0x04300000:
        g_Reg->MI_MODE_REG &= ~0x7F;
        g_Reg->MI_MODE_REG |= (m_MemLookupValue.UW[0] & 0x7F);
        if ((m_MemLookupValue.UW[0] & MI_CLR_INIT) != 0)
        {
            g_Reg->MI_MODE_REG &= ~MI_MODE_INIT;
        }
        if ((m_MemLookupValue.UW[0] & MI_SET_INIT) != 0)
        {
            g_Reg->MI_MODE_REG |= MI_MODE_INIT;
        }
        if ((m_MemLookupValue.UW[0] & MI_CLR_EBUS) != 0)
        {
            g_Reg->MI_MODE_REG &= ~MI_MODE_EBUS;
        }
        if ((m_MemLookupValue.UW[0] & MI_SET_EBUS) != 0)
        {
            g_Reg->MI_MODE_REG |= MI_MODE_EBUS;
        }
        if ((m_MemLookupValue.UW[0] & MI_CLR_DP_INTR) != 0)
        {
            g_Reg->MI_INTR_REG &= ~MI_INTR_DP;
            g_Reg->m_GfxIntrReg &= ~MI_INTR_DP;
            g_Reg->CheckInterrupts();
        }
        if ((m_MemLookupValue.UW[0] & MI_CLR_RDRAM) != 0)
        {
            g_Reg->MI_MODE_REG &= ~MI_MODE_RDRAM;
        }
        if ((m_MemLookupValue.UW[0] & MI_SET_RDRAM) != 0)
        {
            g_Reg->MI_MODE_REG |= MI_MODE_RDRAM;
        }
        break;
    case 0x0430000C:
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_CLR_SP) != 0)
        {
            g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_SET_SP) != 0)
        {
            g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SP;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_CLR_SI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_SET_SI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_CLR_AI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_SET_AI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_AI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_CLR_VI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_SET_VI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_VI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_CLR_PI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_SET_PI) != 0)
        {
            g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_PI;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_CLR_DP) != 0)
        {
            g_Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP;
        }
        if ((m_MemLookupValue.UW[0] & MI_INTR_MASK_SET_DP) != 0)
        {
            g_Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_DP;
        }
        break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32VideoInterface(void)
{
    switch ((m_MemLookupAddress & 0xFFFFFFF))
    {
    case 0x04400000:
        if (g_Reg->VI_STATUS_REG != m_MemLookupValue.UW[0])
        {
            g_Reg->VI_STATUS_REG = m_MemLookupValue.UW[0];
            if (g_Plugins->Gfx()->ViStatusChanged != NULL)
            {
                g_Plugins->Gfx()->ViStatusChanged();
            }
        }
        break;
    case 0x04400004:
#ifdef CFB_READ
        if (g_Reg->VI_ORIGIN_REG > 0x280)
        {
            SetFrameBuffer(g_Reg->VI_ORIGIN_REG, (uint32_t)(VI_WIDTH_REG * (VI_WIDTH_REG *.75)));
        }
#endif
        g_Reg->VI_ORIGIN_REG = (m_MemLookupValue.UW[0] & 0xFFFFFF);
        //if (UpdateScreen != NULL )
        //{
        //	UpdateScreen();
        //}
        break;
    case 0x04400008:
        if (g_Reg->VI_WIDTH_REG != m_MemLookupValue.UW[0])
        {
            g_Reg->VI_WIDTH_REG = m_MemLookupValue.UW[0];
            if (g_Plugins->Gfx()->ViWidthChanged != NULL)
            {
                g_Plugins->Gfx()->ViWidthChanged();
            }
        }
        break;
    case 0x0440000C: g_Reg->VI_INTR_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400010:
        g_Reg->MI_INTR_REG &= ~MI_INTR_VI;
        g_Reg->CheckInterrupts();
        break;
    case 0x04400014: g_Reg->VI_BURST_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400018: g_Reg->VI_V_SYNC_REG = m_MemLookupValue.UW[0]; break;
    case 0x0440001C: g_Reg->VI_H_SYNC_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400020: g_Reg->VI_LEAP_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400024: g_Reg->VI_H_START_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400028: g_Reg->VI_V_START_REG = m_MemLookupValue.UW[0]; break;
    case 0x0440002C: g_Reg->VI_V_BURST_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400030: g_Reg->VI_X_SCALE_REG = m_MemLookupValue.UW[0]; break;
    case 0x04400034: g_Reg->VI_Y_SCALE_REG = m_MemLookupValue.UW[0]; break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32AudioInterface(void)
{
    switch (m_MemLookupAddress & 0xFFFFFFF)
    {
    case 0x04500000: g_Reg->AI_DRAM_ADDR_REG = m_MemLookupValue.UW[0]; break;
    case 0x04500004:
        g_Reg->AI_LEN_REG = m_MemLookupValue.UW[0];
        if (g_System->bFixedAudio())
        {
            g_Audio->LenChanged();
        }
        else
        {
            if (g_Plugins->Audio()->AiLenChanged != NULL)
            {
                g_Plugins->Audio()->AiLenChanged();
            }
        }
        break;
    case 0x04500008: g_Reg->AI_CONTROL_REG = (m_MemLookupValue.UW[0] & 1); break;
    case 0x0450000C:
        /* Clear Interrupt */;
        g_Reg->MI_INTR_REG &= ~MI_INTR_AI;
        g_Reg->m_AudioIntrReg &= ~MI_INTR_AI;
        g_Reg->CheckInterrupts();
        break;
    case 0x04500010:
        g_Reg->AI_DACRATE_REG = m_MemLookupValue.UW[0];
        g_Plugins->Audio()->DacrateChanged(g_System->SystemType());
        if (g_System->bFixedAudio())
        {
            g_Audio->SetFrequency(m_MemLookupValue.UW[0], g_System->SystemType());
        }
        break;
    case 0x04500014:  g_Reg->AI_BITRATE_REG = m_MemLookupValue.UW[0]; break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32PeripheralInterface(void)
{
    switch (m_MemLookupAddress & 0xFFFFFFF)
    {
    case 0x04600000: g_Reg->PI_DRAM_ADDR_REG = m_MemLookupValue.UW[0]; break;
    case 0x04600004:
        g_Reg->PI_CART_ADDR_REG = m_MemLookupValue.UW[0];
        if (g_Settings->LoadBool(Setting_EnableDisk))
        {
            DiskDMACheck();
        }
        break;
    case 0x04600008:
        g_Reg->PI_RD_LEN_REG = m_MemLookupValue.UW[0];
        g_MMU->PI_DMA_READ();
        break;
    case 0x0460000C:
        g_Reg->PI_WR_LEN_REG = m_MemLookupValue.UW[0];
        g_MMU->PI_DMA_WRITE();
        break;
    case 0x04600010:
        //if ((Value & PI_SET_RESET) != 0 )
        //{
        //	g_Notify->DisplayError("reset Controller");
        //}
        if ((m_MemLookupValue.UW[0] & PI_CLR_INTR) != 0)
        {
            g_Reg->MI_INTR_REG &= ~MI_INTR_PI;
            g_Reg->CheckInterrupts();
        }
        break;
    case 0x04600014: g_Reg->PI_DOMAIN1_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x04600018: g_Reg->PI_BSD_DOM1_PWD_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x0460001C: g_Reg->PI_BSD_DOM1_PGS_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x04600020: g_Reg->PI_BSD_DOM1_RLS_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x04600024: g_Reg->PI_DOMAIN2_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x04600028: g_Reg->PI_BSD_DOM2_PWD_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x0460002C: g_Reg->PI_BSD_DOM2_PGS_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    case 0x04600030: g_Reg->PI_BSD_DOM2_RLS_REG = (m_MemLookupValue.UW[0] & 0xFF); break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32RDRAMInterface(void)
{
    switch (m_MemLookupAddress & 0xFFFFFFF)
    {
    case 0x04700000: g_Reg->RI_MODE_REG = m_MemLookupValue.UW[0]; break;
    case 0x04700004: g_Reg->RI_CONFIG_REG = m_MemLookupValue.UW[0]; break;
    case 0x04700008: g_Reg->RI_CURRENT_LOAD_REG = m_MemLookupValue.UW[0]; break;
    case 0x0470000C: g_Reg->RI_SELECT_REG = m_MemLookupValue.UW[0]; break;
    case 0x04700010: g_Reg->RI_REFRESH_REG = m_MemLookupValue.UW[0]; break;
    case 0x04700014: g_Reg->RI_LATENCY_REG = m_MemLookupValue.UW[0]; break;
    case 0x04700018: g_Reg->RI_RERROR_REG = m_MemLookupValue.UW[0]; break;
    case 0x0470001C: g_Reg->RI_WERROR_REG = m_MemLookupValue.UW[0]; break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32SerialInterface(void)
{
    switch (m_MemLookupAddress & 0xFFFFFFF)
    {
    case 0x04800000: g_Reg->SI_DRAM_ADDR_REG = m_MemLookupValue.UW[0]; break;
    case 0x04800004:
        g_Reg->SI_PIF_ADDR_RD64B_REG = m_MemLookupValue.UW[0];
        g_MMU->SI_DMA_READ();
        break;
    case 0x04800010:
        g_Reg->SI_PIF_ADDR_WR64B_REG = m_MemLookupValue.UW[0];
        g_MMU->SI_DMA_WRITE();
        break;
    case 0x04800018:
        g_Reg->MI_INTR_REG &= ~MI_INTR_SI;
        g_Reg->SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
        g_Reg->CheckInterrupts();
        break;
    default:
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CMipsMemoryVM::Write32CartridgeDomain2Address1(void)
{
    //64DD Registers
    if (g_Settings->LoadBool(Setting_EnableDisk))
    {
        switch (m_MemLookupAddress & 0xFFFFFFF)
        {
        case 0x05000500: g_Reg->ASIC_DATA = m_MemLookupValue.UW[0]; break;
        case 0x05000508:
            g_Reg->ASIC_CMD = m_MemLookupValue.UW[0];
            DiskCommand();
            g_Reg->ASIC_STATUS |= DD_STATUS_MECHA_INT;
            g_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP3;
            g_Reg->CheckInterrupts();
            break;
        case 0x05000510:
            //ASIC_BM_STATUS_CTL
            g_Reg->ASIC_BM_CTL = m_MemLookupValue.UW[0];
            DiskBMControl();
            break;
        case 0x05000518:
            //ASIC_SEQ_STATUS_CTL
            break;
        case 0x05000520: DiskReset(); break;
        case 0x05000528: g_Reg->ASIC_HOST_SECBYTE = m_MemLookupValue.UW[0]; break;
        case 0x05000530: g_Reg->ASIC_SEC_BYTE = m_MemLookupValue.UW[0]; break;
        case 0x05000548: g_Reg->ASIC_TEST_PIN_SEL = m_MemLookupValue.UW[0]; break;
        default:
            if (bHaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
}

void CMipsMemoryVM::Write32CartridgeDomain2Address2(void)
{
    if (g_System->m_SaveUsing == SaveChip_Sram)
    {
        //Store Sram
        uint8_t tmp[4] = "";
        tmp[0] = 0xFF & (m_MemLookupValue.UW[0]);
        tmp[1] = 0xFF & (m_MemLookupValue.UW[0] >> 8);
        tmp[2] = 0xFF & (m_MemLookupValue.UW[0] >> 16);
        tmp[3] = 0xFF & (m_MemLookupValue.UW[0] >> 24);
        g_MMU->DmaToSram(tmp, (m_MemLookupAddress & 0x1FFFFFFF) - 0x08000000, 4);
        return;
    }
    /*if ((m_MemLookupAddress & 0x1FFFFFFF) != 0x08010000)
    {
    if (bHaveDebugger())
    {
    g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    }*/
    if (g_System->m_SaveUsing == SaveChip_Auto)
    {
        g_System->m_SaveUsing = SaveChip_FlashRam;
    }
    if (g_System->m_SaveUsing == SaveChip_FlashRam)
    {
        g_MMU->WriteToFlashCommand(m_MemLookupValue.UW[0]);
    }
}

void CMipsMemoryVM::Write32PifRam(void)
{
    if ((m_MemLookupAddress & 0x1FFFFFFF) < 0x1FC007C0)
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if ((m_MemLookupAddress & 0x1FFFFFFF) < 0x1FC00800)
    {
        uint32_t Value = swap32by8(m_MemLookupValue.UW[0]);
        *(uint32_t *)(&g_MMU->m_PifRam[(m_MemLookupAddress & 0x1FFFFFFF) - 0x1FC007C0]) = Value;
        if ((m_MemLookupAddress & 0x1FFFFFFF) == 0x1FC007FC)
        {
            g_MMU->PifRamWrite();
        }
    }
}