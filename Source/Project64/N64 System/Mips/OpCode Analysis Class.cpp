#include "stdafx.h"

#ifdef tofix
COpcodeAnalysis::COpcodeAnalysis(OPCODE &opcode) :
	m_opcode(opcode)
{
}

bool COpcodeAnalysis::DelaySlotEffectsCompare (DWORD Reg1, DWORD Reg2) {
	COpcode DelayOp(m_opcode.VirtualAddress + 4);

	switch (DelayOp.m_opcode.op) {
	case R4300i_SPECIAL:
		switch (DelayOp.m_opcode.funct) {
		case R4300i_SPECIAL_SLL:   case R4300i_SPECIAL_SRL:   case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:  case R4300i_SPECIAL_SRLV:  case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:  case R4300i_SPECIAL_MTHI:  case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:  case R4300i_SPECIAL_DSLLV: case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV: case R4300i_SPECIAL_ADD:   case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:   case R4300i_SPECIAL_SUBU:  case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:    case R4300i_SPECIAL_XOR:   case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			if (DelayOp.m_opcode.rd == 0) { return false; }
			if (DelayOp.m_opcode.rd == Reg1) { return true; }
			if (DelayOp.m_opcode.rd == Reg2) { return true; }
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
		case R4300i_SPECIAL_BREAK:
		case R4300i_SPECIAL_SYNC:
		case R4300i_SPECIAL_SYSCALL:
			break;
		case R4300i_SPECIAL_JR:
		case R4300i_SPECIAL_JALR:
			return true;
		default:
			//_Notify->DisplayError("%s\nDoes %s effect Delay slot at %X?",Name().c_str(),DelayOp.Name().c_str(),DelayOp.PC());
			return true;
		}
		break;
	case R4300i_REGIMM: return true;
	case R4300i_CP0:
		switch (DelayOp.m_opcode.rs) {
		case R4300i_COP0_MT: break;
		case R4300i_COP0_MF:
			if (DelayOp.m_opcode.rt == 0) { return false; }
			if (DelayOp.m_opcode.rt == Reg1) { return true; }
			if (DelayOp.m_opcode.rt == Reg2) { return true; }
			break;
		default:
			if ( (DelayOp.m_opcode.rs & 0x10 ) != 0 ) {
				switch( DelayOp.m_opcode.funct ) {
				case R4300i_COP0_CO_TLBR: break;
				case R4300i_COP0_CO_TLBWI: break;
				case R4300i_COP0_CO_TLBWR: break;
				case R4300i_COP0_CO_TLBP: break;
				default: 
					//_Notify->DisplayError("%s\nDoes %s effect Delay slot at %X?",Name().c_str(),DelayOp.Name().c_str(),DelayOp.PC());
					return true;
				}
			} else {
				//_Notify->DisplayError("%s\nDoes %s effect Delay slot at %X?",Name().c_str(),DelayOp.Name().c_str(),DelayOp.PC());
				return true;
			}
		}
		break;
	case R4300i_CP1:
		switch (DelayOp.m_opcode.fmt) {
		case R4300i_COP1_MF:
			if (DelayOp.m_opcode.rt == 0) { return false; }
			if (DelayOp.m_opcode.rt == Reg1) { return true; }
			if (DelayOp.m_opcode.rt == Reg2) { return true; }
			break;
		case R4300i_COP1_CF: break;
		case R4300i_COP1_MT: break;
		case R4300i_COP1_CT: break;
		case R4300i_COP1_BC: return true;
		case R4300i_COP1_S: break;
		case R4300i_COP1_D: break;
		case R4300i_COP1_W: break;
		case R4300i_COP1_L: break;
		default:
			//_Notify->DisplayError("%s\nDoes %s effect Delay slot at %X?",Name().c_str(),DelayOp.Name().c_str(),DelayOp.PC(),DelayOp.m_opcode.op);
			return true;
		}
		break;
	case R4300i_J:
	case R4300i_JAL:
	case R4300i_BEQ:
	case R4300i_BNE:
	case R4300i_BLEZ:
	case R4300i_BGTZ:
	case R4300i_BEQL:
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		return true;
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
	case R4300i_LUI:
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_DADDI:
	case R4300i_DADDIU:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LW:
	case R4300i_LWL:
	case R4300i_LWR:
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LD:
	case R4300i_LWC1:
	case R4300i_LDC1:
		if (DelayOp.m_opcode.rt == 0) { return false; }
		if (DelayOp.m_opcode.rt == Reg1) { return true; }
		if (DelayOp.m_opcode.rt == Reg2) { return true; }
		break;
	case R4300i_CACHE: break;
	case R4300i_SB: break;
	case R4300i_SH: break;
	case R4300i_SW: break;
	case R4300i_SWR: break;
	case R4300i_SWL: break;
	case R4300i_SWC1: break;
	case R4300i_SDC1: break;
	case R4300i_SD: break;
	//Unknown opcodes
	case 0x12: 
	case 0x1C: 
	case 0x32: 
	case 0x3a: 
	case 0x3E: 
		break;
	default:
		//_Notify->DisplayError("%s\nDoes %s effect Delay slot at %X?\n%x",Name().c_str(),DelayOp.Name().c_str(),DelayOp.PC(),DelayOp.m_opcode.op);
		return true;
	}
	return false;
}

