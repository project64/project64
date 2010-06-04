#include "stdafx.h"

unsigned int CRegInfo::m_fpuControl = 0;

char *Format_Name[] = {"Unknown","dword","qword","float","double"};

void CRegInfo::Initilize ( void )
{
	int count;
	
	MIPS_RegState[0]  = STATE_CONST_32;
	MIPS_RegVal[0].DW = 0;
	RegMapLo[0] = x86_Unknown;
	RegMapHi[0] = x86_Unknown;
	
	for (count = 1; count < 32; count ++ ) {
		MIPS_RegState[count]   = STATE_UNKNOWN;
		MIPS_RegVal[count].DW = 0;
		RegMapLo[count] = x86_Unknown;
		RegMapHi[count] = x86_Unknown;
	}
	for (count = 0; count < 10; count ++ ) {
		x86reg_MappedTo[count]  = NotMapped;
		x86reg_Protected[count] = false;
		x86reg_MapOrder[count]  = 0;
	}
	m_CycleCount = 0;

	Stack_TopPos = 0;
	for (count = 0; count < 8; count ++ ) {
		x86fpu_MappedTo[count] = -1;
		x86fpu_State[count] = FPU_Unknown;
		x86fpu_RoundingModel[count] = RoundDefault;
	}
	Fpu_Used = false;
	RoundingModel = RoundUnknown;
}

void CRegInfo::FixRoundModel(FPU_ROUND RoundMethod )
{
	if (CurrentRoundingModel() == RoundMethod) 
	{
		return;
	}
	CPU_Message("    FixRoundModel: CurrentRoundingModel: %s  targetRoundModel: %s",RoundingModelName(CurrentRoundingModel()),RoundingModelName(RoundMethod));

	m_fpuControl = 0;			
	fpuStoreControl(&m_fpuControl, "m_fpuControl");
	x86Reg reg = Map_TempReg(x86_Any,-1,FALSE);
	MoveVariableToX86reg(&m_fpuControl, "m_fpuControl", reg);
	AndConstToX86Reg(reg, 0xF3FF);
		
	if (RoundMethod == RoundDefault)
	{ 
		x86Reg RoundReg = Map_TempReg(x86_Any,-1,FALSE);
		MoveVariableToX86reg(&_Reg->m_RoundingModel,"m_RoundingModel", RoundReg);
		ShiftLeftSignImmed(RoundReg,2);
		OrX86RegToX86Reg(reg,RoundReg);
		x86Protected(RoundReg) = false;		
	} else {
		switch (RoundMethod) {
		case RoundTruncate: OrConstToX86Reg(0x0C00, reg); break;
		case RoundNearest: /*OrConstToX86Reg(0x0000, reg);*/ break;
		case RoundDown:     OrConstToX86Reg(0x0400, reg); break;
		case RoundUp:       OrConstToX86Reg(0x0800, reg); break;
		default:
			DisplayError("Unknown Rounding model");
		}
	}
	MoveX86regToVariable(reg, &m_fpuControl, "m_fpuControl");
	x86Protected(reg) = false;		
	fpuLoadControl(&m_fpuControl, "m_fpuControl");
	CurrentRoundingModel() = RoundMethod;
}

void CRegInfo::ChangeFPURegFormat (int Reg, FPU_STATE OldFormat, FPU_STATE NewFormat, FPU_ROUND RoundingModel)
{
	for (DWORD i = 0; i < 8; i++) 
	{
		if (FpuMappedTo(i) != (DWORD)Reg) 
		{
			continue;
		}
		if (FpuState(i) != OldFormat) {		
			UnMap_FPR(Reg,TRUE);
			Load_FPR_ToTop(Reg,Reg,OldFormat);
			ChangeFPURegFormat(Reg,OldFormat,NewFormat,RoundingModel);
			return;
		}
		CPU_Message("    regcache: Changed format of ST(%d) from %s to %s", (i - StackTopPos() + 8) & 7,Format_Name[OldFormat],Format_Name[NewFormat]);			
		FpuRoundingModel(i) = RoundingModel;
		FpuState(i)         = NewFormat;
		return;
	}

#ifndef EXTERNAL_RELEASE
	DisplayError("ChangeFormat: Register not on stack!!");
#endif
}

