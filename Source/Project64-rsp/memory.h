#include "cpu/RspTypes.h"

int AllocateMemory(void);
void FreeMemory(void);
void SetJumpTable(uint32_t End);

extern uint8_t *RecompCode, *RecompCodeSecondary, *RecompPos;
extern void ** JumpTable;
extern uint32_t Table;

void RSP_LB_DMEM(uint32_t Addr, uint8_t * Value);
void RSP_LBV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LDV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LFV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LH_DMEM(uint32_t Addr, uint16_t * Value);
void RSP_LHV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LLV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LPV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LRV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LQV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LSV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LTV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LUV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_LW_DMEM(uint32_t Addr, uint32_t * Value);
void RSP_LW_IMEM(uint32_t Addr, uint32_t * Value);
void RSP_SB_DMEM(uint32_t Addr, uint8_t Value);
void RSP_SBV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SDV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SFV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SH_DMEM(uint32_t Addr, uint16_t Value);
void RSP_SHV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SLV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SPV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SQV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SRV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SSV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_STV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SUV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
void RSP_SW_DMEM(uint32_t Addr, uint32_t Value);
void RSP_SWV_DMEM(uint32_t Addr, uint8_t vect, uint8_t element);
