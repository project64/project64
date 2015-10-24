/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

unsigned int CRegInfo::m_fpuControl = 0;

char *Format_Name[] = {"Unknown","dword","qword","float","double"};

CRegInfo::CRegInfo() :
	m_CycleCount(0),
	m_Stack_TopPos(0),
	m_Fpu_Used(false),
	m_RoundingModel(RoundUnknown)
{
	m_MIPS_RegState[0] = STATE_CONST_32_SIGN;
	m_MIPS_RegVal[0].DW = 0;
	m_RegMapLo[0] = x86_Unknown;
	m_RegMapHi[0] = x86_Unknown;
	
	for (int i = 1; i < 32; i++ ) 
	{
		m_MIPS_RegState[i]   = STATE_UNKNOWN;
		m_MIPS_RegVal[i].DW = 0;
		m_RegMapLo[i] = x86_Unknown;
		m_RegMapHi[i] = x86_Unknown;
	}
	for (int i = 0, n = sizeof(m_x86reg_MappedTo) / sizeof(m_x86reg_MappedTo[0]); i < n; i++ ) 
	{
		m_x86reg_MappedTo[i]  = NotMapped;
		m_x86reg_Protected[i] = false;
		m_x86reg_MapOrder[i]  = 0;
	}
	for (int i = 0, n = sizeof(x86fpu_MappedTo) / sizeof(x86fpu_MappedTo[0]); i < n; i++ ) 
	{
		x86fpu_MappedTo[i] = -1;
		x86fpu_State[i] = FPU_Unknown;
		x86fpu_StateChanged[i] = false;
		x86fpu_RoundingModel[i] = RoundDefault;
	}
}

CRegInfo::CRegInfo(const CRegInfo& rhs)
{
	*this = rhs;
}

CRegInfo::~CRegInfo()
{
}

CRegInfo& CRegInfo::operator=(const CRegInfo& right)
{
	m_CycleCount = right.m_CycleCount;
	m_Stack_TopPos = right.m_Stack_TopPos;
	m_Fpu_Used = right.m_Fpu_Used;
	m_RoundingModel = right.m_RoundingModel;

	memcpy(&m_MIPS_RegState,&right.m_MIPS_RegState,sizeof(m_MIPS_RegState));
	memcpy(&m_MIPS_RegVal,&right.m_MIPS_RegVal,sizeof(m_MIPS_RegVal));
	memcpy(&m_RegMapLo,&right.m_RegMapLo,sizeof(m_RegMapLo));
	memcpy(&m_RegMapHi,&right.m_RegMapHi,sizeof(m_RegMapHi));
	memcpy(&m_x86reg_MappedTo,&right.m_x86reg_MappedTo,sizeof(m_x86reg_MappedTo));
	memcpy(&m_x86reg_Protected,&right.m_x86reg_Protected,sizeof(m_x86reg_Protected));
	memcpy(&m_x86reg_MapOrder,&right.m_x86reg_MapOrder,sizeof(m_x86reg_MapOrder));

	memcpy(&x86fpu_MappedTo,&right.x86fpu_MappedTo,sizeof(x86fpu_MappedTo));
	memcpy(&x86fpu_State,&right.x86fpu_State,sizeof(x86fpu_State));
	memcpy(&x86fpu_StateChanged,&right.x86fpu_StateChanged,sizeof(x86fpu_StateChanged));
	memcpy(&x86fpu_RoundingModel,&right.x86fpu_RoundingModel,sizeof(x86fpu_RoundingModel));

#ifdef _DEBUG
	if (*this != right)
	{
		g_Notify->BreakPoint(__FILEW__,__LINE__);
	}
#endif
	return *this;
}

bool CRegInfo::operator==(const CRegInfo& right) const
{
	int count;

	for (count = 0; count < 32; count ++ ) {
		if (m_MIPS_RegState[count] != right.m_MIPS_RegState[count]) 
		{
			return false; 
		}
		if (m_MIPS_RegState[count] == STATE_UNKNOWN)
		{
			continue;
		}
		if (m_MIPS_RegVal[count].DW != right.m_MIPS_RegVal[count].DW) 
		{
			return false; 
		}
	}
	for (count = 0; count < 10; count ++ ) {
		if (m_x86reg_MappedTo[count] != right.m_x86reg_MappedTo[count]) { return false; }
		if (m_x86reg_Protected[count] != right.m_x86reg_Protected[count]) { return false; }
		if (m_x86reg_MapOrder[count]  != right.m_x86reg_MapOrder[count]) { return false; }
	}
	if (m_CycleCount != right.m_CycleCount) { return false; }
	if (m_Stack_TopPos != right.m_Stack_TopPos) { return false; }

	for (count = 0; count < 8; count ++ ) {
		if (x86fpu_MappedTo[count]  != right.x86fpu_MappedTo[count]) { return false; }
		if (x86fpu_State[count]  != right.x86fpu_State[count]) { return false; }
		if (x86fpu_RoundingModel[count]  != right.x86fpu_RoundingModel[count]) { return false; }
	}
	if (m_Fpu_Used != right.m_Fpu_Used) { return false; }
	if (GetRoundingModel() != right.GetRoundingModel()) { return false; }
	return true;
}

bool CRegInfo::operator!=(const CRegInfo& right) const
{
	return !(right == *this);
}

CRegInfo::REG_STATE CRegInfo::ConstantsType (__int64 Value) 
{
	if (((Value >> 32) == -1) && ((Value & 0x80000000) != 0)) { return STATE_CONST_32_SIGN; } 
	if (((Value >> 32) == 0) && ((Value & 0x80000000) == 0)) { return STATE_CONST_32_SIGN; } 
	return STATE_CONST_64;
}

