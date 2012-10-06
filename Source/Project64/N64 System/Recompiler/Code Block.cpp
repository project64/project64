#include "stdafx.h"

CCodeBlock::CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos) :
	m_VAddrEnter(VAddrEnter), 
	m_VAddrFirst(VAddrEnter),
	m_VAddrLast(VAddrEnter),
	m_CompiledLocation(RecompPos),
	m_NoOfSections(1),
	m_EnterSection(this, VAddrEnter, 1),
	m_Test(1)
{
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
	CPU_Message("====== Code Block ======");
	CPU_Message("x86 code at: %X",CompiledLocation());
	CPU_Message("Start of Block: %X",VAddrEnter() );
	CPU_Message("No of Sections: %d",NoOfSections() );
	CPU_Message("====== recompiled code ======");

	EnterCodeBlock();

	/*if (bLinkBlocks()) {
		for (int i = 0; i < m_NoOfSections; i ++) {
			m_EnterSection.DisplaySectionInformation(i + 1,NextTest());
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
		m_EnterSection.CompileExit((DWORD)-1, ExitIter->TargetPC,ExitIter->ExitRegSet,ExitIter->reason,true,NULL);
	}
}

DWORD CCodeBlock::NextTest ( void )
{
	m_Test += 1;
	return m_Test;
}
