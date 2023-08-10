#include <stdint.h>

enum RSPPIPELINE_STAGE
{
    RSPPIPELINE_NORMAL = 0,
    RSPPIPELINE_DO_DELAY_SLOT = 1,
    RSPPIPELINE_DELAY_SLOT = 2,
    RSPPIPELINE_DELAY_SLOT_DONE = 3,
    RSPPIPELINE_DELAY_SLOT_EXIT = 4,
    RSPPIPELINE_DELAY_SLOT_EXIT_DONE = 5,
    RSPPIPELINE_JUMP = 6,
    RSPPIPELINE_SINGLE_STEP = 7,
    RSPPIPELINE_SINGLE_STEP_DONE = 8,
    RSPPIPELINE_FINISH_BLOCK = 9,
    RSPPIPELINE_FINISH_SUB_BLOCK = 10,
};

extern RSPPIPELINE_STAGE RSP_NextInstruction;
extern uint32_t RSP_JumpTo;
extern uint32_t RSP_MfStatusCount;

// Standard MIPS PC-relative branch
// Returns the new PC, based on whether the condition passes

unsigned int RSP_branch_if(int condition);

void BuildInterpreterCPU(void);
uint32_t RunInterpreterCPU(uint32_t Cycles);
