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

CJumpInfo::CJumpInfo()
{
	TargetPC      = (DWORD)-1;
	JumpPC        = (DWORD)-1;
	BranchLabel   = "";
	LinkLocation  = NULL;
	LinkLocation2 = NULL;
	FallThrough   = false;
	PermLoop      = false;
	DoneDelaySlot = false;
	ExitReason    = CExitInfo::Normal;
}

#ifdef tofix

bool CCodeSection::IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, DWORD Test) 
{ 
	if (IgnoreIfCompiled && Parent->CompiledLocation != NULL)
	{
		return true;
	}
	if (!InLoop)
	{
		return false;
	}
	if (!Parent->InLoop)
	{
		return false;
	}
	if (Parent->ParentSection.empty())
	{
		return false;
	}
	if (this == Parent)
	{
		return true;
	}	
	if (Parent->Test == Test)
	{
		return true;
	}
	Parent->Test = Test;
		
	for (SECTION_LIST::iterator iter = Parent->ParentSection.begin(); iter != Parent->ParentSection.end(); iter++)
	{
		CCodeSection * ParentSection = *iter;
		if (!IsAllParentLoops(ParentSection,IgnoreIfCompiled,Test))
		{
			return false;
		}
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
		}
		else
		{
			iter++;
		}
	}

//	if (Parent->ContinueSection != Parent->JumpSection)
//	{
//		if (!ContinueSection && Parent->ContinueSection == this)
//		{
//			g_Notify->BreakPoint(__FILEW__,__LINE__);
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
//			g_Notify->BreakPoint(__FILEW__,__LINE__);
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
		if (Parent->ContinueSection == this)
		{
			UnlinkParent(Parent, false, true);
		}
		if (Parent->JumpSection == this)
		{
			UnlinkParent(Parent, false, false);
		}
	}
	
	if (ContinueSection)
	{
		ContinueSection->UnlinkParent(this, true, true);
		if (ContinueSection)
		{
			g_Notify->BreakPoint(__FILEW__,__LINE__);
		}
		ContinueSection = NULL;
	}
	if (JumpSection)
	{
		JumpSection->UnlinkParent(this, true, false);
		if (JumpSection)
		{
			g_Notify->BreakPoint(__FILEW__,__LINE__);
		}
		JumpSection = NULL;
	}
}

DWORD CCodeSection::GetNewTestValue(void) 
{
	static DWORD LastTest = 0;
	if (LastTest == 0xFFFFFFFF)
	{
		LastTest = 0;
	}
	LastTest += 1;
	return LastTest;
}

void CRegInfo::Initialize ( void )
{
	int count;
	
	MIPS_RegState[0]  = STATE_CONST_32_SIGN;
	MIPS_RegVal[0].DW = 0;
	for (count = 1; count < 32; count ++ )
	{
		MIPS_RegState[count]   = STATE_UNKNOWN;
		MIPS_RegVal[count].DW = 0;

	}
	for (count = 0; count < 10; count ++ )
	{
		x86reg_MappedTo[count]  = NotMapped;
		x86reg_Protected[count] = false;
		x86reg_MapOrder[count]  = 0;
	}
	CycleCount = 0;
	RandomModifier = 0;

	Stack_TopPos = 0;
	for (count = 0; count < 8; count ++ )
	{
		x86fpu_MappedTo[count] = -1;
		x86fpu_State[count] = FPU_Unkown;
		x86fpu_RoundingModel[count] = RoundDefault;
	}
	Fpu_Used = false;
	RoundingModel = RoundUnknown;
}

#endif
