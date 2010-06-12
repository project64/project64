#include "stdafx.h"

const char * CRegName::GPR[32] = {"r0","at","v0","v1","a0","a1","a2","a3",
					 "t0","t1","t2","t3","t4","t5","t6","t7",
					 "s0","s1","s2","s3","s4","s5","s6","s7",
					 "t8","t9","k0","k1","gp","sp","s8","ra"};

const char *CRegName::GPR_Hi[32] = {"r0.HI","at.HI","v0.HI","v1.HI","a0.HI","a1.HI",
						"a2.HI","a3.HI","t0.HI","t1.HI","t2.HI","t3.HI",
						"t4.HI","t5.HI","t6.HI","t7.HI","s0.HI","s1.HI",
						"s2.HI","s3.HI","s4.HI","s5.HI","s6.HI","s7.HI",
						"t8.HI","t9.HI","k0.HI","k1.HI","gp.HI","sp.HI",
						"s8.HI","ra.HI"};

const char *CRegName::GPR_Lo[32] = {"r0.LO","at.LO","v0.LO","v1.LO","a0.LO","a1.LO",
						"a2.LO","a3.LO","t0.LO","t1.LO","t2.LO","t3.LO",
						"t4.LO","t5.LO","t6.LO","t7.LO","s0.LO","s1.LO",
						"s2.LO","s3.LO","s4.LO","s5.LO","s6.LO","s7.LO",
						"t8.LO","t9.LO","k0.LO","k1.LO","gp.LO","sp.LO",
						"s8.LO","ra.LO"};

const char * CRegName::Cop0[32] = {"Index","Random","EntryLo0","EntryLo1","Context","PageMask","Wired","",
					"BadVAddr","Count","EntryHi","Compare","Status","Cause","EPC","PRId",
					"Config","LLAddr","WatchLo","WatchHi","XContext","","","",
					"","","ECC","CacheErr","TagLo","TagHi","ErrEPC",""};

const char * CRegName::FPR[32] = {"f0","f1","f2","f3","f4","f5","f6","f7",
					 "f8","f9","f10","f11","f12","f13","f14","f15",
					 "f16","f17","f18","f19","f20","f21","f22","f23",
					 "f24","f25","f26","f27","f28","f29","f30","f31"};

const char * CRegName::FPR_Ctrl[32] = {"Revision","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","FCSR"};

DWORD         * CSystemRegisters::_PROGRAM_COUNTER = NULL;
MIPS_DWORD    * CSystemRegisters::_GPR = NULL;
MIPS_DWORD    * CSystemRegisters::_FPR = NULL;
DWORD         * CSystemRegisters::_CP0 = NULL;
MIPS_DWORD    * CSystemRegisters::_RegHI = NULL;
MIPS_DWORD    * CSystemRegisters::_RegLO = NULL;
float        ** CSystemRegisters::_FPR_S;		
double       ** CSystemRegisters::_FPR_D;
DWORD         * CSystemRegisters::_FPCR = NULL;
DWORD         * CSystemRegisters::_LLBit = NULL;
DWORD         * CSystemRegisters::_LLAddr = NULL;
ROUNDING_MODE * CSystemRegisters::_RoundingModel = NULL;


CP0registers::CP0registers(DWORD * _CP0) :
	INDEX_REGISTER(_CP0[0]),
	RANDOM_REGISTER(_CP0[1]),
	ENTRYLO0_REGISTER(_CP0[2]),
	ENTRYLO1_REGISTER(_CP0[3]),
	CONTEXT_REGISTER(_CP0[4]),
	PAGE_MASK_REGISTER(_CP0[5]),
	WIRED_REGISTER(_CP0[6]),
	BAD_VADDR_REGISTER(_CP0[8]),
	COUNT_REGISTER(_CP0[9]),
	ENTRYHI_REGISTER(_CP0[10]),
	COMPARE_REGISTER(_CP0[11]),
	STATUS_REGISTER(_CP0[12]),
	CAUSE_REGISTER(_CP0[13]),
	EPC_REGISTER(_CP0[14]),
	CONFIG_REGISTER(_CP0[16]),
	TAGLO_REGISTER(_CP0[28]),
	TAGHI_REGISTER(_CP0[29]),
	ERROREPC_REGISTER(_CP0[30]),
	FAKE_CAUSE_REGISTER(_CP0[32])
{
}

