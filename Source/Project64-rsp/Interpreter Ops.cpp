#include "CPU.h"
#include "Interpreter CPU.h"
#include "RSP Command.h"
#include "RSP Registers.h"
#include "Rsp.h"
#include "cpu/RspTypes.h"
#include "dma.h"
#include "log.h"
#include "memory.h"
#include "x86.h"
#include "cpu/RSPInstruction.h"
#include <math.h>
#include <stdio.h>
#include <windows.h>

#include <float.h>

// TODO: Is this still an issue? If so, investigate, and if not, remove this!
/*
 * Unfortunately, GCC 4.8.2 stable still has a bug with their <float.h> that
 * includes a different copy of <float.h> from a different directory.
 *
 * Until that bug is fixed, the below macro definitions can be forced.
 *
 * It also is possible to emulate the RSP divide op-codes using a hardware-
 * accurate LUT instead of any floating-point functions, so that works, too.
 */

#ifndef _MCW_RC
#define _MCW_RC 0x00000300
#endif
#ifndef _RC_CHOP
#define _RC_CHOP 0x00000300
#endif

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
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = (RSPOpC.target << 2) & 0xFFC;
}

void RSP_Opcode_JAL(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_GPR[31].UW = (*PrgCount + 8) & 0xFFC;
    RSP_JumpTo = (RSPOpC.target << 2) & 0xFFC;
}

void RSP_Opcode_BEQ(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W == RSP_GPR[RSPOpC.rt].W);
}

void RSP_Opcode_BNE(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W != RSP_GPR[RSPOpC.rt].W);
}

void RSP_Opcode_BLEZ(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W <= 0);
}

void RSP_Opcode_BGTZ(void)
{
    RSP_NextInstruction = DELAY_SLOT;
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
    RSP_LB_DMEM(Address, &RSP_GPR[RSPOpC.rt].UB[0]);
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rt].B[0];
}

void RSP_Opcode_LH(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_LH_DMEM(Address, &RSP_GPR[RSPOpC.rt].UHW[0]);
    RSP_GPR[RSPOpC.rt].W = RSP_GPR[RSPOpC.rt].HW[0];
}

void RSP_Opcode_LW(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_LW_DMEM(Address, &RSP_GPR[RSPOpC.rt].UW);
}

void RSP_Opcode_LBU(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_LB_DMEM(Address, &RSP_GPR[RSPOpC.rt].UB[0]);
    RSP_GPR[RSPOpC.rt].UW = RSP_GPR[RSPOpC.rt].UB[0];
}

void RSP_Opcode_LHU(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_LH_DMEM(Address, &RSP_GPR[RSPOpC.rt].UHW[0]);
    RSP_GPR[RSPOpC.rt].UW = RSP_GPR[RSPOpC.rt].UHW[0];
}

void RSP_Opcode_SB(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_SB_DMEM(Address, RSP_GPR[RSPOpC.rt].UB[0]);
}

void RSP_Opcode_SH(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_SH_DMEM(Address, RSP_GPR[RSPOpC.rt].UHW[0]);
}

void RSP_Opcode_SW(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (short)RSPOpC.offset) & 0xFFF;
    RSP_SW_DMEM(Address, RSP_GPR[RSPOpC.rt].UW);
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
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = (RSP_GPR[RSPOpC.rs].W & 0xFFC);
}

void RSP_Special_JALR(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_GPR[RSPOpC.rd].W = (*PrgCount + 8) & 0xFFC;
    RSP_JumpTo = (RSP_GPR[RSPOpC.rs].W & 0xFFC);
}

void RSP_Special_BREAK(void)
{
    RSP_Running = false;
    *RSPInfo.SP_STATUS_REG |= (SP_STATUS_HALT | SP_STATUS_BROKE);
    if ((*RSPInfo.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
    {
        *RSPInfo.MI_INTR_REG |= R4300i_SP_Intr;
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
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W < 0);
}

void RSP_Opcode_BGEZ(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W >= 0);
}

void RSP_Opcode_BLTZAL(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_GPR[31].UW = (*PrgCount + 8) & 0xFFC;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W < 0);
}

void RSP_Opcode_BGEZAL(void)
{
    RSP_NextInstruction = DELAY_SLOT;
    RSP_GPR[31].UW = (*PrgCount + 8) & 0xFFC;
    RSP_JumpTo = RSP_branch_if(RSP_GPR[RSPOpC.rs].W >= 0);
}

