#include "stdafx.h"
#include "MIPSInterfaceHandler.h"
#include <Project64-core\N64System\SystemGlobals.h>
#include <Project64-core\N64System\Mips\Register.h>

MIPSInterfaceReg::MIPSInterfaceReg(uint32_t * MipsInterface) :
    MI_INIT_MODE_REG(MipsInterface[0]),
    MI_MODE_REG(MipsInterface[0]),
    MI_VERSION_REG(MipsInterface[1]),
    MI_NOOP_REG(MipsInterface[1]),
    MI_INTR_REG(MipsInterface[2]),
    MI_INTR_MASK_REG(MipsInterface[3])
{
}

MIPSInterfaceHandler::MIPSInterfaceHandler(CRegisters & Reg) :
    MIPSInterfaceReg(Reg.m_Mips_Interface),
    m_Reg(Reg),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool MIPSInterfaceHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04300000: Value = MI_MODE_REG; break;
    case 0x04300004: Value = MI_VERSION_REG; break;
    case 0x04300008: Value = MI_INTR_REG; break;
    case 0x0430000C: Value = MI_INTR_MASK_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (LogMIPSInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04300000: LogMessage("%08X: read from MI_INIT_MODE_REG/MI_MODE_REG (%08X)", m_PC, Value); break;
        case 0x04300004: LogMessage("%08X: read from MI_VERSION_REG/MI_NOOP_REG (%08X)", m_PC, Value); break;
        case 0x04300008: LogMessage("%08X: read from MI_INTR_REG (%08X)", m_PC, Value); break;
        case 0x0430000C: LogMessage("%08X: read from MI_INTR_MASK_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool MIPSInterfaceHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (LogMIPSInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04300000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to MI_INIT_MODE_REG/MI_MODE_REG", m_PC, Value, Mask); break;
        case 0x04300004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to MI_VERSION_REG/MI_NOOP_REG", m_PC, Value, Mask); break;
        case 0x04300008: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to MI_INTR_REG", m_PC, Value, Mask); break;
        case 0x0430000C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to MI_INTR_MASK_REG", m_PC, Value, Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    uint32_t MaskedValue = Value & Mask;
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04300000:
        MI_MODE_REG &= ~0x7F;
        MI_MODE_REG |= (MaskedValue & 0x7F);
        if ((MaskedValue & MI_CLR_INIT) != 0)
        {
            MI_MODE_REG &= ~MI_MODE_INIT;
        }
        if ((MaskedValue & MI_SET_INIT) != 0)
        {
            MI_MODE_REG |= MI_MODE_INIT;
        }
        if ((MaskedValue & MI_CLR_EBUS) != 0)
        {
            MI_MODE_REG &= ~MI_MODE_EBUS;
        }
        if ((MaskedValue & MI_SET_EBUS) != 0)
        {
            MI_MODE_REG |= MI_MODE_EBUS;
        }
        if ((MaskedValue & MI_CLR_DP_INTR) != 0)
        {
            MI_INTR_REG &= ~MI_INTR_DP;
            m_Reg.m_GfxIntrReg &= ~MI_INTR_DP;
            m_Reg.CheckInterrupts();
        }
        if ((MaskedValue & MI_CLR_RDRAM) != 0)
        {
            MI_MODE_REG &= ~MI_MODE_RDRAM;
        }
        if ((MaskedValue & MI_SET_RDRAM) != 0)
        {
            MI_MODE_REG |= MI_MODE_RDRAM;
        }
        break;
    case 0x0430000C:
        if ((MaskedValue & MI_INTR_MASK_CLR_SP) != 0)
        {
            MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP;
        }
        if ((MaskedValue & MI_INTR_MASK_SET_SP) != 0)
        {
            MI_INTR_MASK_REG |= MI_INTR_MASK_SP;
        }
        if ((MaskedValue & MI_INTR_MASK_CLR_SI) != 0)
        {
            MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI;
        }
        if ((MaskedValue & MI_INTR_MASK_SET_SI) != 0)
        {
            MI_INTR_MASK_REG |= MI_INTR_MASK_SI;
        }
        if ((MaskedValue & MI_INTR_MASK_CLR_AI) != 0)
        {
            MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI;
        }
        if ((MaskedValue & MI_INTR_MASK_SET_AI) != 0)
        {
            MI_INTR_MASK_REG |= MI_INTR_MASK_AI;
        }
        if ((MaskedValue & MI_INTR_MASK_CLR_VI) != 0)
        {
            MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI;
        }
        if ((MaskedValue & MI_INTR_MASK_SET_VI) != 0)
        {
            MI_INTR_MASK_REG |= MI_INTR_MASK_VI;
        }
        if ((MaskedValue & MI_INTR_MASK_CLR_PI) != 0)
        {
            MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI;
        }
        if ((MaskedValue & MI_INTR_MASK_SET_PI) != 0)
        {
            MI_INTR_MASK_REG |= MI_INTR_MASK_PI;
        }
        if ((MaskedValue & MI_INTR_MASK_CLR_DP) != 0)
        {
            MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP;
        }
        if ((MaskedValue & MI_INTR_MASK_SET_DP) != 0)
        {
            MI_INTR_MASK_REG |= MI_INTR_MASK_DP;
        }
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}