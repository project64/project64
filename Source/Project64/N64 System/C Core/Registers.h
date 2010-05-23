#ifdef toremove

/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */


#define INDEX_REGISTER			_CP0[0]
#define RANDOM_REGISTER			_CP0[1]
#define ENTRYLO0_REGISTER		_CP0[2]
#define ENTRYLO1_REGISTER		_CP0[3]
#define CONTEXT_REGISTER		_CP0[4]
#define PAGE_MASK_REGISTER		_CP0[5]
#define WIRED_REGISTER			_CP0[6]
#define BAD_VADDR_REGISTER		_CP0[8]
#define COUNT_REGISTER			_CP0[9]
#define ENTRYHI_REGISTER		_CP0[10]
#define COMPARE_REGISTER		_CP0[11]
#define STATUS_REGISTER			_CP0[12]
#define CAUSE_REGISTER			_CP0[13]
#define EPC_REGISTER			_CP0[14]
#define CONFIG_REGISTER			_CP0[16]
#define TAGLO_REGISTER			_CP0[28]
#define TAGHI_REGISTER			_CP0[29]
#define ERROREPC_REGISTER		_CP0[30]
#define FAKE_CAUSE_REGISTER		_CP0[32]

#define COMPARE_REGISTER_NO		11
#define STATUS_REGISTER_NO		12
#define CAUSE_REGISTER_NO		13

#define REVISION_REGISTER		_FPCR[0]
#define FSTATUS_REGISTER		_FPCR[31]

#define GPR_S0					_GPR[16]
#define GPR_S1					_GPR[17]
#define GPR_S2					_GPR[18]
#define GPR_S3					_GPR[19]
#define GPR_S4					_GPR[20]
#define GPR_S5					_GPR[21]
#define GPR_S6					_GPR[22]
#define GPR_S7					_GPR[23]
#define GPR_SP					_GPR[29]
#define GPR_RA					_GPR[31]

#define RDRAM_CONFIG_REG		_RegRDRAM[0]
#define RDRAM_DEVICE_TYPE_REG	_RegRDRAM[0]
#define RDRAM_DEVICE_ID_REG		_RegRDRAM[1]
#define RDRAM_DELAY_REG			_RegRDRAM[2]
#define RDRAM_MODE_REG			_RegRDRAM[3]
#define RDRAM_REF_INTERVAL_REG	_RegRDRAM[4]
#define RDRAM_REF_ROW_REG		_RegRDRAM[5]
#define RDRAM_RAS_INTERVAL_REG	_RegRDRAM[6]
#define RDRAM_MIN_INTERVAL_REG	_RegRDRAM[7]
#define RDRAM_ADDR_SELECT_REG	_RegRDRAM[8]
#define RDRAM_DEVICE_MANUF_REG	_RegRDRAM[9]

#define SP_MEM_ADDR_REG			_RegSP[0]
#define SP_DRAM_ADDR_REG		_RegSP[1]
#define SP_RD_LEN_REG			_RegSP[2]
#define SP_WR_LEN_REG			_RegSP[3]
#define SP_STATUS_REG			_RegSP[4]
#define SP_DMA_FULL_REG			_RegSP[5]
#define SP_DMA_BUSY_REG			_RegSP[6]
#define SP_SEMAPHORE_REG		_RegSP[7]
#define SP_PC_REG				_RegSP[8]
#define SP_IBIST_REG			_RegSP[9]

#define DPC_START_REG			_RegDPC[0]
#define DPC_END_REG				_RegDPC[1]
#define DPC_CURRENT_REG			_RegDPC[2]
#define DPC_STATUS_REG			_RegDPC[3]
#define DPC_CLOCK_REG			_RegDPC[4]
#define DPC_BUFBUSY_REG			_RegDPC[5]
#define DPC_PIPEBUSY_REG		_RegDPC[6]
#define DPC_TMEM_REG			_RegDPC[7]