Rdram_InterfaceReg::Rdram_InterfaceReg(DWORD * _RdramInterface) :
	RDRAM_CONFIG_REG(_RdramInterface[0]),
	RDRAM_DEVICE_TYPE_REG(_RdramInterface[0]),
	RDRAM_DEVICE_ID_REG(_RdramInterface[1]),
	RDRAM_DELAY_REG(_RdramInterface[2]),
	RDRAM_MODE_REG(_RdramInterface[3]),
	RDRAM_REF_INTERVAL_REG(_RdramInterface[4]),
	RDRAM_REF_ROW_REG(_RdramInterface[5]),
	RDRAM_RAS_INTERVAL_REG(_RdramInterface[6]),
	RDRAM_MIN_INTERVAL_REG(_RdramInterface[7]),
	RDRAM_ADDR_SELECT_REG(_RdramInterface[8]),
	RDRAM_DEVICE_MANUF_REG(_RdramInterface[9])
{
}

Mips_InterfaceReg::Mips_InterfaceReg(DWORD * _MipsInterface) :
	MI_INIT_MODE_REG(_MipsInterface[0]),
	MI_MODE_REG(_MipsInterface[0]),
	MI_VERSION_REG(_MipsInterface[1]),
	MI_NOOP_REG(_MipsInterface[1]),
	MI_INTR_REG(_MipsInterface[2]),
	MI_INTR_MASK_REG(_MipsInterface[3])
{
}

Video_InterfaceReg::Video_InterfaceReg(DWORD * _VideoInterface) :
	VI_STATUS_REG(_VideoInterface[0]),
	VI_CONTROL_REG(_VideoInterface[0]),
	VI_ORIGIN_REG(_VideoInterface[1]),
	VI_DRAM_ADDR_REG(_VideoInterface[1]),
	VI_WIDTH_REG(_VideoInterface[2]),
	VI_H_WIDTH_REG(_VideoInterface[2]),
	VI_INTR_REG(_VideoInterface[3]),
	VI_V_INTR_REG(_VideoInterface[3]),
	VI_CURRENT_REG(_VideoInterface[4]),
	VI_V_CURRENT_LINE_REG(_VideoInterface[4]),
	VI_BURST_REG(_VideoInterface[5]),
	VI_TIMING_REG(_VideoInterface[5]),
	VI_V_SYNC_REG(_VideoInterface[6]),
	VI_H_SYNC_REG(_VideoInterface[7]),
	VI_LEAP_REG(_VideoInterface[8]),
	VI_H_SYNC_LEAP_REG(_VideoInterface[8]),
	VI_H_START_REG(_VideoInterface[9]),
	VI_H_VIDEO_REG(_VideoInterface[9]),
	VI_V_START_REG(_VideoInterface[10]),
	VI_V_VIDEO_REG(_VideoInterface[10]),
	VI_V_BURST_REG(_VideoInterface[11]),
	VI_X_SCALE_REG(_VideoInterface[12]),
	VI_Y_SCALE_REG(_VideoInterface[13])
{
}

AudioInterfaceReg::AudioInterfaceReg(DWORD * _AudioInterface) :
	AI_DRAM_ADDR_REG(_AudioInterface[0]),
	AI_LEN_REG(_AudioInterface[1]),
	AI_CONTROL_REG(_AudioInterface[2]),
	AI_STATUS_REG(_AudioInterface[3]),
	AI_DACRATE_REG(_AudioInterface[4]),
	AI_BITRATE_REG(_AudioInterface[5])
{
}
	
