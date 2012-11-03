#include "stdafx.h"

bool DelaySlotEffectsCompare (DWORD PC, DWORD Reg1, DWORD Reg2);

CCodeBlock::CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos) :
	m_VAddrEnter(VAddrEnter), 
	m_VAddrFirst(VAddrEnter),
	m_VAddrLast(VAddrEnter),
	m_CompiledLocation(RecompPos),
	m_Test(1),
	m_EnterSection(NULL)
{
	CCodeSection * baseSection = new CCodeSection(this, VAddrEnter, 0, false);
	if (baseSection == NULL)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
	m_Sections.push_back(baseSection);
	baseSection->AddParent(NULL);
	baseSection->m_CompiledLocation = (BYTE *)-1;
	baseSection->m_Cont.JumpPC = VAddrEnter;
	baseSection->m_Cont.FallThrough = true;
	baseSection->m_Cont.RegSet = baseSection->m_RegEnter;

	m_EnterSection = new CCodeSection(this, VAddrEnter, 1, true);
	if (m_EnterSection == NULL)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
	baseSection->m_ContinueSection = m_EnterSection;

	m_EnterSection->AddParent(baseSection);
	m_Sections.push_back(m_EnterSection);
	m_SectionMap.insert(SectionMap::value_type(VAddrEnter,m_EnterSection));

	if (_TransVaddr->VAddrToRealAddr(VAddrEnter,*(reinterpret_cast<void **>(&m_MemLocation[0]))))
	{
		m_MemLocation[1] = m_MemLocation[0] + 1;
		m_MemContents[0] = *m_MemLocation[0];
		m_MemContents[1] = *m_MemLocation[1];
	} else {
		memset(m_MemLocation,0,sizeof(m_MemLocation));
		memset(m_MemContents,0,sizeof(m_MemContents));
	}
	AnalyseBlock();
}

CCodeBlock::~CCodeBlock()
{
	for (SectionList::iterator itr = m_Sections.begin(); itr != m_Sections.end(); itr++)
	{
		CCodeSection * Section = *itr;
		delete Section;
	}
	m_Sections.clear();
}

