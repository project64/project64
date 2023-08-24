#include "RspRecompilerCPU.h"
#include "RspProfiling.h"
#include "RspRecompilerOps.h"
#include "x86.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPInstruction.h>
#include <Project64-rsp-core/cpu/RSPInterpreterCPU.h>
#include <Project64-rsp-core/cpu/RSPOpcode.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <float.h>

#pragma warning(disable : 4152) // Non-standard extension, function/data pointer conversion in expression

// #define REORDER_BLOCK_VERBOSE
#define LINK_BRANCHES_VERBOSE // No choice really
#define X86_RECOMP_VERBOSE
#define BUILD_BRANCHLABELS_VERBOSE

uint32_t CompilePC, JumpTableSize, BlockID = 0;
uint32_t dwBuffer = MainBuffer;
bool ChangedPC;

RSP_BLOCK CurrentBlock;
RSP_CODE RspCode;

uint8_t *pLastSecondary = NULL, *pLastPrimary = NULL;

void BuildRecompilerCPU(void)
{
    RSP_Opcode[0] = Compile_SPECIAL;
    RSP_Opcode[1] = Compile_REGIMM;
    RSP_Opcode[2] = Compile_J;
    RSP_Opcode[3] = Compile_JAL;
    RSP_Opcode[4] = Compile_BEQ;
    RSP_Opcode[5] = Compile_BNE;
    RSP_Opcode[6] = Compile_BLEZ;
    RSP_Opcode[7] = Compile_BGTZ;
    RSP_Opcode[8] = Compile_ADDI;
    RSP_Opcode[9] = Compile_ADDIU;
    RSP_Opcode[10] = Compile_SLTI;
    RSP_Opcode[11] = Compile_SLTIU;
    RSP_Opcode[12] = Compile_ANDI;
    RSP_Opcode[13] = Compile_ORI;
    RSP_Opcode[14] = Compile_XORI;
    RSP_Opcode[15] = Compile_LUI;
    RSP_Opcode[16] = Compile_COP0;
    RSP_Opcode[17] = Compile_UnknownOpcode;
    RSP_Opcode[18] = Compile_COP2;
    RSP_Opcode[19] = Compile_UnknownOpcode;
    RSP_Opcode[20] = Compile_UnknownOpcode;
    RSP_Opcode[21] = Compile_UnknownOpcode;
    RSP_Opcode[22] = Compile_UnknownOpcode;
    RSP_Opcode[23] = Compile_UnknownOpcode;
    RSP_Opcode[24] = Compile_UnknownOpcode;
    RSP_Opcode[25] = Compile_UnknownOpcode;
    RSP_Opcode[26] = Compile_UnknownOpcode;
    RSP_Opcode[27] = Compile_UnknownOpcode;
    RSP_Opcode[28] = Compile_UnknownOpcode;
    RSP_Opcode[29] = Compile_UnknownOpcode;
    RSP_Opcode[30] = Compile_UnknownOpcode;
    RSP_Opcode[31] = Compile_UnknownOpcode;
    RSP_Opcode[32] = Compile_LB;
    RSP_Opcode[33] = Compile_LH;
    RSP_Opcode[34] = Compile_UnknownOpcode;
    RSP_Opcode[35] = Compile_LW;
    RSP_Opcode[36] = Compile_LBU;
    RSP_Opcode[37] = Compile_LHU;
    RSP_Opcode[38] = Compile_UnknownOpcode;
    RSP_Opcode[39] = Compile_UnknownOpcode;
    RSP_Opcode[40] = Compile_SB;
    RSP_Opcode[41] = Compile_SH;
    RSP_Opcode[42] = Compile_UnknownOpcode;
    RSP_Opcode[43] = Compile_SW;
    RSP_Opcode[44] = Compile_UnknownOpcode;
    RSP_Opcode[45] = Compile_UnknownOpcode;
    RSP_Opcode[46] = Compile_UnknownOpcode;
    RSP_Opcode[47] = Compile_UnknownOpcode;
    RSP_Opcode[48] = Compile_UnknownOpcode;
    RSP_Opcode[49] = Compile_UnknownOpcode;
    RSP_Opcode[50] = Compile_LC2;
    RSP_Opcode[51] = Compile_UnknownOpcode;
    RSP_Opcode[52] = Compile_UnknownOpcode;
    RSP_Opcode[53] = Compile_UnknownOpcode;
    RSP_Opcode[54] = Compile_UnknownOpcode;
    RSP_Opcode[55] = Compile_UnknownOpcode;
    RSP_Opcode[56] = Compile_UnknownOpcode;
    RSP_Opcode[57] = Compile_UnknownOpcode;
    RSP_Opcode[58] = Compile_SC2;
    RSP_Opcode[59] = Compile_UnknownOpcode;
    RSP_Opcode[60] = Compile_UnknownOpcode;
    RSP_Opcode[61] = Compile_UnknownOpcode;
    RSP_Opcode[62] = Compile_UnknownOpcode;
    RSP_Opcode[63] = Compile_UnknownOpcode;

    RSP_Special[0] = Compile_Special_SLL;
    RSP_Special[1] = Compile_UnknownOpcode;
    RSP_Special[2] = Compile_Special_SRL;
    RSP_Special[3] = Compile_Special_SRA;
    RSP_Special[4] = Compile_Special_SLLV;
    RSP_Special[5] = Compile_UnknownOpcode;
    RSP_Special[6] = Compile_Special_SRLV;
    RSP_Special[7] = Compile_Special_SRAV;
    RSP_Special[8] = Compile_Special_JR;
    RSP_Special[9] = Compile_Special_JALR;
    RSP_Special[10] = Compile_UnknownOpcode;
    RSP_Special[11] = Compile_UnknownOpcode;
    RSP_Special[12] = Compile_UnknownOpcode;
    RSP_Special[13] = Compile_Special_BREAK;
    RSP_Special[14] = Compile_UnknownOpcode;
    RSP_Special[15] = Compile_UnknownOpcode;
    RSP_Special[16] = Compile_UnknownOpcode;
    RSP_Special[17] = Compile_UnknownOpcode;
    RSP_Special[18] = Compile_UnknownOpcode;
    RSP_Special[19] = Compile_UnknownOpcode;
    RSP_Special[20] = Compile_UnknownOpcode;
    RSP_Special[21] = Compile_UnknownOpcode;
    RSP_Special[22] = Compile_UnknownOpcode;
    RSP_Special[23] = Compile_UnknownOpcode;
    RSP_Special[24] = Compile_UnknownOpcode;
    RSP_Special[25] = Compile_UnknownOpcode;
    RSP_Special[26] = Compile_UnknownOpcode;
    RSP_Special[27] = Compile_UnknownOpcode;
    RSP_Special[28] = Compile_UnknownOpcode;
    RSP_Special[29] = Compile_UnknownOpcode;
    RSP_Special[30] = Compile_UnknownOpcode;
    RSP_Special[31] = Compile_UnknownOpcode;
    RSP_Special[32] = Compile_Special_ADD;
    RSP_Special[33] = Compile_Special_ADDU;
    RSP_Special[34] = Compile_Special_SUB;
    RSP_Special[35] = Compile_Special_SUBU;
    RSP_Special[36] = Compile_Special_AND;
    RSP_Special[37] = Compile_Special_OR;
    RSP_Special[38] = Compile_Special_XOR;
    RSP_Special[39] = Compile_Special_NOR;
    RSP_Special[40] = Compile_UnknownOpcode;
    RSP_Special[41] = Compile_UnknownOpcode;
    RSP_Special[42] = Compile_Special_SLT;
    RSP_Special[43] = Compile_Special_SLTU;
    RSP_Special[44] = Compile_UnknownOpcode;
    RSP_Special[45] = Compile_UnknownOpcode;
    RSP_Special[46] = Compile_UnknownOpcode;
    RSP_Special[47] = Compile_UnknownOpcode;
    RSP_Special[48] = Compile_UnknownOpcode;
    RSP_Special[49] = Compile_UnknownOpcode;
    RSP_Special[50] = Compile_UnknownOpcode;
    RSP_Special[51] = Compile_UnknownOpcode;
    RSP_Special[52] = Compile_UnknownOpcode;
    RSP_Special[53] = Compile_UnknownOpcode;
    RSP_Special[54] = Compile_UnknownOpcode;
    RSP_Special[55] = Compile_UnknownOpcode;
    RSP_Special[56] = Compile_UnknownOpcode;
    RSP_Special[57] = Compile_UnknownOpcode;
    RSP_Special[58] = Compile_UnknownOpcode;
    RSP_Special[59] = Compile_UnknownOpcode;
    RSP_Special[60] = Compile_UnknownOpcode;
    RSP_Special[61] = Compile_UnknownOpcode;
    RSP_Special[62] = Compile_UnknownOpcode;
    RSP_Special[63] = Compile_UnknownOpcode;

    RSP_RegImm[0] = Compile_RegImm_BLTZ;
    RSP_RegImm[1] = Compile_RegImm_BGEZ;
    RSP_RegImm[2] = Compile_UnknownOpcode;
    RSP_RegImm[3] = Compile_UnknownOpcode;
    RSP_RegImm[4] = Compile_UnknownOpcode;
    RSP_RegImm[5] = Compile_UnknownOpcode;
    RSP_RegImm[6] = Compile_UnknownOpcode;
    RSP_RegImm[7] = Compile_UnknownOpcode;
    RSP_RegImm[8] = Compile_UnknownOpcode;
    RSP_RegImm[9] = Compile_UnknownOpcode;
    RSP_RegImm[10] = Compile_UnknownOpcode;
    RSP_RegImm[11] = Compile_UnknownOpcode;
    RSP_RegImm[12] = Compile_UnknownOpcode;
    RSP_RegImm[13] = Compile_UnknownOpcode;
    RSP_RegImm[14] = Compile_UnknownOpcode;
    RSP_RegImm[15] = Compile_UnknownOpcode;
    RSP_RegImm[16] = Compile_RegImm_BLTZAL;
    RSP_RegImm[17] = Compile_RegImm_BGEZAL;
    RSP_RegImm[18] = Compile_UnknownOpcode;
    RSP_RegImm[19] = Compile_UnknownOpcode;
    RSP_RegImm[20] = Compile_UnknownOpcode;
    RSP_RegImm[21] = Compile_UnknownOpcode;
    RSP_RegImm[22] = Compile_UnknownOpcode;
    RSP_RegImm[23] = Compile_UnknownOpcode;
    RSP_RegImm[24] = Compile_UnknownOpcode;
    RSP_RegImm[25] = Compile_UnknownOpcode;
    RSP_RegImm[26] = Compile_UnknownOpcode;
    RSP_RegImm[27] = Compile_UnknownOpcode;
    RSP_RegImm[28] = Compile_UnknownOpcode;
    RSP_RegImm[29] = Compile_UnknownOpcode;
    RSP_RegImm[30] = Compile_UnknownOpcode;
    RSP_RegImm[31] = Compile_UnknownOpcode;

    RSP_Cop0[0] = Compile_Cop0_MF;
    RSP_Cop0[1] = Compile_UnknownOpcode;
    RSP_Cop0[2] = Compile_UnknownOpcode;
    RSP_Cop0[3] = Compile_UnknownOpcode;
    RSP_Cop0[4] = Compile_Cop0_MT;
    RSP_Cop0[5] = Compile_UnknownOpcode;
    RSP_Cop0[6] = Compile_UnknownOpcode;
    RSP_Cop0[7] = Compile_UnknownOpcode;
    RSP_Cop0[8] = Compile_UnknownOpcode;
    RSP_Cop0[9] = Compile_UnknownOpcode;
    RSP_Cop0[10] = Compile_UnknownOpcode;
    RSP_Cop0[11] = Compile_UnknownOpcode;
    RSP_Cop0[12] = Compile_UnknownOpcode;
    RSP_Cop0[13] = Compile_UnknownOpcode;
    RSP_Cop0[14] = Compile_UnknownOpcode;
    RSP_Cop0[15] = Compile_UnknownOpcode;
    RSP_Cop0[16] = Compile_UnknownOpcode;
    RSP_Cop0[17] = Compile_UnknownOpcode;
    RSP_Cop0[18] = Compile_UnknownOpcode;
    RSP_Cop0[19] = Compile_UnknownOpcode;
    RSP_Cop0[20] = Compile_UnknownOpcode;
    RSP_Cop0[21] = Compile_UnknownOpcode;
    RSP_Cop0[22] = Compile_UnknownOpcode;
    RSP_Cop0[23] = Compile_UnknownOpcode;
    RSP_Cop0[24] = Compile_UnknownOpcode;
    RSP_Cop0[25] = Compile_UnknownOpcode;
    RSP_Cop0[26] = Compile_UnknownOpcode;
    RSP_Cop0[27] = Compile_UnknownOpcode;
    RSP_Cop0[28] = Compile_UnknownOpcode;
    RSP_Cop0[29] = Compile_UnknownOpcode;
    RSP_Cop0[30] = Compile_UnknownOpcode;
    RSP_Cop0[31] = Compile_UnknownOpcode;

    RSP_Cop2[0] = Compile_Cop2_MF;
    RSP_Cop2[1] = Compile_UnknownOpcode;
    RSP_Cop2[2] = Compile_Cop2_CF;
    RSP_Cop2[3] = Compile_UnknownOpcode;
    RSP_Cop2[4] = Compile_Cop2_MT;
    RSP_Cop2[5] = Compile_UnknownOpcode;
    RSP_Cop2[6] = Compile_Cop2_CT;
    RSP_Cop2[7] = Compile_UnknownOpcode;
    RSP_Cop2[8] = Compile_UnknownOpcode;
    RSP_Cop2[9] = Compile_UnknownOpcode;
    RSP_Cop2[10] = Compile_UnknownOpcode;
    RSP_Cop2[11] = Compile_UnknownOpcode;
    RSP_Cop2[12] = Compile_UnknownOpcode;
    RSP_Cop2[13] = Compile_UnknownOpcode;
    RSP_Cop2[14] = Compile_UnknownOpcode;
    RSP_Cop2[15] = Compile_UnknownOpcode;
    RSP_Cop2[16] = Compile_COP2_VECTOR;
    RSP_Cop2[17] = Compile_COP2_VECTOR;
    RSP_Cop2[18] = Compile_COP2_VECTOR;
    RSP_Cop2[19] = Compile_COP2_VECTOR;
    RSP_Cop2[20] = Compile_COP2_VECTOR;
    RSP_Cop2[21] = Compile_COP2_VECTOR;
    RSP_Cop2[22] = Compile_COP2_VECTOR;
    RSP_Cop2[23] = Compile_COP2_VECTOR;
    RSP_Cop2[24] = Compile_COP2_VECTOR;
    RSP_Cop2[25] = Compile_COP2_VECTOR;
    RSP_Cop2[26] = Compile_COP2_VECTOR;
    RSP_Cop2[27] = Compile_COP2_VECTOR;
    RSP_Cop2[28] = Compile_COP2_VECTOR;
    RSP_Cop2[29] = Compile_COP2_VECTOR;
    RSP_Cop2[30] = Compile_COP2_VECTOR;
    RSP_Cop2[31] = Compile_COP2_VECTOR;

    RSP_Vector[0] = Compile_Vector_VMULF;
    RSP_Vector[1] = Compile_Vector_VMULU;
    RSP_Vector[2] = Compile_UnknownOpcode;
    RSP_Vector[3] = Compile_UnknownOpcode;
    RSP_Vector[4] = Compile_Vector_VMUDL;
    RSP_Vector[5] = Compile_Vector_VMUDM;
    RSP_Vector[6] = Compile_Vector_VMUDN;
    RSP_Vector[7] = Compile_Vector_VMUDH;
    RSP_Vector[8] = Compile_Vector_VMACF;
    RSP_Vector[9] = Compile_Vector_VMACU;
    RSP_Vector[10] = Compile_UnknownOpcode;
    RSP_Vector[11] = Compile_Vector_VMACQ;
    RSP_Vector[12] = Compile_Vector_VMADL;
    RSP_Vector[13] = Compile_Vector_VMADM;
    RSP_Vector[14] = Compile_Vector_VMADN;
    RSP_Vector[15] = Compile_Vector_VMADH;
    RSP_Vector[16] = Compile_Vector_VADD;
    RSP_Vector[17] = Compile_Vector_VSUB;
    RSP_Vector[18] = Compile_UnknownOpcode;
    RSP_Vector[19] = Compile_Vector_VABS;
    RSP_Vector[20] = Compile_Vector_VADDC;
    RSP_Vector[21] = Compile_Vector_VSUBC;
    RSP_Vector[22] = Compile_UnknownOpcode;
    RSP_Vector[23] = Compile_UnknownOpcode;
    RSP_Vector[24] = Compile_UnknownOpcode;
    RSP_Vector[25] = Compile_UnknownOpcode;
    RSP_Vector[26] = Compile_UnknownOpcode;
    RSP_Vector[27] = Compile_UnknownOpcode;
    RSP_Vector[28] = Compile_UnknownOpcode;
    RSP_Vector[29] = Compile_Vector_VSAW;
    RSP_Vector[30] = Compile_UnknownOpcode;
    RSP_Vector[31] = Compile_UnknownOpcode;
    RSP_Vector[32] = Compile_Vector_VLT;
    RSP_Vector[33] = Compile_Vector_VEQ;
    RSP_Vector[34] = Compile_Vector_VNE;
    RSP_Vector[35] = Compile_Vector_VGE;
    RSP_Vector[36] = Compile_Vector_VCL;
    RSP_Vector[37] = Compile_Vector_VCH;
    RSP_Vector[38] = Compile_Vector_VCR;
    RSP_Vector[39] = Compile_Vector_VMRG;
    RSP_Vector[40] = Compile_Vector_VAND;
    RSP_Vector[41] = Compile_Vector_VNAND;
    RSP_Vector[42] = Compile_Vector_VOR;
    RSP_Vector[43] = Compile_Vector_VNOR;
    RSP_Vector[44] = Compile_Vector_VXOR;
    RSP_Vector[45] = Compile_Vector_VNXOR;
    RSP_Vector[46] = Compile_UnknownOpcode;
    RSP_Vector[47] = Compile_UnknownOpcode;
    RSP_Vector[48] = Compile_Vector_VRCP;
    RSP_Vector[49] = Compile_Vector_VRCPL;
    RSP_Vector[50] = Compile_Vector_VRCPH;
    RSP_Vector[51] = Compile_Vector_VMOV;
    RSP_Vector[52] = Compile_Vector_VRSQ;
    RSP_Vector[53] = Compile_Vector_VRSQL;
    RSP_Vector[54] = Compile_Vector_VRSQH;
    RSP_Vector[55] = Compile_Vector_VNOOP;
    RSP_Vector[56] = Compile_UnknownOpcode;
    RSP_Vector[57] = Compile_UnknownOpcode;
    RSP_Vector[58] = Compile_UnknownOpcode;
    RSP_Vector[59] = Compile_UnknownOpcode;
    RSP_Vector[60] = Compile_UnknownOpcode;
    RSP_Vector[61] = Compile_UnknownOpcode;
    RSP_Vector[62] = Compile_UnknownOpcode;
    RSP_Vector[63] = Compile_UnknownOpcode;

    RSP_Lc2[0] = Compile_Opcode_LBV;
    RSP_Lc2[1] = Compile_Opcode_LSV;
    RSP_Lc2[2] = Compile_Opcode_LLV;
    RSP_Lc2[3] = Compile_Opcode_LDV;
    RSP_Lc2[4] = Compile_Opcode_LQV;
    RSP_Lc2[5] = Compile_Opcode_LRV;
    RSP_Lc2[6] = Compile_Opcode_LPV;
    RSP_Lc2[7] = Compile_Opcode_LUV;
    RSP_Lc2[8] = Compile_Opcode_LHV;
    RSP_Lc2[9] = Compile_Opcode_LFV;
    RSP_Lc2[10] = Compile_UnknownOpcode;
    RSP_Lc2[11] = Compile_Opcode_LTV;
    RSP_Lc2[12] = Compile_UnknownOpcode;
    RSP_Lc2[13] = Compile_UnknownOpcode;
    RSP_Lc2[14] = Compile_UnknownOpcode;
    RSP_Lc2[15] = Compile_UnknownOpcode;
    RSP_Lc2[16] = Compile_UnknownOpcode;
    RSP_Lc2[17] = Compile_UnknownOpcode;
    RSP_Lc2[18] = Compile_UnknownOpcode;
    RSP_Lc2[19] = Compile_UnknownOpcode;
    RSP_Lc2[20] = Compile_UnknownOpcode;
    RSP_Lc2[21] = Compile_UnknownOpcode;
    RSP_Lc2[22] = Compile_UnknownOpcode;
    RSP_Lc2[23] = Compile_UnknownOpcode;
    RSP_Lc2[24] = Compile_UnknownOpcode;
    RSP_Lc2[25] = Compile_UnknownOpcode;
    RSP_Lc2[26] = Compile_UnknownOpcode;
    RSP_Lc2[27] = Compile_UnknownOpcode;
    RSP_Lc2[28] = Compile_UnknownOpcode;
    RSP_Lc2[29] = Compile_UnknownOpcode;
    RSP_Lc2[30] = Compile_UnknownOpcode;
    RSP_Lc2[31] = Compile_UnknownOpcode;

    RSP_Sc2[0] = Compile_Opcode_SBV;
    RSP_Sc2[1] = Compile_Opcode_SSV;
    RSP_Sc2[2] = Compile_Opcode_SLV;
    RSP_Sc2[3] = Compile_Opcode_SDV;
    RSP_Sc2[4] = Compile_Opcode_SQV;
    RSP_Sc2[5] = Compile_Opcode_SRV;
    RSP_Sc2[6] = Compile_Opcode_SPV;
    RSP_Sc2[7] = Compile_Opcode_SUV;
    RSP_Sc2[8] = Compile_Opcode_SHV;
    RSP_Sc2[9] = Compile_Opcode_SFV;
    RSP_Sc2[10] = Compile_Opcode_SWV;
    RSP_Sc2[11] = Compile_Opcode_STV;
    RSP_Sc2[12] = Compile_UnknownOpcode;
    RSP_Sc2[13] = Compile_UnknownOpcode;
    RSP_Sc2[14] = Compile_UnknownOpcode;
    RSP_Sc2[15] = Compile_UnknownOpcode;
    RSP_Sc2[16] = Compile_UnknownOpcode;
    RSP_Sc2[17] = Compile_UnknownOpcode;
    RSP_Sc2[18] = Compile_UnknownOpcode;
    RSP_Sc2[19] = Compile_UnknownOpcode;
    RSP_Sc2[20] = Compile_UnknownOpcode;
    RSP_Sc2[21] = Compile_UnknownOpcode;
    RSP_Sc2[22] = Compile_UnknownOpcode;
    RSP_Sc2[23] = Compile_UnknownOpcode;
    RSP_Sc2[24] = Compile_UnknownOpcode;
    RSP_Sc2[25] = Compile_UnknownOpcode;
    RSP_Sc2[26] = Compile_UnknownOpcode;
    RSP_Sc2[27] = Compile_UnknownOpcode;
    RSP_Sc2[28] = Compile_UnknownOpcode;
    RSP_Sc2[29] = Compile_UnknownOpcode;
    RSP_Sc2[30] = Compile_UnknownOpcode;
    RSP_Sc2[31] = Compile_UnknownOpcode;

    BlockID = 0;
    ChangedPC = false;
#ifdef Log_x86Code
    Start_x86_Log();
#endif
}