PeripheralInterfaceReg::PeripheralInterfaceReg(DWORD * PeripheralInterface) :
	PI_DRAM_ADDR_REG(PeripheralInterface[0]),
	PI_CART_ADDR_REG(PeripheralInterface[1]),
	PI_RD_LEN_REG(PeripheralInterface[2]),
	PI_WR_LEN_REG(PeripheralInterface[3]),
	PI_STATUS_REG(PeripheralInterface[4]),
	PI_BSD_DOM1_LAT_REG(PeripheralInterface[5]),
	PI_DOMAIN1_REG(PeripheralInterface[5]),
	PI_BSD_DOM1_PWD_REG(PeripheralInterface[6]),
	PI_BSD_DOM1_PGS_REG(PeripheralInterface[7]),
	PI_BSD_DOM1_RLS_REG(PeripheralInterface[8]),
	PI_BSD_DOM2_LAT_REG(PeripheralInterface[9]),
	PI_DOMAIN2_REG(PeripheralInterface[9]),
	PI_BSD_DOM2_PWD_REG(PeripheralInterface[10]),
	PI_BSD_DOM2_PGS_REG(PeripheralInterface[11]),
	PI_BSD_DOM2_RLS_REG(PeripheralInterface[12])
{
}

RDRAMInt_InterfaceReg::RDRAMInt_InterfaceReg(DWORD * RdramInterface) :
	RI_MODE_REG(RdramInterface[0]),
	RI_CONFIG_REG(RdramInterface[1]),
	RI_CURRENT_LOAD_REG(RdramInterface[2]),
	RI_SELECT_REG(RdramInterface[3]),
	RI_COUNT_REG(RdramInterface[4]),
	RI_REFRESH_REG(RdramInterface[4]),
	RI_LATENCY_REG(RdramInterface[5]),
	RI_RERROR_REG(RdramInterface[6]),
	RI_WERROR_REG(RdramInterface[7])
{
}

DisplayControlReg::DisplayControlReg(DWORD * _DisplayProcessor) :
	DPC_START_REG(_DisplayProcessor[0]),
	DPC_END_REG(_DisplayProcessor[1]),
	DPC_CURRENT_REG(_DisplayProcessor[2]),
	DPC_STATUS_REG(_DisplayProcessor[3]),
	DPC_CLOCK_REG(_DisplayProcessor[4]),
	DPC_BUFBUSY_REG(_DisplayProcessor[5]),
	DPC_PIPEBUSY_REG(_DisplayProcessor[6]),
	DPC_TMEM_REG(_DisplayProcessor[7])
{
}

SigProcessor_InterfaceReg::SigProcessor_InterfaceReg(DWORD * _SignalProcessorInterface) :
	SP_MEM_ADDR_REG(_SignalProcessorInterface[0]),
	SP_DRAM_ADDR_REG(_SignalProcessorInterface[1]),
	SP_RD_LEN_REG(_SignalProcessorInterface[2]),
	SP_WR_LEN_REG(_SignalProcessorInterface[3]),
	SP_STATUS_REG(_SignalProcessorInterface[4]),
	SP_DMA_FULL_REG(_SignalProcessorInterface[5]),
	SP_DMA_BUSY_REG(_SignalProcessorInterface[6]),
	SP_SEMAPHORE_REG(_SignalProcessorInterface[7]),
	SP_PC_REG(_SignalProcessorInterface[8]),
	SP_IBIST_REG(_SignalProcessorInterface[9])
{
}

Serial_InterfaceReg::Serial_InterfaceReg(DWORD * SerialInterface) :
	SI_DRAM_ADDR_REG(SerialInterface[0]),
	SI_PIF_ADDR_RD64B_REG(SerialInterface[1]),
	SI_PIF_ADDR_WR64B_REG(SerialInterface[2]),
	SI_STATUS_REG(SerialInterface[3])
{
}

CRegisters::CRegisters (void) :
	CP0registers(m_CP0),
	Rdram_InterfaceReg(m_RDRAM_Registers),
	Mips_InterfaceReg(m_Mips_Interface),
	Video_InterfaceReg(m_Video_Interface),
	AudioInterfaceReg(m_Audio_Interface),
	PeripheralInterfaceReg(m_Peripheral_Interface),
	RDRAMInt_InterfaceReg(m_RDRAM_Interface),
	SigProcessor_InterfaceReg(m_SigProcessor_Interface),
	DisplayControlReg(m_Display_ControlReg),
	Serial_InterfaceReg(m_SerialInterface)
{ 
	Reset();
}