// COP0 functions

void RSP_Cop0_MF(void)
{
    if (LogRDP && CPUCore == InterpreterCPU)
    {
        RDP_LogMF0(*PrgCount, RSPOpC.rd);
    }
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
        DisplayError("We have not implemented RSP MF CP0 reg %s (%d)", COP0_Name(RSPOpC.rd), RSPOpC.rd);
    }
}

void RSP_Cop0_MT(void)
{
    if (LogRDP && CPUCore == InterpreterCPU)
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
            *RSPInfo.MI_INTR_REG &= ~R4300i_SP_Intr;
        }
        if ((RSP_GPR[RSPOpC.rt].W & SP_SET_INTR) != 0)
        {
            *RSPInfo.MI_INTR_REG |= R4300i_SP_Intr;
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
            RSP_NextInstruction = SINGLE_STEP;
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
            DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR");
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_CMD_CTR) != 0)
        {
            DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR");
        }
        if ((RSP_GPR[RSPOpC.rt].W & DPC_CLR_CLOCK_CTR) != 0)
        { /* DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR"); */
        }
        break;
    default:
        DisplayError("We have not implemented RSP MT CP0 reg %s (%d)", COP0_Name(RSPOpC.rd), RSPOpC.rd);
    }
}

// COP2 functions

void RSP_Cop2_MF(void)
{
    uint8_t element = (uint8_t)(RSPOpC.sa >> 1);
    RSP_GPR[RSPOpC.rt].B[1] = RSP_Vect[RSPOpC.rd].s8(15 - element);
    RSP_GPR[RSPOpC.rt].B[0] = RSP_Vect[RSPOpC.rd].s8(15 - ((element + 1) % 16));
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
    RSP_Vect[RSPOpC.rd].s8(element) = RSP_GPR[RSPOpC.rt].B[1];
    if (element != 0)
    {
        RSP_Vect[RSPOpC.rd].s8(element - 1) = RSP_GPR[RSPOpC.rt].B[0];
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
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_Vect[RSPOpC.rd].u16(el) != 0x8000 || RSP_Vect[RSPOpC.rt].u16(del) != 0x8000)
        {
            temp.W = ((int32_t)RSP_Vect[RSPOpC.rd].s16(el) * (int32_t)RSP_Vect[RSPOpC.rt].s16(del)) << 1;
            temp.UW += 0x8000;
            RSP_ACCUM[el].HW[2] = temp.HW[1];
            RSP_ACCUM[el].HW[1] = temp.HW[0];
            RSP_ACCUM[el].HW[3] = (RSP_ACCUM[el].HW[2] < 0) ? -1 : 0;
            Result.s16(el) = RSP_ACCUM[el].HW[2];
        }
        else
        {
            temp.W = 0x80000000;
            RSP_ACCUM[el].UHW[3] = 0;
            RSP_ACCUM[el].UHW[2] = 0x8000;
            RSP_ACCUM[el].UHW[1] = 0x8000;
            Result.s16(el) = 0x7FFF;
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMULU(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        RSP_ACCUM[el].DW = (int64_t)(RSP_Vect[RSPOpC.rd].s16(el) * RSP_Vect[RSPOpC.rt].s16(del)) << 17;
        RSP_ACCUM[el].DW += 0x80000000;
        if (RSP_ACCUM[el].DW < 0)
        {
            Result.s16(el) = 0;
        }
        else if ((int16_t)(RSP_ACCUM[el].UHW[3] ^ RSP_ACCUM[el].UHW[2]) < 0)
        {
            Result.s16(el) = -1;
        }
        else
        {
            Result.s16(el) = RSP_ACCUM[el].HW[2];
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMUDL(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (uint32_t)RSP_Vect[RSPOpC.rd].u16(el) * (uint32_t)RSP_Vect[RSPOpC.rt].u16(del);
        RSP_ACCUM[el].W[1] = 0;
        RSP_ACCUM[el].HW[1] = temp.HW[1];
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMUDM(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (uint32_t)((int32_t)RSP_Vect[RSPOpC.rd].s16(el)) * (uint32_t)RSP_Vect[RSPOpC.rt].u16(del);
        if (temp.W < 0)
        {
            RSP_ACCUM[el].HW[3] = -1;
        }
        else
        {
            RSP_ACCUM[el].HW[3] = 0;
        }
        RSP_ACCUM[el].HW[2] = temp.HW[1];
        RSP_ACCUM[el].HW[1] = temp.HW[0];
        Result.s16(el) = RSP_ACCUM[el].HW[2];
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMUDN(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (uint32_t)RSP_Vect[RSPOpC.rd].u16(el) * (uint32_t)((int32_t)RSP_Vect[RSPOpC.rt].s16(del));
        if (temp.W < 0)
        {
            RSP_ACCUM[el].HW[3] = -1;
        }
        else
        {
            RSP_ACCUM[el].HW[3] = 0;
        }
        RSP_ACCUM[el].HW[2] = temp.HW[1];
        RSP_ACCUM[el].HW[1] = temp.HW[0];
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMUDH(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        RSP_ACCUM[el].W[1] = (int32_t)RSP_Vect[RSPOpC.rd].s16(el) * (int32_t)RSP_Vect[RSPOpC.rt].s16(del);
        RSP_ACCUM[el].HW[1] = 0;
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            if (RSP_ACCUM[el].UHW[3] != 0xFFFF)
            {
                Result.u16(el) = 0x8000;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] >= 0)
                {
                    Result.u16(el) = 0x8000;
                }
                else
                {
                    Result.s16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.u16(el) = 0x7FFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.s16(el) = 0x7FFF;
                }
                else
                {
                    Result.s16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMACF(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        /*temp.W = (long)RSP_Vect[RSPOpC.rd].s16(el) * (long)(DWORD)RSP_Vect[RSPOpC.rt].s16(del);
		RSP_ACCUM[el].UHW[3] += (WORD)(temp.W >> 31);
		temp.UW = temp.UW << 1;
		temp2.UW = temp.UHW[0] + RSP_ACCUM[el].UHW[1];
		RSP_ACCUM[el].HW[1] = temp2.HW[0];
		temp2.UW = temp.UHW[1] + RSP_ACCUM[el].UHW[2] + temp2.UHW[1];
		RSP_ACCUM[el].HW[2] = temp2.HW[0];
		RSP_ACCUM[el].HW[3] += temp2.HW[1];*/
        temp.W = (int32_t)RSP_Vect[RSPOpC.rd].s16(el) * (int32_t)(uint32_t)RSP_Vect[RSPOpC.rt].s16(del);
        RSP_ACCUM[el].DW += ((int64_t)temp.W) << 17;
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            if (RSP_ACCUM[el].UHW[3] != 0xFFFF)
            {
                Result.u16(el) = 0x8000;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] >= 0)
                {
                    Result.u16(el) = 0x8000;
                }
                else
                {
                    Result.s16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.s16(el) = 0x7FFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.s16(el) = 0x7FFF;
                }
                else
                {
                    Result.s16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMACU(void)
{
    uint8_t el, del;
    UWORD32 temp, temp2;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.W = (int32_t)RSP_Vect[RSPOpC.rd].s16(el) * (int32_t)(uint32_t)RSP_Vect[RSPOpC.rt].s16(del);
        RSP_ACCUM[el].UHW[3] = (RSP_ACCUM[el].UHW[3] + (WORD)(temp.W >> 31)) & 0xFFFF;
        temp.UW = temp.UW << 1;
        temp2.UW = temp.UHW[0] + RSP_ACCUM[el].UHW[1];
        RSP_ACCUM[el].HW[1] = temp2.HW[0];
        temp2.UW = temp.UHW[1] + RSP_ACCUM[el].UHW[2] + temp2.UHW[1];
        RSP_ACCUM[el].HW[2] = temp2.HW[0];
        RSP_ACCUM[el].HW[3] += temp2.HW[1];
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            Result.s16(el) = 0;
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.u16(el) = 0xFFFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.u16(el) = 0xFFFF;
                }
                else
                {
                    Result.s16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMACQ(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_ACCUM[el].W[1] > 0x20)
        {
            if ((RSP_ACCUM[el].W[1] & 0x20) == 0)
            {
                RSP_ACCUM[el].W[1] -= 0x20;
            }
        }
        else if (RSP_ACCUM[el].W[1] < -0x20)
        {
            if ((RSP_ACCUM[el].W[1] & 0x20) == 0)
            {
                RSP_ACCUM[el].W[1] += 0x20;
            }
        }
        temp.W = RSP_ACCUM[el].W[1] >> 1;
        if (temp.HW[1] < 0)
        {
            if (temp.UHW[1] != 0xFFFF)
            {
                Result.u16(el) = 0x8000;
            }
            else
            {
                if (temp.HW[0] >= 0)
                {
                    Result.u16(el) = 0x8000;
                }
                else
                {
                    Result.u16(el) = (uint16_t)(temp.UW & 0xFFF0);
                }
            }
        }
        else
        {
            if (temp.UHW[1] != 0)
            {
                Result.u16(el) = 0x7FF0;
            }
            else
            {
                if (temp.HW[0] < 0)
                {
                    Result.u16(el) = 0x7FF0;
                }
                else
                {
                    Result.u16(el) = (uint16_t)(temp.UW & 0xFFF0);
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMADL(void)
{
    uint8_t el, del;
    UWORD32 temp, temp2;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (uint32_t)RSP_Vect[RSPOpC.rd].u16(el) * (uint32_t)RSP_Vect[RSPOpC.rt].u16(del);
        temp2.UW = temp.UHW[1] + RSP_ACCUM[el].UHW[1];
        RSP_ACCUM[el].HW[1] = temp2.HW[0];
        temp2.UW = RSP_ACCUM[el].UHW[2] + temp2.UHW[1];
        RSP_ACCUM[el].HW[2] = temp2.HW[0];
        RSP_ACCUM[el].HW[3] += temp2.HW[1];
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            if (RSP_ACCUM[el].UHW[3] != 0xFFFF)
            {
                Result.u16(el) = 0;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] >= 0)
                {
                    Result.u16(el) = 0;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].UHW[1];
                }
            }
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.u16(el) = 0xFFFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.u16(el) = 0xFFFF;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].UHW[1];
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMADM(void)
{
    uint8_t el, del;
    UWORD32 temp, temp2;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (uint32_t)((int32_t)RSP_Vect[RSPOpC.rd].s16(el)) * (uint32_t)RSP_Vect[RSPOpC.rt].u16(del);
        temp2.UW = temp.UHW[0] + RSP_ACCUM[el].UHW[1];
        RSP_ACCUM[el].HW[1] = temp2.HW[0];
        temp2.UW = temp.UHW[1] + RSP_ACCUM[el].UHW[2] + temp2.UHW[1];
        RSP_ACCUM[el].HW[2] = temp2.HW[0];
        RSP_ACCUM[el].HW[3] += temp2.HW[1];
        if (temp.W < 0)
        {
            RSP_ACCUM[el].HW[3] -= 1;
        }
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            if (RSP_ACCUM[el].UHW[3] != 0xFFFF)
            {
                Result.u16(el) = 0x8000;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] >= 0)
                {
                    Result.u16(el) = 0x8000;
                }
                else
                {
                    Result.s16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.u16(el) = 0x7FFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.u16(el) = 0x7FFF;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].UHW[2];
                }
            }
        }
        //Result.s16(el) = RSP_ACCUM[el].HW[2];
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMADN(void)
{
    uint8_t el, del;
    UWORD32 temp, temp2;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (uint32_t)RSP_Vect[RSPOpC.rd].u16(el) * (uint32_t)((int32_t)RSP_Vect[RSPOpC.rt].s16(del));
        temp2.UW = temp.UHW[0] + RSP_ACCUM[el].UHW[1];
        RSP_ACCUM[el].HW[1] = temp2.HW[0];
        temp2.UW = temp.UHW[1] + RSP_ACCUM[el].UHW[2] + temp2.UHW[1];
        RSP_ACCUM[el].HW[2] = temp2.HW[0];
        RSP_ACCUM[el].HW[3] += temp2.HW[1];
        if (temp.W < 0)
        {
            RSP_ACCUM[el].HW[3] -= 1;
        }
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            if (RSP_ACCUM[el].UHW[3] != 0xFFFF)
            {
                Result.u16(el) = 0;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] >= 0)
                {
                    Result.u16(el) = 0;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].UHW[1];
                }
            }
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.u16(el) = 0xFFFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.u16(el) = 0xFFFF;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].UHW[1];
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMADH(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        RSP_ACCUM[el].W[1] += (int32_t)RSP_Vect[RSPOpC.rd].s16(el) * (int32_t)RSP_Vect[RSPOpC.rt].s16(del);
        if (RSP_ACCUM[el].HW[3] < 0)
        {
            if (RSP_ACCUM[el].UHW[3] != 0xFFFF)
            {
                Result.u16(el) = 0x8000;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] >= 0)
                {
                    Result.u16(el) = 0x8000;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].HW[2];
                }
            }
        }
        else
        {
            if (RSP_ACCUM[el].UHW[3] != 0)
            {
                Result.u16(el) = 0x7FFF;
            }
            else
            {
                if (RSP_ACCUM[el].HW[2] < 0)
                {
                    Result.u16(el) = 0x7FFF;
                }
                else
                {
                    Result.u16(el) = RSP_ACCUM[el].UHW[2];
                }
            }
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VADD(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.W = (int)RSP_Vect[RSPOpC.rd].s16(el) + (int)RSP_Vect[RSPOpC.rt].s16(del) +
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
                Result.u16(el) = temp.HW[0];
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
    RSP_Vect[RSPOpC.sa] = Result;
    RSP_Flags[0].UW = 0;
}

void RSP_Vector_VSUB(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.W = (int)RSP_Vect[RSPOpC.rd].s16(el) - (int)RSP_Vect[RSPOpC.rt].s16(del) -
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
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VABS(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_Vect[RSPOpC.rd].s16(el) > 0)
        {
            Result.s16(el) = RSP_Vect[RSPOpC.rt].u16(del);
        }
        else if (RSP_Vect[RSPOpC.rd].s16(el) < 0)
        {
            if (RSP_Vect[RSPOpC.rt].u16(del) == 0x8000)
            {
                Result.u16(el) = 0x7FFF;
            }
            else
            {
                Result.u16(el) = RSP_Vect[RSPOpC.rt].s16(del) * -1;
            }
        }
        else
        {
            Result.u16(el) = 0;
        }
        RSP_ACCUM[el].UHW[1] = Result.u16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VADDC(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    RSP_Flags[0].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (int)RSP_Vect[RSPOpC.rd].u16(el) + (int)RSP_Vect[RSPOpC.rt].u16(del);
        RSP_ACCUM[el].HW[1] = temp.HW[0];
        Result.u16(el) = temp.UHW[0];
        if (temp.UW & 0xffff0000)
        {
            RSP_Flags[0].UW |= (1 << (7 - el));
        }
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VSUBC(void)
{
    uint8_t el, del;
    UWORD32 temp;
    RSPVector Result;

    RSP_Flags[0].UW = 0x0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        temp.UW = (int)RSP_Vect[RSPOpC.rd].u16(el) - (int)RSP_Vect[RSPOpC.rt].u16(del);
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
    RSP_Vect[RSPOpC.sa] = Result;
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
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VLT(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_Vect[RSPOpC.rd].s16(el) < RSP_Vect[RSPOpC.rt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.rd].u16(el);
            RSP_Flags[1].UW |= (1 << (7 - el));
        }
        else if (RSP_Vect[RSPOpC.rd].s16(el) != RSP_Vect[RSPOpC.rt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.rt].u16(del);
            RSP_Flags[1].UW &= ~(1 << (7 - el));
        }
        else
        {
            Result.u16(el) = RSP_Vect[RSPOpC.rd].u16(el);
            if ((RSP_Flags[0].UW & (0x101 << (7 - el))) == (WORD)(0x101 << (7 - el)))
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
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VEQ(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_Vect[RSPOpC.rd].u16(el) == RSP_Vect[RSPOpC.rt].u16(del))
        {
            if ((RSP_Flags[0].UW & (1 << (15 - el))) == 0)
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
        }
        Result.u16(el) = RSP_Vect[RSPOpC.rt].u16(del);
        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rt].u16(del);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VNE(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_Vect[RSPOpC.rd].u16(el) != RSP_Vect[RSPOpC.rt].u16(del))
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
        Result.u16(el) = RSP_Vect[RSPOpC.rd].u16(el);
        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].u16(el);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VGE(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[1].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if (RSP_Vect[RSPOpC.rd].s16(el) == RSP_Vect[RSPOpC.rt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.rd].u16(el);
            if ((RSP_Flags[0].UW & (0x101 << (7 - el))) == (WORD)(0x101 << (7 - el)))
            {
                RSP_Flags[1].UW &= ~(1 << (7 - el));
            }
            else
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
        }
        else if (RSP_Vect[RSPOpC.rd].s16(el) > RSP_Vect[RSPOpC.rt].s16(del))
        {
            Result.u16(el) = RSP_Vect[RSPOpC.rd].u16(el);
            RSP_Flags[1].UW |= (1 << (7 - el));
        }
        else
        {
            Result.u16(el) = RSP_Vect[RSPOpC.rt].u16(del);
            RSP_Flags[1].UW &= ~(1 << (7 - el));
        }
        RSP_ACCUM[el].UHW[1] = Result.u16(el);
    }
    RSP_Flags[0].UW = 0;
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VCL(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if ((RSP_Flags[0].UW & (1 << (7 - el))) != 0)
        {
            if ((RSP_Flags[0].UW & (1 << (15 - el))) != 0)
            {
                if ((RSP_Flags[1].UW & (1 << (7 - el))) != 0)
                {
                    RSP_ACCUM[el].HW[1] = -RSP_Vect[RSPOpC.rt].u16(del);
                }
                else
                {
                    RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
                }
            }
            else
            {
                if ((RSP_Flags[2].UW & (1 << (7 - el))))
                {
                    if (RSP_Vect[RSPOpC.rd].u16(el) + RSP_Vect[RSPOpC.rt].u16(del) > 0x10000)
                    {
                        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
                        RSP_Flags[1].UW &= ~(1 << (7 - el));
                    }
                    else
                    {
                        RSP_ACCUM[el].HW[1] = -RSP_Vect[RSPOpC.rt].u16(del);
                        RSP_Flags[1].UW |= (1 << (7 - el));
                    }
                }
                else
                {
                    if (RSP_Vect[RSPOpC.rt].u16(del) + RSP_Vect[RSPOpC.rd].u16(el) != 0)
                    {
                        RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
                        RSP_Flags[1].UW &= ~(1 << (7 - el));
                    }
                    else
                    {
                        RSP_ACCUM[el].HW[1] = -RSP_Vect[RSPOpC.rt].u16(del);
                        RSP_Flags[1].UW |= (1 << (7 - el));
                    }
                }
            }
        }
        else
        {
            if ((RSP_Flags[0].UW & (1 << (15 - el))) != 0)
            {
                if ((RSP_Flags[1].UW & (1 << (15 - el))) != 0)
                {
                    RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rt].s16(del);
                }
                else
                {
                    RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
                }
            }
            else
            {
                if (RSP_Vect[RSPOpC.rd].u16(el) - RSP_Vect[RSPOpC.rt].u16(del) >= 0)
                {
                    RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rt].u16(del);
                    RSP_Flags[1].UW |= (1 << (15 - el));
                }
                else
                {
                    RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
                    RSP_Flags[1].UW &= ~(1 << (15 - el));
                }
            }
        }
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Flags[0].UW = 0;
    RSP_Flags[2].UW = 0;
    RSP_Vect[RSPOpC.sa] = Result;
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
        del = EleSpec[RSPOpC.rs].B[el];

        if ((RSP_Vect[RSPOpC.rd].s16(el) ^ RSP_Vect[RSPOpC.rt].s16(del)) < 0)
        {
            RSP_Flags[0].UW |= (1 << (7 - el));
            if (RSP_Vect[RSPOpC.rt].s16(del) < 0)
            {
                RSP_Flags[1].UW |= (1 << (15 - el));
            }
            if (RSP_Vect[RSPOpC.rd].s16(el) + RSP_Vect[RSPOpC.rt].s16(del) <= 0)
            {
                if (RSP_Vect[RSPOpC.rd].s16(el) + RSP_Vect[RSPOpC.rt].s16(del) == -1)
                {
                    RSP_Flags[2].UW |= (1 << (7 - el));
                }
                RSP_Flags[1].UW |= (1 << (7 - el));
                RSP_ACCUM[el].HW[1] = -RSP_Vect[RSPOpC.rt].u16(del);
            }
            else
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
            }
            if (RSP_Vect[RSPOpC.rd].s16(el) + RSP_Vect[RSPOpC.rt].s16(del) != 0)
            {
                if (RSP_Vect[RSPOpC.rd].s16(el) != ~RSP_Vect[RSPOpC.rt].s16(del))
                {
                    RSP_Flags[0].UW |= (1 << (15 - el));
                }
            }
        }
        else
        {
            if (RSP_Vect[RSPOpC.rt].s16(del) < 0)
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
            if (RSP_Vect[RSPOpC.rd].s16(el) - RSP_Vect[RSPOpC.rt].s16(del) >= 0)
            {
                RSP_Flags[1].UW |= (1 << (15 - el));
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rt].u16(del);
            }
            else
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
            }
            if (RSP_Vect[RSPOpC.rd].s16(el) - RSP_Vect[RSPOpC.rt].s16(del) != 0)
            {
                if (RSP_Vect[RSPOpC.rd].s16(el) != ~RSP_Vect[RSPOpC.rt].s16(del))
                {
                    RSP_Flags[0].UW |= (1 << (15 - el));
                }
            }
        }
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VCR(void)
{
    uint8_t el, del;
    RSPVector Result;

    RSP_Flags[0].UW = 0;
    RSP_Flags[1].UW = 0;
    RSP_Flags[2].UW = 0;
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if ((RSP_Vect[RSPOpC.rd].s16(el) ^ RSP_Vect[RSPOpC.rt].s16(del)) < 0)
        {
            if (RSP_Vect[RSPOpC.rt].s16(del) < 0)
            {
                RSP_Flags[1].UW |= (1 << (15 - el));
            }
            if (RSP_Vect[RSPOpC.rd].s16(el) + RSP_Vect[RSPOpC.rt].s16(del) <= 0)
            {
                RSP_ACCUM[el].HW[1] = ~RSP_Vect[RSPOpC.rt].u16(del);
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
            else
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
            }
        }
        else
        {
            if (RSP_Vect[RSPOpC.rt].s16(del) < 0)
            {
                RSP_Flags[1].UW |= (1 << (7 - el));
            }
            if (RSP_Vect[RSPOpC.rd].s16(el) - RSP_Vect[RSPOpC.rt].s16(del) >= 0)
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rt].u16(del);
                RSP_Flags[1].UW |= (1 << (15 - el));
            }
            else
            {
                RSP_ACCUM[el].HW[1] = RSP_Vect[RSPOpC.rd].s16(el);
            }
        }
        Result.s16(el) = RSP_ACCUM[el].HW[1];
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VMRG(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];

        if ((RSP_Flags[1].UW & (1 << (7 - el))) != 0)
        {
            Result.s16(el) = RSP_Vect[RSPOpC.rd].s16(el);
        }
        else
        {
            Result.s16(el) = RSP_Vect[RSPOpC.rt].s16(del);
        }
        RSP_ACCUM[el].HW[1] = Result.s16(el); // Suggested by Angrylion
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VAND(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        Result.s16(el) = RSP_Vect[RSPOpC.rd].s16(el) & RSP_Vect[RSPOpC.rt].s16(del);
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VNAND(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        Result.s16(el) = ~(RSP_Vect[RSPOpC.rd].s16(el) & RSP_Vect[RSPOpC.rt].s16(del));
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        Result.s16(el) = RSP_Vect[RSPOpC.rd].s16(el) | RSP_Vect[RSPOpC.rt].s16(del);
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VNOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        Result.s16(el) = ~(RSP_Vect[RSPOpC.rd].s16(el) | RSP_Vect[RSPOpC.rt].s16(del));
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VXOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        Result.s16(el) = RSP_Vect[RSPOpC.rd].s16(el) ^ RSP_Vect[RSPOpC.rt].s16(del);
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VNXOR(void)
{
    uint8_t el, del;
    RSPVector Result;

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.rs].B[el];
        Result.s16(el) = ~(RSP_Vect[RSPOpC.rd].s16(el) ^ RSP_Vect[RSPOpC.rt].s16(del));
        RSP_ACCUM[el].HW[1] = Result.s16(el);
    }
    RSP_Vect[RSPOpC.sa] = Result;
}