/*
ReOrderSubBlock
Description:
This can be done, but will be interesting to put
between branches labels, and actual branches, whichever
occurs first in code
*/

void ReOrderInstructions(uint32_t StartPC, uint32_t EndPC)
{
    uint32_t InstructionCount = EndPC - StartPC;
    uint32_t Count, ReorderedOps, CurrentPC;
    RSPOpcode PreviousOp, CurrentOp, RspOp;

    PreviousOp.Value = *(uint32_t *)(RSPInfo.IMEM + (StartPC & 0xFFC));

    if (IsOpcodeBranch(StartPC, PreviousOp))
    {
        // The sub block ends here anyway
        return;
    }

    if (IsOpcodeNop(StartPC) && IsOpcodeNop(StartPC + 4) && IsOpcodeNop(StartPC + 8))
    {
        // Don't even bother
        return;
    }

    CPU_Message("***** Doing reorder (%X to %X) *****", StartPC, EndPC);

    if (InstructionCount < 0x0010)
    {
        return;
    }
    if (InstructionCount > 0x0A00)
    {
        return;
    }

    CPU_Message(" Before:");
    for (Count = StartPC; Count < EndPC; Count += 4)
    {
        RSP_LW_IMEM(Count, &RspOp.Value);
        CPU_Message("  %X %s", Count, RSPInstruction(Count, RspOp.Value).NameAndParam().c_str());
    }

    for (Count = 0; Count < InstructionCount; Count += 4)
    {
        CurrentPC = StartPC;
        PreviousOp.Value = *(uint32_t *)(RSPInfo.IMEM + (CurrentPC & 0xFFC));
        ReorderedOps = 0;

        for (;;)
        {
            CurrentPC += 4;
            if (CurrentPC >= EndPC)
            {
                break;
            }
            CurrentOp.Value = *(uint32_t *)(RSPInfo.IMEM + CurrentPC);

            if (CompareInstructions(CurrentPC, &PreviousOp, &CurrentOp))
            {
                // Move current opcode up
                *(uint32_t *)(RSPInfo.IMEM + CurrentPC - 4) = CurrentOp.Value;
                *(uint32_t *)(RSPInfo.IMEM + CurrentPC) = PreviousOp.Value;

                ReorderedOps++;

#ifdef REORDER_BLOCK_VERBOSE
                CPU_Message("Swapped %X and %X", CurrentPC - 4, CurrentPC);
#endif
            }
            PreviousOp.Value = *(uint32_t *)(RSPInfo.IMEM + (CurrentPC & 0xFFC));

            if (IsOpcodeNop(CurrentPC) && IsOpcodeNop(CurrentPC + 4) && IsOpcodeNop(CurrentPC + 8))
            {
                CurrentPC = EndPC;
            }
        }

        if (ReorderedOps == 0)
        {
            Count = InstructionCount;
        }
    }

    CPU_Message(" After:");
    for (Count = StartPC; Count < EndPC; Count += 4)
    {
        RSP_LW_IMEM(Count, &RspOp.Value);
        CPU_Message("  %X %s", Count, RSPInstruction(Count, RspOp.Value).NameAndParam().c_str());
    }
    CPU_Message("");
}