bool COpcodeAnalysis::DelaySlotEffectsJump (void) {
	switch (m_opcode.op) {
	case R4300i_SPECIAL:
		switch (m_opcode.funct) {
		case R4300i_SPECIAL_JR:	return DelaySlotEffectsCompare(m_opcode.rs,0);
		case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(m_opcode.rs,31);
		}
		break;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
			return DelaySlotEffectsCompare(m_opcode.rs,0);
		}
		break;
	case R4300i_JAL: 
	case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(31,0); break;
	case R4300i_J: return false;
	case R4300i_BEQ: 
	case R4300i_BNE: 
	case R4300i_BLEZ: 
	case R4300i_BGTZ: 
		return DelaySlotEffectsCompare(m_opcode.rs,m_opcode.rt);
	case R4300i_CP1:
		switch (m_opcode.fmt) {
		case R4300i_COP1_BC:
			switch (m_opcode.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				{
					COpcode DelayOp(m_opcode.VirtualAddress + 4);
				
					if (DelayOp.m_opcode.op == R4300i_CP1) {
						if (DelayOp.m_opcode.fmt == R4300i_COP1_S && (DelayOp.m_opcode.funct & 0x30) == 0x30 ) {
							return true;
						} 
						if (DelayOp.m_opcode.fmt == R4300i_COP1_D && (DelayOp.m_opcode.funct & 0x30) == 0x30 ) {
							return true;
						} 
					}
					return false;
				} 
				break;
			}
			break;
		}
		break;
	case R4300i_BEQL: 
	case R4300i_BNEL: 
	case R4300i_BLEZL: 
	case R4300i_BGTZL: 
		return DelaySlotEffectsCompare(m_opcode.rs,m_opcode.rt);
	}
	return true;
}

bool COpcodeAnalysis::HasDelaySlot ( void ) {
	if (HardJump())     { return true; }
	if (LinkedJump())   { return true; }
	if (RelativeJump()) { return true; }
	return false;
}

//A non relative jump which does not record the address that it was at
//so it would be impossible to return.
bool COpcodeAnalysis::HardJump ( void ) {
	switch (m_opcode.op) {
	case R4300i_SPECIAL:
		switch (m_opcode.funct) {
		case R4300i_SPECIAL_JR: return true;			
		}
		break;
	case R4300i_J: return true;
	}
	return false;
}

bool COpcodeAnalysis::IsCop1Instruction  ( void ) {
	if (m_opcode.op == R4300i_CP1) { return true; }
	if (m_opcode.op == R4300i_LWC1) { return true; }
	if (m_opcode.op == R4300i_LDC1) { return true; }
	if (m_opcode.op == R4300i_SWC1) { return true; }
	if (m_opcode.op == R4300i_SDC1) { return true; }
	return false;
}


bool COpcodeAnalysis::LinkedJump ( void ) {
	switch (m_opcode.op) {
	case R4300i_JAL: return true;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
		case R4300i_REGIMM_BLTZALL: 
		case R4300i_REGIMM_BGEZALL: 
			return true;
		}
		break;
	}
	return false;
}

bool COpcodeAnalysis::RelativeJump ( void ) {
	switch (m_opcode.op) {
	case R4300i_BEQ:
	case R4300i_BNE:
	case R4300i_BLEZ:
	case R4300i_BGTZ:
	case R4300i_BEQL:
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		return true;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
		case R4300i_REGIMM_BLTZALL: 
		case R4300i_REGIMM_BGEZALL: 
			return true;
		}
		break;
	case R4300i_CP1:
		if (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCF)  { return true; }
		if (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCT)  { return true; }
		if (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCFL) { return true; }
		if (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCTL) { return true; }
		break;
	}
	return false;
}

bool COpcodeAnalysis::LikelyJump ( void ) {
	switch (m_opcode.op) {
	case R4300i_BEQL:
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		return true;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZALL: 
		case R4300i_REGIMM_BGEZALL: 
			return true;
		}
		break;
	case R4300i_CP1:
		if (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCFL) { return true; }
		if (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCTL) { return true; }
		break;
	}
	return false;
}

//Always will jump
bool COpcodeAnalysis::NonConditionalJump ( void ) {
	switch (m_opcode.op) {
	case R4300i_J:
		return true;
	case R4300i_BEQ:
		if (m_opcode.rs == 0 && m_opcode.rt == 0) {
			return true;
		}
		break;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BGEZAL: 
		case R4300i_REGIMM_BGEZALL: 
			if (m_opcode.rs == 0) {
				return true;
			}
		}
		break;
	}
	return false;
}

DWORD COpcodeAnalysis::JumpLocation ( void ) {
	switch (m_opcode.op) {
	case R4300i_J: 
	case R4300i_JAL: 
		return (m_opcode.VirtualAddress & 0xF0000000) + (m_opcode.target << 2);
	case R4300i_BEQ:
	case R4300i_BNE:
	case R4300i_BLEZ:
	case R4300i_BGTZ:
	case R4300i_BEQL:
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		return m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
		case R4300i_REGIMM_BLTZALL: 
		case R4300i_REGIMM_BGEZALL: 
			return m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4;
		}
		break;
	case R4300i_CP1:
		if ((m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCF) ||
		    (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCT) ||
		    (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCFL) ||
		    (m_opcode.fmt == R4300i_COP1_BC && m_opcode.ft == R4300i_COP1_BC_BCTL))
		{ 
			return m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4; 
		}
		break;
	}
	return -1;
}

bool COpcodeAnalysis::TerminateExecution ( void ) {
	switch (m_opcode.op) {
	case R4300i_SPECIAL:
		switch (m_opcode.funct) {
		case R4300i_SPECIAL_SYSCALL: return true;			
		case R4300i_SPECIAL_BREAK: return true;			
		}
		break;
	case R4300i_CP0:
		if ( (m_opcode.rs & 0x10 ) != 0 ) {
			switch( m_opcode.funct ) {
			case R4300i_COP0_CO_ERET:  return true;
			}
		}
		break;
	}
	return false;
}

stdstr COpcodeAnalysis::Name( void ) {	
	const char * szOpName = OpcodeName();
	char Command[200];
	OpcodeParam(Command);
	return  stdstr_f("%-15s%s",szOpName,Command);
}