void CRegInfo::FixRoundModel(FPU_ROUND RoundMethod )
{
	if (GetRoundingModel() == RoundMethod) 
	{
		return;
	}
	CPU_Message("    FixRoundModel: CurrentRoundingModel: %s  targetRoundModel: %s",RoundingModelName(GetRoundingModel()),RoundingModelName(RoundMethod));

	m_fpuControl = 0;
	fpuStoreControl(&m_fpuControl, "m_fpuControl");
	x86Reg reg = Map_TempReg(x86_Any, -1, false);
	MoveVariableToX86reg(&m_fpuControl, "m_fpuControl", reg);
	AndConstToX86Reg(reg, 0xF3FF);

	if (RoundMethod == RoundDefault)
	{
		x86Reg RoundReg = Map_TempReg(x86_Any, -1, false);
		MoveVariableToX86reg(&g_Reg->m_RoundingModel,"m_RoundingModel", RoundReg);
		ShiftLeftSignImmed(RoundReg,2);
		OrX86RegToX86Reg(reg,RoundReg);
		SetX86Protected(RoundReg,false);
	} else {
		switch (RoundMethod) {
		case RoundTruncate: OrConstToX86Reg(0x0C00, reg); break;
		case RoundNearest: /*OrConstToX86Reg(0x0000, reg);*/ break;
		case RoundDown:     OrConstToX86Reg(0x0400, reg); break;
		case RoundUp:       OrConstToX86Reg(0x0800, reg); break;
		default:
			g_Notify->DisplayError(L"Unknown Rounding model");
		}
	}
	MoveX86regToVariable(reg, &m_fpuControl, "m_fpuControl");
	SetX86Protected(reg,false);
	fpuLoadControl(&m_fpuControl, "m_fpuControl");
	SetRoundingModel(RoundMethod);
}

void CRegInfo::ChangeFPURegFormat (int Reg, FPU_STATE OldFormat, FPU_STATE NewFormat, FPU_ROUND RoundingModel)
{
	for (DWORD i = 0; i < 8; i++) 
	{
		if (x86fpu_MappedTo[i] != Reg) 
		{
			continue;
		}
		if (x86fpu_State[i] != OldFormat || x86fpu_StateChanged[i])
		{
			UnMap_FPR(Reg, true);
			Load_FPR_ToTop(Reg,Reg,OldFormat);
		} else {
			CPU_Message("    regcache: Changed format of ST(%d) from %s to %s", (i - StackTopPos() + 8) & 7,Format_Name[OldFormat],Format_Name[NewFormat]);			
		}
		FpuRoundingModel(i)    = RoundingModel;
		x86fpu_State[i]        = NewFormat;
		x86fpu_StateChanged[i] = true;
		return;
	}

	if (bHaveDebugger())
	{
		g_Notify->DisplayError(L"ChangeFormat: Register not on stack!!");
	}
}