void ReOrderSubBlock(RSP_BLOCK * Block)
{
    uint32_t end = 0x0ffc;
    uint32_t count;

    if (!Compiler.bReOrdering)
    {
        return;
    }
    if (Block->CurrPC > 0xFF0)
    {
        return;
    }

    // Find the label or jump closest to us
    if (RspCode.LabelCount)
    {
        for (count = 0; count < RspCode.LabelCount; count++)
        {
            if (RspCode.BranchLabels[count] < end && RspCode.BranchLabels[count] > Block->CurrPC)
            {
                end = RspCode.BranchLabels[count];
            }
        }
    }
    if (RspCode.BranchCount)
    {
        for (count = 0; count < RspCode.BranchCount; count++)
        {
            if (RspCode.BranchLocations[count] < end && RspCode.BranchLocations[count] > Block->CurrPC)
            {
                end = RspCode.BranchLocations[count];
            }
        }
    }
    // It wont actually re-order the op at the end
    ReOrderInstructions(Block->CurrPC, end);
}

/*
DetectGPRConstants
Description:
This needs to be called on a sub-block basis, like
after every time we hit a branch and delay slot
*/

void DetectGPRConstants(RSP_CODE * code)
{
    uint32_t Count, Constant = 0;

    memset(&code->bIsRegConst, 0, sizeof(bool) * 0x20);
    memset(&code->MipsRegConst, 0, sizeof(uint32_t) * 0x20);

    if (!Compiler.bGPRConstants)
    {
        return;
    }
    CPU_Message("***** Detecting constants *****");

    // R0 is constant zero, R31 or RA is not constant
    code->bIsRegConst[0] = true;
    code->MipsRegConst[0] = 0;

    // Do your global search for them
    for (Count = 1; Count < 31; Count++)
    {
        if (IsRegisterConstant(Count, &Constant))
        {
            CPU_Message("Global: %s is a constant of: %08X", GPR_Name(Count), Constant);
            code->bIsRegConst[Count] = true;
            code->MipsRegConst[Count] = Constant;
        }
    }
    CPU_Message("");
}