void CRegInfo::Load_FPR_ToTop ( int Reg, int RegToLoad, FPU_STATE Format) 
{
	if (CurrentRoundingModel() != RoundDefault)
	{
		FixRoundModel(RoundDefault);
	}
	CPU_Message("CurrentRoundingModel: %s  FpuRoundingModel(StackTopPos()): %s",RoundingModelName(CurrentRoundingModel()),RoundingModelName(FpuRoundingModel(StackTopPos())));
	int i;

	if (RegToLoad < 0) { DisplayError("Load_FPR_ToTop\nRegToLoad < 0 ???"); return; }
	if (Reg < 0) { DisplayError("Load_FPR_ToTop\nReg < 0 ???"); return; }

	if (Format == FPU_Double || Format == FPU_Qword) {
		UnMap_FPR(Reg + 1,TRUE);
		UnMap_FPR(RegToLoad + 1,TRUE);
	} else {
		if ((Reg & 1) != 0) {
			for (i = 0; i < 8; i++) {
				if (FpuMappedTo(i) == (DWORD)(Reg - 1)) {
					if (FpuState(i) == FPU_Double || FpuState(i) == FPU_Qword) {
						UnMap_FPR(Reg,TRUE);
					}
					i = 8;
				}
			}		
		}
		if ((RegToLoad & 1) != 0) {
			for (i = 0; i < 8; i++) {
				if (FpuMappedTo(i) == (DWORD)(RegToLoad - 1)) {
					if (FpuState(i) == FPU_Double || FpuState(i) == FPU_Qword) {
						UnMap_FPR(RegToLoad,TRUE);
					}
					i = 8;
				}
			}		
		}
	}

	if (Reg == RegToLoad) {
		//if different format then unmap original reg from stack
		for (i = 0; i < 8; i++) {
			if (FpuMappedTo(i) == (DWORD)Reg) {
				if (FpuState(i) != (DWORD)Format) {
					UnMap_FPR(Reg,TRUE);
				}
				i = 8;
			}
		}
	} else {
		UnMap_FPR(Reg,FALSE);
	}

	if (RegInStack(RegToLoad,Format)) {
		if (Reg != RegToLoad) {
			if (FpuMappedTo((StackTopPos() - 1) & 7) != (DWORD)RegToLoad) {
				UnMap_FPR(FpuMappedTo((StackTopPos() - 1) & 7),TRUE);
				CPU_Message("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);
				fpuLoadReg(&StackTopPos(),StackPosition(RegToLoad));		
				FpuRoundingModel(StackTopPos()) = RoundDefault;
				FpuMappedTo(StackTopPos())      = Reg;
				FpuState(StackTopPos())         = Format;
			} else {
				UnMap_FPR(FpuMappedTo((StackTopPos() - 1) & 7),TRUE);
				Load_FPR_ToTop (Reg, RegToLoad, Format);
			}
		} else {
			x86FpuValues RegPos, StackPos;
			DWORD i;

			for (i = 0; i < 8; i++) {
				if (FpuMappedTo(i) == (DWORD)Reg) {
					RegPos = (x86FpuValues)i;
					i = 8;
				}
			}

			if (RegPos == StackTopPos()) {
				return;
			}
			StackPos = StackPosition(Reg);

			FpuRoundingModel(RegPos) = FpuRoundingModel(StackTopPos());
			FpuMappedTo(RegPos)      = FpuMappedTo(StackTopPos());
			FpuState(RegPos)         = FpuState(StackTopPos());
			CPU_Message("    regcache: allocate ST(%d) to %s", StackPos,CRegName::FPR[FpuMappedTo(RegPos)]);
			CPU_Message("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);

			fpuExchange(StackPos);

			FpuRoundingModel(StackTopPos()) = RoundDefault;
			FpuMappedTo(StackTopPos())      = Reg;
			FpuState(StackTopPos())         = Format;
		}
	} else {
		char Name[50];
		x86Reg TempReg;

		UnMap_FPR(FpuMappedTo((StackTopPos() - 1) & 7),TRUE);
		for (i = 0; i < 8; i++) {
			if (FpuMappedTo(i) == (DWORD)RegToLoad) {
				UnMap_FPR(RegToLoad,TRUE);
				i = 8;
			}
		}
		CPU_Message("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		switch (Format) {
		case FPU_Dword:
			sprintf(Name,"m_FPR_S[%d]",RegToLoad);
			MoveVariableToX86reg(&_Reg->m_FPR_S[RegToLoad],Name,TempReg);
			fpuLoadIntegerDwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		case FPU_Qword:
			sprintf(Name,"m_FPR_D[%d]",RegToLoad);
			MoveVariableToX86reg(&_Reg->m_FPR_D[RegToLoad],Name,TempReg);
			fpuLoadIntegerQwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		case FPU_Float:
			sprintf(Name,"m_FPR_S[%d]",RegToLoad);
			MoveVariableToX86reg(&_Reg->m_FPR_S[RegToLoad],Name,TempReg);
			fpuLoadDwordFromX86Reg(&StackTopPos(),TempReg);
			break;
		case FPU_Double:
			sprintf(Name,"m_FPR_D[%d]",RegToLoad);
			MoveVariableToX86reg(&_Reg->m_FPR_D[RegToLoad],Name,TempReg);
			fpuLoadQwordFromX86Reg(&StackTopPos(),TempReg);
			break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Load_FPR_ToTop\nUnkown format to load %d",Format);
#endif
		}
		x86Protected(TempReg) = FALSE;
		FpuRoundingModel(StackTopPos()) = RoundDefault;
		FpuMappedTo(StackTopPos())      = Reg;
		FpuState(StackTopPos())         = Format;
	}
}

CRegInfo::x86FpuValues CRegInfo::StackPosition (int Reg) 
{
	int i;

	for (i = 0; i < 8; i++) {
		if (FpuMappedTo(i) == (DWORD)Reg) {
			return (x86FpuValues)((i - StackTopPos()) & 7);
		}
	}
	return x86_ST_Unknown;
}

CX86Ops::x86Reg CRegInfo::FreeX86Reg ( void ) 
{
	if (x86Mapped(x86_EDI) == NotMapped && !x86Protected(x86_EDI)) { return x86_EDI; }
	if (x86Mapped(x86_ESI) == NotMapped && !x86Protected(x86_ESI)) { return x86_ESI; }
	if (x86Mapped(x86_EBX) == NotMapped && !x86Protected(x86_EBX)) { return x86_EBX; }
	if (x86Mapped(x86_EAX) == NotMapped && !x86Protected(x86_EAX)) { return x86_EAX; }
	if (x86Mapped(x86_EDX) == NotMapped && !x86Protected(x86_EDX)) { return x86_EDX; }
	if (x86Mapped(x86_ECX) == NotMapped && !x86Protected(x86_ECX)) { return x86_ECX; }

	x86Reg Reg = UnMap_TempReg();
	if (Reg != x86_Unknown) { return Reg; }

	int count, MapCount[10];
	x86Reg MapReg[10];

	for (count = 0; count < 10; count ++) 
	{
		MapCount[count] = x86MapOrder((x86Reg)count);
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
		if (MapCount[count] > 0 && x86Mapped(MapReg[count]) != Stack_Mapped) 
		{
			if (UnMap_X86reg((x86Reg)MapReg[count])) 
			{
				return (x86Reg)MapReg[count];
			}			
		}
		if (x86Mapped(MapReg[count]) == Stack_Mapped) { StackReg = MapReg[count]; }
	}
	if (StackReg != x86_Unknown) {
		UnMap_X86reg(StackReg);
		return StackReg;
	}

	return x86_Unknown;
}

CX86Ops::x86Reg CRegInfo::Free8BitX86Reg ( void ) 
{
	
	if (x86Mapped(x86_EBX) == NotMapped && !x86Protected(x86_EBX)) {return x86_EBX; }
	if (x86Mapped(x86_EAX) == NotMapped && !x86Protected(x86_EAX)) {return x86_EAX; }
	if (x86Mapped(x86_EDX) == NotMapped && !x86Protected(x86_EDX)) {return x86_EDX; }
	if (x86Mapped(x86_ECX) == NotMapped && !x86Protected(x86_ECX)) {return x86_ECX; }


	x86Reg Reg = UnMap_8BitTempReg();
	if (Reg > 0) { return Reg; }
	
	int count, MapCount[10], MapReg[10];
	for (count = 0; count < 10; count ++) {
		MapCount[count] = x86MapOrder((x86Reg)count);
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

CX86Ops::x86Reg CRegInfo::UnMap_8BitTempReg (void )
{
	int count;

	for (count = 0; count < 10; count ++) {
		if (!Is8BitReg((x86Reg)count)) { continue; }
		if (MipsRegState((x86Reg)count) == Temp_Mapped) {
			if (x86Protected((x86Reg)count) == FALSE) {
				CPU_Message("    regcache: unallocate %s from temp storage",x86_Name((x86Reg)count));
				x86Mapped((x86Reg)count) = CRegInfo::NotMapped;
				return (x86Reg)count;
			}		
		}
	}
	return x86_Unknown;
}

void CRegInfo::Map_GPR_32bit (int MipsReg, BOOL SignValue, int MipsRegToLoad) 
{
	int count;

	x86Reg Reg;
	if (MipsReg == 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Map_GPR_32bit\n\nWhy are you trying to map reg 0");
#endif
		return;
	}

	if (IsUnknown(MipsReg) || IsConst(MipsReg)) {		
		Reg = FreeX86Reg();		
		if (Reg < 0) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("Map_GPR_32bit\n\nOut of registers"); 
			BreakPoint(__FILE__,__LINE__); 
#endif
			return; 
		}		
		CPU_Message("    regcache: allocate %s to %s",x86_Name(Reg),CRegName::GPR[MipsReg]);
	} else {
		if (Is64Bit(MipsReg)) { 
			CPU_Message("    regcache: unallocate %s from high 32bit of %s",x86_Name(MipsRegMapHi(MipsReg)),CRegName::GPR_Hi[MipsReg]);
			x86MapOrder(MipsRegMapHi(MipsReg)) = 0;
			x86Mapped(MipsRegMapHi(MipsReg)) = NotMapped;
			x86Protected(MipsRegMapHi(MipsReg)) = FALSE;
			MipsRegHi(MipsReg) = 0;
		}
		Reg = MipsRegMapLo(MipsReg);
	}
	for (count = 0; count < 10; count ++) {
		if (x86MapOrder((x86Reg)count) > 0) { 
			x86MapOrder((x86Reg)count) += 1;
		}
	}
	x86MapOrder(Reg) = 1;
	
	if (MipsRegToLoad > 0) {
		if (IsUnknown(MipsRegToLoad)) {
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0],CRegName::GPR_Lo[MipsRegToLoad],Reg);
		} else if (IsMapped(MipsRegToLoad)) {
			if (MipsReg != MipsRegToLoad) {
				MoveX86RegToX86Reg(MipsRegMapLo(MipsRegToLoad),Reg);
			}
		} else {
			MoveConstToX86reg(MipsRegLo(MipsRegToLoad),Reg);
		}
	} else if (MipsRegToLoad == 0) {
		XorX86RegToX86Reg(Reg,Reg);
	}
	x86Mapped(Reg) = GPR_Mapped;
	x86Protected(Reg) = TRUE;
	MipsRegMapLo(MipsReg) = Reg;
	MipsRegState(MipsReg) = SignValue ? STATE_MAPPED_32_SIGN : STATE_MAPPED_32_ZERO;
}

void CRegInfo::Map_GPR_64bit ( int MipsReg, int MipsRegToLoad) 
{
	x86Reg x86Hi, x86lo;
	int count;

	if (MipsReg == 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("Map_GPR_32bit\n\nWhy are you trying to map reg 0");
#endif
		return;
	}

	ProtectGPR(MipsReg);
	if (IsUnknown(MipsReg) || IsConst(MipsReg)) {
		x86Hi = FreeX86Reg();
		if (x86Hi < 0) {  DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
		x86Protected(x86Hi) = TRUE;

		x86lo = FreeX86Reg();
		if (x86lo < 0) {  DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
		x86Protected(x86lo) = TRUE;
		
		CPU_Message("    regcache: allocate %s to hi word of %s",x86_Name(x86Hi),CRegName::GPR[MipsReg]);
		CPU_Message("    regcache: allocate %s to low word of %s",x86_Name(x86lo),CRegName::GPR[MipsReg]);
	} else {
		x86lo = MipsRegMapLo(MipsReg);
		if (Is32Bit(MipsReg)) {
			x86Protected(x86lo) = TRUE;
			x86Hi = FreeX86Reg();
			if (x86Hi < 0) {  DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
			x86Protected(x86Hi) = TRUE;
		} else {
			x86Hi = MipsRegMapHi(MipsReg);
		}
	}
	
	for (count = 0; count < 10; count ++) {
		if (x86MapOrder((x86Reg)count) > 0) { x86MapOrder((x86Reg)count) += 1; }
	}
	
	x86MapOrder(x86Hi) = 1;
	x86MapOrder(x86lo) = 1;
	if (MipsRegToLoad > 0) {
		if (IsUnknown(MipsRegToLoad)) {
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[1],CRegName::GPR_Hi[MipsRegToLoad],x86Hi);
			MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0],CRegName::GPR_Lo[MipsRegToLoad],x86lo);
		} else if (IsMapped(MipsRegToLoad)) {
			if (Is32Bit(MipsRegToLoad)) {
				if (IsSigned(MipsRegToLoad)) {
					MoveX86RegToX86Reg(MipsRegMapLo(MipsRegToLoad),x86Hi);
					ShiftRightSignImmed(x86Hi,31);
				} else {
					XorX86RegToX86Reg(x86Hi,x86Hi);
				}
				if (MipsReg != MipsRegToLoad) {
					MoveX86RegToX86Reg(MipsRegMapLo(MipsRegToLoad),x86lo);
				}
			} else {
				if (MipsReg != MipsRegToLoad) {
					MoveX86RegToX86Reg(MipsRegMapHi(MipsRegToLoad),x86Hi);
					MoveX86RegToX86Reg(MipsRegMapLo(MipsRegToLoad),x86lo);
				}
			}
		} else {
CPU_Message("Map_GPR_64bit 11");
			if (Is32Bit(MipsRegToLoad)) {
				if (IsSigned(MipsRegToLoad)) {
					MoveConstToX86reg(MipsRegLo_S(MipsRegToLoad) >> 31,x86Hi);
				} else {
					MoveConstToX86reg(0,x86Hi);
				}
			} else {
				MoveConstToX86reg(MipsRegHi(MipsRegToLoad),x86Hi);
			}
			MoveConstToX86reg(MipsRegLo(MipsRegToLoad),x86lo);
		}
	} else if (MipsRegToLoad == 0) {
		XorX86RegToX86Reg(x86Hi,x86Hi);
		XorX86RegToX86Reg(x86lo,x86lo);
	}
	x86Mapped(x86Hi) = GPR_Mapped;
	x86Mapped(x86lo) = GPR_Mapped;
	MipsRegMapHi(MipsReg) = x86Hi;
	MipsRegMapLo(MipsReg) = x86lo;
	MipsRegState(MipsReg) = STATE_MAPPED_64;
}

CX86Ops::x86Reg CRegInfo::Map_TempReg (CX86Ops::x86Reg Reg, int MipsReg, BOOL LoadHiWord)
{
	int count;

	if (Reg == x86_Any) 
	{	
		if (x86Mapped(x86_EAX) == Temp_Mapped && !x86Protected(x86_EAX)) { Reg = x86_EAX; } 
		else if (x86Mapped(x86_EBX) == Temp_Mapped && !x86Protected(x86_EBX)) { Reg = x86_EBX; } 
		else if (x86Mapped(x86_ECX) == Temp_Mapped && !x86Protected(x86_ECX)) { Reg = x86_ECX; } 
		else if (x86Mapped(x86_EDX) == Temp_Mapped && !x86Protected(x86_EDX)) { Reg = x86_EDX; } 
		else if (x86Mapped(x86_ESI) == Temp_Mapped && !x86Protected(x86_ESI)) { Reg = x86_ESI; } 
		else if (x86Mapped(x86_EDI) == Temp_Mapped && !x86Protected(x86_EDI)) { Reg = x86_EDI; } 
		else if (x86Mapped(x86_EBP) == Temp_Mapped && !x86Protected(x86_EBP)) { Reg = x86_EBP; } 
		else if (x86Mapped(x86_ESP) == Temp_Mapped && !x86Protected(x86_ESP)) { Reg = x86_ESP; } 

		if (Reg == x86_Any) {
			Reg = FreeX86Reg();
			if (Reg == x86_Unknown)
			{
				WriteTrace(TraceError,"CRegInfo::Map_TempReg: Failed to find a free register");
				_Notify->BreakPoint(__FILE__,__LINE__);
				return x86_Unknown;
			}
		}
	} 
	else if (Reg == x86_Any8Bit) 
	{
		if (x86Mapped(x86_EAX) == Temp_Mapped && !x86Protected(x86_EAX)) { Reg = x86_EAX; } 
		else if (x86Mapped(x86_EBX) == Temp_Mapped && !x86Protected(x86_EBX)) { Reg = x86_EBX; } 
		else if (x86Mapped(x86_ECX) == Temp_Mapped && !x86Protected(x86_ECX)) { Reg = x86_ECX; } 
		else if (x86Mapped(x86_EDX) == Temp_Mapped && !x86Protected(x86_EDX)) { Reg = x86_EDX; } 
		
		if (Reg == x86_Any8Bit) 
		{	
			Reg = Free8BitX86Reg();
			if (Reg < 0) { 
				WriteTrace(TraceError,"CRegInfo::Map_TempReg: Failed to find a free 8 bit register");
				_Notify->BreakPoint(__FILE__,__LINE__);
				return x86_Unknown;
			}
		}
	} else if (x86Mapped(Reg) == GPR_Mapped) {
		if (x86Protected(Reg)) 
		{
			WriteTrace(TraceError,"CRegInfo::Map_TempReg: Register is protected");
			_Notify->BreakPoint(__FILE__,__LINE__);
			return x86_Unknown;
		}
		
		x86Protected(Reg) = true;
		x86Reg NewReg = FreeX86Reg();
		for (count = 1; count < 32; count ++) 
		{
			if (!IsMapped(count)) 
			{
				continue;
			}
			if (cMipsRegMapLo(count) == Reg)
			{
				if (NewReg == x86_Unknown)
				{
					UnMap_GPR(count,TRUE);
					break;
				}
				CPU_Message("    regcache: change allocation of %s from %s to %s",CRegName::GPR[count],x86_Name(Reg),x86_Name(NewReg));
				x86Mapped(NewReg) = GPR_Mapped;
				x86MapOrder(NewReg) = x86MapOrder(Reg);
				MipsRegMapLo(count) = NewReg;
				MoveX86RegToX86Reg(Reg,NewReg);
				if (MipsReg == count && LoadHiWord == FALSE) { MipsReg = -1; }
				break;
			}
			if (Is64Bit(count) && cMipsRegMapHi(count) == Reg) 
			{
				if (NewReg == x86_Unknown) 
				{
					UnMap_GPR(count,TRUE);
					break;
				}
				CPU_Message("    regcache: change allocation of %s from %s to %s",CRegName::GPR_Hi[count],x86_Name(Reg),x86_Name(NewReg));
				x86Mapped(NewReg) = GPR_Mapped;
				x86MapOrder(NewReg) = x86MapOrder(Reg);
				MipsRegMapHi(count) = NewReg;
				MoveX86RegToX86Reg(Reg,NewReg);
				if (MipsReg == count && LoadHiWord == TRUE) { MipsReg = -1; }
				break;
			}
		}
	} 
	else if (x86Mapped(Reg) == Stack_Mapped) 
	{
		UnMap_X86reg(Reg);
	}
	if (x86Mapped(Reg) != Temp_Mapped)
	{
		CPU_Message("    regcache: allocate %s as temp storage",x86_Name(Reg));		
	}

	if (MipsReg >= 0) {
		if (LoadHiWord) {
			if (IsUnknown(MipsReg)) 
			{
				MoveVariableToX86reg(&_GPR[MipsReg].UW[1],CRegName::GPR_Hi[MipsReg],Reg);
			} 
			else if (IsMapped(MipsReg)) 
			{
				if (Is64Bit(MipsReg)) {
					MoveX86RegToX86Reg(cMipsRegMapHi(MipsReg),Reg);
				} else if (IsSigned(MipsReg)){
					MoveX86RegToX86Reg(cMipsRegMapLo(MipsReg),Reg);
					ShiftRightSignImmed(Reg,31);
				} else {
					MoveConstToX86reg(0,Reg);
				}
			} else {
				if (Is64Bit(MipsReg)) 
				{
					MoveConstToX86reg(MipsRegHi(MipsReg),Reg);
				} else {
					MoveConstToX86reg(MipsRegLo_S(MipsReg) >> 31,Reg);
				}
			}
		} else {
			if (IsUnknown(MipsReg)) {
				MoveVariableToX86reg(&_GPR[MipsReg].UW[0],CRegName::GPR_Lo[MipsReg],Reg);
			} else if (IsMapped(MipsReg)) {
				MoveX86RegToX86Reg(MipsRegMapLo(MipsReg),Reg);
			} else {
				MoveConstToX86reg(MipsRegLo(MipsReg),Reg);
			}
		}
	}
	x86Mapped(Reg) = Temp_Mapped;
	x86Protected(Reg) = TRUE;
	for (count = 0; count < 10; count ++) {
		if (x86MapOrder((x86Reg)count) > 0) { 
			x86MapOrder((x86Reg)count) += 1;
		}
	}
	x86MapOrder(Reg) = 1;
	return Reg;
}

void CRegInfo::ProtectGPR(DWORD Reg) {
	if (IsUnknown(Reg) || IsConst(Reg)) { return; }
	if (Is64Bit(Reg)) {
		x86Protected(MipsRegMapHi(Reg)) = TRUE;
	}
	x86Protected(MipsRegMapLo(Reg)) = TRUE;
}

void CRegInfo::UnProtectGPR(DWORD Reg) {
	if (IsUnknown(Reg) || IsConst(Reg)) { return; }
	if (Is64Bit(Reg)) {
		x86Protected(MipsRegMapHi(Reg)) = false;
	}
	x86Protected(MipsRegMapLo(Reg)) = false;
}

void CRegInfo::ResetX86Protection (void)
{
	for (int count = 0; count < 10; count ++) 
	{ 
		x86Protected((x86Reg)count) = false;
	}
}

BOOL CRegInfo::RegInStack( int Reg, FPU_STATE Format) {
	int i;

	for (i = 0; i < 8; i++) 
	{
		if (FpuMappedTo(i) == (DWORD)Reg) 
		{
			if (FpuState(i) == Format || Format == FPU_Any) 
			{ 
				return TRUE; 
			}
			return FALSE;
		}
	}
	return FALSE;
}

void CRegInfo::UnMap_AllFPRs ( void )
{
	DWORD StackPos;

	for (;;) {
		int i, StartPos;
		StackPos = StackTopPos();
		if (FpuMappedTo(StackTopPos()) != -1 ) {
			UnMap_FPR(FpuMappedTo(StackTopPos()),TRUE);
			continue;
		}
		//see if any more registers mapped
		StartPos = StackTopPos();
		for (i = 0; i < 8; i++) {
			if (FpuMappedTo((StartPos + i) & 7) != -1 ) { fpuIncStack(&StackTopPos()); }
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
		if (FpuMappedTo(i) != (DWORD)Reg) { continue; }
		CPU_Message("    regcache: unallocate %s from ST(%d)",CRegName::FPR[Reg],(i - StackTopPos() + 8) & 7);
		if (WriteBackValue) {
			int RegPos;
			
			if (((i - StackTopPos() + 8) & 7) != 0) {
				CRegInfo::FPU_ROUND RoundingModel = FpuRoundingModel(StackTopPos());
				FPU_STATE RegState      = FpuState(StackTopPos());
				DWORD MappedTo      = FpuMappedTo(StackTopPos());
				FpuRoundingModel(StackTopPos()) = FpuRoundingModel(i);
				FpuMappedTo(StackTopPos())      = FpuMappedTo(i);
				FpuState(StackTopPos())         = FpuState(i);
				FpuRoundingModel(i) = RoundingModel; 
				FpuMappedTo(i)      = MappedTo;
				FpuState(i)         = RegState;
				fpuExchange((x86FpuValues)((i - StackTopPos()) & 7));
			}
			
			FixRoundModel(FpuRoundingModel(i));

			RegPos = StackTopPos();
			x86Reg TempReg = Map_TempReg(x86_Any,-1,FALSE);
			switch (FpuState(StackTopPos())) {
			case FPU_Dword: 
				sprintf(Name,"_FPR_S[%d]",FpuMappedTo(StackTopPos()));
				MoveVariableToX86reg(&_FPR_S[FpuMappedTo(StackTopPos())],Name,TempReg);
				fpuStoreIntegerDwordFromX86Reg(&StackTopPos(),TempReg, TRUE); 
				break;
			case FPU_Qword: 
				sprintf(Name,"_FPR_D[%d]",FpuMappedTo(StackTopPos()));
				MoveVariableToX86reg(&_FPR_D[FpuMappedTo(StackTopPos())],Name,TempReg);
				fpuStoreIntegerQwordFromX86Reg(&StackTopPos(),TempReg, TRUE); 
				break;
			case FPU_Float: 
				sprintf(Name,"_FPR_S[%d]",FpuMappedTo(StackTopPos()));
				MoveVariableToX86reg(&_FPR_S[FpuMappedTo(StackTopPos())],Name,TempReg);
				fpuStoreDwordFromX86Reg(&StackTopPos(),TempReg, TRUE); 
				break;
			case FPU_Double: 
				sprintf(Name,"_FPR_D[%d]",FpuMappedTo(StackTopPos()));
				MoveVariableToX86reg(&_FPR_D[FpuMappedTo(StackTopPos())],Name,TempReg);
				fpuStoreQwordFromX86Reg(&StackTopPos(),TempReg, TRUE); 
				break;
#ifndef EXTERNAL_RELEASE
			default:
				DisplayError("UnMap_FPR\nUnknown format to load %d",FpuState(StackTopPos()));
#endif
			}
			x86Protected(TempReg) = FALSE;
			FpuRoundingModel(RegPos) = RoundDefault;
			FpuMappedTo(RegPos)      = -1;
			FpuState(RegPos)         = FPU_Unknown;
		} else {				
			fpuFree((x86FpuValues)((i - StackTopPos()) & 7));
			FpuRoundingModel(i) = RoundDefault;
			FpuMappedTo(i)      = -1;
			FpuState(i)         = FPU_Unknown;
		}
		return;
	}
}

void CRegInfo::UnMap_GPR (DWORD Reg, bool WriteBackValue) 
{
	if (Reg == 0) {
#ifndef EXTERNAL_RELEASE
		DisplayError("UnMap_GPR\n\nWhy are you trying to unmap reg 0");
#endif
		return;
	}

	if (IsUnknown(Reg)) { return; }
	//CPU_Message("UnMap_GPR: State: %X\tReg: %s\tWriteBack: %s",State,CRegName::GPR[Reg],WriteBackValue?"TRUE":"FALSE");
	if (IsConst(Reg)) { 
		if (!WriteBackValue) { 
			MipsRegState(Reg) = STATE_UNKNOWN;
			return; 
		}
		if (Is64Bit(Reg)) {
			MoveConstToVariable(MipsRegHi(Reg),&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
			MoveConstToVariable(MipsRegLo(Reg),&_GPR[Reg].UW[0],CRegName::GPR_Lo[Reg]);
			MipsRegState(Reg) = STATE_UNKNOWN;
			return;
		}
		if ((MipsRegLo(Reg) & 0x80000000) != 0) {
			MoveConstToVariable(0xFFFFFFFF,&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		} else {
			MoveConstToVariable(0,&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		}
		MoveConstToVariable(MipsRegLo(Reg),&_GPR[Reg].UW[0],CRegName::GPR_Lo[Reg]);
		MipsRegState(Reg) = STATE_UNKNOWN;
		return;
	}
	if (Is64Bit(Reg)) {
		CPU_Message("    regcache: unallocate %s from %s",x86_Name(MipsRegMapHi(Reg)),CRegName::GPR_Hi[Reg]);
		x86Mapped(MipsRegMapHi(Reg)) = NotMapped;
		x86Protected(MipsRegMapHi(Reg)) = FALSE;
	}
	CPU_Message("    regcache: unallocate %s from %s",x86_Name(MipsRegMapLo(Reg)),CRegName::GPR_Lo[Reg]);
	x86Mapped(MipsRegMapLo(Reg)) = NotMapped;
	x86Protected(MipsRegMapLo(Reg)) = FALSE;
	if (!WriteBackValue) { 
		MipsRegState(Reg) = STATE_UNKNOWN;
		return; 
	}
	MoveX86regToVariable(MipsRegMapLo(Reg),&_GPR[Reg].UW[0],CRegName::GPR_Lo[Reg]);
	if (Is64Bit(Reg)) {
		MoveX86regToVariable(MipsRegMapHi(Reg),&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
	} else {
		if (IsSigned(Reg)) {
			ShiftRightSignImmed(MipsRegMapLo(Reg),31);
			MoveX86regToVariable(MipsRegMapLo(Reg),&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		} else {
			MoveConstToVariable(0,&_GPR[Reg].UW[1],CRegName::GPR_Hi[Reg]);
		}
	}
	MipsRegState(Reg) = STATE_UNKNOWN;
}

CX86Ops::x86Reg CRegInfo::UnMap_TempReg ( void ) 
{
	CX86Ops::x86Reg Reg = x86_Unknown;

	if (x86Mapped(x86_EAX) == Temp_Mapped && !x86Protected(x86_EAX)) { Reg = x86_EAX; } 
	else if (x86Mapped(x86_EBX) == Temp_Mapped && !x86Protected(x86_EBX)) { Reg = x86_EBX; } 
	else if (x86Mapped(x86_ECX) == Temp_Mapped && !x86Protected(x86_ECX)) { Reg = x86_ECX; } 
	else if (x86Mapped(x86_EDX) == Temp_Mapped && !x86Protected(x86_EDX)) { Reg = x86_EDX; } 
	else if (x86Mapped(x86_ESI) == Temp_Mapped && !x86Protected(x86_ESI)) { Reg = x86_ESI; } 
	else if (x86Mapped(x86_EDI) == Temp_Mapped && !x86Protected(x86_EDI)) { Reg = x86_EDI; } 
	else if (x86Mapped(x86_EBP) == Temp_Mapped && !x86Protected(x86_EBP)) { Reg = x86_EBP; } 
	else if (x86Mapped(x86_ESP) == Temp_Mapped && !x86Protected(x86_ESP)) { Reg = x86_ESP; } 

	if (Reg != x86_Unknown)
	{
		if (x86Mapped(Reg) == Temp_Mapped)
		{
			CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(Reg));
		}
		x86Mapped(Reg) = NotMapped;
	}
	return Reg;
}

bool CRegInfo::UnMap_X86reg ( CX86Ops::x86Reg Reg )
{
	int count;

	if (x86Mapped(Reg) == NotMapped && x86Protected(Reg) == FALSE) { return TRUE; }
	if (x86Mapped(Reg) == CRegInfo::Temp_Mapped) { 
		if (x86Protected(Reg) == FALSE) {
			CPU_Message("    regcache: unallocate %s from temp storage",x86_Name(Reg));
			x86Mapped(Reg) = NotMapped;
			return TRUE;
		}
		return FALSE;
	}
	for (count = 1; count < 32; count ++) 
	{
		if (!IsMapped(count)) 
		{
			continue;
		}
		if (Is64Bit(count) && MipsRegMapHi(count) == Reg) 
		{
			if (x86Protected(Reg) == FALSE) 
			{
				UnMap_GPR(count,TRUE);
				return TRUE;
			}
			break;
		} 
		if (MipsRegMapLo(count) == Reg) 
		{
			if (x86Protected(Reg) == FALSE) 
			{
				UnMap_GPR(count,TRUE);
				return TRUE;
			}
			break;
		}
	}
	if (x86Mapped(Reg) == CRegInfo::Stack_Mapped) { 
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		CPU_Message("    regcache: unallocate %s from Memory Stack",x86_Name(Reg));
		MoveX86regToVariable(Reg,g_MemoryStack,"MemoryStack");
		x86Mapped(Reg) = NotMapped;
		return TRUE;
#endif
	}
	return FALSE;
}

void CRegInfo::WriteBackRegisters (void)
{
	int count;
	BOOL bEdiZero = FALSE;
	BOOL bEsiSign = FALSE;
	/*** coming soon ***/
	BOOL bEaxGprLo = FALSE;
	BOOL bEbxGprHi = FALSE;

	for (count = 0; count < 10; count ++) { x86Protected((CX86Ops::x86Reg)count) = FALSE; }
	for (count = 0; count < 10; count ++) { UnMap_X86reg ((CX86Ops::x86Reg)count); }

	/*************************************/
	
	for (count = 1; count < 32; count ++) {
		switch (MipsRegState(count)) {
		case CRegInfo::STATE_UNKNOWN: break;
		case CRegInfo::STATE_CONST_32:
			if (!bEdiZero && (!MipsRegLo(count) || !(MipsRegLo(count) & 0x80000000))) {
				XorX86RegToX86Reg(x86_EDI, x86_EDI);
				bEdiZero = TRUE;
			}
			if (!bEsiSign && (MipsRegLo(count) & 0x80000000)) {
				MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
				bEsiSign = TRUE;
			}

			if ((MipsRegLo(count) & 0x80000000) != 0) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} else {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			}

			if (MipsRegLo(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} else if (MipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} else
				MoveConstToVariable(MipsRegLo(count),&_GPR[count].UW[0],CRegName::GPR_Lo[count]);

			MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
			break;
		case CRegInfo::STATE_CONST_64:
			if (MipsRegLo(count) == 0 || MipsRegHi(count) == 0) {
				XorX86RegToX86Reg(x86_EDI, x86_EDI);
				bEdiZero = TRUE;
			}
			if (MipsRegLo(count) == 0xFFFFFFFF || MipsRegHi(count) == 0xFFFFFFFF) {
				MoveConstToX86reg(0xFFFFFFFF, x86_ESI);
				bEsiSign = TRUE;
			}

			if (MipsRegHi(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} else if (MipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} else {
				MoveConstToVariable(MipsRegHi(count),&_GPR[count].UW[1],CRegName::GPR_Hi[count]);
			} 

			if (MipsRegLo(count) == 0) {
				MoveX86regToVariable(x86_EDI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} else if (MipsRegLo(count) == 0xFFFFFFFF) {
				MoveX86regToVariable(x86_ESI,&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			} else {
				MoveConstToVariable(MipsRegLo(count),&_GPR[count].UW[0],CRegName::GPR_Lo[count]);
			}
			MipsRegState(count) = CRegInfo::STATE_UNKNOWN;
			break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Unknown State: %d\nin WriteBackRegisters",MipsRegState(count));
			BreakPoint(__FILE__,__LINE__);
#endif
		}
	}
	UnMap_AllFPRs();
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