void CRegInfo::Load_FPR_ToTop ( int Reg, int RegToLoad, FPU_STATE Format) 
{
	if (GetRoundingModel() != RoundDefault)
	{
		FixRoundModel(RoundDefault);
	}
	CPU_Message("CurrentRoundingModel: %s  FpuRoundingModel(StackTopPos()): %s",RoundingModelName(GetRoundingModel()),RoundingModelName(FpuRoundingModel(StackTopPos())));
	int i;

	if (RegToLoad < 0) { g_Notify->DisplayError(L"Load_FPR_ToTop\nRegToLoad < 0 ???"); return; }
	if (Reg < 0) { g_Notify->DisplayError(L"Load_FPR_ToTop\nReg < 0 ???"); return; }

	if (Format == FPU_Double || Format == FPU_Qword) {
		UnMap_FPR(Reg + 1, true);
		UnMap_FPR(RegToLoad + 1, true);
	} else {
		if ((Reg & 1) != 0) {
			for (i = 0; i < 8; i++) {
				if (x86fpu_MappedTo[i] == (Reg - 1)) {
					if (x86fpu_State[i] == FPU_Double || x86fpu_State[i] == FPU_Qword) {
						UnMap_FPR(Reg, true);
					}
					i = 8;
				}
			}		
		}
		if ((RegToLoad & 1) != 0) {
			for (i = 0; i < 8; i++) {
				if (x86fpu_MappedTo[i] == (RegToLoad - 1)) {
					if (x86fpu_State[i] == FPU_Double || x86fpu_State[i] == FPU_Qword) {
						UnMap_FPR(RegToLoad, true);
					}
					i = 8;
				}
			}		
		}
	}

	if (Reg == RegToLoad) {
		//if different format then unmap original reg from stack
		for (i = 0; i < 8; i++) {
			if (x86fpu_MappedTo[i] != Reg) 
			{
				continue;
			}
			if (x86fpu_State[i] != Format) {
				UnMap_FPR(Reg, true);
			}
			break;
		}
	} else {
		//if different format then unmap original reg from stack
		for (i = 0; i < 8; i++) 
		{
			if (x86fpu_MappedTo[i] != Reg) 
			{
				continue;
			}
			UnMap_FPR(Reg,x86fpu_State[i] != Format);
			break;
		}
	}

	if (RegInStack(RegToLoad,Format)) {
		if (Reg != RegToLoad) {
			if (x86fpu_MappedTo[(StackTopPos() - 1) & 7] != RegToLoad) {
				UnMap_FPR(x86fpu_MappedTo[(StackTopPos() - 1) & 7], true);
				CPU_Message("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);
				fpuLoadReg(&StackTopPos(),StackPosition(RegToLoad));
				FpuRoundingModel(StackTopPos())    = RoundDefault;
				x86fpu_MappedTo[StackTopPos()]     = Reg;
				x86fpu_State[StackTopPos()]        = Format;
				x86fpu_StateChanged[StackTopPos()] = false;
			} else {
				UnMap_FPR(x86fpu_MappedTo[(StackTopPos() - 1) & 7], true);
				Load_FPR_ToTop (Reg, RegToLoad, Format);
			}
		} else {
			x86FpuValues RegPos = x86_ST_Unknown;
			for (DWORD i = 0; i < 8; i++) {
				if (x86fpu_MappedTo[i] == Reg) {
					RegPos = (x86FpuValues)i;
					i = 8;
				}
			}

			if (RegPos == StackTopPos()) {
				return;
			}
			x86FpuValues StackPos = StackPosition(Reg);

			FpuRoundingModel(RegPos) = FpuRoundingModel(StackTopPos());
			x86fpu_MappedTo[RegPos]  = x86fpu_MappedTo[StackTopPos()];
			x86fpu_State[RegPos]     = x86fpu_State[StackTopPos()];
			x86fpu_StateChanged[RegPos] = x86fpu_StateChanged[StackTopPos()];
			CPU_Message("    regcache: allocate ST(%d) to %s", StackPos,CRegName::FPR[x86fpu_MappedTo[RegPos]]);
			CPU_Message("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);

			fpuExchange(StackPos);

			FpuRoundingModel(StackTopPos()) = RoundDefault;
			x86fpu_MappedTo[StackTopPos()]      = Reg;
			x86fpu_State[StackTopPos()]         = Format;
			x86fpu_StateChanged[StackTopPos()]  = false;
		}
	} else {
		char Name[50];
		x86Reg TempReg;

		UnMap_FPR(x86fpu_MappedTo[(StackTopPos() - 1) & 7], true);
		for (i = 0; i < 8; i++) {
			if (x86fpu_MappedTo[i] == RegToLoad) {
				UnMap_FPR(RegToLoad, true);
				i = 8;
			}
		}
		CPU_Message("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);
		TempReg = Map_TempReg(x86_Any, -1, false);
		switch (Format) {
		case FPU_Dword:
			sprintf(Name,"m_FPR_S[%d]",RegToLoad);
			MoveVariableToX86reg(&g_Reg->m_FPR_S[RegToLoad],Name,TempReg);
			fpuLoadIntegerDwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		case FPU_Qword:
			sprintf(Name,"m_FPR_D[%d]",RegToLoad);
			MoveVariableToX86reg(&g_Reg->m_FPR_D[RegToLoad],Name,TempReg);
			fpuLoadIntegerQwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		case FPU_Float:
			sprintf(Name,"m_FPR_S[%d]",RegToLoad);
			MoveVariableToX86reg(&g_Reg->m_FPR_S[RegToLoad],Name,TempReg);
			fpuLoadDwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		case FPU_Double:
			sprintf(Name,"m_FPR_D[%d]",RegToLoad);
			MoveVariableToX86reg(&g_Reg->m_FPR_D[RegToLoad],Name,TempReg);
			fpuLoadQwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		default:
			if (bHaveDebugger()) { g_Notify->DisplayError(L"Load_FPR_ToTop\nUnkown format to load %d",Format); }
		}
		SetX86Protected(TempReg, false);
		FpuRoundingModel(StackTopPos()) = RoundDefault;
		x86fpu_MappedTo[StackTopPos()]      = Reg;
		x86fpu_State[StackTopPos()]         = Format;
		x86fpu_StateChanged[StackTopPos()]  = false;
	}
}

CRegInfo::x86FpuValues CRegInfo::StackPosition (int Reg) 
{
	int i;

	for (i = 0; i < 8; i++) {
		if (x86fpu_MappedTo[i] == Reg) {
			return (x86FpuValues)((i - StackTopPos()) & 7);
		}
	}
	return x86_ST_Unknown;
}

CX86Ops::x86Reg CRegInfo::FreeX86Reg()
{
	if (GetX86Mapped(x86_EDI) == NotMapped && !GetX86Protected(x86_EDI)) { return x86_EDI; }
	if (GetX86Mapped(x86_ESI) == NotMapped && !GetX86Protected(x86_ESI)) { return x86_ESI; }
	if (GetX86Mapped(x86_EBX) == NotMapped && !GetX86Protected(x86_EBX)) { return x86_EBX; }
	if (GetX86Mapped(x86_EAX) == NotMapped && !GetX86Protected(x86_EAX)) { return x86_EAX; }
	if (GetX86Mapped(x86_EDX) == NotMapped && !GetX86Protected(x86_EDX)) { return x86_EDX; }
	if (GetX86Mapped(x86_ECX) == NotMapped && !GetX86Protected(x86_ECX)) { return x86_ECX; }

	x86Reg Reg = UnMap_TempReg();
	if (Reg != x86_Unknown) { return Reg; }

	int count, MapCount[10];
	x86Reg MapReg[10];

	for (count = 0; count < 10; count ++) 
	{
		MapCount[count] = GetX86MapOrder((x86Reg)count);
		MapReg[count] = (x86Reg)count;
	}
	for (count = 0; count < 10; count ++) {
		int i;
		
		for (i = 0; i < 9; i ++) {
			x86Reg tempReg;
			DWORD temp;

			if (MapCount[i] < MapCount[i+1]) {
				temp = MapCount[i];
				MapCount[i] = MapCount[i+1];
				MapCount[i+1] = temp;
				tempReg = MapReg[i];
				MapReg[i] = MapReg[i+1];
				MapReg[i+1] = tempReg;
			}
		}

	}

	x86Reg StackReg = x86_Unknown;
	for (count = 0; count < 10; count ++) 
	{
		if (MapCount[count] > 0 && GetX86Mapped(MapReg[count]) != Stack_Mapped) 
		{
			if (UnMap_X86reg((x86Reg)MapReg[count])) 
			{
				return (x86Reg)MapReg[count];
			}			
		}
		if (GetX86Mapped(MapReg[count]) == Stack_Mapped) { StackReg = MapReg[count]; }
	}
	if (StackReg != x86_Unknown) {
		UnMap_X86reg(StackReg);
		return StackReg;
	}

	return x86_Unknown;
}

CX86Ops::x86Reg CRegInfo::Free8BitX86Reg()
{
	
	if (GetX86Mapped(x86_EBX) == NotMapped && !GetX86Protected(x86_EBX)) {return x86_EBX; }
	if (GetX86Mapped(x86_EAX) == NotMapped && !GetX86Protected(x86_EAX)) {return x86_EAX; }
	if (GetX86Mapped(x86_EDX) == NotMapped && !GetX86Protected(x86_EDX)) {return x86_EDX; }
	if (GetX86Mapped(x86_ECX) == NotMapped && !GetX86Protected(x86_ECX)) {return x86_ECX; }


	x86Reg Reg = UnMap_8BitTempReg();
	if (Reg > 0) { return Reg; }
	
	int count, MapCount[10], MapReg[10];
	for (count = 0; count < 10; count ++) {
		MapCount[count] = GetX86MapOrder((x86Reg)count);
		MapReg[count] = count;
	}
	for (count = 0; count < 10; count ++) {
		int i;
		
		for (i = 0; i < 9; i ++) {
			int temp;

			if (MapCount[i] < MapCount[i+1]) {
				temp = MapCount[i];
				MapCount[i] = MapCount[i+1];
				MapCount[i+1] = temp;
				temp = MapReg[i];
				MapReg[i] = MapReg[i+1];
				MapReg[i+1] = temp;
			}
		}

	}
	for (count = 0; count < 10; count ++) {
		if (MapCount[count] > 0) {			
			if (!Is8BitReg((x86Reg)count)) {  continue; }
			if (UnMap_X86reg((x86Reg)count)) {
				return (x86Reg)count;
			}
		}
	}
	return x86_Unknown;
}

CX86Ops::x86Reg CRegInfo::UnMap_8BitTempReg()
{
	int count;

	for (count = 0; count < 10; count ++) {
		if (!Is8BitReg((x86Reg)count)) { continue; }
		if (GetMipsRegState((x86Reg)count) == Temp_Mapped) {
			if (GetX86Protected((x86Reg)count) == false) {
				CPU_Message("    regcache: unallocate %s from temp storage",x86_Name((x86Reg)count));
				SetX86Mapped((x86Reg)count, CRegInfo::NotMapped);
				return (x86Reg)count;
			}
		}
	}
	return x86_Unknown;
}

CRegInfo::x86Reg CRegInfo::Get_MemoryStack() const
{
	for (int i = 0, n = sizeof(x86_Registers)/ sizeof(x86_Registers[0]); i < n; i++) 
	{
		if (GetX86Mapped(x86_Registers[i]) == Stack_Mapped)
		{
			return x86_Registers[i];
		}
	} 
	return x86_Unknown;
}

CRegInfo::x86Reg CRegInfo::Map_MemoryStack ( x86Reg Reg, bool bMapRegister, bool LoadValue)
{
	x86Reg CurrentMap = Get_MemoryStack();
	if (!bMapRegister)
	{
		//if not mapping then just return what the current mapping is
		return CurrentMap;
	}
	
	if (CurrentMap != x86_Unknown && CurrentMap == Reg)
	{
		//already mapped to correct reg
		return CurrentMap;
	}
	// map a register
	if (Reg == x86_Any)
	{
		if (CurrentMap != x86_Unknown)
		{
			return CurrentMap;
		}
		Reg = FreeX86Reg();	
		if (Reg == x86_Unknown) 
		{
			g_Notify->DisplayError(L"Map_MemoryStack\n\nOut of registers");
			g_Notify->BreakPoint(__FILEW__,__LINE__); 
		}
		SetX86Mapped(Reg,CRegInfo::Stack_Mapped);
		CPU_Message("    regcache: allocate %s as Memory Stack",x86_Name(Reg));		
		if (LoadValue)
		{
			MoveVariableToX86reg(&g_Recompiler->MemoryStackPos(),"MemoryStack",Reg);
		}
		return Reg;
	}

	//move to a register/allocate register
	UnMap_X86reg(Reg);
	if (CurrentMap != x86_Unknown)
	{
		CPU_Message("    regcache: change allocation of Memory Stack from %s to %s",x86_Name(CurrentMap),x86_Name(Reg));
		SetX86Mapped(Reg, CRegInfo::Stack_Mapped);
		SetX86Mapped(CurrentMap,CRegInfo::NotMapped);
		MoveX86RegToX86Reg(CurrentMap,Reg);
	} else {
		SetX86Mapped(Reg,CRegInfo::Stack_Mapped);
		CPU_Message("    regcache: allocate %s as Memory Stack",x86_Name(Reg));		
		if (LoadValue)
		{
			MoveVariableToX86reg(&g_Recompiler->MemoryStackPos(),"MemoryStack",Reg);
		}
	}
	return Reg;
}

void CRegInfo::Map_GPR_32bit (int MipsReg, bool SignValue, int MipsRegToLoad) 
{
	int count;

	x86Reg Reg;
	if (MipsReg == 0) 
	{
		g_Notify->BreakPoint(__FILEW__,__LINE__);
		return;
	}

	if (IsUnknown(MipsReg) || IsConst(MipsReg)) 
	{
		Reg = FreeX86Reg();		
		if (Reg < 0) { 
			if (bHaveDebugger()) { g_Notify->DisplayError(L"Map_GPR_32bit\n\nOut of registers"); }
			g_Notify->BreakPoint(__FILEW__,__LINE__); 
			return; 
		}		
		CPU_Message("    regcache: allocate %s to %s",x86_Name(Reg),CRegName::GPR[MipsReg]);
	} else {
		if (Is64Bit(MipsReg)) 
		{
			CPU_Message("    regcache: unallocate %s from high 32bit of %s",x86_Name(GetMipsRegMapHi(MipsReg)),CRegName::GPR_Hi[MipsReg]);
			SetX86MapOrder(GetMipsRegMapHi(MipsReg),0);
			SetX86Mapped(GetMipsRegMapHi(MipsReg),NotMapped);
			SetX86Protected(GetMipsRegMapHi(MipsReg), false);
			SetMipsRegHi(MipsReg,0);
		}
		Reg = GetMipsRegMapLo(MipsReg);
	}
	for (count = 0; count < 10; count ++) 
	{
		DWORD Count = GetX86MapOrder((x86Reg)count);
		if ( Count > 0)
		{ 
			SetX86MapOrder((x86Reg)count,Count + 1);
		}
	}
	SetX86MapOrder(Reg,1);
	
	if (MipsRegToLoad > 0) 
	{
		if (IsUnknown(MipsRegToLoad)) 
		{
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0],CRegName::GPR_Lo[MipsRegToLoad],Reg);
		} else if (IsMapped(MipsRegToLoad)) {
			if (MipsReg != MipsRegToLoad) 
			{
				MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad),Reg);
			}
		} else {
			MoveConstToX86reg(GetMipsRegLo(MipsRegToLoad),Reg);
		}
	} else if (MipsRegToLoad == 0) {
		XorX86RegToX86Reg(Reg,Reg);
	}
	SetX86Mapped(Reg,GPR_Mapped);
	SetX86Protected(Reg, true);
	SetMipsRegMapLo(MipsReg,Reg);
	SetMipsRegState(MipsReg,SignValue ? STATE_MAPPED_32_SIGN : STATE_MAPPED_32_ZERO);
}