bool CCodeBlock::SetSection ( CCodeSection * & Section, CCodeSection * CurrentSection, DWORD TargetPC, bool LinkAllowed, DWORD CurrentPC ) 
{
	if (Section != NULL)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
	
	if (TargetPC >= ((CurrentPC + 0x1000) & 0xFFFFF000))
	{
		return false;
	}

	if (TargetPC < m_EnterSection->m_EnterPC)
	{
		return false;
	}

	if (LinkAllowed)
	{
		if (Section != NULL)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
		SectionMap::const_iterator itr = m_SectionMap.find(TargetPC);
		if (itr != m_SectionMap.end())
		{
			Section = itr->second;
			Section->AddParent(CurrentSection);
		}
	}

	if (Section == NULL)
	{
		Section = new CCodeSection(this,TargetPC,m_Sections.size(),LinkAllowed);
		if (Section == NULL)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		m_Sections.push_back(Section);
		if (LinkAllowed)
		{
			m_SectionMap.insert(SectionMap::value_type(TargetPC,Section));
		}
		Section->AddParent(CurrentSection);
		if (TargetPC <= CurrentPC && TargetPC != m_VAddrEnter)
		{
			CCodeSection * SplitSection = NULL;
			for (SectionMap::const_iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
			{
				if (itr->first >= TargetPC)
				{
					break;
				}
				SplitSection = itr->second;
			}
			if (SplitSection == NULL)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			if (SplitSection->m_EndPC == (DWORD)-1)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			if (SplitSection->m_EndPC >= TargetPC)
			{
				CPU_Message(__FUNCTION__ ": Split Section: %d with section: %d",SplitSection->m_SectionID, Section->m_SectionID);
				CCodeSection * BaseSection = Section;
				BaseSection->m_EndPC = SplitSection->m_EndPC;
				BaseSection->SetJumpAddress(SplitSection->m_Jump.JumpPC, SplitSection->m_Jump.TargetPC,SplitSection->m_Jump.PermLoop);
				BaseSection->m_JumpSection = SplitSection->m_JumpSection;
				BaseSection->SetContinueAddress(SplitSection->m_Cont.JumpPC,SplitSection->m_Cont.TargetPC);
				BaseSection->m_ContinueSection = SplitSection->m_ContinueSection;
				BaseSection->m_JumpSection->SwitchParent(SplitSection,BaseSection);
				BaseSection->m_ContinueSection->SwitchParent(SplitSection,BaseSection);
				BaseSection->AddParent(SplitSection);

				SplitSection->m_EndPC = TargetPC - 4;
				SplitSection->m_JumpSection = NULL;
				SplitSection->m_ContinueSection = BaseSection;
				SplitSection->SetContinueAddress(TargetPC - 4, TargetPC);
				SplitSection->SetJumpAddress((DWORD)-1,(DWORD)-1,false);
			}
		}
	}
	return true;
}

bool CCodeBlock::CreateBlockLinkage ( CCodeSection * EnterSection ) 
{
	CCodeSection * CurrentSection = EnterSection;

	CPU_Message("Section %d",CurrentSection->m_SectionID);
	for (DWORD TestPC = EnterSection->m_EnterPC, EndPC = ((EnterSection->m_EnterPC + 0x1000) & 0xFFFFF000); TestPC <= EndPC; TestPC += 4)
	{
		if (TestPC != EndPC)
		{
			SectionMap::const_iterator itr = m_SectionMap.find(TestPC);
			if (itr != m_SectionMap.end() && CurrentSection != itr->second)
			{
				if (CurrentSection->m_ContinueSection != NULL && 
					CurrentSection->m_ContinueSection != itr->second)
				{
					_Notify->BreakPoint(__FILE__,__LINE__);
				}
				if (CurrentSection->m_ContinueSection == NULL)
				{
					SetSection(CurrentSection->m_ContinueSection, CurrentSection, TestPC,true,TestPC);
					CurrentSection->SetContinueAddress(TestPC - 4, TestPC);
				}
				CurrentSection->m_EndPC = TestPC - 4;
				CurrentSection = itr->second;
								
				CPU_Message("Section %d",CurrentSection->m_SectionID);
				if (EnterSection != m_EnterSection)
				{
					if (CurrentSection->m_JumpSection != NULL || 
						CurrentSection->m_ContinueSection != NULL ||
						CurrentSection->m_EndSection)
					{
						break;
					}
				}
			}
		} else {
			CurrentSection->m_EndSection = true;
			break;
		}

		bool LikelyBranch, EndBlock, IncludeDelaySlot, PermLoop;
		DWORD TargetPC, ContinuePC;

		CurrentSection->m_EndPC = TestPC; 
		if (!AnalyzeInstruction(TestPC, TargetPC, ContinuePC, LikelyBranch, IncludeDelaySlot, EndBlock, PermLoop))
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}

		if (TestPC + 4 == EndPC && IncludeDelaySlot)
		{
			TargetPC = (DWORD)-1;
			ContinuePC = (DWORD)-1;
			EndBlock = true;
		}
		if (TargetPC == (DWORD)-1 && !EndBlock)
		{
			if (ContinuePC != (DWORD)-1)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			continue;
		}

		if (EndBlock)
		{
			CPU_Message(__FUNCTION__ ": End Block");
			CurrentSection->m_EndSection = true;
			// find other sections that need compiling
			break;
		}

		if (ContinuePC != (DWORD)-1)
		{
			CPU_Message(__FUNCTION__ ": SetContinueAddress TestPC = %X ContinuePC = %X",TestPC,ContinuePC);
			CurrentSection->SetContinueAddress(TestPC, ContinuePC);
			if (!SetSection(CurrentSection->m_ContinueSection, CurrentSection, ContinuePC,true,TestPC))
			{
				ContinuePC = (DWORD)-1;
			}
		}

		if (LikelyBranch)
		{
			CPU_Message(__FUNCTION__ ": SetJumpAddress TestPC = %X Target = %X",TestPC,TestPC + 4);
			CurrentSection->SetJumpAddress(TestPC, TestPC + 4,false);
			if (SetSection(CurrentSection->m_JumpSection, CurrentSection, TestPC + 4,false,TestPC))
			{
				CCodeSection * JumpSection = CurrentSection->m_JumpSection;
				JumpSection->m_EndPC = TestPC + 4;
				JumpSection->SetJumpAddress(TestPC, TargetPC,false);
				JumpSection->SetDelaySlot();
				SetSection(JumpSection->m_JumpSection,CurrentSection->m_JumpSection,TargetPC,true,TestPC);
			} else {
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
		} 
		else if (TargetPC != ((DWORD)-1))
		{
			CPU_Message(__FUNCTION__ ": SetJumpAddress TestPC = %X Target = %X",TestPC,TargetPC);
			CurrentSection->SetJumpAddress(TestPC, TargetPC,PermLoop);
			if (PermLoop || !SetSection(CurrentSection->m_JumpSection, CurrentSection, TargetPC,true,TestPC))
			{
				if (ContinuePC == (DWORD)-1)
				{
					CurrentSection->m_EndSection = true;
				}
			}
		}

		TestPC += IncludeDelaySlot ? 8 : 4;

		//Find the next section
		CCodeSection * NewSection = NULL;
		for (SectionMap::const_iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
		{
			if (CurrentSection->m_JumpSection != NULL || 
				CurrentSection->m_ContinueSection != NULL ||
				CurrentSection->m_EndSection)
			{
				continue;
			}
			NewSection = itr->second;
			break;
		}
		if (NewSection == NULL)
		{
			break;
		}
		if (CurrentSection == NewSection)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
		CurrentSection = NewSection;
		if (CurrentSection->m_JumpSection != NULL || 
			CurrentSection->m_ContinueSection != NULL ||
			CurrentSection->m_EndSection)
		{
			break;
		}
		TestPC = CurrentSection->m_EnterPC;
		CPU_Message("a. Section %d",CurrentSection->m_SectionID);
		TestPC -= 4;
	}

	for (SectionMap::iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
	{
		CCodeSection * Section = itr->second;
		if (Section->m_JumpSection != NULL || 
			Section->m_ContinueSection != NULL ||
			Section->m_EndSection)
		{
			continue;
		}
		if (!CreateBlockLinkage(Section)) { return false; }
		break;
	}
	if (CurrentSection->m_EndPC == (DWORD)-1)
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
	return true;
}

void CCodeBlock::DetermineLoops ( void ) 
{
	for (SectionMap::iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
	{
		CCodeSection * Section = itr->second;

		DWORD Test = NextTest();
		Section->DetermineLoop(Test,Test,Section->m_SectionID);
	}
}

void CCodeBlock::LogSectionInfo ( void )
{
	for (SectionList::iterator itr = m_Sections.begin(); itr != m_Sections.end(); itr++)
	{
		CCodeSection * Section = *itr;
		Section->DisplaySectionInformation();
	}
}

bool CCodeBlock::AnalyseBlock ( void ) 
{
	if (!bLinkBlocks()) { return true; }
	if (!CreateBlockLinkage(m_EnterSection)) { return false; }
	DetermineLoops();
	LogSectionInfo();
	return true;
}

bool CCodeBlock::AnalyzeInstruction ( DWORD PC, DWORD & TargetPC, DWORD & ContinuePC, bool & LikelyBranch, bool & IncludeDelaySlot, bool & EndBlock, bool & PermLoop )
{
	TargetPC = (DWORD)-1;
	ContinuePC = (DWORD)-1;
	LikelyBranch = false;
	IncludeDelaySlot = false;
	EndBlock = false;
	PermLoop = false;

	OPCODE Command;
	if (!_MMU->LW_VAddr(PC, Command.Hex)) {
		_Notify->BreakPoint(__FILE__,__LINE__);
		return false;
	}

#ifdef _DEBUG
	char * Name = R4300iOpcodeName(Command.Hex,PC);
	CPU_Message("  0x%08X %s",PC,Name);
#endif
	switch (Command.op) 
	{
	case R4300i_SPECIAL:
		switch (Command.funct) 
		{
		case R4300i_SPECIAL_SLL:    case R4300i_SPECIAL_SRL:    case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:   case R4300i_SPECIAL_SRLV:   case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:   case R4300i_SPECIAL_MTHI:   case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:   case R4300i_SPECIAL_DSLLV:  case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:  case R4300i_SPECIAL_ADD:    case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:    case R4300i_SPECIAL_SUBU:   case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:     case R4300i_SPECIAL_XOR:    case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:    case R4300i_SPECIAL_SLTU:   case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:  case R4300i_SPECIAL_DSUB:   case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:   case R4300i_SPECIAL_DSRL:   case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32: case R4300i_SPECIAL_DSRL32: case R4300i_SPECIAL_DSRA32:
		case R4300i_SPECIAL_MULT:   case R4300i_SPECIAL_MULTU:  case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:   case R4300i_SPECIAL_DMULT:  case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:   case R4300i_SPECIAL_DDIVU:  
			break;
		case R4300i_SPECIAL_JALR:
		case R4300i_SPECIAL_JR:
			EndBlock = true;
			IncludeDelaySlot = true;
			break;
		case R4300i_SPECIAL_SYSCALL:
		case R4300i_SPECIAL_BREAK:
			EndBlock = true;
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		break;
	case R4300i_REGIMM:
		switch (Command.rt) 
		{
		case R4300i_REGIMM_BLTZ:
			TargetPC = PC + ((short)Command.offset << 2) + 4;
			if (TargetPC == PC + 8)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			if (TargetPC == PC)
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			ContinuePC = PC + 8;
			IncludeDelaySlot = true;
			break;
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BGEZAL:
			TargetPC = PC + ((short)Command.offset << 2) + 4;
			if (TargetPC == PC + 8)
			{
				TargetPC = (DWORD)-1;
			} else {
				if (TargetPC == PC)
				{
					if (Command.rs == 0)
					{
						TargetPC = (DWORD)-1;
						EndBlock = true;
					} else {
						if (!DelaySlotEffectsCompare(PC,Command.rs,Command.rt))
						{
							PermLoop = true;
						}
					}
				}
				if (Command.rs != 0)
				{
					ContinuePC = PC + 8;
				}
				IncludeDelaySlot = true;
			}
			break;
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
			TargetPC = PC + ((short)Command.offset << 2) + 4;
			if (TargetPC == PC)
			{
				if (!DelaySlotEffectsCompare(PC,Command.rs,0)) 
				{
					PermLoop = true;
				}
			}
			ContinuePC = PC + 8;
			LikelyBranch = true;
			IncludeDelaySlot = true;
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		break;
	case R4300i_J:
		TargetPC = (PC & 0xF0000000) + (Command.target << 2);
		if (TargetPC == PC)
		{
			PermLoop = true;
		}
		IncludeDelaySlot = true;
		break;
	case R4300i_JAL:
		EndBlock = true;
		IncludeDelaySlot = true;
		break;
	case R4300i_BEQ:
		TargetPC = PC + ((short)Command.offset << 2) + 4;
		if (TargetPC == PC + 8)
		{
			TargetPC = (DWORD)-1;
		} else {
			if (Command.rs != 0 || Command.rt != 0)
			{
				ContinuePC = PC + 8;
			}

			if (TargetPC == PC && !DelaySlotEffectsCompare(PC,Command.rs,Command.rt)) 
			{
				PermLoop = true;
			}
			IncludeDelaySlot = true;
		}
		break;
	case R4300i_BNE:
	case R4300i_BLEZ:
	case R4300i_BGTZ:
		TargetPC = PC + ((short)Command.offset << 2) + 4;
		if (TargetPC == PC + 8)
		{
			TargetPC = (DWORD)-1;
		} else {
			if (TargetPC == PC)
			{
				if (!DelaySlotEffectsCompare(PC,Command.rs,Command.rt))
				{
					PermLoop = true;
				}
			}
			ContinuePC = PC + 8;
			IncludeDelaySlot = true;
		}
		break;
	case R4300i_CP0:
		switch (Command.rs) 
		{
		case R4300i_COP0_MT: case R4300i_COP0_MF: 
			break;
		default:
			if ( (Command.rs & 0x10 ) != 0 ) 
			{
				switch( Command.funct ) {
				case R4300i_COP0_CO_TLBR: case R4300i_COP0_CO_TLBWI: 
				case R4300i_COP0_CO_TLBWR: case R4300i_COP0_CO_TLBP: 
					break;
				case R4300i_COP0_CO_ERET:
					EndBlock = true;
					break;
				default: 
					_Notify->BreakPoint(__FILE__,__LINE__);
					return false;
				}
			} else {
				_Notify->BreakPoint(__FILE__,__LINE__);
				return false;
			}
			break;
		}
		break;
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_MF:  case R4300i_COP1_DMF: case R4300i_COP1_CF: case R4300i_COP1_MT: 
		case R4300i_COP1_DMT: case R4300i_COP1_CT:  case R4300i_COP1_S:  case R4300i_COP1_D:  
		case R4300i_COP1_W:   case R4300i_COP1_L:
			break;
		case R4300i_COP1_BC:
			switch (Command.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
				TargetPC = PC + ((short)Command.offset << 2) + 4;
				if (TargetPC == PC + 8)
				{
					TargetPC = (DWORD)-1;
				} else {
					if (TargetPC == PC)
					{
						_Notify->BreakPoint(__FILE__,__LINE__);
					}
					ContinuePC = PC + 8;
					IncludeDelaySlot = true;
				}
				break;
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				TargetPC = PC + ((short)Command.offset << 2) + 4;
				if (TargetPC == PC)
				{
					_Notify->BreakPoint(__FILE__,__LINE__);
				}
				ContinuePC = PC + 8;
				LikelyBranch = true;
				IncludeDelaySlot = true;
				break;
			default:
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		break;
	case R4300i_ANDI:  case R4300i_ORI:    case R4300i_XORI:  case R4300i_LUI:
	case R4300i_ADDI:  case R4300i_ADDIU:  case R4300i_SLTI:  case R4300i_SLTIU:
	case R4300i_DADDI: case R4300i_DADDIU: case R4300i_LDL:   case R4300i_LDR: 
	case R4300i_LB:    case R4300i_LH:     case R4300i_LWL:   case R4300i_LW:
	case R4300i_LBU:   case R4300i_LHU:    case R4300i_LWR:   case R4300i_LWU: 
	case R4300i_SB:    case R4300i_SH:     case R4300i_SWL:   case R4300i_SW:
	case R4300i_SDL:   case R4300i_SDR:    case R4300i_SWR:   case R4300i_CACHE:
	case R4300i_LWC1:  case R4300i_LDC1:   case R4300i_LD:    case R4300i_SWC1:
	case R4300i_SDC1:  case R4300i_SD:
		break;
	case R4300i_BEQL:
		TargetPC = PC + ((short)Command.offset << 2) + 4;
		if (TargetPC == PC)
		{
			if (!DelaySlotEffectsCompare(PC,Command.rs,Command.rt)) 
			{
				PermLoop = true;
			}
		}
		if (Command.rs != 0 || Command.rt != 0)
		{
			ContinuePC = PC + 8;
		}
		IncludeDelaySlot = true;
		LikelyBranch = true;
		break;
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		TargetPC = PC + ((short)Command.offset << 2) + 4;
		ContinuePC = PC + 8;
		if (TargetPC == PC)
		{
			if (!DelaySlotEffectsCompare(PC,Command.rs,Command.rt)) 
			{
				PermLoop = true;
			}
		}
		LikelyBranch = true;
		IncludeDelaySlot = true;
		break;
	default:
		if (Command.Hex == 0x7C1C97C0)
		{
			EndBlock = true;
			break;
		}
		_Notify->BreakPoint(__FILE__,__LINE__);
		return false;
	}
	return true;
}

bool CCodeBlock::Compile()
{
	CPU_Message("====== Code Block ======");
	CPU_Message("x86 code at: %X",CompiledLocation());
	CPU_Message("Start of Block: %X",VAddrEnter() );
	CPU_Message("No of Sections: %d",NoOfSections() );
	CPU_Message("====== recompiled code ======");

	EnterCodeBlock();

	if (_SyncSystem) {
		//if ((DWORD)BlockInfo.CompiledLocation == 0x60A7B73B) { X86BreakPoint(__FILE__,__LINE__); }
		//MoveConstToVariable((DWORD)BlockInfo.CompiledLocation,&CurrentBlock,"CurrentBlock");
	}

	if (bLinkBlocks()) {
		while (m_EnterSection->GenerateX86Code(NextTest()));
	} else {
		if (!m_EnterSection->GenerateX86Code(NextTest()))
		{
			return false;
		}
	}
	CompileExitCode();

	DWORD PAddr;
	_TransVaddr->TranslateVaddr(VAddrFirst(),PAddr);
	MD5(_MMU->Rdram() + PAddr,(VAddrLast() - VAddrFirst()) + 4).get_digest(m_Hash);

	return true;
}


void CCodeBlock::CompileExitCode ( void )
{
	for (EXIT_LIST::iterator ExitIter = m_ExitInfo.begin(); ExitIter != m_ExitInfo.end(); ExitIter++)
	{
		CPU_Message("");
		CPU_Message("      $Exit_%d",ExitIter->ID);
		SetJump32(ExitIter->JumpLoc,(DWORD *)m_RecompPos);	
		m_NextInstruction = ExitIter->NextInstruction;
		m_EnterSection->CompileExit((DWORD)-1, ExitIter->TargetPC,ExitIter->ExitRegSet,ExitIter->reason,true,NULL);
	}
}

DWORD CCodeBlock::NextTest ( void )
{
	return InterlockedIncrement(&m_Test);
}