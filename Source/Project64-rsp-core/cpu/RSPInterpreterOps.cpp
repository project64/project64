#include "RSPCpu.h"
#include "RSPInterpreterCPU.h"
#include "RSPRegisters.h"
#include "RspLog.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/RSPDebugger.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RspClamp.h>
#include <Project64-rsp-core/cpu/RspDma.h>
#include <Settings/Settings.h>
#include <algorithm>
#include <float.h>
#include <math.h>

extern UWORD32 Recp, RecpResult, SQroot, SQrootResult;
extern bool AudioHle, GraphicsHle;

// Opcode functions

void RSP_Opcode_SPECIAL(void)
{
    RSP_Special[RSPOpC.funct]();
}

void RSP_Opcode_REGIMM(void)
{
    RSP_RegImm[RSPOpC.rt]();
}

void RSP_Opcode_J(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = (RSPOpC.target << 2) & 0xFFC;
}

void RSP_Opcode_JAL(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_GPR[31].UW = (*PrgCount + 8) & 0xFFC;
    RSP_JumpTo = (RSPOpC.target << 2) & 0xFFC;
}

void RSP_Opcode_BEQ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W == RSP_GPR[RSPOpC.rt].W);
}

void RSP_Opcode_BNE(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W != RSP_GPR[RSPOpC.rt].W);
}

void RSP_Opcode_BLEZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W <= 0);
}

void RSP_Opcode_BGTZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W > 0);
}

void RSP_Opcode_ADDI(void)
{
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rs].W + (int16_t)RSPOpC.immediate;
}

void RSP_Opcode_ADDIU(void)
{
    RSP_GPR[RSPOpC.rt].UW = RSP_GPR[RSPOpC.rs].UW + (uint32_t)((int16_t)RSPOpC.immediate);
}

void RSP_Opcode_SLTI(void)
{
    RSP_GPR[RSPOpC.rt].W = (RSP_GPR[RSPOpC.rs].W < (int16_t)RSPOpC.immediate) ? 1 : 0;
}

void RSP_Opcode_SLTIU(void)
{
    RSP_GPR[RSPOpC.rt].W = (RSP_GPR[RSPOpC.rs].UW < (uint32_t)(int16_t)RSPOpC.immediate) ? 1 : 0;
}

void RSP_Opcode_ANDI(void)
{
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rs].W & RSPOpC.immediate;
}

void RSP_Opcode_ORI(void)
{
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rs].W | RSPOpC.immediate;
}

void RSP_Opcode_XORI(void)
{
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rs].W ^ RSPOpC.immediate;
}

void RSP_Opcode_LUI(void)
{
    RSP_GPR[RSPOpC.rt].W = RSPOpC.immediate << 16;
}

void RSP_Opcode_COP0(void)
{
    RSP_Cop0[RSPOpC.rs]();
}

void RSP_Opcode_COP2(void)
{
    RSP_Cop2[RSPOpC.rs]();
}

void RSP_Opcode_LB(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_GPR[RSPOpC.rt].W = *(int8_t *)(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF));
}