stdstr COpcodeAnalysis::FullName(bool * MultipleOps) {	
	stdstr OpName, OpParam;

	if (MultipleOps) { *MultipleOps = false; }
	if (MultipleOps == NULL) { return Name(); }

	//Use LI macro
	if (m_opcode.op == R4300i_LUI) {		
		DWORD Value    = m_opcode.immediate * 0x10000;
		int   Register = m_opcode.rt; 
		
		COpcode NextOp(m_opcode.VirtualAddress + 4);
		if ((NextOp.m_opcode.op == R4300i_ADDIU || NextOp.m_opcode.op == R4300i_ORI) && 
			NextOp.m_opcode.rt == m_opcode.rt && NextOp.m_opcode.rs == m_opcode.rt)
		{
			Value += (DWORD)((short)NextOp.m_opcode.immediate);
			*MultipleOps = true;
		}
		char Param[100];
		sprintf(Param, "%s, 0x%08X",CRegName::GPR[Register],Value);

		OpName = stdstr("li");
		OpParam = stdstr(Param);
	}
	if (m_opcode.op == R4300i_ADDIU && m_opcode.rs == 0) {		
		char Param[100];
		sprintf(Param, "%s, 0x%08X",CRegName::GPR[m_opcode.rt], (DWORD)((short)m_opcode.immediate));

		OpName = stdstr("li");
		OpParam = stdstr(Param);
	} 
	if (m_opcode.op == R4300i_ORI && m_opcode.rs == 0) {		
		char Param[100];
		sprintf(Param, "%s, 0x%08X",CRegName::GPR[m_opcode.rt],m_opcode.immediate);

		OpName = stdstr("li");
		OpParam = stdstr(Param);
	} 

	_Notify->BreakPoint(__FILE__,__LINE__);
//	if (OpName.length() == 0) { OpName = OpcodeName(); }
//	if (OpParam.length() == 0) { OpParam = OpcodeParam(); }
	while (OpName.length() < 7) { OpName += " "; }
	return  stdstr_f("%s\t%s",OpName.c_str(),OpParam.c_str());
}