void CRegInfo::Map_GPR_64bit ( int MipsReg, int MipsRegToLoad) 
{
	x86Reg x86Hi, x86lo;
	int count;

	if (MipsReg == 0) {
		if (bHaveDebugger()) { g_Notify->DisplayError(L"Map_GPR_32bit\n\nWhy are you trying to map reg 0"); }
		return;
	}

	ProtectGPR(MipsReg);
	if (IsUnknown(MipsReg) || IsConst(MipsReg)) {
		x86Hi = FreeX86Reg();
		if (x86Hi < 0) 
		{
			if (bHaveDebugger()) { g_Notify->DisplayError(L"Map_GPR_64bit\n\nOut of registers"); }
			return; 
		}
		SetX86Protected(x86Hi, true);

		x86lo = FreeX86Reg();
		if (x86lo < 0) {  g_Notify->DisplayError(L"Map_GPR_64bit\n\nOut of registers"); return; }
		SetX86Protected(x86lo, true);
		
		CPU_Message("    regcache: allocate %s to hi word of %s",x86_Name(x86Hi),CRegName::GPR[MipsReg]);
		CPU_Message("    regcache: allocate %s to low word of %s",x86_Name(x86lo),CRegName::GPR[MipsReg]);
	} else {
		x86lo = GetMipsRegMapLo(MipsReg);
		if (Is32Bit(MipsReg)) {
			SetX86Protected(x86lo, true);
			x86Hi = FreeX86Reg();
			if (x86Hi == x86_Unknown)
			{
				g_Notify->BreakPoint(__FILEW__,__LINE__); 
				return;
			}
			SetX86Protected(x86Hi, true);

			CPU_Message("    regcache: allocate %s to hi word of %s",x86_Name(x86Hi),CRegName::GPR[MipsReg]);
		} else {
			x86Hi = GetMipsRegMapHi(MipsReg);
		}
	}
	
	for (count = 0; count < 10; count ++) 
	{
		int MapOrder = GetX86MapOrder((x86Reg)count);
		if (MapOrder > 0) 
		{ 
			SetX86MapOrder((x86Reg)count,MapOrder + 1);
		}
	}
	
	SetX86MapOrder(x86Hi,1);
	SetX86MapOrder(x86lo,1);
	if (MipsRegToLoad > 0) {
		if (IsUnknown(MipsRegToLoad)) {
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[1],CRegName::GPR_Hi[MipsRegToLoad],x86Hi);
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0],CRegName::GPR_Lo[MipsRegToLoad],x86lo);
		} else if (IsMapped(MipsRegToLoad)) {
			if (Is32Bit(MipsRegToLoad)) {
				if (IsSigned(MipsRegToLoad)) {
					MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad),x86Hi);
					ShiftRightSignImmed(x86Hi,31);
				} else {
					XorX86RegToX86Reg(x86Hi,x86Hi);
				}
				if (MipsReg != MipsRegToLoad) {
					MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad),x86lo);
				}
			} else {
				if (MipsReg != MipsRegToLoad) {
					MoveX86RegToX86Reg(GetMipsRegMapHi(MipsRegToLoad),x86Hi);
					MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad),x86lo);
				}
			}
		} else {
CPU_Message("Map_GPR_64bit 11");
			if (Is32Bit(MipsRegToLoad)) {
				if (IsSigned(MipsRegToLoad)) {
					MoveConstToX86reg(GetMipsRegLo_S(MipsRegToLoad) >> 31,x86Hi);
				} else {
					MoveConstToX86reg(0,x86Hi);
				}
			} else {
				MoveConstToX86reg(GetMipsRegHi(MipsRegToLoad),x86Hi);
			}
			MoveConstToX86reg(GetMipsRegLo(MipsRegToLoad),x86lo);
		}
	} else if (MipsRegToLoad == 0) {
		XorX86RegToX86Reg(x86Hi,x86Hi);
		XorX86RegToX86Reg(x86lo,x86lo);
	}
	SetX86Mapped(x86Hi,GPR_Mapped);
	SetX86Mapped(x86lo,GPR_Mapped);
	SetMipsRegMapHi(MipsReg,x86Hi);
	SetMipsRegMapLo(MipsReg,x86lo);
	SetMipsRegState(MipsReg,STATE_MAPPED_64);
}

