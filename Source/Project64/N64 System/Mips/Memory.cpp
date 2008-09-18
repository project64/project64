#include "..\..\N64 System.h"
#include "..\..\Plugin.h"

#include <windows.h> //needed for virtual memory

CMipsMemory::CMipsMemory ( CMipsMemory_CallBack * CallBack, CN64System * System, CN64Rom * CurrentRom, 
						  CNotification * Notify, CRegisters * RegSet,
						  bool SavesReadOnly ) :
	CBClass(CallBack),
	_Notify(Notify),
	CTLB(System,RDRAM,RegSet),
//	CPIFRam(System->_Plugins,_Reg,Notify,SavesReadOnly),
	_System(System),
	_Rom(CurrentRom),
	_Reg(RegSet),
	m_MemoryState(NULL),
	m_MemoryStateSize(0)
{ 
	RDRAM      = NULL;
	DMEM       = NULL;
	IMEM       = NULL;
	ROM        = NULL;
//	m_Sram     = NULL;
//	m_FlashRam = NULL;
//	m_SavesReadOnly = SavesReadOnly;
//	m_WrittenToRom  = false;
	m_HalfLine      = 0;
	JumpTable       = NULL;
//	DelaySlotTable  = NULL;
	m_RecompCode      = NULL;
	m_RecompSize    = 0;     
	InitalizeSystem(true);
}

CMipsMemory::~CMipsMemory (void) {
	if (_Reg) { delete _Reg; } //Delete Created Reg Class
	if (RDRAM) {
		VirtualFree( RDRAM, 0 , MEM_RELEASE);
	}
	if (JumpTable)  { VirtualFree( JumpTable, 0 , MEM_RELEASE); }
	if (m_RecompCode) { VirtualFree( m_RecompCode, 0 , MEM_RELEASE); }
//	if (DelaySlotTable) { VirtualFree( DelaySlotTable, 0 , MEM_RELEASE); }
	JumpTable      = NULL;
	m_RecompCode   = NULL;
	m_RecompSize   = 0;
//	DelaySlotTable = NULL;
	RDRAM          = NULL;
	DMEM           = NULL;
	IMEM           = NULL;
	ROM            = NULL;
//	if (m_Sram) {
//		delete m_Sram;
//		m_Sram = NULL;
//	}
//	if (m_FlashRam) {
//		delete m_FlashRam;
//		m_FlashRam = NULL;
//	}
	if (m_MemoryState)
	{
		delete m_MemoryState;
		m_MemoryState = NULL;
		m_MemoryStateSize = 0;
	}
}

void CMipsMemory::InitalizeSystem ( bool PostPif ) 
{
	ROM = _Rom->GetRomAddress();
	m_RomFileSize = _Rom->GetRomSize();
	
	AllocateSystemMemory();
	_Reg->InitalizeR4300iRegisters(this, PostPif, _Rom->GetCountry(), _Rom->CicChipID());
	if (PostPif) {
		memcpy( (DMEM+0x40), (ROM + 0x040), 0xFBC);
	}
}

