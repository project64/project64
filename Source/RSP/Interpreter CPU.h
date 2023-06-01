#define NORMAL				    0
#define DO_DELAY_SLOT 			1
#define DELAY_SLOT 				2
#define DELAY_SLOT_DONE			3
#define DELAY_SLOT_EXIT			4
#define DELAY_SLOT_EXIT_DONE	5
#define JUMP	 				6
#define SINGLE_STEP	 		    7
#define SINGLE_STEP_DONE		8
#define FINISH_BLOCK			9
#define FINISH_SUB_BLOCK		10

extern DWORD RSP_NextInstruction, RSP_JumpTo;
extern uint32_t RSP_MfStatusCount;

// Standard MIPS PC-relative branch
// Returns the new PC, based on whether the condition passes

unsigned int RSP_branch_if(int condition);

void BuildInterpreterCPU(void);
DWORD RunInterpreterCPU(DWORD Cycles);