void CRegisters::Reset()
{
	m_FirstInterupt = true;

	memset(m_GPR,0,sizeof(m_GPR));	
	memset(m_CP0,0,sizeof(m_CP0));	
	memset(m_FPR,0,sizeof(m_FPR));	
	memset(m_FPCR,0,sizeof(m_FPCR));	
	m_HI.DW   = 0;
	m_LO.DW   = 0;
	m_RoundingModel = ROUND_NEAR;
	
	m_LLBit   = 0;
	m_LLAddr  = 0;

	//Reset System Registers
	memset(m_RDRAM_Interface,0,sizeof(m_RDRAM_Interface));	
	memset(m_RDRAM_Registers,0,sizeof(m_RDRAM_Registers));	
	memset(m_Mips_Interface,0,sizeof(m_Mips_Interface));	
	memset(m_Video_Interface,0,sizeof(m_Video_Interface));	
	memset(m_Display_ControlReg,0,sizeof(m_Display_ControlReg));	
	memset(m_Audio_Interface,0,sizeof(m_Audio_Interface));	
	memset(m_SigProcessor_Interface,0,sizeof(m_SigProcessor_Interface));	
	memset(m_Peripheral_Interface,0,sizeof(m_Peripheral_Interface));	
	memset(m_SerialInterface,0,sizeof(m_SerialInterface));	

	FixFpuLocations();
}

void CRegisters::SetAsCurrentSystem ( void )
{
	_PROGRAM_COUNTER = &m_PROGRAM_COUNTER;
	_GPR = m_GPR;
	_FPR = m_FPR;
	_CP0 = m_CP0;
	_RegHI = &m_HI;
	_RegLO = &m_LO;
	_FPR_S = m_FPR_S;
	_FPR_D = m_FPR_D;
	_FPCR = m_FPCR;
	_LLBit = &m_LLBit;
	_LLAddr = &m_LLAddr;
	_RoundingModel = &m_RoundingModel;
}

void CRegisters::CheckInterrupts ( void ) 
{
	if (!g_FixedAudio && g_CPU_Type != CPU_SyncCores) {
		MI_INTR_REG &= ~MI_INTR_AI;
		MI_INTR_REG |= (m_AudioIntrReg & MI_INTR_AI);
	}
	if ((MI_INTR_MASK_REG & MI_INTR_REG) != 0) {
		FAKE_CAUSE_REGISTER |= CAUSE_IP2;
	} else  {
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP2;
	}

	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }

	if (( STATUS_REGISTER & FAKE_CAUSE_REGISTER & 0xFF00) != 0) {
		if (m_FirstInterupt)
		{
			m_FirstInterupt = false;
			if (_Recompiler)
			{
				_Recompiler->ClearRecompCode_Virt(0x80000000,0x200,CRecompiler::Remove_InitialCode);
			}
		}
		_SystemEvents->QueueEvent(SysEvent_ExecuteInterrupt);
	}
}

void CRegisters::DoAddressError ( BOOL DelaySlot, DWORD BadVaddr, BOOL FromRead) 
{
#ifndef EXTERNAL_RELEASE
	DisplayError("AddressError");
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in AddressError Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in AddressError Exception");
	}
#endif
	if (FromRead) {
		CAUSE_REGISTER = EXC_RADE;
	} else {
		CAUSE_REGISTER = EXC_WADE;
	}
	BAD_VADDR_REGISTER = BadVaddr;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = m_PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = m_PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	m_PROGRAM_COUNTER = 0x80000180;
}

void CRegisters::FixFpuLocations ( void ) {	
	if ((STATUS_REGISTER & STATUS_FR) == 0) {
		for (int count = 0; count < 32; count ++) {
			m_FPR_S[count] = &m_FPR[count >> 1].F[count & 1];
			m_FPR_D[count] = &m_FPR[count >> 1].D;
		}
	} else {
		for (int count = 0; count < 32; count ++) {
			m_FPR_S[count] = &m_FPR[count].F[1];
			m_FPR_D[count] = &m_FPR[count].D;
		}
	}
}