void RSP_Vector_VRCP(void)
{
    RecpResult.W = RSP_Vect[RSPOpC.rt].s16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]);
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
            DWORD RoundMethod = _RC_CHOP;
            DWORD OldModel = _controlfp(RoundMethod, _MCW_RC);
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
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].s16(7 - (RSPOpC.rd & 0x7)) = RecpResult.UHW[0];
}

void RSP_Vector_VRCPL(void)
{
    int count;
    bool neg;

    RecpResult.UW = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]) | Recp.W;
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
            DWORD OldModel = _controlfp(_RC_CHOP, _MCW_RC);
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
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].s16(7 - (RSPOpC.rd & 0x7)) = RecpResult.UHW[0];
}

void RSP_Vector_VRCPH(void)
{
    int count;

    Recp.UHW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]);
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].u16(7 - (RSPOpC.rd & 0x7)) = RecpResult.UHW[1];
}

void RSP_Vector_VMOV(void)
{
    int count;

    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].u16(7 - (RSPOpC.rd & 0x7)) =
        RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]);
}

void RSP_Vector_VRSQ(void)
{
    int count;
    bool neg;

    SQrootResult.W = RSP_Vect[RSPOpC.rt].s16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]);
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
            DWORD RoundMethod = _RC_CHOP;
            DWORD OldModel = _controlfp(RoundMethod, _MCW_RC);
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
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].s16(7 - (RSPOpC.rd & 0x7)) = SQrootResult.UHW[0];
}

