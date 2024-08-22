#pragma once

#include <Project64-rsp-core/cpu/RspTypes.h>

enum
{
    MaxMaps = 32
};

int AllocateMemory(void);
void FreeMemory(void);

extern uint8_t *RecompCode, *RecompCodeSecondary, *RecompPos;
extern void ** JumpTable;
extern uint32_t Table;

void RSP_LW_IMEM(uint32_t Addr, uint32_t * Value);