void RSP_Opcode_LH(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    if ((Address & 0x1) != 0)
    {
        RSP_GPR[RSPOpC.rt].UHW[0] = *(uint8_t *)(RSPInfo.DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 8;
        RSP_GPR[RSPOpC.rt].UHW[0] += *(uint8_t *)(RSPInfo.DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        RSP_GPR[RSPOpC.rt].UHW[0] = *(uint16_t *)(RSPInfo.DMEM + ((Address ^ 2) & 0xFFF));
    }
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rt].HW[0];
}

void RSP_Opcode_LW(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    if ((Address & 0x3) != 0)
    {
        RSP_GPR[RSPOpC.rt].UW = *(uint8_t *)(RSPInfo.DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 24;
        RSP_GPR[RSPOpC.rt].UW += *(uint8_t *)(RSPInfo.DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 16;
        RSP_GPR[RSPOpC.rt].UW += *(uint8_t *)(RSPInfo.DMEM + (((Address + 2) & 0xFFF) ^ 3)) << 8;
        RSP_GPR[RSPOpC.rt].UW += *(uint8_t *)(RSPInfo.DMEM + (((Address + 3) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        RSP_GPR[RSPOpC.rt].UW = *(uint32_t *)(RSPInfo.DMEM + (Address & 0xFFF));
    }
}

void RSP_Opcode_LBU(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_GPR[RSPOpC.rt].UW = *(uint8_t *)(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF));
}

void RSP_Opcode_LHU(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    if ((Address & 0x1) != 0)
    {
        RSP_GPR[RSPOpC.rt].UHW[0] = *(uint8_t *)(RSPInfo.DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 8;
        RSP_GPR[RSPOpC.rt].UHW[0] += *(uint8_t *)(RSPInfo.DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        RSP_GPR[RSPOpC.rt].UHW[0] = *(uint16_t *)(RSPInfo.DMEM + ((Address ^ 2) & 0xFFF));
    }
    RSP_GPR[RSPOpC.rt].UW = RSP_GPR[RSPOpC.rt].UHW[0];
}

void RSP_Opcode_LWU(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    if ((Address & 0x3) != 0)
    {
        RSP_GPR[RSPOpC.rt].UW = *(uint8_t *)(RSPInfo.DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 24;
        RSP_GPR[RSPOpC.rt].UW += *(uint8_t *)(RSPInfo.DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 16;
        RSP_GPR[RSPOpC.rt].UW += *(uint8_t *)(RSPInfo.DMEM + (((Address + 2) & 0xFFF) ^ 3)) << 8;
        RSP_GPR[RSPOpC.rt].UW += *(uint8_t *)(RSPInfo.DMEM + (((Address + 3) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        RSP_GPR[RSPOpC.rt].UW = *(uint32_t *)(RSPInfo.DMEM + (Address & 0xFFF));
    }
}

void RSP_Opcode_SB(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    *(uint8_t *)(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_GPR[RSPOpC.rt].UB[0];
}

void RSP_Opcode_SH(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    if ((Address & 0x1) != 0)
    {
        *(uint8_t *)(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = (RSP_GPR[RSPOpC.rt].UHW[0] >> 8);
        *(uint8_t *)(RSPInfo.DMEM + (((Address + 1) ^ 3) & 0xFFF)) = (RSP_GPR[RSPOpC.rt].UHW[0] & 0xFF);
    }
    else
    {
        *(uint16_t *)(RSPInfo.DMEM + ((Address ^ 2) & 0xFFF)) = RSP_GPR[RSPOpC.rt].UHW[0];
    }
}

void RSP_Opcode_SW(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    if ((Address & 0x3) != 0)
    {
        *(uint8_t *)(RSPInfo.DMEM + (((Address + 0) ^ 3) & 0xFFF)) = (RSP_GPR[RSPOpC.rt].UW >> 24) & 0xFF;
        *(uint8_t *)(RSPInfo.DMEM + (((Address + 1) ^ 3) & 0xFFF)) = (RSP_GPR[RSPOpC.rt].UW >> 16) & 0xFF;
        *(uint8_t *)(RSPInfo.DMEM + (((Address + 2) ^ 3) & 0xFFF)) = (RSP_GPR[RSPOpC.rt].UW >> 8) & 0xFF;
        *(uint8_t *)(RSPInfo.DMEM + (((Address + 3) ^ 3) & 0xFFF)) = (RSP_GPR[RSPOpC.rt].UW >> 0) & 0xFF;
    }
    else
    {
        *(uint32_t *)(RSPInfo.DMEM + (Address & 0xFFF)) = RSP_GPR[RSPOpC.rt].UW;
    }
}

void RSP_Opcode_LC2(void)
{
    RSP_Lc2[RSPOpC.rd]();
}

void RSP_Opcode_SC2(void)
{
    RSP_Sc2[RSPOpC.rd]();
}

// R4300i Opcodes: Special

void RSP_Special_SLL(void)
{
    RSP_GPR[RSPOpC.rd].W = RSP_GPR[RSPOpC.rt].W << RSPOpC.sa;
}

void RSP_Special_SRL(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rt].UW >> RSPOpC.sa;
}

void RSP_Special_SRA(void)
{
    RSP_GPR[RSPOpC.rd].W = RSP_GPR[RSPOpC.rt].W >> RSPOpC.sa;
}

void RSP_Special_SLLV(void)
{
    RSP_GPR[RSPOpC.rd].W = RSP_GPR[RSPOpC.rt].W << (RSP_GPR[RSPOpC.rs].W & 0x1F);
}

void RSP_Special_SRLV(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rt].UW >> (RSP_GPR[RSPOpC.rs].W & 0x1F);
}

void RSP_Special_SRAV(void)
{
    RSP_GPR[RSPOpC.rd].W = RSP_GPR[RSPOpC.rt].W >> (RSP_GPR[RSPOpC.rs].W & 0x1F);
}

void RSP_Special_JR(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = (RSP_GPR[RSPOpC.rs].W & 0xFFC);
}

void RSP_Special_JALR(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = (RSP_GPR[RSPOpC.rs].W & 0xFFC);
    RSP_GPR[RSPOpC.rd].W = (*PrgCount + 8) & 0xFFC;
}

void RSP_Special_BREAK(void)
{
    RSP_Running = false;
    *RSPInfo.SP_STATUS_REG |= (SP_STATUS_HALT | SP_STATUS_BROKE);
    if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
    {
        *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
        RSPInfo.CheckInterrupts();
    }
}

void RSP_Special_ADD(void)
{
    RSP_GPR[RSPOpC.rd].W = RSP_GPR[RSPOpC.rs].W + RSP_GPR[RSPOpC.rt].W;
}

void RSP_Special_ADDU(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rs].UW + RSP_GPR[RSPOpC.rt].UW;
}

void RSP_Special_SUB(void)
{
    RSP_GPR[RSPOpC.rd].W = RSP_GPR[RSPOpC.rs].W - RSP_GPR[RSPOpC.rt].W;
}

void RSP_Special_SUBU(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rs].UW - RSP_GPR[RSPOpC.rt].UW;
}

void RSP_Special_AND(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rs].UW & RSP_GPR[RSPOpC.rt].UW;
}

void RSP_Special_OR(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rs].UW | RSP_GPR[RSPOpC.rt].UW;
}

void RSP_Special_XOR(void)
{
    RSP_GPR[RSPOpC.rd].UW = RSP_GPR[RSPOpC.rs].UW ^ RSP_GPR[RSPOpC.rt].UW;
}

void RSP_Special_NOR(void)
{
    RSP_GPR[RSPOpC.rd].UW = ~(RSP_GPR[RSPOpC.rs].UW | RSP_GPR[RSPOpC.rt].UW);
}

void RSP_Special_SLT(void)
{
    RSP_GPR[RSPOpC.rd].UW = (RSP_GPR[RSPOpC.rs].W < RSP_GPR[RSPOpC.rt].W) ? 1 : 0;
}

void RSP_Special_SLTU(void)
{
    RSP_GPR[RSPOpC.rd].UW = (RSP_GPR[RSPOpC.rs].UW < RSP_GPR[RSPOpC.rt].UW) ? 1 : 0;
}

// R4300i Opcodes: RegImm

void RSP_Opcode_BLTZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W < 0);
}

void RSP_Opcode_BGEZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W >= 0);
}

void RSP_Opcode_BLTZAL(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W < 0);
    RSP_GPR[31].UW = (*PrgCount + 8) & 0xFFC;
}

void RSP_Opcode_BGEZAL(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W >= 0);
    RSP_GPR[31].UW = (*PrgCount + 8) & 0xFFC;
}

// COP0 functions

void RSP_Cop0_MF(void)
{
    g_RSPDebugger->RDP_LogMF0(*PrgCount, RSPOpC.rd);
    switch (RSPOpC.rd)
    {
    case 0: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.SP_MEM_ADDR_REG; break;
    case 1: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.SP_DRAM_ADDR_REG; break;
    case 4:
        RSP_MfStatusCount += 1;
        RSP_GPR[RSPOpC.rt].UW = *RSPInfo.SP_STATUS_REG;
        if (Mfc0Count != 0 && RSP_MfStatusCount > Mfc0Count)
        {
            RSP_Running = false;
        }
        break;
    case 5: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.SP_DMA_FULL_REG; break;
    case 6: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.SP_DMA_BUSY_REG; break;
    case 7:
        if (AudioHle || GraphicsHle || SemaphoreExit == 0)
        {
            RSP_GPR[RSPOpC.rt].W = 0;
        }
        else
        {
            RSP_GPR[RSPOpC.rt].W = *RSPInfo.SP_SEMAPHORE_REG;
            *RSPInfo.SP_SEMAPHORE_REG = 1;
            RSP_Running = false;
        }
        break;
    case 8: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.DPC_START_REG; break;
    case 9: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.DPC_END_REG; break;
    case 10: RSP_GPR[RSPOpC.rt].UW = *RSPInfo.DPC_CURRENT_REG; break;
    case 11: RSP_GPR[RSPOpC.rt].W = *RSPInfo.DPC_STATUS_REG; break;
    case 12: RSP_GPR[RSPOpC.rt].W = *RSPInfo.DPC_CLOCK_REG; break;
    default:
        g_Notify->DisplayError(stdstr_f("We have not implemented RSP MF CP0 reg %s (%d)", COP0_Name(RSPOpC.rd), RSPOpC.rd).c_str());
    }
}

void RSP_Cop0_MT(void)
{
    if (LogRDP && g_CPUCore == InterpreterCPU)
    {
        RDP_LogMT0(*PrgCount, RSPOpC.rd, RSP_GPR[RSPOpC.rt].UW);
    }
    switch (RSPOpC.rd)
    {
    case 0: *RSPInfo.SP_MEM_ADDR_REG = RSP_GPR[RSPOpC.rt].UW; break;
    case 1: *RSPInfo.SP_DRAM_ADDR_REG = RSP_GPR[RSPOpC.rt].UW; break;
    case 2:
        *RSPInfo.SP_RD_LEN_REG = RSP_GPR[RSPOpC.rt].UW;
        SP_DMA_READ();
        break;
    case 3:
        *RSPInfo.SP_WR_LEN_REG = RSP_GPR[RSPOpC.rt].UW;
        SP_DMA_WRITE();
        break;
    case 4:
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_HALT) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_HALT;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_HALT) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_HALT;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_BROKE) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_BROKE;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_INTR) != 0)
        {
            *RSPInfo.MI_INTR_REG &= ~MI_INTR_SP;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_INTR) != 0)
        {
            *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
            RSPInfo.CheckInterrupts();
            RSP_Running = false;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SSTEP) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SSTEP;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SSTEP) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SSTEP;
            RSP_NextInstruction = RSPPIPELINE_SINGLE_STEP;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_INTR_BREAK) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_INTR_BREAK) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_INTR_BREAK;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG0) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG0;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG0) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG0;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG1) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG1;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG1) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG1;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG2) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG2;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG2) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG2;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG3) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG3;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG3) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG3;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG4) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG4;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG4) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG4;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG5) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG5;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG5) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG5;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG6) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG6;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG6) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG6;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_CLR_SIG7) != 0)
        {
            *RSPInfo.SP_STATUS_REG &= ~SP_STATUS_SIG7;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_SIG7) != 0)
        {
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_SIG7;
        }
        break;
    case 7: *RSPInfo.SP_SEMAPHORE_REG = 0; break;
    case 8:
        *RSPInfo.DPC_START_REG = RSP_GPR[RSPOpC.rt].UW;
        *RSPInfo.DPC_CURRENT_REG = RSP_GPR[RSPOpC.rt].UW;
        break;
    case 9:
        *RSPInfo.DPC_END_REG = RSP_GPR[RSPOpC.rt].UW;
        RDP_LogDlist();
        if (RSPInfo.ProcessRdpList != NULL)
        {
            RSPInfo.ProcessRdpList();
        }
        break;
    case 10: *RSPInfo.DPC_CURRENT_REG = RSP_GPR[RSPOpC.rt].UW; break;
    case 11:
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_XBUS_DMEM_DMA) != 0)
        {
            *RSPInfo.DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_SET_XBUS_DMEM_DMA) != 0)
        {
            *RSPInfo.DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_FREEZE) != 0)
        {
            *RSPInfo.DPC_STATUS_REG &= ~DPC_STATUS_FREEZE;
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_SET_FREEZE) != 0)
        {
            *RSPInfo.DPC_STATUS_REG |= DPC_STATUS_FREEZE;
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_FLUSH) != 0)
        {
            *RSPInfo.DPC_STATUS_REG &= ~DPC_STATUS_FLUSH;
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_SET_FLUSH) != 0)
        {
            *RSPInfo.DPC_STATUS_REG |= DPC_STATUS_FLUSH;
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_TMEM_CTR) != 0)
        { /* DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_TMEM_CTR"); */
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_PIPE_CTR) != 0)
        {
            g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR");
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_CMD_CTR) != 0)
        {
            g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR");
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_CLOCK_CTR) != 0)
        { /* DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR"); */
        }
        break;
    default:
        g_Notify->DisplayError(stdstr_f("We have not implemented RSP MT CP0 reg %s (%d)", COP0_Name(RSPOpC.rd), RSPOpC.rd).c_str());
    }
}