void RSP_Vector_VRSQL(void)
{
    int count;
    bool neg;

    SQrootResult.UW = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]) | SQroot.W;
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
            DWORD OldModel = _controlfp(_RC_CHOP, _MCW_RC);
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
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].s16(7 - (RSPOpC.rd & 0x7)) = SQrootResult.UHW[0];
}

void RSP_Vector_VRSQH(void)
{
    int count;

    SQroot.UHW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)]);
    for (count = 0; count < 8; count++)
    {
        RSP_ACCUM[count].HW[1] = RSP_Vect[RSPOpC.rt].u16(EleSpec[RSPOpC.rs].B[count]);
    }
    RSP_Vect[RSPOpC.sa].u16(7 - (RSPOpC.rd & 0x7)) = SQrootResult.UHW[1];
}

void RSP_Vector_VNOOP(void)
{
}

// LC2 functions

void RSP_Opcode_LBV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 0)) & 0xFFF;
    RSP_LBV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LSV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 1)) & 0xFFF;
    RSP_LSV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LLV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 2)) & 0xFFF;
    RSP_LLV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LDV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    RSP_LDV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LQV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_LQV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LRV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_LRV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LPV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    RSP_LPV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LUV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    RSP_LUV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LHV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_LHV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LFV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_LFV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_LTV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_LTV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

