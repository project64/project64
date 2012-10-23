#include "stdafx.h"

DWORD RegModValue;

CMipsMemoryVM::CMipsMemoryVM ( CMipsMemory_CallBack * CallBack, bool SavesReadOnly ) :
	
	CPifRam(SavesReadOnly),
	CFlashram(SavesReadOnly),
	CSram(SavesReadOnly),
	CDMA(*this,*this),
	m_CBClass(CallBack),
	m_TLB_ReadMap(NULL),
	m_TLB_WriteMap(NULL),
	m_RomMapped(false),
	m_Rom(NULL),
	m_RomSize(0),
	m_RomWrittenTo(false),
	m_RomWroteValue(0)
{ 
	m_RDRAM      = NULL;
	m_DMEM       = NULL;
	m_IMEM       = NULL;
	m_HalfLine      = 0;
}

CMipsMemoryVM::~CMipsMemoryVM (void) 
{
	FreeMemory();
}

void CMipsMemoryVM::Reset( bool /*EraseMemory*/ )
{
	if (m_TLB_ReadMap)
	{
		memset(m_TLB_ReadMap,0,(0xFFFFF * sizeof(DWORD)));
		memset(m_TLB_WriteMap,0,(0xFFFFF * sizeof(DWORD)));
		for (DWORD address = 0x80000000; address < 0xC0000000; address += 0x1000) 
		{
			m_TLB_ReadMap[address >> 12] = ((DWORD)m_RDRAM + (address & 0x1FFFFFFF)) - address;
			m_TLB_WriteMap[address >> 12] = ((DWORD)m_RDRAM + (address & 0x1FFFFFFF)) - address;
		}
		
		if (_Settings->LoadDword(Rdb_TLB_VAddrStart) != 0)
		{
			DWORD Start = _Settings->LoadDword(Rdb_TLB_VAddrStart); //0x7F000000;
			DWORD Len   = _Settings->LoadDword(Rdb_TLB_VAddrLen);   //0x01000000;
			DWORD PAddr = _Settings->LoadDword(Rdb_TLB_PAddrStart); //0x10034b30;
			DWORD End   = Start + Len;
			for (DWORD address = Start; address < End; address += 0x1000) {
				m_TLB_ReadMap[address >> 12] = ((DWORD)m_RDRAM + (address - Start + PAddr)) - address;
				m_TLB_WriteMap[address >> 12] = ((DWORD)m_RDRAM + (address - Start + PAddr)) - address;
			}
		}
	}
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
	CPifRam::Reset();

	m_TLB_ReadMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (m_TLB_ReadMap == NULL) 
	{
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate m_TLB_ReadMap (Size: 0x%X)",0xFFFFF * sizeof(DWORD));
		FreeMemory();
		return false;
	}

	m_TLB_WriteMap = (DWORD *)VirtualAlloc(NULL,0xFFFFF * sizeof(DWORD),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (m_TLB_WriteMap == NULL) 
	{
		WriteTraceF(TraceError,"CMipsMemoryVM::Initialize:: Failed to Allocate m_TLB_ReadMap (Size: 0x%X)",0xFFFFF * sizeof(DWORD));
		FreeMemory();
		return false;
	}
	Reset(false);
	_Settings->RegisterChangeCB(Game_RDRamSize,this,(CSettings::SettingChangedFunc)RdramChanged);

	return true;
}

void CMipsMemoryVM::FreeMemory ( void )
{
	_Settings->UnregisterChangeCB(Game_RDRamSize,this,(CSettings::SettingChangedFunc)RdramChanged);

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
	CPifRam::Reset();
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
	return m_PifRam;
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
//		_Notify->BreakPoint(__FILE__,__LINE__);
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
			SW_NonMemory(VAddr,Value);
			return true;
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

bool CMipsMemoryVM::VAddrToRealAddr ( DWORD VAddr, void * &RealAddress ) const
{
	if (m_TLB_ReadMap[VAddr >> 12] == 0) { return false; }
	RealAddress = (BYTE *)(m_TLB_ReadMap[VAddr >> 12] + VAddr);
	return true;
}

bool CMipsMemoryVM::TranslateVaddr ( DWORD VAddr, DWORD &PAddr) const 
{
	//Change the Virtual address to a Phyiscal Address
	if (m_TLB_ReadMap[VAddr >> 12] == 0) { return false; }
	PAddr = (DWORD)((BYTE *)(m_TLB_ReadMap[VAddr >> 12] + VAddr) - m_RDRAM);
	return true;
}

void  CMipsMemoryVM::Compile_LB ( x86Reg Reg, DWORD VAddr, BOOL SignExtend) {
	DWORD PAddr;
	char VarName[100];

	if (!TranslateVaddr(VAddr,PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LB\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LB\nFailed to translate address %X",VAddr); }
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
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LB\nFailed to compile address: %X",VAddr); }
	}
}

void  CMipsMemoryVM::Compile_LH ( x86Reg Reg, DWORD VAddr, BOOL SignExtend) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LH\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LH\nFailed to translate address %X",VAddr); }
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
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LHU\nFailed to compile address: %X",VAddr); }
	}
}

void  CMipsMemoryVM::Compile_LW (x86Reg Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		MoveConstToX86reg(0,Reg);
		CPU_Message("Compile_LW\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address %X",VAddr); }
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
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04100000:
		{
			static DWORD TempValue = 0;
			BeforeCallDirect(m_RegWorkingSet);
			PushImm32("TempValue",(DWORD)&TempValue);
			PushImm32(PAddr);
			MoveConstToX86reg((ULONG)((CMipsMemoryVM *)this),x86_ECX);
			Call_Direct(AddressOf(&CMipsMemoryVM::LW_NonMemory),"CMipsMemoryVM::LW_NonMemory");
			AfterCallDirect(m_RegWorkingSet);
			MoveVariableToX86reg(&TempValue,"TempValue",Reg);
		}
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: MoveVariableToX86reg(&_Reg->MI_MODE_REG,"MI_MODE_REG",Reg); break;
		case 0x04300004: MoveVariableToX86reg(&_Reg->MI_VERSION_REG,"MI_VERSION_REG",Reg); break;
		case 0x04300008: MoveVariableToX86reg(&_Reg->MI_INTR_REG,"MI_INTR_REG",Reg); break;
		case 0x0430000C: MoveVariableToX86reg(&_Reg->MI_INTR_MASK_REG,"MI_INTR_MASK_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400010:
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
			UpdateCounters(m_RegWorkingSet,false, true);
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)this,x86_ECX);
			Call_Direct(AddressOf(&CMipsMemoryVM::UpdateHalfLine),"CMipsMemoryVM::UpdateHalfLine");
			AfterCallDirect(m_RegWorkingSet);
			MoveVariableToX86reg(&m_HalfLine,"m_HalfLine",Reg);
			break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500004: 
			if (bFixedAudio())
			{
				m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
				UpdateCounters(m_RegWorkingSet,false, true);
				m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
				BeforeCallDirect(m_RegWorkingSet);
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);
				Call_Direct(AddressOf(&CAudio::GetLength),"CAudio::GetLength");
				MoveX86regToVariable(x86_EAX,&m_TempValue,"m_TempValue"); 
				AfterCallDirect(m_RegWorkingSet);
				MoveVariableToX86reg(&m_TempValue,"m_TempValue",Reg);
			} else {
				if (_Plugins->Audio()->ReadLength != NULL) {
					BeforeCallDirect(m_RegWorkingSet);
					Call_Direct(_Plugins->Audio()->ReadLength,"AiReadLength");
					MoveX86regToVariable(x86_EAX,&m_TempValue,"m_TempValue"); 
					AfterCallDirect(m_RegWorkingSet);
					MoveVariableToX86reg(&m_TempValue,"m_TempValue",Reg);
				} else {
					MoveConstToX86reg(0,Reg);
				}						
			}
			break;
		case 0x0450000C: 
			if (bFixedAudio())
			{
				BeforeCallDirect(m_RegWorkingSet);
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);
				Call_Direct(AddressOf(&CAudio::GetStatus),"GetStatus");
				MoveX86regToVariable(x86_EAX,&m_TempValue,"m_TempValue"); 
				AfterCallDirect(m_RegWorkingSet);
				MoveVariableToX86reg(&m_TempValue,"m_TempValue",Reg);
			} else {
				MoveVariableToX86reg(&_Reg->AI_STATUS_REG,"AI_STATUS_REG",Reg); 
			}
			break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
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
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x0470000C: MoveVariableToX86reg(&_Reg->RI_SELECT_REG,"RI_SELECT_REG",Reg); break;
		case 0x04700010: MoveVariableToX86reg(&_Reg->RI_REFRESH_REG,"RI_REFRESH_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: MoveVariableToX86reg(&_Reg->SI_STATUS_REG,"SI_STATUS_REG",Reg); break;
		default:
			MoveConstToX86reg(0,Reg);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); }
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
			_Notify->DisplayError("Compile_LW\nFailed to translate address: %X",VAddr); 
		}
	}
}

void  CMipsMemoryVM::Compile_SB_Const ( BYTE Value, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SB\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SB\nFailed to translate address %X",VAddr); }
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
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SB_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
}

void  CMipsMemoryVM::Compile_SB_Register ( x86Reg Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SB\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SB\nFailed to translate address %X",VAddr); }
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
		MoveX86regByteToVariable(Reg,PAddr + m_RDRAM,VarName); 
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SB_Register\ntrying to store in %X?",VAddr); }
	}
}

void  CMipsMemoryVM::Compile_SH_Const ( WORD Value, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SH\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SH\nFailed to translate address %X",VAddr); }
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
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SH_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
}

void CMipsMemoryVM::Compile_SH_Register ( x86Reg Reg, DWORD VAddr ) {
	char VarName[100];
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SH\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SH\nFailed to translate address %X",VAddr); }
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
		MoveX86regHalfToVariable(Reg,PAddr + m_RDRAM,VarName); 
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SH_Register\ntrying to store in %X?",PAddr); }
	}
}

