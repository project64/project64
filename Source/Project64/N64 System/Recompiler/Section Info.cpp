#include "stdafx.h"

CCodeBlock::CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos, bool bDelaySlot) :
	m_VAddrEnter(VAddrEnter), 
	m_VAddrFirst(VAddrEnter),
	m_VAddrLast(VAddrEnter),
	m_CompiledLocation(RecompPos),
	m_NoOfSections(1),
	m_EnterSection(this, VAddrEnter, 1),
	m_bDelaySlot(bDelaySlot)
{
	AnalyseBlock();
}

bool CCodeBlock::AnalyseBlock ( void ) 
{
	if (bLinkBlocks())
	{ 	
		if (!m_EnterSection.CreateSectionLinkage ()) { return false; }
		m_EnterSection.DetermineLoop(NextTest(),NextTest(), m_EnterSection.m_SectionID);
		while (m_EnterSection.FixConstants(NextTest())) {}
	}
	return true;
}

bool CCodeBlock::Compile()
{
	if (m_bDelaySlot)
	{
		CPU_Message("====== Delay Block ======");
	} else {
		CPU_Message("====== Code Block ======");
	}
	CPU_Message("x86 code at: %X",CompiledLocation());
	CPU_Message("Start of Block: %X",VAddrEnter() );
	CPU_Message("No of Sections: %d",NoOfSections() );
	CPU_Message("====== recompiled code ======");
	
	if (m_bDelaySlot)
	{
		Pop(x86_EAX);
		MoveX86regToVariable(x86_EAX,_PROGRAM_COUNTER,"_PROGRAM_COUNTER");
	} else {
		EnterCodeBlock();
	}

	if (m_bDelaySlot)
	{
		m_EnterSection.GenerateX86Code(m_EnterSection.m_Test + 1);
	} else {
#ifdef tofix
	if (bLinkBlocks()) {
		for (int count = 0; count < BlockInfo.NoOfSections; count ++) {
			DisplaySectionInformation(&BlockInfo.ParentSection,count + 1,CBlockSection::GetNewTestValue());
		}
	}
	if (m_SyncSystem) {
		//if ((DWORD)BlockInfo.CompiledLocation == 0x60A7B73B) { X86BreakPoint(__FILE__,__LINE__); }
		//MoveConstToVariable((DWORD)BlockInfo.CompiledLocation,&CurrentBlock,"CurrentBlock");
	}
#endif
	
#ifdef tofix
	if (bLinkBlocks()) {
		while (GenerateX86Code(BlockInfo,&BlockInfo.ParentSection,CBlockSection::GetNewTestValue()));
	} else {
#endif
		if (!m_EnterSection.GenerateX86Code(m_EnterSection.m_Test + 1))
		{
			return false;
		}
#ifdef tofix
	}
#endif
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
		m_EnterSection.CompileExit(-1, ExitIter->TargetPC,ExitIter->ExitRegSet,ExitIter->reason,true,NULL);
	}
}

CJumpInfo::CJumpInfo()
{
	TargetPC      = (DWORD)-1;
	BranchLabel   = "";
	LinkLocation  = NULL;
	LinkLocation2 = NULL;
	FallThrough   = false;
	PermLoop      = false;
	DoneDelaySlot = false;
}

#ifdef tofix

bool CCodeSection::IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, DWORD Test) 
{ 
	if (IgnoreIfCompiled && Parent->CompiledLocation != NULL) { return true; }
	if (!InLoop) { return false; }
	if (!Parent->InLoop) { return false; }
	if (Parent->ParentSection.empty()) { return false; }
	if (this == Parent) { return true; }	
	if (Parent->Test == Test) { return true; }
	Parent->Test = Test;
		
	for (SECTION_LIST::iterator iter = Parent->ParentSection.begin(); iter != Parent->ParentSection.end(); iter++)
	{
		CCodeSection * ParentSection = *iter;
		if (!IsAllParentLoops(ParentSection,IgnoreIfCompiled,Test)) { return false; }
	}
	return true;
}

