enum
{
    MaxMaps = 32
};

#include "Rsp.h"
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <windows.h>

uint32_t NoOfMaps, MapsCRC[MaxMaps];
uint32_t Table;
BYTE *RecompCode, *RecompCodeSecondary, *RecompPos, *JumpTables;
void ** JumpTable;

int AllocateMemory(void)
{
    if (RecompCode == NULL)
    {
        RecompCode = (BYTE *)VirtualAlloc(NULL, 0x00400004, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        RecompCode = (BYTE *)VirtualAlloc(RecompCode, 0x00400000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

        if (RecompCode == NULL)
        {
            DisplayError("Not enough memory for RSP RecompCode!");
            return false;
        }
    }

    if (RecompCodeSecondary == NULL)
    {
        RecompCodeSecondary = (BYTE *)VirtualAlloc(NULL, 0x00200000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (RecompCodeSecondary == NULL)
        {
            DisplayError("Not enough memory for RSP RecompCode Secondary!");
            return false;
        }
    }

    if (JumpTables == NULL)
    {
        JumpTables = (BYTE *)VirtualAlloc(NULL, 0x1000 * MaxMaps, MEM_COMMIT, PAGE_READWRITE);
        if (JumpTables == NULL)
        {
            DisplayError("Not enough memory for jump table!");
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
    VirtualFree(RecompCode, 0, MEM_RELEASE);
    VirtualFree(JumpTable, 0, MEM_RELEASE);
    VirtualFree(RecompCodeSecondary, 0, MEM_RELEASE);

    RecompCode = NULL;
    JumpTables = NULL;
    RecompCodeSecondary = NULL;
}

void ResetJumpTables(void)
{
    memset(JumpTables, 0, 0x1000 * MaxMaps);
    RecompPos = RecompCode;
    NoOfMaps = 0;
}

void SetJumpTable(uint32_t End)
{
    uint32_t CRC, count;

    CRC = 0;
    if (End < 0x800)
    {
        End = 0x800;
    }

    if (End == 0x1000 && ((*RSPInfo.SP_MEM_ADDR_REG & 0x0FFF) & ~7) == 0x80)
    {
        End = 0x800;
    }

    for (count = 0; count < End; count += 0x40)
    {
        CRC += *(uint32_t *)(RSPInfo.IMEM + count);
    }

    for (count = 0; count < NoOfMaps; count++)
    {
        if (CRC == MapsCRC[count])
        {
            JumpTable = (void **)(JumpTables + count * 0x1000);
            Table = count;
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
        DisplayError("Unaligned RSP_LW_IMEM");
    }
    *Value = *(uint32_t *)(RSPInfo.IMEM + (Addr & 0xFFF));
}