CX86Ops::x86Reg CRegInfo::Map_TempReg (CX86Ops::x86Reg Reg, int MipsReg, bool LoadHiWord)
{
	int count;

	if (Reg == x86_Any) 
	{	
		if (GetX86Mapped(x86_EAX) == Temp_Mapped && !GetX86Protected(x86_EAX)) { Reg = x86_EAX; } 
		else if (GetX86Mapped(x86_EBX) == Temp_Mapped && !GetX86Protected(x86_EBX)) { Reg = x86_EBX; } 
		else if (GetX86Mapped(x86_ECX) == Temp_Mapped && !GetX86Protected(x86_ECX)) { Reg = x86_ECX; } 
		else if (GetX86Mapped(x86_EDX) == Temp_Mapped && !GetX86Protected(x86_EDX)) { Reg = x86_EDX; } 
		else if (GetX86Mapped(x86_ESI) == Temp_Mapped && !GetX86Protected(x86_ESI)) { Reg = x86_ESI; } 
		else if (GetX86Mapped(x86_EDI) == Temp_Mapped && !GetX86Protected(x86_EDI)) { Reg = x86_EDI; } 
		else if (GetX86Mapped(x86_EBP) == Temp_Mapped && !GetX86Protected(x86_EBP)) { Reg = x86_EBP; } 
		else if (GetX86Mapped(x86_ESP) == Temp_Mapped && !GetX86Protected(x86_ESP)) { Reg = x86_ESP; } 

		if (Reg == x86_Any) {
			Reg = FreeX86Reg();
			if (Reg == x86_Unknown)
			{
				WriteTrace(TraceError,__FUNCTION__ ": Failed to find a free register");
				g_Notify->BreakPoint(__FILEW__,__LINE__);
				return x86_Unknown;
			}
		}
	} 
	else if (Reg == x86_Any8Bit) 
	{
		if (GetX86Mapped(x86_EAX) == Temp_Mapped && !GetX86Protected(x86_EAX)) { Reg = x86_EAX; } 
		else if (GetX86Mapped(x86_EBX) == Temp_Mapped && !GetX86Protected(x86_EBX)) { Reg = x86_EBX; } 
		else if (GetX86Mapped(x86_ECX) == Temp_Mapped && !GetX86Protected(x86_ECX)) { Reg = x86_ECX; } 
		else if (GetX86Mapped(x86_EDX) == Temp_Mapped && !GetX86Protected(x86_EDX)) { Reg = x86_EDX; } 
		
		if (Reg == x86_Any8Bit) 
		{	
			Reg = Free8BitX86Reg();
			if (Reg < 0) { 
				WriteTrace(TraceError,__FUNCTION__ ": Failed to find a free 8 bit register");
				g_Notify->BreakPoint(__FILEW__,__LINE__);
				return x86_Unknown;
			}
		}
	} else if (GetX86Mapped(Reg) == GPR_Mapped) {
		if (GetX86Protected(Reg)) 
		{
			WriteTrace(TraceError,__FUNCTION__ ": Register is protected");
			g_Notify->BreakPoint(__FILEW__,__LINE__);
			return x86_Unknown;
		}
		
		SetX86Protected(Reg,true);
		x86Reg NewReg = FreeX86Reg();
		for (count = 1; count < 32; count ++) 
		{
			if (!IsMapped(count)) 
			{
				continue;
			}
			if (GetMipsRegMapLo(count) == Reg)
			{
				if (NewReg == x86_Unknown)
				{
					UnMap_GPR(count, true);
					break;
				}
				CPU_Message("    regcache: change allocation of %s from %s to %s",CRegName::GPR[count],x86_Name(Reg),x86_Name(NewReg));
				SetX86Mapped(NewReg,GPR_Mapped);
				SetX86MapOrder(NewReg,GetX86MapOrder(Reg));
				SetMipsRegMapLo(count,NewReg);
				MoveX86RegToX86Reg(Reg,NewReg);
				if (MipsReg == count && !LoadHiWord)
					MipsReg = -1;
				break;
			}
			if (Is64Bit(count) && GetMipsRegMapHi(count) == Reg)
			{
				if (NewReg == x86_Unknown)
				{
					UnMap_GPR(count, true);
					break;
				}
				CPU_Message("    regcache: change allocation of %s from %s to %s",CRegName::GPR_Hi[count],x86_Name(Reg),x86_Name(NewReg));
				SetX86Mapped(NewReg,GPR_Mapped);
				SetX86MapOrder(NewReg,GetX86MapOrder(Reg));
				SetMipsRegMapHi(count,NewReg);
				MoveX86RegToX86Reg(Reg,NewReg);
				if (MipsReg == count && LoadHiWord)
					MipsReg = -1;
				break;
			}
		}
	} 
	else if (GetX86Mapped(Reg) == Stack_Mapped) 
	{
		UnMap_X86reg(Reg);
	}
	CPU_Message("    regcache: allocate %s as temp storage",x86_Name(Reg));		

	if (MipsReg >= 0) {
		if (LoadHiWord) {
			if (IsUnknown(MipsReg)) 
			{
				MoveVariableToX86reg(&_GPR[MipsReg].UW[1],CRegName::GPR_Hi[MipsReg],Reg);
			} 
			else if (IsMapped(MipsReg)) 
			{
				if (Is64Bit(MipsReg)) {
					MoveX86RegToX86Reg(GetMipsRegMapHi(MipsReg),Reg);
				} else if (IsSigned(MipsReg)){
					MoveX86RegToX86Reg(GetMipsRegMapLo(MipsReg),Reg);
					ShiftRightSignImmed(Reg,31);
				} else {
					MoveConstToX86reg(0,Reg);
				}
			} else {
				if (Is64Bit(MipsReg))
				{
					MoveConstToX86reg(GetMipsRegHi(MipsReg),Reg);
				} else {
					MoveConstToX86reg(GetMipsRegLo_S(MipsReg) >> 31,Reg);
				}
			}
		} else {
			if (IsUnknown(MipsReg)) {
				MoveVariableToX86reg(&_GPR[MipsReg].UW[0],CRegName::GPR_Lo[MipsReg],Reg);
			} else if (IsMapped(MipsReg)) {
				MoveX86RegToX86Reg(GetMipsRegMapLo(MipsReg),Reg);
			} else {
				MoveConstToX86reg(GetMipsRegLo(MipsReg),Reg);
			}
		}
	}
	SetX86Mapped(Reg,Temp_Mapped);
	SetX86Protected(Reg, true);
	for (count = 0; count < 10; count++)
	{
		int MapOrder = GetX86MapOrder((x86Reg)count);
		if (MapOrder > 0) {
			SetX86MapOrder((x86Reg)count,MapOrder + 1);
		}
	}
	SetX86MapOrder(Reg,1);
	return Reg;
}

