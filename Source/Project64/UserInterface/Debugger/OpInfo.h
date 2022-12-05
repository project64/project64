#pragma once

#include <Project64-core/N64System/Mips/R4300iOpcode.h>

class COpInfo
{
public:
    R4300iOpcode m_OpCode;

    COpInfo()
    {
    }

    inline COpInfo(R4300iOpcode opcode) :
        m_OpCode(opcode)
    {
    }

    bool IsStaticJump()
    {
        // J, JAL

        uint32_t op = m_OpCode.op;

        if (op == R4300i_J || op == R4300i_JAL)
        {
            return true;
        }

        return false;
    }

    bool IsJump()
    {
        // J, JAL, JR, JALR, exception

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
        if (m_OpCode.Value == 0)
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
        return (m_OpCode.op >= R4300i_LDL && m_OpCode.op <= R4300i_LWU || m_OpCode.op >= R4300i_LL && m_OpCode.op <= R4300i_LD);
    }

    inline bool IsStoreCommand()
    {
        return (m_OpCode.op >= R4300i_SB && m_OpCode.op <= R4300i_SWR || m_OpCode.op >= R4300i_SC && m_OpCode.op <= R4300i_SD);
    }

    // ADDIU SP, SP, X
    bool IsStackShift()
    {
        return (m_OpCode.op == R4300i_ADDIU || m_OpCode.op == R4300i_ADDI) && m_OpCode.rt == 29;
    }

    // ADDIU SP, SP, <negative value>
    bool IsStackAlloc()
    {
        if (!IsStackShift())
        {
            return false;
        }

        return (short)m_OpCode.immediate < 0;
    }

    // ADDIU SP, SP, <positive value>
    bool IsStackFree()
    {
        if (!IsStackShift())
        {
            return false;
        }

        return (short)m_OpCode.immediate > 0;
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