// COP2 functions

void RSP_Cop2_MF(void)
{
    uint8_t element = (uint8_t)(RSPOpC.sa >> 1);
    RSP_GPR[RSPOpC.rt].B[1] = RSP_Vect[RSPOpC.vs].s8(15 - element);
    RSP_GPR[RSPOpC.rt].B[0] = RSP_Vect[RSPOpC.vs].s8(15 - ((element + 1) % 16));
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rt].HW[0];
}

void RSP_Cop2_CF(void)
{
    switch ((RSPOpC.rd & 0x03))
    {
    case 0: RSP_GPR[RSPOpC.rt].W = RSP_Flags[0].HW[0]; break;
    case 1: RSP_GPR[RSPOpC.rt].W = RSP_Flags[1].HW[0]; break;
    case 2: RSP_GPR[RSPOpC.rt].W = RSP_Flags[2].HW[0]; break;
    case 3: RSP_GPR[RSPOpC.rt].W = RSP_Flags[2].HW[0]; break;
    }
}

void RSP_Cop2_MT(void)
{
    uint8_t element = (uint8_t)(15 - (RSPOpC.sa >> 1));
    RSP_Vect[RSPOpC.vs].s8(element) = RSP_GPR[RSPOpC.rt].B[1];
    if (element != 0)
    {
        RSP_Vect[RSPOpC.vs].s8(element - 1) = RSP_GPR[RSPOpC.rt].B[0];
    }
}

void RSP_Cop2_CT(void)
{
    switch ((RSPOpC.rd & 0x03))
    {
    case 0: RSP_Flags[0].HW[0] = RSP_GPR[RSPOpC.rt].HW[0]; break;
    case 1: RSP_Flags[1].HW[0] = RSP_GPR[RSPOpC.rt].HW[0]; break;
    case 2: RSP_Flags[2].B[0] = RSP_GPR[RSPOpC.rt].B[0]; break;
    case 3: RSP_Flags[2].B[0] = RSP_GPR[RSPOpC.rt].B[0]; break;
    }
}

void RSP_COP2_VECTOR(void)
{
    RSP_Vector[RSPOpC.funct]();
}

// Vector functions