/*
CompilerToggleBuffer and ClearX86Code
Description:
1: Toggles the compiler buffer, useful for poorly
taken branches like alignment
2: Clears all the x86 code, jump tables etc.
*/

void CompilerToggleBuffer(void)
{
    if (dwBuffer == MainBuffer)
    {
        dwBuffer = SecondaryBuffer;
        pLastPrimary = RecompPos;

        if (pLastSecondary == NULL)
        {
            pLastSecondary = RecompCodeSecondary;
        }

        RecompPos = pLastSecondary;
        CPU_Message("   (Secondary buffer active 0x%08X)", pLastSecondary);
    }
    else
    {
        dwBuffer = MainBuffer;
        pLastSecondary = RecompPos;

        if (pLastPrimary == NULL)
        {
            pLastPrimary = RecompCode;
        }

        RecompPos = pLastPrimary;
        CPU_Message("   (Primary buffer active 0x%08X)", pLastPrimary);
    }
}

void ClearAllx86Code(void)
{
    extern uint32_t NoOfMaps, MapsCRC[32];
    extern uint8_t * JumpTables;

    memset(&MapsCRC, 0, sizeof(uint32_t) * 0x20);
    NoOfMaps = 0;
    memset(JumpTables, 0, 0x1000 * 32);

    RecompPos = RecompCode;

    pLastPrimary = NULL;
    pLastSecondary = NULL;
}

