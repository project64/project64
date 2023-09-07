#include "RSPInterpreterCPU.h"
#include "RSPCpu.h"
#include "RSPInterpreterOps.h"
#include "RSPRegisters.h"
#include <Project64-rsp-core\RSPDebugger.h>
#include <Project64-rsp-core\RSPInfo.h>

RSPPIPELINE_STAGE RSP_NextInstruction;
uint32_t RSP_JumpTo;

void BuildInterpreterCPU(void)
{
    RSP_Opcode[0] = RSP_Opcode_SPECIAL;
    RSP_Opcode[1] = RSP_Opcode_REGIMM;
    RSP_Opcode[2] = RSP_Opcode_J;
    RSP_Opcode[3] = RSP_Opcode_JAL;
    RSP_Opcode[4] = RSP_Opcode_BEQ;
    RSP_Opcode[5] = RSP_Opcode_BNE;
    RSP_Opcode[6] = RSP_Opcode_BLEZ;
    RSP_Opcode[7] = RSP_Opcode_BGTZ;
    RSP_Opcode[8] = RSP_Opcode_ADDI;
    RSP_Opcode[9] = RSP_Opcode_ADDIU;
    RSP_Opcode[10] = RSP_Opcode_SLTI;
    RSP_Opcode[11] = RSP_Opcode_SLTIU;
    RSP_Opcode[12] = RSP_Opcode_ANDI;
    RSP_Opcode[13] = RSP_Opcode_ORI;
    RSP_Opcode[14] = RSP_Opcode_XORI;
    RSP_Opcode[15] = RSP_Opcode_LUI;
    RSP_Opcode[16] = RSP_Opcode_COP0;
    RSP_Opcode[17] = rsp_UnknownOpcode;
    RSP_Opcode[18] = RSP_Opcode_COP2;
    RSP_Opcode[19] = rsp_UnknownOpcode;
    RSP_Opcode[20] = rsp_UnknownOpcode;
    RSP_Opcode[21] = rsp_UnknownOpcode;
    RSP_Opcode[22] = rsp_UnknownOpcode;
    RSP_Opcode[23] = rsp_UnknownOpcode;
    RSP_Opcode[24] = rsp_UnknownOpcode;
    RSP_Opcode[25] = rsp_UnknownOpcode;
    RSP_Opcode[26] = rsp_UnknownOpcode;
    RSP_Opcode[27] = rsp_UnknownOpcode;
    RSP_Opcode[28] = rsp_UnknownOpcode;
    RSP_Opcode[29] = rsp_UnknownOpcode;
    RSP_Opcode[30] = rsp_UnknownOpcode;
    RSP_Opcode[31] = rsp_UnknownOpcode;
    RSP_Opcode[32] = RSP_Opcode_LB;
    RSP_Opcode[33] = RSP_Opcode_LH;
    RSP_Opcode[34] = rsp_UnknownOpcode;
    RSP_Opcode[35] = RSP_Opcode_LW;
    RSP_Opcode[36] = RSP_Opcode_LBU;
    RSP_Opcode[37] = RSP_Opcode_LHU;
    RSP_Opcode[38] = rsp_UnknownOpcode;
    RSP_Opcode[39] = RSP_Opcode_LWU;
    RSP_Opcode[40] = RSP_Opcode_SB;
    RSP_Opcode[41] = RSP_Opcode_SH;
    RSP_Opcode[42] = rsp_UnknownOpcode;
    RSP_Opcode[43] = RSP_Opcode_SW;
    RSP_Opcode[44] = rsp_UnknownOpcode;
    RSP_Opcode[45] = rsp_UnknownOpcode;
    RSP_Opcode[46] = rsp_UnknownOpcode;
    RSP_Opcode[47] = rsp_UnknownOpcode;
    RSP_Opcode[48] = rsp_UnknownOpcode;
    RSP_Opcode[49] = rsp_UnknownOpcode;
    RSP_Opcode[50] = RSP_Opcode_LC2;
    RSP_Opcode[51] = rsp_UnknownOpcode;
    RSP_Opcode[52] = rsp_UnknownOpcode;
    RSP_Opcode[53] = rsp_UnknownOpcode;
    RSP_Opcode[54] = rsp_UnknownOpcode;
    RSP_Opcode[55] = rsp_UnknownOpcode;
    RSP_Opcode[56] = rsp_UnknownOpcode;
    RSP_Opcode[57] = rsp_UnknownOpcode;
    RSP_Opcode[58] = RSP_Opcode_SC2;
    RSP_Opcode[59] = rsp_UnknownOpcode;
    RSP_Opcode[60] = rsp_UnknownOpcode;
    RSP_Opcode[61] = rsp_UnknownOpcode;
    RSP_Opcode[62] = rsp_UnknownOpcode;
    RSP_Opcode[63] = rsp_UnknownOpcode;

    RSP_Special[0] = RSP_Special_SLL;
    RSP_Special[1] = rsp_UnknownOpcode;
    RSP_Special[2] = RSP_Special_SRL;
    RSP_Special[3] = RSP_Special_SRA;
    RSP_Special[4] = RSP_Special_SLLV;
    RSP_Special[5] = rsp_UnknownOpcode;
    RSP_Special[6] = RSP_Special_SRLV;
    RSP_Special[7] = RSP_Special_SRAV;
    RSP_Special[8] = RSP_Special_JR;
    RSP_Special[9] = RSP_Special_JALR;
    RSP_Special[10] = rsp_UnknownOpcode;
    RSP_Special[11] = rsp_UnknownOpcode;
    RSP_Special[12] = rsp_UnknownOpcode;
    RSP_Special[13] = RSP_Special_BREAK;
    RSP_Special[14] = rsp_UnknownOpcode;
    RSP_Special[15] = rsp_UnknownOpcode;
    RSP_Special[16] = rsp_UnknownOpcode;
    RSP_Special[17] = rsp_UnknownOpcode;
    RSP_Special[18] = rsp_UnknownOpcode;
    RSP_Special[19] = rsp_UnknownOpcode;
    RSP_Special[20] = rsp_UnknownOpcode;
    RSP_Special[21] = rsp_UnknownOpcode;
    RSP_Special[22] = rsp_UnknownOpcode;
    RSP_Special[23] = rsp_UnknownOpcode;
    RSP_Special[24] = rsp_UnknownOpcode;
    RSP_Special[25] = rsp_UnknownOpcode;
    RSP_Special[26] = rsp_UnknownOpcode;
    RSP_Special[27] = rsp_UnknownOpcode;
    RSP_Special[28] = rsp_UnknownOpcode;
    RSP_Special[29] = rsp_UnknownOpcode;
    RSP_Special[30] = rsp_UnknownOpcode;
    RSP_Special[31] = rsp_UnknownOpcode;
    RSP_Special[32] = RSP_Special_ADD;
    RSP_Special[33] = RSP_Special_ADDU;
    RSP_Special[34] = RSP_Special_SUB;
    RSP_Special[35] = RSP_Special_SUBU;
    RSP_Special[36] = RSP_Special_AND;
    RSP_Special[37] = RSP_Special_OR;
    RSP_Special[38] = RSP_Special_XOR;
    RSP_Special[39] = RSP_Special_NOR;
    RSP_Special[40] = rsp_UnknownOpcode;
    RSP_Special[41] = rsp_UnknownOpcode;
    RSP_Special[42] = RSP_Special_SLT;
    RSP_Special[43] = RSP_Special_SLTU;
    RSP_Special[44] = rsp_UnknownOpcode;
    RSP_Special[45] = rsp_UnknownOpcode;
    RSP_Special[46] = rsp_UnknownOpcode;
    RSP_Special[47] = rsp_UnknownOpcode;
    RSP_Special[48] = rsp_UnknownOpcode;
    RSP_Special[49] = rsp_UnknownOpcode;
    RSP_Special[50] = rsp_UnknownOpcode;
    RSP_Special[51] = rsp_UnknownOpcode;
    RSP_Special[52] = rsp_UnknownOpcode;
    RSP_Special[53] = rsp_UnknownOpcode;
    RSP_Special[54] = rsp_UnknownOpcode;
    RSP_Special[55] = rsp_UnknownOpcode;
    RSP_Special[56] = rsp_UnknownOpcode;
    RSP_Special[57] = rsp_UnknownOpcode;
    RSP_Special[58] = rsp_UnknownOpcode;
    RSP_Special[59] = rsp_UnknownOpcode;
    RSP_Special[60] = rsp_UnknownOpcode;
    RSP_Special[61] = rsp_UnknownOpcode;
    RSP_Special[62] = rsp_UnknownOpcode;
    RSP_Special[63] = rsp_UnknownOpcode;

    RSP_RegImm[0] = RSP_Opcode_BLTZ;
    RSP_RegImm[1] = RSP_Opcode_BGEZ;
    RSP_RegImm[2] = rsp_UnknownOpcode;
    RSP_RegImm[3] = rsp_UnknownOpcode;
    RSP_RegImm[4] = rsp_UnknownOpcode;
    RSP_RegImm[5] = rsp_UnknownOpcode;
    RSP_RegImm[6] = rsp_UnknownOpcode;
    RSP_RegImm[7] = rsp_UnknownOpcode;
    RSP_RegImm[8] = rsp_UnknownOpcode;
    RSP_RegImm[9] = rsp_UnknownOpcode;
    RSP_RegImm[10] = rsp_UnknownOpcode;
    RSP_RegImm[11] = rsp_UnknownOpcode;
    RSP_RegImm[12] = rsp_UnknownOpcode;
    RSP_RegImm[13] = rsp_UnknownOpcode;
    RSP_RegImm[14] = rsp_UnknownOpcode;
    RSP_RegImm[15] = rsp_UnknownOpcode;
    RSP_RegImm[16] = RSP_Opcode_BLTZAL;
    RSP_RegImm[17] = RSP_Opcode_BGEZAL;
    RSP_RegImm[18] = rsp_UnknownOpcode;
    RSP_RegImm[19] = rsp_UnknownOpcode;
    RSP_RegImm[20] = rsp_UnknownOpcode;
    RSP_RegImm[21] = rsp_UnknownOpcode;
    RSP_RegImm[22] = rsp_UnknownOpcode;
    RSP_RegImm[23] = rsp_UnknownOpcode;
    RSP_RegImm[24] = rsp_UnknownOpcode;
    RSP_RegImm[25] = rsp_UnknownOpcode;
    RSP_RegImm[26] = rsp_UnknownOpcode;
    RSP_RegImm[27] = rsp_UnknownOpcode;
    RSP_RegImm[28] = rsp_UnknownOpcode;
    RSP_RegImm[29] = rsp_UnknownOpcode;
    RSP_RegImm[30] = rsp_UnknownOpcode;
    RSP_RegImm[31] = rsp_UnknownOpcode;

    RSP_Cop0[0] = RSP_Cop0_MF;
    RSP_Cop0[1] = rsp_UnknownOpcode;
    RSP_Cop0[2] = rsp_UnknownOpcode;
    RSP_Cop0[3] = rsp_UnknownOpcode;
    RSP_Cop0[4] = RSP_Cop0_MT;
    RSP_Cop0[5] = rsp_UnknownOpcode;
    RSP_Cop0[6] = rsp_UnknownOpcode;
    RSP_Cop0[7] = rsp_UnknownOpcode;
    RSP_Cop0[8] = rsp_UnknownOpcode;
    RSP_Cop0[9] = rsp_UnknownOpcode;
    RSP_Cop0[10] = rsp_UnknownOpcode;
    RSP_Cop0[11] = rsp_UnknownOpcode;
    RSP_Cop0[12] = rsp_UnknownOpcode;
    RSP_Cop0[13] = rsp_UnknownOpcode;
    RSP_Cop0[14] = rsp_UnknownOpcode;
    RSP_Cop0[15] = rsp_UnknownOpcode;
    RSP_Cop0[16] = rsp_UnknownOpcode;
    RSP_Cop0[17] = rsp_UnknownOpcode;
    RSP_Cop0[18] = rsp_UnknownOpcode;
    RSP_Cop0[19] = rsp_UnknownOpcode;
    RSP_Cop0[20] = rsp_UnknownOpcode;
    RSP_Cop0[21] = rsp_UnknownOpcode;
    RSP_Cop0[22] = rsp_UnknownOpcode;
    RSP_Cop0[23] = rsp_UnknownOpcode;
    RSP_Cop0[24] = rsp_UnknownOpcode;
    RSP_Cop0[25] = rsp_UnknownOpcode;
    RSP_Cop0[26] = rsp_UnknownOpcode;
    RSP_Cop0[27] = rsp_UnknownOpcode;
    RSP_Cop0[28] = rsp_UnknownOpcode;
    RSP_Cop0[29] = rsp_UnknownOpcode;
    RSP_Cop0[30] = rsp_UnknownOpcode;
    RSP_Cop0[31] = rsp_UnknownOpcode;

    RSP_Cop2[0] = RSP_Cop2_MF;
    RSP_Cop2[1] = rsp_UnknownOpcode;
    RSP_Cop2[2] = RSP_Cop2_CF;
    RSP_Cop2[3] = rsp_UnknownOpcode;
    RSP_Cop2[4] = RSP_Cop2_MT;
    RSP_Cop2[5] = rsp_UnknownOpcode;
    RSP_Cop2[6] = RSP_Cop2_CT;
    RSP_Cop2[7] = rsp_UnknownOpcode;
    RSP_Cop2[8] = rsp_UnknownOpcode;
    RSP_Cop2[9] = rsp_UnknownOpcode;
    RSP_Cop2[10] = rsp_UnknownOpcode;
    RSP_Cop2[11] = rsp_UnknownOpcode;
    RSP_Cop2[12] = rsp_UnknownOpcode;
    RSP_Cop2[13] = rsp_UnknownOpcode;
    RSP_Cop2[14] = rsp_UnknownOpcode;
    RSP_Cop2[15] = rsp_UnknownOpcode;
    RSP_Cop2[16] = RSP_COP2_VECTOR;
    RSP_Cop2[17] = RSP_COP2_VECTOR;
    RSP_Cop2[18] = RSP_COP2_VECTOR;
    RSP_Cop2[19] = RSP_COP2_VECTOR;
    RSP_Cop2[20] = RSP_COP2_VECTOR;
    RSP_Cop2[21] = RSP_COP2_VECTOR;
    RSP_Cop2[22] = RSP_COP2_VECTOR;
    RSP_Cop2[23] = RSP_COP2_VECTOR;
    RSP_Cop2[24] = RSP_COP2_VECTOR;
    RSP_Cop2[25] = RSP_COP2_VECTOR;
    RSP_Cop2[26] = RSP_COP2_VECTOR;
    RSP_Cop2[27] = RSP_COP2_VECTOR;
    RSP_Cop2[28] = RSP_COP2_VECTOR;
    RSP_Cop2[29] = RSP_COP2_VECTOR;
    RSP_Cop2[30] = RSP_COP2_VECTOR;
    RSP_Cop2[31] = RSP_COP2_VECTOR;

    RSP_Vector[0] = RSP_Vector_VMULF;
    RSP_Vector[1] = RSP_Vector_VMULU;
    RSP_Vector[2] = RSP_Vector_VRNDP;
    RSP_Vector[3] = RSP_Vector_VMULQ;
    RSP_Vector[4] = RSP_Vector_VMUDL;
    RSP_Vector[5] = RSP_Vector_VMUDM;
    RSP_Vector[6] = RSP_Vector_VMUDN;
    RSP_Vector[7] = RSP_Vector_VMUDH;
    RSP_Vector[8] = RSP_Vector_VMACF;
    RSP_Vector[9] = RSP_Vector_VMACU;
    RSP_Vector[10] = RSP_Vector_VRNDN;
    RSP_Vector[11] = RSP_Vector_VMACQ;
    RSP_Vector[12] = RSP_Vector_VMADL;
    RSP_Vector[13] = RSP_Vector_VMADM;
    RSP_Vector[14] = RSP_Vector_VMADN;
    RSP_Vector[15] = RSP_Vector_VMADH;
    RSP_Vector[16] = RSP_Vector_VADD;
    RSP_Vector[17] = RSP_Vector_VSUB;
    RSP_Vector[18] = RSP_Vector_Reserved;
    RSP_Vector[19] = RSP_Vector_VABS;
    RSP_Vector[20] = RSP_Vector_VADDC;
    RSP_Vector[21] = RSP_Vector_VSUBC;
    RSP_Vector[22] = RSP_Vector_Reserved;
    RSP_Vector[23] = RSP_Vector_Reserved;
    RSP_Vector[24] = RSP_Vector_Reserved;
    RSP_Vector[25] = RSP_Vector_Reserved;
    RSP_Vector[26] = RSP_Vector_Reserved;
    RSP_Vector[27] = RSP_Vector_Reserved;
    RSP_Vector[28] = RSP_Vector_Reserved;
    RSP_Vector[29] = RSP_Vector_VSAW;
    RSP_Vector[30] = RSP_Vector_Reserved;
    RSP_Vector[31] = RSP_Vector_Reserved;
    RSP_Vector[32] = RSP_Vector_VLT;
    RSP_Vector[33] = RSP_Vector_VEQ;
    RSP_Vector[34] = RSP_Vector_VNE;
    RSP_Vector[35] = RSP_Vector_VGE;
    RSP_Vector[36] = RSP_Vector_VCL;
    RSP_Vector[37] = RSP_Vector_VCH;
    RSP_Vector[38] = RSP_Vector_VCR;
    RSP_Vector[39] = RSP_Vector_VMRG;
    RSP_Vector[40] = RSP_Vector_VAND;
    RSP_Vector[41] = RSP_Vector_VNAND;
    RSP_Vector[42] = RSP_Vector_VOR;
    RSP_Vector[43] = RSP_Vector_VNOR;
    RSP_Vector[44] = RSP_Vector_VXOR;
    RSP_Vector[45] = RSP_Vector_VNXOR;
    RSP_Vector[46] = RSP_Vector_Reserved;
    RSP_Vector[47] = RSP_Vector_Reserved;
    RSP_Vector[48] = RSP_Vector_VRCP;
    RSP_Vector[49] = RSP_Vector_VRCPL;
    RSP_Vector[50] = RSP_Vector_VRCPH;
    RSP_Vector[51] = RSP_Vector_VMOV;
    RSP_Vector[52] = RSP_Vector_VRSQ;
    RSP_Vector[53] = RSP_Vector_VRSQL;
    RSP_Vector[54] = RSP_Vector_VRSQH;
    RSP_Vector[55] = RSP_Vector_VNOOP;
    RSP_Vector[56] = RSP_Vector_Reserved;
    RSP_Vector[57] = RSP_Vector_Reserved;
    RSP_Vector[58] = RSP_Vector_Reserved;
    RSP_Vector[59] = RSP_Vector_Reserved;
    RSP_Vector[60] = RSP_Vector_Reserved;
    RSP_Vector[61] = RSP_Vector_Reserved;
    RSP_Vector[62] = RSP_Vector_Reserved;
    RSP_Vector[63] = RSP_Vector_VNOOP;

    RSP_Lc2[0] = RSP_Opcode_LBV;
    RSP_Lc2[1] = RSP_Opcode_LSV;
    RSP_Lc2[2] = RSP_Opcode_LLV;
    RSP_Lc2[3] = RSP_Opcode_LDV;
    RSP_Lc2[4] = RSP_Opcode_LQV;
    RSP_Lc2[5] = RSP_Opcode_LRV;
    RSP_Lc2[6] = RSP_Opcode_LPV;
    RSP_Lc2[7] = RSP_Opcode_LUV;
    RSP_Lc2[8] = RSP_Opcode_LHV;
    RSP_Lc2[9] = RSP_Opcode_LFV;
    RSP_Lc2[10] = RSP_Opcode_LWV;
    RSP_Lc2[11] = RSP_Opcode_LTV;
    RSP_Lc2[12] = rsp_UnknownOpcode;
    RSP_Lc2[13] = rsp_UnknownOpcode;
    RSP_Lc2[14] = rsp_UnknownOpcode;
    RSP_Lc2[15] = rsp_UnknownOpcode;
    RSP_Lc2[16] = rsp_UnknownOpcode;
    RSP_Lc2[17] = rsp_UnknownOpcode;
    RSP_Lc2[18] = rsp_UnknownOpcode;
    RSP_Lc2[19] = rsp_UnknownOpcode;
    RSP_Lc2[20] = rsp_UnknownOpcode;
    RSP_Lc2[21] = rsp_UnknownOpcode;
    RSP_Lc2[22] = rsp_UnknownOpcode;
    RSP_Lc2[23] = rsp_UnknownOpcode;
    RSP_Lc2[24] = rsp_UnknownOpcode;
    RSP_Lc2[25] = rsp_UnknownOpcode;
    RSP_Lc2[26] = rsp_UnknownOpcode;
    RSP_Lc2[27] = rsp_UnknownOpcode;
    RSP_Lc2[28] = rsp_UnknownOpcode;
    RSP_Lc2[29] = rsp_UnknownOpcode;
    RSP_Lc2[30] = rsp_UnknownOpcode;
    RSP_Lc2[31] = rsp_UnknownOpcode;

    RSP_Sc2[0] = RSP_Opcode_SBV;
    RSP_Sc2[1] = RSP_Opcode_SSV;
    RSP_Sc2[2] = RSP_Opcode_SLV;
    RSP_Sc2[3] = RSP_Opcode_SDV;
    RSP_Sc2[4] = RSP_Opcode_SQV;
    RSP_Sc2[5] = RSP_Opcode_SRV;
    RSP_Sc2[6] = RSP_Opcode_SPV;
    RSP_Sc2[7] = RSP_Opcode_SUV;
    RSP_Sc2[8] = RSP_Opcode_SHV;
    RSP_Sc2[9] = RSP_Opcode_SFV;
    RSP_Sc2[10] = RSP_Opcode_SWV;
    RSP_Sc2[11] = RSP_Opcode_STV;
    RSP_Sc2[12] = rsp_UnknownOpcode;
    RSP_Sc2[13] = rsp_UnknownOpcode;
    RSP_Sc2[14] = rsp_UnknownOpcode;
    RSP_Sc2[15] = rsp_UnknownOpcode;
    RSP_Sc2[16] = rsp_UnknownOpcode;
    RSP_Sc2[17] = rsp_UnknownOpcode;
    RSP_Sc2[18] = rsp_UnknownOpcode;
    RSP_Sc2[19] = rsp_UnknownOpcode;
    RSP_Sc2[20] = rsp_UnknownOpcode;
    RSP_Sc2[21] = rsp_UnknownOpcode;
    RSP_Sc2[22] = rsp_UnknownOpcode;
    RSP_Sc2[23] = rsp_UnknownOpcode;
    RSP_Sc2[24] = rsp_UnknownOpcode;
    RSP_Sc2[25] = rsp_UnknownOpcode;
    RSP_Sc2[26] = rsp_UnknownOpcode;
    RSP_Sc2[27] = rsp_UnknownOpcode;
    RSP_Sc2[28] = rsp_UnknownOpcode;
    RSP_Sc2[29] = rsp_UnknownOpcode;
    RSP_Sc2[30] = rsp_UnknownOpcode;
    RSP_Sc2[31] = rsp_UnknownOpcode;
}