void CRegInfo::ProtectGPR(DWORD Reg) {
	if (IsUnknown(Reg) || IsConst(Reg)) {
		return;
	}
	if (Is64Bit(Reg)) {
		SetX86Protected(GetMipsRegMapHi(Reg), true);
	}

	SetX86Protected(GetMipsRegMapLo(Reg), true);
}

void CRegInfo::UnProtectGPR(DWORD Reg) {
	if (IsUnknown(Reg) || IsConst(Reg)) {
		return;
	}
	if (Is64Bit(Reg)) {
		SetX86Protected(GetMipsRegMapHi(Reg), false);
	}

	SetX86Protected(GetMipsRegMapLo(Reg),false);
}

void CRegInfo::ResetX86Protection()
{
	for (int count = 0; count < 10; count ++) 
	{ 
		SetX86Protected((x86Reg)count, false);
	}
}

bool CRegInfo::RegInStack( int Reg, FPU_STATE Format) {
	for (int i = 0; i < 8; i++)
	{
		if (x86fpu_MappedTo[i] == Reg)
		{
			if (x86fpu_State[i] == Format || Format == FPU_Any)
			{
				return true;
			}

			return false;
		}
	}

	return false;
}

void CRegInfo::UnMap_AllFPRs()
{
	for (;;) {
		int StackPos = StackTopPos();
		if (x86fpu_MappedTo[StackPos] != -1 ) {
			UnMap_FPR(x86fpu_MappedTo[StackPos], true);
			continue;
		}
		//see if any more registers mapped
		int StartPos = StackTopPos();
		for (int i = 0; i < 8; i++) {
			if (x86fpu_MappedTo[(StartPos + i) & 7] != -1 ) { fpuIncStack(&StackTopPos()); }
		}
		if (StackPos != StackTopPos()) { continue; }
		return;
	}
}