void CRegisters::DoBreakException ( BOOL DelaySlot) 
{
#ifndef EXTERNAL_RELEASE
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in Break Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in Break Exception");
	}
#endif

	CAUSE_REGISTER = EXC_BREAK;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = m_PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = m_PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	m_PROGRAM_COUNTER = 0x80000180;
}

void CRegisters::DoCopUnusableException ( BOOL DelaySlot, int Coprocessor )
{
#ifndef EXTERNAL_RELEASE
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in Break Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in Break Exception");
	}
#endif

	CAUSE_REGISTER = EXC_CPU;
	if (Coprocessor == 1) { CAUSE_REGISTER |= 0x10000000; }
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = m_PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = m_PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	m_PROGRAM_COUNTER = 0x80000180;
}


BOOL CRegisters::DoIntrException ( BOOL DelaySlot ) 
{
	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return FALSE; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return FALSE; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return FALSE; }
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.GenerateLog && LogOptions.LogExceptions && !LogOptions.NoInterrupts) {
		LogMessage("%08X: Interupt Generated", m_PROGRAM_COUNTER );
	}
#endif
	CAUSE_REGISTER = FAKE_CAUSE_REGISTER;
	CAUSE_REGISTER |= EXC_INT;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = m_PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = m_PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	m_PROGRAM_COUNTER = 0x80000180;
	return TRUE;
}

void CRegisters::DoTLBMiss ( BOOL DelaySlot, DWORD BadVaddr ) 
{
	CAUSE_REGISTER = EXC_RMISS;
	BAD_VADDR_REGISTER = BadVaddr;
	CONTEXT_REGISTER &= 0xFF80000F;
	CONTEXT_REGISTER |= (BadVaddr >> 9) & 0x007FFFF0;
	ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
	if ((STATUS_REGISTER & STATUS_EXL) == 0) {
		if (DelaySlot) {
			CAUSE_REGISTER |= CAUSE_BD;
			EPC_REGISTER = m_PROGRAM_COUNTER - 4;
		} else {
			EPC_REGISTER = m_PROGRAM_COUNTER;
		}
		if (_TLB->AddressDefined(BadVaddr)) 
		{
			m_PROGRAM_COUNTER = 0x80000180;
		} else {
			m_PROGRAM_COUNTER = 0x80000000;
		}
		STATUS_REGISTER |= STATUS_EXL;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("TLBMiss - EXL Set\nBadVaddr = %X\nAddress Defined: %s",BadVaddr,_TLB->AddressDefined(BadVaddr)?"TRUE":"FALSE");
#endif
		m_PROGRAM_COUNTER = 0x80000180;
	}
}

void CRegisters::DoSysCallException ( BOOL DelaySlot) 
{
#ifndef EXTERNAL_RELEASE
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		DisplayError("EXL set in SysCall Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		DisplayError("ERL set in SysCall Exception");
	}
#endif

	CAUSE_REGISTER = EXC_SYSCALL;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = m_PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = m_PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	m_PROGRAM_COUNTER = 0x80000180;
}

