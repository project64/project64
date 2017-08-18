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

	bool IsLoadStore()
	{
		return (m_OpCode.op >= R4300i_LDL && m_OpCode.op <= R4300i_SD && m_OpCode.op != R4300i_CACHE);
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

	bool ReadsGPR(int nReg)
	{
		uint32_t op = m_OpCode.op;

		if (op >= R4300i_LDL && op <= R4300i_LWU ||
			op >= R4300i_ADDI && op <= R4300i_XORI ||
			op == R4300i_LD ||
			op == R4300i_BGTZ || op == R4300i_BGTZL ||
			op == R4300i_BLEZ || op == R4300i_BLEZL)
		{
			if (m_OpCode.rs == nReg)
			{
				return true;
			}
		}

		if (op >= R4300i_SB && op <= R4300i_SWR ||
			op >= R4300i_SC && op <= R4300i_SD ||
			op == R4300i_BEQ || op == R4300i_BEQL ||
			op == R4300i_BNE || op == R4300i_BNEL)
		{
			// stores read value and index
			if (m_OpCode.rs == nReg || m_OpCode.rt == nReg)
			{
				return true;
			}
		}

		if (op == R4300i_SPECIAL)
		{
			uint32_t fn = m_OpCode.funct;

			switch (fn)
			{
			case R4300i_SPECIAL_MTLO:
			case R4300i_SPECIAL_MTHI:
			case R4300i_SPECIAL_JR:
			case R4300i_SPECIAL_JALR:
				if (m_OpCode.rs == nReg)
				{
					return true;
				}
				break;
			case R4300i_SPECIAL_SLL:
			case R4300i_SPECIAL_SRL:
			case R4300i_SPECIAL_SRA:
				if (m_OpCode.rt == nReg)
				{
					return true;
				}
				break;
			}

			if (fn >= R4300i_SPECIAL_SLLV && fn <= R4300i_SPECIAL_SRAV ||
				fn >= R4300i_SPECIAL_DSLLV && fn <= R4300i_SPECIAL_DSRAV ||
				fn >= R4300i_SPECIAL_DIVU && fn <= R4300i_SPECIAL_DSUBU)
			{
				// two register operands
				if (m_OpCode.rt == nReg || m_OpCode.rs == nReg)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool WritesGPR(int nReg)
	{
		uint32_t op = m_OpCode.op;

		if (op >= R4300i_LDL && op <= R4300i_LWU ||
			op >= R4300i_ADDI && op <= R4300i_XORI ||
			op == R4300i_LUI || op == R4300i_LD)
		{
			// loads write value
			if (m_OpCode.rt == nReg)
			{
				return true;
			}
		}

		if (op == R4300i_JAL)
		{
			if (nReg == 31) // RA
			{
				return true;
			}
		}

		if (op == R4300i_SPECIAL)
		{
			uint32_t fn = m_OpCode.funct;

			switch (fn)
			{
			case R4300i_SPECIAL_MFLO:
			case R4300i_SPECIAL_MFHI:
			case R4300i_SPECIAL_SLL:
			case R4300i_SPECIAL_SRL:
			case R4300i_SPECIAL_SRA:
				if (m_OpCode.rd == nReg)
				{
					return true;
				}
				break;
			}

			if (fn >= R4300i_SPECIAL_SLLV && fn <= R4300i_SPECIAL_SRAV ||
				fn >= R4300i_SPECIAL_DSLLV && fn <= R4300i_SPECIAL_DSRAV ||
				fn >= R4300i_SPECIAL_DIVU && fn <= R4300i_SPECIAL_DSUBU ||
				fn == R4300i_SPECIAL_JALR)
			{
				// result register
				if (m_OpCode.rd == nReg)
				{
					return true;
				}
			}
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

};