/*
Link branches
Description:
Resolves all the collected branches, x86 style
*/

void LinkBranches(RSP_BLOCK * Block)
{
    uint32_t OrigPrgCount = *PrgCount;
    uint32_t Count, Target;
    uint32_t * JumpWord;
    uint8_t * X86Code;
    RSP_BLOCK Save;

    if (!CurrentBlock.ResolveCount)
    {
        return;
    }
    CPU_Message("***** Linking branches (%i) *****", CurrentBlock.ResolveCount);

    for (Count = 0; Count < CurrentBlock.ResolveCount; Count++)
    {
        Target = CurrentBlock.BranchesToResolve[Count].TargetPC;
        X86Code = (uint8_t *)*(JumpTable + (Target >> 2));

        if (!X86Code)
        {
            *PrgCount = Target;
            CPU_Message("");
            CPU_Message("===== (Generate code: %04X) =====", Target);
            Save = *Block;

            // Compile this block and link
            CompilerRSPBlock();
            LinkBranches(Block);

            *Block = Save;
            CPU_Message("===== (End generate code: %04X) =====", Target);
            CPU_Message("");
            X86Code = (uint8_t *)*(JumpTable + (Target >> 2));
        }

        JumpWord = CurrentBlock.BranchesToResolve[Count].X86JumpLoc;
        x86_SetBranch32b(JumpWord, (uint32_t *)X86Code);

        CPU_Message("Linked RSP branch from x86: %08X, to RSP: %X / x86: %08X",
                    JumpWord, Target, X86Code);
    }
    *PrgCount = OrigPrgCount;
    CPU_Message("***** Done linking branches *****");
    CPU_Message("");
}