void RSP_Vector_VMULF(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, ((int64_t)RSP_Vect[RSPOpC.vs].s16(el) * (int64_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) * 2) + 0x8000);
        Result.s16(el) = AccumulatorSaturate(el, true);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMULU(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, ((int64_t)RSP_Vect[RSPOpC.vs].s16(el) * (int64_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) * 2) + 0x8000);
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            Result.s16(el) = 0;
        }
        else if ((RSP_ACCUM[el].HW[3] ^ RSP_ACCUM[el].HW[2]) < 0)
        {
            Result.s16(el) = -1;
        }
        else
        {
            Result.s16(el) = RSP_ACCUM[el].HW[2];
        }
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VRNDP(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e);
        if (RSPOpC.vs & 1)
        {
            Value <<= 16;
        }
        int64_t Accum = AccumulatorGet(el);
        if (Accum >= 0)
        {
            Accum = clip48(Accum + Value);
        }
        AccumulatorSet(el, Accum);
        Result.s16(el) = clamp16((int32_t)(Accum >> 16));
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMUDL(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, (uint16_t)((uint32_t)RSP_Vect[RSPOpC.vs].u16(el) * (uint32_t)RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) >> 16));
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMUDM(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, (int32_t)((int32_t)RSP_Vect[RSPOpC.vs].s16(el) * (uint32_t)RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e)));
        Result.s16(el) = RSP_ACCUM[el].HW[2];
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMULQ(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Temp = RSP_Vect[RSPOpC.vs].s16(el) * RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e);
        if (Temp < 0)
        {
            Temp += 31;
        }
        RSP_ACCUM[el].HW[3] = (int16_t)(Temp >> 16);
        RSP_ACCUM[el].HW[2] = (int16_t)Temp;
        RSP_ACCUM[el].HW[1] = 0;

        Result.s16(el) = clamp16(Temp >> 1) & ~15;
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMUDN(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, (int32_t)((uint32_t)RSP_Vect[RSPOpC.vs].u16(el) * (uint32_t)((int32_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e))));
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMUDH(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        RSP_ACCUM[el].W[1] = (int32_t)RSP_Vect[RSPOpC.vs].s16(el) * (int32_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e);
        RSP_ACCUM[el].HW[1] = 0;
        Result.u16(el) = AccumulatorSaturate(el, true);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMACF(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, AccumulatorGet(el) + (((int64_t)RSP_Vect[RSPOpC.vs].s16(el) * (int64_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e)) << 1));
        Result.u16(el) = AccumulatorSaturate(el, true);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMACU(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, AccumulatorGet(el) + (((int64_t)RSP_Vect[RSPOpC.vs].s16(el) * (int64_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e)) << 1));
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            Result.s16(el) = 0;
        }
        else if (RSP_ACCUM[el].UHW[3] != 0 || RSP_ACCUM[el].HW[2] < 0)
        {
            Result.u16(el) = 0xFFFF;
        }
        else
        {
            Result.s16(el) = RSP_ACCUM[el].HW[2];
        }
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMACQ(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Accum = (RSP_ACCUM[el].UHW[3] << 16) | RSP_ACCUM[el].UHW[2];
        if (Accum < -0x20 && ((Accum & 0x20) == 0))
        {
            Accum += 0x20;
        }
        else if (Accum > 0x20 && (Accum & 0x20) == 0)
        {
            Accum -= 0x20;
        }
        Result.u16(el) = clamp16(Accum >> 1) & 0xFFF0;
        RSP_ACCUM[el].UHW[3] = (uint16_t)(Accum >> 16);
        RSP_ACCUM[el].UHW[2] = (uint16_t)Accum;
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VRNDN(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e);
        if (RSPOpC.vs & 1)
        {
            Value <<= 16;
        }
        int64_t Accum = AccumulatorGet(el);
        if (Accum < 0)
        {
            Accum = clip48(Accum + Value);
        }
        AccumulatorSet(el, Accum);
        Result.s16(el) = clamp16((int32_t)(Accum >> 16));
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMADL(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, AccumulatorGet(el) + (((uint32_t)(RSP_Vect[RSPOpC.vs].u16(el)) * (uint32_t)RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e)) >> 16));
        Result.u16(el) = AccumulatorSaturate(el, false);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMADM(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, AccumulatorGet(el) + (RSP_Vect[RSPOpC.vs].s16(el) * RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e)));
        Result.u16(el) = AccumulatorSaturate(el, true);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMADN(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        AccumulatorSet(el, AccumulatorGet(el) + (int64_t)(RSP_Vect[RSPOpC.vs].u16(el) * RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e)));
        Result.u16(el) = AccumulatorSaturate(el, false);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VMADH(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = (int32_t)((AccumulatorGet(el) >> 16) + (int32_t)RSP_Vect[RSPOpC.vs].s16(el) * (int32_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e));
        RSP_ACCUM[el].HW[3] = (int16_t)(Value >> 16);
        RSP_ACCUM[el].HW[2] = (int16_t)(Value >> 0);
        Result.u16(el) = AccumulatorSaturate(el, true);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VADD(void)
{
    UWORD32 temp;
    RSPVector Result;

    for (uint8_t el = 0; el < 8; el++)
    {
        temp.W = (int32_t)RSP_Vect[RSPOpC.vs].s16(el) + (int32_t)RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) + ((RSP_Flags[0].UW >> (7 - el)) & 0x1);
        RSP_ACCUM[el].HW[1] = temp.HW[0];
        if ((temp.HW[0] & 0x8000) == 0)
        {
            Result.u16(el) = temp.HW[1] != 0 ? 0x8000 : temp.UHW[0];
        }
        else
        {
            Result.u16(el) = temp.HW[1] != -1 ? 0x7FFF : temp.UHW[0];
        }
    }
    RSP_Vect[RSPOpC.vd] = Result;
    RSP_Flags[0].UW = 0;
}

void RSP_Vector_VSUB(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        temp.W = (int)RSP_Vect[RSPOpC.vs].s16(el) - (int)RSP_Vect[RSPOpC.vt].s16(del) -
                 ((RSP_Flags[0].UW >> (7 - el)) & 0x1);
        RSP_ACCUM[el].HW[1] = temp.HW[0];
        if ((temp.HW[0] & 0x8000) == 0)
        {
            if (temp.HW[1] != 0)
            {
                Result.u16(el) = 0x8000;
            }
            else
            {
                Result.u16(el) = temp.UHW[0];
            }
        }
        else
        {
            if (temp.HW[1] != -1)
            {
                Result.u16(el) = 0x7FFF;
            }
            else
            {
                Result.u16(el) = temp.UHW[0];
            }
        }
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VABS(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (RSP_Vect[RSPOpC.vs].s16(el) > 0)
        {
            Result.s16(el) = RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e);
        }
        else if (RSP_Vect[RSPOpC.vs].s16(el) < 0)
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) == 0x8000 ? 0x7FFF : RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) * -1;
        }
        else
        {
            Result.u16(el) = 0;
        }
        RSP_ACCUM[el].UHW[1] = Result.u16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VADDC(void)
{
    RSPVector Result;
    VCOH.Clear();
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Temp = (int32_t)RSP_Vect[RSPOpC.vs].u16(el) + (int32_t)RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e);
        RSP_ACCUM[el].HW[1] = (int16_t)Temp;
        Result.u16(el) = RSP_ACCUM[el].HW[1];
        VCOL.Set(el, (Temp >> 16) != 0);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VSUBC(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    RSP_Flags[0].UW = 0x0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        temp.UW = (int)RSP_Vect[RSPOpC.vs].u16(el) - (int)RSP_Vect[RSPOpC.vt].u16(del);
        RSP_ACCUM[el].HW[1] = temp.HW[0];
        Result.u16(el) = temp.UHW[0];
        if (temp.HW[0] != 0)
        {
            RSP_Flags[0].UW |= (0x1 << (15 - el));
        }
        if (temp.UW & 0xffff0000)
        {
            RSP_Flags[0].UW |= (0x1 << (7 - el));
        }
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_Reserved(void)
{
    for (uint8_t el = 0; el < 8; el++)
    {
        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.vs].s16(el) + RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e);
    }
    RSP_Vect[RSPOpC.vd] = RSPVector();
}

