#include "stdafx.h"

#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\SystemGlobals.h>
#include <Project64-core\N64System\N64Rom.h>
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\Recompiler\RecompilerCodeLog.h>
#include <Project64-core\N64System\Mips\OpcodeName.h>
#include <Project64-core\N64System\Mips\Disk.h>
#include <Project64-core\ExceptionHandler.h>

#include <stdio.h>
#include <Common/MemoryManagement.h>

uint8_t * CMipsMemoryVM::m_Reserve1 = nullptr;
uint8_t * CMipsMemoryVM::m_Reserve2 = nullptr;
uint32_t CMipsMemoryVM::m_MemLookupAddress = 0;
MIPS_DWORD CMipsMemoryVM::m_MemLookupValue;
bool CMipsMemoryVM::m_MemLookupValid = true;
uint32_t CMipsMemoryVM::RegModValue;

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

CMipsMemoryVM::CMipsMemoryVM(CN64System & System, bool SavesReadOnly) :
    CPifRam(SavesReadOnly),
    CFlashram(SavesReadOnly),
    CSram(SavesReadOnly),
    CDMA(*this, *this),
    m_Reg(System.m_Reg),
    m_AudioInterfaceHandler(System, System.m_Reg),
    m_CartridgeDomain2Address1Handler(System.m_Reg),
    m_RDRAMRegistersHandler(System.m_Reg),
    m_RomMapped(false),
    m_DPCommandRegistersHandler(System, System.GetPlugins(), System.m_Reg),
    m_MIPSInterfaceHandler(System.m_Reg),
    m_PeripheralInterfaceHandler(*this, System.m_Reg),
    m_RDRAMInterfaceHandler(System.m_Reg),
    m_SerialInterfaceHandler(*this, System.m_Reg),
    m_SPRegistersHandler(System, *this, System.m_Reg),
    m_VideoInterfaceHandler(System, *this, System.m_Reg),
    m_Rom(nullptr),
    m_RomSize(0),
    m_RomWrittenTo(false),
    m_RomWroteValue(0),
    m_TLB_ReadMap(nullptr),
    m_TLB_WriteMap(nullptr),
    m_RDRAM(nullptr),
    m_DMEM(nullptr),
    m_IMEM(nullptr),
    m_DDRomMapped(false),
    m_DDRom(nullptr),
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

        memset(m_TLB_ReadMap, -1, 0xFFFFF * sizeof(size_t));
        memset(m_TLB_WriteMap, -1, 0xFFFFF * sizeof(size_t));
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
        m_Reserve1 = nullptr;
    }
    if (m_Reserve2)
    {
        FreeAddressSpace(m_Reserve2, 0x20000000);
        m_Reserve2 = nullptr;
    }
}