/*
BuildBranchLabels
Description:
Branch labels are used to start and stop re-ordering
sections as well as set the jump table to points
within a block that are safe.
*/

void BuildBranchLabels(void)
{
    RSPOpcode RspOp;
    uint32_t i, Dest;

#ifdef BUILD_BRANCHLABELS_VERBOSE
    CPU_Message("***** Building branch labels *****");
#endif

    for (i = 0; i < 0x1000; i += 4)
    {
        RspOp.Value = *(uint32_t *)(RSPInfo.IMEM + i);

        if (IsOpcodeBranch(i, RspOp))
        {
            if (RspCode.LabelCount >= (sizeof(RspCode.BranchLabels) / sizeof(RspCode.BranchLabels[0])) - 1)
            {
                CompilerWarning("Out of space for branch labels");
                return;
            }

            if (RspCode.BranchCount >= (sizeof(RspCode.BranchLocations) / sizeof(RspCode.BranchLocations[0])) - 1)
            {
                CompilerWarning("Out of space for branch locations");
                return;
            }
            RspCode.BranchLocations[RspCode.BranchCount++] = i;
            if (RspOp.op == RSP_SPECIAL)
            {
                // Register jump not predictable
            }
            else if (RspOp.op == RSP_J || RspOp.op == RSP_JAL)
            {
                // For JAL its a sub-block for returns
                Dest = (RspOp.target << 2) & 0xFFC;
                RspCode.BranchLabels[RspCode.LabelCount] = Dest;
                RspCode.LabelCount += 1;
#ifdef BUILD_BRANCHLABELS_VERBOSE
                CPU_Message("[%02i] Added branch at %X to %X", RspCode.LabelCount, i, Dest);
#endif
            }
            else
            {
                Dest = (i + ((short)RspOp.offset << 2) + 4) & 0xFFC;
                RspCode.BranchLabels[RspCode.LabelCount] = Dest;
                RspCode.LabelCount += 1;
#ifdef BUILD_BRANCHLABELS_VERBOSE
                CPU_Message("[%02i] Added branch at %X to %X", RspCode.LabelCount, i, Dest);
#endif
            }
        }
    }

#ifdef BUILD_BRANCHLABELS_VERBOSE
    CPU_Message("***** End branch labels *****");
    CPU_Message("");
#endif
}

