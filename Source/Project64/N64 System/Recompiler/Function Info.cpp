#include "stdafx.h"

CCompiledFunc::CCompiledFunc( const CCodeBlock & CodeBlock ) :
	m_EnterPC(CodeBlock.VAddrEnter()),
	m_MinPC(CodeBlock.VAddrFirst()),
	m_MaxPC(CodeBlock.VAddrLast()),
	m_Function((Func)CodeBlock.CompiledLocation())
{
}

#ifdef tofix

CCompiledFunc::CCompiledFunc(DWORD StartAddress, DWORD PhysicalStartAddress) :
	m_VEnterPC(StartAddress),
	m_PEnterPC(PhysicalStartAddress),
	m_VMinPC(StartAddress),
	m_VMaxPC(StartAddress),
	m_Function(NULL),
	Next(NULL)
{
	for (int i = 0; i < (sizeof(MemContents) / sizeof(MemContents[0])); i++ )
	{
		MemContents[i] = 0;
		MemLocation[i] = NULL;
	}
}

bool CCompiledFunc::CompilerCodeBlock(void)
{
	DWORD StartTime = timeGetTime();
	WriteTraceF(TraceRecompiler,"Compile Block-Start: VEnterPC: %X PEnterPC",m_VEnterPC,m_PEnterPC);
	
	//if (bProfiling())    { m_Profile.StartTimer(Timer_GetBlockInfo); }
	
	CBlockInfo BlockInfo(*_PROGRAM_COUNTER, RecompPos);
	//if (bProfiling())    { m_Profile.StartTimer(Timer_AnalyseBlock); }
#ifdef tofix
	if (bProfiling())    { m_Profile.StartTimer(Timer_CompileBlock); }
	_Notify->BreakPoint(__FILE__,__LINE__);
	DWORD StartAddress;
	if (!_TLB->TranslateVaddr(BlockInfo.StartVAddr,StartAddress))
	{
		DisplayError("Ummm... Where does this block go\n%X",BlockInfo.StartVAddr);
		return false;
	}
	MarkCodeBlock(StartAddress);
	if (StartAddress < RdramSize()) {
		CPU_Message("====== RDRAM: block (%X:%d) ======", StartAddress>>12,N64_Blocks.NoOfRDRamBlocks[StartAddress>>12]);
	} else if (StartAddress >= 0x04000000 && StartAddress <= 0x04000FFC) {
		CPU_Message("====== DMEM: block (%d) ======", N64_Blocks.NoOfDMEMBlocks);
	} else if (StartAddress >= 0x04001000 && StartAddress <= 0x04001FFC) {
		CPU_Message("====== IMEM: block (%d) ======", N64_Blocks.NoOfIMEMBlocks);
	} else if (StartAddress >= 0x1FC00000 && StartAddress <= 0x1FC00800) {
		CPU_Message("====== PIF ROM: block ======");
	} else {
#ifndef ROM_IN_MAPSPACE
#ifndef EXTERNAL_RELEASE
		DisplayError("Ummm... Where does this block go");
#endif
		ExitThread(0);			
#endif
	}
#endif
	CPU_Message("====== Code block ======");
	CPU_Message("VAddress: %X",BlockInfo.StartVAddr );
	CPU_Message("x86 code at: %X",BlockInfo.CompiledLocation);
	CPU_Message("No of Sections: %d",BlockInfo.NoOfSections );
	CPU_Message("====== recompiled code ======");
	/*if (bLinkBlocks()) {
		for (int count = 0; count < BlockInfo.NoOfSections; count ++) {
			DisplaySectionInformation(&BlockInfo.ParentSection,count + 1,CBlockSection::GetNewTestValue());
		}
	}
	if (m_SyncSystem) {
		//if ((DWORD)BlockInfo.CompiledLocation == 0x60A7B73B) { X86BreakPoint(__FILE__,__LINE__); }
		//MoveConstToVariable((DWORD)BlockInfo.CompiledLocation,&CurrentBlock,"CurrentBlock");
	}
	
	BlockInfo.ParentSection.RegStart.Initilize();
	BlockInfo.ParentSection.RegWorking = BlockInfo.ParentSection.RegStart;
	if (bLinkBlocks()) {
		while (GenerateX86Code(BlockInfo,&BlockInfo.ParentSection,CBlockSection::GetNewTestValue()));
	} else {
		GenerateX86Code(BlockInfo,&BlockInfo.ParentSection,CBlockSection::GetNewTestValue());
	}
	CompileExitCode(BlockInfo);
	
	CPU_Message("====== End of recompiled code ======");
	if (bProfiling())    { m_Profile.StartTimer(Timer_CompileDone); }

	info->SetVEndPC(BlockInfo.EndVAddr);
//	info->SetFunctionAddr(BlockInfo.CompiledLocation);
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	_TLB->VAddrToRealAddr(info->VStartPC(),*(reinterpret_cast<void **>(&info->MemLocation[0])));
	info->MemLocation[1] = info->MemLocation[0] + 1;
	info->MemContents[0] = *info->MemLocation[0];
	info->MemContents[1] = *info->MemLocation[1];
	if (bSMM_Protect())
	{
		_MMU->ProtectMemory(info->VStartPC(),info->VEndPC());
	}
	NextInstruction = NORMAL;

	if (bShowRecompMemSize()) 
	{
		DWORD Size = RecompPos - RecompCode;
		DWORD MB = Size / 0x100000;
		Size -= MB * 0x100000;
		DWORD KB = Size / 1024;
		Size -= KB  * 1024;

		DWORD TotalAvaliable = _MMU->GetRecompBufferSize() / 0x100000;
		
		DisplayMessage(0,"Memory used: %d mb %-3d kb %-3d bytes     Total Available: %d mb",MB,KB,Size, TotalAvaliable);
	}
	if (bProfiling())    { m_Profile.StopTimer(); }
#endif

	DWORD TimeTaken = timeGetTime() - StartTime;
	WriteTraceF(TraceRecompiler,"Compile Block-Done: %X-%X  - Taken: %d",info->VStartPC(),info->VEndPC(),TimeTaken);*/
	return true;
}

#endif