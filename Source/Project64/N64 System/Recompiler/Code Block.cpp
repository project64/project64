#include "stdafx.h"

CCodeBlock::CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos, bool bDelaySlot) :
	m_VAddrEnter(VAddrEnter), 
	m_VAddrFirst(VAddrEnter),
	m_VAddrLast(VAddrEnter),
	m_CompiledLocation(RecompPos),
	m_NoOfSections(1),
	m_EnterSection(this, VAddrEnter, 1),
	m_bDelaySlot(bDelaySlot),
	m_Test(1)
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
		/*if (bLinkBlocks()) {
			for (int i = 0; i < m_NoOfSections; i ++) {
				m_EnterSection.DisplaySectionInformation(i + 1,m_EnterSection.m_Test + 1);
			}
		}*/
		if (_SyncSystem) {
			//if ((DWORD)BlockInfo.CompiledLocation == 0x60A7B73B) { X86BreakPoint(__FILE__,__LINE__); }
			//MoveConstToVariable((DWORD)BlockInfo.CompiledLocation,&CurrentBlock,"CurrentBlock");
		}

		if (bLinkBlocks()) {
			while (m_EnterSection.GenerateX86Code(NextTest()));
		} else {
			if (!m_EnterSection.GenerateX86Code(NextTest()))
			{
				return false;
			}
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
		m_EnterSection.CompileExit(-1, ExitIter->TargetPC,ExitIter->ExitRegSet,ExitIter->reason,true,NULL);
	}
}

DWORD CCodeBlock::NextTest ( void )
{
	m_Test += 1;
	return m_Test;
}