bool IsJumpLabel(uint32_t PC)
{
    uint32_t Count;

    if (!RspCode.LabelCount)
    {
        return false;
    }

    for (Count = 0; Count < RspCode.LabelCount; Count++)
    {
        if (PC == RspCode.BranchLabels[Count])
        {
            return true;
        }
    }
    return false;
}

void CompilerLinkBlocks(void)
{
    uint8_t * KnownCode = (uint8_t *)*(JumpTable + (CompilePC >> 2));

    CPU_Message("***** Linking block to X86: %08X *****", KnownCode);
    NextInstruction = RSPPIPELINE_FINISH_BLOCK;

    // Block linking scenario
    JmpLabel32("Linked block", 0);
    x86_SetBranch32b(RecompPos - 4, KnownCode);
}

void CompilerRSPBlock(void)
{
    uint8_t * IMEM_SAVE = (uint8_t *)malloc(0x1000);
    const size_t X86BaseAddress = (size_t)RecompPos;

    NextInstruction = RSPPIPELINE_NORMAL;
    CompilePC = *PrgCount;

    memset(&CurrentBlock, 0, sizeof(CurrentBlock));
    CurrentBlock.StartPC = CompilePC;
    CurrentBlock.CurrPC = CompilePC;

    // Align the block to a boundary
    if (X86BaseAddress & 7)
    {
        register size_t Count;
        const size_t Padding = (8 - (X86BaseAddress & 7)) & 7;

        for (Count = 0; Count < Padding; Count++)
        {
            CPU_Message("%08X: nop", RecompPos);
            *(RecompPos++) = 0x90;
        }
    }

    CPU_Message("====== Block %d ======", BlockID++);
    CPU_Message("x86 code at: %X", RecompPos);
    CPU_Message("Jump table: %X", Table);
    CPU_Message("Start of block: %X", CurrentBlock.StartPC);
    CPU_Message("====== Recompiled code ======");

    if (Compiler.bReOrdering)
    {
        memcpy(IMEM_SAVE, RSPInfo.IMEM, 0x1000);
        ReOrderSubBlock(&CurrentBlock);
    }

    // This is for the block about to be compiled
    *(JumpTable + (CompilePC >> 2)) = RecompPos;

    do
    {

        // Reordering is setup to allow us to have loop labels
        // so here we see if this is one and put it in the jump table

        if (NextInstruction == RSPPIPELINE_NORMAL && IsJumpLabel(CompilePC))
        {
            // Jumps come around twice
            if (NULL == *(JumpTable + (CompilePC >> 2)))
            {
                CPU_Message("***** Adding jump table entry for PC: %04X at X86: %08X *****", CompilePC, RecompPos);
                CPU_Message("");
                *(JumpTable + (CompilePC >> 2)) = RecompPos;

                // Reorder from here to next label or branch
                CurrentBlock.CurrPC = CompilePC;
                ReOrderSubBlock(&CurrentBlock);
            }
            else if (NextInstruction != RSPPIPELINE_DELAY_SLOT_DONE)
            {

                // We could link the blocks here, but performance-wise it might be better to just let it run
            }
        }

        if (Compiler.bSections)
        {
            if (RSP_DoSections())
            {
                continue;
            }
        }

#ifdef X86_RECOMP_VERBOSE
        if (!IsOpcodeNop(CompilePC))
        {
            CPU_Message("X86 Address: %08X", RecompPos);
        }
#endif

        RSP_LW_IMEM(CompilePC, &RSPOpC.Value);

        if (LogRDP && NextInstruction != RSPPIPELINE_DELAY_SLOT_DONE)
        {
            char str[40];
            sprintf(str, "%X", CompilePC);
            PushImm32(str, CompilePC);
            Call_Direct(RDP_LogLoc, "RDP_LogLoc");
            AddConstToX86Reg(x86_ESP, 4);
        }

        if (RSPOpC.Value == 0xFFFFFFFF)
        {
            // I think this pops up an unknown OP dialog
            // NextInstruction = RSPPIPELINE_FINISH_BLOCK;
        }
        else
        {
            RSP_Opcode[RSPOpC.op]();
        }

        switch (NextInstruction)
        {
        case RSPPIPELINE_NORMAL:
            CompilePC += 4;
            break;
        case RSPPIPELINE_DO_DELAY_SLOT:
            NextInstruction = RSPPIPELINE_DELAY_SLOT;
            CompilePC += 4;
            break;
        case RSPPIPELINE_DELAY_SLOT:
            NextInstruction = RSPPIPELINE_DELAY_SLOT_DONE;
            CompilePC -= 4;
            break;
        case RSPPIPELINE_DELAY_SLOT_EXIT:
            NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT_DONE;
            CompilePC -= 4;
            break;
        case RSPPIPELINE_FINISH_SUB_BLOCK:
            NextInstruction = RSPPIPELINE_NORMAL;
            CompilePC += 8;
            if (CompilePC >= 0x1000)
            {
                NextInstruction = RSPPIPELINE_FINISH_BLOCK;
            }
            else if (NULL == *(JumpTable + (CompilePC >> 2)))
            {
                // This is for the new block being compiled now
                CPU_Message("***** Continuing static SubBlock (jump table entry added for PC: %04X at X86: %08X) *****", CompilePC, RecompPos);
                *(JumpTable + (CompilePC >> 2)) = RecompPos;

                CurrentBlock.CurrPC = CompilePC;
                // Reorder from after delay to next label or branch
                ReOrderSubBlock(&CurrentBlock);
            }
            else
            {
                CompilerLinkBlocks();
            }
            break;

        case RSPPIPELINE_FINISH_BLOCK: break;
        default:
            g_Notify->DisplayError(stdstr_f("RSP main loop\n\nWTF NextInstruction = %d", NextInstruction).c_str());
            CompilePC += 4;
            break;
        }
    } while (NextInstruction != RSPPIPELINE_FINISH_BLOCK && (CompilePC < 0x1000 || NextInstruction == RSPPIPELINE_DELAY_SLOT));
    CPU_Message("===== End of recompiled code =====");

    if (Compiler.bReOrdering)
    {
        memcpy(RSPInfo.IMEM, IMEM_SAVE, 0x1000);
    }
    free(IMEM_SAVE);
}