void CMipsMemory::AllocateSystemMemory (void) 
{
	if (RDRAM != NULL)
	{
		memset(PIF_Ram,0,sizeof(PIF_Ram));
		TLB_Reset(true);
		return;
	}
	
	DWORD RdramMemorySize = 0x20000000;
	if ((CPU_TYPE)_Settings->LoadDword(ROM_CPUType) == CPU_SyncCores)
	{
		RdramMemorySize = 0x18000000;
	}
	RDRAM = (unsigned char *) VirtualAlloc( NULL, RdramMemorySize, MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );
	if(RDRAM==NULL) {  
		_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
	}
	
	m_AllocatedRdramSize = 0x00400000;
	if(VirtualAlloc(RDRAM, m_AllocatedRdramSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
		_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
	}

	if(VirtualAlloc(RDRAM + 0x04000000, 0x2000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
		_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
	}

	DMEM  = (unsigned char *)(RDRAM+0x04000000);
	IMEM  = (unsigned char *)(RDRAM+0x04001000);

	if (_Settings->LoadDword(RomInMemory))
	{
		if(VirtualAlloc(RDRAM + 0x10000000, m_RomFileSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
			_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
		}
		memcpy(RDRAM + 0x10000000,ROM,m_RomFileSize);
		ROM = (unsigned char *)(RDRAM+0x10000000);
		_Rom->UnallocateRomImage();
	}
	memset(PIF_Ram,0,sizeof(PIF_Ram));
	TLB_Reset(true);
}

bool CMipsMemory::AllocateRecompilerMemory ( bool AllocateJumpTable ) 
{
	if (JumpTable)
	{
		VirtualFree( JumpTable, 0 , MEM_RELEASE);
	}
	JumpTable = NULL;
	if (AllocateJumpTable)
	{
		DWORD JumpTableSize = _Settings->LoadDword(RomInMemory) ? 0x20000000 : 0x10000000;
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

		if (_Settings->LoadDword(RomInMemory))
		{
			if(VirtualAlloc((BYTE *)JumpTable + 0x10000000, m_RomFileSize, MEM_COMMIT, PAGE_READWRITE)==NULL) {
				_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
				return FALSE;
			}
		}
		
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

void CMipsMemory::CheckRecompMem( BYTE * RecompPos )
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

void CMipsMemory::FixRDramSize ( void ) {
	if (_Settings->LoadDword(RamSize) != m_AllocatedRdramSize) {
		if (m_AllocatedRdramSize == 0x400000) { 
			if (VirtualAlloc(RDRAM + 0x400000, 0x400000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
				_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
			}
			m_AllocatedRdramSize = 0x800000;
		} else {
			VirtualFree(RDRAM + 0x400000, 0x400000,MEM_DECOMMIT);
			m_AllocatedRdramSize = 0x400000;
		}
	}
}

bool CMipsMemory::Store64 ( DWORD VAddr, QWORD Value, MemorySize Size ) {
	//__try {
		DWORD PAddr;
		if (!TranslateVaddr(VAddr,PAddr)) {
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		if (PAddr > _Settings->LoadDword(RamSize) && 
			(PAddr < 0x04000000 || PAddr > 0x04002000))
		{			
//			switch (Size) {
//			case _16Bit: 
//				if (!StoreHalf_NonMemory(PAddr,static_cast<WORD>(Value))) {
//					MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<WORD>(Value));
//				}
//				return true;
//				break;
//			case _32Bit: 
//				if (!StoreWord_NonMemory(PAddr,static_cast<DWORD>(Value))) {
//					MemoryFilterFailed("Store word",PAddr,PROGRAM_COUNTER, static_cast<DWORD>(Value));
//				}
//				return true;
//				break;
//			default:
				_Notify->BreakPoint(__FILE__,__LINE__);
//			}
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

bool CMipsMemory::StorePhysical64 ( DWORD PAddr, QWORD Value, MemorySize Size ) {
	if (PAddr > m_AllocatedRdramSize && 
		(PAddr < 0x04000000 || PAddr > 0x04002000))
	{			
		return false;
	}
	
	switch (Size) {
	case _8Bit:
		*(BYTE *)(RDRAM + (PAddr ^ 3)) = (BYTE)Value;
		return true;
	case _16Bit:
		*(WORD *)(RDRAM + (PAddr ^ 2)) = (WORD)Value;
		return true;
	case _32Bit:
		*(DWORD *)(RDRAM + PAddr) = (DWORD)Value;
		return true;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
	}

	return false;
}

bool CMipsMemory::Load32 ( DWORD VAddr, DWORD & Variable, MemorySize Size, bool SignExtend ) {
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

bool CMipsMemory::LoadPhysical32 ( DWORD PAddr, DWORD & Variable, MemorySize Size, bool SignExtend ) {
	if (PAddr >= 0x18000000)
	{
		return false;
	}
	__try {
		void * MemLoc;

		switch (Size) {
		case _8Bit:
			MemLoc = RDRAM + (PAddr ^ 3);
			Variable = (DWORD)SignExtend?(int)(*(char *)MemLoc):*(BYTE *)MemLoc;
			return true;
		case _16Bit:
			MemLoc = RDRAM + (PAddr ^ 2);
			Variable = (DWORD)SignExtend?(int)(*(short *)MemLoc):*(WORD *)MemLoc;
			return true;
		case _32Bit:
			MemLoc = RDRAM + PAddr;
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

void CMipsMemory::MemoryFilterFailed( char * FailureType, DWORD MipsAddress,  DWORD x86Address, DWORD Value) {
	if (_Settings->LoadDword(ShowUnhandledMemory)) {
		_Notify->DisplayError("Failed to %s\n\nProgram Counter: %X\nMIPS Address: %08X\nX86 Address: %X\n Value: %X",
			FailureType, _Reg->PROGRAM_COUNTER, MipsAddress, x86Address, Value);
	}
	return;
}

#ifdef ggg

bool CMipsMemory::Load64 ( DWORD VAddr, QWORD & Variable, MemorySize Size, bool SignExtend ) {
	__try {
		void * MemLoc;
		
		DWORD PAddr;
		if (!TranslateVaddr(VAddr,PAddr)) {
			return false;
		}
		if (PAddr > _Settings->LoadDword(RamSize) && 
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
bool CMipsMemory::LoadByte_NonMemory ( DWORD PAddr, BYTE * Value ) {
	*Value = 0;
	return false;
}

bool CMipsMemory::LoadHalf_NonMemory ( DWORD PAddr, WORD * Value ) {
	*Value = 0;
	return false;
}

bool CMipsMemory::LoadWord_NonMemory ( DWORD PAddr, DWORD * Value ) {
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
		case 0x03F00000: * Value = RDRAM_CONFIG_REG; break;
		case 0x03F00004: * Value = RDRAM_DEVICE_ID_REG; break;
		case 0x03F00008: * Value = RDRAM_DELAY_REG; break;
		case 0x03F0000C: * Value = RDRAM_MODE_REG; break;
		case 0x03F00010: * Value = RDRAM_REF_INTERVAL_REG; break;
		case 0x03F00014: * Value = RDRAM_REF_ROW_REG; break;
		case 0x03F00018: * Value = RDRAM_RAS_INTERVAL_REG; break;
		case 0x03F0001C: * Value = RDRAM_MIN_INTERVAL_REG; break;
		case 0x03F00020: * Value = RDRAM_ADDR_SELECT_REG; break;
		case 0x03F00024: * Value = RDRAM_DEVICE_MANUF_REG; break;	
		default:
			*Value = ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF);
			return false;
		}
		break;
	case 0x04000000:
		switch (PAddr) {
		case 0x04040010: * Value = SP_STATUS_REG; break;
		case 0x04040014: * Value = SP_DMA_FULL_REG; break;
		case 0x04040018: * Value = SP_DMA_BUSY_REG; break;
		case 0x04080000: * Value = SP_PC_REG; break;
		default:
			* Value = 0;
			return FALSE;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x0410000C: *Value = DPC_STATUS_REG; break;
		case 0x04100010: *Value = DPC_CLOCK_REG; break;
		case 0x04100014: *Value = DPC_BUFBUSY_REG; break;
		case 0x04100018: *Value = DPC_PIPEBUSY_REG; break;
		case 0x0410001C: *Value = DPC_TMEM_REG; break;
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

void CMipsMemory::UpdateHalfLine (void) {
	m_HalfLine += 1;
	if (m_HalfLine > 250) { m_HalfLine = 0; }
//	m_HalfLine = (_Reg->GetTimer(ViTimer) / 1500);
//	m_HalfLine &= ~1;
//	m_HalfLine += ViFieldNumber;
}

bool CMipsMemory::Store64 ( DWORD VAddr, QWORD Value, MemorySize Size ) {
	__try {
		DWORD PAddr;
		if (!TranslateVaddr(VAddr,PAddr)) {
			_Notify->BreakPoint(__FILE__,__LINE__);
			return false;
		}
		if (PAddr > _Settings->LoadDword(RamSize) && 
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
bool CMipsMemory::StoreByte_NonMemory ( DWORD PAddr, BYTE Value ) {	
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < _Settings->LoadDword(RamSize)) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0x80000000,1);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((RDRAM + PAddr), 1, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(BYTE *)(RDRAM+PAddr) = Value;
			if (VirtualProtect((RDRAM + PAddr), 1, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
		}
		break;
	default:
		return false;
	}
	return true;
}

bool CMipsMemory::StoreHalf_NonMemory ( DWORD PAddr, WORD Value ) {	
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < _Settings->LoadDword(RamSize)) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0x80000000,1);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((RDRAM + PAddr), 1, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(WORD *)(RDRAM+PAddr) = Value;
			if (VirtualProtect((RDRAM + PAddr), 1, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
		}
		break;
	default:
		return false;
	}
	return true;
	return false;
}

bool CMipsMemory::StoreWord_NonMemory ( DWORD PAddr, DWORD Value ) {
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
		if (PAddr < _Settings->LoadDword(RamSize)) {
//			CRecompiler * Recomp = _System->GetRecompiler();
//			if (Recomp) { 
//				Recomp->ClearRecomplierCode(PAddr + 0x80000000,4);
//			}
			
			DWORD OldProtect, NewProtect;			
			if (VirtualProtect((RDRAM + PAddr), 4, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(DWORD *)(RDRAM+PAddr) = Value;
			if (VirtualProtect((RDRAM + PAddr), 4, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
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
			if (VirtualProtect((RDRAM + PAddr), 4, PAGE_READWRITE, &OldProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
			*(DWORD *)(RDRAM+PAddr) = Value;
			if (VirtualProtect((RDRAM + PAddr), 4, OldProtect, &NewProtect) == 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
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
			if ( ( Value & SP_CLR_HALT ) != 0) { SP_STATUS_REG &= ~SP_STATUS_HALT; }
			if ( ( Value & SP_SET_HALT ) != 0) { SP_STATUS_REG |= SP_STATUS_HALT;  }
			if ( ( Value & SP_CLR_BROKE ) != 0) { SP_STATUS_REG &= ~SP_STATUS_BROKE; }
			if ( ( Value & SP_CLR_INTR ) != 0) { 
				MI_INTR_REG &= ~MI_INTR_SP; 
				_Reg->CheckInterrupts();
			}
//			if ( ( Value & SP_SET_INTR ) != 0) { DisplayError("SP_SET_INTR"); }
			if ( ( Value & SP_CLR_SSTEP ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
			if ( ( Value & SP_SET_SSTEP ) != 0) { SP_STATUS_REG |= SP_STATUS_SSTEP;  }
			if ( ( Value & SP_CLR_INTR_BREAK ) != 0) { SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
			if ( ( Value & SP_SET_INTR_BREAK ) != 0) { SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
			if ( ( Value & SP_CLR_SIG0 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG0; }
			if ( ( Value & SP_SET_SIG0 ) != 0) { 
				SP_STATUS_REG |= SP_STATUS_SIG0;  
				MI_INTR_REG |= MI_INTR_SP; 
				_Reg->CheckInterrupts();				
			}
			if ( ( Value & SP_CLR_SIG1 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG1; }
			if ( ( Value & SP_SET_SIG1 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG1;  }
			if ( ( Value & SP_CLR_SIG2 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG2; }
			if ( ( Value & SP_SET_SIG2 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG2;  }
			if ( ( Value & SP_CLR_SIG3 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG3; }
			if ( ( Value & SP_SET_SIG3 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG3;  }
			if ( ( Value & SP_CLR_SIG4 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG4; }
			if ( ( Value & SP_SET_SIG4 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG4;  }
			if ( ( Value & SP_CLR_SIG5 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG5; }
			if ( ( Value & SP_SET_SIG5 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG5;  }
			if ( ( Value & SP_CLR_SIG6 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG6; }
			if ( ( Value & SP_SET_SIG6 ) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); SP_STATUS_REG |= SP_STATUS_SIG6;  }
			if ( ( Value & SP_CLR_SIG7 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG7; }
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
			if ( ( Value & MI_CLR_INIT ) != 0 ) { MI_MODE_REG &= ~MI_MODE_INIT; }
			if ( ( Value & MI_SET_INIT ) != 0 ) { MI_MODE_REG |= MI_MODE_INIT; }
			if ( ( Value & MI_CLR_EBUS ) != 0 ) { MI_MODE_REG &= ~MI_MODE_EBUS; }
			if ( ( Value & MI_SET_EBUS ) != 0 ) { MI_MODE_REG |= MI_MODE_EBUS; }
			if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
				MI_INTR_REG &= ~MI_INTR_DP; 
				_Reg->CheckInterrupts();
			}
			if ( ( Value & MI_CLR_RDRAM ) != 0 ) { MI_MODE_REG &= ~MI_MODE_RDRAM; }
			if ( ( Value & MI_SET_RDRAM ) != 0 ) { MI_MODE_REG |= MI_MODE_RDRAM; }
			break;
		case 0x0430000C: 
			if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
			if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
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
#endif

void CMipsMemory::ProtectMemory( DWORD StartVaddr, DWORD EndVaddr ) {
	if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr)) { return; }

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
}

void CMipsMemory::UnProtectMemory( DWORD StartVaddr, DWORD EndVaddr ) {
	if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr)) { return; }

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
}

int CMipsMemory::SystemMemoryFilter( DWORD dwExptCode, void * lpExceptionPointer ) {
	//convert the pointer since we are not having win32 stuctures in headers
	LPEXCEPTION_POINTERS lpEP = (LPEXCEPTION_POINTERS)lpExceptionPointer;
	
	//if not an access violation then do not try to handle it
	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) { return EXCEPTION_CONTINUE_SEARCH; }

	//if Memory Address is greater then base memory then do not handle exception
	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)RDRAM;
    if ((int)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF) { return EXCEPTION_CONTINUE_SEARCH; }

	//Get where the code was executing from
	BYTE * ExecPos = (unsigned char *)lpEP->ContextRecord->Eip;

	//Handle if the exception occured from a DMA
	if (*ExecPos == 0xF3 && *(ExecPos + 1) == 0xA5) {
//		CRecompiler * Recomp = _System->GetRecompiler();
//		if (!Recomp) { _Notify->BreakPoint(__FILE__,__LINE__); }
		int Start = (lpEP->ContextRecord->Edi - (DWORD)RDRAM);
		int End = (Start + (lpEP->ContextRecord->Ecx << 2) - 1);
		
		if ((int)Start < 0) {  _Notify->BreakPoint(__FILE__,__LINE__); }
		if ((int)End < _Settings->LoadDword(RamSize)) {
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

bool CMipsMemory::SearchForValue (DWORD Value, MemorySize Size, DWORD &StartAddress, DWORD &Len )
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
				if (*(DWORD *)(RDRAM + pos) == Value)
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
				if (*(WORD *)(RDRAM + (pos ^ 2)) == (WORD)Value)
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
				if (*(BYTE *)(RDRAM + (pos ^ 3)) == (BYTE)Value)
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

bool CMipsMemory::SearchSetBaseForChanges ( void )
{
	if (m_MemoryState == NULL)
	{
		delete [] m_MemoryState;
	}
	m_MemoryStateSize = RdramSize();
	m_MemoryState = new BYTE[m_MemoryStateSize];
	memcpy(m_MemoryState,RDRAM,m_MemoryStateSize);
	return true;
}

bool CMipsMemory::SearchForChanges(SearchMemChangeState SearchType, MemorySize Size, 
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
				NewValue = *(DWORD *)(RDRAM + pos);
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
				NewValue = *(WORD *)(RDRAM + (pos ^ 2));
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
				NewValue = *(BYTE *)(RDRAM + (pos ^ 3));
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