void RSP_Vector_VSAW(void)
{
    RSPVector Result;

    switch ((RSPOpC.rs & 0xF))
    {
    case 8:
        Result.s16(0) = RSP_ACCUM[0].HW[3];
        Result.s16(1) = RSP_ACCUM[1].HW[3];
        Result.s16(2) = RSP_ACCUM[2].HW[3];
        Result.s16(3) = RSP_ACCUM[3].HW[3];
        Result.s16(4) = RSP_ACCUM[4].HW[3];
        Result.s16(5) = RSP_ACCUM[5].HW[3];
        Result.s16(6) = RSP_ACCUM[6].HW[3];
        Result.s16(7) = RSP_ACCUM[7].HW[3];
        break;
    case 9:
        Result.s16(0) = RSP_ACCUM[0].HW[2];
        Result.s16(1) = RSP_ACCUM[1].HW[2];
        Result.s16(2) = RSP_ACCUM[2].HW[2];
        Result.s16(3) = RSP_ACCUM[3].HW[2];
        Result.s16(4) = RSP_ACCUM[4].HW[2];
        Result.s16(5) = RSP_ACCUM[5].HW[2];
        Result.s16(6) = RSP_ACCUM[6].HW[2];
        Result.s16(7) = RSP_ACCUM[7].HW[2];
        break;
    case 10:
        Result.s16(0) = RSP_ACCUM[0].HW[1];
        Result.s16(1) = RSP_ACCUM[1].HW[1];
        Result.s16(2) = RSP_ACCUM[2].HW[1];
        Result.s16(3) = RSP_ACCUM[3].HW[1];
        Result.s16(4) = RSP_ACCUM[4].HW[1];
        Result.s16(5) = RSP_ACCUM[5].HW[1];
        Result.s16(6) = RSP_ACCUM[6].HW[1];
        Result.s16(7) = RSP_ACCUM[7].HW[1];
        break;
    default:
        Result.u64(1) = 0;
        Result.u64(0) = 0;
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VLT(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        if (RSP_Vect[RSPOpC.vs].s16(el) < RSP_Vect[RSPOpC.vt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vs].u16(el);
            RSP_Flags[1].UW |= (1 << (7 - el));
        }
        else if (RSP_Vect[RSPOpC.vs].s16(el) != RSP_Vect[RSPOpC.vt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vt].u16(del);
            RSP_Flags[1].UW &= ~(1 << (7 - el));
        }
        else
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vs].u16(el);
            if ((RSP_Flags[0].UW & (0x101 << (7 - el))) == (uint16_t)(0x101 << (7 - el)))
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
            else
            {
                RSP_Flags[1].UW &= ~(1 << (7 - el));
            }
        }
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VEQ(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        if (RSP_Vect[RSPOpC.vs].u16(el) == RSP_Vect[RSPOpC.vt].u16(del))
        {
            if ((RSP_Flags[0].UW & (1 << (15 - el))) == 0)
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
        }
        Result.u16(el) = RSP_Vect[RSPOpC.vt].u16(del);
        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.vt].u16(del);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VNE(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        if (RSP_Vect[RSPOpC.vs].u16(el) != RSP_Vect[RSPOpC.vt].u16(del))
        {
            RSP_Flags[1].UW |= (1 << (7 - el));
        }
        else
        {
            if ((RSP_Flags[0].UW & (1 << (15 - el))) != 0)
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
        }
        Result.u16(el) = RSP_Vect[RSPOpC.vs].u16(el);
        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.vs].u16(el);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VGE(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        if (RSP_Vect[RSPOpC.vs].s16(el) == RSP_Vect[RSPOpC.vt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vs].u16(el);
            if ((RSP_Flags[0].UW & (0x101 << (7 - el))) == (uint16_t)(0x101 << (7 - el)))
            {
                RSP_Flags[1].UW &= ~(1 << (7 - el));
            }
            else
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
        }
        else if (RSP_Vect[RSPOpC.vs].s16(el) > RSP_Vect[RSPOpC.vt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vs].u16(el);
            RSP_Flags[1].UW |= (1 << (7 - el));
        }
        else
        {
            Result.u16(el) = RSP_Vect[RSPOpC.vt].u16(del);
            RSP_Flags[1].UW &= ~(1 << (7 - el));
        }
        RSP_ACCUM[el].UHW[1] = Result.u16(el);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VCL(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (VCOL.Get(el))
        {
            if (VCOH.Get(el))
            {
                RSP_ACCUM[el].HW[1] = VCCL.Get(el) ? -RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) : RSP_Vect[RSPOpC.vs].s16(el);
            }
            else
            {
                bool Set = VCE.Get(el) ? (RSP_Vect[RSPOpC.vs].u16(el) + RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) <= 0x10000) : (RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) + RSP_Vect[RSPOpC.vs].u16(el) == 0);
                RSP_ACCUM[el].HW[1] = Set ? -RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) : RSP_Vect[RSPOpC.vs].s16(el);
                VCCL.Set(el, Set);
            }
        }
        else
        {
            if (VCOH.Get(el))
            {
                RSP_ACCUM[el].UHW[1] = VCCH.Get(el) ? RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) : RSP_Vect[RSPOpC.vs].s16(el);
            }
            else
            {
                RSP_ACCUM[el].HW[1] = VCCH.Set(el, RSP_Vect[RSPOpC.vs].u16(el) - RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) >= 0) ? RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) : RSP_Vect[RSPOpC.vs].s16(el);
            }
        }
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    VCOL.Clear();
    VCOH.Clear();
    VCE.Clear();
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VCH(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[0].UW = 0;
    RSP_Flags[1].UW = 0;
    RSP_Flags[2].UW = 0;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        if ((RSP_Vect[RSPOpC.vs].s16(el) ^ RSP_Vect[RSPOpC.vt].s16(del)) < 0)
        {
            RSP_Flags[0].UW |= (1 << (7 - el));
            if (RSP_Vect[RSPOpC.vt].s16(del) < 0)
            {
                RSP_Flags[1].UW |= (1 << (15 - el));
            }
            if (RSP_Vect[RSPOpC.vs].s16(el) + RSP_Vect[RSPOpC.vt].s16(del) <= 0)
            {
                if (RSP_Vect[RSPOpC.vs].s16(el) + RSP_Vect[RSPOpC.vt].s16(del) == -1)
                {
                    RSP_Flags[2].UW |= (1 << (7 - el));
                }
                RSP_Flags[1].UW |= (1 << (7 - el));
                RSP_ACCUM[el].HW[1] = -RSP_Vect[RSPOpC.vt].u16(del);
            }
            else
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.vs].s16(el);
            }
            if (RSP_Vect[RSPOpC.vs].s16(el) + RSP_Vect[RSPOpC.vt].s16(del) != 0)
            {
                if (RSP_Vect[RSPOpC.vs].s16(el) != ~RSP_Vect[RSPOpC.vt].s16(del))
                {
                    RSP_Flags[0].UW |= (1 << (15 - el));
                }
            }
        }
        else
        {
            if (RSP_Vect[RSPOpC.vt].s16(del) < 0)
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
            if (RSP_Vect[RSPOpC.vs].s16(el) - RSP_Vect[RSPOpC.vt].s16(del) >= 0)
            {
                RSP_Flags[1].UW |= (1 << (15 - el));
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.vt].u16(del);
            }
            else
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.vs].s16(el);
            }
            if (RSP_Vect[RSPOpC.vs].s16(el) - RSP_Vect[RSPOpC.vt].s16(del) != 0)
            {
                if (RSP_Vect[RSPOpC.vs].s16(el) != ~RSP_Vect[RSPOpC.vt].s16(del))
                {
                    RSP_Flags[0].UW |= (1 << (15 - el));
                }
            }
        }
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VCR(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if ((RSP_Vect[RSPOpC.vs].s16(el) ^ RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e)) < 0)
        {
            VCCH.Set(el, RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) < 0);
            RSP_ACCUM[el].HW[1] = VCCL.Set(el, RSP_Vect[RSPOpC.vs].s16(el) + RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) + 1 <= 0) ? ~RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) : RSP_Vect[RSPOpC.vs].u16(el);
        }
        else
        {
            VCCL.Set(el, RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) < 0);
            RSP_ACCUM[el].HW[1] = VCCH.Set(el, RSP_Vect[RSPOpC.vs].s16(el) - RSP_Vect[RSPOpC.vt].se(el, RSPOpC.e) >= 0) ? RSP_Vect[RSPOpC.vt].ue(el, RSPOpC.e) : RSP_Vect[RSPOpC.vs].u16(el);
        }
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.vd] = Result;
    VCOL.Clear();
    VCOH.Clear();
    VCE.Clear();
}