bool CMipsMemoryVM::Initialize(bool SyncSystem)
{
    if (m_RDRAM != nullptr)
    {
        return true;
    }

    if (!SyncSystem && m_RDRAM == nullptr && m_Reserve1 != nullptr)
    {
        m_RDRAM = m_Reserve1;
        m_Reserve1 = nullptr;
    }
    if (SyncSystem && m_RDRAM == nullptr && m_Reserve2 != nullptr)
    {
        m_RDRAM = m_Reserve2;
        m_Reserve2 = nullptr;
    }
    if (m_RDRAM == nullptr)
    {
        m_RDRAM = (uint8_t *)AllocateAddressSpace(0x20000000);
    }
    if (m_RDRAM == nullptr)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to reserve RDRAM (Size: 0x%X)", 0x20000000);
        FreeMemory();
        return false;
    }

    m_AllocatedRdramSize = g_Settings->LoadDword(Game_RDRamSize);
    if (CommitMemory(m_RDRAM, m_AllocatedRdramSize, MEM_READWRITE) == nullptr)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to allocate RDRAM (Size: 0x%X)", m_AllocatedRdramSize);
        FreeMemory();
        return false;
    }

    if (CommitMemory(m_RDRAM + 0x04000000, 0x2000, MEM_READWRITE) == nullptr)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to allocate DMEM/IMEM (Size: 0x%X)", 0x2000);
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
        if (CommitMemory(m_Rom, g_Rom->GetRomSize(), MEM_READWRITE) == nullptr)
        {
            WriteTrace(TraceN64System, TraceError, "Failed to allocate ROM (Size: 0x%X)", g_Rom->GetRomSize());
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

    // 64DD IPL
    if (g_DDRom != nullptr)
    {
        if (g_Settings->LoadBool(Game_LoadRomToMemory))
        {
            m_DDRomMapped = true;
            m_DDRom = m_RDRAM + 0x06000000;
            m_DDRomSize = g_DDRom->GetRomSize();
            if (CommitMemory(m_DDRom, g_DDRom->GetRomSize(), MEM_READWRITE) == nullptr)
            {
                WriteTrace(TraceN64System, TraceError, "Failed to allocate ROM (Size: 0x%X)", g_DDRom->GetRomSize());
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
    if (m_TLB_ReadMap == nullptr)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to allocate m_TLB_ReadMap (Size: 0x%X)", 0x100000 * sizeof(size_t));
        FreeMemory();
        return false;
    }

    m_TLB_WriteMap = new size_t[0x100000];
    if (m_TLB_WriteMap == nullptr)
    {
        WriteTrace(TraceN64System, TraceError, "Failed to allocate m_TLB_WriteMap (Size: 0x%X)", 0xFFFFF * sizeof(size_t));
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
            if (m_Reserve1 == nullptr)
            {
                m_Reserve1 = m_RDRAM;
            }
            else if (m_Reserve2 == nullptr)
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
        m_RDRAM = nullptr;
        m_IMEM = nullptr;
        m_DMEM = nullptr;
    }
    if (m_TLB_ReadMap)
    {
        delete[] m_TLB_ReadMap;
        m_TLB_ReadMap = nullptr;
    }
    if (m_TLB_WriteMap)
    {
        delete[] m_TLB_WriteMap;
        m_TLB_WriteMap = nullptr;
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

CSram* CMipsMemoryVM::GetSram(void)
{
	return dynamic_cast<CSram*>(this);
}

CFlashram* CMipsMemoryVM::GetFlashram()
{
	return dynamic_cast<CFlashram*>(this);
}

bool CMipsMemoryVM::LB_VAddr(uint32_t VAddr, uint8_t& Value)
{
    if (m_TLB_ReadMap[VAddr >> 12] == -1)
    {
        return false;
    }

    Value = *(uint8_t*)(m_TLB_ReadMap[VAddr >> 12] + (VAddr ^ 3));
    return true;
}

bool CMipsMemoryVM::LH_VAddr(uint32_t VAddr, uint16_t& Value)
{
    if (m_TLB_ReadMap[VAddr >> 12] == -1)
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
    if (BaseAddress == (uint8_t*)-1)
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
    if (m_TLB_ReadMap[VAddr >> 12] == -1)
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
    if (m_TLB_WriteMap[VAddr >> 12] == -1)
    {
        return false;
    }

    *(uint8_t*)(m_TLB_WriteMap[VAddr >> 12] + (VAddr ^ 3)) = Value;
    return true;
}

bool CMipsMemoryVM::SH_VAddr(uint32_t VAddr, uint16_t Value)
{
    if (m_TLB_WriteMap[VAddr >> 12] == -1)
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

    if (m_TLB_WriteMap[VAddr >> 12] == -1)
    {
        return false;
    }

    *(uint32_t*)(m_TLB_WriteMap[VAddr >> 12] + VAddr) = Value;
    return true;
}

bool CMipsMemoryVM::SD_VAddr(uint32_t VAddr, uint64_t Value)
{
    if (m_TLB_WriteMap[VAddr >> 12] == -1)
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
    return m_TLB_ReadMap[VAddr >> 12] != -1;
}

bool CMipsMemoryVM::VAddrToRealAddr(uint32_t VAddr, void * &RealAddress) const
{
    if (m_TLB_ReadMap[VAddr >> 12] == -1)
    {
        return false;
    }
    RealAddress = (uint8_t *)(m_TLB_ReadMap[VAddr >> 12] + VAddr);
    return true;
}

bool CMipsMemoryVM::TranslateVaddr(uint32_t VAddr, uint32_t &PAddr) const
{
    // Change the virtual address to a physical address
    if (m_TLB_ReadMap[VAddr >> 12] == -1)
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
    }
    *Value = 0;
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
    *Value = 0;
    return false;
}

bool CMipsMemoryVM::LW_NonMemory(uint32_t PAddr, uint32_t* Value)
{
    m_MemLookupAddress = PAddr;
    if (PAddr >= 0x10000000 && PAddr < 0x16000000)
    {
        Load32Rom();
    }
    else
    {
        switch (PAddr & 0xFFF00000)
        {
        case 0x03F00000: m_RDRAMRegistersHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04000000: m_SPRegistersHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04100000: m_DPCommandRegistersHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04300000: m_MIPSInterfaceHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04400000: m_VideoInterfaceHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04500000: m_AudioInterfaceHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04600000: m_PeripheralInterfaceHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04700000: m_RDRAMInterfaceHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x04800000: m_SerialInterfaceHandler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
        case 0x05000000: m_CartridgeDomain2Address1Handler.Read32(PAddr, m_MemLookupValue.UW[0]); break;
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
        if (PAddr < RdramSize())
        {
            g_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF, 0x1000, CRecompiler::Remove_ProtectedMem);
            ::ProtectMemory(m_RDRAM + (PAddr & ~0xFFF), 0xFFC, MEM_READWRITE);
            *(uint32_t *)(m_RDRAM + PAddr) = Value;
        }
        break;
    case 0x03F00000: m_RDRAMRegistersHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04000000:
        if (PAddr < 0x04002000)
        {
            g_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF, 0xFFF, CRecompiler::Remove_ProtectedMem);
            *(uint32_t *)(m_RDRAM + PAddr) = Value;
        }
        else
        {
            m_SPRegistersHandler.Write32(PAddr, Value, 0xFFFFFFFF);
        }
        break;
    case 0x04100000: m_DPCommandRegistersHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04300000: m_MIPSInterfaceHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04400000: m_VideoInterfaceHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04500000: m_AudioInterfaceHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04600000: m_PeripheralInterfaceHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04700000: m_RDRAMInterfaceHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x04800000: m_SerialInterfaceHandler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x05000000: m_CartridgeDomain2Address1Handler.Write32(PAddr, Value, 0xFFFFFFFF); break;
    case 0x08000000: Write32CartridgeDomain2Address2(); break;
    case 0x1FC00000: Write32PifRam(); break;
    default:
        return false;
        break;
    }

    return true;
}

void CMipsMemoryVM::ProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr)
{
    WriteTrace(TraceProtectedMem, TraceDebug, "StartVaddr: %08X EndVaddr: %08X", StartVaddr, EndVaddr);
    if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr))
    {
        return;
    }

    // Get physical addresses passed
    uint32_t StartPAddr, EndPAddr;
    if (!TranslateVaddr(StartVaddr, StartPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (!TranslateVaddr(EndVaddr, EndPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    // Get length of memory being protected
    int32_t Length = ((EndPAddr + 3) - StartPAddr) & ~3;
    if (Length < 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    // Protect that memory address space
    uint8_t * MemLoc = Rdram() + StartPAddr;
    WriteTrace(TraceProtectedMem, TraceDebug, "Paddr: %08X Length: %X", StartPAddr, Length);

    ::ProtectMemory(MemLoc, Length, MEM_READONLY);
}

void CMipsMemoryVM::UnProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr)
{
    WriteTrace(TraceProtectedMem, TraceDebug, "StartVaddr: %08X EndVaddr: %08X", StartVaddr, EndVaddr);
    if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr)) { return; }

    // Get physical addresses passed
    uint32_t StartPAddr, EndPAddr;
    if (!TranslateVaddr(StartVaddr, StartPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (!TranslateVaddr(EndVaddr, EndPAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    // Get length of memory being protected
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
        m_TLB_ReadMap[Index] = -1;
        m_TLB_WriteMap[Index] = -1;
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
        if (result == nullptr)
        {
            WriteTrace(TraceN64System, TraceError, "Failed to allocate extended memory");
            g_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
        }
    }

    if (new_size > 0xFFFFFFFFul)
    { // Should be unreachable because:  size_t new_size = g_Settings->(uint32_t)
        g_Notify->BreakPoint(__FILE__, __LINE__);
    } // However, FFFFFFFF also is a limit to RCP addressing, so we care
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
    if ((RegModValue & SP_SET_INTR) != 0 && HaveDebugger())
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

void CMipsMemoryVM::Load32CartridgeDomain1Address1(void)
{
    // 64DD IPL ROM
    if (g_DDRom != nullptr && (m_MemLookupAddress & 0xFFFFFF) < g_MMU->m_DDRomSize)
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

void CMipsMemoryVM::Load32CartridgeDomain2Address2(void)
{
    uint32_t offset = (m_MemLookupAddress & 0x1FFFFFFF) - 0x08000000;
    if (offset > 0x88000)
    {
        m_MemLookupValue.UW[0] = ((offset & 0xFFFF) << 16) | (offset & 0xFFFF);
        return;
    }
    if (g_System->m_SaveUsing == SaveChip_Auto)
    {
        g_System->m_SaveUsing = SaveChip_FlashRam;
    }
    if (g_System->m_SaveUsing == SaveChip_Sram)
    {
        // Load SRAM
        uint8_t tmp[4] = "";
        g_MMU->DmaFromSram(tmp, offset, 4);
        m_MemLookupValue.UW[0] = tmp[3] << 24 | tmp[2] << 16 | tmp[1] << 8 | tmp[0];
    }
    else if (g_System->m_SaveUsing != SaveChip_FlashRam)
    {
        if (HaveDebugger())
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
        if (HaveDebugger())
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
        if (HaveDebugger())
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
        //LogMessage("%X: Read crap from ROM %08X from %08X",PROGRAM_COUNTER,*Value,PAddr);
        g_MMU->m_RomWrittenTo = false;
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

void CMipsMemoryVM::Write32CartridgeDomain2Address2(void)
{
    uint32_t offset = (m_MemLookupAddress & 0x1FFFFFFF) - 0x08000000;
    if (g_System->m_SaveUsing == SaveChip_Sram && offset < 0x88000)
    {
        // Store SRAM
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
    if (HaveDebugger())
    {
    g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    }*/
    if (offset > 0x10000)
    {
        return;
    }
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
        if (HaveDebugger())
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