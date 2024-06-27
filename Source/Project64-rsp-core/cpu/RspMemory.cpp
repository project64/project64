#include <Common/MemoryManagement.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Settings/Settings.h>
#include <stdio.h>
#include <string.h>
#include <zlib/zlib.h>

enum
{
    MaxMaps = 32
};

uint32_t NoOfMaps, MapsCRC[MaxMaps];
uint32_t Table;
uint8_t *RecompCode, *RecompCodeSecondary, *RecompPos, *JumpTables;
void ** JumpTable;

int AllocateMemory(void)
{
    if (RecompCode == nullptr)
    {
        RecompCode = (uint8_t *)AllocateAddressSpace(0x00400004);
        RecompCode = (uint8_t *)CommitMemory(RecompCode, 0x00400000, MEM_EXECUTE_READWRITE);

        if (RecompCode == nullptr)
        {
            g_Notify->DisplayError("Not enough memory for RSP RecompCode!");
            return false;
        }
    }

    if (RecompCodeSecondary == nullptr)
    {
        RecompCodeSecondary = (uint8_t *)AllocateAddressSpace(0x00200004);
        RecompCodeSecondary = (uint8_t *)CommitMemory(RecompCodeSecondary, 0x00200000, MEM_EXECUTE_READWRITE);
        if (RecompCodeSecondary == nullptr)
        {
            g_Notify->DisplayError("Not enough memory for RSP RecompCode Secondary!");
            return false;
        }
    }

    if (JumpTables == nullptr)
    {
        JumpTables = (uint8_t *)AllocateAddressSpace(0x1000 * MaxMaps);
        JumpTables = (uint8_t *)CommitMemory(JumpTables, 0x1000 * MaxMaps, MEM_READWRITE);
        if (JumpTables == nullptr)
        {
            g_Notify->DisplayError("Not enough memory for jump table!");
            return false;
        }
    }

    JumpTable = (void **)JumpTables;
    RecompPos = RecompCode;
    NoOfMaps = 0;
    return true;
}

void FreeMemory(void)
{
    FreeAddressSpace(RecompCode, 0x00400004);
    FreeAddressSpace(JumpTable, 0x1000 * MaxMaps);
    FreeAddressSpace(RecompCodeSecondary, 0x00200004);

    RecompCode = nullptr;
    JumpTables = nullptr;
    RecompCodeSecondary = nullptr;
}

void ResetJumpTables(void)
{
    memset(JumpTables, 0, 0x1000 * MaxMaps);
    RecompPos = RecompCode;
    NoOfMaps = 0;
}

void SetJumpTable(uint32_t End)
{
    uint32_t CRC = crc32(0L, Z_NULL, 0);
    if (End < 0x800)
    {
        End = 0x800;
    }

    if (End == 0x1000 && ((g_RSPRegisterHandler->PendingSPMemAddr() & 0x0FFF) & ~7) == 0x80)
    {
        End = 0x800;
    }

    CRC = crc32(CRC, RSPInfo.IMEM, End);
    for (uint32_t i = 0; i < NoOfMaps; i++)
    {
        if (CRC == MapsCRC[i])
        {
            JumpTable = (void **)(JumpTables + i * 0x1000);
            Table = i;
            return;
        }
    }
    //DisplayError("%X %X",NoOfMaps,CRC);
    if (NoOfMaps == MaxMaps)
    {
        ResetJumpTables();
    }
    MapsCRC[NoOfMaps] = CRC;
    JumpTable = (void **)(JumpTables + NoOfMaps * 0x1000);
    Table = NoOfMaps;
    NoOfMaps += 1;
}

void RSP_LW_IMEM(uint32_t Addr, uint32_t * Value)
{
    if ((Addr & 0x3) != 0)
    {
        g_Notify->DisplayError("Unaligned RSP_LW_IMEM");
    }
    *Value = *(uint32_t *)(RSPInfo.IMEM + (Addr & 0xFFF));
}