void RSP_Vector_VMRG(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];

        if ((RSP_Flags[1].UW & (1 << (7 - el))) != 0)
        {
            Result.s16(el) = RSP_Vect[RSPOpC.vs].s16(el);
        }
        else
        {
            Result.s16(el) = RSP_Vect[RSPOpC.vt].s16(del);
        }
        RSP_ACCUM[el].HW[1] = Result.s16(el); // Suggested by Angrylion
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VAND(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        Result.s16(el) = RSP_Vect[RSPOpC.vs].s16(el) & RSP_Vect[RSPOpC.vt].s16(del);
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VNAND(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        Result.s16(el) = ~(RSP_Vect[RSPOpC.vs].s16(el) & RSP_Vect[RSPOpC.vt].s16(del));
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        Result.s16(el) = RSP_Vect[RSPOpC.vs].s16(el) | RSP_Vect[RSPOpC.vt].s16(del);
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VNOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        Result.s16(el) = ~(RSP_Vect[RSPOpC.vs].s16(el) | RSP_Vect[RSPOpC.vt].s16(del));
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VXOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        Result.s16(el) = RSP_Vect[RSPOpC.vs].s16(el) ^ RSP_Vect[RSPOpC.vt].s16(del);
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VNXOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        Result.s16(el) = ~(RSP_Vect[RSPOpC.vs].s16(el) ^ RSP_Vect[RSPOpC.vt].s16(del));
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.vd] = Result;
}

void RSP_Vector_VRCP(void)
{
    RecpResult.W = RSP_Vect[RSPOpC.vt].s16(EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)]);
    if (RecpResult.UW == 0)
    {
        RecpResult.UW = 0x7FFFFFFF;
    }
    else
    {
        bool neg = RecpResult.W < 0;
        if (neg)
        {
            RecpResult.W = ~RecpResult.W + 1;
        }
        for (int count = 15; count > 0; count--)
        {
            if ((RecpResult.W & (1 << count)))
            {
                RecpResult.W &= (0xFFC0 >> (15 - count));
                count = 0;
            }
        }
        {
            uint32_t RoundMethod = _RC_CHOP;
            uint32_t OldModel = _controlfp(RoundMethod, _MCW_RC);
            RecpResult.W = (long)((0x7FFFFFFF / (double)RecpResult.W));
            OldModel = _controlfp(OldModel, _MCW_RC);
        }
        for (int count = 31; count > 0; count--)
        {
            if ((RecpResult.W & (1 << count)))
            {
                RecpResult.W &= (0xFFFF8000 >> (31 - count));
                count = 0;
            }
        }
        if (neg == true)
        {
            RecpResult.W = ~RecpResult.W;
        }
    }
    for (int count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]);
    }
    RSP_Vect[RSPOpC.vd].s16(7 - (RSPOpC.rd & 0x7)) = RecpResult.UHW[0];
}

void RSP_Vector_VRCPL(void)
{
    int count;
    bool neg;

    RecpResult.UW = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)]) | Recp.W;
    if (RecpResult.UW == 0)
    {
        RecpResult.UW = 0x7FFFFFFF;
    }
    else
    {
        if (RecpResult.W < 0)
        {
            neg = true;
            if (RecpResult.UHW[1] == 0xFFFF && RecpResult.HW[0] < 0)
            {
                RecpResult.W = ~RecpResult.W + 1;
            }
            else
            {
                RecpResult.W = ~RecpResult.W;
            }
        }
        else
        {
            neg = false;
        }
        for (count = 31; count > 0; count--)
        {
            if ((RecpResult.W & (1 << count)))
            {
                RecpResult.W &= (0xFFC00000 >> (31 - count));
                count = 0;
            }
        }
        {
            uint32_t OldModel = _controlfp(_RC_CHOP, _MCW_RC);
            //RecpResult.W = 0x7FFFFFFF / RecpResult.W;
            RecpResult.W = (long)((0x7FFFFFFF / (double)RecpResult.W));
            OldModel = _controlfp(OldModel, _MCW_RC);
        }
        for (count = 31; count > 0; count--)
        {
            if ((RecpResult.W & (1 << count)))
            {
                RecpResult.W &= (0xFFFF8000 >> (31 - count));
                count = 0;
            }
        }
        if (neg == true)
        {
            RecpResult.W = ~RecpResult.W;
        }
    }
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]);
    }
    RSP_Vect[RSPOpC.vd].s16(7 - (RSPOpC.rd & 0x7)) = RecpResult.UHW[0];
}

void RSP_Vector_VRCPH(void)
{
    int count;

    Recp.UHW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)]);
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]);
    }
    RSP_Vect[RSPOpC.vd].u16(7 - (RSPOpC.rd & 0x7)) = RecpResult.UHW[1];
}

void RSP_Vector_VMOV(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        RSP_ACCUM[i].HW[1] = RSP_Vect[RSPOpC.vt].ue(i, RSPOpC.e);
    }
    uint8_t Index = 7 - (RSPOpC.de & 0x7);
    RSP_Vect[RSPOpC.vd].u16(Index) = RSP_Vect[RSPOpC.vt].se(Index, RSPOpC.e);
}

