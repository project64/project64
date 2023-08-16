#pragma once

#include <Project64-rsp-core/cpu/RspTypes.h>

int AllocateMemory(void);
void FreeMemory(void);
void SetJumpTable(uint32_t End);

extern uint8_t *RecompCode, *RecompCodeSecondary, *RecompPos;
extern void ** JumpTable;
extern uint32_t Table;

void RSP_LW_IMEM(uint32_t Addr, uint32_t * Value);
void RSP_SWV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
