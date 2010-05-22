#include "..\..\N64 System.h"
#include <float.h> //needed for fpu setting flag

extern CLog TlbLog;

const char * CRegistersName::GPR_Name[32] = {"r0","at","v0","v1","a0","a1","a2","a3",
					 "t0","t1","t2","t3","t4","t5","t6","t7",
					 "s0","s1","s2","s3","s4","s5","s6","s7",
					 "t8","t9","k0","k1","gp","sp","s8","ra"};

const char *CRegistersName::GPR_NameHi[32] = {"r0.HI","at.HI","v0.HI","v1.HI","a0.HI","a1.HI",
						"a2.HI","a3.HI","t0.HI","t1.HI","t2.HI","t3.HI",
						"t4.HI","t5.HI","t6.HI","t7.HI","s0.HI","s1.HI",
						"s2.HI","s3.HI","s4.HI","s5.HI","s6.HI","s7.HI",
						"t8.HI","t9.HI","k0.HI","k1.HI","gp.HI","sp.HI",
						"s8.HI","ra.HI"};

const char *CRegistersName::GPR_NameLo[32] = {"r0.LO","at.LO","v0.LO","v1.LO","a0.LO","a1.LO",
						"a2.LO","a3.LO","t0.LO","t1.LO","t2.LO","t3.LO",
						"t4.LO","t5.LO","t6.LO","t7.LO","s0.LO","s1.LO",
						"s2.LO","s3.LO","s4.LO","s5.LO","s6.LO","s7.LO",
						"t8.LO","t9.LO","k0.LO","k1.LO","gp.LO","sp.LO",
						"s8.LO","ra.LO"};

const char * CRegistersName::Cop0_Name[32] = {"Index","Random","EntryLo0","EntryLo1","Context","PageMask","Wired","",
					"BadVAddr","Count","EntryHi","Compare","Status","Cause","EPC","PRId",
					"Config","LLAddr","WatchLo","WatchHi","XContext","","","",
					"","","ECC","CacheErr","TagLo","TagHi","ErrEPC",""};

const char * CRegistersName::FPR_Name[32] = {"f0","f1","f2","f3","f4","f5","f6","f7",
					 "f8","f9","f10","f11","f12","f13","f14","f15",
					 "f16","f17","f18","f19","f20","f21","f22","f23",
					 "f24","f25","f26","f27","f28","f29","f30","f31"};

const char * CRegistersName::FPR_Ctrl_Name[32] = {"Revision","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","Unknown","Unknown","Unknown","Unknown",
					"Unknown","Unknown","FCSR"};

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

Mips_InterfaceReg::Mips_InterfaceReg(DWORD * _MipsInterface) :
	MI_INIT_MODE_REG(_MipsInterface[0]),
	MI_MODE_REG(_MipsInterface[0]),
	MI_VERSION_REG(_MipsInterface[1]),
	MI_NOOP_REG(_MipsInterface[1]),
	MI_INTR_REG(_MipsInterface[2]),
	MI_INTR_MASK_REG(_MipsInterface[3])
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

void CRegisters::FixFpuLocations ( void ) {	
	if ((STATUS_REGISTER & STATUS_FR) == 0) {
		for (int count = 0; count < 32; count ++) {
			FPR_S[count] = &FPR[count >> 1].F[count & 1];
			FPR_D[count] = &FPR[count >> 1].D;
		}
	} else {
		for (int count = 0; count < 32; count ++) {
			FPR_S[count] = &FPR[count].F[1];
			FPR_D[count] = &FPR[count].D;
		}
	}
}


void CRegisters::CheckInterrupts ( void ) {	
	if ((MI_INTR_MASK_REG & MI_INTR_REG) != 0) {
		FAKE_CAUSE_REGISTER |= CAUSE_IP2;
	} else  {
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP2;
	}

	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }

	if (( STATUS_REGISTER & FAKE_CAUSE_REGISTER & 0xFF00) != 0) {
		_N64System->ExternalEvent(ExecuteInterrupt);
	}
}