void CRegInfo::UnMap_FPR (int Reg, int WriteBackValue ) 
{
	char Name[50];
	int i;

	if (Reg < 0) { return; }
	for (i = 0; i < 8; i++) {
		if (x86fpu_MappedTo[i] != Reg) { continue; }
		CPU_Message("    regcache: unallocate %s from ST(%d)",CRegName::FPR[Reg],(i - StackTopPos() + 8) & 7);
		if (WriteBackValue) {
			int RegPos;
			
			if (((i - StackTopPos() + 8) & 7) != 0) 
			{
				if (x86fpu_MappedTo[StackTopPos()] == -1 && x86fpu_MappedTo[(StackTopPos() + 1) & 7] == Reg)
				{
					fpuIncStack(&StackTopPos());
				} else {
					CRegInfo::FPU_ROUND RoundingModel = FpuRoundingModel(StackTopPos());
					FPU_STATE RegState  = x86fpu_State[StackTopPos()];
					bool Changed        = x86fpu_StateChanged[StackTopPos()];
					DWORD MappedTo      = x86fpu_MappedTo[StackTopPos()];
					FpuRoundingModel(StackTopPos()) = FpuRoundingModel(i);
					x86fpu_MappedTo[StackTopPos()]      = x86fpu_MappedTo[i];
					x86fpu_State[StackTopPos()]         = x86fpu_State[i];
					x86fpu_StateChanged[StackTopPos()] = x86fpu_StateChanged[i];
					FpuRoundingModel(i) = RoundingModel; 
					x86fpu_MappedTo[i]      = MappedTo;
					x86fpu_State[i]         = RegState;
					x86fpu_StateChanged[i]  = Changed;
					fpuExchange((x86FpuValues)((i - StackTopPos()) & 7));
				}
			}
			
			FixRoundModel(FpuRoundingModel(i));

			RegPos = StackTopPos();
			x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
			switch (x86fpu_State[StackTopPos()]) {
			case FPU_Dword: 
				sprintf(Name,"_FPR_S[%d]",x86fpu_MappedTo[StackTopPos()]);
				MoveVariableToX86reg(&_FPR_S[x86fpu_MappedTo[StackTopPos()]],Name,TempReg);
				fpuStoreIntegerDwordFromX86Reg(&StackTopPos(),TempReg, true);
				break;
			case FPU_Qword: 
				sprintf(Name,"_FPR_D[%d]",x86fpu_MappedTo[StackTopPos()]);
				MoveVariableToX86reg(&_FPR_D[x86fpu_MappedTo[StackTopPos()]],Name,TempReg);
				fpuStoreIntegerQwordFromX86Reg(&StackTopPos(),TempReg, true);
				break;
			case FPU_Float: 
				sprintf(Name,"_FPR_S[%d]",x86fpu_MappedTo[StackTopPos()]);
				MoveVariableToX86reg(&_FPR_S[x86fpu_MappedTo[StackTopPos()]],Name,TempReg);
				fpuStoreDwordFromX86Reg(&StackTopPos(),TempReg, true);
				break;
			case FPU_Double: 
				sprintf(Name,"_FPR_D[%d]",x86fpu_MappedTo[StackTopPos()]);
				MoveVariableToX86reg(&_FPR_D[x86fpu_MappedTo[StackTopPos()]],Name,TempReg);
				fpuStoreQwordFromX86Reg(&StackTopPos(),TempReg, true);
				break;
			default:
				if (bHaveDebugger()) { g_Notify->DisplayError(__FUNCTIONW__ L"\nUnknown format to load %d",x86fpu_State[StackTopPos()]); }
			}
			SetX86Protected(TempReg, false);
			FpuRoundingModel(RegPos) = RoundDefault;
			x86fpu_MappedTo[RegPos]      = -1;
			x86fpu_State[RegPos]         = FPU_Unknown;
			x86fpu_StateChanged[RegPos]  = false;
		} else {
			fpuFree((x86FpuValues)((i - StackTopPos()) & 7));
			FpuRoundingModel(i) = RoundDefault;
			x86fpu_MappedTo[i]      = -1;
			x86fpu_State[i]         = FPU_Unknown;
			x86fpu_StateChanged[i]  = false;
		}
		return;
	}
}

