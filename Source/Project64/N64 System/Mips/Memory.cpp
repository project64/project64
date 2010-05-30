#include "stdafx.h"

void ** JumpTable, ** DelaySlotTable;
BYTE *RecompPos;

CMipsMemoryVM::CMipsMemoryVM ( CMipsMemory_CallBack * CallBack ) :
	m_CBClass(CallBack),
	m_TLB_ReadMap(NULL),
	m_TLB_WriteMap(NULL),
	m_RomMapped(false),
	m_Rom(NULL),
	m_RomSize(0),
	m_RomWrittenTo(false),
	m_RomWroteValue(0),
#ifdef toremove
	CTLB(System,m_RDRAM,RegSet),
//	CPIFRam(System->_Plugins,_Reg,Notify,SavesReadOnly),
	_System(System),
	_Rom2(CurrentRom),
	_Reg(RegSet),
#endif
	m_MemoryState(NULL),
	m_MemoryStateSize(0)
{ 
	m_RDRAM      = NULL;
	m_DMEM       = NULL;
	m_IMEM       = NULL;
#ifdef toremove
//	m_Sram     = NULL;
//	m_FlashRam = NULL;
//	m_SavesReadOnly = SavesReadOnly;
//	m_WrittenToRom  = false;
#endif
	m_HalfLine      = 0;
	JumpTable       = NULL;
//	DelaySlotTable  = NULL;
	m_RecompCode      = NULL;
	m_RecompSize    = 0;     
	//InitalizeSystem(true);
}

CMipsMemoryVM::~CMipsMemoryVM (void) 
{
	FreeMemory();
}

BOOL CMipsMemoryVM::Initialize ( void )
{
	if (m_RDRAM != NULL)
	{
		return true;
	}

	DWORD RdramMemorySize = 0x20000000;
	if ((CPU_TYPE)_Settings->LoadDword(Game_CpuType) == CPU_SyncCores)
	{
		RdramMemorySize = 0x18000000;
	}

	m_RDRAM = (unsigned char *) VirtualAlloc( NULL, RdramMemorySize, MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );
	if( m_RDRAM == NULL ) 
	{  
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Reserve RDRAM (Size: 0x%X)",RdramMemorySize);
		FreeMemory();
		return false;
	}
	
	m_AllocatedRdramSize = _Settings->LoadDword(Game_RDRamSize);
	if(VirtualAlloc(m_RDRAM, m_AllocatedRdramSize, MEM_COMMIT, PAGE_READWRITE)==NULL) 
	{
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate RDRAM (Size: 0x%X)",m_AllocatedRdramSize);
		FreeMemory();
		return false;
	}

	if(VirtualAlloc(m_RDRAM + 0x04000000, 0x2000, MEM_COMMIT, PAGE_READWRITE)==NULL)
	{
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate DMEM/IMEM (Size: 0x%X)",0x2000);
		FreeMemory();
		return false;
	}

	m_DMEM  = (unsigned char *)(m_RDRAM+0x04000000);
	m_IMEM  = (unsigned char *)(m_RDRAM+0x04001000);

	if (_Settings->LoadBool(Game_LoadRomToMemory))
	{
		m_RomMapped = true;
		m_Rom = m_RDRAM + 0x10000000;
		m_RomSize = _Rom->GetRomSize();
		if(VirtualAlloc(m_Rom, _Rom->GetRomSize(), MEM_COMMIT, PAGE_READWRITE)==NULL) 
		{
			WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate Rom (Size: 0x%X)",_Rom->GetRomSize());
			FreeMemory();
			return false;
		}
		memcpy(m_Rom,_Rom->GetRomAddress(),_Rom->GetRomSize());
		
		DWORD OldProtect;
		VirtualProtect(m_Rom,_Rom->GetRomSize(),PAGE_READONLY, &OldProtect);
	} else {
		m_RomMapped = false;
		m_Rom = _Rom->GetRomAddress();
		m_RomSize = _Rom->GetRomSize();
	}
	memset(m_PIF_Ram,0,sizeof(m_PIF_Ram));

	m_TLB_ReadMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (m_TLB_ReadMap == NULL) 
	{
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate TLB_ReadMap (Size: 0x%X)",0xFFFFF * sizeof(DWORD));
		FreeMemory();
		return false;
	}

	m_TLB_WriteMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (m_TLB_WriteMap == NULL) 
	{
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate TLB_ReadMap (Size: 0x%X)",0xFFFFF * sizeof(DWORD));
		FreeMemory();
		return false;
	}

	memset(m_TLB_ReadMap,0,(0xFFFFF * sizeof(DWORD)));
	memset(m_TLB_WriteMap,0,(0xFFFFF * sizeof(DWORD)));
	for (DWORD count = 0x80000000; count < 0xC0000000; count += 0x1000) 
	{
		m_TLB_ReadMap[count >> 12] = ((DWORD)m_RDRAM + (count & 0x1FFFFFFF)) - count;
		m_TLB_WriteMap[count >> 12] = ((DWORD)m_RDRAM + (count & 0x1FFFFFFF)) - count;
	}
	
	if (_Settings->LoadDword(Rdb_TLB_VAddrStart) != 0)
	{
		DWORD Start = _Settings->LoadDword(Rdb_TLB_VAddrStart); //0x7F000000;
		DWORD Len   = _Settings->LoadDword(Rdb_TLB_VAddrLen);   //0x01000000;
		DWORD PAddr = _Settings->LoadDword(Rdb_TLB_PAddrStart); //0x10034b30;
		DWORD End   = Start + Len;
		for (count = Start; count < End; count += 0x1000) {
			m_TLB_ReadMap[count >> 12] = ((DWORD)m_RDRAM + (count - Start + PAddr)) - count;
			m_TLB_WriteMap[count >> 12] = ((DWORD)m_RDRAM + (count - Start + PAddr)) - count;
		}
	}

	//TLB_Reset(true);
	return true;
}

void CMipsMemoryVM::FreeMemory ( void )
{
	if (m_RDRAM) 
	{
		VirtualFree( m_RDRAM, 0 , MEM_RELEASE);
		m_RDRAM = NULL;
		m_IMEM  = NULL;
		m_DMEM  = NULL;
	}
	if (m_TLB_ReadMap)
	{
		VirtualFree( m_TLB_ReadMap, 0 , MEM_RELEASE);
		m_TLB_ReadMap = NULL;
	}
	if (m_TLB_WriteMap)
	{
		VirtualFree( m_TLB_WriteMap, 0 , MEM_RELEASE);
		m_TLB_WriteMap = NULL;
	}
	memset(m_PIF_Ram,0,sizeof(m_PIF_Ram));
}

BYTE * CMipsMemoryVM::Rdram ( void )
{
	return m_RDRAM;
}

DWORD CMipsMemoryVM::RdramSize ( void )
{
	return m_AllocatedRdramSize; 
}

BYTE * CMipsMemoryVM::Dmem ( void )
{
	return m_DMEM;
}

BYTE * CMipsMemoryVM::Imem ( void )
{
	return m_IMEM;
}

BYTE * CMipsMemoryVM::PifRam ( void )
{
	return m_PIF_Ram;
}

#ifdef toremove
void CMipsMemoryVM::InitalizeSystem ( bool PostPif ) 
{
	ROM = _Rom->GetRomAddress();
	m_RomFileSize = _Rom->GetRomSize();
	
	AllocateSystemMemory();
	_Reg->InitalizeR4300iRegisters(this, PostPif, _Rom->GetCountry(), _Rom->CicChipID());
	if (PostPif) {
		memcpy( (DMEM+0x40), (ROM + 0x040), 0xFBC);
	}
}
#endif