#ifdef hhh

void CRegisters::ExecuteInterruptException ( bool DelaySlot ) {
	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }
//	TlbLog.Log("%08X: ExecuteInterruptException %X", PROGRAM_COUNTER,FAKE_CAUSE_REGISTER);
	CAUSE_REGISTER = FAKE_CAUSE_REGISTER;
	CAUSE_REGISTER |= EXC_INT;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void CRegisters::ExecuteCopUnusableException ( bool DelaySlot, int Coprocessor ) {
//	TlbLog.Log("%08X: ExecuteCopUnusableException %X", PROGRAM_COUNTER,Coprocessor);
	CAUSE_REGISTER = EXC_CPU;
	if (Coprocessor == 1) { CAUSE_REGISTER |= 0x10000000; }
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}

void CRegisters::ExecuteSysCallException ( bool DelaySlot) {
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { 
		_Notify->DisplayError("EXL set in SysCall Exception");
	}
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { 
		_Notify->DisplayError("ERL set in SysCall Exception");
	}

	CAUSE_REGISTER = EXC_SYSCALL;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	PROGRAM_COUNTER = 0x80000180;
}


void CRegisters::ExecuteTLBMissException ( CMipsMemory * MMU, bool DelaySlot, DWORD BadVaddr ) {
	//TlbLog.Log("ExecuteTLBMissException PC: %X BadVaddr: %X",PROGRAM_COUNTER,BadVaddr);
	CAUSE_REGISTER = EXC_RMISS;
	BAD_VADDR_REGISTER = BadVaddr;
	CONTEXT_REGISTER &= 0xFF80000F;
	CONTEXT_REGISTER |= (BadVaddr >> 9) & 0x007FFFF0;
	ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
	if ((STATUS_REGISTER & STATUS_EXL) == 0) {
		if (DelaySlot) {
			CAUSE_REGISTER |= CAUSE_BD;
			EPC_REGISTER = PROGRAM_COUNTER - 4;
		} else {
			EPC_REGISTER = PROGRAM_COUNTER;
		}
		if (MMU->TLB_AddressDefined(BadVaddr)) {
			PROGRAM_COUNTER = 0x80000180;
		} else {
			PROGRAM_COUNTER = 0x80000000;
		}
		STATUS_REGISTER |= STATUS_EXL;
	} else {
		_Notify->BreakPoint(__FILE__,__LINE__);
		PROGRAM_COUNTER = 0x80000180;
	}
}

void CRegisters::UpdateRegisterAfterOpcode (float StepIncrease) {
	COUNT_REGISTER += (DWORD)StepIncrease;
//	RANDOM_REGISTER -= 1;
//	if ((int)RANDOM_REGISTER < (int)WIRED_REGISTER) { RANDOM_REGISTER = 31; }
	UpdateTimer(StepIncrease);
}

void CRegisters::SetCurrentRoundingModel   (ROUNDING_MODE RoundMode) {
	switch (RoundMode) {
	case ROUND_NEAR: _controlfp(_RC_NEAR,_MCW_RC);	break;
	case ROUND_CHOP: _controlfp(_RC_CHOP,_MCW_RC);	break;
	case ROUND_UP:   _controlfp(_RC_UP,  _MCW_RC);	break;
	case ROUND_DOWN: _controlfp(_RC_DOWN,_MCW_RC);	break;
	}
	
}

void CRegisters::ChangeDefaultRoundingModel (int Reg) {
	switch((FPCR[Reg] & 3)) {
	case 0: _RoundingModel = ROUND_NEAR; break;
	case 1: _RoundingModel = ROUND_CHOP; break;
	case 2: _RoundingModel = ROUND_UP;   break;
	case 3: _RoundingModel = ROUND_DOWN; break;
	}
}
#endif