/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <stdafx.h>

#include <Project64-core\N64System\Mips\OpCode.h>

class COpInfo
{
public:

    OPCODE m_OpCode;

    COpInfo()
    {
    }

    inline COpInfo(OPCODE opcode):
        m_OpCode(opcode)
    {
    }

    bool IsStaticJump()
    {
        // j, jal

        uint32_t op = m_OpCode.op;

        if (op == R4300i_J || op == R4300i_JAL)
        {
            return true;
        }

        return false;
    }

    bool IsJump()
    {
        // j, jal, jr, jalr, exception

        uint32_t op = m_OpCode.op;

        if (op == R4300i_J || op == R4300i_JAL)
        {
            return true;
        }

        if (op == R4300i_SPECIAL)
        {
            uint32_t fn = m_OpCode.funct;

            if (fn >= R4300i_SPECIAL_JR && fn <= R4300i_SPECIAL_BREAK)
            {
                return true;
            }
        }

        if (op == R4300i_REGIMM)
        {
            uint32_t rt = m_OpCode.rt;

            if (rt >= R4300i_REGIMM_TGEI && rt <= R4300i_REGIMM_TNEI)
            {
                return true;
            }
        }

        if (op == R4300i_CP0)
        {
            if ((m_OpCode.rs & 0x10) != 0)
            {
                uint32_t fn = m_OpCode.funct;
                if (fn == R4300i_COP0_CO_ERET)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool IsJAL()
    {
        return (m_OpCode.op == R4300i_JAL || (m_OpCode.op == R4300i_SPECIAL && m_OpCode.funct == R4300i_SPECIAL_JALR));
    }

    bool IsBranch()
    {
        uint32_t op = m_OpCode.op;

        if (op >= R4300i_BEQ && op <= R4300i_BGTZ)
        {
            return true;
        }

        if (op >= R4300i_BEQL && op <= R4300i_BGTZL)
        {
            return true;
        }

        if (op == R4300i_REGIMM)
        {
            uint32_t rt = m_OpCode.rt;

            if (rt >= R4300i_REGIMM_BLTZ && rt <= R4300i_REGIMM_BGEZL)
            {
                return true;
            }

            if (rt >= R4300i_REGIMM_BLTZAL && rt <= R4300i_REGIMM_BGEZALL)
            {
                return true;
            }
        }

        if (op == R4300i_CP1 && m_OpCode.fmt == R4300i_COP1_BC)
        {
            return true;
        }

        return false;
    }

    bool IsNOP()
    {
        if (m_OpCode.Hex == 0)
        {
            return true;
        }
        return false;
    }

    inline bool IsLoadStoreCommand()
    {
        return (m_OpCode.op >= R4300i_LDL && m_OpCode.op <= R4300i_SD && m_OpCode.op != R4300i_CACHE);
    }

    inline bool IsLoadCommand()
    {
        return (m_OpCode.op <= R4300i_LWU || (m_OpCode.op >= R4300i_LL && m_OpCode.op <= R4300i_LD));
    }

    inline bool IsStoreCommand()
    {
        return (m_OpCode.op >= R4300i_SB && m_OpCode.op <= R4300i_SWR ||
                m_OpCode.op >= R4300i_SC && m_OpCode.op <= R4300i_SD);
    }

    bool IsStackShift()
    {
        return (m_OpCode.op == R4300i_ADDIU || m_OpCode.op == R4300i_ADDI) && m_OpCode.rt == 29;
    }

    bool IsStackAlloc()
    {
        if (!IsStackShift())
        {
            return false;
        }

        return (short)m_OpCode.immediate < 0;
    }

    bool IsStackFree()
    {
        if (!IsStackShift())
        {
            return false;
        }

        return (short)m_OpCode.immediate > 0;
    }

    bool ReadsGPR(unsigned int nReg)
    {
        uint32_t op = m_OpCode.op;

        if (op == R4300i_SPECIAL)
        {
            uint32_t fn = m_OpCode.funct;

            if (m_OpCode.rs == nReg || m_OpCode.rt == nReg)
            {
                if (fn >= R4300i_SPECIAL_SLLV && fn <= R4300i_SPECIAL_SRAV ||
                    fn >= R4300i_SPECIAL_DSLLV && fn <= R4300i_SPECIAL_DSRAV ||
                    fn >= R4300i_SPECIAL_MULT && fn <= R4300i_SPECIAL_TNE)
                {
                    return true;
                }

                if (m_OpCode.rs == nReg)
                {
                    if (fn == R4300i_SPECIAL_MTLO || fn == R4300i_SPECIAL_MTHI ||
                        fn == R4300i_SPECIAL_JR || fn == R4300i_SPECIAL_JALR)
                    {
                        return true;
                    }
                }

                if (m_OpCode.rt == nReg)
                {
                    if (fn >= R4300i_SPECIAL_SLL && fn <= R4300i_SPECIAL_SRA ||
                        fn >= R4300i_SPECIAL_DSLL && fn <= R4300i_SPECIAL_DSRA32)
                    {
                        return true;
                    }
                }
            }
        }
        else
        {
            if (m_OpCode.rs == nReg || m_OpCode.rt == nReg)
            {
                if (op >= R4300i_SB && op <= R4300i_SWR ||
                    op >= R4300i_SC && op <= R4300i_SD ||
                    op == R4300i_BEQ || op == R4300i_BEQL ||
                    op == R4300i_BNE || op == R4300i_BNEL)
                {
                    return true;
                }

                if (m_OpCode.rs == nReg)
                {
                    if (op == R4300i_REGIMM)
                    {
                        return true;
                    }

                    if (op >= R4300i_BLEZL && op <= R4300i_LWU ||
                        op >= R4300i_BLEZ && op <= R4300i_XORI ||
                        op >= R4300i_CACHE && op <= R4300i_LD ||
                        op == R4300i_SWC1 || op == R4300i_SDC1)
                    {
                        return true;
                    }
                }

                if (m_OpCode.rt == nReg)
                {
                    if (op == R4300i_CP0 && m_OpCode.fmt == R4300i_COP0_MT)
                    {
                        return true;
                    }

                    if (op == R4300i_CP1)
                    {
                        if (m_OpCode.fmt == R4300i_COP1_MT ||
                            m_OpCode.fmt == R4300i_COP1_DMT ||
                            m_OpCode.fmt == R4300i_COP1_CT)
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool WritesGPR(unsigned int nReg)
    {
        uint32_t op = m_OpCode.op;

        if (op == R4300i_SPECIAL)
        {
            if (m_OpCode.rd == nReg)
            {
                uint32_t fn = m_OpCode.funct;

                if (fn >= R4300i_SPECIAL_SLL && fn <= R4300i_SPECIAL_SRAV ||
                    fn >= R4300i_SPECIAL_DSLLV && fn <= R4300i_SPECIAL_DSRAV ||
                    fn >= R4300i_SPECIAL_DIVU && fn <= R4300i_SPECIAL_DSUBU ||
                    fn >= R4300i_SPECIAL_DSLL && fn <= R4300i_SPECIAL_DSRA32 ||
                    fn == R4300i_SPECIAL_JALR || fn == R4300i_SPECIAL_MFLO ||
                    fn == R4300i_SPECIAL_MFHI)
                {
                    return true;
                }
            }
        }
        else
        {
            if (m_OpCode.rt == nReg)
            {
                if (op >= R4300i_DADDI && op <= R4300i_LWU ||
                    op >= R4300i_ADDI && op <= R4300i_LUI ||
                    op == R4300i_LL || op == R4300i_LD ||
                    (op == R4300i_CP0 && m_OpCode.fmt == R4300i_COP0_MF) ||
                    (op == R4300i_CP1 && m_OpCode.fmt == R4300i_COP1_MF) ||
                    (op == R4300i_CP1 && m_OpCode.fmt == R4300i_COP1_CF))
                {
                    return true;
                }
            }
        }

        if (op == R4300i_JAL && nReg == 31) // nReg == RA
        {
            return true;
        }

        return false;
    }

    bool ReadsLO()
    {
        if (m_OpCode.op == R4300i_SPECIAL && m_OpCode.funct == R4300i_SPECIAL_MFLO)
        {
            return true;
        }
        return false;
    }

    bool WritesLO()
    {
        if (m_OpCode.op == R4300i_SPECIAL && m_OpCode.funct == R4300i_SPECIAL_MTLO)
        {
            return true;
        }
        return false;
    }

    // todo add mult, div etc

    bool ReadsHI()
    {
        if (m_OpCode.op == R4300i_SPECIAL && m_OpCode.funct == R4300i_SPECIAL_MFHI)
        {
            return true;
        }
        return false;
    }

    bool WritesHI()
    {
        if (m_OpCode.op == R4300i_SPECIAL && m_OpCode.funct == R4300i_SPECIAL_MTHI)
        {
            return true;
        }
        return false;
    }

    inline uint32_t GetLoadStoreAddress()
    {
        return g_Reg->m_GPR[m_OpCode.base].UW[0] + (int16_t)m_OpCode.offset;
    }

    inline uint32_t GetStoreValueUnsigned()
    {
        return g_Reg->m_GPR[m_OpCode.rt].UW[0];
    }

    inline int NumBytesToLoad()
    {
        switch (m_OpCode.op)
        {
        case R4300i_LB:
        case R4300i_LBU:
            return 1;
        case R4300i_LH:
        case R4300i_LHU:
            return 2;
        case R4300i_LD:
        case R4300i_LDL:
        case R4300i_LDR:
        case R4300i_LDC1:
            return 8;
        default:
            return 4;
        }
    }

    inline int NumBytesToStore()
    {
        switch (m_OpCode.op)
        {
        case R4300i_SB:
            return 1;
        case R4300i_SH:
            return 2;
        case R4300i_SD:
        case R4300i_SDL:
        case R4300i_SDR:
        case R4300i_SDC1:
        case R4300i_SDC2:
            return 8;
        default:
            return 4;
        }
    }
};