void CMipsMemoryVM::AllocateSystemMemory (void) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (m_RDRAM != NULL)
	{
		memset(PIF_Ram,0,sizeof(PIF_Ram));
		TLB_Reset(true);
		return;
	}
	
	DWORD RdramMemorySize = 0x20000000;
	if ((CPU_TYPE)_Settings->LoadDword(Game_CpuType) == CPU_SyncCores)
	{
		RdramMemorySize = 0x18000000;
	}
	m_RDRAM = (unsigned char *) VirtualAlloc( NULL, RdramMemorySize, MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );
	if(m_RDRAM==NULL) {  
		_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
	}
	
	m_AllocatedRdramSize = 0x00400000;
	if(VirtualAlloc(m_RDRAM, m_AllocatedRdramSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
		_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
	}

	if(VirtualAlloc(m_RDRAM + 0x04000000, 0x2000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
		_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
	}

	DMEM  = (unsigned char *)(m_RDRAM+0x04000000);
	IMEM  = (unsigned char *)(m_RDRAM+0x04001000);

	if (_Settings->LoadBool(Game_LoadRomToMemory))
	{
		if(VirtualAlloc(m_RDRAM + 0x10000000, m_RomFileSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
			_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
		}
		memcpy(m_RDRAM + 0x10000000,ROM,m_RomFileSize);
		ROM = (unsigned char *)(m_RDRAM+0x10000000);
		_Rom->UnallocateRomImage();
	}
	memset(PIF_Ram,0,sizeof(PIF_Ram));

	TLB_ReadMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (TLB_ReadMap == NULL) {
		Notify().FatalError(MSG_MEM_ALLOC_ERROR);
	}

	TLB_WriteMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (TLB_WriteMap == NULL) {
		Notify().FatalError(MSG_MEM_ALLOC_ERROR);
	}


	TLB_Reset(true);
#endif
}

bool CMipsMemoryVM::AllocateRecompilerMemory ( bool AllocateJumpTable ) 
{
	if (JumpTable)
	{
		VirtualFree( JumpTable, 0 , MEM_RELEASE);
	}
	JumpTable = NULL;
	if (AllocateJumpTable)
	{
		DWORD JumpTableSize = _Settings->LoadDword(Game_LoadRomToMemory) ? 0x20000000 : 0x10000000;
		JumpTable = (void **)VirtualAlloc( NULL, JumpTableSize, MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );
		if( JumpTable == NULL ) {  
			_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
			return FALSE;
		}

		/* Jump Table */
		if(VirtualAlloc(JumpTable, m_AllocatedRdramSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
			_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
			return FALSE;
		}

		if(VirtualAlloc((BYTE *)JumpTable + 0x04000000, 0x2000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
			_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
			return FALSE;
		}

		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if (_Settings->LoadDword(Game_LoadRomToMemory))
		{
			if(VirtualAlloc((BYTE *)JumpTable + 0x10000000, m_RomFileSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
				_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
				return FALSE;
			}
		}
#endif		
	}

	/* Recomp code */
	if (m_RecompCode)
	{
		VirtualFree( m_RecompCode, 0 , MEM_RELEASE);
		m_RecompSize = 0;
	}
	m_RecompSize = InitialCompileBufferSize;
	m_RecompCode=(BYTE *) VirtualAlloc( NULL, MaxCompileBufferSize + 4, MEM_RESERVE|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
	m_RecompCode=(BYTE *) VirtualAlloc( m_RecompCode, m_RecompSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(m_RecompCode==NULL) {  
		_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
		return FALSE;
	}
	return true;
}

void CMipsMemoryVM::CheckRecompMem( BYTE * RecompPos )
{
	int Size = (int)((BYTE *)RecompPos - (BYTE *)m_RecompCode);
	if ((Size + 0x20000) < m_RecompSize)
	{
		return;
	}
	if (m_RecompSize == MaxCompileBufferSize) 
	{ 
		return; 
	}
	LPVOID MemAddr = VirtualAlloc( m_RecompCode + m_RecompSize , IncreaseCompileBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	m_RecompSize += IncreaseCompileBufferSize;

	if (MemAddr == NULL) {
		_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
	}
}

void CMipsMemoryVM::FixRDramSize ( void ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (_Settings->LoadDword(Game_RDRamSize) != m_AllocatedRdramSize) {
		if (m_AllocatedRdramSize == 0x400000) { 
			if (VirtualAlloc(m_RDRAM + 0x400000, 0x400000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
				_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
			}
			m_AllocatedRdramSize = 0x800000;
		} else {
			VirtualFree(m_RDRAM + 0x400000, 0x400000,MEM_DECOMMIT);
			m_AllocatedRdramSize = 0x400000;
		}
	}
#endif
}

BOOL CMipsMemoryVM::LB_VAddr ( DWORD VAddr, BYTE & Value ) 
{
	if (m_TLB_ReadMap[VAddr >> 12] == 0) { return FALSE; }
	Value = *(BYTE *)(m_TLB_ReadMap[VAddr >> 12] + (VAddr ^ 3));
	return TRUE;
}

BOOL CMipsMemoryVM::LH_VAddr ( DWORD VAddr, WORD & Value ) 
{
	if (m_TLB_ReadMap[VAddr >> 12] == 0) { return FALSE; }
	Value = *(WORD *)(m_TLB_ReadMap[VAddr >> 12] + (VAddr ^ 2));
	return TRUE;
}

BOOL CMipsMemoryVM::LW_VAddr ( DWORD VAddr, DWORD & Value ) 
{
	if (VAddr >= 0xA3F00000 && VAddr < 0xC0000000)
	{
		if (VAddr < 0xA4000000 || VAddr >= 0xA4002000)
		{
			VAddr &= 0x1FFFFFFF;
			LW_NonMemory(VAddr,&Value);
			return true;
		}
	}
	BYTE * BaseAddress = (BYTE *)m_TLB_ReadMap[VAddr >> 12];
	if (BaseAddress == 0) { return FALSE; }
	Value = *(DWORD *)(BaseAddress + VAddr);

//	if (LookUpMode == FuncFind_ChangeMemory)
//	{
//		BreakPoint(__FILE__,__LINE__);
//		if ( (Command.Hex >> 16) == 0x7C7C) {
//			Command.Hex = OrigMem[(Command.Hex & 0xFFFF)].OriginalValue;
//		}
//	}
	return TRUE;
	return false;
}

BOOL CMipsMemoryVM::LD_VAddr ( DWORD VAddr, QWORD & Value ) 
{
	if (m_TLB_ReadMap[VAddr >> 12] == 0) { return FALSE; }
	*((DWORD *)(&Value) + 1) = *(DWORD *)(m_TLB_ReadMap[VAddr >> 12] + VAddr);
	*((DWORD *)(&Value)) = *(DWORD *)(m_TLB_ReadMap[VAddr >> 12] + VAddr + 4);
	return TRUE;
}

BOOL CMipsMemoryVM::SB_VAddr ( DWORD VAddr, BYTE Value ) 
{
	if (m_TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(BYTE *)(m_TLB_WriteMap[VAddr >> 12] + (VAddr ^ 3)) = Value;
	return TRUE;
}

BOOL CMipsMemoryVM::SH_VAddr ( DWORD VAddr, WORD Value )
{
	if (m_TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(WORD *)(m_TLB_WriteMap[VAddr >> 12] + (VAddr ^ 2)) = Value;
	return TRUE;
}

BOOL CMipsMemoryVM::SW_VAddr ( DWORD VAddr, DWORD Value ) 
{
	if (VAddr >= 0xA3F00000 && VAddr < 0xC0000000)
	{
		if (VAddr < 0xA4000000 || VAddr >= 0xA4002000)
		{
			VAddr &= 0x1FFFFFFF;
			return SW_NonMemory(VAddr,Value);
		}
	}
	if (m_TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(DWORD *)(m_TLB_WriteMap[VAddr >> 12] + VAddr) = Value;
	return TRUE;
}


BOOL CMipsMemoryVM::SD_VAddr ( DWORD VAddr, QWORD Value )
{
	if (m_TLB_WriteMap[VAddr >> 12] == 0) { return FALSE; }
	*(DWORD *)(m_TLB_WriteMap[VAddr >> 12] + VAddr) = *((DWORD *)(&Value) + 1);
	*(DWORD *)(m_TLB_WriteMap[VAddr >> 12] + VAddr + 4) = *((DWORD *)(&Value));
	return TRUE;
}

bool CMipsMemoryVM::ValidVaddr ( DWORD VAddr ) const
{
	return m_TLB_ReadMap[VAddr >> 12] != 0;
}

bool CMipsMemoryVM::TranslateVaddr ( DWORD VAddr, DWORD &PAddr) const 
{
	//Change the Virtual address to a Phyiscal Address
	if (m_TLB_ReadMap[VAddr >> 12] == 0) { return false; }
	PAddr = (DWORD)((BYTE *)(m_TLB_ReadMap[VAddr >> 12] + VAddr) - m_RDRAM);
	return true;
}


#ifdef toremove
bool CMipsMemoryVM::Store64 ( DWORD VAddr, QWORD Value, MemorySize Size ) {
	//__try {
		DWORD PAddr;
		if (!TranslateVaddr(VAddr,PAddr)) {
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		if (PAddr > _Settings->LoadDword(Game_RDRamSize) && 
			(PAddr < 0x04000000 || PAddr > 0x04002000))
		{			
			switch (Size) {
			case _8Bit: 
				if (!StoreByte_NonMemory(PAddr,static_cast<BYTE>(Value))) {
					//MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<WORD>(Value));
				}
				return true;
				break;
			case _16Bit: 
				if (!StoreHalf_NonMemory(PAddr,static_cast<WORD>(Value))) {
					//MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<WORD>(Value));
				}
				return true;
				break;
			case _32Bit: 
				if (!StoreWord_NonMemory(PAddr,static_cast<DWORD>(Value))) {
					//MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<DWORD>(Value));
				}
				return true;
				break;
			default:
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			return false;
		}

		void * MemLoc;
		switch (Size) {
		case _8Bit: 
			if (!VAddrToRealAddr((VAddr ^ 3),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(BYTE *)MemLoc = (BYTE)Value;
			return true;
			break;
		case _16Bit: 
			if ((VAddr & 1) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr((VAddr ^ 2),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(WORD *)MemLoc = (WORD)Value;
			return true;
			break;
		case _32Bit: 
			if ((VAddr & 3) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(DWORD *)MemLoc = (DWORD)Value;
			return true;
			break;
		case _64Bit: 
			if ((VAddr & 7) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*((DWORD *)(MemLoc))     = *((DWORD *)(&Value) + 1);
			*(((DWORD *)MemLoc) + 1) = *((DWORD *)(&Value));
			return true;
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	//} __except( SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
	//	_Notify->FatalError("Unknown memory action\n\nEmulation stop");
	//}
	return false;
}

bool CMipsMemoryVM::StorePhysical64 ( DWORD PAddr, QWORD Value, MemorySize Size ) {
	if (PAddr > m_AllocatedRdramSize && 
		(PAddr < 0x04000000 || PAddr > 0x04002000))
	{			
		return false;
	}
	
	switch (Size) {
	case _8Bit:
		*(BYTE *)(m_RDRAM + (PAddr ^ 3)) = (BYTE)Value;
		return true;
	case _16Bit:
		*(WORD *)(m_RDRAM + (PAddr ^ 2)) = (WORD)Value;
		return true;
	case _32Bit:
		*(DWORD *)(m_RDRAM + PAddr) = (DWORD)Value;
		return true;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
	}

	return false;
}

bool CMipsMemoryVM::Load32 ( DWORD VAddr, DWORD & Variable, MemorySize Size, bool SignExtend ) {
	__try {
		void * MemLoc;

		switch (Size) {
		case _8Bit:
			if (!VAddrToRealAddr((VAddr ^ 3),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			Variable = (DWORD)SignExtend?(int)(*(char *)MemLoc):*(BYTE *)MemLoc;
			return true;
			break;
		case _16Bit:
			if ((VAddr & 1) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr((VAddr ^ 2),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			Variable = (DWORD)SignExtend?(int)(*(short *)MemLoc):*(WORD *)MemLoc;
			return true;
			break;
		case _32Bit:
			if ((VAddr & 3) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { return false; }
			Variable = *(DWORD *)MemLoc;
			return true;
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	} __except( SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		_Notify->FatalError("Unknown memory action\n\nEmulation stop");
	}
	return false;
}

bool CMipsMemoryVM::LoadPhysical32 ( DWORD PAddr, DWORD & Variable, MemorySize Size, bool SignExtend ) {
	if (PAddr >= 0x18000000)
	{
		return false;
	}
	__try {
		void * MemLoc;

		switch (Size) {
		case _8Bit:
			MemLoc = m_RDRAM + (PAddr ^ 3);
			Variable = (DWORD)SignExtend?(int)(*(char *)MemLoc):*(BYTE *)MemLoc;
			return true;
		case _16Bit:
			MemLoc = m_RDRAM + (PAddr ^ 2);
			Variable = (DWORD)SignExtend?(int)(*(short *)MemLoc):*(WORD *)MemLoc;
			return true;
		case _32Bit:
			MemLoc = m_RDRAM + PAddr;
			Variable = *(DWORD *)MemLoc;
			return true;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	} __except( SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		_Notify->FatalError("Unknown memory action\n\nEmulation stop");
	}
	return false;
}

void CMipsMemoryVM::MemoryFilterFailed( char * FailureType, DWORD MipsAddress,  DWORD x86Address, DWORD Value) {
	if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
		_Notify->DisplayError("Failed to %s\n\nProgram Counter: %X\nMIPS Address: %08X\nX86 Address: %X\n Value: %X",
			FailureType, _Reg->PROGRAM_COUNTER, MipsAddress, x86Address, Value);
	}
	return;
}
#endif

void  CMipsMemoryVM::Compile_LB ( CX86Ops::x86Reg Reg, DWORD VAddr, BOOL SignExtend) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD PAddr;
	char VarName[100];

	if (!TranslateVaddr(VAddr,PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LB\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LB\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
	case 0x10000000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		if (SignExtend) {
			MoveSxVariableToX86regByte(PAddr + m_RDRAM,VarName,Reg); 
		} else {
			MoveZxVariableToX86regByte(PAddr + m_RDRAM,VarName,Reg); 
		}
		break;
	default:
		MoveConstToX86reg(0,Reg);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LB\nFailed to compile address: %X",VAddr); }
	}
#endif
}

void  CMipsMemoryVM::Compile_LH ( CX86Ops::x86Reg Reg, DWORD VAddr, BOOL SignExtend) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LH\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LH\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
	case 0x10000000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		if (SignExtend) {
			MoveSxVariableToX86regHalf(PAddr + m_RDRAM,VarName,Reg); 
		} else {
			MoveZxVariableToX86regHalf(PAddr + m_RDRAM,VarName,Reg); 
		}
		break;
	default:
		MoveConstToX86reg(0,Reg);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LHU\nFailed to compile address: %X",VAddr); }
	}
#endif
}

void  CMipsMemoryVM::Compile_LW (CCodeSection * Section, CX86Ops::x86Reg Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LW\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address %X",VAddr); }
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
	case 0x10000000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveVariableToX86reg(PAddr + m_RDRAM,VarName,Reg); 
		break;
	case 0x04000000:
		if (PAddr < 0x04002000) { 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveVariableToX86reg(PAddr + m_RDRAM,VarName,Reg); 
			break; 
		}
		switch (PAddr) {
		case 0x04040010: MoveVariableToX86reg(&_Reg->SP_STATUS_REG,"SP_STATUS_REG",Reg); break;
		case 0x04040014: MoveVariableToX86reg(&_Reg->SP_DMA_FULL_REG,"SP_DMA_FULL_REG",Reg); break;
		case 0x04040018: MoveVariableToX86reg(&_Reg->SP_DMA_BUSY_REG,"SP_DMA_BUSY_REG",Reg); break;
		case 0x04080000: MoveVariableToX86reg(&_Reg->SP_PC_REG,"SP_PC_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04100000:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveVariableToX86reg(PAddr + m_RDRAM,VarName,Reg); 
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: MoveVariableToX86reg(&_Reg->MI_MODE_REG,"MI_MODE_REG",Reg); break;
		case 0x04300004: MoveVariableToX86reg(&_Reg->MI_VERSION_REG,"MI_VERSION_REG",Reg); break;
		case 0x04300008: MoveVariableToX86reg(&_Reg->MI_INTR_REG,"MI_INTR_REG",Reg); break;
		case 0x0430000C: MoveVariableToX86reg(&_Reg->MI_INTR_MASK_REG,"MI_INTR_MASK_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400010:
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix			
			Section->UpdateCounters(&RegInfo.BlockCycleCount(),&RegInfo.BlockRandomModifier(),FALSE);
			RegInfo.BlockCycleCount() = 0;
			RegInfo.BlockRandomModifier() = 0;
			Pushad();
			MoveConstToX86reg((DWORD)this,x86_ECX);
			Call_Direct(AddressOf(CMipsMemoryVM::UpdateHalfLine),"CMipsMemoryVM::UpdateHalfLine");
			Popad();
			MoveVariableToX86reg(&m_HalfLine,"m_HalfLine",Reg);
#endif
			break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500004: 
			if (_Settings->LoadBool(Game_FixedAudio))
			{
				_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix			
				CRecompilerOps::UpdateCounters(&RegInfo.BlockCycleCount(),&RegInfo.BlockRandomModifier(),FALSE);
				Pushad();
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);				
				Call_Direct(AddressOf(CAudio::AiGetLength),"AiGetLength");
				MoveX86regToVariable(x86_EAX,&m_TempValue,"m_TempValue"); 
				Popad();
				MoveVariableToX86reg(&m_TempValue,"m_TempValue",Reg);
#endif
			} else {
				if (AiReadLength != NULL) {
					Pushad();
					Call_Direct(AiReadLength,"AiReadLength");
					MoveX86regToVariable(x86_EAX,&m_TempValue,"m_TempValue"); 
					Popad();
					MoveVariableToX86reg(&m_TempValue,"m_TempValue",Reg);
				} else {
					MoveConstToX86reg(0,Reg);
				}						
			}
			break;
		case 0x0450000C: 
			if (_Settings->LoadBool(Game_FixedAudio))
			{
				Pushad();
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);
				Call_Direct(AddressOf(CAudio::AiGetStatus),"AiGetStatus");
				MoveX86regToVariable(x86_EAX,&m_TempValue,"m_TempValue"); 
				Popad();
				MoveVariableToX86reg(&m_TempValue,"m_TempValue",Reg);
			} else {
				MoveVariableToX86reg(&_Reg->AI_STATUS_REG,"AI_STATUS_REG",Reg); 
			}
			break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600010: MoveVariableToX86reg(&_Reg->PI_STATUS_REG,"PI_STATUS_REG",Reg); break;
		case 0x04600014: MoveVariableToX86reg(&_Reg->PI_DOMAIN1_REG,"PI_DOMAIN1_REG",Reg); break;
		case 0x04600018: MoveVariableToX86reg(&_Reg->PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG",Reg); break;
		case 0x0460001C: MoveVariableToX86reg(&_Reg->PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG",Reg); break;
		case 0x04600020: MoveVariableToX86reg(&_Reg->PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG",Reg); break;
		case 0x04600024: MoveVariableToX86reg(&_Reg->PI_DOMAIN2_REG,"PI_DOMAIN2_REG",Reg); break;
		case 0x04600028: MoveVariableToX86reg(&_Reg->PI_BSD_DOM2_PWD_REG,"PI_BSD_DOM2_PWD_REG",Reg); break;
		case 0x0460002C: MoveVariableToX86reg(&_Reg->PI_BSD_DOM2_PGS_REG,"PI_BSD_DOM2_PGS_REG",Reg); break;
		case 0x04600030: MoveVariableToX86reg(&_Reg->PI_BSD_DOM2_RLS_REG,"PI_BSD_DOM2_RLS_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x0470000C: MoveVariableToX86reg(&_Reg->RI_SELECT_REG,"RI_SELECT_REG",Reg); break;
		case 0x04700010: MoveVariableToX86reg(&_Reg->RI_REFRESH_REG,"RI_REFRESH_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: MoveVariableToX86reg(&_Reg->SI_STATUS_REG,"SI_STATUS_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x1FC00000:
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveVariableToX86reg(PAddr + m_RDRAM,VarName,Reg); 
		break;
	default:
		MoveConstToX86reg(((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF),Reg);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { 
			CPU_Message("Compile_LW\nFailed to translate address: %X",VAddr); 
			DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); 
		}
	}
}

void  CMipsMemoryVM::Compile_SB_Const ( BYTE Value, DWORD VAddr ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SB\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SB\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveConstByteToVariable(Value,PAddr + m_RDRAM,VarName); 
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SB_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
#endif
}

void  CMipsMemoryVM::Compile_SB_Register ( CX86Ops::x86Reg Reg, DWORD VAddr ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SB\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SB\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveX86regByteToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SB_Register\ntrying to store in %X?",VAddr); }
	}
#endif
}

void  CMipsMemoryVM::Compile_SH_Const ( WORD Value, DWORD VAddr ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SH\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SH\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveConstHalfToVariable(Value,PAddr + m_RDRAM,VarName); 
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SH_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
#endif
}

void CMipsMemoryVM::Compile_SH_Register ( CX86Ops::x86Reg Reg, DWORD VAddr ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SH\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SH\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveX86regHalfToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SH_Register\ntrying to store in %X?",PAddr); }
	}
#endif
}

void CMipsMemoryVM::Compile_SW_Const ( DWORD Value, DWORD VAddr ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	BYTE * Jump;
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SW\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
		break;
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: MoveConstToVariable(Value,&RDRAM_CONFIG_REG,"RDRAM_CONFIG_REG"); break;
		case 0x03F00004: MoveConstToVariable(Value,&RDRAM_DEVICE_ID_REG,"RDRAM_DEVICE_ID_REG"); break;
		case 0x03F00008: MoveConstToVariable(Value,&RDRAM_DELAY_REG,"RDRAM_DELAY_REG"); break;
		case 0x03F0000C: MoveConstToVariable(Value,&RDRAM_MODE_REG,"RDRAM_MODE_REG"); break;
		case 0x03F00010: MoveConstToVariable(Value,&RDRAM_REF_INTERVAL_REG,"RDRAM_REF_INTERVAL_REG"); break;
		case 0x03F00014: MoveConstToVariable(Value,&RDRAM_REF_ROW_REG,"RDRAM_REF_ROW_REG"); break;
		case 0x03F00018: MoveConstToVariable(Value,&RDRAM_RAS_INTERVAL_REG,"RDRAM_RAS_INTERVAL_REG"); break;
		case 0x03F0001C: MoveConstToVariable(Value,&RDRAM_MIN_INTERVAL_REG,"RDRAM_MIN_INTERVAL_REG"); break;
		case 0x03F00020: MoveConstToVariable(Value,&RDRAM_ADDR_SELECT_REG,"RDRAM_ADDR_SELECT_REG"); break;
		case 0x03F00024: MoveConstToVariable(Value,&RDRAM_DEVICE_MANUF_REG,"RDRAM_DEVICE_MANUF_REG"); break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04000000:
		if (PAddr < 0x04002000) { 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
			break;
		}
		switch (PAddr) {
		case 0x04040000: MoveConstToVariable(Value,&SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG"); break;
		case 0x04040004: MoveConstToVariable(Value,&SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG"); break;
		case 0x04040008:
			MoveConstToVariable(Value,&SP_RD_LEN_REG,"SP_RD_LEN_REG");
			Pushad();
			Call_Direct(&SP_DMA_READ,"SP_DMA_READ");
			Popad();
			break;
		case 0x04040010: 
			{
				DWORD ModValue;
				ModValue = 0;
				if ( ( Value & SP_CLR_HALT ) != 0 ) { ModValue |= SP_STATUS_HALT; }
				if ( ( Value & SP_CLR_BROKE ) != 0 ) { ModValue |= SP_STATUS_BROKE; }
				if ( ( Value & SP_CLR_SSTEP ) != 0 ) { ModValue |= SP_STATUS_SSTEP; }
				if ( ( Value & SP_CLR_INTR_BREAK ) != 0 ) { ModValue |= SP_STATUS_INTR_BREAK; }
				if ( ( Value & SP_CLR_SIG0 ) != 0 ) { ModValue |= SP_STATUS_SIG0; }
				if ( ( Value & SP_CLR_SIG1 ) != 0 ) { ModValue |= SP_STATUS_SIG1; }
				if ( ( Value & SP_CLR_SIG2 ) != 0 ) { ModValue |= SP_STATUS_SIG2; }
				if ( ( Value & SP_CLR_SIG3 ) != 0 ) { ModValue |= SP_STATUS_SIG3; }
				if ( ( Value & SP_CLR_SIG4 ) != 0 ) { ModValue |= SP_STATUS_SIG4; }
				if ( ( Value & SP_CLR_SIG5 ) != 0 ) { ModValue |= SP_STATUS_SIG5; }
				if ( ( Value & SP_CLR_SIG6 ) != 0 ) { ModValue |= SP_STATUS_SIG6; }
				if ( ( Value & SP_CLR_SIG7 ) != 0 ) { ModValue |= SP_STATUS_SIG7; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&SP_STATUS_REG,"SP_STATUS_REG");
				}

				ModValue = 0;
				if ( ( Value & SP_SET_HALT ) != 0 ) { ModValue |= SP_STATUS_HALT; }
				if ( ( Value & SP_SET_SSTEP ) != 0 ) { ModValue |= SP_STATUS_SSTEP; }
				if ( ( Value & SP_SET_INTR_BREAK ) != 0) { ModValue |= SP_STATUS_INTR_BREAK;  }
				if ( ( Value & SP_SET_SIG0 ) != 0 ) { ModValue |= SP_STATUS_SIG0; }
				if ( ( Value & SP_SET_SIG1 ) != 0 ) { ModValue |= SP_STATUS_SIG1; }
				if ( ( Value & SP_SET_SIG2 ) != 0 ) { ModValue |= SP_STATUS_SIG2; }
				if ( ( Value & SP_SET_SIG3 ) != 0 ) { ModValue |= SP_STATUS_SIG3; }
				if ( ( Value & SP_SET_SIG4 ) != 0 ) { ModValue |= SP_STATUS_SIG4; }
				if ( ( Value & SP_SET_SIG5 ) != 0 ) { ModValue |= SP_STATUS_SIG5; }
				if ( ( Value & SP_SET_SIG6 ) != 0 ) { ModValue |= SP_STATUS_SIG6; }
				if ( ( Value & SP_SET_SIG7 ) != 0 ) { ModValue |= SP_STATUS_SIG7; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&SP_STATUS_REG,"SP_STATUS_REG");
				}
				if ( ( Value & SP_SET_SIG0 ) != 0 && _Settings->LoadBool(Game_RspAudioSignal) ) 
				{ 
					OrConstToVariable(MI_INTR_SP,&MI_INTR_REG,"MI_INTR_REG");
					Pushad();
					Call_Direct(CheckInterrupts,"CheckInterrupts");
					Popad();
				}
				if ( ( Value & SP_CLR_INTR ) != 0) { 
					AndConstToVariable(~MI_INTR_SP,&MI_INTR_REG,"MI_INTR_REG");
					Pushad();
					Call_Direct(RunRsp,"RunRsp");
					Call_Direct(CheckInterrupts,"CheckInterrupts");
					Popad();
				} else {
					Pushad();
					Call_Direct(RunRsp,"RunRsp");
					Popad();
				}
			}
			break;
		case 0x0404001C: MoveConstToVariable(0,&SP_SEMAPHORE_REG,"SP_SEMAPHORE_REG"); break;
		case 0x04080000: MoveConstToVariable(Value & 0xFFC,&SP_PC_REG,"SP_PC_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			{
				DWORD ModValue;
				ModValue = 0x7F;
				if ( ( Value & MI_CLR_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_CLR_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_CLR_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&MI_MODE_REG,"MI_MODE_REG");
				}

				ModValue = (Value & 0x7F);
				if ( ( Value & MI_SET_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_SET_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_SET_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&MI_MODE_REG,"MI_MODE_REG");
				}
				if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
					AndConstToVariable(~MI_INTR_DP,&MI_INTR_REG,"MI_INTR_REG");
				}
			}
			break;
		case 0x0430000C: 
			{
				DWORD ModValue;
				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&MI_INTR_MASK_REG,"MI_INTR_MASK_REG");
				}

				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&MI_INTR_MASK_REG,"MI_INTR_MASK_REG");
				}
			}
			break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (ViStatusChanged != NULL) {
				CompConstToVariable(Value,&VI_STATUS_REG,"VI_STATUS_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveConstToVariable(Value,&VI_STATUS_REG,"VI_STATUS_REG");
				Pushad();
				Call_Direct(ViStatusChanged,"ViStatusChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x04400004: MoveConstToVariable((Value & 0xFFFFFF),&VI_ORIGIN_REG,"VI_ORIGIN_REG"); break;
		case 0x04400008: 
			if (ViWidthChanged != NULL) {
				CompConstToVariable(Value,&VI_WIDTH_REG,"VI_WIDTH_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveConstToVariable(Value,&VI_WIDTH_REG,"VI_WIDTH_REG");
				Pushad();
				Call_Direct(ViWidthChanged,"ViWidthChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x0440000C: MoveConstToVariable(Value,&VI_INTR_REG,"VI_INTR_REG"); break;
		case 0x04400010: 
			AndConstToVariable(~MI_INTR_VI,&MI_INTR_REG,"MI_INTR_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04400014: MoveConstToVariable(Value,&VI_BURST_REG,"VI_BURST_REG"); break;
		case 0x04400018: MoveConstToVariable(Value,&VI_V_SYNC_REG,"VI_V_SYNC_REG"); break;
		case 0x0440001C: MoveConstToVariable(Value,&VI_H_SYNC_REG,"VI_H_SYNC_REG"); break;
		case 0x04400020: MoveConstToVariable(Value,&VI_LEAP_REG,"VI_LEAP_REG"); break;
		case 0x04400024: MoveConstToVariable(Value,&VI_H_START_REG,"VI_H_START_REG"); break;
		case 0x04400028: MoveConstToVariable(Value,&VI_V_START_REG,"VI_V_START_REG"); break;
		case 0x0440002C: MoveConstToVariable(Value,&VI_V_BURST_REG,"VI_V_BURST_REG"); break;
		case 0x04400030: MoveConstToVariable(Value,&VI_X_SCALE_REG,"VI_X_SCALE_REG"); break;
		case 0x04400034: MoveConstToVariable(Value,&VI_Y_SCALE_REG,"VI_Y_SCALE_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500000: MoveConstToVariable(Value,&AI_DRAM_ADDR_REG,"AI_DRAM_ADDR_REG"); break;
		case 0x04500004: 
			MoveConstToVariable(Value,&AI_LEN_REG,"AI_LEN_REG");
			Pushad();
			if (_Settings->LoadBool(Game_FixedAudio))
			{
				X86BreakPoint(__FILE__,__LINE__);
				MoveConstToX86reg((DWORD)Value,x86_EDX);				
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);				
				Call_Direct(CAudio::AiSetLength,"AiSetLength");
			}
			Call_Direct(AiLenChanged,"AiLenChanged");
			Popad();
			break;
		case 0x04500008: MoveConstToVariable((Value & 1),&AI_CONTROL_REG,"AI_CONTROL_REG"); break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			AndConstToVariable(~MI_INTR_AI,&MI_INTR_REG,"MI_INTR_REG");
			if (!_Settings->LoadBool(Game_FixedAudio))
			{
				AndConstToVariable(~MI_INTR_AI,&_Reg->AudioIntrReg,"AudioIntrReg");
			}
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04500010: 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
			break;
		case 0x04500014: MoveConstToVariable(Value,&AI_BITRATE_REG,"AI_BITRATE_REG"); break;
		default:
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: MoveConstToVariable(Value,&PI_DRAM_ADDR_REG,"PI_DRAM_ADDR_REG"); break;
		case 0x04600004: MoveConstToVariable(Value,&PI_CART_ADDR_REG,"PI_CART_ADDR_REG"); break;
		case 0x04600008: 
			MoveConstToVariable(Value,&PI_RD_LEN_REG,"PI_RD_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_READ,"PI_DMA_READ");
			Popad();
			break;
		case 0x0460000C:
			MoveConstToVariable(Value,&PI_WR_LEN_REG,"PI_WR_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_WRITE,"PI_DMA_WRITE");
			Popad();
			break;
		case 0x04600010: 
			if ((Value & PI_CLR_INTR) != 0 ) {
				AndConstToVariable(~MI_INTR_PI,&MI_INTR_REG,"MI_INTR_REG");
				Pushad();
				Call_Direct(CheckInterrupts,"CheckInterrupts");
				Popad();
			}
			break;
		case 0x04600014: MoveConstToVariable((Value & 0xFF),&PI_DOMAIN1_REG,"PI_DOMAIN1_REG"); break;
		case 0x04600018: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); break;
		case 0x0460001C: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); break;
		case 0x04600020: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: MoveConstToVariable(Value,&RI_MODE_REG,"RI_MODE_REG"); break;
		case 0x04700004: MoveConstToVariable(Value,&RI_CONFIG_REG,"RI_CONFIG_REG"); break;
		case 0x04700008: MoveConstToVariable(Value,&RI_CURRENT_LOAD_REG,"RI_CURRENT_LOAD_REG"); break;
		case 0x0470000C: MoveConstToVariable(Value,&RI_SELECT_REG,"RI_SELECT_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: MoveConstToVariable(Value,&SI_DRAM_ADDR_REG,"SI_DRAM_ADDR_REG"); break;
		case 0x04800004: 			
			MoveConstToVariable(Value,&SI_PIF_ADDR_RD64B_REG,"SI_PIF_ADDR_RD64B_REG");		
			Pushad();
			Call_Direct(&SI_DMA_READ,"SI_DMA_READ");
			Popad();
			break;
		case 0x04800010: 
			MoveConstToVariable(Value,&SI_PIF_ADDR_WR64B_REG,"SI_PIF_ADDR_WR64B_REG");
			Pushad();
			Call_Direct(&SI_DMA_WRITE,"SI_DMA_WRITE");
			Popad();
			break;
		case 0x04800018: 
			AndConstToVariable(~MI_INTR_SI,&MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable(~SI_STATUS_INTERRUPT,&SI_STATUS_REG,"SI_STATUS_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
#endif
}

void CMipsMemoryVM::Compile_SW_Register (CRegInfo & RegInfo, CX86Ops::x86Reg Reg, DWORD VAddr ) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	char VarName[100];
	BYTE * Jump;
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SW_Register\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\nFailed to translate address %X",VAddr); }
		return;
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000: 
	case 0x00100000: 
	case 0x00200000: 
	case 0x00300000: 
	case 0x00400000: 
	case 0x00500000: 
	case 0x00600000: 
	case 0x00700000: 
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveX86regToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
		break;
	case 0x04000000: 
		switch (PAddr) {
		case 0x04040000: MoveX86regToVariable(x86Reg,&SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG"); break;
		case 0x04040004: MoveX86regToVariable(x86Reg,&SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG"); break;
		case 0x04040008: 
			MoveX86regToVariable(x86Reg,&SP_RD_LEN_REG,"SP_RD_LEN_REG");
			Pushad();
			Call_Direct(&SP_DMA_READ,"SP_DMA_READ");
			Popad();
			break;
		case 0x0404000C: 
			MoveX86regToVariable(x86Reg,&SP_WR_LEN_REG,"SP_WR_LEN_REG");
			Pushad();
			Call_Direct(&SP_DMA_WRITE,"SP_DMA_WRITE");
			Popad();
			break;
		case 0x04040010: 
			MoveX86regToVariable(x86Reg,&RegModValue,"RegModValue");
			Pushad();
			Call_Direct(ChangeSpStatus,"ChangeSpStatus");
			Popad();
			break;
		case 0x0404001C: MoveConstToVariable(0,&SP_SEMAPHORE_REG,"SP_SEMAPHORE_REG"); break;
		case 0x04080000: 
			MoveX86regToVariable(x86Reg,&SP_PC_REG,"SP_PC_REG");
			AndConstToVariable(0xFFC,&SP_PC_REG,"SP_PC_REG");
			break;
		default:
			if (PAddr < 0x04002000) {
				sprintf(VarName,"m_RDRAM + %X",PAddr);
				MoveX86regToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
			} else {
				CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
				if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
			}
		}
		break;
	case 0x04100000: 
		CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveX86regToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			MoveX86regToVariable(x86Reg,&RegModValue,"RegModValue");
			Pushad();
			Call_Direct(ChangeMiIntrMask,"ChangeMiModeReg");
			Popad();
			break;
		case 0x0430000C: 
			MoveX86regToVariable(x86Reg,&RegModValue,"RegModValue");
			Pushad();
			Call_Direct(ChangeMiIntrMask,"ChangeMiIntrMask");
			Popad();
			break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (ViStatusChanged != NULL) {
				CompX86regToVariable(x86Reg,&VI_STATUS_REG,"VI_STATUS_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveX86regToVariable(x86Reg,&VI_STATUS_REG,"VI_STATUS_REG");
				Pushad();
				Call_Direct(ViStatusChanged,"ViStatusChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x04400004: 
			MoveX86regToVariable(x86Reg,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			AndConstToVariable(0xFFFFFF,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			break;
		case 0x04400008: 
			if (ViWidthChanged != NULL) {
				CompX86regToVariable(x86Reg,&VI_WIDTH_REG,"VI_WIDTH_REG");
				JeLabel8("Continue",0);
				Jump = RecompPos - 1;
				MoveX86regToVariable(x86Reg,&VI_WIDTH_REG,"VI_WIDTH_REG");
				Pushad();
				Call_Direct(ViWidthChanged,"ViWidthChanged");
				Popad();
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			}
			break;
		case 0x0440000C: MoveX86regToVariable(x86Reg,&VI_INTR_REG,"VI_INTR_REG"); break;
		case 0x04400010: 
			AndConstToVariable(~MI_INTR_VI,&MI_INTR_REG,"MI_INTR_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04400014: MoveX86regToVariable(x86Reg,&VI_BURST_REG,"VI_BURST_REG"); break;
		case 0x04400018: MoveX86regToVariable(x86Reg,&VI_V_SYNC_REG,"VI_V_SYNC_REG"); break;
		case 0x0440001C: MoveX86regToVariable(x86Reg,&VI_H_SYNC_REG,"VI_H_SYNC_REG"); break;
		case 0x04400020: MoveX86regToVariable(x86Reg,&VI_LEAP_REG,"VI_LEAP_REG"); break;
		case 0x04400024: MoveX86regToVariable(x86Reg,&VI_H_START_REG,"VI_H_START_REG"); break;
		case 0x04400028: MoveX86regToVariable(x86Reg,&VI_V_START_REG,"VI_V_START_REG"); break;
		case 0x0440002C: MoveX86regToVariable(x86Reg,&VI_V_BURST_REG,"VI_V_BURST_REG"); break;
		case 0x04400030: MoveX86regToVariable(x86Reg,&VI_X_SCALE_REG,"VI_X_SCALE_REG"); break;
		case 0x04400034: MoveX86regToVariable(x86Reg,&VI_Y_SCALE_REG,"VI_Y_SCALE_REG"); break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500000: MoveX86regToVariable(x86Reg,&AI_DRAM_ADDR_REG,"AI_DRAM_ADDR_REG"); break;
		case 0x04500004: 
			MoveX86regToVariable(x86Reg,&AI_LEN_REG,"AI_LEN_REG");
			Pushad();
			if (_Settings->LoadBool(Game_FixedAudio))
			{
				_N64System->GetRecompiler()->UpdateCounters(&Section->BlockCycleCount(),&Section->BlockRandomModifier(),FALSE);			
				X86BreakPoint(__FILE__,__LINE__);
				MoveX86RegToX86Reg(x86Reg,x86_EDX);				
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);				
				Call_Direct(CAudio::AiSetLength,"AiSetLength");
			}
			Call_Direct(AiLenChanged,"AiLenChanged");
			Popad();
			break;
		case 0x04500008: 
			MoveX86regToVariable(x86Reg,&AI_CONTROL_REG,"AI_CONTROL_REG");
			AndConstToVariable(1,&AI_CONTROL_REG,"AI_CONTROL_REG");
		case 0x0450000C:
			/* Clear Interrupt */; 
			AndConstToVariable(~MI_INTR_AI,&MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable(~MI_INTR_AI,&_Reg->AudioIntrReg,"AudioIntrReg");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		case 0x04500010: 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveX86regToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
			break;
		case 0x04500014: MoveX86regToVariable(x86Reg,&AI_BITRATE_REG,"AI_BITRATE_REG"); break;
		default:
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveX86regToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: MoveX86regToVariable(x86Reg,&PI_DRAM_ADDR_REG,"PI_DRAM_ADDR_REG"); break;
		case 0x04600004: MoveX86regToVariable(x86Reg,&PI_CART_ADDR_REG,"PI_CART_ADDR_REG"); break;
		case 0x04600008:
			MoveX86regToVariable(x86Reg,&PI_RD_LEN_REG,"PI_RD_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_READ,"PI_DMA_READ");
			Popad();
			break;
		case 0x0460000C:
			MoveX86regToVariable(x86Reg,&PI_WR_LEN_REG,"PI_WR_LEN_REG");
			Pushad();
			Call_Direct(&PI_DMA_WRITE,"PI_DMA_WRITE");
			Popad();
			break;
		case 0x04600010: 
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
			AndConstToVariable(~MI_INTR_PI,&MI_INTR_REG,"MI_INTR_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
			MoveX86regToVariable(x86Reg,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			AndConstToVariable(0xFFFFFF,&VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
		case 0x04600014: 
			MoveX86regToVariable(x86Reg,&PI_DOMAIN1_REG,"PI_DOMAIN1_REG");
			AndConstToVariable(0xFF,&PI_DOMAIN1_REG,"PI_DOMAIN1_REG"); 
			break;
		case 0x04600018: 
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); 
			AndConstToVariable(0xFF,&PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); 
			break;
		case 0x0460001C: 
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); 
			AndConstToVariable(0xFF,&PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); 
			break;
		case 0x04600020: 
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); 
			AndConstToVariable(0xFF,&PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); 
			break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700010: MoveX86regToVariable(x86Reg,&RI_REFRESH_REG,"RI_REFRESH_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: MoveX86regToVariable(x86Reg,&SI_DRAM_ADDR_REG,"SI_DRAM_ADDR_REG"); break;
		case 0x04800004: 
			MoveX86regToVariable(x86Reg,&SI_PIF_ADDR_RD64B_REG,"SI_PIF_ADDR_RD64B_REG"); 
			Pushad();
			Call_Direct(&SI_DMA_READ,"SI_DMA_READ");
			Popad();
			break;
		case 0x04800010: 
			MoveX86regToVariable(x86Reg,&SI_PIF_ADDR_WR64B_REG,"SI_PIF_ADDR_WR64B_REG"); 
			Pushad();
			Call_Direct(&SI_DMA_WRITE,"SI_DMA_WRITE");
			Popad();
			break;
		case 0x04800018: 
			AndConstToVariable(~MI_INTR_SI,&MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable(~SI_STATUS_INTERRUPT,&SI_STATUS_REG,"SI_STATUS_REG");
			Pushad();
			Call_Direct(CheckInterrupts,"CheckInterrupts");
			Popad();
			break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x1FC00000:
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveX86regToVariable(x86Reg,PAddr + m_RDRAM,VarName); 
		break;
	default:
		CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(x86Reg),VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { DisplayError("Compile_SW_Register\ntrying to store in %X?",VAddr); }
	}
#endif
}

void CMipsMemoryVM::ResetMemoryStack (CRegInfo & RegInfo) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	int x86reg, TempReg;

	CPU_Message("    ResetMemoryStack");
	x86reg = Map_MemoryStack(Section, x86_Any, false);
	if (x86reg >= 0) { UnMap_X86reg(Section,x86reg); }

	x86reg = Map_TempReg(Section,x86_Any, 29, FALSE);
	if (_Settings->LoadBool(Game_UseTlb)) 
	{	
	    TempReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(x86reg,TempReg);
		ShiftRightUnsignImmed(TempReg,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg,TempReg,4);
		AddX86RegToX86Reg(x86reg,TempReg);
	} else {
		AndConstToX86Reg(x86reg,0x1FFFFFFF);
		AddConstToX86Reg(x86reg,(DWORD)m_RDRAM);
	}
	MoveX86regToVariable(x86reg, g_MemoryStack, "MemoryStack");
#endif
}

int CMipsMemoryVM::MemoryFilter( DWORD dwExptCode, void * lpExceptionPointer ) 
{
	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) 
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	//convert the pointer since we are not having win32 stuctures in headers
	LPEXCEPTION_POINTERS lpEP = (LPEXCEPTION_POINTERS)lpExceptionPointer;

	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)m_RDRAM;
    if ((int)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF) { return EXCEPTION_CONTINUE_SEARCH; }

	DWORD * Reg;
	
	BYTE * TypePos = (unsigned char *)lpEP->ContextRecord->Eip;
	EXCEPTION_RECORD exRec = *lpEP->ExceptionRecord;
	
	if (*TypePos == 0xF3 && *(TypePos + 1) == 0xA5) {
		DWORD Start, End, count, OldProtect;
		Start = (lpEP->ContextRecord->Edi - (DWORD)m_RDRAM);
		End = (Start + (lpEP->ContextRecord->Ecx << 2) - 1);
		if ((int)Start < 0) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("hmmm.... where does this dma start ?");
#endif
			return EXCEPTION_CONTINUE_SEARCH;
		}
#ifdef CFB_READ
		if (Start >= CFBStart && End < CFBEnd) {
			for ( count = Start; count < End; count += 0x1000 ) {
				VirtualProtect(m_RDRAM+count,4,PAGE_READONLY, &OldProtect);
				if (FrameBufferRead) { FrameBufferRead(count & ~0xFFF); }
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}	
#endif
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if ((int)End < RdramSize) {
			for ( count = Start; count < End; count += 0x1000 ) {
				BreakPoint(__FILE__,__LINE__);
				if (N64_Blocks.NoOfRDRamBlocks[(count >> 12)] > 0) {
					N64_Blocks.NoOfRDRamBlocks[(count >> 12)] = 0;		
					memset(JumpTable + ((count & 0x00FFFFF0) >> 2),0,0x1000);
					*(DelaySlotTable + count) = NULL;
					if (VirtualProtect(m_RDRAM + count, 4, PAGE_READWRITE, &OldProtect) == 0) {
#ifndef EXTERNAL_RELEASE
						DisplayError("Failed to unprotect %X\n1", count);
#endif
					}
				}
			}			
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04000000 && End < 0x04001000) {
			BreakPoint(__FILE__,__LINE__);
			N64_Blocks.NoOfDMEMBlocks = 0;
			memset(JumpTable + (0x04000000 >> 2),0,0x1000);
			*(DelaySlotTable + (0x04000000 >> 12)) = NULL;
			if (VirtualProtect(m_RDRAM + 0x04000000, 4, PAGE_READWRITE, &OldProtect) == 0) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Failed to unprotect %X\n7", 0x04000000);
#endif
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04001000 && End < 0x04002000) {
			BreakPoint(__FILE__,__LINE__);
			N64_Blocks.NoOfIMEMBlocks = 0;
			memset(JumpTable + (0x04001000 >> 2),0,0x1000);
			*(DelaySlotTable + (0x04001000 >> 12)) = NULL;
			if (VirtualProtect(m_RDRAM + 0x04001000, 4, PAGE_READWRITE, &OldProtect) == 0) {
#ifndef EXTERNAL_RELEASE
				DisplayError("Failed to unprotect %X\n6", 0x04001000);
#endif
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}
#ifndef EXTERNAL_RELEASE
		DisplayError("hmmm.... where does this dma End ?\nstart: %X\nend:%X\nlocation %X", 
			Start,End,lpEP->ContextRecord->Eip);
#endif
#endif
		return EXCEPTION_CONTINUE_SEARCH;
	}

	BYTE * ReadPos;
	if (*TypePos == 0x0F && *(TypePos + 1) == 0xB6) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xB7) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBE) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBF) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x66) {
		ReadPos = TypePos + 2;
	} else {
		ReadPos = TypePos + 1;
	}

	switch ((*ReadPos & 0x38)) {
	case 0x00: Reg = &lpEP->ContextRecord->Eax; break;
	case 0x08: Reg = &lpEP->ContextRecord->Ecx; break; 
	case 0x10: Reg = &lpEP->ContextRecord->Edx; break; 
	case 0x18: Reg = &lpEP->ContextRecord->Ebx; break; 
	case 0x20: Reg = &lpEP->ContextRecord->Esp; break;
	case 0x28: Reg = &lpEP->ContextRecord->Ebp; break;
	case 0x30: Reg = &lpEP->ContextRecord->Esi; break;
	case 0x38: Reg = &lpEP->ContextRecord->Edi; break;
	}

	switch ((*ReadPos & 0xC7)) {
	case 0: ReadPos += 1; break;
	case 1: ReadPos += 1; break;
	case 2: ReadPos += 1; break;
	case 3: ReadPos += 1; break;
	case 4: 
		ReadPos += 1; 
		switch ((*ReadPos & 0xC7)) {
		case 0: ReadPos += 1; break;
		case 1: ReadPos += 1; break;
		case 2: ReadPos += 1; break;
		case 3: ReadPos += 1; break;
		case 6: ReadPos += 1; break;
		case 7: ReadPos += 1; break;
		default:
			BreakPoint(__FILE__,__LINE__);
		}
		break;
	case 5: ReadPos += 5; break;
	case 6: ReadPos += 1; break;
	case 7: ReadPos += 1; break;
	case 0x40: ReadPos += 2; break;
	case 0x41: ReadPos += 2; break;
	case 0x42: ReadPos += 2; break;
	case 0x43: ReadPos += 2; break;
	case 0x46: ReadPos += 2; break;
	case 0x47: ReadPos += 2; break;
	case 0x80: ReadPos += 5; break;
	case 0x81: ReadPos += 5; break;
	case 0x82: ReadPos += 5; break;
	case 0x83: ReadPos += 5; break;
	case 0x86: ReadPos += 5; break;
	case 0x87: ReadPos += 5; break;
	default:
		DisplayError("Unknown x86 opcode %X\nlocation %X\nloc: %X\nfgh2", 
			*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	switch(*TypePos) {
	case 0x0F:
		switch(*(TypePos + 1)) {
		case 0xB6:
			if (!LB_NonMemory(MemAddress,(DWORD *)Reg,FALSE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xB7:
			if (!LH_NonMemory(MemAddress,(DWORD *)Reg,FALSE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to load half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xBE:
			if (!LB_NonMemory(MemAddress,Reg,TRUE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xBF:
			if (!LH_NonMemory(MemAddress,Reg,TRUE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to load half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		default:
			DisplayError("Unkown x86 opcode %X\nlocation %X\nloc: %X\nfhfgh2", 
				*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x66:
		switch(*(TypePos + 1)) {
		case 0x8B:
			if (!LH_NonMemory(MemAddress,Reg,FALSE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0x89:
			if (!SH_NonMemory(MemAddress,*(WORD *)Reg)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to store half word\n\nMIPS Address: %X\nX86 Address",MemAddress,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xC7:
			if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
			if (!SH_NonMemory(MemAddress,*(WORD *)ReadPos)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					DisplayError("Failed to store half word\n\nMIPS Address: %X\nX86 Address",MemAddress,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 2);
			return EXCEPTION_CONTINUE_EXECUTION;		
		default:
			DisplayError("Unkown x86 opcode %X\nlocation %X\nloc: %X\nfhfgh2", 
				*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x88: 
		if (!SB_NonMemory(MemAddress,*(BYTE *)Reg)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				DisplayError("Failed to store byte\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x8A: 
		if (!LB_NonMemory(MemAddress,Reg,FALSE)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x8B: 
		if (!LW_NonMemory(MemAddress,Reg)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				DisplayError("Failed to load word\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x89:
		if (!SW_NonMemory(MemAddress,*(DWORD *)Reg)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				DisplayError("Failed to store word\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC6:
		if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!SB_NonMemory(MemAddress,*(BYTE *)ReadPos)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				DisplayError("Failed to store byte\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 1);
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC7:
		if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!SW_NonMemory(MemAddress,*(DWORD *)ReadPos)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				DisplayError("Failed to store word\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 4);
		return EXCEPTION_CONTINUE_EXECUTION;		
	default:
		DisplayError("Unkown x86 opcode %X\nlocation %X\nloc: %X\nfhfgh2", 
			*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, (char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM);
		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

int CMipsMemoryVM::LB_NonMemory ( DWORD PAddr, DWORD * Value, BOOL SignExtend ) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if (WrittenToRom) { return FALSE; }
		if ((PAddr & 2) == 0) { PAddr = (PAddr + 4) ^ 2; }
		if ((PAddr - 0x10000000) < RomFileSize) {
			if (SignExtend) {
				*Value = (int)((char)ROM[PAddr - 0x10000000]);
			} else {
				*Value = ROM[PAddr - 0x10000000];
			}
			return TRUE;
		} else {
			*Value = 0;
			return FALSE;
		}
	}
#endif
//	switch (PAddr & 0xFFF00000) {
//	default:
		* Value = 0;
		return FALSE;
//		break;
//	}
//	return TRUE;
}

int CMipsMemoryVM::LH_NonMemory ( DWORD PAddr, DWORD * Value, int SignExtend ) {
//	switch (PAddr & 0xFFF00000) {
//	default:
		* Value = 0;
		return FALSE;
//		break;
//	}
//	return TRUE;
}

int CMipsMemoryVM::LW_NonMemory ( DWORD PAddr, DWORD * Value ) {
#ifdef CFB_READ
	if (PAddr >= CFBStart && PAddr < CFBEnd) {
		DWORD OldProtect;
		VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READONLY, &OldProtect);
		if (FrameBufferRead) { FrameBufferRead(PAddr & ~0xFFF); }
		*Value = *(DWORD *)(m_RDRAM+PAddr);
		return TRUE;
	}	
#endif
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) 
	{
		if (m_RomWrittenTo) 
		{ 
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			*Value = WroteToRom;
			//LogMessage("%X: Read crap from Rom %X from %X",PROGRAM_COUNTER,*Value,PAddr);
			WrittenToRom = FALSE;
#ifdef ROM_IN_MAPSPACE
			{
				DWORD OldProtect;
				VirtualProtect(ROM,RomFileSize,PAGE_READONLY, &OldProtect);
			}
#endif
#endif
			return TRUE;
		}
		if ((PAddr - 0x10000000) < m_RomSize) {
			*Value = *(DWORD *)&m_Rom[PAddr - 0x10000000];
			return TRUE;
		} else {
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return FALSE;
		}
	}

	switch (PAddr & 0xFFF00000) {
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: * Value = _Reg->RDRAM_CONFIG_REG; break;
		case 0x03F00004: * Value = _Reg->RDRAM_DEVICE_ID_REG; break;
		case 0x03F00008: * Value = _Reg->RDRAM_DELAY_REG; break;
		case 0x03F0000C: * Value = _Reg->RDRAM_MODE_REG; break;
		case 0x03F00010: * Value = _Reg->RDRAM_REF_INTERVAL_REG; break;
		case 0x03F00014: * Value = _Reg->RDRAM_REF_ROW_REG; break;
		case 0x03F00018: * Value = _Reg->RDRAM_RAS_INTERVAL_REG; break;
		case 0x03F0001C: * Value = _Reg->RDRAM_MIN_INTERVAL_REG; break;
		case 0x03F00020: * Value = _Reg->RDRAM_ADDR_SELECT_REG; break;
		case 0x03F00024: * Value = _Reg->RDRAM_DEVICE_MANUF_REG; break;	
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04000000:
		switch (PAddr) {
		case 0x04040010: *Value = _Reg->SP_STATUS_REG; break;
		case 0x04040014: *Value = _Reg->SP_DMA_FULL_REG; break;
		case 0x04040018: *Value = _Reg->SP_DMA_BUSY_REG; break;
		case 0x04080000: *Value = _Reg->SP_PC_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x0410000C: *Value = _Reg->DPC_STATUS_REG; break;
		case 0x04100010: *Value = _Reg->DPC_CLOCK_REG; break;
		case 0x04100014: *Value = _Reg->DPC_BUFBUSY_REG; break;
		case 0x04100018: *Value = _Reg->DPC_PIPEBUSY_REG; break;
		case 0x0410001C: *Value = _Reg->DPC_TMEM_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: * Value = _Reg->MI_MODE_REG; break;
		case 0x04300004: * Value = _Reg->MI_VERSION_REG; break;
		case 0x04300008: * Value = _Reg->MI_INTR_REG; break;
		case 0x0430000C: * Value = _Reg->MI_INTR_MASK_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04400000:
		switch (PAddr) {
		case 0x04400000: *Value = _Reg->VI_STATUS_REG; break;
		case 0x04400004: *Value = _Reg->VI_ORIGIN_REG; break;
		case 0x04400008: *Value = _Reg->VI_WIDTH_REG; break;
		case 0x0440000C: *Value = _Reg->VI_INTR_REG; break;
		case 0x04400010: 
			UpdateHalfLine();
			*Value = m_HalfLine; 
			break;
		case 0x04400014: *Value = _Reg->VI_BURST_REG; break;
		case 0x04400018: *Value = _Reg->VI_V_SYNC_REG; break;
		case 0x0440001C: *Value = _Reg->VI_H_SYNC_REG; break;
		case 0x04400020: *Value = _Reg->VI_LEAP_REG; break;
		case 0x04400024: *Value = _Reg->VI_H_START_REG; break;
		case 0x04400028: *Value = _Reg->VI_V_START_REG ; break;
		case 0x0440002C: *Value = _Reg->VI_V_BURST_REG; break;
		case 0x04400030: *Value = _Reg->VI_X_SCALE_REG; break;
		case 0x04400034: *Value = _Reg->VI_Y_SCALE_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04500000:
		switch (PAddr) {
		case 0x04500004: 
#ifdef tofix
			if (_Settings->LoadBool(Game_FixedAudio))
			{
				*Value = CAudio::AiGetLength(g_Audio);
			} else {
#endif
				if (AiReadLength != NULL) {
					*Value = AiReadLength(); 
				} else {
					*Value = 0;
				}
#ifdef tofix
			}
#endif
			break;
		case 0x0450000C: 
#ifdef tofix
			if (_Settings->LoadBool(Game_FixedAudio))
			{
				*Value = CAudio::AiGetStatus(g_Audio);
			} else {
#endif
				*Value = _Reg->AI_STATUS_REG; 
#ifdef tofix
			}
#endif
			break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600010: *Value = _Reg->PI_STATUS_REG; break;
		case 0x04600014: *Value = _Reg->PI_DOMAIN1_REG; break;
		case 0x04600018: *Value = _Reg->PI_BSD_DOM1_PWD_REG; break;
		case 0x0460001C: *Value = _Reg->PI_BSD_DOM1_PGS_REG; break;
		case 0x04600020: *Value = _Reg->PI_BSD_DOM1_RLS_REG; break;
		case 0x04600024: *Value = _Reg->PI_DOMAIN2_REG; break;
		case 0x04600028: *Value = _Reg->PI_BSD_DOM2_PWD_REG; break;
		case 0x0460002C: *Value = _Reg->PI_BSD_DOM2_PGS_REG; break;
		case 0x04600030: *Value = _Reg->PI_BSD_DOM2_RLS_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: * Value = _Reg->RI_MODE_REG; break;
		case 0x04700004: * Value = _Reg->RI_CONFIG_REG; break;
		case 0x04700008: * Value = _Reg->RI_CURRENT_LOAD_REG; break;
		case 0x0470000C: * Value = _Reg->RI_SELECT_REG; break;
		case 0x04700010: * Value = _Reg->RI_REFRESH_REG; break;
		case 0x04700014: * Value = _Reg->RI_LATENCY_REG; break;
		case 0x04700018: * Value = _Reg->RI_RERROR_REG; break;
		case 0x0470001C: * Value = _Reg->RI_WERROR_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: *Value = _Reg->SI_STATUS_REG; break;
		default:
			*Value = 0;
			return FALSE;
		}
		break;
	case 0x05000000:
		*Value = PAddr & 0xFFFF;
		*Value = (*Value << 16) | *Value;
		return FALSE;
	case 0x08000000:
		if (g_SaveUsing == SaveChip_Auto) { g_SaveUsing = SaveChip_FlashRam; }
		if (g_SaveUsing != SaveChip_FlashRam) { 
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return FALSE;
		}
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		*Value = ReadFromFlashStatus(PAddr);
#endif
		break;
	case 0x1FC00000:
		if (PAddr < 0x1FC007C0) {
/*			DWORD ToSwap = *(DWORD *)(&PifRom[PAddr - 0x1FC00000]);
			_asm {
				mov eax,ToSwap
				bswap eax
				mov ToSwap,eax
			}
			* Value = ToSwap;*/
			BreakPoint(__FILE__,__LINE__);
			return TRUE;
		}
		else if (PAddr < 0x1FC00800) 
		{
			BYTE * PIF_Ram = _MMU->PifRam();
			DWORD ToSwap = *(DWORD *)(&PIF_Ram[PAddr - 0x1FC007C0]);
			_asm {
				mov eax,ToSwap
				bswap eax
				mov ToSwap,eax
			}
			* Value = ToSwap;
			return TRUE;
		} else {
			* Value = 0;
			return FALSE;
		}
		BreakPoint(__FILE__,__LINE__);
		break;
	default:
		*Value = PAddr & 0xFFFF;
		*Value = (*Value << 16) | *Value;
		return FALSE;
		break;
	}
	return TRUE;
}

int CMipsMemoryVM::SB_NonMemory ( DWORD PAddr, BYTE Value ) {
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
#ifdef CFB_READ
		if (PAddr >= CFBStart && PAddr < CFBEnd) {
			DWORD OldProtect;
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(BYTE *)(m_RDRAM+PAddr) = Value;
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,OldProtect, &OldProtect);
			DisplayError("FrameBufferWrite");
			if (FrameBufferWrite) { FrameBufferWrite(PAddr,1); }
			break;
		}	
#endif
		if (PAddr < RdramSize()) {
			bool ProtectMem = false;
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((m_RDRAM + PAddr), 1, PAGE_READWRITE, &OldProtect);
			*(BYTE *)(m_RDRAM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((m_RDRAM + PAddr), 1, PAGE_READONLY, &OldProtect);
			}
		}
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

int CMipsMemoryVM::SH_NonMemory ( DWORD PAddr, WORD Value ) {
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
#ifdef CFB_READ
		if (PAddr >= CFBStart && PAddr < CFBEnd) {
			DWORD OldProtect;
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(WORD *)(m_RDRAM+PAddr) = Value;
			if (FrameBufferWrite) { FrameBufferWrite(PAddr & ~0xFFF,2); }
			//*(WORD *)(m_RDRAM+PAddr) = 0xFFFF;
			//VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_NOACCESS, &OldProtect);
			DisplayError("PAddr = %x",PAddr);
			break;
		}	
#endif
		if (PAddr < RdramSize()) {
			bool ProtectMem = false;
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((m_RDRAM + PAddr), 1, PAGE_READWRITE, &OldProtect);
			*(WORD *)(m_RDRAM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((m_RDRAM + PAddr), 1, PAGE_READONLY, &OldProtect);
			}
		}
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

int CMipsMemoryVM::SW_NonMemory ( DWORD PAddr, DWORD Value ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if ((PAddr - 0x10000000) < RomFileSize) {
			WrittenToRom = TRUE;
			WroteToRom = Value;
#ifdef ROM_IN_MAPSPACE
			{
				DWORD OldProtect;
				VirtualProtect(ROM,RomFileSize,PAGE_NOACCESS, &OldProtect);
			}
#endif
			//LogMessage("%X: Wrote To Rom %X from %X",PROGRAM_COUNTER,Value,PAddr);
		} else {
			return FALSE;
		}
#endif
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
#ifdef CFB_READ
		if (PAddr >= CFBStart && PAddr < CFBEnd) {
			DWORD OldProtect;
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(DWORD *)(m_RDRAM+PAddr) = Value;
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,OldProtect, &OldProtect);
			DisplayError("FrameBufferWrite %X",PAddr);
			if (FrameBufferWrite) { FrameBufferWrite(PAddr,4); }
			break;
		}	
#endif
		if (PAddr < RdramSize()) {
			bool ProtectMem = false;
			
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((m_RDRAM + PAddr), 4, PAGE_READWRITE, &OldProtect);
			*(DWORD *)(m_RDRAM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((m_RDRAM + PAddr), 4, PAGE_READONLY, &OldProtect);
			}
		}
		break;
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: _Reg->RDRAM_CONFIG_REG = Value; break;
		case 0x03F00004: _Reg->RDRAM_DEVICE_ID_REG = Value; break;
		case 0x03F00008: _Reg->RDRAM_DELAY_REG = Value; break;
		case 0x03F0000C: _Reg->RDRAM_MODE_REG = Value; break;
		case 0x03F00010: _Reg->RDRAM_REF_INTERVAL_REG = Value; break;
		case 0x03F00014: _Reg->RDRAM_REF_ROW_REG = Value; break;
		case 0x03F00018: _Reg->RDRAM_RAS_INTERVAL_REG = Value; break;
		case 0x03F0001C: _Reg->RDRAM_MIN_INTERVAL_REG = Value; break;
		case 0x03F00020: _Reg->RDRAM_ADDR_SELECT_REG = Value; break;
		case 0x03F00024: _Reg->RDRAM_DEVICE_MANUF_REG = Value; break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			return FALSE;
		}
		break;
	case 0x04000000: 
		if (PAddr < 0x04002000) {
			bool ProtectMem = false;
			
			DWORD Start = PAddr & ~0xFFF;
			WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",PAddr,1);
			if (!ClearRecompCodeProtectMem(Start,0xFFF)) { ProtectMem = true; }

			DWORD OldProtect;
			VirtualProtect((m_RDRAM + PAddr), 4, PAGE_READWRITE, &OldProtect);
			*(DWORD *)(m_RDRAM+PAddr) = Value;
			if (ProtectMem)
			{
				VirtualProtect((m_RDRAM + PAddr), 4, PAGE_READONLY, &OldProtect);
			}
		} else {
			switch (PAddr) {
			case 0x04040000: _Reg->SP_MEM_ADDR_REG = Value; break;
			case 0x04040004: _Reg->SP_DRAM_ADDR_REG = Value; break;
			case 0x04040008: 
				_Reg->SP_RD_LEN_REG = Value; 
				SP_DMA_READ();
				break;
			case 0x0404000C: 
				_Reg->SP_WR_LEN_REG = Value; 
				SP_DMA_WRITE();
				break;
			case 0x04040010: 
				if ( ( Value & SP_CLR_HALT ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_HALT; }
				if ( ( Value & SP_SET_HALT ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_HALT;  }
				if ( ( Value & SP_CLR_BROKE ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_BROKE; }
				if ( ( Value & SP_CLR_INTR ) != 0) { 
					_Reg->MI_INTR_REG &= ~MI_INTR_SP; 
					CheckInterrupts();
				}
	#ifndef EXTERNAL_RELEASE
				if ( ( Value & SP_SET_INTR ) != 0) { DisplayError("SP_SET_INTR"); }
	#endif
				if ( ( Value & SP_CLR_SSTEP ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
				if ( ( Value & SP_SET_SSTEP ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SSTEP;  }
				if ( ( Value & SP_CLR_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
				if ( ( Value & SP_SET_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
				if ( ( Value & SP_CLR_SIG0 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG0; }
				if ( ( Value & SP_SET_SIG0 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG0;  }
				if ( ( Value & SP_CLR_SIG1 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG1; }
				if ( ( Value & SP_SET_SIG1 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG1;  }
				if ( ( Value & SP_CLR_SIG2 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG2; }
				if ( ( Value & SP_SET_SIG2 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG2;  }
				if ( ( Value & SP_CLR_SIG3 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG3; }
				if ( ( Value & SP_SET_SIG3 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG3;  }
				if ( ( Value & SP_CLR_SIG4 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG4; }
				if ( ( Value & SP_SET_SIG4 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG4;  }
				if ( ( Value & SP_CLR_SIG5 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG5; }
				if ( ( Value & SP_SET_SIG5 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG5;  }
				if ( ( Value & SP_CLR_SIG6 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG6; }
				if ( ( Value & SP_SET_SIG6 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG6;  }
				if ( ( Value & SP_CLR_SIG7 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG7; }
				if ( ( Value & SP_SET_SIG7 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG7;  }
				
#ifdef tofix
				if ( ( Value & SP_SET_SIG0 ) != 0 && AudioSignal) 
				{ 
					MI_INTR_REG |= MI_INTR_SP; 
					CheckInterrupts();				
				}
#endif
				//if (*( DWORD *)(DMEM + 0xFC0) == 1) {
				//	ChangeTimer(RspTimer,0x30000);
				//} else {
					RunRsp();
				//}
				break;
			case 0x0404001C: _Reg->SP_SEMAPHORE_REG = 0; break;
			case 0x04080000: _Reg->SP_PC_REG = Value & 0xFFC; break;
			default:
				return FALSE;
			}
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x04100000: 
			_Reg->DPC_START_REG = Value; 
			_Reg->DPC_CURRENT_REG = Value; 
			break;
		case 0x04100004: 
			_Reg->DPC_END_REG = Value; 
			if (ProcessRDPList) { ProcessRDPList(); }
			break;
		//case 0x04100008: _Reg->DPC_CURRENT_REG = Value; break;
		case 0x0410000C:
			if ( ( Value & DPC_CLR_XBUS_DMEM_DMA ) != 0) { _Reg->DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA; }
			if ( ( Value & DPC_SET_XBUS_DMEM_DMA ) != 0) { _Reg->DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) { _Reg->DPC_STATUS_REG &= ~DPC_STATUS_FREEZE; }
			if ( ( Value & DPC_SET_FREEZE ) != 0) { _Reg->DPC_STATUS_REG |= DPC_STATUS_FREEZE;  }		
			if ( ( Value & DPC_CLR_FLUSH ) != 0) { _Reg->DPC_STATUS_REG &= ~DPC_STATUS_FLUSH; }
			if ( ( Value & DPC_SET_FLUSH ) != 0) { _Reg->DPC_STATUS_REG |= DPC_STATUS_FLUSH;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) 
			{
				if ( ( _Reg->SP_STATUS_REG & SP_STATUS_HALT ) == 0) 
				{
					if ( ( _Reg->SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) 
					{
						RunRsp();
					}
				}
			}
#ifdef tofix
			if (ShowUnhandledMemory) {
				//if ( ( Value & DPC_CLR_TMEM_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_TMEM_CTR"); }
				//if ( ( Value & DPC_CLR_PIPE_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR"); }
				//if ( ( Value & DPC_CLR_CMD_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR"); }
				//if ( ( Value & DPC_CLR_CLOCK_CTR ) != 0) { DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR"); }
			}
#endif
			break;
		default:
			return FALSE;
		}
		break;
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			_Reg->MI_MODE_REG &= ~0x7F;
			_Reg->MI_MODE_REG |= (Value & 0x7F);
			if ( ( Value & MI_CLR_INIT ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_INIT; }
			if ( ( Value & MI_SET_INIT ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_INIT; }
			if ( ( Value & MI_CLR_EBUS ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_EBUS; }
			if ( ( Value & MI_SET_EBUS ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_EBUS; }
			if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
				_Reg->MI_INTR_REG &= ~MI_INTR_DP; 
				CheckInterrupts();
			}
			if ( ( Value & MI_CLR_RDRAM ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_RDRAM; }
			if ( ( Value & MI_SET_RDRAM ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_RDRAM; }
			break;
		case 0x0430000C: 
			if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
			if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
			break;
		default:
			return FALSE;
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (_Reg->VI_STATUS_REG != Value) { 
				_Reg->VI_STATUS_REG = Value; 
				if (ViStatusChanged != NULL ) { ViStatusChanged(); }
			}
			break;
		case 0x04400004: 
#ifdef CFB_READ
			if (_Reg->VI_ORIGIN_REG > 0x280) {
				SetFrameBuffer(_Reg->VI_ORIGIN_REG, (DWORD)(VI_WIDTH_REG * (VI_WIDTH_REG *.75)));
			}
#endif
			_Reg->VI_ORIGIN_REG = (Value & 0xFFFFFF); 
			//if (UpdateScreen != NULL ) { UpdateScreen(); }
			break;
		case 0x04400008: 
			if (_Reg->VI_WIDTH_REG != Value) {
				_Reg->VI_WIDTH_REG = Value; 
				if (ViWidthChanged != NULL ) { ViWidthChanged(); }
			}
			break;
		case 0x0440000C: _Reg->VI_INTR_REG = Value; break;
		case 0x04400010: 
			_Reg->MI_INTR_REG &= ~MI_INTR_VI;
			CheckInterrupts();
			break;
		case 0x04400014: _Reg->VI_BURST_REG = Value; break;
		case 0x04400018: _Reg->VI_V_SYNC_REG = Value; break;
		case 0x0440001C: _Reg->VI_H_SYNC_REG = Value; break;
		case 0x04400020: _Reg->VI_LEAP_REG = Value; break;
		case 0x04400024: _Reg->VI_H_START_REG = Value; break;
		case 0x04400028: _Reg->VI_V_START_REG = Value; break;
		case 0x0440002C: _Reg->VI_V_BURST_REG = Value; break;
		case 0x04400030: _Reg->VI_X_SCALE_REG = Value; break;
		case 0x04400034: _Reg->VI_Y_SCALE_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04500000: 
		switch (PAddr) {
		case 0x04500000: _Reg->AI_DRAM_ADDR_REG = Value; break;
		case 0x04500004: 
			_Reg->AI_LEN_REG = Value; 
			if (g_FixedAudio)
			{
				_Audio->AiSetLength();
			}
			if (AiLenChanged != NULL) { AiLenChanged(); }				
			break;
		case 0x04500008: _Reg->AI_CONTROL_REG = (Value & 1); break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			_Reg->MI_INTR_REG &= ~MI_INTR_AI;
			_Reg->m_AudioIntrReg &= ~MI_INTR_AI;
			CheckInterrupts();
			break;
		case 0x04500010: 
			_Reg->AI_DACRATE_REG = Value;  
			DacrateChanged(g_SystemType);
			if (_Settings->LoadBool(Game_FixedAudio))
			{
#ifdef tofix
				g_Audio->AiSetFrequency(Value,g_SystemType);
#endif
			}
			break;
		case 0x04500014:  _Reg->AI_BITRATE_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04600000: 
		switch (PAddr) {
		case 0x04600000: _Reg->PI_DRAM_ADDR_REG = Value; break;
		case 0x04600004: _Reg->PI_CART_ADDR_REG = Value; break;
		case 0x04600008: 
			_Reg->PI_RD_LEN_REG = Value; 
			PI_DMA_READ();
			break;
		case 0x0460000C: 
			_Reg->PI_WR_LEN_REG = Value; 
			PI_DMA_WRITE();
			break;
		case 0x04600010:
			//if ((Value & PI_SET_RESET) != 0 ) { DisplayError("reset Controller"); }
			if ((Value & PI_CLR_INTR) != 0 ) {
				_Reg->MI_INTR_REG &= ~MI_INTR_PI;
				CheckInterrupts();
			}
			break;
		case 0x04600014: _Reg->PI_DOMAIN1_REG = (Value & 0xFF); break; 
		case 0x04600018: _Reg->PI_BSD_DOM1_PWD_REG = (Value & 0xFF); break; 
		case 0x0460001C: _Reg->PI_BSD_DOM1_PGS_REG = (Value & 0xFF); break; 
		case 0x04600020: _Reg->PI_BSD_DOM1_RLS_REG = (Value & 0xFF); break; 
		default:
			return FALSE;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: _Reg->RI_MODE_REG = Value; break;
		case 0x04700004: _Reg->RI_CONFIG_REG = Value; break;
		case 0x04700008: _Reg->RI_CURRENT_LOAD_REG = Value; break;
		case 0x0470000C: _Reg->RI_SELECT_REG = Value; break;
		case 0x04700010: _Reg->RI_REFRESH_REG = Value; break;
		case 0x04700014: _Reg->RI_LATENCY_REG = Value; break;
		case 0x04700018: _Reg->RI_RERROR_REG = Value; break;
		case 0x0470001C: _Reg->RI_WERROR_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: _Reg->SI_DRAM_ADDR_REG = Value; break;
		case 0x04800004: 
			_Reg->SI_PIF_ADDR_RD64B_REG = Value; 
			SI_DMA_READ ();
			break;
		case 0x04800010: 
			_Reg->SI_PIF_ADDR_WR64B_REG = Value; 
			SI_DMA_WRITE();
			break;
		case 0x04800018: 
			_Reg->MI_INTR_REG &= ~MI_INTR_SI; 
			_Reg->SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
			CheckInterrupts();
			break;
		default:
			return FALSE;
		}
		break;
	case 0x08000000:
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if (PAddr != 0x08010000) { return FALSE; }
		if (SaveUsing == SaveChip_Auto) { SaveUsing = SaveChip_FlashRam; }
		if (SaveUsing != SaveChip_FlashRam) { return TRUE; }
		WriteToFlashCommand(Value);
#endif
		return FALSE;
		break;
	case 0x1FC00000:
		if (PAddr < 0x1FC007C0) {
			return FALSE;
		} else if (PAddr < 0x1FC00800) {
			_asm {
				mov eax,Value
				bswap eax
				mov Value,eax
			}
			*(DWORD *)(&m_PIF_Ram[PAddr - 0x1FC007C0]) = Value;
			if (PAddr == 0x1FC007FC) {
				PifRamWrite();
			}
			return TRUE;
		}
		return FALSE;
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

extern DWORD g_ViRefreshRate;

void CMipsMemoryVM::UpdateHalfLine (void)
{
    if (*_NextTimer < 0) { 
		m_HalfLine = 0;
		return;
	}
	m_HalfLine = (DWORD)(*_NextTimer / g_ViRefreshRate);
	m_HalfLine &= ~1;
}

#ifdef toremove

bool CMipsMemoryVM::Load64 ( DWORD VAddr, QWORD & Variable, MemorySize Size, bool SignExtend ) {
	__try {
		void * MemLoc;
		
		DWORD PAddr;
		if (!TranslateVaddr(VAddr,PAddr)) {
			return false;
		}
		if (PAddr > _Settings->LoadDword(Game_RDRamSize) && 
			(PAddr < 0x04000000 || PAddr > 0x04002000))
		{
			switch (Size) {
			case _8Bit: 
				if (!LoadByte_NonMemory(PAddr,reinterpret_cast<BYTE *>(&Variable))) {
					MemoryFilterFailed("load byte",PAddr,PROGRAM_COUNTER,0);
				}
				Variable = SignExtend?(__int64)((char)Variable):(BYTE)Variable;
				return true;
			case _16Bit: 
				if (!LoadHalf_NonMemory(PAddr,reinterpret_cast<WORD *>(&Variable))) {
					MemoryFilterFailed("load half",PAddr,PROGRAM_COUNTER,0);
				}
				Variable = SignExtend?(__int64)((short)Variable):(WORD)Variable;
				return true;
			case _32Bit: 
				if (!LoadWord_NonMemory(PAddr,reinterpret_cast<DWORD *>(&Variable))) {
					MemoryFilterFailed("load word",PAddr,PROGRAM_COUNTER,0);
				}
				Variable = SignExtend?(__int64)((int)Variable):(DWORD)Variable;
				return true;
			default:
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			return false;
		}

		switch (Size) {
		case _8Bit:
			if (!VAddrToRealAddr((VAddr ^ 3),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			Variable = SignExtend?(__int64)(*(char *)MemLoc):*(BYTE *)MemLoc;
			return true;
			break;
		case _16Bit:
			if ((VAddr & 1) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr((VAddr ^ 2),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			Variable = SignExtend?(__int64)(*(short *)MemLoc):*(WORD *)MemLoc;
			return true;
			break;
		case _32Bit:
			if ((VAddr & 3) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			Variable = SignExtend?(__int64)(*(int *)MemLoc):*(DWORD *)MemLoc;
			return true;
			break;
		case _64Bit:
			if ((VAddr & 7) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			Variable = (((__int64)(*(DWORD *)MemLoc)) << 32) | (*(((DWORD *)MemLoc) + 1));
			return true;
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	} __except( SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		_Notify->FatalError("Unknown memory action\n\nEmulation stop");
	}
	return false;
}

/* Function handles when memory is accessed that is not RDRAM/IMEM/DMEM
   This includes things like rom, registers, and memory that has not been defined.
*/
bool CMipsMemoryVM::LoadByte_NonMemory ( DWORD PAddr, BYTE * Value ) {
	*Value = 0;
	return false;
}

bool CMipsMemoryVM::LoadHalf_NonMemory ( DWORD PAddr, WORD * Value ) {
	*Value = 0;
	return false;
}

bool CMipsMemoryVM::LoadWord_NonMemory ( DWORD PAddr, DWORD * Value ) {
	//TlbLog.Log("LW %X",PAddr);
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if ((PAddr - 0x10000000) < RomFileSize()) {
			if (m_WrittenToRom) { 
				*Value = m_WroteToRom;
				m_WrittenToRom = false;
				return true;
			}
			*Value = *(DWORD *)&ROM[PAddr - 0x10000000];
			return TRUE;
		} else {
			*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
			return false;
		}
	}

	switch (PAddr & 0xFFF00000) {
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: * Value = _Reg->RDRAM_CONFIG_REG; break;
		case 0x03F00004: * Value = _Reg->RDRAM_DEVICE_ID_REG; break;
		case 0x03F00008: * Value = _Reg->RDRAM_DELAY_REG; break;
		case 0x03F0000C: * Value = _Reg->RDRAM_MODE_REG; break;
		case 0x03F00010: * Value = _Reg->RDRAM_REF_INTERVAL_REG; break;
		case 0x03F00014: * Value = _Reg->RDRAM_REF_ROW_REG; break;
		case 0x03F00018: * Value = _Reg->RDRAM_RAS_INTERVAL_REG; break;
		case 0x03F0001C: * Value = _Reg->RDRAM_MIN_INTERVAL_REG; break;
		case 0x03F00020: * Value = _Reg->RDRAM_ADDR_SELECT_REG; break;
		case 0x03F00024: * Value = _Reg->RDRAM_DEVICE_MANUF_REG; break;	
		default:
			*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
			return false;
		}
		break;
	case 0x04000000:
		switch (PAddr) {
		case 0x04040010: * Value = _Reg->SP_STATUS_REG; break;
		case 0x04040014: * Value = _Reg->SP_DMA_FULL_REG; break;
		case 0x04040018: * Value = _Reg->SP_DMA_BUSY_REG; break;
		case 0x04080000: * Value = _Reg->SP_PC_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x0410000C: *Value = _Reg->DPC_STATUS_REG; break;
		case 0x04100010: *Value = _Reg->DPC_CLOCK_REG; break;
		case 0x04100014: *Value = _Reg->DPC_BUFBUSY_REG; break;
		case 0x04100018: *Value = _Reg->DPC_PIPEBUSY_REG; break;
		case 0x0410001C: *Value = _Reg->DPC_TMEM_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: * Value = MI_MODE_REG; break;
		case 0x04300004: * Value = MI_VERSION_REG; break;
		case 0x04300008: * Value = MI_INTR_REG; break;
		case 0x0430000C: * Value = MI_INTR_MASK_REG; break;
		default:
			*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
			return false;
		}
		break;
	case 0x04400000:
		switch (PAddr) {
		case 0x04400000: *Value = VI_STATUS_REG; break;
		case 0x04400004: *Value = VI_ORIGIN_REG; break;
		case 0x04400008: *Value = VI_WIDTH_REG; break;
		case 0x0440000C: *Value = VI_INTR_REG; break;
		case 0x04400010: 
			UpdateHalfLine();
			*Value = m_HalfLine; 
			break;
		case 0x04400014: *Value = VI_BURST_REG; break;
		case 0x04400018: *Value = VI_V_SYNC_REG; break;
		case 0x0440001C: *Value = VI_H_SYNC_REG; break;
		case 0x04400020: *Value = VI_LEAP_REG; break;
		case 0x04400024: *Value = VI_H_START_REG; break;
		case 0x04400028: *Value = VI_V_START_REG ; break;
		case 0x0440002C: *Value = VI_V_BURST_REG; break;
		case 0x04400030: *Value = VI_X_SCALE_REG; break;
		case 0x04400034: *Value = VI_Y_SCALE_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04500000:
		switch (PAddr) {
		case 0x04500004: 
			*Value = _Reg->GetTimer(AiTimer); 
			if (*Value != 0) {
				*Value = (*Value / _System->_Plugins->Audio()->CountsPerByte()) + 1;
			}
			break;
		case 0x0450000C: 
			*Value = AI_STATUS_REG; 
			break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600010: *Value = PI_STATUS_REG; break;
		case 0x04600014: *Value = PI_DOMAIN1_REG; break;
		case 0x04600018: *Value = PI_BSD_DOM1_PWD_REG; break;
		case 0x0460001C: *Value = PI_BSD_DOM1_PGS_REG; break;
		case 0x04600020: *Value = PI_BSD_DOM1_RLS_REG; break;
		case 0x04600024: *Value = PI_DOMAIN2_REG; break;
		case 0x04600028: *Value = PI_BSD_DOM2_PWD_REG; break;
		case 0x0460002C: *Value = PI_BSD_DOM2_PGS_REG; break;
		case 0x04600030: *Value = PI_BSD_DOM2_RLS_REG; break;
		default:
			*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
			return false;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: * Value = RI_MODE_REG; break;
		case 0x04700004: * Value = RI_CONFIG_REG; break;
		case 0x04700008: * Value = RI_CURRENT_LOAD_REG; break;
		case 0x0470000C: * Value = RI_SELECT_REG; break;
		case 0x04700010: * Value = RI_REFRESH_REG; break;
		case 0x04700014: * Value = RI_LATENCY_REG; break;
		case 0x04700018: * Value = RI_RERROR_REG; break;
		case 0x0470001C: * Value = RI_WERROR_REG; break;
		default:
			*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
			return false;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: *Value = SI_STATUS_REG; break;
		default:
			*Value = 0;
			return FALSE;
		}
		break;
	case 0x08000000:
		if (_Settings->LoadDword(SaveChipType) == SaveChip_Auto) { 
			_Settings->Save(SaveChipType,SaveChip_FlashRam); 
		}
		if (_Settings->LoadDword(SaveChipType) != SaveChip_FlashRam) {
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return FALSE;
		}
		if (m_FlashRam == NULL) { m_FlashRam = new CFlashRam(_Settings,_Notify,m_SavesReadOnly); }
		*Value = m_FlashRam->ReadFromFlashStatus(PAddr);
		break;
	default:
		*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
		return false;
	}
	return true;
}

bool CMipsMemoryVM::Store64 ( DWORD VAddr, QWORD Value, MemorySize Size ) {
	__try {
		DWORD PAddr;
		if (!TranslateVaddr(VAddr,PAddr)) {
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		if (PAddr > _Settings->LoadDword(Game_RDRamSize) && 
			(PAddr < 0x04000000 || PAddr > 0x04002000))
		{			
			switch (Size) {
			case _16Bit: 
				if (!StoreHalf_NonMemory(PAddr,static_cast<WORD>(Value))) {
					MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<WORD>(Value));
				}
				return true;
				break;
			case _32Bit: 
				if (!StoreWord_NonMemory(PAddr,static_cast<DWORD>(Value))) {
					MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<DWORD>(Value));
				}
				return true;
				break;
			default:
				_Notify->BreakPoint(__FILE__,__LINE__);
			}
			return false;
		}

		void * MemLoc;
		switch (Size) {
		case _8Bit: 
			if (!VAddrToRealAddr((VAddr ^ 3),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(BYTE *)MemLoc = (BYTE)Value;
			return true;
			break;
		case _16Bit: 
			if ((VAddr & 1) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr((VAddr ^ 2),MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(WORD *)MemLoc = (WORD)Value;
			return true;
			break;
		case _32Bit: 
			if ((VAddr & 3) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(DWORD *)MemLoc = (DWORD)Value;
			return true;
			break;
		case _64Bit: 
			if ((VAddr & 7) != 0) { /*ADDRESS_ERROR_EXCEPTION(Address,TRUE);*/ _Notify->BreakPoint(__FILE__,__LINE__); }
			if (!VAddrToRealAddr(VAddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*((DWORD *)(MemLoc))     = *((DWORD *)(&Value) + 1);
			*(((DWORD *)MemLoc) + 1) = *((DWORD *)(&Value));
			return true;
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	} __except( SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		_Notify->FatalError("Unknown memory action\n\nEmulation stop");
	}
	return false;
}

/* Function handles when memory is accessed that is not RDRAM/IMEM/DMEM
   This includes things like rom, registers, and memory that has not been defined.
*/
bool CMipsMemoryVM::StoreByte_NonMemory ( DWORD PAddr, BYTE Value ) {	
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < _Settings->LoadDword(Game_RDRamSize)) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0x80000000,1);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((m_RDRAM + PAddr), 1, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(BYTE *)(m_RDRAM+PAddr) = Value;
			if (VirtualProtect((m_RDRAM + PAddr), 1, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
		}
		break;
	default:
		return false;
	}
	return true;
}

bool CMipsMemoryVM::StoreHalf_NonMemory ( DWORD PAddr, WORD Value ) {	
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < _Settings->LoadDword(Game_RDRamSize)) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0x80000000,1);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((m_RDRAM + PAddr), 1, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(WORD *)(m_RDRAM+PAddr) = Value;
			if (VirtualProtect((m_RDRAM + PAddr), 1, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
		}
		break;
	default:
		return false;
	}
	return true;
	return false;
}

bool CMipsMemoryVM::StoreWord_NonMemory ( DWORD PAddr, DWORD Value ) {
//	TlbLog.Log("SW %08X %X",PAddr,(DWORD)Value);
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if ((PAddr - 0x10000000) < RomFileSize()) {
			m_WrittenToRom = true;
			m_WroteToRom   = Value;
			return true;
		}
	}

	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < _Settings->LoadDword(Game_RDRamSize)) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0x80000000,4);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((m_RDRAM + PAddr), 4, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(DWORD *)(m_RDRAM+PAddr) = Value;
			if (VirtualProtect((m_RDRAM + PAddr), 4, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
		}
		break;
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: RDRAM_CONFIG_REG = Value; break;
		case 0x03F00004: RDRAM_DEVICE_ID_REG = Value; break;
		case 0x03F00008: RDRAM_DELAY_REG = Value; break;
		case 0x03F0000C: RDRAM_MODE_REG = Value; break;
		case 0x03F00010: RDRAM_REF_INTERVAL_REG = Value; break;
		case 0x03F00014: RDRAM_REF_ROW_REG = Value; break;
		case 0x03F00018: RDRAM_RAS_INTERVAL_REG = Value; break;
		case 0x03F0001C: RDRAM_MIN_INTERVAL_REG = Value; break;
		case 0x03F00020: RDRAM_ADDR_SELECT_REG = Value; break;
		case 0x03F00024: RDRAM_DEVICE_MANUF_REG = Value; break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			return FALSE;
		}
		break;
	case 0x04000000: 
		if (PAddr < 0x04002000) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0xA0000000,4);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((m_RDRAM + PAddr), 4, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(DWORD *)(m_RDRAM+PAddr) = Value;
			if (VirtualProtect((m_RDRAM + PAddr), 4, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			break;
		}
		switch (PAddr) {
		case 0x04040000: SP_MEM_ADDR_REG = Value; break;
		case 0x04040004: SP_DRAM_ADDR_REG = Value; break;
		case 0x04040008: 
			SP_RD_LEN_REG = Value; 
			SP_DMA_READ();
			break;
		case 0x0404000C: 
			SP_WR_LEN_REG = Value; 
			SP_DMA_WRITE();
			break;
		case 0x04040010: 
			if ( ( Value & SP_CLR_HALT ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_HALT; }
			if ( ( Value & SP_SET_HALT ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_HALT;  }
			if ( ( Value & SP_CLR_BROKE ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_BROKE; }
			if ( ( Value & SP_CLR_INTR ) != 0) { 
				MI_INTR_REG &= ~MI_INTR_SP; 
				_Reg->CheckInterrupts();
			}
//			if ( ( Value & SP_SET_INTR ) != 0) { DisplayError("SP_SET_INTR"); }
			if ( ( Value & SP_CLR_SSTEP ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
			if ( ( Value & SP_SET_SSTEP ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SSTEP;  }
			if ( ( Value & SP_CLR_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
			if ( ( Value & SP_SET_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
			if ( ( Value & SP_CLR_SIG0 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG0; }
			if ( ( Value & SP_SET_SIG0 ) != 0) { 
				SP_STATUS_REG |= SP_STATUS_SIG0;  
				MI_INTR_REG |= MI_INTR_SP; 
				_Reg->CheckInterrupts();				
			}
			if ( ( Value & SP_CLR_SIG1 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG1; }
			if ( ( Value & SP_SET_SIG1 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG1;  }
			if ( ( Value & SP_CLR_SIG2 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG2; }
			if ( ( Value & SP_SET_SIG2 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG2;  }
			if ( ( Value & SP_CLR_SIG3 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG3; }
			if ( ( Value & SP_SET_SIG3 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG3;  }
			if ( ( Value & SP_CLR_SIG4 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG4; }
			if ( ( Value & SP_SET_SIG4 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG4;  }
			if ( ( Value & SP_CLR_SIG5 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG5; }
			if ( ( Value & SP_SET_SIG5 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG5;  }
			if ( ( Value & SP_CLR_SIG6 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG6; }
			if ( ( Value & SP_SET_SIG6 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG6;  }
			if ( ( Value & SP_CLR_SIG7 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG7; }
			if ( ( Value & SP_SET_SIG7 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG7;  }
			if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
				if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {
					_System->ExternalEvent(ExecuteRSP);
				}
			}
			break;
		case 0x0404001C: SP_SEMAPHORE_REG = 0; break;
		case 0x04080000: SP_PC_REG = Value & 0xFFC; break;
		default:
			return FALSE;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x04100000: 
			DPC_START_REG = Value; 
			DPC_CURRENT_REG = Value; 
			break;
		case 0x04100004: 
			DPC_END_REG = Value; 
			WriteTrace(TraceGfxPlugin,"ProcessRDPList: Starting");
			_System->_Plugins->Gfx()->ProcessRDPList(); 
			WriteTrace(TraceGfxPlugin,"ProcessRDPList: Done");
			break;
		case 0x0410000C:
			if ( ( Value & DPC_CLR_XBUS_DMEM_DMA ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA; }
			if ( ( Value & DPC_SET_XBUS_DMEM_DMA ) != 0) { DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_FREEZE; }
			if ( ( Value & DPC_SET_FREEZE ) != 0) { DPC_STATUS_REG |= DPC_STATUS_FREEZE;  }		
			if ( ( Value & DPC_CLR_FLUSH ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_FLUSH; }
			if ( ( Value & DPC_SET_FLUSH ) != 0) { DPC_STATUS_REG |= DPC_STATUS_FLUSH;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) {
				if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
					if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {
						_System->ExternalEvent(ExecuteRSP);
					}
				}
			}
			break;
		default:
			return FALSE;
		}
		break;
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			MI_MODE_REG &= ~0x7F;
			MI_MODE_REG |= (Value & 0x7F);
			if ( ( Value & MI_CLR_INIT ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_INIT; }
			if ( ( Value & MI_SET_INIT ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_INIT; }
			if ( ( Value & MI_CLR_EBUS ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_EBUS; }
			if ( ( Value & MI_SET_EBUS ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_EBUS; }
			if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
				MI_INTR_REG &= ~MI_INTR_DP; 
				_Reg->CheckInterrupts();
			}
			if ( ( Value & MI_CLR_RDRAM ) != 0 ) { _Reg->MI_MODE_REG &= ~MI_MODE_RDRAM; }
			if ( ( Value & MI_SET_RDRAM ) != 0 ) { _Reg->MI_MODE_REG |= MI_MODE_RDRAM; }
			break;
		case 0x0430000C: 
			if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
			if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
			break;
		default:
			return FALSE;
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (VI_STATUS_REG != Value) { 
				VI_STATUS_REG = Value; 
				//if (ViStatusChanged != NULL ) { ViStatusChanged(); }
			}
			break;
		case 0x04400004: 
			VI_ORIGIN_REG = (Value & 0xFFFFFF); 
			//if (UpdateScreen != NULL ) { UpdateScreen(); }
			break;
		case 0x04400008: 
			if (VI_WIDTH_REG != Value) {
				VI_WIDTH_REG = Value; 
				//if (ViWidthChanged != NULL ) { ViWidthChanged(); }
			}
			break;
		case 0x0440000C: VI_INTR_REG = Value; break;
		case 0x04400010: 
			MI_INTR_REG &= ~MI_INTR_VI;
			_Reg->CheckInterrupts();
			break;
		case 0x04400014: VI_BURST_REG = Value; break;
		case 0x04400018: VI_V_SYNC_REG = Value; break;
		case 0x0440001C: VI_H_SYNC_REG = Value; break;
		case 0x04400020: VI_LEAP_REG = Value; break;
		case 0x04400024: VI_H_START_REG = Value; break;
		case 0x04400028: VI_V_START_REG = Value; break;
		case 0x0440002C: VI_V_BURST_REG = Value; break;
		case 0x04400030: VI_X_SCALE_REG = Value; break;
		case 0x04400034: VI_Y_SCALE_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04500000: 
		switch (PAddr) {
		case 0x04500000: AI_DRAM_ADDR_REG = Value; break;
		case 0x04500004: 
			AI_LEN_REG = Value;  
			if (_Reg->GetTimer(AiTimer) == 0) {
				_System->_Plugins->Audio()->LenChanged();
				_Reg->ChangeTimerFixed(AiTimer,AI_LEN_REG * _System->_Plugins->Audio()->CountsPerByte()); 
				AI_LEN_REG = 0;
			} else {
				AI_STATUS_REG |= AI_STATUS_FIFO_FULL;
				_Reg->ChangeTimerFixed(AiTimerDMA,100); 
			}
			break;
		case 0x04500008: 
			AI_CONTROL_REG = (Value & 1); 
			break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			MI_INTR_REG &= ~MI_INTR_AI;
			_Reg->CheckInterrupts();
			break;
		case 0x04500010: 
			AI_DACRATE_REG = Value;  
			_System->_Plugins->Audio()->DacrateChanged(SYSTEM_NTSC);
			break;
		case 0x04500014:  AI_BITRATE_REG = Value; break;
		default:
			return FALSE;
		}
		break;
	case 0x04600000: 
		switch (PAddr) {
		case 0x04600000: PI_DRAM_ADDR_REG = Value; break;
		case 0x04600004: PI_CART_ADDR_REG = Value; break;
		case 0x04600008: 
			PI_RD_LEN_REG = Value; 
			PI_DMA_Read();
			break;
		case 0x0460000C: 
			PI_WR_LEN_REG = Value; 
			PI_DMA_Write();
			break;
		case 0x04600010:
			if ((Value & PI_CLR_INTR) != 0 ) {
				MI_INTR_REG &= ~MI_INTR_PI;
				_Reg->CheckInterrupts();
			}
			break;
		case 0x04600014: PI_DOMAIN1_REG = (Value & 0xFF); break; 
		case 0x04600018: PI_BSD_DOM1_PWD_REG = (Value & 0xFF); break; 
		case 0x0460001C: PI_BSD_DOM1_PGS_REG = (Value & 0xFF); break; 
		case 0x04600020: PI_BSD_DOM1_RLS_REG = (Value & 0xFF); break; 
		case 0x04600024: PI_DOMAIN2_REG = (Value & 0xFF); break; 
		case 0x04600028: PI_BSD_DOM2_PWD_REG = (Value & 0xFF); break; 
		case 0x0460002C: PI_BSD_DOM2_PGS_REG = (Value & 0xFF); break; 
		case 0x04600030: PI_BSD_DOM2_RLS_REG = (Value & 0xFF); break; 
		default:
			return FALSE;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: RI_MODE_REG = Value; break;
		case 0x04700004: RI_CONFIG_REG = Value; break;
		case 0x04700008: RI_CURRENT_LOAD_REG = Value; break;
		case 0x0470000C: RI_SELECT_REG = Value; break;
		case 0x04700010: RI_REFRESH_REG = Value; break;
		case 0x04700014: RI_LATENCY_REG = Value; break;
		case 0x04700018: RI_RERROR_REG = Value; break;
		case 0x0470001C: RI_WERROR_REG = Value; break;
		default:
			return false;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: SI_DRAM_ADDR_REG = Value; break;
		case 0x04800004: 
			SI_PIF_ADDR_RD64B_REG = Value; 
			SI_DMA_READ ();
			break;
		case 0x04800010: 
			SI_PIF_ADDR_WR64B_REG = Value; 
			SI_DMA_WRITE();
			break;
		case 0x04800018: 
			MI_INTR_REG &= ~MI_INTR_SI; 
			SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
			_Reg->CheckInterrupts();
			break;
		default:
			return FALSE;
		}
		break;
	case 0x08000000:
		if (PAddr != 0x08010000) { return FALSE; }
		if (_Settings->LoadDword(SaveChipType) == SaveChip_Auto) { 
			_Settings->Save(SaveChipType,SaveChip_FlashRam); 
		}
		if (_Settings->LoadDword(SaveChipType) != SaveChip_FlashRam) {
			return true; 
		}
		if (m_FlashRam == NULL) { m_FlashRam = new CFlashRam(_Settings,_Notify,m_SavesReadOnly); }
		m_FlashRam->WriteToFlashCommand(Value);
		break;
	default:
		return false;
	}
	return true;
}

int CMipsMemoryVM::SystemMemoryFilter( DWORD dwExptCode, void * lpExceptionPointer ) {
	//convert the pointer since we are not having win32 stuctures in headers
	LPEXCEPTION_POINTERS lpEP = (LPEXCEPTION_POINTERS)lpExceptionPointer;
	
	//if not an access violation then do not try to handle it
	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) { return EXCEPTION_CONTINUE_SEARCH; }

	//if Memory Address is greater then base memory then do not handle exception
	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)m_RDRAM;
    if ((int)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF) { return EXCEPTION_CONTINUE_SEARCH; }

	//Get where the code was executing from
	BYTE * ExecPos = (unsigned char *)lpEP->ContextRecord->Eip;

	//Handle if the exception occured from a DMA
	if (*ExecPos == 0xF3 && *(ExecPos + 1) == 0xA5) {
//		CRecompiler * Recomp = _System->GetRecompiler();
//		if (!Recomp) { _Notify->BreakPoint(__FILE__,__LINE__); }
		int Start = (lpEP->ContextRecord->Edi - (DWORD)m_RDRAM);
		int End = (Start + (lpEP->ContextRecord->Ecx << 2) - 1);
		
		if ((int)Start < 0) {  _Notify->BreakPoint(__FILE__,__LINE__); }
		if ((int)End < _Settings->LoadDword(Game_RDRamSize)) {
			for ( int count = Start & ~0x1000; count < End; count += 0x1000 ) {
				CBClass->WriteToProtectedMemory(Start, 0xFFF);
			}			
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04000000 && End < 0x04002000) {
			for ( int count = Start & ~0x1000; count < End; count += 0x1000 ) {
				CBClass->WriteToProtectedMemory(Start, 0xFFF);
			}			
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		_Notify->BreakPoint(__FILE__,__LINE__);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	//Set up where reading register information from
	BYTE * ReadPos;
	if (*ExecPos == 0x0F && *(ExecPos + 1) == 0xB6) {
		ReadPos = ExecPos + 2;
	} else if (*ExecPos == 0x0F && *(ExecPos + 1) == 0xB7) {
		ReadPos = ExecPos + 2;
	} else if (*ExecPos == 0x0F && *(ExecPos + 1) == 0xBE) {
		ReadPos = ExecPos + 2;
	} else if (*ExecPos == 0x0F && *(ExecPos + 1) == 0xBF) {
		ReadPos = ExecPos + 2;
	} else if (*ExecPos == 0x66) {
		ReadPos = ExecPos + 2;
	} else {
		ReadPos = ExecPos + 1;
	}

	//Find the target Register to store results
	void * TargetRegister;
	switch ((*ReadPos & 0x38)) {
	case 0x00: TargetRegister = &lpEP->ContextRecord->Eax; break;
	case 0x08: TargetRegister = &lpEP->ContextRecord->Ecx; break; 
	case 0x10: TargetRegister = &lpEP->ContextRecord->Edx; break; 
	case 0x18: TargetRegister = &lpEP->ContextRecord->Ebx; break; 
	case 0x20: TargetRegister = &lpEP->ContextRecord->Esp; break;
	case 0x28: TargetRegister = &lpEP->ContextRecord->Ebp; break;
	case 0x30: TargetRegister = &lpEP->ContextRecord->Esi; break;
	case 0x38: TargetRegister = &lpEP->ContextRecord->Edi; break;
	}

	//Calculate the length of the opcode that cause the exception
//7877086D C6 05 C2 10 DF 3A 04 mov         byte ptr ds:[3ADF10C2h],4

	BYTE * NextOp = ReadPos;
	if ((*ReadPos & 7) == 4) {
		switch ((*(ReadPos + 1) & 0xC7)) {
		case 0: NextOp += 1; break;
		case 1: NextOp += 1; break;
		case 2: NextOp += 1; break;
		case 3: NextOp += 1; break;
		case 6: NextOp += 1; break;
		case 7: NextOp += 1; break;
		case 0x87: NextOp += 1; break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	}
	if ((*ReadPos & 7) == 5) 
	{
		NextOp += 4; 
	}
	switch ((*ReadPos & 0xC0)) {
	case 0x00: NextOp += 1; break;
	case 0x40: NextOp += 2; break; 
	case 0x80: NextOp += 5; break;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
	}

	//Handle the type of access
//789EDD5F 0F B7 3C 0A          movzx       edi,word ptr [edx+ecx]
//789E43BB 0F B7 3C 0A          movzx       edi,word ptr [edx+ecx]
	switch(*ExecPos) {
	case 0x0F:
		switch(*(ExecPos + 1)) {
		case 0xB6:
			{
				BYTE Value;
				if (!LoadByte_NonMemory(MemAddress,(BYTE *)&Value)) {
					MemoryFilterFailed("load half word",MemAddress,(DWORD)ExecPos,0);
				} 
				*((DWORD *)TargetRegister) = (DWORD)Value;
			}
			break;
		case 0xB7:
			{
				WORD Value;
				if (!LoadHalf_NonMemory(MemAddress,(WORD *)&Value)) {
					MemoryFilterFailed("load half word",MemAddress,(DWORD)ExecPos,0);
				} 
				*((DWORD *)TargetRegister) = (DWORD)Value;
			}
			break;
		case 0xBE:
			{
				BYTE Value;
				if (!LoadByte_NonMemory(MemAddress,(BYTE *)&Value)) {
					MemoryFilterFailed("load half word",MemAddress,(DWORD)ExecPos,0);
				} 
				*((DWORD *)TargetRegister) = (DWORD)((long)((char)Value));
			}
			break;
		case 0xBF:
			{
				WORD Value;
				if (!LoadHalf_NonMemory(MemAddress,(WORD *)&Value)) {
					MemoryFilterFailed("load half word",MemAddress,(DWORD)ExecPos,0);
				} 
				*((DWORD *)TargetRegister) = (DWORD)((long)((short)Value));
			}
			break;
		default:
			_Notify->DisplayError("Unknown x86 opcode 0x0F 0x%X\nSystem location: %X\nlocation in n64 space: %X", 
				*(ExecPos + 1), lpEP->ContextRecord->Eip, MemAddress);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x66:
		switch(*(ExecPos + 1)) {
		case 0x8B: 
			if (!LoadHalf_NonMemory(MemAddress,(WORD *)TargetRegister)) {
				MemoryFilterFailed("load half word",MemAddress,(DWORD)ExecPos,0);
			}
			break;
		case 0x89:
			if (!StoreHalf_NonMemory(MemAddress,*(WORD *)TargetRegister)) {
				MemoryFilterFailed("Store half word",MemAddress,(DWORD)ExecPos,*(WORD *)TargetRegister);
			}
			break;
		case 0xC7:
			if (TargetRegister != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
			if (!StoreHalf_NonMemory(MemAddress,*(WORD *)NextOp)) {
				MemoryFilterFailed("Store half word",MemAddress,(DWORD)ExecPos,*(WORD *)TargetRegister);
			}
			NextOp += 2;
			break;
		default:
			//Could Not handle the x86 opcode
			_Notify->DisplayError("Unknown x86 opcode 0x66 0x%X\nSystem location: %X\nlocation in n64 space: %X", 
				*(ExecPos + 1), lpEP->ContextRecord->Eip, MemAddress);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x88:
		if (!StoreByte_NonMemory(MemAddress,*(BYTE *)TargetRegister)) {
			MemoryFilterFailed("Store Byte",MemAddress,(DWORD)ExecPos,*(BYTE *)TargetRegister);
		}
		break;
	case 0x89:
		if (!StoreWord_NonMemory(MemAddress,*(DWORD *)TargetRegister)) {
			MemoryFilterFailed("Store word",MemAddress,(DWORD)ExecPos,*(DWORD *)TargetRegister);
		}
		break;
	case 0x8B: 
		if (!LoadWord_NonMemory(MemAddress,(DWORD *)TargetRegister)) {
			MemoryFilterFailed("load word",MemAddress,(DWORD)ExecPos,0);
		}
		break;
	case 0xC6:
		if (TargetRegister != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!StoreByte_NonMemory(MemAddress,*(BYTE *)ReadPos)) {
			MemoryFilterFailed("Store byte C6",MemAddress,(DWORD)ExecPos,*(DWORD *)NextOp);
		}
		NextOp += 1;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC7:
		if (TargetRegister != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!StoreWord_NonMemory(MemAddress,*(DWORD *)NextOp)) {
			MemoryFilterFailed("Store word",MemAddress,(DWORD)ExecPos,*(DWORD *)NextOp);
		}
		NextOp += 4;
		break;		
	default:
		//Could Not handle the x86 opcode
		_Notify->DisplayError("Unknown x86 opcode %X\nSystem location: %X\nlocation in n64 space: %X", 
			*(unsigned char *)lpEP->ContextRecord->Eip, lpEP->ContextRecord->Eip, MemAddress);
		return EXCEPTION_CONTINUE_SEARCH;
	}
	lpEP->ContextRecord->Eip = (DWORD)NextOp;
	return EXCEPTION_CONTINUE_EXECUTION;
}

bool CMipsMemoryVM::SearchForValue (DWORD Value, MemorySize Size, DWORD &StartAddress, DWORD &Len )
{
	if (Size == _32Bit) { StartAddress = ((StartAddress + 3) & ~3); }
	if (Size == _16Bit) { StartAddress = ((StartAddress + 1) & ~1); }

	//search memory
	if (StartAddress < RdramSize())
	{
		DWORD EndMemSearchAddr = StartAddress + Len;
		if (EndMemSearchAddr > RdramSize())
		{
			EndMemSearchAddr = RdramSize(); 
		}
		
		DWORD pos;
		switch (Size)
		{
		case _32Bit:
			for (pos  = StartAddress; pos < EndMemSearchAddr; pos += 4)
			{
				if (*(DWORD *)(m_RDRAM + pos) == Value)
				{
					Len -= pos - StartAddress;
					StartAddress = pos;
					return true;
				}
			}
			break;
		case _16Bit:
			for (pos = StartAddress; pos < EndMemSearchAddr; pos += 2)
			{
				if (*(WORD *)(m_RDRAM + (pos ^ 2)) == (WORD)Value)
				{
					Len -= pos - StartAddress;
					StartAddress = pos;
					return true;
				}
			}
			break;
		case _8Bit:
			for (pos = StartAddress; pos < EndMemSearchAddr; pos ++)
			{
				if (*(BYTE *)(m_RDRAM + (pos ^ 3)) == (BYTE)Value)
				{
					Len -= pos - StartAddress;
					StartAddress = pos;
					return true;
				}
			}
			break;
		default:
			Notify().BreakPoint(__FILE__,__LINE__);
		}
	}
	if (StartAddress >= 0x10000000)
	{
		DWORD EndMemSearchAddr = StartAddress + Len - 0x10000000;
		if (EndMemSearchAddr > m_RomFileSize)
		{
			EndMemSearchAddr = m_RomFileSize; 
		}
		StartAddress -= 0x10000000;
		
		DWORD pos;
		switch (Size)
		{
		case _32Bit:
			for (pos  = StartAddress; pos < EndMemSearchAddr; pos += 4)
			{
				if (*(DWORD *)(ROM + pos) == Value)
				{
					Len -= pos - StartAddress;
					StartAddress = pos + 0x10000000;
					return true;
				}
			}
			break;
		case _16Bit:
			for (pos = StartAddress; pos < EndMemSearchAddr; pos += 2)
			{
				if (*(WORD *)(ROM + (pos ^ 2)) == (WORD)Value)
				{
					Len -= pos - StartAddress;
					StartAddress = pos + 0x10000000;
					return true;
				}
			}
			break;
		case _8Bit:
			for (pos = StartAddress; pos < EndMemSearchAddr; pos ++)
			{
				if (*(BYTE *)(ROM + (pos ^ 3)) == (BYTE)Value)
				{
					Len -= pos - StartAddress;
					StartAddress = pos + 0x10000000;
					return true;
				}
			}
			break;
		default:
			Notify().BreakPoint(__FILE__,__LINE__);
		}
	}
	return false;
}

bool CMipsMemoryVM::SearchSetBaseForChanges ( void )
{
	if (m_MemoryState == NULL)
	{
		delete [] m_MemoryState;
	}
	m_MemoryStateSize = RdramSize();
	m_MemoryState = new BYTE[m_MemoryStateSize];
	memcpy(m_MemoryState,m_RDRAM,m_MemoryStateSize);
	return true;
}

bool CMipsMemoryVM::SearchForChanges(SearchMemChangeState SearchType, MemorySize Size, 
								   DWORD &StartAddress, DWORD &Len, 
								   DWORD &OldValue,     DWORD &NewValue)
{
	if (SearchType == SearchChangeState_Reset)
	{
		Notify().BreakPoint(__FILE__,__LINE__);
	}
	if (Size == _32Bit) { StartAddress = ((StartAddress + 3) & ~3); }
	if (Size == _16Bit) { StartAddress = ((StartAddress + 1) & ~1); }

	//search memory
	if (StartAddress < RdramSize())
	{
		DWORD EndMemSearchAddr = StartAddress + Len;
		if (EndMemSearchAddr > RdramSize())
		{
			EndMemSearchAddr = RdramSize(); 
		}
		
		DWORD pos;
		switch (Size)
		{
		case _32Bit:
			for (pos  = StartAddress; pos < EndMemSearchAddr; pos += 4)
			{
				OldValue = *(DWORD *)(m_MemoryState + pos);
				NewValue = *(DWORD *)(m_RDRAM + pos);
				if ((SearchType == SearchChangeState_Changed && NewValue != OldValue) ||
					(SearchType == SearchChangeState_Unchanged && NewValue == OldValue) ||
					(SearchType == SearchChangeState_Greater && NewValue > OldValue) || 
					(SearchType == SearchChangeState_Lessthan && NewValue < OldValue))
				{
					*(DWORD *)(m_MemoryState + pos) = NewValue;
					Len -= pos - StartAddress;
					StartAddress = pos;
					return true;
				}
			}
			break;
		case _16Bit:
			for (pos = StartAddress; pos < EndMemSearchAddr; pos += 2)
			{
				OldValue = *(WORD *)(m_MemoryState + (pos ^ 2));
				NewValue = *(WORD *)(m_RDRAM + (pos ^ 2));
				if ((SearchType == SearchChangeState_Changed && NewValue != OldValue) ||
					(SearchType == SearchChangeState_Unchanged && NewValue == OldValue) ||
					(SearchType == SearchChangeState_Greater && NewValue > OldValue) || 
					(SearchType == SearchChangeState_Lessthan && NewValue < OldValue))
				{
					Len -= pos - StartAddress;
					StartAddress = pos;
					return true;
				}
			}
			break;
		case _8Bit:
			for (pos = StartAddress; pos < EndMemSearchAddr; pos ++)
			{
				OldValue = *(BYTE *)(m_MemoryState + (pos ^ 3));
				NewValue = *(BYTE *)(m_RDRAM + (pos ^ 3));
				if ((SearchType == SearchChangeState_Changed && NewValue != OldValue) ||
					(SearchType == SearchChangeState_Unchanged && NewValue == OldValue) ||
					(SearchType == SearchChangeState_Greater && NewValue > OldValue) || 
					(SearchType == SearchChangeState_Lessthan && NewValue < OldValue))
				{
					Len -= pos - StartAddress;
					StartAddress = pos;
					return true;
				}
			}
			break;
		default:
			Notify().BreakPoint(__FILE__,__LINE__);
		}
	}
	return false;
}
#endif

void CMipsMemoryVM::ProtectMemory( DWORD StartVaddr, DWORD EndVaddr ) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (!CTLB::ValidVaddr(StartVaddr) || !CTLB::ValidVaddr(EndVaddr)) { return; }

	//Get Physical Addresses passed
	DWORD StartPAddr, EndPAddr;
	if (!TranslateVaddr(StartVaddr,StartPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	if (!TranslateVaddr(EndVaddr,EndPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	
	//Get Length of memory being protected
	int Length = (EndPAddr + 4) - StartPAddr;
	if (Length < 0) { _Notify->BreakPoint(__FILE__,__LINE__); }

	//Proect that memory address space
	DWORD OldProtect;
	void * MemLoc;
	
	if (!VAddrToRealAddr(StartVaddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	VirtualProtect(MemLoc, Length, PAGE_READONLY, &OldProtect);	
#endif
}

void CMipsMemoryVM::UnProtectMemory( DWORD StartVaddr, DWORD EndVaddr ) 
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	if (!CTLB::ValidVaddr(StartVaddr) || !CTLB::ValidVaddr(EndVaddr)) { return; }

	//Get Physical Addresses passed
	DWORD StartPAddr, EndPAddr;
	if (!TranslateVaddr(StartVaddr,StartPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	if (!TranslateVaddr(EndVaddr,EndPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	
	//Get Length of memory being protected
	int Length = (EndPAddr + 4) - StartPAddr;
	if (Length < 0) { _Notify->BreakPoint(__FILE__,__LINE__); }

	//Proect that memory address space
	DWORD OldProtect;
	void * MemLoc;
	
	if (!VAddrToRealAddr(StartVaddr,MemLoc)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	VirtualProtect(MemLoc, Length, PAGE_READWRITE, &OldProtect);	
#endif
}


LPCTSTR CMipsMemoryVM::LabelName ( DWORD Address ) const
{
	//StringMap::iterator theIterator = m_LabelList.find(Address);
	//if (theIterator != m_LabelList.end()) {
	//	return (*theIterator).second;
	//}
	
	sprintf(m_strLabelName,"0x%08X",Address);		
	return m_strLabelName;
}

void CMipsMemoryVM::TLB_Mapped( DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly )
{
	for (DWORD count = VAddr, VEnd = VAddr + Len; count < VEnd; count += 0x1000) {
		DWORD Index = count >> 12;
		m_TLB_ReadMap[Index] = ((DWORD)m_RDRAM + (count - VAddr + PAddr)) - count;
		if (!bReadOnly) 
		{
			m_TLB_WriteMap[Index] = ((DWORD)m_RDRAM + (count - VAddr + PAddr)) - count;
		}
	}
}

void CMipsMemoryVM::TLB_Unmaped( DWORD Vaddr, DWORD Len )
{
	for (DWORD count = Vaddr, End = Vaddr + Len; count < End; count += 0x1000) 
	{
		DWORD Index = count >> 12;
		m_TLB_ReadMap[Index] = NULL;
		m_TLB_WriteMap[Index] = NULL;
	}
}