#define MI_INIT_MODE_REG		_RegMI[0]
#define MI_MODE_REG				_RegMI[0]
#define MI_VERSION_REG			_RegMI[1]
#define MI_NOOP_REG				_RegMI[1]
#define MI_INTR_REG				_RegMI[2]
#define MI_INTR_MASK_REG		_RegMI[3]

#define VI_STATUS_REG			_RegVI[0]
#define VI_CONTROL_REG			_RegVI[0]
#define VI_ORIGIN_REG 			_RegVI[1]
#define VI_DRAM_ADDR_REG		_RegVI[1]
#define VI_WIDTH_REG 			_RegVI[2]
#define VI_H_WIDTH_REG 			_RegVI[2]
#define VI_INTR_REG  			_RegVI[3]
#define VI_V_INTR_REG 			_RegVI[3]
#define VI_CURRENT_REG 			_RegVI[4]
#define VI_V_CURRENT_LINE_REG	_RegVI[4]
#define VI_BURST_REG  			_RegVI[5]
#define VI_TIMING_REG 			_RegVI[5]
#define VI_V_SYNC_REG 			_RegVI[6]
#define VI_H_SYNC_REG 			_RegVI[7]
#define VI_LEAP_REG  			_RegVI[8]
#define VI_H_SYNC_LEAP_REG		_RegVI[8]
#define VI_H_START_REG 			_RegVI[9]
#define VI_H_VIDEO_REG			_RegVI[9]
#define VI_V_START_REG 			_RegVI[10]
#define VI_V_VIDEO_REG			_RegVI[10]
#define VI_V_BURST_REG			_RegVI[11]
#define VI_X_SCALE_REG			_RegVI[12]
#define VI_Y_SCALE_REG			_RegVI[13]

#define AI_DRAM_ADDR_REG		_RegAI[0]
#define AI_LEN_REG				_RegAI[1]
#define AI_CONTROL_REG			_RegAI[2]
#define AI_STATUS_REG			_RegAI[3]
#define AI_DACRATE_REG			_RegAI[4]
#define AI_BITRATE_REG			_RegAI[5]

#define PI_DRAM_ADDR_REG		_RegPI[0]
#define PI_CART_ADDR_REG		_RegPI[1]
#define PI_RD_LEN_REG			_RegPI[2]
#define PI_WR_LEN_REG			_RegPI[3]
#define PI_STATUS_REG			_RegPI[4]
#define PI_BSD_DOM1_LAT_REG 	_RegPI[5]
#define PI_DOMAIN1_REG		 	_RegPI[5]
#define PI_BSD_DOM1_PWD_REG	 	_RegPI[6]
#define PI_BSD_DOM1_PGS_REG	 	_RegPI[7]
#define PI_BSD_DOM1_RLS_REG	 	_RegPI[8]
#define PI_BSD_DOM2_LAT_REG	 	_RegPI[9]
#define PI_DOMAIN2_REG		 	_RegPI[9]
#define PI_BSD_DOM2_PWD_REG	 	_RegPI[10]
#define PI_BSD_DOM2_PGS_REG	 	_RegPI[11]
#define PI_BSD_DOM2_RLS_REG	 	_RegPI[12]

#define RI_MODE_REG				_RegRI[0]
#define RI_CONFIG_REG			_RegRI[1]
#define RI_CURRENT_LOAD_REG		_RegRI[2]
#define RI_SELECT_REG			_RegRI[3]
#define RI_COUNT_REG			_RegRI[4]
#define RI_REFRESH_REG			_RegRI[4]
#define RI_LATENCY_REG			_RegRI[5]
#define RI_RERROR_REG			_RegRI[6]
#define RI_WERROR_REG			_RegRI[7]

#define SI_DRAM_ADDR_REG		_RegSI[0]
#define SI_PIF_ADDR_RD64B_REG	_RegSI[1]
#define SI_PIF_ADDR_WR64B_REG	_RegSI[2]
#define SI_STATUS_REG			_RegSI[3]