void CMipsMemoryVM::Compile_SW_Const ( DWORD Value, DWORD VAddr ) {
	char VarName[100];
	BYTE * Jump;
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SW\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW\nFailed to translate address %X",VAddr); }
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
		case 0x03F00000: MoveConstToVariable(Value,&_Reg->RDRAM_CONFIG_REG,"RDRAM_CONFIG_REG"); break;
		case 0x03F00004: MoveConstToVariable(Value,&_Reg->RDRAM_DEVICE_ID_REG,"RDRAM_DEVICE_ID_REG"); break;
		case 0x03F00008: MoveConstToVariable(Value,&_Reg->RDRAM_DELAY_REG,"RDRAM_DELAY_REG"); break;
		case 0x03F0000C: MoveConstToVariable(Value,&_Reg->RDRAM_MODE_REG,"RDRAM_MODE_REG"); break;
		case 0x03F00010: MoveConstToVariable(Value,&_Reg->RDRAM_REF_INTERVAL_REG,"RDRAM_REF_INTERVAL_REG"); break;
		case 0x03F00014: MoveConstToVariable(Value,&_Reg->RDRAM_REF_ROW_REG,"RDRAM_REF_ROW_REG"); break;
		case 0x03F00018: MoveConstToVariable(Value,&_Reg->RDRAM_RAS_INTERVAL_REG,"RDRAM_RAS_INTERVAL_REG"); break;
		case 0x03F0001C: MoveConstToVariable(Value,&_Reg->RDRAM_MIN_INTERVAL_REG,"RDRAM_MIN_INTERVAL_REG"); break;
		case 0x03F00020: MoveConstToVariable(Value,&_Reg->RDRAM_ADDR_SELECT_REG,"RDRAM_ADDR_SELECT_REG"); break;
		case 0x03F00024: MoveConstToVariable(Value,&_Reg->RDRAM_DEVICE_MANUF_REG,"RDRAM_DEVICE_MANUF_REG"); break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04000000:
		if (PAddr < 0x04002000) { 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
			break;
		}
		switch (PAddr) {
		case 0x04040000: MoveConstToVariable(Value,&_Reg->SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG"); break;
		case 0x04040004: MoveConstToVariable(Value,&_Reg->SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG"); break;
		case 0x04040008:
			MoveConstToVariable(Value,&_Reg->SP_RD_LEN_REG,"SP_RD_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::SP_DMA_READ),"CDMA::SP_DMA_READ");
			AfterCallDirect(m_RegWorkingSet);
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
					AndConstToVariable(~ModValue,&_Reg->SP_STATUS_REG,"SP_STATUS_REG");
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
					OrConstToVariable(ModValue,&_Reg->SP_STATUS_REG,"SP_STATUS_REG");
				}
				if ( ( Value & SP_SET_SIG0 ) != 0 && _Settings->LoadBool(Game_RspAudioSignal) ) 
				{ 
					OrConstToVariable(MI_INTR_SP,&_Reg->MI_INTR_REG,"MI_INTR_REG");
					BeforeCallDirect(m_RegWorkingSet);
					MoveConstToX86reg((DWORD)_Reg,x86_ECX);
					Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
					AfterCallDirect(m_RegWorkingSet);
				}
				if ( ( Value & SP_CLR_INTR ) != 0) { 
					AndConstToVariable((DWORD)~MI_INTR_SP,&_Reg->MI_INTR_REG,"MI_INTR_REG");
					AndConstToVariable((DWORD)~MI_INTR_SP,&_Reg->m_RspIntrReg,"m_RspIntrReg");
					BeforeCallDirect(m_RegWorkingSet);
					MoveConstToX86reg((DWORD)_System,x86_ECX);
					Call_Direct(AddressOf(&CN64System::RunRSP),"CN64System::RunRSP");
					MoveConstToX86reg((DWORD)_Reg,x86_ECX);
					Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
					AfterCallDirect(m_RegWorkingSet);
				} else {
					BeforeCallDirect(m_RegWorkingSet);
					MoveConstToX86reg((DWORD)_System,x86_ECX);	
					Call_Direct(AddressOf(&CN64System::RunRSP),"CN64System::RunRSP");
					AfterCallDirect(m_RegWorkingSet);
				}
			}
			break;
		case 0x0404001C: MoveConstToVariable(0,&_Reg->SP_SEMAPHORE_REG,"SP_SEMAPHORE_REG"); break;
		case 0x04080000: MoveConstToVariable(Value & 0xFFC,&_Reg->SP_PC_REG,"SP_PC_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
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
					AndConstToVariable(~ModValue,&_Reg->MI_MODE_REG,"MI_MODE_REG");
				}

				ModValue = (Value & 0x7F);
				if ( ( Value & MI_SET_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_SET_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_SET_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&_Reg->MI_MODE_REG,"MI_MODE_REG");
				}
				if ( ( Value & MI_CLR_DP_INTR ) != 0 ) { 
					AndConstToVariable((DWORD)~MI_INTR_DP,&_Reg->MI_INTR_REG,"MI_INTR_REG");
					AndConstToVariable((DWORD)~MI_INTR_DP,&_Reg->m_GfxIntrReg,"m_GfxIntrReg");
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
					AndConstToVariable(~ModValue,&_Reg->MI_INTR_MASK_REG,"MI_INTR_MASK_REG");
				}

				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&_Reg->MI_INTR_MASK_REG,"MI_INTR_MASK_REG");
				}
			}
			break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (_Plugins->Gfx()->ViStatusChanged != NULL) {
				CompConstToVariable(Value,&_Reg->VI_STATUS_REG,"VI_STATUS_REG");
				JeLabel8("Continue",0);
				Jump = m_RecompPos - 1;
				MoveConstToVariable(Value,&_Reg->VI_STATUS_REG,"VI_STATUS_REG");
				BeforeCallDirect(m_RegWorkingSet);
				Call_Direct(_Plugins->Gfx()->ViStatusChanged,"ViStatusChanged");
				AfterCallDirect(m_RegWorkingSet);
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			}
			break;
		case 0x04400004: MoveConstToVariable((Value & 0xFFFFFF),&_Reg->VI_ORIGIN_REG,"VI_ORIGIN_REG"); break;
		case 0x04400008: 
			if (_Plugins->Gfx()->ViWidthChanged != NULL) {
				CompConstToVariable(Value,&_Reg->VI_WIDTH_REG,"VI_WIDTH_REG");
				JeLabel8("Continue",0);
				Jump = m_RecompPos - 1;
				MoveConstToVariable(Value,&_Reg->VI_WIDTH_REG,"VI_WIDTH_REG");
				BeforeCallDirect(m_RegWorkingSet);
				Call_Direct(_Plugins->Gfx()->ViWidthChanged,"ViWidthChanged");
				AfterCallDirect(m_RegWorkingSet);
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			}
			break;
		case 0x0440000C: MoveConstToVariable(Value,&_Reg->VI_INTR_REG,"VI_INTR_REG"); break;
		case 0x04400010: 
			AndConstToVariable((DWORD)~MI_INTR_VI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04400014: MoveConstToVariable(Value,&_Reg->VI_BURST_REG,"VI_BURST_REG"); break;
		case 0x04400018: MoveConstToVariable(Value,&_Reg->VI_V_SYNC_REG,"VI_V_SYNC_REG"); break;
		case 0x0440001C: MoveConstToVariable(Value,&_Reg->VI_H_SYNC_REG,"VI_H_SYNC_REG"); break;
		case 0x04400020: MoveConstToVariable(Value,&_Reg->VI_LEAP_REG,"VI_LEAP_REG"); break;
		case 0x04400024: MoveConstToVariable(Value,&_Reg->VI_H_START_REG,"VI_H_START_REG"); break;
		case 0x04400028: MoveConstToVariable(Value,&_Reg->VI_V_START_REG,"VI_V_START_REG"); break;
		case 0x0440002C: MoveConstToVariable(Value,&_Reg->VI_V_BURST_REG,"VI_V_BURST_REG"); break;
		case 0x04400030: MoveConstToVariable(Value,&_Reg->VI_X_SCALE_REG,"VI_X_SCALE_REG"); break;
		case 0x04400034: MoveConstToVariable(Value,&_Reg->VI_Y_SCALE_REG,"VI_Y_SCALE_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500000: MoveConstToVariable(Value,&_Reg->AI_DRAM_ADDR_REG,"AI_DRAM_ADDR_REG"); break;
		case 0x04500004: 
			MoveConstToVariable(Value,&_Reg->AI_LEN_REG,"AI_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			if (bFixedAudio())
			{
				X86BreakPoint(__FILE__,__LINE__);
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);				
				Call_Direct(AddressOf(&CAudio::LenChanged),"LenChanged");
			} else {
				Call_Direct(_Plugins->Audio()->LenChanged,"AiLenChanged");
			}
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04500008: MoveConstToVariable((Value & 1),&_Reg->AI_CONTROL_REG,"AI_CONTROL_REG"); break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			AndConstToVariable((DWORD)~MI_INTR_AI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			if (!bFixedAudio())
			{
				AndConstToVariable((DWORD)~MI_INTR_AI,&_Reg->m_AudioIntrReg,"m_AudioIntrReg");
			}
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04500010: 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
			break;
		case 0x04500014: MoveConstToVariable(Value,&_Reg->AI_BITRATE_REG,"AI_BITRATE_REG"); break;
		default:
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveConstToVariable(Value,PAddr + m_RDRAM,VarName); 
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: MoveConstToVariable(Value,&_Reg->PI_DRAM_ADDR_REG,"PI_DRAM_ADDR_REG"); break;
		case 0x04600004: MoveConstToVariable(Value,&_Reg->PI_CART_ADDR_REG,"PI_CART_ADDR_REG"); break;
		case 0x04600008: 
			MoveConstToVariable(Value,&_Reg->PI_RD_LEN_REG,"PI_RD_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::PI_DMA_READ),"CDMA::PI_DMA_READ");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x0460000C:
			MoveConstToVariable(Value,&_Reg->PI_WR_LEN_REG,"PI_WR_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::PI_DMA_WRITE),"CDMA::PI_DMA_WRITE");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04600010: 
			if ((Value & PI_CLR_INTR) != 0 ) {
				AndConstToVariable((DWORD)~MI_INTR_PI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
				BeforeCallDirect(m_RegWorkingSet);
				MoveConstToX86reg((DWORD)_Reg,x86_ECX);
				Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
				AfterCallDirect(m_RegWorkingSet);
			}
			break;
		case 0x04600014: MoveConstToVariable((Value & 0xFF),&_Reg->PI_DOMAIN1_REG,"PI_DOMAIN1_REG"); break;
		case 0x04600018: MoveConstToVariable((Value & 0xFF),&_Reg->PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); break;
		case 0x0460001C: MoveConstToVariable((Value & 0xFF),&_Reg->PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); break;
		case 0x04600020: MoveConstToVariable((Value & 0xFF),&_Reg->PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: MoveConstToVariable(Value,&_Reg->RI_MODE_REG,"RI_MODE_REG"); break;
		case 0x04700004: MoveConstToVariable(Value,&_Reg->RI_CONFIG_REG,"RI_CONFIG_REG"); break;
		case 0x04700008: MoveConstToVariable(Value,&_Reg->RI_CURRENT_LOAD_REG,"RI_CURRENT_LOAD_REG"); break;
		case 0x0470000C: MoveConstToVariable(Value,&_Reg->RI_SELECT_REG,"RI_SELECT_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: MoveConstToVariable(Value,&_Reg->SI_DRAM_ADDR_REG,"SI_DRAM_ADDR_REG"); break;
		case 0x04800004: 			
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
			UpdateCounters(m_RegWorkingSet,false, true);
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
			MoveConstToVariable(Value,&_Reg->SI_PIF_ADDR_RD64B_REG,"SI_PIF_ADDR_RD64B_REG");		
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)((CPifRam *)this),x86_ECX);
			Call_Direct(AddressOf(&CPifRam::SI_DMA_READ),"CPifRam::SI_DMA_READ");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04800010: 
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
			UpdateCounters(m_RegWorkingSet,false, true);
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
			MoveConstToVariable(Value,&_Reg->SI_PIF_ADDR_WR64B_REG,"SI_PIF_ADDR_WR64B_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)((CPifRam *)this),x86_ECX);
			Call_Direct(AddressOf(&CPifRam::SI_DMA_WRITE),"CPifRam::SI_DMA_WRITE");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04800018: 
			AndConstToVariable((DWORD)~MI_INTR_SI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable((DWORD)~SI_STATUS_INTERRUPT,&_Reg->SI_STATUS_REG,"SI_STATUS_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
		}
		break;
	default:
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Const\ntrying to store %X in %X?",Value,VAddr); }
	}
}

void CMipsMemoryVM::Compile_SW_Register (x86Reg Reg, DWORD VAddr ) 
{
	char VarName[100];
	BYTE * Jump;
	DWORD PAddr;

	if (!TranslateVaddr(VAddr, PAddr)) {
		CPU_Message("Compile_SW_Register\nFailed to translate address %X",VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\nFailed to translate address %X",VAddr); }
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
		MoveX86regToVariable(Reg,PAddr + m_RDRAM,VarName); 
		break;
	case 0x04000000: 
		switch (PAddr) {
		case 0x04040000: MoveX86regToVariable(Reg,&_Reg->SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG"); break;
		case 0x04040004: MoveX86regToVariable(Reg,&_Reg->SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG"); break;
		case 0x04040008: 
			MoveX86regToVariable(Reg,&_Reg->SP_RD_LEN_REG,"SP_RD_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::SP_DMA_READ),"CDMA::SP_DMA_READ");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x0404000C: 
			MoveX86regToVariable(Reg,&_Reg->SP_WR_LEN_REG,"SP_WR_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::SP_DMA_WRITE),"CDMA::SP_DMA_WRITE");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04040010: 
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
			UpdateCounters(m_RegWorkingSet,false, true);
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
			MoveX86regToVariable(Reg,&RegModValue,"RegModValue");
			BeforeCallDirect(m_RegWorkingSet);
			Call_Direct(ChangeSpStatus,"ChangeSpStatus");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x0404001C: MoveConstToVariable(0,&_Reg->SP_SEMAPHORE_REG,"SP_SEMAPHORE_REG"); break;
		case 0x04080000: 
			MoveX86regToVariable(Reg,&_Reg->SP_PC_REG,"SP_PC_REG");
			AndConstToVariable(0xFFC,&_Reg->SP_PC_REG,"SP_PC_REG");
			break;
		default:
			if (PAddr < 0x04002000) {
				sprintf(VarName,"m_RDRAM + %X",PAddr);
				MoveX86regToVariable(Reg,PAddr + m_RDRAM,VarName); 
			} else {
				CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(Reg),VAddr);
				if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
			}
		}
		break;
	case 0x04100000: 
		if (PAddr == 0x0410000C)
		{
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount()-CountPerOp());
			UpdateCounters(m_RegWorkingSet,false,true);
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount()+CountPerOp());
		}
		BeforeCallDirect(m_RegWorkingSet);
		Push(Reg);
		PushImm32(PAddr);
		MoveConstToX86reg((ULONG)((CMipsMemoryVM *)this),x86_ECX);
		Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory),"CMipsMemoryVM::SW_NonMemory");
		AfterCallDirect(m_RegWorkingSet);
		break;
	case 0x04300000: 
		switch (PAddr) {
		case 0x04300000: 
			MoveX86regToVariable(Reg,&RegModValue,"RegModValue");
			BeforeCallDirect(m_RegWorkingSet);
			Call_Direct(ChangeMiIntrMask,"ChangeMiModeReg");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x0430000C: 
			MoveX86regToVariable(Reg,&RegModValue,"RegModValue");
			BeforeCallDirect(m_RegWorkingSet);
			Call_Direct(ChangeMiIntrMask,"ChangeMiIntrMask");
			AfterCallDirect(m_RegWorkingSet);
			break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(Reg),VAddr);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04400000: 
		switch (PAddr) {
		case 0x04400000: 
			if (_Plugins->Gfx()->ViStatusChanged != NULL) {
				CompX86regToVariable(Reg,&_Reg->VI_STATUS_REG,"VI_STATUS_REG");
				JeLabel8("Continue",0);
				Jump = m_RecompPos - 1;
				MoveX86regToVariable(Reg,&_Reg->VI_STATUS_REG,"VI_STATUS_REG");
				BeforeCallDirect(m_RegWorkingSet);
				Call_Direct(_Plugins->Gfx()->ViStatusChanged,"ViStatusChanged");
				AfterCallDirect(m_RegWorkingSet);
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			}
			break;
		case 0x04400004: 
			MoveX86regToVariable(Reg,&_Reg->VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			AndConstToVariable(0xFFFFFF,&_Reg->VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			break;
		case 0x04400008: 
			if (_Plugins->Gfx()->ViWidthChanged != NULL) {
				CompX86regToVariable(Reg,&_Reg->VI_WIDTH_REG,"VI_WIDTH_REG");
				JeLabel8("Continue",0);
				Jump = m_RecompPos - 1;
				MoveX86regToVariable(Reg,&_Reg->VI_WIDTH_REG,"VI_WIDTH_REG");
				BeforeCallDirect(m_RegWorkingSet);
				Call_Direct(_Plugins->Gfx()->ViWidthChanged,"ViWidthChanged");
				AfterCallDirect(m_RegWorkingSet);
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			}
			break;
		case 0x0440000C: MoveX86regToVariable(Reg,&_Reg->VI_INTR_REG,"VI_INTR_REG"); break;
		case 0x04400010: 
			AndConstToVariable((DWORD)~MI_INTR_VI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04400014: MoveX86regToVariable(Reg,&_Reg->VI_BURST_REG,"VI_BURST_REG"); break;
		case 0x04400018: MoveX86regToVariable(Reg,&_Reg->VI_V_SYNC_REG,"VI_V_SYNC_REG"); break;
		case 0x0440001C: MoveX86regToVariable(Reg,&_Reg->VI_H_SYNC_REG,"VI_H_SYNC_REG"); break;
		case 0x04400020: MoveX86regToVariable(Reg,&_Reg->VI_LEAP_REG,"VI_LEAP_REG"); break;
		case 0x04400024: MoveX86regToVariable(Reg,&_Reg->VI_H_START_REG,"VI_H_START_REG"); break;
		case 0x04400028: MoveX86regToVariable(Reg,&_Reg->VI_V_START_REG,"VI_V_START_REG"); break;
		case 0x0440002C: MoveX86regToVariable(Reg,&_Reg->VI_V_BURST_REG,"VI_V_BURST_REG"); break;
		case 0x04400030: MoveX86regToVariable(Reg,&_Reg->VI_X_SCALE_REG,"VI_X_SCALE_REG"); break;
		case 0x04400034: MoveX86regToVariable(Reg,&_Reg->VI_Y_SCALE_REG,"VI_Y_SCALE_REG"); break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(Reg),VAddr);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04500000: /* AI registers */
		switch (PAddr) {
		case 0x04500000: MoveX86regToVariable(Reg,&_Reg->AI_DRAM_ADDR_REG,"AI_DRAM_ADDR_REG"); break;
		case 0x04500004: 
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
			UpdateCounters(m_RegWorkingSet,false, true);
			m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
			MoveX86regToVariable(Reg,&_Reg->AI_LEN_REG,"AI_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			if (bFixedAudio())
			{
				MoveConstToX86reg((DWORD)_Audio,x86_ECX);				
				Call_Direct(AddressOf(&CAudio::LenChanged),"LenChanged");
			} else {
				Call_Direct(_Plugins->Audio()->LenChanged,"_Plugins->Audio()->LenChanged");
			}
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04500008: 
			MoveX86regToVariable(Reg,&_Reg->AI_CONTROL_REG,"AI_CONTROL_REG");
			AndConstToVariable(1,&_Reg->AI_CONTROL_REG,"AI_CONTROL_REG");
		case 0x0450000C:
			/* Clear Interrupt */; 
			AndConstToVariable((DWORD)~MI_INTR_AI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			if (!bFixedAudio())
			{
				AndConstToVariable((DWORD)~MI_INTR_AI,&_Reg->m_AudioIntrReg,"m_AudioIntrReg");
			}
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04500010: 
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveX86regToVariable(Reg,PAddr + m_RDRAM,VarName); 
			break;
		case 0x04500014: MoveX86regToVariable(Reg,&_Reg->AI_BITRATE_REG,"AI_BITRATE_REG"); break;
		default:
			sprintf(VarName,"m_RDRAM + %X",PAddr);
			MoveX86regToVariable(Reg,PAddr + m_RDRAM,VarName); 
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: MoveX86regToVariable(Reg,&_Reg->PI_DRAM_ADDR_REG,"PI_DRAM_ADDR_REG"); break;
		case 0x04600004: MoveX86regToVariable(Reg,&_Reg->PI_CART_ADDR_REG,"PI_CART_ADDR_REG"); break;
		case 0x04600008:
			MoveX86regToVariable(Reg,&_Reg->PI_RD_LEN_REG,"PI_RD_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::PI_DMA_READ),"CDMA::PI_DMA_READ");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x0460000C:
			MoveX86regToVariable(Reg,&_Reg->PI_WR_LEN_REG,"PI_WR_LEN_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((ULONG)((CDMA *)this),x86_ECX);
			Call_Direct(AddressOf(&CDMA::PI_DMA_WRITE),"CDMA::PI_DMA_WRITE");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04600010: 
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
			AndConstToVariable((DWORD)~MI_INTR_PI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
			MoveX86regToVariable(Reg,&_Reg->VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
			AndConstToVariable(0xFFFFFF,&_Reg->VI_ORIGIN_REG,"VI_ORIGIN_REG"); 
		case 0x04600014: 
			MoveX86regToVariable(Reg,&_Reg->PI_DOMAIN1_REG,"PI_DOMAIN1_REG");
			AndConstToVariable(0xFF,&_Reg->PI_DOMAIN1_REG,"PI_DOMAIN1_REG"); 
			break;
		case 0x04600018: 
			MoveX86regToVariable(Reg,&_Reg->PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); 
			AndConstToVariable(0xFF,&_Reg->PI_BSD_DOM1_PWD_REG,"PI_BSD_DOM1_PWD_REG"); 
			break;
		case 0x0460001C: 
			MoveX86regToVariable(Reg,&_Reg->PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); 
			AndConstToVariable(0xFF,&_Reg->PI_BSD_DOM1_PGS_REG,"PI_BSD_DOM1_PGS_REG"); 
			break;
		case 0x04600020: 
			MoveX86regToVariable(Reg,&_Reg->PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); 
			AndConstToVariable(0xFF,&_Reg->PI_BSD_DOM1_RLS_REG,"PI_BSD_DOM1_RLS_REG"); 
			break;
		default:
			CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(Reg),VAddr);
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700010: MoveX86regToVariable(Reg,&_Reg->RI_REFRESH_REG,"RI_REFRESH_REG"); break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: MoveX86regToVariable(Reg,&_Reg->SI_DRAM_ADDR_REG,"SI_DRAM_ADDR_REG"); break;
		case 0x04800004: 
			MoveX86regToVariable(Reg,&_Reg->SI_PIF_ADDR_RD64B_REG,"SI_PIF_ADDR_RD64B_REG"); 
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)((CPifRam *)this),x86_ECX);
			Call_Direct(AddressOf(&CPifRam::SI_DMA_READ),"CPifRam::SI_DMA_READ");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04800010: 
			MoveX86regToVariable(Reg,&_Reg->SI_PIF_ADDR_WR64B_REG,"SI_PIF_ADDR_WR64B_REG"); 
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)((CPifRam *)this),x86_ECX);
			Call_Direct(AddressOf(&CPifRam::SI_DMA_WRITE),"CPifRam::SI_DMA_WRITE");
			AfterCallDirect(m_RegWorkingSet);
			break;
		case 0x04800018: 
			AndConstToVariable((DWORD)~MI_INTR_SI,&_Reg->MI_INTR_REG,"MI_INTR_REG");
			AndConstToVariable((DWORD)~SI_STATUS_INTERRUPT,&_Reg->SI_STATUS_REG,"SI_STATUS_REG");
			BeforeCallDirect(m_RegWorkingSet);
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			AfterCallDirect(m_RegWorkingSet);
			break;
		default:
			if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store at %X?",VAddr); }
		}
		break;
	case 0x1FC00000:
		sprintf(VarName,"m_RDRAM + %X",PAddr);
		MoveX86regToVariable(Reg,PAddr + m_RDRAM,VarName); 
		break;
	default:
		CPU_Message("    Should be moving %s in to %X ?!?",x86_Name(Reg),VAddr);
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("Compile_SW_Register\ntrying to store in %X?",VAddr); }
	}
}

void CMipsMemoryVM::ResetMemoryStack ( void) 
{
	x86Reg Reg, TempReg;

	int MipsReg = 29;
	CPU_Message("    ResetMemoryStack");
	Reg = Get_MemoryStack();
	if (Reg == x86_Unknown) 
	{
		Reg = Map_TempReg(x86_Any, MipsReg, FALSE);
	} else {
		if (IsUnknown(MipsReg)) {
			MoveVariableToX86reg(&_GPR[MipsReg].UW[0],CRegName::GPR_Lo[MipsReg],Reg);
		} else if (IsMapped(MipsReg)) {
			MoveX86RegToX86Reg(MipsRegMapLo(MipsReg),Reg);
		} else {
			MoveConstToX86reg(MipsRegLo(MipsReg),Reg);
		}
	}

	if (bUseTlb()) 
	{	
	    TempReg = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(Reg,TempReg);
		ShiftRightUnsignImmed(TempReg,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg,TempReg,4);
		AddX86RegToX86Reg(Reg,TempReg);
	} else {
		AndConstToX86Reg(Reg,0x1FFFFFFF);
		AddConstToX86Reg(Reg,(DWORD)m_RDRAM);
	}
	MoveX86regToVariable(Reg,&(_Recompiler->MemoryStackPos()), "MemoryStack");
}

int CMipsMemoryVM::MemoryFilter( DWORD dwExptCode, void * lpExceptionPointer ) 
{
	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) 
	{
		if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
		return EXCEPTION_EXECUTE_HANDLER;
	}

	//convert the pointer since we are not having win32 stuctures in headers
	LPEXCEPTION_POINTERS lpEP = (LPEXCEPTION_POINTERS)lpExceptionPointer;

	DWORD MemAddress = (char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)_MMU->Rdram();
    if ((int)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF) 
	{ 
//		if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
		return EXCEPTION_EXECUTE_HANDLER; 
	}

	DWORD * Reg = NULL;
	
	BYTE * TypePos = (unsigned char *)lpEP->ContextRecord->Eip;
	EXCEPTION_RECORD exRec = *lpEP->ExceptionRecord;
	
	if (*TypePos == 0xF3 && *(TypePos + 1) == 0xA5) {
		DWORD Start, End;
		Start = (lpEP->ContextRecord->Edi - (DWORD)m_RDRAM);
		End = (Start + (lpEP->ContextRecord->Ecx << 2) - 1);
		if ((int)Start < 0) 
		{ 
			if (bHaveDebugger()) {
				_Notify->BreakPoint(__FILE__,__LINE__); 
			}
			return EXCEPTION_EXECUTE_HANDLER;
		}
#ifdef CFB_READ
		DWORD count, OldProtect;
		if (Start >= CFBStart && End < CFBEnd) {
			for ( count = Start; count < End; count += 0x1000 ) {
				VirtualProtect(m_RDRAM+count,4,PAGE_READONLY, &OldProtect);
				if (FrameBufferRead) { FrameBufferRead(count & ~0xFFF); }
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}	
#endif
		if (End < RdramSize()) 
		{
			for (DWORD count = (Start & ~0xFFF); count < End; count += 0x1000 ) 
			{				
				_Recompiler->ClearRecompCode_Phys(count,0x1000,CRecompiler::Remove_ProtectedMem);
			}			
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04000000 && End < 0x04002000) {
			_Recompiler->ClearRecompCode_Phys(Start & ~0xFFF,0x1000,CRecompiler::Remove_ProtectedMem);
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
		return EXCEPTION_EXECUTE_HANDLER;
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
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
		break;
	case 5: ReadPos += 5; break;
	case 6: ReadPos += 1; break;
	case 7: ReadPos += 1; break;
	case 0x40: ReadPos += 2; break;
	case 0x41: ReadPos += 2; break;
	case 0x42: ReadPos += 2; break;
	case 0x43: ReadPos += 2; break;
	case 0x44: ReadPos += 3; break;
	case 0x46: ReadPos += 2; break;
	case 0x47: ReadPos += 2; break;
	case 0x80: ReadPos += 5; break;
	case 0x81: ReadPos += 5; break;
	case 0x82: ReadPos += 5; break;
	case 0x83: ReadPos += 5; break;
	case 0x86: ReadPos += 5; break;
	case 0x87: ReadPos += 5; break;
	default:
		if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
		return EXCEPTION_EXECUTE_HANDLER;
	}

	if (Reg == NULL)
	{
		if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
		return EXCEPTION_EXECUTE_HANDLER;
	}

	switch(*TypePos) {
	case 0x0F:
		switch(*(TypePos + 1)) {
		case 0xB6:
			if (!LB_NonMemory(MemAddress,(DWORD *)Reg,FALSE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xB7:
			if (!LH_NonMemory(MemAddress,(DWORD *)Reg,FALSE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to load half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xBE:
			if (!LB_NonMemory(MemAddress,Reg,TRUE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xBF:
			if (!LH_NonMemory(MemAddress,Reg,TRUE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to load half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		default:
			if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
			return EXCEPTION_EXECUTE_HANDLER;
		}
		break;
	case 0x66:
		switch(*(TypePos + 1)) {
		case 0x8B:
			if (!LH_NonMemory(MemAddress,Reg,FALSE)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to half word\n\nMIPS Address: %X\nX86 Address",
						(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0x89:
			if (!SH_NonMemory(MemAddress,*(WORD *)Reg)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to store half word\n\nMIPS Address: %X\nX86 Address",MemAddress,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;		
		case 0xC7:
			if (Reg != &lpEP->ContextRecord->Eax)
			{
				if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
				return EXCEPTION_EXECUTE_HANDLER; 
			}
			if (!SH_NonMemory(MemAddress,*(WORD *)ReadPos)) {
				if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
					_Notify->DisplayError("Failed to store half word\n\nMIPS Address: %X\nX86 Address",MemAddress,
						*(unsigned char *)lpEP->ContextRecord->Eip);
				}
			}
			lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 2);
			return EXCEPTION_CONTINUE_EXECUTION;		
		default:
			if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
			return EXCEPTION_EXECUTE_HANDLER;
		}
		break;
	case 0x88: 
		if (!SB_NonMemory(MemAddress,*(BYTE *)Reg)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				_Notify->DisplayError("Failed to store byte\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x8A: 
		if (!LB_NonMemory(MemAddress,Reg,FALSE)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				_Notify->DisplayError("Failed to load byte\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x8B: 
		if (!LW_NonMemory(MemAddress,Reg)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				_Notify->DisplayError("Failed to load word\n\nMIPS Address: %X\nX86 Address",
					(char *)exRec.ExceptionInformation[1] - (char *)m_RDRAM,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0x89:
		if (!SW_NonMemory(MemAddress,*(DWORD *)Reg)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				_Notify->DisplayError("Failed to store word\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC6:
		if (Reg != &lpEP->ContextRecord->Eax) 
		{
			if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
			return EXCEPTION_EXECUTE_HANDLER; 
		}
		if (!SB_NonMemory(MemAddress,*(BYTE *)ReadPos)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				_Notify->DisplayError("Failed to store byte\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 1);
		return EXCEPTION_CONTINUE_EXECUTION;		
	case 0xC7:
		if (Reg != &lpEP->ContextRecord->Eax) 
		{
			if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
			return EXCEPTION_EXECUTE_HANDLER; 
		}
		if (!SW_NonMemory(MemAddress,*(DWORD *)ReadPos)) {
			if (_Settings->LoadDword(Debugger_ShowUnhandledMemory)) {
				_Notify->DisplayError("Failed to store word\n\nMIPS Address: %X\nX86 Address",MemAddress,
					*(unsigned char *)lpEP->ContextRecord->Eip);
			}
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 4);
		return EXCEPTION_CONTINUE_EXECUTION;		
	default:
		if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
		return EXCEPTION_EXECUTE_HANDLER;
	}
	if (bHaveDebugger()) { _Notify->BreakPoint(__FILE__,__LINE__); }
	return EXCEPTION_EXECUTE_HANDLER;
}

int CMipsMemoryVM::LB_NonMemory ( DWORD /*PAddr*/, DWORD * Value, BOOL /*SignExtend*/ ) {
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

int CMipsMemoryVM::LH_NonMemory ( DWORD PAddr, DWORD * Value, int/* SignExtend*/ ) 
{
	if (PAddr < 0x800000)
	{
		* Value = 0;
		return true;
	}

	_Notify->BreakPoint(__FILE__,__LINE__);
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
			*Value = m_RomWroteValue;
			//LogMessage("%X: Read crap from Rom %X from %X",PROGRAM_COUNTER,*Value,PAddr);
			m_RomWrittenTo = FALSE;
#ifdef ROM_IN_MAPSPACE
			{
				DWORD OldProtect;
				VirtualProtect(ROM,RomFileSize,PAGE_READONLY, &OldProtect);
			}
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
			if (bFixedAudio())
			{
				*Value = _Audio->GetLength();
			} else {
				if (_Plugins->Audio()->ReadLength != NULL) {
					*Value = _Plugins->Audio()->ReadLength(); 
				} else {
					*Value = 0;
				}
			}
			break;
		case 0x0450000C: 
			if (bFixedAudio())
			{
				*Value = _Audio->GetStatus();
			} else {
				*Value = _Reg->AI_STATUS_REG; 
			}
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
		if (_System->m_SaveUsing == SaveChip_Auto) { _System->m_SaveUsing = SaveChip_FlashRam; }
		if (_System->m_SaveUsing != SaveChip_FlashRam) { 
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return FALSE;
		}
		*Value = ReadFromFlashStatus(PAddr);
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
			_Notify->BreakPoint(__FILE__,__LINE__);
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
		_Notify->BreakPoint(__FILE__,__LINE__);
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
			_Notify->DisplayError("FrameBufferWrite");
			if (FrameBufferWrite) { FrameBufferWrite(PAddr,1); }
			break;
		}	
#endif
		if (PAddr < RdramSize()) 
		{
			DWORD OldProtect;
			_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF,0x1000,CRecompiler::Remove_ProtectedMem);
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(BYTE *)(m_RDRAM+PAddr) = Value;
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
			_Notify->DisplayError("PAddr = %x",PAddr);
			break;
		}	
#endif
		if (PAddr < RdramSize()) {
			DWORD OldProtect;
			_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF,0x1000,CRecompiler::Remove_ProtectedMem);
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(WORD *)(m_RDRAM+PAddr) = Value;
		}
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

int CMipsMemoryVM::SW_NonMemory ( DWORD PAddr, DWORD Value ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) 
	{
		if ((PAddr - 0x10000000) < _Rom->GetRomSize()) {
			m_RomWrittenTo = TRUE;
			m_RomWroteValue = Value;
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
			_Notify->DisplayError("FrameBufferWrite %X",PAddr);
			if (FrameBufferWrite) { FrameBufferWrite(PAddr,4); }
			break;
		}	
#endif
		if (PAddr < RdramSize()) {
			DWORD OldProtect;
			_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF,0x1000,CRecompiler::Remove_ProtectedMem);
			VirtualProtect(m_RDRAM+(PAddr & ~0xFFF),0xFFC,PAGE_READWRITE, &OldProtect);
			*(DWORD *)(m_RDRAM+PAddr) = Value;
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
			_Recompiler->ClearRecompCode_Phys(PAddr & ~0xFFF,0xFFF,CRecompiler::Remove_ProtectedMem);
			*(DWORD *)(m_RDRAM+PAddr) = Value;
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
					_Reg->m_RspIntrReg &= ~MI_INTR_SP; 
					_Reg->CheckInterrupts();
				}
	#ifndef EXTERNAL_RELEASE
				if ( ( Value & SP_SET_INTR ) != 0) { _Notify->DisplayError("SP_SET_INTR"); }
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
					_Reg->CheckInterrupts();				
				}
#endif
				//if (*( DWORD *)(DMEM + 0xFC0) == 1) {
				//	ChangeTimer(RspTimer,0x30000);
				//} else {
					try {
						_System->RunRSP();
					} catch (...) {
						_Notify->BreakPoint(__FILE__,__LINE__);
					}
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
			if (_Plugins->Gfx()->ProcessRDPList) { _Plugins->Gfx()->ProcessRDPList(); }
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
						try {
							_System->RunRSP();
						} catch (...) {
							_Notify->BreakPoint(__FILE__,__LINE__);
						}
					}
				}
			}
#ifdef tofix
			if (ShowUnhandledMemory) {
				//if ( ( Value & DPC_CLR_TMEM_CTR ) != 0) { _Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_TMEM_CTR"); }
				//if ( ( Value & DPC_CLR_PIPE_CTR ) != 0) { _Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR"); }
				//if ( ( Value & DPC_CLR_CMD_CTR ) != 0) { _Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR"); }
				//if ( ( Value & DPC_CLR_CLOCK_CTR ) != 0) { _Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR"); }
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
				_Reg->m_GfxIntrReg &= ~MI_INTR_DP; 
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
			if (_Reg->VI_STATUS_REG != Value) { 
				_Reg->VI_STATUS_REG = Value; 
				if (_Plugins->Gfx()->ViStatusChanged != NULL ) { _Plugins->Gfx()->ViStatusChanged(); }
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
				if (_Plugins->Gfx()->ViWidthChanged != NULL ) { _Plugins->Gfx()->ViWidthChanged(); }
			}
			break;
		case 0x0440000C: _Reg->VI_INTR_REG = Value; break;
		case 0x04400010: 
			_Reg->MI_INTR_REG &= ~MI_INTR_VI;
			_Reg->CheckInterrupts();
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
			if (bFixedAudio())
			{
				_Audio->LenChanged();
			} else {
				if (_Plugins->Audio()->LenChanged != NULL) { _Plugins->Audio()->LenChanged(); }				
			}
			break;
		case 0x04500008: _Reg->AI_CONTROL_REG = (Value & 1); break;
		case 0x0450000C:
			/* Clear Interrupt */; 
			_Reg->MI_INTR_REG &= ~MI_INTR_AI;
			_Reg->m_AudioIntrReg &= ~MI_INTR_AI;
			_Reg->CheckInterrupts();
			break;
		case 0x04500010: 
			_Reg->AI_DACRATE_REG = Value;
			_Plugins->Audio()->DacrateChanged(_System->m_SystemType);
			if (bFixedAudio())
			{
				_Audio->SetFrequency(Value,_System->m_SystemType);
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
			//if ((Value & PI_SET_RESET) != 0 ) { _Notify->DisplayError("reset Controller"); }
			if ((Value & PI_CLR_INTR) != 0 ) {
				_Reg->MI_INTR_REG &= ~MI_INTR_PI;
				_Reg->CheckInterrupts();
			}
			break;
		case 0x04600014: _Reg->PI_DOMAIN1_REG = (Value & 0xFF); break; 
		case 0x04600018: _Reg->PI_BSD_DOM1_PWD_REG = (Value & 0xFF); break; 
		case 0x0460001C: _Reg->PI_BSD_DOM1_PGS_REG = (Value & 0xFF); break; 
		case 0x04600020: _Reg->PI_BSD_DOM1_RLS_REG = (Value & 0xFF); break; 
		case 0x04600024: _Reg->PI_DOMAIN2_REG = (Value & 0xFF); break;
		case 0x04600028: _Reg->PI_BSD_DOM2_PWD_REG = (Value & 0xFF); break;
		case 0x0460002C: _Reg->PI_BSD_DOM2_PGS_REG = (Value & 0xFF); break;
		case 0x04600030: _Reg->PI_BSD_DOM2_RLS_REG = (Value & 0xFF); break;
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
			_Reg->CheckInterrupts();
			break;
		default:
			return FALSE;
		}
		break;
	case 0x08000000:
		if (PAddr != 0x08010000) { return FALSE; }
		if (_System->m_SaveUsing == SaveChip_Auto) { _System->m_SaveUsing = SaveChip_FlashRam; }
		if (_System->m_SaveUsing != SaveChip_FlashRam) { return TRUE; }
		
		WriteToFlashCommand(Value);
		return TRUE;
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
			*(DWORD *)(&m_PifRam[PAddr - 0x1FC007C0]) = Value;
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

void CMipsMemoryVM::UpdateHalfLine (void)
{
    if (*_NextTimer < 0) { 
		m_HalfLine = 0;
		return;
	}
	m_HalfLine = (DWORD)(*_NextTimer / ViRefreshRate());
	m_HalfLine &= ~1;
}

void CMipsMemoryVM::ProtectMemory( DWORD StartVaddr, DWORD EndVaddr ) 
{
	WriteTraceF(TraceProtectedMem,"ProtectMemory: StartVaddr: %X EndVaddr: %X",StartVaddr,EndVaddr);
	if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr)) { return; }

	//Get Physical Addresses passed
	DWORD StartPAddr, EndPAddr;
	if (!TranslateVaddr(StartVaddr,StartPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	if (!TranslateVaddr(EndVaddr,EndPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	
	//Get Length of memory being protected
	int Length = ((EndPAddr + 3) - StartPAddr) & ~3;
	if (Length < 0) { _Notify->BreakPoint(__FILE__,__LINE__); }

	//Proect that memory address space
	DWORD OldProtect;
	BYTE * MemLoc = Rdram() + StartPAddr;
	WriteTraceF(TraceProtectedMem,"ProtectMemory: Paddr: %X Length: %X",StartPAddr,Length);
	
	VirtualProtect(MemLoc, Length, PAGE_READONLY, &OldProtect);	
}

void CMipsMemoryVM::UnProtectMemory( DWORD StartVaddr, DWORD EndVaddr ) 
{
	WriteTraceF(TraceProtectedMem,"UnProtectMemory: StartVaddr: %X EndVaddr: %X",StartVaddr,EndVaddr);
	if (!ValidVaddr(StartVaddr) || !ValidVaddr(EndVaddr)) { return; }

	//Get Physical Addresses passed
	DWORD StartPAddr, EndPAddr;
	if (!TranslateVaddr(StartVaddr,StartPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	if (!TranslateVaddr(EndVaddr,EndPAddr)) { _Notify->BreakPoint(__FILE__,__LINE__); }
	
	//Get Length of memory being protected
	int Length = ((EndPAddr + 3) - StartPAddr) & ~3;
	if (Length < 0) { _Notify->BreakPoint(__FILE__,__LINE__); }

	//Proect that memory address space
	DWORD OldProtect;
	BYTE * MemLoc = Rdram() + StartPAddr;
	
	VirtualProtect(MemLoc, Length, PAGE_READWRITE, &OldProtect);
}

void CMipsMemoryVM::Compile_LB (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(Opcode.rt,TRUE,0);
		Compile_LB(MipsRegMapLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		MoveSxByteX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regByte(MipsRegMapLo(Opcode.rt), TempReg1);
	}
}

void CMipsMemoryVM::Compile_LBU (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(Opcode.rt,FALSE,0);
		Compile_LB(MipsRegMapLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(Opcode.rt,FALSE,-1);
		MoveZxByteX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(Opcode.rt,FALSE,-1);
		MoveZxN64MemToX86regByte(MipsRegMapLo(Opcode.rt), TempReg1);
	}
}

void CMipsMemoryVM::Compile_LH (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(Opcode.rt,TRUE,0);
		Compile_LH(MipsRegMapLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		MoveSxHalfX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regHalf(MipsRegMapLo(Opcode.rt), TempReg1);
	}
}

void CMipsMemoryVM::Compile_LHU (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(Opcode.rt,FALSE,0);
		Compile_LH(MipsRegMapLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(MipsRegMapLo(Opcode.rt), TempReg1);
	}
}

void CMipsMemoryVM::Compile_LW (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	x86Reg TempReg1, TempReg2;
	if (Opcode.base == 29 && bFastSP()) {
		char String[100];

		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		TempReg1 = Map_MemoryStack(x86_Any,true);
		sprintf(String,"%Xh",(short)Opcode.offset);
		MoveVariableDispToX86Reg((void *)((DWORD)(short)Opcode.offset),String,MipsRegMapLo(Opcode.rt),TempReg1,1);
	} else {
		if (IsConst(Opcode.base)) { 
			DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
			Map_GPR_32bit(Opcode.rt,TRUE,-1);
			Compile_LW(MipsRegMapLo(Opcode.rt),Address);
		} else {
			if (bUseTlb()) {	
				if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
				if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
					ProtectGPR(Opcode.base);
					TempReg1 = MipsRegMapLo(Opcode.base);
				} else {
					if (IsMapped(Opcode.base)) { 
						ProtectGPR(Opcode.base);
						if (Opcode.offset != 0) {
							TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
							LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
						} else {
							TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
						}
					} else {
						TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
						AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
					}
				}
				TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
				MoveX86RegToX86Reg(TempReg1, TempReg2);
				ShiftRightUnsignImmed(TempReg2,12);
				MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
				CompileReadTLBMiss(TempReg1,TempReg2);
				Map_GPR_32bit(Opcode.rt,TRUE,-1);
				MoveX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt));
			} else {
				if (IsMapped(Opcode.base)) { 
					ProtectGPR(Opcode.base);
					if (Opcode.offset != 0) {
						Map_GPR_32bit(Opcode.rt,TRUE,-1);
						LeaSourceAndOffset(MipsRegMapLo(Opcode.rt),MipsRegMapLo(Opcode.base),(short)Opcode.offset);
					} else {
						Map_GPR_32bit(Opcode.rt,TRUE,Opcode.base);
					}
				} else {
					Map_GPR_32bit(Opcode.rt,TRUE,Opcode.base);
					AddConstToX86Reg(MipsRegMapLo(Opcode.rt),(short)Opcode.immediate);
				}
				AndConstToX86Reg(MipsRegMapLo(Opcode.rt),0x1FFFFFFF);
				MoveN64MemToX86reg(MipsRegMapLo(Opcode.rt),MipsRegMapLo(Opcode.rt));
			}
		}
	}
	if (bFastSP() && Opcode.rt == 29)
	{ 
		ResetX86Protection();
		ResetMemoryStack(); 
	}
}

void CMipsMemoryVM::Compile_LWC1 (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2, TempReg3;
	char Name[50];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	m_Section->CompileCop1Test();
	if ((Opcode.ft & 1) != 0) {
		if (RegInStack(Opcode.ft-1,CRegInfo::FPU_Double) || RegInStack(Opcode.ft-1,CRegInfo::FPU_Qword)) {
			UnMap_FPR(Opcode.ft-1,TRUE);
		}
	}
	if (RegInStack(Opcode.ft,CRegInfo::FPU_Double) || RegInStack(Opcode.ft,CRegInfo::FPU_Qword)) {
		UnMap_FPR(Opcode.ft,TRUE);
	} else {
		UnMap_FPR(Opcode.ft,FALSE);
	}
	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;

		TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
		Compile_LW(TempReg1,Address);

		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_S[Opcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg1,TempReg2);
		return;
	}
	if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
		if (bUseTlb()) {
			ProtectGPR(Opcode.base);
			TempReg1 = MipsRegMapLo(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			}
			UnProtectGPR(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
			}
		}
	}
	TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
	if (bUseTlb()) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		
		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg3);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		MoveN64MemToX86reg(TempReg3,TempReg1);
	}
	sprintf(Name,"_FPR_S[%d]",Opcode.ft);
	MoveVariableToX86reg(&_FPR_S[Opcode.ft],Name,TempReg2);
	MoveX86regToX86Pointer(TempReg3,TempReg2);
}

void CMipsMemoryVM::Compile_LWL (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1 = x86_Unknown, TempReg2 = x86_Unknown, OffsetReg = x86_Unknown, shift = x86_Unknown;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		DWORD Offset  = Address & 3;

		Map_GPR_32bit(Opcode.rt,TRUE,Opcode.rt);
		x86Reg Value = Map_TempReg(x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(MipsRegMapLo(Opcode.rt),LWL_MASK[Offset]);
		ShiftLeftSignImmed(Value,(BYTE)LWL_SHIFT[Offset]);
		AddX86RegToX86Reg(MipsRegMapLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(TempReg1,TempReg2);
	}
	OffsetReg = Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, OffsetReg);
	AndConstToX86Reg(OffsetReg,3);
	AndConstToX86Reg(TempReg1,(DWORD)~3);

	Map_GPR_32bit(Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg((void *)LWL_MASK,"LWL_MASK",MipsRegMapLo(Opcode.rt),OffsetReg,Multip_x4);
	MoveVariableDispToX86Reg((void *)LWL_SHIFT,"LWL_SHIFT",shift,OffsetReg,4);
	if (bUseTlb()) {			
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftLeftSign(TempReg1);
	AddX86RegToX86Reg(MipsRegMapLo(Opcode.rt),TempReg1);
}

void CMipsMemoryVM::Compile_LWR (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1 = x86_Unknown, TempReg2 = x86_Unknown, OffsetReg = x86_Unknown, shift = x86_Unknown;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 		
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		DWORD Offset  = Address & 3;

		Map_GPR_32bit(Opcode.rt,TRUE,Opcode.rt);
		x86Reg Value = Map_TempReg(x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(MipsRegMapLo(Opcode.rt),LWR_MASK[Offset]);
		ShiftRightUnsignImmed(Value,(BYTE)LWR_SHIFT[Offset]);
		AddX86RegToX86Reg(MipsRegMapLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(TempReg1,TempReg2);
	}
	OffsetReg = Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, OffsetReg);
	AndConstToX86Reg(OffsetReg,3);
	AndConstToX86Reg(TempReg1,(DWORD)~3);

	Map_GPR_32bit(Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg((void *)LWR_MASK,"LWR_MASK",MipsRegMapLo(Opcode.rt),OffsetReg,Multip_x4);
	MoveVariableDispToX86Reg((void *)LWR_SHIFT,"LWR_SHIFT",shift,OffsetReg,4);
	if (bUseTlb()) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftRightUnsign(TempReg1);
	AddX86RegToX86Reg(MipsRegMapLo(Opcode.rt),TempReg1);
}

void CMipsMemoryVM::Compile_LWU (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset);
		Map_GPR_32bit(Opcode.rt,FALSE,0);
		Compile_LW(MipsRegMapLo(Opcode.rt),Address);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		Map_GPR_32bit(Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_32bit(Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(MipsRegMapLo(Opcode.rt), TempReg1);
	}
}

void CMipsMemoryVM::Compile_LD (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (Opcode.rt == 0) return;
	
	x86Reg TempReg1, TempReg2;

	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_64bit(Opcode.rt,-1);
		Compile_LW(MipsRegMapHi(Opcode.rt),Address);
		Compile_LW(MipsRegMapLo(Opcode.rt),Address + 4);
		if (bFastSP() && Opcode.rt == 29) 
		{ 
			ResetMemoryStack(); 
		}
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
		if (bUseTlb()) {
			ProtectGPR(Opcode.base);
			TempReg1 = MipsRegMapLo(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			}
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
		Map_GPR_64bit(Opcode.rt,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,MipsRegMapHi(Opcode.rt));
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,MipsRegMapLo(Opcode.rt),4);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_64bit(Opcode.rt,-1);
		MoveN64MemToX86reg(MipsRegMapHi(Opcode.rt),TempReg1);
		MoveN64MemDispToX86reg(MipsRegMapLo(Opcode.rt),TempReg1,4);
	}
	if (bFastSP() && Opcode.rt == 29) 
	{
		ResetX86Protection();
		_MMU->ResetMemoryStack(); 
	}
}

void CMipsMemoryVM::Compile_LDC1 (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2, TempReg3;
	char Name[50];
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	m_Section->CompileCop1Test();

	UnMap_FPR(Opcode.ft,FALSE);
	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
		Compile_LW(TempReg1,Address);

		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg2);
		AddConstToX86Reg(TempReg2,4);
		MoveX86regToX86Pointer(TempReg1,TempReg2);

		Compile_LW(TempReg1,Address + 4);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg1,TempReg2);
		return;
	}
	if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
		if (bUseTlb()) {
			ProtectGPR(Opcode.base);
			TempReg1 = MipsRegMapLo(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			}
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
			}
		}
	}

	TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
	if (bUseTlb()) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(TempReg1,TempReg2);
		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg3);
		Push(TempReg2);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg2);
		AddConstToX86Reg(TempReg2,4);
		MoveX86regToX86Pointer(TempReg3,TempReg2);
		Pop(TempReg2);
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,TempReg3,4);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg3,TempReg2);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		MoveN64MemToX86reg(TempReg3,TempReg1);

		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg2);
		AddConstToX86Reg(TempReg2,4);
		MoveX86regToX86Pointer(TempReg3,TempReg2);

		MoveN64MemDispToX86reg(TempReg3,TempReg1,4);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg2);
		MoveX86regToX86Pointer(TempReg3,TempReg2);
	}
}

void CMipsMemoryVM::Compile_LDL (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Opcode.rt,TRUE); }
	BeforeCallDirect(m_RegWorkingSet);
	MoveConstToVariable(Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::LDL, "R4300iOp::LDL");
	AfterCallDirect(m_RegWorkingSet);
}

void CMipsMemoryVM::Compile_LDR (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Opcode.rt,TRUE); }
	BeforeCallDirect(m_RegWorkingSet);
	MoveConstToVariable(Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::LDR, "R4300iOp::LDR");
	AfterCallDirect(m_RegWorkingSet);
}

void CMipsMemoryVM::Compile_SB (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		
		if (IsConst(Opcode.rt)) {
			Compile_SB_Const((BYTE)cMipsRegLo(Opcode.rt), Address);
		} else if (IsMapped(Opcode.rt) && Is8BitReg(MipsRegMapLo(Opcode.rt))) {
			Compile_SB_Register(MipsRegMapLo(Opcode.rt), Address);
		} else {
			Compile_SB_Register(Map_TempReg(x86_Any8Bit,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	Compile_StoreInstructClean(TempReg1,4);
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,3);	
		if (IsConst(Opcode.rt)) {
			MoveConstByteToX86regPointer((BYTE)cMipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(Opcode.rt) && Is8BitReg(MipsRegMapLo(Opcode.rt))) {
			MoveX86regByteToX86regPointer(MipsRegMapLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			UnProtectGPR(Opcode.rt);
			MoveX86regByteToX86regPointer(Map_TempReg(x86_Any8Bit,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		if (IsConst(Opcode.rt)) {
			MoveConstByteToN64Mem((BYTE)cMipsRegLo(Opcode.rt),TempReg1);
		} else if (IsMapped(Opcode.rt) && Is8BitReg(MipsRegMapLo(Opcode.rt))) {
			MoveX86regByteToN64Mem(MipsRegMapLo(Opcode.rt),TempReg1);
		} else {	
			UnProtectGPR(Opcode.rt);
			MoveX86regByteToN64Mem(Map_TempReg(x86_Any8Bit,Opcode.rt,FALSE),TempReg1);
		}
	}
}

void CMipsMemoryVM::Compile_SH (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = (cMipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		
		if (IsConst(Opcode.rt)) {
			Compile_SH_Const((WORD)cMipsRegLo(Opcode.rt), Address);
		} else if (IsMapped(Opcode.rt)) {
			Compile_SH_Register(MipsRegMapLo(Opcode.rt), Address);
		} else {
			Compile_SH_Register(Map_TempReg(x86_Any,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,2);	
		if (IsConst(Opcode.rt)) {
			MoveConstHalfToX86regPointer((WORD)cMipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regHalfToX86regPointer(MipsRegMapLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regHalfToX86regPointer(Map_TempReg(x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		XorConstToX86Reg(TempReg1,2);
		if (IsConst(Opcode.rt)) {
			MoveConstHalfToN64Mem((WORD)cMipsRegLo(Opcode.rt),TempReg1);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regHalfToN64Mem(MipsRegMapLo(Opcode.rt),TempReg1);		
		} else {	
			MoveX86regHalfToN64Mem(Map_TempReg(x86_Any,Opcode.rt,FALSE),TempReg1);		
		}
	}
}

void CMipsMemoryVM::Compile_SW (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	
	x86Reg TempReg1, TempReg2;
	if (Opcode.base == 29 && bFastSP()) {
		if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
		TempReg1 = Map_MemoryStack(x86_Any,true);

		if (IsConst(Opcode.rt)) {
			MoveConstToMemoryDisp (cMipsRegLo(Opcode.rt),TempReg1, (DWORD)((short)Opcode.offset));
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToMemory(MipsRegMapLo(Opcode.rt),TempReg1,(DWORD)((short)Opcode.offset));
		} else {	
			TempReg2 = Map_TempReg(x86_Any,Opcode.rt,FALSE);
			MoveX86regToMemory(TempReg2,TempReg1,(DWORD)((short)Opcode.offset));
		}		
	} else {
		if (IsConst(Opcode.base)) { 
			DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
			
			if (IsConst(Opcode.rt)) {
				Compile_SW_Const(cMipsRegLo(Opcode.rt), Address);
			} else if (IsMapped(Opcode.rt)) {
				Compile_SW_Register(MipsRegMapLo(Opcode.rt), Address);
			} else {
				Compile_SW_Register(Map_TempReg(x86_Any,Opcode.rt,FALSE), Address);
			}
			return;
		}
		if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Opcode.base);
			if (bDelaySI() || bDelayDP())
			{
				m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - CountPerOp());
				UpdateCounters(m_RegWorkingSet,false, true);
				m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + CountPerOp());
			}
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			}
			UnProtectGPR(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
		Compile_StoreInstructClean(TempReg1,4);
		if (bUseTlb()) {
			TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
			MoveX86RegToX86Reg(TempReg1, TempReg2);
			ShiftRightUnsignImmed(TempReg2,12);
			MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);
			//For tlb miss
			//0041C522 85 C0                test        eax,eax
			//0041C524 75 01                jne         0041C527

			if (IsConst(Opcode.rt)) {
				MoveConstToX86regPointer(cMipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else if (IsMapped(Opcode.rt)) {
				MoveX86regToX86regPointer(MipsRegMapLo(Opcode.rt),TempReg1, TempReg2);
			} else {	
				MoveX86regToX86regPointer(Map_TempReg(x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
			}
		} else {
			AndConstToX86Reg(TempReg1,0x1FFFFFFF);
			if (IsConst(Opcode.rt)) {
				MoveConstToN64Mem(cMipsRegLo(Opcode.rt),TempReg1);
			} else if (IsMapped(Opcode.rt)) {
				MoveX86regToN64Mem(MipsRegMapLo(Opcode.rt),TempReg1);
			} else {	
				MoveX86regToN64Mem(Map_TempReg(x86_Any,Opcode.rt,FALSE),TempReg1);
			}
		}
	}
}

void CMipsMemoryVM::Compile_SWC1 (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2, TempReg3;
	char Name[50];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	m_Section->CompileCop1Test();
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		
		UnMap_FPR(Opcode.ft,TRUE);
		TempReg1 = Map_TempReg(x86_Any,-1,FALSE);

		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_S[Opcode.ft],Name,TempReg1);
		MoveX86PointerToX86reg(TempReg1,TempReg1);
		Compile_SW_Register(TempReg1, Address);
		return;
	}
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		UnMap_FPR(Opcode.ft,TRUE);
		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_S[Opcode.ft],Name,TempReg3);
		MoveX86PointerToX86reg(TempReg3,TempReg3);
		MoveX86regToX86regPointer(TempReg3,TempReg1, TempReg2);
	} else {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		UnMap_FPR(Opcode.ft,TRUE);
		sprintf(Name,"_FPR_S[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_S[Opcode.ft],Name,TempReg2);
		MoveX86PointerToX86reg(TempReg2,TempReg2);
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveX86regToN64Mem(TempReg2, TempReg1);
	}
}

void CMipsMemoryVM::Compile_SWL (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1 = x86_Unknown, TempReg2 = x86_Unknown, Value = x86_Unknown, 
		shift = x86_Unknown, OffsetReg = x86_Unknown;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		DWORD Offset  = Address & 3;
		
		Value = Map_TempReg(x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(Value,R4300iOp::SWL_MASK[Offset]);
		TempReg1 = Map_TempReg(x86_Any,Opcode.rt,FALSE);
		ShiftRightUnsignImmed(TempReg1,(BYTE)SWL_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		Compile_SW_Register(Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}		
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	OffsetReg = Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, OffsetReg);
	AndConstToX86Reg(OffsetReg,3);
	AndConstToX86Reg(TempReg1,(DWORD)~3);

	Value = Map_TempReg(x86_Any,-1,FALSE);
	if (bUseTlb()) {	
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg((void *)SWL_MASK,"SWL_MASK",Value,OffsetReg,Multip_x4);
	if (!IsConst(Opcode.rt) || cMipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg((void *)SWL_SHIFT,"SWL_SHIFT",shift,OffsetReg,4);
		if (IsConst(Opcode.rt)) {
			MoveConstToX86reg(cMipsRegLo(Opcode.rt),OffsetReg);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(MipsRegMapLo(Opcode.rt),OffsetReg);
		} else {
			MoveVariableToX86reg(&_GPR[Opcode.rt].UW[0],CRegName::GPR_Lo[Opcode.rt],OffsetReg);
		}
		ShiftRightUnsign(OffsetReg);
		AddX86RegToX86Reg(Value,OffsetReg);
	}

	if (bUseTlb()) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);

		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
}

void CMipsMemoryVM::Compile_SWR (void) 
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1 = x86_Unknown, TempReg2 = x86_Unknown, Value = x86_Unknown, 
		OffsetReg = x86_Unknown, shift = x86_Unknown;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		DWORD Offset  = Address & 3;
		
		Value = Map_TempReg(x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(Value,SWR_MASK[Offset]);
		TempReg1 = Map_TempReg(x86_Any,Opcode.rt,FALSE);
		ShiftLeftSignImmed(TempReg1,(BYTE)SWR_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		Compile_SW_Register(Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
	}		
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_ReadMap,"m_TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	OffsetReg = Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, OffsetReg);
	AndConstToX86Reg(OffsetReg,3);
	AndConstToX86Reg(TempReg1,(DWORD)~3);

	Value = Map_TempReg(x86_Any,-1,FALSE);
	if (bUseTlb()) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg((void *)SWR_MASK,"SWR_MASK",Value,OffsetReg,Multip_x4);
	if (!IsConst(Opcode.rt) || cMipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg((void *)SWR_SHIFT,"SWR_SHIFT",shift,OffsetReg,4);
		if (IsConst(Opcode.rt)) {
			MoveConstToX86reg(cMipsRegLo(Opcode.rt),OffsetReg);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(MipsRegMapLo(Opcode.rt),OffsetReg);
		} else {
			MoveVariableToX86reg(&_GPR[Opcode.rt].UW[0],CRegName::GPR_Lo[Opcode.rt],OffsetReg);
		}
		ShiftLeftSign(OffsetReg);
		AddX86RegToX86Reg(Value,OffsetReg);
	}

	if (bUseTlb()) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);

		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
}

void CMipsMemoryVM::Compile_StoreInstructClean (x86Reg AddressReg, int Length )
{
	if (!_Recompiler->bSMM_StoreInstruc())
	{ 
		return;
	}
	_Notify->BreakPoint(__FILE__,__LINE__);

	/*
	stdstr_f strLen("%d",Length);
	UnMap_AllFPRs();
	
	/*x86Reg StoreTemp1 = Map_TempReg(x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(AddressReg, StoreTemp1);
 	AndConstToX86Reg(StoreTemp1,0xFFC);*/		
	BeforeCallDirect(m_RegWorkingSet);
	PushImm32("CRecompiler::Remove_StoreInstruc",CRecompiler::Remove_StoreInstruc);
	PushImm32(Length);
	Push(AddressReg);
	MoveConstToX86reg((DWORD)_Recompiler,x86_ECX);
	Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
	AfterCallDirect(m_RegWorkingSet);
	/*JmpLabel8("MemCheckDone",0);
	BYTE * MemCheckDone = m_RecompPos - 1;
	
	CPU_Message("      ");
	CPU_Message("      NotDelaySlot:");
	SetJump8(NotDelaySlotJump,m_RecompPos);

	MoveX86RegToX86Reg(AddressReg, StoreTemp1);
	ShiftRightUnsignImmed(StoreTemp1,12);
	LeaRegReg(StoreTemp1,StoreTemp1,(ULONG)&(_Recompiler->FunctionTable()[0]),Multip_x4);
	CompConstToX86regPointer(StoreTemp1,0);
	JeLabel8("MemCheckDone",0);
	BYTE * MemCheckDone2 = m_RecompPos - 1;

	BeforeCallDirect(m_RegWorkingSet);
	PushImm32("CRecompiler::Remove_StoreInstruc",CRecompiler::Remove_StoreInstruc);
	PushImm32(strLen.c_str(),Length);
	Push(AddressReg);
	MoveConstToX86reg((DWORD)_Recompiler,x86_ECX);
	Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
	AfterCallDirect(m_RegWorkingSet);
	
	CPU_Message("      ");
	CPU_Message("      MemCheckDone:");
	SetJump8(MemCheckDone,m_RecompPos);			
	SetJump8(MemCheckDone2,m_RecompPos);			

	X86Protected(StoreTemp1) = false;*/
}

void CMipsMemoryVM::Compile_SD (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		
		if (IsConst(Opcode.rt)) {
			Compile_SW_Const(Is64Bit(Opcode.rt) ? MipsRegHi(Opcode.rt) : (MipsRegLo_S(Opcode.rt) >> 31), Address);
			Compile_SW_Const(cMipsRegLo(Opcode.rt), Address + 4);
		} else if (IsMapped(Opcode.rt)) {
			Compile_SW_Register(Is64Bit(Opcode.rt) ? MipsRegMapHi(Opcode.rt) : Map_TempReg(x86_Any,Opcode.rt,TRUE), Address);
			Compile_SW_Register(MipsRegMapLo(Opcode.rt), Address + 4);		
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.rt,TRUE);
			Compile_SW_Register(TempReg1, Address);
			Compile_SW_Register(Map_TempReg(TempReg1,Opcode.rt,FALSE), Address + 4);		
		}
	} else {
		if (IsMapped(Opcode.rt)) { ProtectGPR(Opcode.rt); }
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			}
			UnProtectGPR(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
		
		Compile_StoreInstructClean(TempReg1,8);
		
		if (bUseTlb()) {
			TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
			MoveX86RegToX86Reg(TempReg1, TempReg2);
			ShiftRightUnsignImmed(TempReg2,12);
			MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);
			//For tlb miss
			//0041C522 85 C0                test        eax,eax
			//0041C524 75 01                jne         0041C527

			if (IsConst(Opcode.rt)) {
				if (Is64Bit(Opcode.rt)) {
					MoveConstToX86regPointer(MipsRegHi(Opcode.rt),TempReg1, TempReg2);
				} else {
					MoveConstToX86regPointer((MipsRegLo_S(Opcode.rt) >> 31),TempReg1, TempReg2);
				}
				AddConstToX86Reg(TempReg1,4);
				MoveConstToX86regPointer(cMipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else if (IsMapped(Opcode.rt)) {
				if (Is64Bit(Opcode.rt)) {
					MoveX86regToX86regPointer(MipsRegMapHi(Opcode.rt),TempReg1, TempReg2);
				} else {
					MoveX86regToX86regPointer(Map_TempReg(x86_Any,Opcode.rt,TRUE),TempReg1, TempReg2);
				}
				AddConstToX86Reg(TempReg1,4);
				MoveX86regToX86regPointer(MipsRegMapLo(Opcode.rt),TempReg1, TempReg2);
			} else {	
				x86Reg Reg = Map_TempReg(x86_Any,Opcode.rt,TRUE);
				MoveX86regToX86regPointer(Reg,TempReg1, TempReg2);
				AddConstToX86Reg(TempReg1,4);
				MoveX86regToX86regPointer(Map_TempReg(Reg,Opcode.rt,FALSE),TempReg1, TempReg2);
			}
		} else {
			AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
			if (IsConst(Opcode.rt)) {
				if (Is64Bit(Opcode.rt)) {
					MoveConstToN64Mem(MipsRegHi(Opcode.rt),TempReg1);
				} else if (IsSigned(Opcode.rt)) {
					MoveConstToN64Mem((cMipsRegLo_S(Opcode.rt) >> 31),TempReg1);
				} else {
					MoveConstToN64Mem(0,TempReg1);
				}
				MoveConstToN64MemDisp(cMipsRegLo(Opcode.rt),TempReg1,4);
			} else if (IsKnown(Opcode.rt) && IsMapped(Opcode.rt)) {
				if (Is64Bit(Opcode.rt)) {
					MoveX86regToN64Mem(MipsRegMapHi(Opcode.rt),TempReg1);
				} else if (IsSigned(Opcode.rt)) {
					MoveX86regToN64Mem(Map_TempReg(x86_Any,Opcode.rt,TRUE), TempReg1);
				} else {
					MoveConstToN64Mem(0,TempReg1);
				}
				MoveX86regToN64MemDisp(MipsRegMapLo(Opcode.rt),TempReg1, 4);		
			} else {	
				x86Reg Reg;
				MoveX86regToN64Mem(Reg = Map_TempReg(x86_Any,Opcode.rt,TRUE), TempReg1);
				MoveX86regToN64MemDisp(Map_TempReg(Reg,Opcode.rt,FALSE), TempReg1,4);
			}
		}
	}
}

void CMipsMemoryVM::Compile_SDC1 (void)
{
	OPCODE & Opcode = CRecompilerOps::m_Opcode;
	x86Reg TempReg1, TempReg2, TempReg3;
	char Name[50];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));

	m_Section->CompileCop1Test();
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = cMipsRegLo(Opcode.base) + (short)Opcode.offset;
		
		TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_D[Opcode.ft],Name,TempReg1);
		AddConstToX86Reg(TempReg1,4);
		MoveX86PointerToX86reg(TempReg1,TempReg1);
		Compile_SW_Register(TempReg1, Address);

		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg(&_FPR_D[Opcode.ft],Name,TempReg1);
		MoveX86PointerToX86reg(TempReg1,TempReg1);
		Compile_SW_Register(TempReg1, Address + 4);		
		return;
	}
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegMapLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (bUseTlb()) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(m_TLB_WriteMap,"m_TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_D[Opcode.ft],Name,TempReg3);
		AddConstToX86Reg(TempReg3,4);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToX86regPointer(TempReg3,TempReg1, TempReg2);
		AddConstToX86Reg(TempReg1,4);

		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_D[Opcode.ft],Name,TempReg3);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToX86regPointer(TempReg3,TempReg1, TempReg2);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		TempReg3 = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_D[Opcode.ft],Name,TempReg3);
		AddConstToX86Reg(TempReg3,4);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToN64Mem(TempReg3, TempReg1);
		sprintf(Name,"_FPR_D[%d]",Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_D[Opcode.ft],Name,TempReg3);
		MoveX86PointerToX86reg(TempReg3,TempReg3);		
		MoveX86regToN64MemDisp(TempReg3, TempReg1,4);
	}
}

void CMipsMemoryVM::Compile_SDL (void) {
	OPCODE & Opcode = CRecompilerOps::m_Opcode;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Opcode.rt,TRUE); }
	BeforeCallDirect(m_RegWorkingSet);
	MoveConstToVariable(Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::SDL, "R4300iOp::SDL");
	AfterCallDirect(m_RegWorkingSet);
}

void CMipsMemoryVM::Compile_SDR (void) {
	OPCODE & Opcode = CRecompilerOps::m_Opcode;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(Opcode.Hex,m_CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Opcode.rt,TRUE); }
	BeforeCallDirect(m_RegWorkingSet);
	MoveConstToVariable(Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::SDR, "R4300iOp::SDR");
	AfterCallDirect(m_RegWorkingSet);
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

void CMipsMemoryVM::RdramChanged ( CMipsMemoryVM * _this )
{
	if (_this->m_AllocatedRdramSize == _Settings->LoadDword(Game_RDRamSize))
	{
		return;
	}
	if (_this->m_AllocatedRdramSize == 0x400000) { 
		if (VirtualAlloc(_this->m_RDRAM + 0x400000, 0x400000, MEM_COMMIT, PAGE_READWRITE)==NULL)
		{
			WriteTrace(TraceError,"CMipsMemoryVM::RdramChanged: failed to allocate extended memory");
			_Notify->FatalError(GS(MSG_MEM_ALLOC_ERROR));
		}
		_this->m_AllocatedRdramSize = 0x800000;
	} else {
		VirtualFree(_this->m_RDRAM + 0x400000, 0x400000,MEM_DECOMMIT);
		_this->m_AllocatedRdramSize = 0x400000;
	}

}

void CMipsMemoryVM::ChangeSpStatus (void)
{
	if ( ( RegModValue & SP_CLR_HALT ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_HALT; }
	if ( ( RegModValue & SP_SET_HALT ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_HALT;  }
	if ( ( RegModValue & SP_CLR_BROKE ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_BROKE; }
	if ( ( RegModValue & SP_CLR_INTR ) != 0) { 
		_Reg->MI_INTR_REG &= ~MI_INTR_SP; 
		_Reg->m_RspIntrReg &= ~MI_INTR_SP;
		_Reg->CheckInterrupts();
	}
#ifndef EXTERNAL_RELEASE
	if ( ( RegModValue & SP_SET_INTR ) != 0) { _Notify->DisplayError("SP_SET_INTR"); }
#endif
	if ( ( RegModValue & SP_CLR_SSTEP ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
	if ( ( RegModValue & SP_SET_SSTEP ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SSTEP;  }
	if ( ( RegModValue & SP_CLR_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
	if ( ( RegModValue & SP_SET_INTR_BREAK ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
	if ( ( RegModValue & SP_CLR_SIG0 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG0; }
	if ( ( RegModValue & SP_SET_SIG0 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG0;  }
	if ( ( RegModValue & SP_CLR_SIG1 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG1; }
	if ( ( RegModValue & SP_SET_SIG1 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG1;  }
	if ( ( RegModValue & SP_CLR_SIG2 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG2; }
	if ( ( RegModValue & SP_SET_SIG2 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG2;  }
	if ( ( RegModValue & SP_CLR_SIG3 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG3; }
	if ( ( RegModValue & SP_SET_SIG3 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG3;  }
	if ( ( RegModValue & SP_CLR_SIG4 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG4; }
	if ( ( RegModValue & SP_SET_SIG4 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG4;  }
	if ( ( RegModValue & SP_CLR_SIG5 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG5; }
	if ( ( RegModValue & SP_SET_SIG5 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG5;  }
	if ( ( RegModValue & SP_CLR_SIG6 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG6; }
	if ( ( RegModValue & SP_SET_SIG6 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG6;  }
	if ( ( RegModValue & SP_CLR_SIG7 ) != 0) { _Reg->SP_STATUS_REG &= ~SP_STATUS_SIG7; }
	if ( ( RegModValue & SP_SET_SIG7 ) != 0) { _Reg->SP_STATUS_REG |= SP_STATUS_SIG7;  }

	if ( ( RegModValue & SP_SET_SIG0 ) != 0 && _Settings->LoadBool(Game_RspAudioSignal))
	{
		_Reg->MI_INTR_REG |= MI_INTR_SP; 
		_Reg->CheckInterrupts();				
	}
	//if (*( DWORD *)(DMEM + 0xFC0) == 1) {
	//	ChangeTimer(RspTimer,0x40000);
	//} else {
		try {
			_System->RunRSP();
		} catch (...) {
			_Notify->BreakPoint(__FILE__,__LINE__);
		}
	//}
}

void CMipsMemoryVM::ChangeMiIntrMask (void) {
	if ( ( RegModValue & MI_INTR_MASK_CLR_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
	if ( ( RegModValue & MI_INTR_MASK_SET_SP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_SI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_AI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_VI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
	if ( ( RegModValue & MI_INTR_MASK_SET_PI ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
	if ( ( RegModValue & MI_INTR_MASK_CLR_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
	if ( ( RegModValue & MI_INTR_MASK_SET_DP ) != 0 ) { _Reg->MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
}