uint32_t RunInterpreterCPU(uint32_t Cycles)
{
    uint32_t CycleCount;
    RSP_Running = true;
    g_RSPDebugger->StartingCPU();
    CycleCount = 0;

    while (RSP_Running)
    {
        g_RSPDebugger->BeforeExecuteOp();

        RSPOpC.Value = *(uint32_t *)(RSPInfo.IMEM + (*PrgCount & 0xFFC));
        RSP_Opcode[RSPOpC.op]();
        RSP_GPR[0].W = 0x00000000; // MIPS $zero hard-wired to 0

        switch (RSP_NextInstruction)
        {
        case RSPPIPELINE_NORMAL:
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            break;
        case RSPPIPELINE_DELAY_SLOT:
            RSP_NextInstruction = RSPPIPELINE_JUMP;
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            break;
        case RSPPIPELINE_JUMP:
            RSP_NextInstruction = RSPPIPELINE_NORMAL;
            *PrgCount = RSP_JumpTo;
            break;
        case RSPPIPELINE_SINGLE_STEP:
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            RSP_NextInstruction = RSPPIPELINE_SINGLE_STEP_DONE;
            break;
        case RSPPIPELINE_SINGLE_STEP_DONE:
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_HALT;
            RSP_Running = false;
            break;
        }
    }
    return Cycles;
}

unsigned int RSP_branch_if(int condition)
{
    unsigned int new_PC;

    if (condition)
    {
        new_PC = *PrgCount + 4 + ((short)RSPOpC.offset << 2);
    }
    else
    {
        new_PC = *PrgCount + 4 + 4;
    }
    return (new_PC & 0xFFC);
}