const char * COpcodeAnalysis::OpcodeName ( void ) 
{
	switch (m_opcode.op) {
	case R4300i_SPECIAL:
		switch (m_opcode.funct) {
		case R4300i_SPECIAL_SLL:
			if (m_opcode.Hex != 0) {
				return "sll";
			} else {
				return "nop";
			}
			break;
		case R4300i_SPECIAL_SRL: return "srl";
		case R4300i_SPECIAL_SRA: return "sra";
		case R4300i_SPECIAL_SLLV: return "sllv";
		case R4300i_SPECIAL_SRLV: return "srlv";
		case R4300i_SPECIAL_SRAV: return "srav";
		case R4300i_SPECIAL_JR: return "jr";
		case R4300i_SPECIAL_JALR: return "jalr";
		case R4300i_SPECIAL_SYSCALL: return "system call";
		case R4300i_SPECIAL_BREAK: return "break";
		case R4300i_SPECIAL_SYNC: return "sync";
		case R4300i_SPECIAL_MFHI: return "mfhi";
		case R4300i_SPECIAL_MTHI: return "mthi";
		case R4300i_SPECIAL_MFLO: return "mflo";
		case R4300i_SPECIAL_MTLO: return "mtlo";
		case R4300i_SPECIAL_DSLLV: return "dsllv";
		case R4300i_SPECIAL_DSRLV: return "dsrlv";
		case R4300i_SPECIAL_DSRAV: return "dsrav";
		case R4300i_SPECIAL_MULT: return "mult";
		case R4300i_SPECIAL_MULTU: return "multu";
		case R4300i_SPECIAL_DIV: return "div";
		case R4300i_SPECIAL_DIVU: return "divu";
		case R4300i_SPECIAL_DMULT: return "dmult";
		case R4300i_SPECIAL_DMULTU: return "dmultu";
		case R4300i_SPECIAL_DDIV: return "ddiv";
		case R4300i_SPECIAL_DDIVU: return "ddivu";
		case R4300i_SPECIAL_ADD: return "add";
		case R4300i_SPECIAL_ADDU: return "addu";
		case R4300i_SPECIAL_SUB: return "sub";
		case R4300i_SPECIAL_SUBU: return "subu";
		case R4300i_SPECIAL_AND: return "and";
		case R4300i_SPECIAL_OR: return "or";
		case R4300i_SPECIAL_XOR: return "xor";
		case R4300i_SPECIAL_NOR: return "nor";
		case R4300i_SPECIAL_SLT: return "slt";
		case R4300i_SPECIAL_SLTU: return "sltu";
		case R4300i_SPECIAL_DADD: return "dadd";
		case R4300i_SPECIAL_DADDU: return "daddu";
		case R4300i_SPECIAL_DSUB: return "dsub";
		case R4300i_SPECIAL_DSUBU: return "dsubu";
		case R4300i_SPECIAL_TGE: return "tge";
		case R4300i_SPECIAL_TGEU: return "tgeu";
		case R4300i_SPECIAL_TLT: return "tlt";
		case R4300i_SPECIAL_TLTU: return "tltu";
		case R4300i_SPECIAL_TEQ: return "teq";
		case R4300i_SPECIAL_TNE: return "tne";
		case R4300i_SPECIAL_DSLL: return "dsll";
		case R4300i_SPECIAL_DSRL: return "dsrl";		
		case R4300i_SPECIAL_DSRA: return "dsra";
		case R4300i_SPECIAL_DSLL32: return "dsll32";
		case R4300i_SPECIAL_DSRL32: return "dsrl32";
		case R4300i_SPECIAL_DSRA32: return "dsra32";
		}
		break;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZ: return "bltz";
		case R4300i_REGIMM_BGEZ:
			if (m_opcode.rs == 0) { return "b"; }
			return "bgez";
		case R4300i_REGIMM_BLTZL: return "bltzl";
		case R4300i_REGIMM_BGEZL: return "bgezl";
		case R4300i_REGIMM_TGEI: return "tgei";
		case R4300i_REGIMM_TGEIU: return "tgeiu";
		case R4300i_REGIMM_TLTI: return "tlti";
		case R4300i_REGIMM_TLTIU: return "tltiu";
		case R4300i_REGIMM_TEQI: return "teqi";
		case R4300i_REGIMM_TNEI: return "tnei";
		case R4300i_REGIMM_BLTZAL: return "bltzal";
		case R4300i_REGIMM_BGEZAL:
			if (m_opcode.rs == 0) { return "bal"; }
			return "bgezal";
		case R4300i_REGIMM_BLTZALL: return "bltzall";
		case R4300i_REGIMM_BGEZALL: return "bgezall";
		}
		break;
	case R4300i_J: return "j";
	case R4300i_JAL: return "jal";
	case R4300i_BEQ:
		if (m_opcode.rs == 0 && m_opcode.rt == 0) {
			return "b";
		} else if (m_opcode.rs == 0 || m_opcode.rt == 0) {
			return "beqz";
		} else {
			return "beq";
		}
		break;
	case R4300i_BNE:
		if ((m_opcode.rs == 0) ^ (m_opcode.rt == 0)){
			return "bnez";
		} else {
			return "bne";
		}
		break;
	case R4300i_BLEZ: return "blez";
	case R4300i_BGTZ: return "bgtz";
	case R4300i_ADDI: return "addi";
	case R4300i_ADDIU: return "addiu";
	case R4300i_SLTI: return "slti";
	case R4300i_SLTIU: return "sltiu";
	case R4300i_ANDI: return "andi";
	case R4300i_ORI: return "ori";
	case R4300i_XORI: return "xori";
	case R4300i_LUI: return "lui";
	case R4300i_CP0:
		switch (m_opcode.rs) {
		case R4300i_COP0_MF: return "mfc0";
		case R4300i_COP0_MT: return "mtc0";
		default:
			if ( (m_opcode.rs & 0x10 ) != 0 ) {
				switch( m_opcode.funct ) {
				case R4300i_COP0_CO_TLBR:  return "tlbr";
				case R4300i_COP0_CO_TLBWI: return "tlbwi";
				case R4300i_COP0_CO_TLBWR: return "tlbwr";
				case R4300i_COP0_CO_TLBP:  return "tlbp";
				case R4300i_COP0_CO_ERET:  return "eret";
				}
			}
			break;
		}
		break;
	case R4300i_CP1:
		switch (m_opcode.fmt) {
		case R4300i_COP1_MF:  return "mfc1";
		case R4300i_COP1_DMF: return "dmfc1";
		case R4300i_COP1_CF:  return "cfc1";
		case R4300i_COP1_MT:  return "mtc1";
		case R4300i_COP1_DMT: return "dmtc1";
		case R4300i_COP1_CT:  return "ctc1";
		case R4300i_COP1_BC:
			switch (m_opcode.ft) {
			case R4300i_COP1_BC_BCF:  return "BC1F";
			case R4300i_COP1_BC_BCT:  return "BC1T";
			case R4300i_COP1_BC_BCFL: return "BC1FL";
			case R4300i_COP1_BC_BCTL: return "BC1TL";
			}
			break;
		case R4300i_COP1_S:
			switch (m_opcode.funct) {			
			case R4300i_COP1_FUNCT_ADD:     return "ADD.S";
			case R4300i_COP1_FUNCT_SUB:     return "SUB.S";
			case R4300i_COP1_FUNCT_MUL:     return "MUL.S";
			case R4300i_COP1_FUNCT_DIV:     return "DIV.S";
			case R4300i_COP1_FUNCT_SQRT:    return "SQRT.S";
			case R4300i_COP1_FUNCT_ABS:     return "ABS.S";
			case R4300i_COP1_FUNCT_MOV:     return "MOV.S";
			case R4300i_COP1_FUNCT_NEG:     return "NEG.S";
			case R4300i_COP1_FUNCT_ROUND_L: return "ROUND.S";
			case R4300i_COP1_FUNCT_TRUNC_L: return "TRUNC.L.S";
			case R4300i_COP1_FUNCT_CEIL_L:  return "CEIL.L.S";
			case R4300i_COP1_FUNCT_FLOOR_L: return "FLOOR.L.S";
			case R4300i_COP1_FUNCT_ROUND_W: return "ROUND.W.S";
			case R4300i_COP1_FUNCT_TRUNC_W: return "TRUNC.W.S";
			case R4300i_COP1_FUNCT_CEIL_W:  return "CEIL.W.S";
			case R4300i_COP1_FUNCT_FLOOR_W: return "FLOOR.W.S";
			case R4300i_COP1_FUNCT_CVT_S:   return "CVT.S.S";
			case R4300i_COP1_FUNCT_CVT_D:   return "CVT.D.S";
			case R4300i_COP1_FUNCT_CVT_W:   return "CVT.W.S";
			case R4300i_COP1_FUNCT_CVT_L:   return "CVT.L.S";
			case R4300i_COP1_FUNCT_C_F:     return "C.F.S";
			case R4300i_COP1_FUNCT_C_UN:    return "C.UN.S";
			case R4300i_COP1_FUNCT_C_EQ:    return "C.EQ.S";
			case R4300i_COP1_FUNCT_C_UEQ:   return "C.UEQ.S";
			case R4300i_COP1_FUNCT_C_OLT:   return "C.OLT.S";
			case R4300i_COP1_FUNCT_C_ULT:   return "C.ULT.S";
			case R4300i_COP1_FUNCT_C_OLE:   return "C.OLE.S";
			case R4300i_COP1_FUNCT_C_ULE:   return "C.ULE.S";
			case R4300i_COP1_FUNCT_C_SF:    return "C.SF.S";
			case R4300i_COP1_FUNCT_C_NGLE:  return "C.NGLE.S";
			case R4300i_COP1_FUNCT_C_SEQ:   return "C.SEQ.S";
			case R4300i_COP1_FUNCT_C_NGL:   return "C.NGL.S";
			case R4300i_COP1_FUNCT_C_LT:    return "C.LT.S";
			case R4300i_COP1_FUNCT_C_NGE:   return "C.NGE.S";
			case R4300i_COP1_FUNCT_C_LE:    return "C.LE.S";
			case R4300i_COP1_FUNCT_C_NGT:   return "C.NGT.S";
			}
			break;
		case R4300i_COP1_D:
			switch (m_opcode.funct) {			
			case R4300i_COP1_FUNCT_ADD:     return "ADD.D";
			case R4300i_COP1_FUNCT_SUB:     return "SUB.D";
			case R4300i_COP1_FUNCT_MUL:     return "MUL.D";
			case R4300i_COP1_FUNCT_DIV:     return "DIV.D";
			case R4300i_COP1_FUNCT_SQRT:    return "SQRT.D";
			case R4300i_COP1_FUNCT_ABS:     return "ABS.D";
			case R4300i_COP1_FUNCT_MOV:     return "MOV.D";
			case R4300i_COP1_FUNCT_NEG:     return "NEG.D";
			case R4300i_COP1_FUNCT_ROUND_L: return "ROUND.D";
			case R4300i_COP1_FUNCT_TRUNC_L: return "TRUNC.L.D";
			case R4300i_COP1_FUNCT_CEIL_L:  return "CEIL.L.D";
			case R4300i_COP1_FUNCT_FLOOR_L: return "FLOOR.L.D";
			case R4300i_COP1_FUNCT_ROUND_W: return "ROUND.W.D";
			case R4300i_COP1_FUNCT_TRUNC_W: return "TRUNC.W.D";
			case R4300i_COP1_FUNCT_CEIL_W:  return "CEIL.W.D";
			case R4300i_COP1_FUNCT_FLOOR_W: return "FLOOR.W.D";
			case R4300i_COP1_FUNCT_CVT_S:   return "CVT.D.D";
			case R4300i_COP1_FUNCT_CVT_D:   return "CVT.D.D";
			case R4300i_COP1_FUNCT_CVT_W:   return "CVT.W.D";
			case R4300i_COP1_FUNCT_CVT_L:   return "CVT.L.D";
			case R4300i_COP1_FUNCT_C_F:     return "C.F.D";
			case R4300i_COP1_FUNCT_C_UN:    return "C.UN.D";
			case R4300i_COP1_FUNCT_C_EQ:    return "C.EQ.D";
			case R4300i_COP1_FUNCT_C_UEQ:   return "C.UEQ.D";
			case R4300i_COP1_FUNCT_C_OLT:   return "C.OLT.D";
			case R4300i_COP1_FUNCT_C_ULT:   return "C.ULT.D";
			case R4300i_COP1_FUNCT_C_OLE:   return "C.OLE.D";
			case R4300i_COP1_FUNCT_C_ULE:   return "C.ULE.D";
			case R4300i_COP1_FUNCT_C_SF:    return "C.DF.D";
			case R4300i_COP1_FUNCT_C_NGLE:  return "C.NGLE.D";
			case R4300i_COP1_FUNCT_C_SEQ:   return "C.DEQ.D";
			case R4300i_COP1_FUNCT_C_NGL:   return "C.NGL.D";
			case R4300i_COP1_FUNCT_C_LT:    return "C.LT.D";
			case R4300i_COP1_FUNCT_C_NGE:   return "C.NGE.D";
			case R4300i_COP1_FUNCT_C_LE:    return "C.LE.D";
			case R4300i_COP1_FUNCT_C_NGT:   return "C.NGT.D";
			}
			break;
		case R4300i_COP1_W:
			switch (m_opcode.funct) {			
			case R4300i_COP1_FUNCT_ADD:     return "ADD.W";
			case R4300i_COP1_FUNCT_SUB:     return "SUB.W";
			case R4300i_COP1_FUNCT_MUL:     return "MUL.W";
			case R4300i_COP1_FUNCT_DIV:     return "DIV.W";
			case R4300i_COP1_FUNCT_SQRT:    return "SQRT.W";
			case R4300i_COP1_FUNCT_ABS:     return "ABS.W";
			case R4300i_COP1_FUNCT_MOV:     return "MOV.W";
			case R4300i_COP1_FUNCT_NEG:     return "NEG.W";
			case R4300i_COP1_FUNCT_ROUND_L: return "ROUND.W";
			case R4300i_COP1_FUNCT_TRUNC_L: return "TRUNC.L.W";
			case R4300i_COP1_FUNCT_CEIL_L:  return "CEIL.L.W";
			case R4300i_COP1_FUNCT_FLOOR_L: return "FLOOR.L.W";
			case R4300i_COP1_FUNCT_ROUND_W: return "ROUND.W.W";
			case R4300i_COP1_FUNCT_TRUNC_W: return "TRUNC.W.W";
			case R4300i_COP1_FUNCT_CEIL_W:  return "CEIL.W.W";
			case R4300i_COP1_FUNCT_FLOOR_W: return "FLOOR.W.W";
			case R4300i_COP1_FUNCT_CVT_S:   return "CVT.W.W";
			case R4300i_COP1_FUNCT_CVT_D:   return "CVT.D.W";
			case R4300i_COP1_FUNCT_CVT_W:   return "CVT.W.W";
			case R4300i_COP1_FUNCT_CVT_L:   return "CVT.L.W";
			case R4300i_COP1_FUNCT_C_F:     return "C.F.W";
			case R4300i_COP1_FUNCT_C_UN:    return "C.UN.W";
			case R4300i_COP1_FUNCT_C_EQ:    return "C.EQ.W";
			case R4300i_COP1_FUNCT_C_UEQ:   return "C.UEQ.W";
			case R4300i_COP1_FUNCT_C_OLT:   return "C.OLT.W";
			case R4300i_COP1_FUNCT_C_ULT:   return "C.ULT.W";
			case R4300i_COP1_FUNCT_C_OLE:   return "C.OLE.W";
			case R4300i_COP1_FUNCT_C_ULE:   return "C.ULE.W";
			case R4300i_COP1_FUNCT_C_SF:    return "C.WF.W";
			case R4300i_COP1_FUNCT_C_NGLE:  return "C.NGLE.W";
			case R4300i_COP1_FUNCT_C_SEQ:   return "C.WEQ.W";
			case R4300i_COP1_FUNCT_C_NGL:   return "C.NGL.W";
			case R4300i_COP1_FUNCT_C_LT:    return "C.LT.W";
			case R4300i_COP1_FUNCT_C_NGE:   return "C.NGE.W";
			case R4300i_COP1_FUNCT_C_LE:    return "C.LE.W";
			case R4300i_COP1_FUNCT_C_NGT:   return "C.NGT.W";
			}
			break;
		case R4300i_COP1_L:
			switch (m_opcode.funct) {			
			case R4300i_COP1_FUNCT_ADD:     return "ADD.L";
			case R4300i_COP1_FUNCT_SUB:     return "SUB.L";
			case R4300i_COP1_FUNCT_MUL:     return "MUL.L";
			case R4300i_COP1_FUNCT_DIV:     return "DIV.L";
			case R4300i_COP1_FUNCT_SQRT:    return "SQRT.L";
			case R4300i_COP1_FUNCT_ABS:     return "ABS.L";
			case R4300i_COP1_FUNCT_MOV:     return "MOV.L";
			case R4300i_COP1_FUNCT_NEG:     return "NEG.L";
			case R4300i_COP1_FUNCT_ROUND_L: return "ROUND.L";
			case R4300i_COP1_FUNCT_TRUNC_L: return "TRUNC.L.L";
			case R4300i_COP1_FUNCT_CEIL_L:  return "CEIL.L.L";
			case R4300i_COP1_FUNCT_FLOOR_L: return "FLOOR.L.L";
			case R4300i_COP1_FUNCT_ROUND_W: return "ROUND.W.L";
			case R4300i_COP1_FUNCT_TRUNC_W: return "TRUNC.W.L";
			case R4300i_COP1_FUNCT_CEIL_W:  return "CEIL.W.L";
			case R4300i_COP1_FUNCT_FLOOR_W: return "FLOOR.W.L";
			case R4300i_COP1_FUNCT_CVT_S:   return "CVT.L.L";
			case R4300i_COP1_FUNCT_CVT_D:   return "CVT.D.L";
			case R4300i_COP1_FUNCT_CVT_W:   return "CVT.W.L";
			case R4300i_COP1_FUNCT_CVT_L:   return "CVT.L.L";
			case R4300i_COP1_FUNCT_C_F:     return "C.F.L";
			case R4300i_COP1_FUNCT_C_UN:    return "C.UN.L";
			case R4300i_COP1_FUNCT_C_EQ:    return "C.EQ.L";
			case R4300i_COP1_FUNCT_C_UEQ:   return "C.UEQ.L";
			case R4300i_COP1_FUNCT_C_OLT:   return "C.OLT.L";
			case R4300i_COP1_FUNCT_C_ULT:   return "C.ULT.L";
			case R4300i_COP1_FUNCT_C_OLE:   return "C.OLE.L";
			case R4300i_COP1_FUNCT_C_ULE:   return "C.ULE.L";
			case R4300i_COP1_FUNCT_C_SF:    return "C.LF.L";
			case R4300i_COP1_FUNCT_C_NGLE:  return "C.NGLE.L";
			case R4300i_COP1_FUNCT_C_SEQ:   return "C.LEQ.L";
			case R4300i_COP1_FUNCT_C_NGL:   return "C.NGL.L";
			case R4300i_COP1_FUNCT_C_LT:    return "C.LT.L";
			case R4300i_COP1_FUNCT_C_NGE:   return "C.NGE.L";
			case R4300i_COP1_FUNCT_C_LE:    return "C.LE.L";
			case R4300i_COP1_FUNCT_C_NGT:   return "C.NGT.L";
			}
			break;
		}
		break;
	case R4300i_BEQL:
		if (m_opcode.rs == m_opcode.rt) {
			return "b";
		} else if ((m_opcode.rs == 0) ^ (m_opcode.rt == 0)){
			return "beqzl";
		} else {
			return "beql";
		}
		break;
	case R4300i_BNEL:
		if ((m_opcode.rs == 0) ^ (m_opcode.rt == 0)){
			return "bnezl";
		} else {
			return "bnel";
		}
		break;
	case R4300i_BLEZL: return "blezl";
	case R4300i_BGTZL: return "bgtzl";
	case R4300i_DADDI: return "daddi";
	case R4300i_DADDIU: return "daddiu";
	case R4300i_LDL: return "ldl";
	case R4300i_LDR: return "ldr";
	case R4300i_LB: return "lb";
	case R4300i_LH: return "lh";
	case R4300i_LWL: return "lwl";
	case R4300i_LW: return "lw";
	case R4300i_LBU: return "lbu";
	case R4300i_LHU: return "lhu";
	case R4300i_LWR: return "lwr";
	case R4300i_LWU: return "lwu";
	case R4300i_SB: return "sb";
	case R4300i_SH: return "sh";
	case R4300i_SWL: return "swl";
	case R4300i_SW: return "sw";
	case R4300i_SDL: return "sdl";
	case R4300i_SDR: return "sdr";
	case R4300i_SWR: return "swr";
	case R4300i_CACHE: return "cache";
	case R4300i_LL: return "ll";
	case R4300i_LWC1: return "lwc1";
	case R4300i_LDC1: return "ldc1";
	case R4300i_LD: return "ld";
	case R4300i_SC: return "sc";
	case R4300i_SWC1: return "swc1";
	case R4300i_SDC1: return "sdc1";
	case R4300i_SD: return "sd";
	}
	return "Unknown";
}

