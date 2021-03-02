#include "stdafx.h"
#include "SectionInfo.h"
#include "JumpInfo.h"

CJumpInfo::CJumpInfo()
{
	TargetPC = (uint32_t)-1;
	JumpPC = (uint32_t)-1;
	BranchLabel = "";
	LinkLocation = NULL;
	LinkLocation2 = NULL;
	FallThrough = false;
	PermLoop = false;
	DoneDelaySlot = false;
	ExitReason = CExitInfo::Normal;
}

#ifdef legacycode

bool CCodeSection::IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, uint32_t Test)
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
	//			g_Notify->BreakPoint(__FILE__, __LINE__);
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
	//			g_Notify->BreakPoint(__FILE__, __LINE__);
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

CCodeSection::~CCodeSection()
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
			g_Notify->BreakPoint(__FILE__, __LINE__);
		}
		ContinueSection = NULL;
	}
	if (JumpSection)
	{
		JumpSection->UnlinkParent(this, true, false);
		if (JumpSection)
		{
			g_Notify->BreakPoint(__FILE__, __LINE__);
		}
		JumpSection = NULL;
	}
}

uint32_t CCodeSection::GetNewTestValue()
{
	static uint32_t LastTest = 0;
	if (LastTest == 0xFFFFFFFF) { LastTest = 0; }
	LastTest += 1;
	return LastTest;
}

void CRegInfo::Initialize()
{
	int count;

	MIPS_RegState[0]  = STATE_CONST_32_SIGN;
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