#ifdef toremove
void CRegisters::InitalizeR4300iRegisters (CMipsMemory & MMU, bool PostPif, int Country, CICChip CIC_Chip)
{
	//Reset General Registers
	memset(GPR,0,sizeof(GPR));	
	memset(CP0,0,sizeof(CP0));	
	memset(FPR,0,sizeof(FPR));	
	memset(FPCR,0,sizeof(FPCR));	
	HI.DW   = 0;
	LO.DW   = 0;
	LLBit   = 0;
	LLAddr  = 0;

	//Reset System Registers
	memset(RDRAM_Interface,0,sizeof(RDRAM_Interface));	
	memset(RDRAM_Registers,0,sizeof(RDRAM_Registers));	
	memset(Mips_Interface,0,sizeof(Mips_Interface));	
	memset(Video_Interface,0,sizeof(Video_Interface));	
	memset(Display_ControlReg,0,sizeof(Display_ControlReg));	
	memset(Audio_Interface,0,sizeof(Audio_Interface));	
	memset(SigProcessor_Interface,0,sizeof(SigProcessor_Interface));	
	memset(Peripheral_Interface,0,sizeof(Peripheral_Interface));	
	memset(SerialInterface,0,sizeof(SerialInterface));	

	//COP0 Registers
	RANDOM_REGISTER	    = 0x1F;
	COUNT_REGISTER	    = 0x5000;
	MI_VERSION_REG	    = 0x02020102;
	SP_STATUS_REG       = 0x00000001;
	CAUSE_REGISTER	    = 0x0000005C;
	CONTEXT_REGISTER    = 0x007FFFF0;
	EPC_REGISTER        = 0xFFFFFFFF;
	BAD_VADDR_REGISTER  = 0xFFFFFFFF;
	ERROREPC_REGISTER   = 0xFFFFFFFF;
	CONFIG_REGISTER     = 0x0006E463;
	STATUS_REGISTER     = 0x34000000;

	//REVISION_REGISTER   = 0x00000511;

	ChangeTimerFixed(CompareTimer,COMPARE_REGISTER - COUNT_REGISTER); 
	AudioIntrReg = 0;

	if (PostPif) {
		PROGRAM_COUNTER	  = 0xA4000040;	
		
		GPR[0].DW=0x0000000000000000;
		GPR[6].DW=0xFFFFFFFFA4001F0C;
		GPR[7].DW=0xFFFFFFFFA4001F08;
		GPR[8].DW=0x00000000000000C0;
		GPR[9].DW=0x0000000000000000;
		GPR[10].DW=0x0000000000000040;
		GPR[11].DW=0xFFFFFFFFA4000040;
		GPR[16].DW=0x0000000000000000;
		GPR[17].DW=0x0000000000000000;
		GPR[18].DW=0x0000000000000000;
		GPR[19].DW=0x0000000000000000;
		GPR[21].DW=0x0000000000000000; 
		GPR[26].DW=0x0000000000000000;
		GPR[27].DW=0x0000000000000000;
		GPR[28].DW=0x0000000000000000;
		GPR[29].DW=0xFFFFFFFFA4001FF0;
		GPR[30].DW=0x0000000000000000;
		
		switch (Country) {
		case Germany: case french:  case Italian:
		case Europe:  case Spanish: case Australia:
		case X_PAL:   case Y_PAL:
			switch (CIC_Chip) {
			case CIC_NUS_6102:
				GPR[5].DW=0xFFFFFFFFC0F1D859;
				GPR[14].DW=0x000000002DE108EA;
				GPR[24].DW=0x0000000000000000;
				break;
			case CIC_NUS_6103:
				GPR[5].DW=0xFFFFFFFFD4646273;
				GPR[14].DW=0x000000001AF99984;
				GPR[24].DW=0x0000000000000000;
				break;
			case CIC_NUS_6105:
				MMU.SW_VAddr(0xA4001004,0xBDA807FC);
				GPR[5].DW=0xFFFFFFFFDECAAAD1;
				GPR[14].DW=0x000000000CF85C13;
				GPR[24].DW=0x0000000000000002;
				break;
			case CIC_NUS_6106:
				GPR[5].DW=0xFFFFFFFFB04DC903;
				GPR[14].DW=0x000000001AF99984;
				GPR[24].DW=0x0000000000000002;
				break;
			}

			GPR[20].DW=0x0000000000000000;
			GPR[23].DW=0x0000000000000006;
			GPR[31].DW=0xFFFFFFFFA4001554;
			break;
		case NTSC_BETA: case X_NTSC: case USA: case Japan:
		default:
			switch (CIC_Chip) {
			case CIC_NUS_6102:
				GPR[5].DW=0xFFFFFFFFC95973D5;
				GPR[14].DW=0x000000002449A366;
				break;
			case CIC_NUS_6103:
				GPR[5].DW=0xFFFFFFFF95315A28;
				GPR[14].DW=0x000000005BACA1DF;
				break;
			case CIC_NUS_6105:
				MMU.SW_VAddr(0xA4001004,0x8DA807FC);
				GPR[5].DW=0x000000005493FB9A;
				GPR[14].DW=0xFFFFFFFFC2C20384;
			case CIC_NUS_6106:
				GPR[5].DW=0xFFFFFFFFE067221F;
				GPR[14].DW=0x000000005CD2B70F;
				break;
			}
			GPR[20].DW=0x0000000000000001;
			GPR[23].DW=0x0000000000000000;
			GPR[24].DW=0x0000000000000003;
			GPR[31].DW=0xFFFFFFFFA4001550;
		}

		switch (CIC_Chip) {
		case CIC_NUS_6101: 
			GPR[22].DW=0x000000000000003F; 
			break;
		case CIC_NUS_6102: 
			GPR[1].DW=0x0000000000000001;
			GPR[2].DW=0x000000000EBDA536;
			GPR[3].DW=0x000000000EBDA536;
			GPR[4].DW=0x000000000000A536;
			GPR[12].DW=0xFFFFFFFFED10D0B3;
			GPR[13].DW=0x000000001402A4CC;
			GPR[15].DW=0x000000003103E121;
			GPR[22].DW=0x000000000000003F; 
			GPR[25].DW=0xFFFFFFFF9DEBB54F;
			break;
		case CIC_NUS_6103: 
			GPR[1].DW=0x0000000000000001;
			GPR[2].DW=0x0000000049A5EE96;
			GPR[3].DW=0x0000000049A5EE96;
			GPR[4].DW=0x000000000000EE96;
			GPR[12].DW=0xFFFFFFFFCE9DFBF7;
			GPR[13].DW=0xFFFFFFFFCE9DFBF7;
			GPR[15].DW=0x0000000018B63D28;
			GPR[22].DW=0x0000000000000078; 
			GPR[25].DW=0xFFFFFFFF825B21C9;
			break;
		case CIC_NUS_6105: 
			MMU.SW_VAddr(0xA4001000,0x3C0DBFC0);
			MMU.SW_VAddr(0xA4001008,0x25AD07C0);
			MMU.SW_VAddr(0xA400100C,0x31080080);
			MMU.SW_VAddr(0xA4001010,0x5500FFFC);
			MMU.SW_VAddr(0xA4001014,0x3C0DBFC0);
			MMU.SW_VAddr(0xA4001018,0x8DA80024);
			MMU.SW_VAddr(0xA400101C,0x3C0BB000);
			GPR[1].DW=0x0000000000000000;
			GPR[2].DW=0xFFFFFFFFF58B0FBF;
			GPR[3].DW=0xFFFFFFFFF58B0FBF;
			GPR[4].DW=0x0000000000000FBF;
			GPR[12].DW=0xFFFFFFFF9651F81E;
			GPR[13].DW=0x000000002D42AAC5;
			GPR[15].DW=0x0000000056584D60;
			GPR[22].DW=0x0000000000000091; 
			GPR[25].DW=0xFFFFFFFFCDCE565F;
			break;
		case CIC_NUS_6106: 
			GPR[1].DW=0x0000000000000000;
			GPR[2].DW=0xFFFFFFFFA95930A4;
			GPR[3].DW=0xFFFFFFFFA95930A4;
			GPR[4].DW=0x00000000000030A4;
			GPR[12].DW=0xFFFFFFFFBCB59510;
			GPR[13].DW=0xFFFFFFFFBCB59510;
			GPR[15].DW=0x000000007A3C07F4;
			GPR[22].DW=0x0000000000000085; 
			GPR[25].DW=0x00000000465E3F72;
			break;
		}
	} else {
		PROGRAM_COUNTER = 0xBFC00000;			
/*		PIF_Ram[36] = 0x00; PIF_Ram[39] = 0x3F; //common pif ram start values

		switch (CIC_Chip) {
		case CIC_NUS_6101: PIF_Ram[37] = 0x06; PIF_Ram[38] = 0x3F; break;
		case CIC_NUS_6102: PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x3F; break;
		case CIC_NUS_6103:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x78; break;
		case CIC_NUS_6105:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x91; break;
		case CIC_NUS_6106:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x85; break;
		}*/
	}
	FixFpuLocations();
}
#endif
