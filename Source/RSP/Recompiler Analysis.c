#include <windows.h>
#include "Rsp.h"
#include "CPU.h"
#include "Interpreter CPU.h"
#include "Recompiler CPU.h"
#include "RSP Command.h"
#include "memory.h"
#include "opcode.h"
#include "log.h"
#include "Types.h"

//#define COMPARE_INSTRUCTIONS_VERBOSE

/************************************************************
** IsOpcodeNop
**
** Output: Boolean whether opcode at PC is a NOP
** Input: PC
*************************************************************/

Boolean IsOpcodeNop(DWORD PC)
{
	OPCODE RspOp;
	RSP_LW_IMEM(PC, &RspOp.Hex);

	if (RspOp.op == RSP_SPECIAL && RspOp.funct == RSP_SPECIAL_SLL) {
		return (RspOp.rd == 0) ? TRUE : FALSE;
	}

	return FALSE;
}

/************************************************************
** IsNextInstructionMmx
**
** Output: determines EMMS status
** Input: PC
*************************************************************/

Boolean IsNextInstructionMmx(DWORD PC)
{
	OPCODE RspOp;
	
	if (IsMmxEnabled == FALSE)
		return FALSE;

	PC += 4;
	if (PC >= 0x1000) return FALSE;
	RSP_LW_IMEM(PC, &RspOp.Hex);

	if (RspOp.op != RSP_CP2)
		return FALSE;

	if ((RspOp.rs & 0x10) != 0) {
		switch (RspOp.funct) {
		case RSP_VECTOR_VMULF:
		case RSP_VECTOR_VMUDL: // Warning: Not all handled?
		case RSP_VECTOR_VMUDM:
		case RSP_VECTOR_VMUDN:
		case RSP_VECTOR_VMUDH:
			if (TRUE == WriteToAccum(7, PC)) {
				return FALSE;
			} else if ((RspOp.rs & 0x0f) >= 2 && (RspOp.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE) {
				return FALSE;
			} else 
				return TRUE;

		case RSP_VECTOR_VABS:
		case RSP_VECTOR_VAND:
		case RSP_VECTOR_VOR:
		case RSP_VECTOR_VXOR:
		case RSP_VECTOR_VNAND:
		case RSP_VECTOR_VNOR:
		case RSP_VECTOR_VNXOR:
			if (TRUE == WriteToAccum(Low16BitAccum, PC)) {
				return FALSE;
			} else if ((RspOp.rs & 0x0f) >= 2 && (RspOp.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE) {
				return FALSE;
			} else
				return TRUE;

		case RSP_VECTOR_VADD:
		case RSP_VECTOR_VSUB:
			// Requires no accumulator write, and no flags!
			if (WriteToAccum(Low16BitAccum, PC) == TRUE) {
				return FALSE;
			} else if (UseRspFlags(PC) == TRUE) {
				return FALSE;
			} else if ((RspOp.rs & 0x0f) >= 2 && (RspOp.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE) {
				return FALSE;
			} else
				return TRUE;

		default:
			return FALSE;
		}
	} else 
		return FALSE;
}

/************************************************************
** WriteToAccum2
**
** Output:
**	TRUE: Accumulation series
**	FALSE: Accumulator is reset after this op
**
** Input: PC, location in accumulator
*************************************************************/

#define HIT_BRANCH	0x2

DWORD WriteToAccum2(int Location, int PC, Boolean RecursiveCall)
{
	OPCODE RspOp;
	DWORD BranchTarget = 0;
	signed int BranchImmed = 0;
	int Instruction_State = NextInstruction;

	if (Compiler.bAccum == FALSE) return TRUE;

	if (Instruction_State == DELAY_SLOT) { 
		return TRUE;
	}

	do {
		PC += 4;
		if (PC >= 0x1000) { return TRUE; }
		RSP_LW_IMEM(PC, &RspOp.Hex);

		switch (RspOp.op) {
 		case RSP_REGIMM:
			switch (RspOp.rt) {
			case RSP_REGIMM_BLTZ:
			case RSP_REGIMM_BGEZ:
			case RSP_REGIMM_BLTZAL:
			case RSP_REGIMM_BGEZAL:
				Instruction_State = DO_DELAY_SLOT;
				break;
			default:
				CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_SPECIAL:
			switch (RspOp.funct) {
			case RSP_SPECIAL_SLL:
			case RSP_SPECIAL_SRL:
			case RSP_SPECIAL_SRA:
			case RSP_SPECIAL_SLLV:
			case RSP_SPECIAL_SRLV:
			case RSP_SPECIAL_SRAV:
			case RSP_SPECIAL_ADD:
			case RSP_SPECIAL_ADDU:
			case RSP_SPECIAL_SUB:
			case RSP_SPECIAL_SUBU:
			case RSP_SPECIAL_AND:
			case RSP_SPECIAL_OR:
			case RSP_SPECIAL_XOR:
			case RSP_SPECIAL_NOR:
			case RSP_SPECIAL_SLT:
			case RSP_SPECIAL_SLTU:
			case RSP_SPECIAL_BREAK:
				break;
				
			case RSP_SPECIAL_JALR:
				return TRUE;

			case RSP_SPECIAL_JR:
				Instruction_State = DO_DELAY_SLOT;
				break;

			default:
				CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_J:
			// There is no way a loopback is going to use accumulator
			if (Compiler.bAudioUcode && (((int)(RspOp.target << 2) & 0xFFC) < PC)) {
				return FALSE;
			}
			// Rarely occurs let them have their way
			else {
				Instruction_State = DO_DELAY_SLOT;
				break;
			}

		case RSP_JAL:
			// There is no way calling a subroutine is going to use an accumulator
			// or come back and continue an existing calculation
			if(Compiler.bAudioUcode) {
				break;
			} else {
				Instruction_State = DO_DELAY_SLOT;
				break;
			}

		case RSP_BEQ:
		case RSP_BNE:
		case RSP_BLEZ:
		case RSP_BGTZ:			
			BranchImmed = (short)RspOp.offset;
			if (Compiler.bAudioUcode) {
				OPCODE NextOp;

				// Ignore backward branches and pretend it's a NOP
				if (BranchImmed <= 0) {
					break;
				}
				// If the opcode (which is 8 bytes before the destination and also a J backward) then ignore this
				BranchImmed = (PC + ((short)RspOp.offset << 2) + 4) & 0xFFC;
				RSP_LW_IMEM(BranchImmed - 8, &NextOp.Hex);
				if (RspOp.op == RSP_J && (int)(RspOp.target << 2) < PC) {
					break;
				}
			}
			BranchTarget = (PC + ((short)RspOp.offset << 2) + 4) & 0xFFC;
			Instruction_State = DO_DELAY_SLOT;
			break;
		case RSP_ADDI:
		case RSP_ADDIU:
		case RSP_SLTI:
		case RSP_SLTIU:
		case RSP_ANDI:
		case RSP_ORI:
		case RSP_XORI:
		case RSP_LUI:
		case RSP_CP0:
			break;

		case RSP_CP2:
			if ((RspOp.rs & 0x10) != 0) {
				switch (RspOp.funct) {
				case RSP_VECTOR_VMULF:
				case RSP_VECTOR_VMULU:
				case RSP_VECTOR_VMUDL:
				case RSP_VECTOR_VMUDM:
				case RSP_VECTOR_VMUDN:
				case RSP_VECTOR_VMUDH:
					return FALSE;
				case RSP_VECTOR_VMACF:
				case RSP_VECTOR_VMACU:
				case RSP_VECTOR_VMADL:
				case RSP_VECTOR_VMADM:
				case RSP_VECTOR_VMADN:
					return TRUE;
				case RSP_VECTOR_VMADH:
					if (Location == Low16BitAccum) { break; }
					return TRUE;

				case RSP_VECTOR_VABS:
				case RSP_VECTOR_VADD:
				case RSP_VECTOR_VADDC:
				case RSP_VECTOR_VSUB:
				case RSP_VECTOR_VSUBC:
				case RSP_VECTOR_VAND:
				case RSP_VECTOR_VNAND:
				case RSP_VECTOR_VOR:
				case RSP_VECTOR_VNOR:
				case RSP_VECTOR_VXOR:
				case RSP_VECTOR_VNXOR:
					// Since these modify the accumulator lower-16 bits we can
					// safely assume these 'reset' the accumulator no matter what
					//			return FALSE;
				case RSP_VECTOR_VCR:
				case RSP_VECTOR_VCH:
				case RSP_VECTOR_VCL:
				case RSP_VECTOR_VRCP:
				case RSP_VECTOR_VRCPL:
				case RSP_VECTOR_VRCPH:
				case RSP_VECTOR_VRSQL:
				case RSP_VECTOR_VRSQH:
				case RSP_VECTOR_VLT:
				case RSP_VECTOR_VEQ:
				case RSP_VECTOR_VGE:
				case RSP_VECTOR_VNE:
				case RSP_VECTOR_VMRG:
				case RSP_VECTOR_VMOV:
					if (Location == Low16BitAccum) { return FALSE; }
					break;

				case RSP_VECTOR_VSAW:
					return TRUE;
				default:
					CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
					return TRUE;
				}
			} else {
				switch (RspOp.rs) {				
				case RSP_COP2_CF:
				case RSP_COP2_CT:
				case RSP_COP2_MT:
				case RSP_COP2_MF:
					break;
				default:
					CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
					return TRUE;
				}
			}
			break;
		case RSP_LB:
		case RSP_LH:
		case RSP_LW:
		case RSP_LBU:
		case RSP_LHU:
		case RSP_SB:
		case RSP_SH:
		case RSP_SW:
			break;
		case RSP_LC2:
			switch (RspOp.rd) {
			case RSP_LSC2_BV:
			case RSP_LSC2_SV:
			case RSP_LSC2_DV:
			case RSP_LSC2_RV:
			case RSP_LSC2_QV:
			case RSP_LSC2_LV:
			case RSP_LSC2_UV:
			case RSP_LSC2_PV:
			case RSP_LSC2_TV:
			case RSP_LSC2_HV:
				break;
			default:
				CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_SC2:
			switch (RspOp.rd) {
			case RSP_LSC2_BV:
			case RSP_LSC2_SV:
			case RSP_LSC2_LV:
			case RSP_LSC2_DV:
			case RSP_LSC2_QV:
			case RSP_LSC2_RV:
			case RSP_LSC2_PV:
			case RSP_LSC2_UV:
			case RSP_LSC2_HV:
			case RSP_LSC2_FV:
			case RSP_LSC2_WV:
			case RSP_LSC2_TV:
				break;
			default:
				CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		default:
			CompilerWarning("Unknown opcode in WriteToAccum\n%s",RSPOpcodeName(RspOp.Hex,PC));
			return TRUE;
		}
		switch (Instruction_State) {
		case NORMAL: break;
		case DO_DELAY_SLOT: 
			Instruction_State = DELAY_SLOT;
			break;
		case DELAY_SLOT: 
			Instruction_State = FINISH_BLOCK;
			break;
		}
	} while (Instruction_State != FINISH_BLOCK);

	/*
	This is a tricky situation because most of the 
	microcode does loops, so looping back and checking
	can prove effective, but it's still a branch...
	*/

	if (BranchTarget != 0 && RecursiveCall == FALSE) {
		DWORD BranchTaken, BranchFall;

		// Analysis of branch taken
		BranchTaken = WriteToAccum2(Location, BranchTarget - 4, TRUE);
		// Analysis of branch as NOP
		BranchFall = WriteToAccum2(Location, PC, TRUE);

		if (BranchImmed < 0) {
			if (BranchTaken != FALSE) {

				// took this back branch and found a place
				// that needs this vector as a source

				return TRUE;
			} else if (BranchFall == HIT_BRANCH) {
				return TRUE;
			}
			// Otherwise this is completely valid
			return BranchFall;
		} else {	
			if (BranchFall != FALSE) {
				
				// Took this forward branch and found a place
				// that needs this vector as a source
				
				return TRUE;
			} else if (BranchTaken == HIT_BRANCH) {
				return TRUE;
			}
			// Otherwise this is completely valid
			return BranchTaken;
		} 
	}
	return TRUE;
}

Boolean WriteToAccum(int Location, int PC)
{
	DWORD value = WriteToAccum2(Location, PC, FALSE);

	if (value == HIT_BRANCH) {
		return TRUE; /* ??? */
	} else
		return value;
}

/************************************************************
** WriteToVectorDest
**
** Output:
**	TRUE: Destination is used as a source later
**	FALSE: Destination is over-written later
** Input: PC, Register
*************************************************************/

Boolean WriteToVectorDest2(DWORD DestReg, int PC, Boolean RecursiveCall)
{
	OPCODE RspOp;
	DWORD BranchTarget = 0;
	signed int BranchImmed = 0;

	int Instruction_State = NextInstruction;

	if (Compiler.bDest == FALSE) return TRUE;

	if (Instruction_State == DELAY_SLOT) { 
		return TRUE;
	}
	
	do {
		PC += 4;
		if (PC >= 0x1000) { return TRUE; }
		RSP_LW_IMEM(PC, &RspOp.Hex);

		switch (RspOp.op) {

		case RSP_REGIMM:
			switch (RspOp.rt) {
			case RSP_REGIMM_BLTZ:
			case RSP_REGIMM_BGEZ:
			case RSP_REGIMM_BLTZAL:
			case RSP_REGIMM_BGEZAL:
				Instruction_State = DO_DELAY_SLOT;
				break;
			default:
				CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_SPECIAL:
			switch (RspOp.funct) {
			case RSP_SPECIAL_SLL:
			case RSP_SPECIAL_SRL:
			case RSP_SPECIAL_SRA:
			case RSP_SPECIAL_SLLV:
			case RSP_SPECIAL_SRLV:
			case RSP_SPECIAL_SRAV:
			case RSP_SPECIAL_ADD:
			case RSP_SPECIAL_ADDU:
			case RSP_SPECIAL_SUB:
			case RSP_SPECIAL_SUBU:
			case RSP_SPECIAL_AND:
			case RSP_SPECIAL_OR:
			case RSP_SPECIAL_XOR:
			case RSP_SPECIAL_NOR:
			case RSP_SPECIAL_SLT:
			case RSP_SPECIAL_SLTU:
			case RSP_SPECIAL_BREAK:
				break;

			case RSP_SPECIAL_JALR:
				return TRUE;
				
			case RSP_SPECIAL_JR:
				Instruction_State = DO_DELAY_SLOT;
				break;

			default:
				CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_J:
			// There is no way a loopback is going to use accumulator
			if (Compiler.bAudioUcode && (int)(RspOp.target << 2) < PC) {
				return FALSE;
			}
			// Rarely occurs let them have their way
			return TRUE;

		case RSP_JAL:
			// Assume register is being passed to function or used after the function call
			return TRUE;

		case RSP_BEQ:
		case RSP_BNE:
		case RSP_BLEZ:
		case RSP_BGTZ:			
			BranchImmed = (short)RspOp.offset;
			if (Compiler.bAudioUcode) {
				OPCODE NextOp;

				// Ignore backward branches and pretend it's a NOP
				if (BranchImmed <= 0) {
					break;
				}
				// If the opcode (which is 8 bytes before the destination and also a J backward) then ignore this
				BranchImmed = (PC + ((short)RspOp.offset << 2) + 4) & 0xFFC;
				RSP_LW_IMEM(BranchImmed - 8, &NextOp.Hex);
				if (RspOp.op == RSP_J && (int)(RspOp.target << 2) < PC) {
					break;
				}
			}
			BranchTarget = (PC + ((short)RspOp.offset << 2) + 4) & 0xFFC;
			Instruction_State = DO_DELAY_SLOT;
			break;
		case RSP_ADDI:
		case RSP_ADDIU:
		case RSP_SLTI:
		case RSP_SLTIU:
		case RSP_ANDI:
		case RSP_ORI:
		case RSP_XORI:
		case RSP_LUI:
		case RSP_CP0:			
			break;

		case RSP_CP2:
			if ((RspOp.rs & 0x10) != 0) {
				switch (RspOp.funct) {
				case RSP_VECTOR_VMULF:
				case RSP_VECTOR_VMULU:
				case RSP_VECTOR_VMUDL:
				case RSP_VECTOR_VMUDM:
				case RSP_VECTOR_VMUDN:
				case RSP_VECTOR_VMUDH:
				case RSP_VECTOR_VMACF:
				case RSP_VECTOR_VMACU:
				case RSP_VECTOR_VMADL:
				case RSP_VECTOR_VMADM:
				case RSP_VECTOR_VMADN:
				case RSP_VECTOR_VMADH:
				case RSP_VECTOR_VADD:
				case RSP_VECTOR_VADDC:
				case RSP_VECTOR_VSUB:
				case RSP_VECTOR_VSUBC:
				case RSP_VECTOR_VAND:
				case RSP_VECTOR_VNAND:
				case RSP_VECTOR_VOR:
				case RSP_VECTOR_VNOR:
				case RSP_VECTOR_VXOR:
				case RSP_VECTOR_VNXOR:
				case RSP_VECTOR_VABS:
					if (DestReg == RspOp.rd) { return TRUE; }
					if (DestReg == RspOp.rt) { return TRUE; }
					if (DestReg == RspOp.sa) { return FALSE; }
					break;

				case RSP_VECTOR_VMOV:
				case RSP_VECTOR_VRCP:
				case RSP_VECTOR_VRCPL:
				case RSP_VECTOR_VRCPH:
				case RSP_VECTOR_VRSQL:
				case RSP_VECTOR_VRSQH:
					if (DestReg == RspOp.rt) { return TRUE; }
					break;

				case RSP_VECTOR_VCH:
				case RSP_VECTOR_VCL:
				case RSP_VECTOR_VCR:
				case RSP_VECTOR_VMRG:
				case RSP_VECTOR_VLT:
				case RSP_VECTOR_VEQ:
				case RSP_VECTOR_VGE:
				case RSP_VECTOR_VNE:
					if (DestReg == RspOp.rd) { return TRUE; }
					if (DestReg == RspOp.rt) { return TRUE; }
					if (DestReg == RspOp.sa) { return FALSE; }
					break;
				case RSP_VECTOR_VSAW:
					if (DestReg == RspOp.sa) { return FALSE; }
					break;
				default:
					CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
					return TRUE;
				}
			} else {
				switch (RspOp.rs) {				
				case RSP_COP2_CF:
				case RSP_COP2_CT:
					break;
				case RSP_COP2_MT:
				/*	if (DestReg == RspOp.rd) { return FALSE; } */
					break;
				case RSP_COP2_MF:
					if (DestReg == RspOp.rd) { return TRUE; }
					break;
				default:
					CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
					return TRUE;
				}
			}
			break;
		case RSP_LB:
		case RSP_LH:
		case RSP_LW:
		case RSP_LBU:
		case RSP_LHU:
		case RSP_SB:
		case RSP_SH:
		case RSP_SW:
			break;
		case RSP_LC2:
			switch (RspOp.rd) {
			case RSP_LSC2_SV:
			case RSP_LSC2_DV:
			case RSP_LSC2_RV:
				break;

			case RSP_LSC2_QV:
			case RSP_LSC2_BV:
			case RSP_LSC2_LV:
			case RSP_LSC2_TV:			
				break;
				
			case RSP_LSC2_PV:
			case RSP_LSC2_UV:
			case RSP_LSC2_HV:
				if (DestReg == RspOp.rt) { return FALSE; }
				break;

			default:
				CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_SC2:
			switch (RspOp.rd) {
			case RSP_LSC2_BV:
			case RSP_LSC2_SV:
			case RSP_LSC2_LV:
			case RSP_LSC2_DV:
			case RSP_LSC2_QV:
			case RSP_LSC2_RV:
			case RSP_LSC2_PV:
			case RSP_LSC2_UV:
			case RSP_LSC2_HV:
			case RSP_LSC2_FV:
			case RSP_LSC2_WV:
				if (DestReg == RspOp.rt) { return TRUE; }
				break;
				
			case RSP_LSC2_TV:
				if (8 <= 32 - RspOp.rt) {
					if (DestReg >= RspOp.rt && DestReg <= RspOp.rt + 7) {
						return TRUE;
					}
				} else {
					int length = 32 - RspOp.rt, count, del = RspOp.del >> 1, vect = RspOp.rt;
					for (count = 0; count < length; count++) {
						if (DestReg == vect + del) {
							return TRUE;
						}
						del = (del + 1) & 7;
					}
				}
				break;
				
			default:
				CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		default:
			CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
			return TRUE;
		}
		switch (Instruction_State) {
		case NORMAL: break;
		case DO_DELAY_SLOT: 
			Instruction_State = DELAY_SLOT;
			break;
		case DELAY_SLOT: 
			Instruction_State = FINISH_BLOCK;
			break;
		}
	} while (Instruction_State != FINISH_BLOCK);
	
	/*
	This is a tricky situation because most of the 
	microcode does loops, so looping back and checking
	can prove effective, but it's still a branch...
	*/

	if (BranchTarget != 0 && RecursiveCall == FALSE) {
		DWORD BranchTaken, BranchFall;

		// Analysis of branch taken
		BranchTaken = WriteToVectorDest2(DestReg, BranchTarget - 4, TRUE);
		// Analysis of branch as NOP
		BranchFall = WriteToVectorDest2(DestReg, PC, TRUE);

		if (BranchImmed < 0) {
			if (BranchTaken != FALSE) {
				/*
				 * Took this back branch and found a place
				 * that needs this vector as a source
				 */
				return TRUE;
			} else if (BranchFall == HIT_BRANCH) {
				return TRUE;
			}
			// Otherwise this is completely valid
			return BranchFall;
		} else {	
			if (BranchFall != FALSE) {
				/*
				 * Took this forward branch and found a place
				 * that needs this vector as a source
				 */
				return TRUE;
			} else if (BranchTaken == HIT_BRANCH) {
				return TRUE;
			}
			// Otherwise this is completely valid
			return BranchTaken;
		} 
	}

	return TRUE;
}

Boolean WriteToVectorDest(DWORD DestReg, int PC)
{
	DWORD value;
	value = WriteToVectorDest2(DestReg, PC, FALSE);

	if (value == HIT_BRANCH) {
		return TRUE; // TODO: ???
	} else
		return value;
}

/************************************************************
** UseRspFlags
**
** Output:
**	TRUE: Flags are determined not in use
**	FALSE: Either unable to determine or are in use
** Input: PC
*************************************************************/

// TODO: consider delay slots and such in a branch?
Boolean UseRspFlags(int PC)
{
	OPCODE RspOp;
	int Instruction_State = NextInstruction;

	if (Compiler.bFlags == FALSE) return TRUE;

	if (Instruction_State == DELAY_SLOT) { 
		return TRUE;
	}

	do {
		PC -= 4;
		if (PC < 0) { return TRUE; }
		RSP_LW_IMEM(PC, &RspOp.Hex);

		switch (RspOp.op) {

		case RSP_REGIMM:
			switch (RspOp.rt) {
			case RSP_REGIMM_BLTZ:
			case RSP_REGIMM_BGEZ:
			case RSP_REGIMM_BLTZAL:
			case RSP_REGIMM_BGEZAL:
				Instruction_State = DO_DELAY_SLOT;
				break;
			default:
				CompilerWarning("Unknown opcode in UseRspFlags\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_SPECIAL:
			switch (RspOp.funct) {
			case RSP_SPECIAL_SLL:
			case RSP_SPECIAL_SRL:
			case RSP_SPECIAL_SRA:
			case RSP_SPECIAL_SLLV:
			case RSP_SPECIAL_SRLV:
			case RSP_SPECIAL_SRAV:
			case RSP_SPECIAL_ADD:
			case RSP_SPECIAL_ADDU:
			case RSP_SPECIAL_SUB:
			case RSP_SPECIAL_SUBU:
			case RSP_SPECIAL_AND:
			case RSP_SPECIAL_OR:
			case RSP_SPECIAL_XOR:
			case RSP_SPECIAL_NOR:
			case RSP_SPECIAL_SLT:
			case RSP_SPECIAL_SLTU:
			case RSP_SPECIAL_BREAK:
				break;

			case RSP_SPECIAL_JR:
				Instruction_State = DO_DELAY_SLOT;
				break;

			default:
				CompilerWarning("Unknown opcode in WriteToVectorDest\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_J:
		case RSP_JAL:
		case RSP_BEQ:
		case RSP_BNE:
		case RSP_BLEZ:
		case RSP_BGTZ:
			Instruction_State = DO_DELAY_SLOT;
			break;
		case RSP_ADDI:
		case RSP_ADDIU:
		case RSP_SLTI:
		case RSP_SLTIU:
		case RSP_ANDI:
		case RSP_ORI:
		case RSP_XORI:
		case RSP_LUI:
		case RSP_CP0:			
			break;

		case RSP_CP2:
			if ((RspOp.rs & 0x10) != 0) {
				switch (RspOp.funct) {
				case RSP_VECTOR_VMULF:
				case RSP_VECTOR_VMULU:
				case RSP_VECTOR_VMUDL:
				case RSP_VECTOR_VMUDM:
				case RSP_VECTOR_VMUDN:
				case RSP_VECTOR_VMUDH:
					break;
				case RSP_VECTOR_VMACF:
				case RSP_VECTOR_VMACU:
				case RSP_VECTOR_VMADL:
				case RSP_VECTOR_VMADM:
				case RSP_VECTOR_VMADN:
				case RSP_VECTOR_VMADH:
					break;

				case RSP_VECTOR_VSUB:
				case RSP_VECTOR_VADD:
					return FALSE;
				case RSP_VECTOR_VSUBC:
				case RSP_VECTOR_VADDC:
					return TRUE;

				case RSP_VECTOR_VABS:				
				case RSP_VECTOR_VAND:
				case RSP_VECTOR_VOR:
				case RSP_VECTOR_VXOR:
				case RSP_VECTOR_VNAND:
				case RSP_VECTOR_VNOR:
				case RSP_VECTOR_VNXOR:
				case RSP_VECTOR_VRCPH:
				case RSP_VECTOR_VRSQL:
				case RSP_VECTOR_VRSQH:
				case RSP_VECTOR_VRCPL:
				case RSP_VECTOR_VRCP:
					break;

				case RSP_VECTOR_VCR:
				case RSP_VECTOR_VCH:
				case RSP_VECTOR_VCL:				
				case RSP_VECTOR_VLT:
				case RSP_VECTOR_VEQ:
				case RSP_VECTOR_VGE:
				case RSP_VECTOR_VNE:
				case RSP_VECTOR_VMRG:
					return TRUE;

				case RSP_VECTOR_VSAW:
				case RSP_VECTOR_VMOV:				
					break;

				default:
					CompilerWarning("Unknown opcode in UseRspFlags\n%s",RSPOpcodeName(RspOp.Hex,PC));
					return TRUE;
				}
			} else {
				switch (RspOp.rs) {
				case RSP_COP2_CT:
					return TRUE;

				case RSP_COP2_CF:
				case RSP_COP2_MT:
				case RSP_COP2_MF:
					break;
				default:
					CompilerWarning("Unknown opcode in UseRspFlags\n%s",RSPOpcodeName(RspOp.Hex,PC));
					return TRUE;
				}
			}
			break;
		case RSP_LB:
		case RSP_LH:
		case RSP_LW:
		case RSP_LBU:
		case RSP_LHU:
		case RSP_SB:
		case RSP_SH:
		case RSP_SW:
			break;
		case RSP_LC2:
			switch (RspOp.rd) {
			case RSP_LSC2_BV:
			case RSP_LSC2_SV:
			case RSP_LSC2_DV:
			case RSP_LSC2_RV:
			case RSP_LSC2_QV:
			case RSP_LSC2_LV:
			case RSP_LSC2_UV:
			case RSP_LSC2_PV:
			case RSP_LSC2_TV:
			case RSP_LSC2_HV:
				break;
			default:
				CompilerWarning("Unknown opcode in UseRspFlags\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		case RSP_SC2:
			switch (RspOp.rd) {
			case RSP_LSC2_BV:
			case RSP_LSC2_SV:
			case RSP_LSC2_LV:
			case RSP_LSC2_DV:
			case RSP_LSC2_QV:
			case RSP_LSC2_RV:
			case RSP_LSC2_PV:
			case RSP_LSC2_UV:
			case RSP_LSC2_HV:
			case RSP_LSC2_FV:
			case RSP_LSC2_WV:
			case RSP_LSC2_TV:
				break;
			default:
				CompilerWarning("Unknown opcode in UseRspFlags\n%s",RSPOpcodeName(RspOp.Hex,PC));
				return TRUE;
			}
			break;
		default:
			CompilerWarning("Unknown opcode in UseRspFlags\n%s",RSPOpcodeName(RspOp.Hex,PC));
			return TRUE;
		}
		switch (Instruction_State) {
		case NORMAL: break;
		case DO_DELAY_SLOT: 
			Instruction_State = DELAY_SLOT;
			break;
		case DELAY_SLOT: 
			Instruction_State = FINISH_BLOCK;
			break;
		}
	} while ( Instruction_State != FINISH_BLOCK);
	return TRUE;
}

/************************************************************
** IsRegisterConstant
**
** Output:
**	TRUE: Register is constant throughout
**	FALSE: Register is not constant at all
** Input: PC, Pointer to constant to fill
*************************************************************/

Boolean IsRegisterConstant(DWORD Reg, DWORD * Constant)
{
	DWORD PC = 0;
	DWORD References = 0;
	DWORD Const = 0;
	OPCODE RspOp;

	if (Compiler.bGPRConstants == FALSE) 
		return FALSE;

	while (PC < 0x1000) {
		
		RSP_LW_IMEM(PC, &RspOp.Hex);
		
		// Resample command in microcode likes S7
	/*	if (PC == 0xFBC) {
			PC += 4;
			continue;
		}*/

		PC += 4;

		switch (RspOp.op) {
		case RSP_REGIMM:
			break;

		case RSP_SPECIAL:
			switch (RspOp.funct) {
			case RSP_SPECIAL_SLL:
			case RSP_SPECIAL_SRL:
			case RSP_SPECIAL_SRA:
			case RSP_SPECIAL_SLLV:
			case RSP_SPECIAL_SRLV:
			case RSP_SPECIAL_SRAV:
			case RSP_SPECIAL_ADD:
			case RSP_SPECIAL_ADDU:
			case RSP_SPECIAL_SUB:
			case RSP_SPECIAL_SUBU:
			case RSP_SPECIAL_AND:
			case RSP_SPECIAL_OR:
			case RSP_SPECIAL_XOR:
			case RSP_SPECIAL_NOR:
			case RSP_SPECIAL_SLT:
			case RSP_SPECIAL_SLTU:
				if (RspOp.rd == Reg) { return FALSE; }
				break;

			case RSP_SPECIAL_BREAK:
			case RSP_SPECIAL_JR:
				break;

			default:
			//	CompilerWarning("Unknown opcode in IsRegisterConstant\n%s",RSPOpcodeName(RspOp.Hex,PC));
			//	return FALSE;
				break;
			}
			break;

		case RSP_J:
		case RSP_JAL:
		case RSP_BEQ:
		case RSP_BNE:
		case RSP_BLEZ:
		case RSP_BGTZ:
			break;
		
		case RSP_ADDI:
		case RSP_ADDIU:
			if (RspOp.rt == Reg) {
				if (RspOp.rs == 0) {
					if (References > 0) {
						return FALSE;
					}
					Const = (short)RspOp.immediate;
					References++;
				} else {
					return FALSE;
				}
			}
			break;
		case RSP_ORI:
			if (RspOp.rt == Reg) {
				if (!RspOp.rs) {
					if (References > 0) { return FALSE; }
					Const = (WORD)RspOp.immediate;
					References++;
				} else
					return FALSE;
			}
			break;

		case RSP_LUI:
			if (RspOp.rt == Reg) {
				if (References > 0) { return FALSE; }
				Const = (short)RspOp.offset << 16;
				References++;
			}
			break;

		case RSP_ANDI:
		case RSP_XORI:
		case RSP_SLTI:
		case RSP_SLTIU:
			if (RspOp.rt == Reg) { return FALSE; }
			break;

		case RSP_CP0:
			switch (RspOp.rs) {
			case RSP_COP0_MF:
				if (RspOp.rt == Reg) { return FALSE; }
			case RSP_COP0_MT:
				break;
			}			
			break;

		case RSP_CP2:
			if ((RspOp.rs & 0x10) == 0) {
				switch (RspOp.rs) {
				case RSP_COP2_CT:
				case RSP_COP2_MT:
					break;

				case RSP_COP2_CF:
				case RSP_COP2_MF:
					if (RspOp.rt == Reg) { return FALSE; }
					break;

				default:
				//	CompilerWarning("Unknown opcode in IsRegisterConstant\n%s",RSPOpcodeName(RspOp.Hex,PC));
				//	return FALSE;
					break;
				}
			}
			break;

		case RSP_LB:
		case RSP_LH:
		case RSP_LW:
		case RSP_LBU:
		case RSP_LHU:
			if (RspOp.rt == Reg) { return FALSE; }
			break;

		case RSP_SB:
		case RSP_SH:
		case RSP_SW:
			break;
		case RSP_LC2:
			break;
		case RSP_SC2:
			break;
		default:
		//	CompilerWarning("Unknown opcode in IsRegisterConstant\n%s",RSPOpcodeName(RspOp.Hex,PC));
		//	return FALSE;
			break;
		}
	}

	if (References > 0) {
		*Constant = Const;
		return TRUE;
	} else {
		*Constant = 0;
		return FALSE;
	}
}

/************************************************************
** IsOpcodeBranch
**
** Output:
**	TRUE: opcode is a branch
**	FALSE: opcode is not a branch
** Input: PC
*************************************************************/

Boolean IsOpcodeBranch(DWORD PC, OPCODE RspOp)
{
	PC = PC; // Unused

	switch (RspOp.op) {
	case RSP_REGIMM:
		switch (RspOp.rt) {
		case RSP_REGIMM_BLTZ:
		case RSP_REGIMM_BGEZ:
		case RSP_REGIMM_BLTZAL:
		case RSP_REGIMM_BGEZAL:		
			return TRUE;
		default:
			//CompilerWarning("Unknown opcode in IsOpcodeBranch\n%s",RSPOpcodeName(RspOp.Hex,PC));
			break;
		}
		break;
	case RSP_SPECIAL:
		switch (RspOp.funct) {
		case RSP_SPECIAL_SLL:
		case RSP_SPECIAL_SRL:
		case RSP_SPECIAL_SRA:
		case RSP_SPECIAL_SLLV:
		case RSP_SPECIAL_SRLV:
		case RSP_SPECIAL_SRAV:
		case RSP_SPECIAL_ADD:
		case RSP_SPECIAL_ADDU:
		case RSP_SPECIAL_SUB:
		case RSP_SPECIAL_SUBU:
		case RSP_SPECIAL_AND:
		case RSP_SPECIAL_OR:
		case RSP_SPECIAL_XOR:
		case RSP_SPECIAL_NOR:
		case RSP_SPECIAL_SLT:
		case RSP_SPECIAL_SLTU:
		case RSP_SPECIAL_BREAK:
			break;

		case RSP_SPECIAL_JALR:
		case RSP_SPECIAL_JR:
			return TRUE;

		default:
			//CompilerWarning("Unknown opcode in IsOpcodeBranch\n%s",RSPOpcodeName(RspOp.Hex,PC));
			break;
		}
		break;
	case RSP_J:
	case RSP_JAL:
	case RSP_BEQ:
	case RSP_BNE:
	case RSP_BLEZ:
	case RSP_BGTZ:
		return TRUE;

	case RSP_ADDI:
	case RSP_ADDIU:
	case RSP_SLTI:
	case RSP_SLTIU:
	case RSP_ANDI:
	case RSP_ORI:
	case RSP_XORI:
	case RSP_LUI:

	case RSP_CP0:
	case RSP_CP2:
		break;

	case RSP_LB:
	case RSP_LH:
	case RSP_LW:
	case RSP_LBU:
	case RSP_LHU:
	case RSP_SB:
	case RSP_SH:
	case RSP_SW:
		break;

	case RSP_LC2:
	case RSP_SC2:
		break;

	default:
		//CompilerWarning("Unknown opcode in IsOpcodeBranch\n%s",RSPOpcodeName(RspOp.Hex,PC));
		break;
	}

	return FALSE;
}

/************************************************************
** GetInstructionInfo
**
** Output: None in regard to return value
**
** Input:
**	pointer to info structure, fills this
**  with valid opcode data
*************************************************************/

// 3 possible values, GPR, VEC, VEC & GPR, NOOP is zero
#define GPR_Instruction		0x0001 /* GPR Instruction flag */
#define VEC_Instruction		0x0002 /* Vec Instruction flag */
#define COPO_MF_Instruction 0x0080 /* MF Cop 0 Instruction */
#define Flag_Instruction    0x0100 /* Access Flags */
#define Instruction_Mask	(GPR_Instruction | VEC_Instruction)

// 3 possible values, one flag must be set only
#define Load_Operation		0x0004 /* Load Instruction flag */
#define Store_Operation		0x0008 /* Store Instruction flag */
#define Accum_Operation		0x0010 /* Vector op uses accum - loads & stores dont */
#define MemOperation_Mask	(Load_Operation | Store_Operation)
#define Operation_Mask		(MemOperation_Mask | Accum_Operation)

// Per situation basis flags
#define VEC_ResetAccum		0x0000 /* Vector op resets acc */
#define VEC_Accumulate		0x0020 /* Vector op accumulates */

// N/A in instruction assembler syntax, possibly an unused register specifier
#define UNUSED_OPERAND		~0u

#define InvalidOpcode		0x0040

#pragma warning(push)
#pragma warning(disable : 4201) // Non-standard extension used: nameless struct/union

typedef struct {
	union {
		DWORD DestReg;
		DWORD StoredReg;
	};
	union {
		DWORD SourceReg0;
		DWORD IndexReg;
	};
	DWORD SourceReg1;
	DWORD flags;
} OPCODE_INFO;

#pragma warning(pop)

void GetInstructionInfo(DWORD PC, OPCODE * RspOp, OPCODE_INFO * info) {
	switch (RspOp->op) {
	case RSP_REGIMM:
		switch (RspOp->rt) {
		case RSP_REGIMM_BLTZ:
		case RSP_REGIMM_BLTZAL:
		case RSP_REGIMM_BGEZ:
		case RSP_REGIMM_BGEZAL:
			info->flags = InvalidOpcode;
			info->SourceReg0 = RspOp->rs;
			info->SourceReg1 = UNUSED_OPERAND;
			break;

		default:
			CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
			info->flags = InvalidOpcode;
			break;
		}
		break;
	case RSP_SPECIAL:
		switch (RspOp->funct) {
		case RSP_SPECIAL_BREAK:
			info->DestReg = UNUSED_OPERAND;
			info->SourceReg0 = UNUSED_OPERAND;
			info->SourceReg1 = UNUSED_OPERAND;
			info->flags = GPR_Instruction;
			break;

		case RSP_SPECIAL_SLL:
		case RSP_SPECIAL_SRL:
		case RSP_SPECIAL_SRA:
			info->DestReg = RspOp->rd;
			info->SourceReg0 = RspOp->rt;
			info->SourceReg1 = UNUSED_OPERAND;
			info->flags = GPR_Instruction;
			break;
		case RSP_SPECIAL_SLLV:
		case RSP_SPECIAL_SRLV:
		case RSP_SPECIAL_SRAV:
		case RSP_SPECIAL_ADD:
		case RSP_SPECIAL_ADDU:
		case RSP_SPECIAL_SUB:
		case RSP_SPECIAL_SUBU:
		case RSP_SPECIAL_AND:
		case RSP_SPECIAL_OR:
		case RSP_SPECIAL_XOR:
		case RSP_SPECIAL_NOR:
		case RSP_SPECIAL_SLT:
		case RSP_SPECIAL_SLTU:
			info->DestReg = RspOp->rd;
			info->SourceReg0 = RspOp->rs;
			info->SourceReg1 = RspOp->rt;
			info->flags = GPR_Instruction;
			break;

		case RSP_SPECIAL_JR:
			info->flags = InvalidOpcode;
			info->SourceReg0 = UNUSED_OPERAND;
			info->SourceReg1 = UNUSED_OPERAND;
			break;

		default:
			CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
			info->flags = InvalidOpcode;
			break;
		}
		break;
	case RSP_J:
	case RSP_JAL:
		info->flags = InvalidOpcode;
		info->SourceReg0 = UNUSED_OPERAND;
		info->SourceReg1 = UNUSED_OPERAND;
		break;
	case RSP_BEQ:
	case RSP_BNE:
		info->flags = InvalidOpcode;
		info->SourceReg0 = RspOp->rt;
		info->SourceReg1 = RspOp->rs;
		break;
	case RSP_BLEZ:
	case RSP_BGTZ:
		info->flags = InvalidOpcode;
		info->SourceReg0 = RspOp->rs;
		info->SourceReg1 = UNUSED_OPERAND;
		break;
		
	case RSP_ADDI:
	case RSP_ADDIU:
	case RSP_SLTI:
	case RSP_SLTIU:
	case RSP_ANDI:
	case RSP_ORI:
	case RSP_XORI:
		info->DestReg = RspOp->rt;
		info->SourceReg0 = RspOp->rs;
		info->SourceReg1 = UNUSED_OPERAND;
		info->flags = GPR_Instruction;
		break;

	case RSP_LUI:
		info->DestReg = RspOp->rt;
		info->SourceReg0 = UNUSED_OPERAND;
		info->SourceReg1 = UNUSED_OPERAND;
		info->flags = GPR_Instruction;
		break;

	case RSP_CP0:
		switch (RspOp->rs) {
		case RSP_COP0_MF:			
			info->DestReg = RspOp->rt;
			info->SourceReg0 = UNUSED_OPERAND;
			info->SourceReg1 = UNUSED_OPERAND;
			if (RspOp->rd == 0x4 || RspOp->rd == 0x7){
				info->flags = InvalidOpcode | COPO_MF_Instruction;
			} else{
				info->flags = COPO_MF_Instruction | GPR_Instruction | Load_Operation;
			}			
			break;

		case RSP_COP0_MT:
			info->StoredReg = RspOp->rt;
			info->SourceReg0 = UNUSED_OPERAND;
			info->SourceReg1 = UNUSED_OPERAND;
			info->flags = GPR_Instruction | Store_Operation;
			break;
		}
		break;

	case RSP_CP2:
		if ((RspOp->rs & 0x10) != 0) {
			switch (RspOp->funct) {
			case RSP_VECTOR_VNOOP:
				info->DestReg = UNUSED_OPERAND;
				info->SourceReg0 = UNUSED_OPERAND;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = VEC_Instruction;
				break;

			case RSP_VECTOR_VMULF:
			case RSP_VECTOR_VMULU:
			case RSP_VECTOR_VMUDL:
			case RSP_VECTOR_VMUDM:
			case RSP_VECTOR_VMUDN:
			case RSP_VECTOR_VMUDH:
			case RSP_VECTOR_VABS:
			case RSP_VECTOR_VAND:
			case RSP_VECTOR_VOR:
			case RSP_VECTOR_VXOR:
			case RSP_VECTOR_VNAND:
			case RSP_VECTOR_VNOR:
			case RSP_VECTOR_VNXOR:
				info->DestReg = RspOp->sa;
				info->SourceReg0 = RspOp->rd;
				info->SourceReg1 = RspOp->rt;
				info->flags = VEC_Instruction | VEC_ResetAccum | Accum_Operation;
				break;
			case RSP_VECTOR_VMACF:
			case RSP_VECTOR_VMACU:
			case RSP_VECTOR_VMADL:
			case RSP_VECTOR_VMADM:
			case RSP_VECTOR_VMADN:
			case RSP_VECTOR_VMADH:
				info->DestReg = RspOp->sa;
				info->SourceReg0 = RspOp->rd;
				info->SourceReg1 = RspOp->rt;
				info->flags = VEC_Instruction | VEC_Accumulate | Accum_Operation;
				break;
			case RSP_VECTOR_VADD:
			case RSP_VECTOR_VADDC:
			case RSP_VECTOR_VSUB:
			case RSP_VECTOR_VSUBC:
			case RSP_VECTOR_VCR:
			case RSP_VECTOR_VCH:
			case RSP_VECTOR_VCL:
			case RSP_VECTOR_VLT:
			case RSP_VECTOR_VEQ:
			case RSP_VECTOR_VGE:
			case RSP_VECTOR_VNE:
				info->DestReg = RspOp->sa;
				info->SourceReg0 = RspOp->rd;
				info->SourceReg1 = RspOp->rt;
				info->flags = VEC_Instruction | VEC_ResetAccum | Accum_Operation | Flag_Instruction;
				break;

			case RSP_VECTOR_VMOV:
			case RSP_VECTOR_VRCP:
			case RSP_VECTOR_VRCPL:
			case RSP_VECTOR_VRCPH:
			case RSP_VECTOR_VRSQL:
			case RSP_VECTOR_VRSQH:
				info->DestReg = RspOp->sa;
				info->SourceReg0 = RspOp->rt;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = VEC_Instruction | VEC_ResetAccum | Accum_Operation; // Assume reset?
				break;

			case RSP_VECTOR_VMRG:
			 	info->DestReg = RspOp->sa;
				info->SourceReg0 = RspOp->rt;
				info->SourceReg1 = RspOp->rd;
				info->flags = VEC_Instruction | VEC_ResetAccum | Accum_Operation | Flag_Instruction; // Assume reset?
				break;

			case RSP_VECTOR_VSAW:
			//	info->flags = InvalidOpcode;
				info->DestReg = RspOp->sa;
				info->SourceReg0 = UNUSED_OPERAND;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = VEC_Instruction | Accum_Operation | VEC_Accumulate;
				break;

			default:
				CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
				info->flags = InvalidOpcode;
				break;
			}
		} else {
			switch (RspOp->rs) {				
			case RSP_COP2_CT:
				info->StoredReg = RspOp->rt;
				info->SourceReg0 = UNUSED_OPERAND;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = GPR_Instruction | Store_Operation | Flag_Instruction;
				break;
			case RSP_COP2_CF:
				info->DestReg = RspOp->rt;
				info->SourceReg0 = UNUSED_OPERAND;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = GPR_Instruction | Load_Operation | Flag_Instruction;
				break;

			// RD is always the vector register, RT is always GPR
			case RSP_COP2_MT:
				info->DestReg = RspOp->rd;
				info->SourceReg0 = RspOp->rt;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = VEC_Instruction | GPR_Instruction | Load_Operation;
				break;
			case RSP_COP2_MF:
				info->DestReg = RspOp->rt;
				info->SourceReg0 = RspOp->rd;
				info->SourceReg1 = UNUSED_OPERAND;
				info->flags = VEC_Instruction | GPR_Instruction | Store_Operation;
				break;
			default:
				CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
				info->flags = InvalidOpcode;
				break;
			}
		}
		break;
	case RSP_LB:
	case RSP_LH:
	case RSP_LW:
	case RSP_LBU:
	case RSP_LHU:
		info->DestReg = RspOp->rt;
		info->IndexReg = RspOp->base;
		info->SourceReg1 = UNUSED_OPERAND;
		info->flags = Load_Operation | GPR_Instruction;
		break;
	case RSP_SB:
	case RSP_SH:
	case RSP_SW:
		info->StoredReg = RspOp->rt;
		info->IndexReg = RspOp->base;
		info->SourceReg1 = UNUSED_OPERAND;
		info->flags = Store_Operation | GPR_Instruction;
		break;
	case RSP_LC2:
		switch (RspOp->rd) {
		case RSP_LSC2_BV:
		case RSP_LSC2_SV:
		case RSP_LSC2_DV:
		case RSP_LSC2_RV:
		case RSP_LSC2_QV:
		case RSP_LSC2_LV:
		case RSP_LSC2_UV:
		case RSP_LSC2_PV:
			info->DestReg = RspOp->rt;
			info->IndexReg = RspOp->base;
			info->SourceReg1 = UNUSED_OPERAND;
			info->flags = Load_Operation | VEC_Instruction;
			break;

		case RSP_LSC2_TV:
			info->flags = InvalidOpcode;
			break;
		default:
			CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
			info->flags = InvalidOpcode;
			break;
		}
		break;
	case RSP_SC2:
		switch (RspOp->rd) {
		case RSP_LSC2_BV:
		case RSP_LSC2_SV:
		case RSP_LSC2_LV:
		case RSP_LSC2_DV:
		case RSP_LSC2_QV:
		case RSP_LSC2_RV:
		case RSP_LSC2_PV:
		case RSP_LSC2_UV:
		case RSP_LSC2_HV:
		case RSP_LSC2_FV:
		case RSP_LSC2_WV:
			info->DestReg = RspOp->rt;
			info->IndexReg = RspOp->base;
			info->SourceReg1 = UNUSED_OPERAND;
			info->flags = Store_Operation | VEC_Instruction;
			break;
		case RSP_LSC2_TV:
			info->flags = InvalidOpcode;
			break;
		default:
			CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
			info->flags = InvalidOpcode;
			break;
		}
		break;
	default:
	/*	CompilerWarning("Unknown opcode in GetInstructionInfo\n%s",RSPOpcodeName(RspOp->Hex,PC));
	*/	info->flags = InvalidOpcode;
		break;
	}
}

/************************************************************
** DelaySlotAffectBranch
**
** Output:
**	TRUE: Delay slot does affect the branch
**	FALSE: Registers do not affect each other
**
** Input: PC
*************************************************************/

Boolean DelaySlotAffectBranch(DWORD PC)
{
	OPCODE Branch, Delay;
	OPCODE_INFO infoBranch, infoDelay;

	if (IsOpcodeNop(PC + 4) == TRUE) { 
		return FALSE;
	}

	RSP_LW_IMEM(PC, &Branch.Hex);
	RSP_LW_IMEM(PC+4, &Delay.Hex);

	memset(&infoDelay,0,sizeof(infoDelay));
	memset(&infoBranch,0,sizeof(infoBranch));
	
	GetInstructionInfo(PC, &Branch, &infoBranch);
	GetInstructionInfo(PC+4, &Delay, &infoDelay);

	if ((infoDelay.flags & COPO_MF_Instruction) == COPO_MF_Instruction) {
		return TRUE;
	}

	if ((infoDelay.flags & Instruction_Mask) == VEC_Instruction) {
		return FALSE;
	}

	if (infoBranch.SourceReg0 == infoDelay.DestReg) { return TRUE; }
	if (infoBranch.SourceReg1 == infoDelay.DestReg) { return TRUE; }

	return FALSE;
}

/************************************************************
** CompareInstructions
**
** Output:
**	TRUE: The opcodes are fine, no dependency
**	FALSE: Watch it, these ops cant be touched
**
** Input:
**	Top, not the current operation, the one above
**	Bottom: the current opcode for re-ordering bubble style
*************************************************************/

Boolean CompareInstructions(DWORD PC, OPCODE * Top, OPCODE * Bottom)
{
	OPCODE_INFO info0, info1;
	DWORD InstructionType;

	GetInstructionInfo(PC - 4, Top, &info0);
	GetInstructionInfo(PC, Bottom, &info1);

#ifdef COMPARE_INSTRUCTIONS_VERBOSE
	CPU_Message("Comparing %s (%X)", RSPOpcodeName ( Top->Hex, PC - 4 ), PC - 4);
	CPU_Message("to %s (%X)", RSPOpcodeName ( Bottom->Hex, PC), PC);
#endif

	// Usually branches and such
	if ((info0.flags & InvalidOpcode) != 0) return FALSE;
	if ((info1.flags & InvalidOpcode) != 0) return FALSE;

	if ((info0.flags & Flag_Instruction) != 0 && (info1.flags & Flag_Instruction) != 0) return FALSE;

	InstructionType = (info0.flags & Instruction_Mask) << 2;
	InstructionType |= info1.flags & Instruction_Mask;
	InstructionType &= 0x0F; // Paranoia

	// 4-bit range, 16 possible combinations
	switch (InstructionType) {

	// Detect NOOP instruction, 7 cases, (see flags)
	case 0x01: case 0x02: case 0x03: // First is a NOOP
		return TRUE;
	case 0x00:						 // Both?
	case 0x10: case 0x20: case 0x30: // Second is a NOOP
		return FALSE;

	case 0x06: // GPR then Vector - 01,10
		if ((info0.flags & MemOperation_Mask) != 0 && (info1.flags & MemOperation_Mask) != 0) {
			// TODO: We have a vector and GPR memory operation
			return FALSE;
		} else if ((info1.flags & MemOperation_Mask) != 0) {
			// We have a vector memory operation
			return (info1.IndexReg == info0.DestReg) ? FALSE : TRUE;
		}
		
		// We could have memory or normal GPR instruction here
		// paired with some kind of vector operation
		
		return TRUE;
	case 0x0A: // Vector then Vector - 10,10

		/*
		** Check for vector store then vector multiply (VMULF)
		**
		** This basically gives preferences to putting stores
		** as close to the finish of an operation as possible
		*/
		
		if ((info0.flags & Store_Operation) != 0 && (info1.flags & Accum_Operation) != 0
			&& !(info1.flags & VEC_Accumulate)) { return FALSE; }
		
		// Look for loads and than some kind of vector operation
		// that does no accumulating, there is no reason to reorder
		
		if ((info0.flags & Load_Operation) != 0 && (info1.flags & Accum_Operation) != 0
			&& !(info1.flags & VEC_Accumulate)) { return FALSE; }

		if ((info0.flags & MemOperation_Mask) != 0 && (info1.flags & MemOperation_Mask) != 0) {

			// TODO: This is a pain, it's best to leave it alone

			return FALSE;
		} else if ((info1.flags & MemOperation_Mask) != 0) {
			// Remember stored REG & loaded REG are the same
			if (info0.DestReg == info1.DestReg) { return FALSE; }

			if (info1.flags & Load_Operation) {
				if (info0.SourceReg0 == info1.DestReg) { return FALSE; }
				if (info0.SourceReg1 == info1.DestReg) { return FALSE; }
			} else if (info1.flags & Store_Operation) {
				// It can store source REGS
				return TRUE;
			}

			return TRUE;
		} else if ((info0.flags & MemOperation_Mask) != 0) {
			// Remember stored REG & loaded REG are the same
			if (info0.DestReg == info1.DestReg) { return FALSE; }

			if (info0.flags & Load_Operation) {
				if (info1.SourceReg0 == info0.DestReg) { return FALSE; }
				if (info1.SourceReg1 == info0.DestReg) { return FALSE; }
			} else if (info0.flags & Store_Operation) {
				// It can store source REGS
				return TRUE;
			}

			return TRUE;
		} else if ((info0.flags & VEC_Accumulate) != 0) {
			
			/*
			** Example:
			** VMACF
			** VMUDH or VMADH or VADD
			*/

			return FALSE;
		} else if ((info1.flags & VEC_Accumulate) != 0) {
			
			/*
			** Example:
			** VMULF
			** VMADH
			*/
			
			return FALSE;
		} else {
			
			/*
			** Example:
			** VMULF or VADDC
			** VADD or VMUDH
			*/

			return FALSE;
		}
		break;

	case 0x09: // Vector then GPR - 10,01
	
		/*
		This is where the bias comes into play, otherwise
		we can sit here all day swapping these 2 types
		*/
		
		return FALSE;

	case 0x05: // GPR then GPR - 01,01
	case 0x07: // GPR then COP2 - 01, 11
	case 0x0D: // COP2 then GPR - 11, 01
	case 0x0F: // COP2 then COP2 - 11, 11
		return FALSE;

	case 0x0B: // Vector then COP2 - 10, 11
		if (info1.flags & Load_Operation) {
			// Move To COP2 (destination) from GPR (source)
			if (info1.DestReg == info0.DestReg) { return FALSE; }
			if (info1.DestReg == info0.SourceReg0) { return FALSE; }
			if (info1.DestReg == info0.SourceReg1) { return FALSE; }
		} else if (info1.flags & Store_Operation) {
			// Move From COP2 (source) to GPR (destination)
			if (info1.SourceReg0 == info0.DestReg) { return FALSE; }
			if (info1.SourceReg0 == info0.SourceReg0) { return FALSE; }
			if (info1.SourceReg0 == info0.SourceReg1) { return FALSE; }
		} else {
			CompilerWarning("Reorder: Unhandled Vector than COP2");
		}
		// We want vectors on top
		return FALSE;

	case 0x0E: // COP2 then Vector - 11, 10
		if (info0.flags & Load_Operation) {
			// Move To COP2 (destination) from GPR (source)
			if (info0.DestReg == info1.DestReg) { return FALSE; }
			if (info0.DestReg == info1.SourceReg0) { return FALSE; }
			if (info0.DestReg == info1.SourceReg1) { return FALSE; }
		} else if (info0.flags & Store_Operation) {
			// Move From COP2 (source) to GPR (destination)
			if (info0.SourceReg0 == info1.DestReg) { return FALSE; }
			if (info0.SourceReg0 == info1.SourceReg0) { return FALSE; }
			if (info0.SourceReg0 == info1.SourceReg1) { return FALSE; }
			if (info0.DestReg == info1.SourceReg0) { return FALSE; }
		} else {
			CompilerWarning("Reorder: Unhandled COP2 than Vector");
		}
		// We want this at the top
		return TRUE;

	default:
		CompilerWarning("Reorder: Unhandled instruction type: %i", InstructionType);
	}

	return FALSE;
}