uint32_t RunRecompilerCPU(uint32_t Cycles)
{
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif

    uint8_t * Block;

    RSP_Running = true;
    SetJumpTable(JumpTableSize);

    while (RSP_Running)
    {
        Block = (uint8_t *)*(JumpTable + (*PrgCount >> 2));

        if (Block == NULL)
        {
            if (Profiling && !IndvidualBlock)
            {
                StartTimer((uint32_t)Timer_Compiling);
            }

            memset(&RspCode, 0, sizeof(RspCode));
#if defined(_MSC_VER)
            __try
            {
                BuildBranchLabels();
                DetectGPRConstants(&RspCode);
                CompilerRSPBlock();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                char ErrorMessage[400];
                sprintf(ErrorMessage, "Error CompilePC = %08X", CompilePC);
                g_Notify->DisplayError(ErrorMessage);
                ClearAllx86Code();
                continue;
            }
#else
            BuildBranchLabels();
            DetectGPRConstants(&RspCode);
            CompilerRSPBlock();
#endif

            Block = (uint8_t *)*(JumpTable + (*PrgCount >> 2));

            // We are done compiling, but we may have references
            // to fill in still either from this block, or jumps
            // that go out of it, let's rock

            LinkBranches(&CurrentBlock);
            if (Profiling && !IndvidualBlock)
            {
                StopTimer();
            }
        }

        if (Profiling && IndvidualBlock)
        {
            StartTimer(*PrgCount);
        }

#if defined(_M_IX86) && defined(_MSC_VER)
        _asm {
			pushad
			call Block
			popad
        }
#else
        __debugbreak();
#endif
        if (Profiling && IndvidualBlock)
        {
            StopTimer();
        }
        if (RSP_NextInstruction == RSPPIPELINE_SINGLE_STEP)
        {
            RSP_Running = false;
        }
    }

    if (IsMmxEnabled)
    {
#if defined(_M_IX86) && defined(_MSC_VER)
        _asm emms
#else
        __debugbreak();
#endif
    }
    return Cycles;
}