void CCodeSection::UnlinkParent( CCodeSection * Parent, bool AllowDelete, bool ContinueSection )
{
	if (this == NULL) 
	{
		return;
	}
	
	SECTION_LIST::iterator iter = ParentSection.begin();
	while ( iter != ParentSection.end())
	{
		CCodeSection * ParentIter = *iter;
		if (ParentIter == Parent && (Parent->ContinueSection != this || Parent->JumpSection != this))
		{
			ParentSection.erase(iter);
			iter = ParentSection.begin();
		} else {
			iter++;
		}
	}

//	if (Parent->ContinueSection != Parent->JumpSection)
//	{
//		if (!ContinueSection && Parent->ContinueSection == this)
//		{
//			_Notify->BreakPoint(__FILE__,__LINE__);
//		}
//	}
	if (ContinueSection && Parent->ContinueSection == this)
	{
		Parent->ContinueSection = NULL;
	}
//	if (Parent->ContinueSection != Parent->JumpSection)
//	{
//		if (ContinueSection && Parent->JumpSection == this)
//		{
//			_Notify->BreakPoint(__FILE__,__LINE__);
//		}
//	}
	if (!ContinueSection && Parent->JumpSection == this)
	{
		Parent->JumpSection = NULL;
	}
	if (AllowDelete)
	{
		bool KillMe = true;
		for (SECTION_LIST::iterator iter = ParentSection.begin(); iter != ParentSection.end(); iter++)
		{
			if (!IsAllParentLoops(*iter,false,GetNewTestValue()))
			{
				KillMe = false;
				break;
			}
		}
		if (KillMe)
		{
			delete this;
		}
	}	
}

CCodeSection::~CCodeSection ( void )
{
	while (ParentSection.size() > 0)
	{
		CCodeSection * Parent = *ParentSection.begin();
		if (Parent->ContinueSection == this) { UnlinkParent(Parent, false, true); }
		if (Parent->JumpSection == this)     { UnlinkParent(Parent, false, false); }
	}
	
	if (ContinueSection)
	{
		ContinueSection->UnlinkParent(this, true, true);
		if (ContinueSection)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
		ContinueSection = NULL;
	}
	if (JumpSection)
	{
		JumpSection->UnlinkParent(this, true, false);
		if (JumpSection)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
		JumpSection = NULL;
	}
}

DWORD CCodeSection::GetNewTestValue(void) 
{
	static DWORD LastTest = 0;
	if (LastTest == 0xFFFFFFFF) { LastTest = 0; }
	LastTest += 1;
	return LastTest;
}

void CRegInfo::Initilize ( void )
{
	int count;
	
	MIPS_RegState[0]  = STATE_CONST_32;
	MIPS_RegVal[0].DW = 0;
	for (count = 1; count < 32; count ++ ) {
		MIPS_RegState[count]   = STATE_UNKNOWN;
		MIPS_RegVal[count].DW = 0;

	}
	for (count = 0; count < 10; count ++ ) {
		x86reg_MappedTo[count]  = NotMapped;
		x86reg_Protected[count] = false;
		x86reg_MapOrder[count]  = 0;
	}
	CycleCount = 0;
	RandomModifier = 0;

	Stack_TopPos = 0;
	for (count = 0; count < 8; count ++ ) {
		x86fpu_MappedTo[count] = -1;
		x86fpu_State[count] = FPU_Unkown;
		x86fpu_RoundingModel[count] = RoundDefault;
	}
	Fpu_Used = false;
	RoundingModel = RoundUnknown;
}

#endif

CRegInfo::REG_STATE CRegInfo::ConstantsType (__int64 Value) 
{
	if (((Value >> 32) == -1) && ((Value & 0x80000000) != 0)) { return STATE_CONST_32; } 
	if (((Value >> 32) == 0) && ((Value & 0x80000000) == 0)) { return STATE_CONST_32; } 
	return STATE_CONST_64;
}

bool CRegInfo::compare(const CRegInfo& right) const
{
	int count;

	for (count = 0; count < 32; count ++ ) {
		if (MIPS_RegState[count] != right.MIPS_RegState[count]) 
		{
			return false; 
		}
		if (MIPS_RegState[count] == STATE_UNKNOWN)
		{
			continue;
		}
		if (MIPS_RegVal[count].DW != right.MIPS_RegVal[count].DW) 
		{
			return false; 
		}
	}
	for (count = 0; count < 10; count ++ ) {
		if (x86reg_MappedTo[count] != right.x86reg_MappedTo[count]) { return false; }
		if (x86reg_Protected[count] != right.x86reg_Protected[count]) { return false; }
		if (x86reg_MapOrder[count]  != right.x86reg_MapOrder[count]) { return false; }
	}
	if (m_CycleCount != right.m_CycleCount) { return false; }
	if (Stack_TopPos != right.Stack_TopPos) { return false; }

	for (count = 0; count < 8; count ++ ) {
		if (x86fpu_MappedTo[count]  != right.x86fpu_MappedTo[count]) { return false; }
		if (x86fpu_State[count]  != right.x86fpu_State[count]) { return false; }
		if (x86fpu_RoundingModel[count]  != right.x86fpu_RoundingModel[count]) { return false; }
	}
	if (Fpu_Used != right.Fpu_Used) { return false; }
	if (GetRoundingModel() != right.GetRoundingModel()) { return false; }
	return true;
}

bool CRegInfo::operator!=(const CRegInfo& right) const
{
	return !compare(right);
}