void RSP_Vector_VRSQ(void)
{
    int count;
    bool neg;

    SQrootResult.W = RSP_Vect[RSPOpC.vt].s16(EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)]);
    if (SQrootResult.UW == 0)
    {
        SQrootResult.UW = 0x7FFFFFFF;
    }
    else if (SQrootResult.UW == 0xFFFF8000)
    {
        SQrootResult.UW = 0xFFFF0000;
    }
    else
    {
        if (SQrootResult.W < 0)
        {
            neg = true;
            SQrootResult.W = ~SQrootResult.W + 1;
        }
        else
        {
            neg = false;
        }
        for (count = 15; count > 0; count--)
        {
            if ((SQrootResult.W & (1 << count)))
            {
                SQrootResult.W &= (0xFF80 >> (15 - count));
                count = 0;
            }
        }
        {
            uint32_t RoundMethod = _RC_CHOP;
            uint32_t OldModel = _controlfp(RoundMethod, _MCW_RC);
            SQrootResult.W = (long)(0x7FFFFFFF / sqrt(SQrootResult.W));
            OldModel = _controlfp(OldModel, _MCW_RC);
        }
        for (count = 31; count > 0; count--)
        {
            if ((SQrootResult.W & (1 << count)))
            {
                SQrootResult.W &= (0xFFFF8000 >> (31 - count));
                count = 0;
            }
        }
        if (neg == true)
        {
            SQrootResult.W = ~SQrootResult.W;
        }
    }
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]);
    }
    RSP_Vect[RSPOpC.vd].s16(7 - (RSPOpC.rd & 0x7)) = SQrootResult.UHW[0];
}

void RSP_Vector_VRSQL(void)
{
    int count;
    bool neg;

    SQrootResult.UW = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)]) | SQroot.W;
    if (SQrootResult.UW == 0)
    {
        SQrootResult.UW = 0x7FFFFFFF;
    }
    else if (SQrootResult.UW == 0xFFFF8000)
    {
        SQrootResult.UW = 0xFFFF0000;
    }
    else
    {
        if (SQrootResult.W < 0)
        {
            neg = true;
            if (SQrootResult.UHW[1] == 0xFFFF && SQrootResult.HW[0] < 0)
            {
                SQrootResult.W = ~SQrootResult.W + 1;
            }
            else
            {
                SQrootResult.W = ~SQrootResult.W;
            }
        }
        else
        {
            neg = false;
        }
        for (count = 31; count > 0; count--)
        {
            if ((SQrootResult.W & (1 << count)))
            {
                SQrootResult.W &= (0xFF800000 >> (31 - count));
                count = 0;
            }
        }
        {
            uint32_t OldModel = _controlfp(_RC_CHOP, _MCW_RC);
            SQrootResult.W = (long)(0x7FFFFFFF / sqrt(SQrootResult.W));
            OldModel = _controlfp(OldModel, _MCW_RC);
        }
        for (count = 31; count > 0; count--)
        {
            if ((SQrootResult.W & (1 << count)))
            {
                SQrootResult.W &= (0xFFFF8000 >> (31 - count));
                count = 0;
            }
        }
        if (neg == true)
        {
            SQrootResult.W = ~SQrootResult.W;
        }
    }
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]);
    }
    RSP_Vect[RSPOpC.vd].s16(7 - (RSPOpC.rd & 0x7)) = SQrootResult.UHW[0];
}

void RSP_Vector_VRSQH(void)
{
    int count;

    SQroot.UHW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)]);
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]);
    }
    RSP_Vect[RSPOpC.vd].u16(7 - (RSPOpC.rd & 0x7)) = SQrootResult.UHW[1];
}

void RSP_Vector_VNOOP(void)
{
}

// LC2 functions

void RSP_Opcode_LBV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 0)) & 0xFFF;
    RSP_Vect[RSPOpC.rt].u8((uint8_t)(15 - RSPOpC.del)) = *(RSPInfo.DMEM + (Address ^ 3));
}

void RSP_Opcode_LSV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 1)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)2, (uint8_t)(16 - RSPOpC.del));
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(Length + RSPOpC.del); i < n; i++, Address++)
    {
        RSP_Vect[RSPOpC.rt].u8(15 - i) = *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSP_Opcode_LLV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 2)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)4, (uint8_t)(16 - RSPOpC.del));
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(Length + RSPOpC.del); i < n; i++, Address++)
    {
        RSP_Vect[RSPOpC.rt].u8(15 - i) = *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSP_Opcode_LDV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)8, (uint8_t)(16 - RSPOpC.del));
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(Length + RSPOpC.del); i < n; i++, Address++)
    {
        RSP_Vect[RSPOpC.rt].u8(15 - i) = *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSP_Opcode_LQV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)(((Address + 0x10) & ~0xF) - Address), (uint8_t)(16 - RSPOpC.del));
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(Length + RSPOpC.del); i < n; i++, Address++)
    {
        RSP_Vect[RSPOpC.rt].u8(15 - i) = *(RSPInfo.DMEM + (Address ^ 3));
    }
}

void RSP_Opcode_LRV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    uint8_t Offset = (uint8_t)((0x10 - (Address & 0xF)) + RSPOpC.del);
    Address &= 0xFF0;
    for (uint8_t i = Offset; i < 16; i++, Address++)
    {
        RSP_Vect[RSPOpC.rt].u8(15 - i) = *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSP_Opcode_LPV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3));
    uint32_t Offset = ((Address & 7) - RSPOpC.del);
    Address &= ~7;

    for (uint8_t i = 0; i < 8; i++)
    {
        RSP_Vect[RSPOpC.rt].u16(7 - i) = *(RSPInfo.DMEM + ((Address + ((Offset + i) & 0xF) ^ 3) & 0xFFF)) << 8;
    }
}

void RSP_Opcode_LUV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3));
    uint32_t Offset = ((Address & 7) - RSPOpC.del);
    Address &= ~7;

    for (uint8_t i = 0; i < 8; i++)
    {
        RSP_Vect[RSPOpC.rt].s16(7 - i) = *(RSPInfo.DMEM + ((Address + ((Offset + i) & 0xF) ^ 3) & 0xFFF)) << 7;
    }
}

void RSP_Opcode_LHV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4));
    uint32_t Offset = ((Address & 7) - RSPOpC.del);
    Address &= ~7;

    for (uint8_t i = 0; i < 8; i++)
    {
        RSP_Vect[RSPOpC.rt].s16(7 - i) = *(RSPInfo.DMEM + ((Address + ((Offset + (i << 1)) & 0xF) ^ 3) & 0xFFF)) << 7;
    }
}

