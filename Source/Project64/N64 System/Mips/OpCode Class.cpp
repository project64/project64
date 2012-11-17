#include "stdafx.h"

#ifdef toremove
QWORD const COpcode::LDL_MASK[8] = { 0x0000000000000000,0x00000000000000FF,0x000000000000FFFF,
                               0x0000000000FFFFFF,0x00000000FFFFFFFF,0x000000FFFFFFFFFF,
							   0x0000FFFFFFFFFFFF,0x00FFFFFFFFFFFFFF };
QWORD const COpcode::LDR_MASK[8] = { 0xFFFFFFFFFFFFFF00,0xFFFFFFFFFFFF0000,0xFFFFFFFFFF000000,
                               0xFFFFFFFF00000000,0xFFFFFF0000000000,0xFFFF000000000000,
                               0xFF00000000000000,0x0000000000000000 };
QWORD const COpcode::SDL_MASK[8] = { 0x0000000000000000,0xFF00000000000000,0xFFFF000000000000,
                               0xFFFFFF0000000000,0xFFFFFFFF00000000,0xFFFFFFFFFF000000,
                               0xFFFFFFFFFFFF0000,0xFFFFFFFFFFFFFF00 };
QWORD const COpcode::SDR_MASK[8] = { 0x00FFFFFFFFFFFFFF,0x0000FFFFFFFFFFFF,0x000000FFFFFFFFFF,
                               0x00000000FFFFFFFF,0x0000000000FFFFFF,0x000000000000FFFF,
                               0x00000000000000FF,0x0000000000000000 };
int   const COpcode::DL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
int   const COpcode::DR_SHIFT[8] = { 56,48,40, 32, 24, 16, 8,  0 };

DWORD const COpcode::LWL_MASK[4] = { 0x00000000,0x000000FF,0x0000FFFF,0x00FFFFFF };
DWORD const COpcode::LWR_MASK[4] = { 0xFFFFFF00,0xFFFF0000,0xFF000000,0x00000000 };
DWORD const COpcode::SWL_MASK[4] = { 0x00000000,0xFF000000,0xFFFF0000,0xFFFFFF00 };
DWORD const COpcode::SWR_MASK[4] = { 0x00FFFFFF,0x0000FFFF,0x000000FF,0x00000000 };
int   const COpcode::WL_SHIFT[4] = {  0,  8, 16, 24 };
int   const COpcode::WR_SHIFT[4] = { 24, 16 , 8,  0 };

COpcode::COpcode ( DWORD VirtualAddress ):
	COpcodeAnalysis(m_opcode),
	m_OpLen(OpCode_Size),
	m_OpcodeCount(_Settings->LoadDword(Game_CounterFactor)),
	m_FixedOpcodeCount(_Settings->LoadDword(Game_CounterFactor) != 0)	
{	
	//setup details about handling opcodes
	m_NextStep     = StepNormal;
	m_JumpLocation = -1;

	//Flags
	m_FlagSet        = false;
	m_ExectionJumped = false;
	m_InPermLoop     = PermLoop_None;
	
	//set up the details about the current opcode
	m_opcode.VirtualAddress = VirtualAddress & ~3;
	if (!SetPC(m_opcode.VirtualAddress)) {
		g_Notify->BreakPoint(__FILE__,__LINE__);
	}
}

float COpcode::CycleCount  ( void ) {
	if (m_FixedOpcodeCount) {
		return m_OpcodeCount;
	}
	return 2;
}

bool COpcode::Next (void) {
	m_ExectionJumped = false;

	switch (m_NextStep) {
	case StepNormal: 
		m_opcode.VirtualAddress += m_OpLen;
		break;
	case StepDelaySlot:
		m_NextStep = StepJump;
		m_opcode.VirtualAddress += m_OpLen;
		break;
	case StepJump:
		m_opcode.VirtualAddress  = m_JumpLocation;
		m_NextStep = StepNormal;
		m_FlagSet        = true;
		m_ExectionJumped = true;
		break;
	//Compiler flags
	case DoDelaySlot:
		m_NextStep = InsideDelaySlot;
		m_opcode.VirtualAddress += m_OpLen;
		break;
	case InsideDelaySlot:
		m_NextStep = DelaySlotDone;
		m_opcode.VirtualAddress -= m_OpLen;
		break;
	case DelaySlotDone:
		m_NextStep = BranchCompiled;
		m_opcode.VirtualAddress += m_OpLen * 2;
		break;
	default:
		m_opcode.VirtualAddress += m_OpLen;
		g_Notify->BreakPoint(__FILE__,__LINE__);
	}

	if (!_MMU->LW_VAddr(m_opcode.VirtualAddress,m_opcode.Hex)) {
		return false;
	}
	return true;
}

void COpcode::SetJump ( DWORD Target, bool Delay ) {	
	if (Target == PC()) { 
		if (_MMU->ValidVaddr(Target + OpCode_Size)) {
			if (HasDelaySlot() && !DelaySlotEffectsJump()) {
				m_InPermLoop = PermLoop_Jump;
				m_FlagSet = true;
			}
		}
	}
	m_NextStep     = Delay ? StepDelaySlot : StepJump;
	m_JumpLocation = Target;
}

bool COpcode::SetPC ( DWORD VirtualAddress ) {	
	PERM_LOOP InPerm  = m_InPermLoop;
	bool FlagSet = m_FlagSet;
	m_opcode.VirtualAddress = -1; 
	SetJump(VirtualAddress,false);
	bool Result = Next();
	m_InPermLoop = InPerm;
	m_FlagSet = FlagSet;
	return Result;
}

#endif 