// SC2 functions

void RSP_Opcode_SBV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 0)) & 0xFFF;
    RSP_SBV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SSV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 1)) & 0xFFF;
    RSP_SSV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SLV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 2)) & 0xFFF;
    RSP_SLV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SDV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    RSP_SDV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SQV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_SQV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SRV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_SRV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SPV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    RSP_SPV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SUV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 3)) & 0xFFF;
    RSP_SUV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SHV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_SHV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SFV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_SFV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_STV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_STV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

void RSP_Opcode_SWV(void)
{
    uint32_t Address = (uint32_t)(RSP_GPR[RSPOpC.base].W + (RSPOpC.voffset << 4)) & 0xFFF;
    RSP_SWV_DMEM(Address, RSPOpC.rt, RSPOpC.del);
}

// Other functions

void rsp_UnknownOpcode(void)
{
    char Message[200];
    int response;

    if (InRSPCommandsWindow)
    {
        SetRSPCommandViewto(*PrgCount);
        DisplayError("Unhandled Opcode\n%s\n\nStopping emulation", RSPInstruction(*PrgCount, RSPOpC.Value).NameAndParam().c_str());
    }
    else
    {
        sprintf(Message, "Unhandled Opcode\n%s\n\nStopping emulation.\n\nWould you like to open the debugger?",
            RSPInstruction(*PrgCount, RSPOpC.Value).NameAndParam().c_str());
        response = MessageBoxA(NULL, Message, "Error", MB_YESNO | MB_ICONERROR);
        if (response == IDYES)
        {
            Enter_RSP_Commands_Window();
        }
    }
    ExitThread(0);
}