void COpcodeAnalysis::OpcodeParam(char * CommandName) 
{
	sprintf(CommandName,"%02X %02X %02X %02X",m_opcode.Ascii[3],
		m_opcode.Ascii[2],m_opcode.Ascii[1],m_opcode.Ascii[0]);

	switch (m_opcode.op) {
	case R4300i_SPECIAL:
		switch (m_opcode.funct) {
		case R4300i_SPECIAL_SLL:
			if (m_opcode.Hex != 0) {
				sprintf(CommandName,"%s, %s, 0x%X",CRegName::GPR[m_opcode.rd],CRegName::GPR[m_opcode.rt], m_opcode.sa);
			} else {
				strcpy(CommandName,"");
			}
			break;
		case R4300i_SPECIAL_SRL:
		case R4300i_SPECIAL_SRA:
			sprintf(CommandName,"%s, %s, 0x%X",CRegName::GPR[m_opcode.rd], CRegName::GPR[m_opcode.rt],m_opcode.sa);
			break;
		case R4300i_SPECIAL_SLLV:
		case R4300i_SPECIAL_SRLV:
		case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_DSLLV:
		case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:
			sprintf(CommandName,"%s, %s, %s",CRegName::GPR[m_opcode.rd], CRegName::GPR[m_opcode.rt],
				CRegName::GPR[m_opcode.rs]);
			break;
		case R4300i_SPECIAL_JR:
			sprintf(CommandName,"%s",CRegName::GPR[m_opcode.rs]);
			break;
		case R4300i_SPECIAL_JALR:
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rd],CRegName::GPR[m_opcode.rs]);
			break;
		case R4300i_SPECIAL_SYNC: 
		case R4300i_SPECIAL_SYSCALL: 
		case R4300i_SPECIAL_BREAK: 
			strcpy(CommandName,"");
			break;
		case R4300i_SPECIAL_MFHI:
		case R4300i_SPECIAL_MFLO:
			sprintf(CommandName,"%s",CRegName::GPR[m_opcode.rd]);
			break;
		case R4300i_SPECIAL_MTHI:
		case R4300i_SPECIAL_MTLO:
			sprintf(CommandName,"%s",CRegName::GPR[m_opcode.rs]);
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rs], CRegName::GPR[m_opcode.rt]);
			break;
		case R4300i_SPECIAL_ADD:
		case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:
		case R4300i_SPECIAL_SUBU:
		case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:
		case R4300i_SPECIAL_XOR:
		case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
			sprintf(CommandName,"%s, %s, %s",CRegName::GPR[m_opcode.rd], CRegName::GPR[m_opcode.rs],
				CRegName::GPR[m_opcode.rt]);
			break;
		case R4300i_SPECIAL_TGE:
		case R4300i_SPECIAL_TGEU:
		case R4300i_SPECIAL_TLT:
		case R4300i_SPECIAL_TLTU:
		case R4300i_SPECIAL_TEQ:
		case R4300i_SPECIAL_TNE:
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rs],CRegName::GPR[m_opcode.rt]);
			break;
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
			sprintf(CommandName,"%s, %s, 0x%X",CRegName::GPR[m_opcode.rd],
				CRegName::GPR[m_opcode.rt], m_opcode.sa);
			break;
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			sprintf(CommandName,"%s, %s, 0x%X",CRegName::GPR[m_opcode.rd],CRegName::GPR[m_opcode.rt], m_opcode.sa);
			break;
		}
		break;
	case R4300i_J:
	case R4300i_JAL: 
		sprintf(CommandName,"%s",_Labels->LabelName((m_opcode.VirtualAddress & 0xF0000000) + (m_opcode.target << 2)));
		break;
	case R4300i_BEQ:
		if (m_opcode.rs == 0 && m_opcode.rt == 0) {
			sprintf(CommandName,"%s", _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		} else if (m_opcode.rs == 0 || m_opcode.rt == 0) {
			sprintf(CommandName,"%s, %s", CRegName::GPR[m_opcode.rs == 0 ? m_opcode.rt : m_opcode.rs ],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		} else {
			sprintf(CommandName,"%s, %s, %s", CRegName::GPR[m_opcode.rs], CRegName::GPR[m_opcode.rt],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		}
		break;
	case R4300i_BNE:
		if ((m_opcode.rs == 0) ^ (m_opcode.rt == 0)){
			sprintf(CommandName,"%s, %s", CRegName::GPR[m_opcode.rs == 0 ? m_opcode.rt : m_opcode.rs ],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		} else {
			sprintf(CommandName,"%s, %s, %s", CRegName::GPR[m_opcode.rs], CRegName::GPR[m_opcode.rt],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		}
		break;
	case R4300i_REGIMM:
		switch (m_opcode.rt) {
		case R4300i_REGIMM_BLTZ: 
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BLTZALL:
		case R4300i_REGIMM_BGEZALL:
			sprintf(CommandName,"%s, %s", CRegName::GPR[m_opcode.rs], _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
			break;
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BGEZAL:
			if (m_opcode.rs == 0) { 
				sprintf(CommandName,"%s", _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
			} else {
				sprintf(CommandName,"%s, %s", CRegName::GPR[m_opcode.rs], _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
			}
			break;
		case R4300i_REGIMM_TGEI: 
		case R4300i_REGIMM_TGEIU:
		case R4300i_REGIMM_TLTI:
		case R4300i_REGIMM_TLTIU:
		case R4300i_REGIMM_TEQI: 
		case R4300i_REGIMM_TNEI: 
			sprintf(CommandName,"%s, 0x%X",CRegName::GPR[m_opcode.rs],m_opcode.immediate);
			break;
		}
		break;
	case R4300i_BLEZ:
	case R4300i_BGTZ:
		sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rs], _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		break;
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
		if (m_opcode.rt == m_opcode.rs) {
			sprintf(CommandName,"%s, 0x%X",CRegName::GPR[m_opcode.rt], m_opcode.immediate);
		} else {
			sprintf(CommandName,"%s, %s, 0x%X",CRegName::GPR[m_opcode.rt], CRegName::GPR[m_opcode.rs],m_opcode.immediate);
		}
		break;
	case R4300i_LUI:
		sprintf(CommandName,"%s, 0x%X",CRegName::GPR[m_opcode.rt], m_opcode.immediate);
		break;
	case R4300i_CP0:
		switch (m_opcode.rs) {
		case R4300i_COP0_MF:
		case R4300i_COP0_MT:
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rt], CRegName::Cop0[m_opcode.rd]);
			break;
		default:
			if ( (m_opcode.rs & 0x10 ) != 0 ) {
				switch( m_opcode.funct ) {
				case R4300i_COP0_CO_TLBR:  strcpy(CommandName,""); break;
				case R4300i_COP0_CO_TLBWI: strcpy(CommandName,""); break;
				case R4300i_COP0_CO_TLBWR: strcpy(CommandName,""); break;
				case R4300i_COP0_CO_TLBP:  strcpy(CommandName,""); break;
				case R4300i_COP0_CO_ERET:  strcpy(CommandName,""); break;
				}
			}
			break;
		}
		break;
	case R4300i_CP1:
	switch (m_opcode.fmt) {
		case R4300i_COP1_MF:
		case R4300i_COP1_DMF:
		case R4300i_COP1_MT:
		case R4300i_COP1_DMT:
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rt], CRegName::FPR[m_opcode.fs]);
			break;
		case R4300i_COP1_CF:
		case R4300i_COP1_CT:
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rt], CRegName::FPR_Ctrl[m_opcode.fs]);
			break;
		case R4300i_COP1_BC:
			switch (m_opcode.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				sprintf(CommandName,"%s",_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
				break;
			}
			break;
		case R4300i_COP1_S:
		case R4300i_COP1_D:
		case R4300i_COP1_W:
		case R4300i_COP1_L:
			switch (m_opcode.funct) {			
			case R4300i_COP1_FUNCT_ADD:
			case R4300i_COP1_FUNCT_SUB:
			case R4300i_COP1_FUNCT_MUL:
			case R4300i_COP1_FUNCT_DIV:
				sprintf(CommandName,"%s, %s, %s",CRegName::FPR[m_opcode.fd], CRegName::FPR[m_opcode.fs], 
					CRegName::FPR[m_opcode.ft]);
				break;
			case R4300i_COP1_FUNCT_SQRT:
			case R4300i_COP1_FUNCT_ABS:
			case R4300i_COP1_FUNCT_MOV:
			case R4300i_COP1_FUNCT_NEG:
			case R4300i_COP1_FUNCT_ROUND_L:
			case R4300i_COP1_FUNCT_TRUNC_L:
			case R4300i_COP1_FUNCT_CEIL_L:
			case R4300i_COP1_FUNCT_FLOOR_L:
			case R4300i_COP1_FUNCT_ROUND_W:
			case R4300i_COP1_FUNCT_TRUNC_W:
			case R4300i_COP1_FUNCT_CEIL_W:
			case R4300i_COP1_FUNCT_FLOOR_W:
			case R4300i_COP1_FUNCT_CVT_S:
			case R4300i_COP1_FUNCT_CVT_D:
			case R4300i_COP1_FUNCT_CVT_W:
			case R4300i_COP1_FUNCT_CVT_L:
				sprintf(CommandName,"%s, %s",CRegName::FPR[m_opcode.fd], CRegName::FPR[m_opcode.fs]);
				break;
			case R4300i_COP1_FUNCT_C_F:
			case R4300i_COP1_FUNCT_C_UN:
			case R4300i_COP1_FUNCT_C_EQ:
			case R4300i_COP1_FUNCT_C_UEQ:
			case R4300i_COP1_FUNCT_C_OLT:
			case R4300i_COP1_FUNCT_C_ULT:
			case R4300i_COP1_FUNCT_C_OLE:
			case R4300i_COP1_FUNCT_C_ULE:
			case R4300i_COP1_FUNCT_C_SF:
			case R4300i_COP1_FUNCT_C_NGLE:
			case R4300i_COP1_FUNCT_C_SEQ:
			case R4300i_COP1_FUNCT_C_NGL:
			case R4300i_COP1_FUNCT_C_LT:
			case R4300i_COP1_FUNCT_C_NGE:
			case R4300i_COP1_FUNCT_C_LE:
			case R4300i_COP1_FUNCT_C_NGT:
				sprintf(CommandName,"%s, %s",CRegName::FPR[m_opcode.fs], CRegName::FPR[m_opcode.ft]);
				break;
			}
		}
		break;
	case R4300i_BEQL:
		if (m_opcode.rs == m_opcode.rt) {
			sprintf(CommandName,"%s", _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		} else if ((m_opcode.rs == 0) ^ (m_opcode.rt == 0)){
			sprintf(CommandName,"%s, %s", CRegName::GPR[m_opcode.rs == 0 ? m_opcode.rt : m_opcode.rs ],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		} else {
			sprintf(CommandName,"%s, %s, %s", CRegName::GPR[m_opcode.rs], CRegName::GPR[m_opcode.rt],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		}
		break;
	case R4300i_BNEL:
		if ((m_opcode.rs == 0) ^ (m_opcode.rt == 0)){
			sprintf(CommandName,"%s, %s", CRegName::GPR[m_opcode.rs == 0 ? m_opcode.rt : m_opcode.rs ],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		} else {
			sprintf(CommandName,"%s, %s, %s", CRegName::GPR[m_opcode.rs], CRegName::GPR[m_opcode.rt],
				_Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		}
		break;
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rs], _Labels->LabelName(m_opcode.VirtualAddress + ((short)m_opcode.offset << 2) + 4));
		break;
	case R4300i_DADDI:
	case R4300i_DADDIU:
		sprintf(CommandName,"%s, %s, 0x%X",CRegName::GPR[m_opcode.rt], CRegName::GPR[m_opcode.rs],m_opcode.immediate);
		break;
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LWL:
	case R4300i_LW:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LWR:
	case R4300i_LWU:
	case R4300i_SB:
	case R4300i_SH:
	case R4300i_SWL:
	case R4300i_SW:
	case R4300i_SDL:
	case R4300i_SDR:
	case R4300i_SWR:
	case R4300i_LL:
	case R4300i_LD:
	case R4300i_SD:
	case R4300i_SC:
		if (m_opcode.offset == 0) {
			sprintf(CommandName,"%s, %s",CRegName::GPR[m_opcode.rt], CRegName::GPR[m_opcode.base]);
		} else {
			sprintf(CommandName,"%s, 0x%X (%s)",CRegName::GPR[m_opcode.rt], m_opcode.offset, CRegName::GPR[m_opcode.base]);
		}
		break;
	case R4300i_CACHE:
		sprintf(CommandName,"%d, 0x%X (%s)",m_opcode.rt, m_opcode.offset, CRegName::GPR[m_opcode.base]);
		break;
	case R4300i_LWC1:
	case R4300i_LDC1:
	case R4300i_SWC1:
	case R4300i_SDC1:
		sprintf(CommandName,"%s, 0x%X (%s)",CRegName::FPR[m_opcode.rt], m_opcode.offset, CRegName::GPR[m_opcode.base]);
		break;
	}
}



#endif