void CRegInfo::UnMap_GPR (DWORD Reg, bool WriteBackValue) 
{
	if (Reg == 0) 
	{
		if (bHaveDebugger()) { g_Notify->DisplayError(__FUNCTIONW__ L"\n\nWhy are you trying to unmap reg 0"); }
		return;
	}

	if (IsUnknown(Reg)) { return; }
	//CPU_Message("UnMap_GPR: State: %X\tReg: %s\tWriteBack: %s",State,CRegName::GPR[Reg],WriteBackValue?"TRUE":"FALSE");
	if (IsConst(Reg)) { 
		if (!WriteBackValue) { 
			SetMipsRegState(Reg,STATE_UNKNOWN);
			return; 
		}
		if (Is64Bit(Reg)) {
			MoveConstToVariable(GetMipsRegHi(Reg),&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
			MoveConstToVariable(GetMipsRegLo(Reg),&_GPR[Reg].UW[0],CRegName::GPR_Lo[Reg]);
			SetMipsRegState(Reg,STATE_UNKNOWN);
			return;
		}
		if ((GetMipsRegLo(Reg) & 0x80000000) != 0) {
			MoveConstToVariable(0xFFFFFFFF,&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		} else {
			MoveConstToVariable(0,&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		}
		MoveConstToVariable(GetMipsRegLo(Reg),&_GPR[Reg].UW[0],CRegName::GPR_Lo[Reg]);
		SetMipsRegState(Reg,STATE_UNKNOWN);
		return;
	}
	if (Is64Bit(Reg)) {
		CPU_Message("    regcache: unallocate %s from %s",x86_Name(GetMipsRegMapHi(Reg)),CRegName::GPR_Hi[Reg]);
		SetX86Mapped(GetMipsRegMapHi(Reg),NotMapped);
		SetX86Protected(GetMipsRegMapHi(Reg), false);
	}
	CPU_Message("    regcache: unallocate %s from %s",x86_Name(GetMipsRegMapLo(Reg)),CRegName::GPR_Lo[Reg]);
	SetX86Mapped(GetMipsRegMapLo(Reg),NotMapped);
	SetX86Protected(GetMipsRegMapLo(Reg), false);
	if (!WriteBackValue)
	{ 
		SetMipsRegState(Reg,STATE_UNKNOWN);
		return; 
	}
	MoveX86regToVariable(GetMipsRegMapLo(Reg),&_GPR[Reg].UW[0],CRegName::GPR_Lo[Reg]);
	if (Is64Bit(Reg)) {
		SetMipsRegMapLo(Reg, x86_Unknown);
		MoveX86regToVariable(GetMipsRegMapHi(Reg),&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		SetMipsRegMapHi(Reg,x86_Unknown);
	} else {
		if (!g_System->b32BitCore()) {
			if (IsSigned(Reg)) {
				ShiftRightSignImmed(GetMipsRegMapLo(Reg),31);
				MoveX86regToVariable(GetMipsRegMapLo(Reg),&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
			} else {
				MoveConstToVariable(0,&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
			}
		}
		SetMipsRegMapLo(Reg, x86_Unknown);
	}
	SetMipsRegState(Reg,STATE_UNKNOWN);
}

CX86Ops::x86Reg CRegInfo::UnMap_TempReg()
{
	CX86Ops::x86Reg Reg = x86_Unknown;

	if (GetX86Mapped(x86_EAX) == Temp_Mapped && !GetX86Protected(x86_EAX)) { Reg = x86_EAX; } 
	else if (GetX86Mapped(x86_EBX) == Temp_Mapped && !GetX86Protected(x86_EBX)) { Reg = x86_EBX; } 
	else if (GetX86Mapped(x86_ECX) == Temp_Mapped && !GetX86Protected(x86_ECX)) { Reg = x86_ECX; } 
	else if (GetX86Mapped(x86_EDX) == Temp_Mapped && !GetX86Protected(x86_EDX)) { Reg = x86_EDX; } 
	else if (GetX86Mapped(x86_ESI) == Temp_Mapped && !GetX86Protected(x86_ESI)) { Reg = x86_ESI; } 
	else if (GetX86Mapped(x86_EDI) == Temp_Mapped && !GetX86Protected(x86_EDI)) { Reg = x86_EDI; } 
	else if (GetX86Mapped(x86_EBP) == Temp_Mapped && !GetX86Protected(x86_EBP)) { Reg = x86_EBP; } 
	else if (GetX86Mapped(x86_ESP) == Temp_Mapped && !GetX86Protected(x86_ESP)) { Reg = x86_ESP; } 

	if (Reg != x86_Unknown)
	{
		if (GetX86Mapped(Reg) == Temp_Mapped)
		{
			CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(Reg));
		}
		SetX86Mapped(Reg,NotMapped);
	}
	return Reg;
}

bool CRegInfo::UnMap_X86reg(CX86Ops::x86Reg Reg)
{
	int count;

	if (GetX86Mapped(Reg) == NotMapped) 
	{
		if (!GetX86Protected(Reg))
		{
			return true;
		}
	}
	else if (GetX86Mapped(Reg) == CRegInfo::GPR_Mapped)
	{
		for (count = 1; count < 32; count ++) 
		{
			if (!IsMapped(count)) 
				continue;

			if (Is64Bit(count) && GetMipsRegMapHi(count) == Reg) 
			{
				if (!GetX86Protected(Reg))
				{
					UnMap_GPR(count, true);
					return true;
				}
				break;
			} 
			if (GetMipsRegMapLo(count) == Reg)
			{
				if (!GetX86Protected(Reg))
				{
					UnMap_GPR(count, true);
					return true;
				}
				break;
			}
		}
	}
	else if (GetX86Mapped(Reg) == CRegInfo::Temp_Mapped)
	{
		if (!GetX86Protected(Reg)) {
			CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(Reg));
			SetX86Mapped(Reg,NotMapped);
			return true;
		}
	}
	else if (GetX86Mapped(Reg) == CRegInfo::Stack_Mapped)
	{
		CPU_Message("    regcache: unallocate %s from Memory Stack",x86_Name(Reg));
		MoveX86regToVariable(Reg,&(g_Recompiler->MemoryStackPos()),"MemoryStack");
		SetX86Mapped(Reg,NotMapped);
		return true;
	}

	return false;
}

void CRegInfo::WriteBackRegisters()
{
	UnMap_AllFPRs();

	int count;
	bool bEdiZero = false;
	bool bEsiSign = false;

	int X86RegCount = sizeof(x86_Registers)/ sizeof(x86_Registers[0]);
	for (int i = 0; i < X86RegCount; i++) { SetX86Protected(x86_Registers[i], false); }
	for (int i = 0; i < X86RegCount; i++) { UnMap_X86reg(x86_Registers[i]); }

	/*************************************/
	
	for (count = 1; count < 32; count ++) {
		switch (GetMipsRegState(count)) {
		case CRegInfo::STATE_UNKNOWN: break;
		case CRegInfo::STATE_CONST_32_SIGN:
			if (!g_System->b32BitCore())
			{
				if (!bEdiZero && (!GetMipsRegLo(count) || !(GetMipsRegLo(count) & 0x80000000))) 
				{
					XorX86RegToX86Reg(x86_EDI, x86_EDI);
					bEdiZero = true;
				}
				if (!bEsiSign && (GetMipsRegLo(count) & 0x80000000))
				{
					MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
					bEsiSign = true;
				}
				if ((GetMipsRegLo(count) & 0x80000000) != 0) 
				{
					MoveX86regToVariable(x86_ESI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
				} 
				else 
				{
					MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
				}
			}

			if (GetMipsRegLo(count) == 0) 
			{
				if (g_System->b32BitCore())
				{
					if (!bEdiZero)
					{
						XorX86RegToX86Reg(x86_EDI, x86_EDI);
						bEdiZero = true;
					}
				}
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			}
			else if (GetMipsRegLo(count) == 0xFFFFFFFF)
			{
				if (g_System->b32BitCore())
				{
					if (!bEsiSign)
					{
						MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
						bEsiSign = true;
					}
				}
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} 
			else
			{
				MoveConstToVariable(GetMipsRegLo(count),&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			}

			SetMipsRegState(count, CRegInfo::STATE_UNKNOWN);
			break;
		case CRegInfo::STATE_CONST_32_ZERO:
			if (!g_System->b32BitCore())
			{
				if (!bEdiZero) 
				{
					XorX86RegToX86Reg(x86_EDI, x86_EDI);
					bEdiZero = true;
				}
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			}

			if (GetMipsRegLo(count) == 0) 
			{
				if (g_System->b32BitCore())
				{
					if (!bEdiZero)
					{
						XorX86RegToX86Reg(x86_EDI, x86_EDI);
						bEdiZero = true;
					}
				}
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			}
			else
			{
				MoveConstToVariable(GetMipsRegLo(count),&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			}

			SetMipsRegState(count, CRegInfo::STATE_UNKNOWN);
			break;
		case CRegInfo::STATE_CONST_64:
			if (GetMipsRegLo(count) == 0 || GetMipsRegHi(count) == 0) {
				XorX86RegToX86Reg(x86_EDI, x86_EDI);
				bEdiZero = true;
			}
			if (GetMipsRegLo(count) == 0xFFFFFFFF || GetMipsRegHi(count) == 0xFFFFFFFF) {
				MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
				bEsiSign = true;
			}

			if (GetMipsRegHi(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} else if (GetMipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} else {
				MoveConstToVariable(GetMipsRegHi(count),&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} 

			if (GetMipsRegLo(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} else if (GetMipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} else {
				MoveConstToVariable(GetMipsRegLo(count),&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			}
			SetMipsRegState(count, CRegInfo::STATE_UNKNOWN);
			break;
		default:
			CPU_Message(__FUNCTION__ ": Unknown State: %d reg %d (%s)",GetMipsRegState(count),count,CRegName::GPR[count])
			g_Notify->BreakPoint(__FILEW__,__LINE__);
		}
	}
}

const char * CRegInfo::RoundingModelName ( FPU_ROUND RoundType )
{
	switch (RoundType)
	{
	case RoundUnknown:  return "RoundUnknown";
	case RoundDefault:  return "RoundDefault";
	case RoundTruncate: return "RoundTruncate";
	case RoundNearest:  return "RoundNearest";
	case RoundDown:     return "RoundDown";
	case RoundUp:       return "RoundUp";
	}
	return "** Invalid **";
}