enum
{

	MI_MODE_INIT			= 0x0080,		/* Bit  7: init mode */
	MI_MODE_EBUS			= 0x0100,		/* Bit  8: ebus test mode */
	MI_MODE_RDRAM			= 0x0200,		/* Bit  9: RDRAM reg mode */

};



#define FPR_Type(Reg)	(Reg) == R4300i_COP1_S ? "S" : (Reg) == R4300i_COP1_D ? "D" :\
						(Reg) == R4300i_COP1_W ? "W" : "L"

typedef struct {
	DWORD      PROGRAM_COUNTER;
    MIPS_DWORD GPR[32];
	MIPS_DWORD FPR[32];
	DWORD      CP0[33];
	DWORD      FPCR[32];
	MIPS_DWORD HI;
	MIPS_DWORD LO;
	DWORD      RDRAM[10];
	DWORD      SP[10];
	DWORD      DPC[10];
	DWORD      MI[4];
	DWORD      VI[14];
	DWORD      AI[6];
	DWORD      PI[13];
	DWORD      RI[8];
	DWORD      SI[4];
	BYTE       PIF_Ram[0x40];
	int        DMAUsed;
} N64_REGISTERS;

#ifdef __cplusplus
extern "C" {
#endif

extern char *GPR_Name[32], *GPR_NameHi[32], *GPR_NameLo[32], *FPR_Name[32], *FPR_NameHi[32],
	*FPR_NameLo[32],*FPR_Ctrl_Name[32];
extern DWORD RegModValue;
//extern DWORD RegModValue, ViFieldNumber, LLAddr;
extern N64_REGISTERS Registers;

/*enum FPU_Format {
	FPU_Unkown,FPU_Dword, FPU_Qword, FPU_Float, FPU_Double
};*/

void ChangeMiIntrMask         ( void );
void ChangeMiModeReg          ( void );
void ChangeSpStatus           ( void );
void InitalizeR4300iRegisters ( int UsePif, int Country, int CIC_Chip );
void SetFpuLocations          ( void );
void SetupRegisters           ( N64_REGISTERS * n64_Registers );

#ifdef __cplusplus

}
#endif
#endif

BOOL Is8BitReg                ( int x86Reg);
void ChangeFPURegFormat       ( CBlockSection * Section, int Reg, CRegInfo::FPU_STATE OldFormat, CRegInfo::FPU_STATE NewFormat, CRegInfo::FPU_ROUND RoundingModel );
void Load_FPR_ToTop           ( CBlockSection * Section, int Reg, int RegToLoad, CRegInfo::FPU_STATE Format);
void Map_GPR_32bit            ( CBlockSection * Section, int Reg, BOOL SignValue, int MipsRegToLoad );
void Map_GPR_64bit            ( CBlockSection * Section, int Reg, int MipsRegToLoad );
int  Map_MemoryStack          ( CBlockSection * Section, int Reg, bool MapRegister );
int  Map_TempReg              ( CBlockSection * Section, int x86Reg, int MipsReg, BOOL LoadHiWord );
BOOL RegInStack               ( CBlockSection * Section, int Reg, int Format );
void ProtectGPR               ( CBlockSection * Section, DWORD Reg );
int  StackPosition            ( CBlockSection * Section, int Reg );
void UnMap_AllFPRs            ( CBlockSection * Section );
void UnMap_FPR                ( CBlockSection * Section, int Reg, int WriteBackValue );
void UnMap_GPR                ( CBlockSection * Section, DWORD Reg, int WriteBackValue );
BOOL UnMap_X86reg             ( CBlockSection * Section, DWORD x86Reg );
void UnProtectGPR             ( CBlockSection * Section, DWORD Reg );
void WriteBackRegisters       ( CBlockSection * Section );
void FixRoundModel            ( CBlockSection * Section, CRegInfo::FPU_ROUND RoundMethod );