void RSP_Opcode_LFV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4));
    uint8_t Length = std::min((uint8_t)(8 + RSPOpC.del), (uint8_t)16);
    uint32_t Offset = ((Address & 7) - RSPOpC.del);
    Address &= ~7;

    RSPVector Temp;
    for (uint8_t i = 0; i < 4; i++)
    {
        Temp.s16(i) = *(RSPInfo.DMEM + ((Address + ((Offset + (i << 2)) & 0xF) ^ 3) & 0xFFF)) << 7;
        Temp.s16(i + 4) = *(RSPInfo.DMEM + ((Address + ((Offset + (i << 2) + 8) & 0xF) ^ 3) & 0xFFF)) << 7;
    }

    for (uint8_t i = RSPOpC.del; i < Length; i++)
    {
        RSP_Vect[RSPOpC.rt].s8(15 - i) = Temp.s8(i ^ 1);
    }
}

void RSP_Opcode_LWV(void)
{
}

void RSP_Opcode_LTV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4));
    uint32_t Start = Address & ~7;
    uint32_t End = Start + 0x10;
    Address = Start + ((RSPOpC.del + (Address & 8)) & 0xF);
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t del = (((RSPOpC.del >> 1) + i) & 7) + (RSPOpC.rt & ~7);
        RSP_Vect[del].s8(15 - (i * 2 + 0)) = *(RSPInfo.DMEM + ((Address++ ^ 3) & 0xFFF));
        if (Address == End)
        {
            Address = Start;
        }
        RSP_Vect[del].s8(15 - (i * 2 + 1)) = *(RSPInfo.DMEM + ((Address++ ^ 3) & 0xFFF));
        if (Address == End)
        {
            Address = Start;
        }
    }
}

// SC2 functions

void RSP_Opcode_SBV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 0)) & 0xFFF;
    *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8((uint8_t)(15 - RSPOpC.del));
}

void RSP_Opcode_SSV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 1)) & 0xFFF;
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(2 + RSPOpC.del); i < n; i++, Address++)
    {
        *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - (i & 0xF));
    }
}

void RSP_Opcode_SLV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 2)) & 0xFFF;
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(4 + RSPOpC.del); i < n; i++, Address++)
    {
        *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - (i & 0xF));
    }
}

void RSP_Opcode_SDV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    for (uint8_t i = RSPOpC.del; i < (8 + RSPOpC.del); i++, Address++)
    {
        *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - (i & 0xF));
    }
}

void RSP_Opcode_SQV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    uint8_t Length = (uint8_t)(((Address + 0x10) & ~0xF) - Address);
    for (uint8_t i = RSPOpC.del; i < (Length + RSPOpC.del); i++, Address++)
    {
        *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - (i & 0xF));
    }
}

void RSP_Opcode_SRV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    uint8_t Length = (Address & 0xF);
    uint8_t Offset = (0x10 - Length) & 0xF;
    Address &= 0xFF0;
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(Length + RSPOpC.del); i < n; i++, Address++)
    {
        *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - ((i + Offset) & 0xF));
    }
}

void RSP_Opcode_SPV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(8 + RSPOpC.del); i < n; i++, Address++)
    {
        if (((i)&0xF) < 8)
        {
            *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - ((i & 0xF) << 1));
        }
        else
        {
            *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u8(15 - ((i & 0x7) << 1)) << 1) + (RSP_Vect[RSPOpC.rt].u8(14 - ((i & 0x7) << 1)) >> 7);
        }
    }
}

void RSP_Opcode_SUV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    for (uint8_t Count = RSPOpC.del; Count < (8 + RSPOpC.del); Count++, Address++)
    {
        if (((Count)&0xF) < 8)
        {
            *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = ((RSP_Vect[RSPOpC.rt].u8(15 - ((Count & 0x7) << 1)) << 1) + (RSP_Vect[RSPOpC.rt].u8(14 - ((Count & 0x7) << 1)) >> 7)) & 0xFF;
        }
        else
        {
            *(RSPInfo.DMEM + ((Address ^ 3) & 0xFFF)) = RSP_Vect[RSPOpC.rt].u8(15 - ((Count & 0x7) << 1));
        }
    }
}

void RSP_Opcode_SHV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4));
    uint8_t Offset = Address & 7;
    Address &= ~7;
    for (uint32_t i = 0; i < 16; i += 2)
    {
        uint8_t Value = (RSP_Vect[RSPOpC.rt].u8(15 - ((RSPOpC.del + i) & 15)) << 1) | (RSP_Vect[RSPOpC.rt].u8(15 - ((RSPOpC.del + i + 1) & 15)) >> 7);
        *(RSPInfo.DMEM + ((Address + (Offset + i & 15) ^ 3) & 0xFFF)) = Value;
    }
}

void RSP_Opcode_SFV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    uint8_t Offset = Address & 7;
    Address &= 0xFF8;

    switch (RSPOpC.del)
    {
    case 0:
    case 15:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(7) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(6) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(5) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(4) >> 7) & 0xFF;
        break;
    case 1:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(1) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(0) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(3) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(2) >> 7) & 0xFF;
        break;
    case 4:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(6) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(5) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(4) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(7) >> 7) & 0xFF;
        break;
    case 5:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(0) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(3) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(2) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(1) >> 7) & 0xFF;
        break;
    case 8:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(3) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(2) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(1) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(0) >> 7) & 0xFF;
        break;
    case 11:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(4) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(7) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(6) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(5) >> 7) & 0xFF;
        break;
    case 12:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(2) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(1) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(0) >> 7) & 0xFF;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (RSP_Vect[RSPOpC.rt].u16(3) >> 7) & 0xFF;
        break;
    default:
        *(RSPInfo.DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = 0;
        *(RSPInfo.DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = 0;
        *(RSPInfo.DMEM + (((Address + ((Offset + 8) & 0xF) & 0xFFF)) ^ 3)) = 0;
        *(RSPInfo.DMEM + (((Address + ((Offset + 12) & 0xF) & 0xFFF)) ^ 3)) = 0;
        break;
    }
}

void RSP_Opcode_STV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4));
    uint8_t Element = 16 - (RSPOpC.del & ~1);
    uint8_t Offset = (Address & 7) - (RSPOpC.del & ~1);
    Address &= ~7;
    for (uint32_t i = 0; i < 16; i += 2)
    {
        uint8_t Del = (uint8_t)((RSPOpC.vt & ~7) + (i >> 1));
        *(RSPInfo.DMEM + (((Address + (Offset + i & 15) ^ 3)) & 0xFFF)) = RSP_Vect[Del].s8(15 - ((Element + i) & 15));
        *(RSPInfo.DMEM + (((Address + (Offset + i + 1 & 15) ^ 3)) & 0xFFF)) = RSP_Vect[Del].s8(15 - ((Element + i + 1) & 15));
    }
}

void RSP_Opcode_SWV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    uint8_t Offset = Address & 7;
    Address &= 0xFF8;
    for (uint8_t i = RSPOpC.del, n = (uint8_t)(16 + RSPOpC.del); i < n; i++, Offset++)
    {
        *(RSPInfo.DMEM + ((Address + (Offset & 0xF)) ^ 3)) = RSP_Vect[RSPOpC.rt].s8(15 - (i & 0xF));
    }
}

// Other functions

void rsp_UnknownOpcode(void)
{
    g_RSPDebugger->UnknownOpcode();
}
