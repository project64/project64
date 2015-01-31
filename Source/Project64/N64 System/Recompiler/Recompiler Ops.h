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
#pragma once

class CCodeSection;

class CRecompilerOps :
	protected CDebugSettings,
	protected CX86Ops,
	protected CSystemRegisters,
	protected CN64SystemSettings,
	protected CRecompilerSettings
{
protected:
	enum BRANCH_TYPE
	{
		BranchTypeCop1, BranchTypeRs, BranchTypeRsRt
	};

	typedef void ( * BranchFunction )();
	
	/************************** Branch functions  ************************/
	static void Compile_Branch         ( BranchFunction CompareFunc, BRANCH_TYPE BranchType, BOOL Link);
	static void Compile_BranchLikely   ( BranchFunction CompareFunc, BOOL Link);
	static void BNE_Compare            ( void );
	static void BEQ_Compare            ( void );
	static void BGTZ_Compare           ( void );
	static void BLEZ_Compare           ( void );
	static void BLTZ_Compare           ( void );
	static void BGEZ_Compare           ( void );
	static void COP1_BCF_Compare       ( void );
	static void COP1_BCT_Compare       ( void );

	/*************************  OpCode functions *************************/
	static void J              ( void );
	static void JAL            ( void );
	static void ADDI           ( void );
	static void ADDIU          ( void );
	static void SLTI           ( void );
	static void SLTIU          ( void );
	static void ANDI           ( void );
	static void ORI            ( void );
	static void XORI           ( void );
	static void LUI            ( void );
	static void DADDIU         ( void );
//	static void LDL            ( void );
//	static void LDR            ( void );
//	static void LB             ( void );
//	static void LH             ( void );
//	static void LWL            ( void );
//	static void LW             ( void );
//	static void LBU            ( void );
//	static void LHU            ( void );
//	static void LWR            ( void );
//	static void LWU            ( void );		//added by Witten
//	static void SB             ( void );
//	static void SH             ( void );
//	static void SWL            ( void );
//	static void SW             ( void );
//	static void SWR            ( void );
//	static void SDL            ( void );
//	static void SDR            ( void );
	static void CACHE          ( void );
//	static void LL             ( void );
//	static void LWC1           ( void );
//	static void LDC1           ( void );
//	static void LD             ( void );
//	static void SC             ( void );
//	static void SWC1           ( void );
//	static void SDC1           ( void );
//	static void SD             ( void );

	/********************** R4300i OpCodes: Special **********************/
	static void SPECIAL_SLL    ( void );
	static void SPECIAL_SRL    ( void );
	static void SPECIAL_SRA    ( void );
	static void SPECIAL_SLLV   ( void );
	static void SPECIAL_SRLV   ( void );
	static void SPECIAL_SRAV   ( void );
	static void SPECIAL_JR     ( void );
	static void SPECIAL_JALR   ( void );
	static void SPECIAL_SYSCALL( void );
	static void SPECIAL_MFLO   ( void );
	static void SPECIAL_MTLO   ( void );
	static void SPECIAL_MFHI   ( void );
	static void SPECIAL_MTHI   ( void );
	static void SPECIAL_DSLLV  ( void );
	static void SPECIAL_DSRLV  ( void );
	static void SPECIAL_DSRAV  ( void );
	static void SPECIAL_MULT   ( void );
	static void SPECIAL_MULTU  ( void );
	static void SPECIAL_DIV    ( void );
	static void SPECIAL_DIVU   ( void );
	static void SPECIAL_DMULT  ( void );
	static void SPECIAL_DMULTU ( void );
	static void SPECIAL_DDIV   ( void );
	static void SPECIAL_DDIVU  ( void );
	static void SPECIAL_ADD    ( void );
	static void SPECIAL_ADDU   ( void );
	static void SPECIAL_SUB    ( void );
	static void SPECIAL_SUBU   ( void );
	static void SPECIAL_AND    ( void );
	static void SPECIAL_OR     ( void );
	static void SPECIAL_XOR    ( void );
	static void SPECIAL_NOR    ( void );
	static void SPECIAL_SLT    ( void );
	static void SPECIAL_SLTU   ( void );
	static void SPECIAL_DADD   ( void );
	static void SPECIAL_DADDU  ( void );
	static void SPECIAL_DSUB   ( void );
	static void SPECIAL_DSUBU  ( void );
	static void SPECIAL_DSLL   ( void );
	static void SPECIAL_DSRL   ( void );
	static void SPECIAL_DSRA   ( void );
	static void SPECIAL_DSLL32 ( void );
	static void SPECIAL_DSRL32 ( void );
	static void SPECIAL_DSRA32 ( void );

	/************************** COP0 functions **************************/
	static void COP0_MF        ( void );
	static void COP0_MT        ( void );

	/************************** COP0 CO functions ***********************/
	static void COP0_CO_TLBR   ( void );
	static void COP0_CO_TLBWI  ( void );
	static void COP0_CO_TLBWR  ( void );
	static void COP0_CO_TLBP   ( void );
	static void COP0_CO_ERET   ( void );

	/************************** COP1 functions **************************/
	static void COP1_MF        ( void );
	static void COP1_DMF       ( void );
	static void COP1_CF        ( void );
	static void COP1_MT        ( void );
	static void COP1_DMT       ( void );
	static void COP1_CT        ( void );

	/************************** COP1: S functions ************************/
	static void COP1_S_ADD     ( void );
	static void COP1_S_SUB     ( void );
	static void COP1_S_MUL     ( void );
	static void COP1_S_DIV     ( void );
	static void COP1_S_ABS     ( void );
	static void COP1_S_NEG     ( void );
	static void COP1_S_SQRT    ( void );
	static void COP1_S_MOV     ( void );
	static void COP1_S_TRUNC_L ( void );
	static void COP1_S_CEIL_L  ( void );			//added by Witten
	static void COP1_S_FLOOR_L ( void );			//added by Witten
	static void COP1_S_ROUND_W ( void );
	static void COP1_S_TRUNC_W ( void );
	static void COP1_S_CEIL_W  ( void );			//added by Witten
	static void COP1_S_FLOOR_W ( void );
	static void COP1_S_CVT_D   ( void );
	static void COP1_S_CVT_W   ( void );
	static void COP1_S_CVT_L   ( void );
	static void COP1_S_CMP     ( void );

	/************************** COP1: D functions ************************/
	static void COP1_D_ADD     ( void );
	static void COP1_D_SUB     ( void );
	static void COP1_D_MUL     ( void );
	static void COP1_D_DIV     ( void );
	static void COP1_D_ABS     ( void );
	static void COP1_D_NEG     ( void );
	static void COP1_D_SQRT    ( void );
	static void COP1_D_MOV     ( void );
	static void COP1_D_TRUNC_L ( void );			//added by Witten
	static void COP1_D_CEIL_L  ( void );			//added by Witten
	static void COP1_D_FLOOR_L ( void );			//added by Witten
	static void COP1_D_ROUND_W ( void );
	static void COP1_D_TRUNC_W ( void );
	static void COP1_D_CEIL_W  ( void );			//added by Witten
	static void COP1_D_FLOOR_W ( void );			//added by Witten
	static void COP1_D_CVT_S   ( void );
	static void COP1_D_CVT_W   ( void );
	static void COP1_D_CVT_L   ( void );
	static void COP1_D_CMP     ( void );

	/************************** COP1: W functions ************************/
	static void COP1_W_CVT_S   ( void );
	static void COP1_W_CVT_D   ( void );

	/************************** COP1: L functions ************************/
	static void COP1_L_CVT_S   ( void );
	static void COP1_L_CVT_D   ( void );

	/************************** Other functions **************************/
	static void UnknownOpcode  ( void );
	
	
	static void BeforeCallDirect  ( CRegInfo  & RegSet );
	static void AfterCallDirect   ( CRegInfo  & RegSet );
	static void EnterCodeBlock    ( void );
	static void ExitCodeBlock     ( void );
	static void CompileReadTLBMiss  (DWORD VirtualAddress, x86Reg LookUpReg );
	static void CompileReadTLBMiss  (x86Reg AddressReg, x86Reg LookUpReg );
	static void CompileWriteTLBMiss (DWORD VirtualAddress, x86Reg LookUpReg );
	static void CompileWriteTLBMiss (x86Reg AddressReg, x86Reg LookUpReg );
	static void UpdateSyncCPU       (CRegInfo & RegSet, DWORD Cycles);
	static void UpdateCounters      (CRegInfo & RegSet, bool CheckTimer, bool ClearValues = false );
	static void CompileSystemCheck  ( DWORD TargetPC, const CRegInfo & RegSet );
	static void ChangeDefaultRoundingModel ( void );
	static void OverflowDelaySlot ( BOOL TestTimer );



	static STEP_TYPE      m_NextInstruction;
	static DWORD          m_CompilePC;
	static OPCODE         m_Opcode;
	static CRegInfo	      m_RegWorkingSet;
	static DWORD          m_BranchCompare;
	static CCodeSection * m_Section;

	/********* Helper Functions *********/
	typedef CRegInfo::REG_STATE REG_STATE;

	static inline REG_STATE         GetMipsRegState ( int Reg ) { return m_RegWorkingSet.GetMipsRegState(Reg); }
	static inline unsigned __int64  GetMipsReg      ( int Reg ) { return m_RegWorkingSet.GetMipsReg(Reg); }
	static inline __int64           GetMipsReg_S    ( int Reg ) { return m_RegWorkingSet.GetMipsReg_S(Reg); }
	static inline DWORD             GetMipsRegLo    ( int Reg ) { return m_RegWorkingSet.GetMipsRegLo(Reg); }
	static inline long              GetMipsRegLo_S  ( int Reg ) { return m_RegWorkingSet.GetMipsRegLo_S(Reg); }
	static inline DWORD             GetMipsRegHi    ( int Reg ) { return m_RegWorkingSet.GetMipsRegHi(Reg); }
	static inline long              GetMipsRegHi_S  ( int Reg ) { return m_RegWorkingSet.GetMipsRegHi_S(Reg); }
	static inline CX86Ops::x86Reg   GetMipsRegMapLo ( int Reg ) { return m_RegWorkingSet.GetMipsRegMapLo(Reg); }
	static inline CX86Ops::x86Reg   GetMipsRegMapHi ( int Reg ) { return m_RegWorkingSet.GetMipsRegMapHi(Reg); }

	static inline bool IsKnown       ( int Reg ) { return m_RegWorkingSet.IsKnown(Reg); }
	static inline bool IsUnknown     ( int Reg ) { return m_RegWorkingSet.IsUnknown(Reg); }
	static inline bool IsMapped      ( int Reg ) { return m_RegWorkingSet.IsMapped(Reg); }
	static inline bool IsConst       ( int Reg ) { return m_RegWorkingSet.IsConst(Reg); }
	static inline bool IsSigned      ( int Reg ) { return m_RegWorkingSet.IsSigned(Reg); }
	static inline bool IsUnsigned    ( int Reg ) { return m_RegWorkingSet.IsUnsigned(Reg); }
	static inline bool Is32Bit       ( int Reg ) { return m_RegWorkingSet.Is32Bit(Reg); }
	static inline bool Is64Bit       ( int Reg ) { return m_RegWorkingSet.Is64Bit(Reg); }
	static inline bool Is32BitMapped ( int Reg ) { return m_RegWorkingSet.Is32BitMapped(Reg); }
	static inline bool Is64BitMapped ( int Reg ) { return m_RegWorkingSet.Is64BitMapped(Reg); }

	static inline void FixRoundModel ( CRegInfo::FPU_ROUND RoundMethod )
	{
		m_RegWorkingSet.FixRoundModel(RoundMethod); 
	}
	static inline void ChangeFPURegFormat ( int Reg, CRegInfo::FPU_STATE OldFormat, CRegInfo::FPU_STATE NewFormat, CRegInfo::FPU_ROUND RoundingModel )
	{
		m_RegWorkingSet.ChangeFPURegFormat(Reg,OldFormat,NewFormat,RoundingModel); 
	}
	static inline void Load_FPR_ToTop ( int Reg, int RegToLoad, CRegInfo::FPU_STATE Format)
	{
		m_RegWorkingSet.Load_FPR_ToTop(Reg,RegToLoad,Format); 
	}
	static inline BOOL RegInStack ( int Reg, CRegInfo::FPU_STATE Format )
	{
		return m_RegWorkingSet.RegInStack(Reg,Format); 
	}
	static inline x86FpuValues StackPosition ( int Reg )
	{
		return m_RegWorkingSet.StackPosition(Reg); 
	}
	static inline void UnMap_AllFPRs ( void )
	{
		m_RegWorkingSet.UnMap_AllFPRs(); 
	}
	static inline void UnMap_FPR ( DWORD Reg, bool WriteBackValue )
	{
		m_RegWorkingSet.UnMap_FPR(Reg,WriteBackValue); 
	}

	static inline x86Reg FreeX86Reg ( void )
	{
		return m_RegWorkingSet.FreeX86Reg(); 
	}
	static inline x86Reg Free8BitX86Reg ( void )
	{
		return m_RegWorkingSet.Free8BitX86Reg();
	}
	static inline void Map_GPR_32bit ( int Reg, bool SignValue, int MipsRegToLoad )
	{
		m_RegWorkingSet.Map_GPR_32bit(Reg,SignValue,MipsRegToLoad);
	}
	static inline void Map_GPR_64bit ( int Reg, int MipsRegToLoad )
	{
		m_RegWorkingSet.Map_GPR_64bit(Reg,MipsRegToLoad);
	}
	static inline x86Reg Get_MemoryStack ( void )
	{
		return m_RegWorkingSet.Get_MemoryStack();
	}
	static inline x86Reg Map_MemoryStack ( x86Reg Reg, bool bMapRegister, bool LoadValue = true )
	{
		return m_RegWorkingSet.Map_MemoryStack(Reg,bMapRegister,LoadValue);
	}
	static inline x86Reg Map_TempReg ( x86Reg Reg, int MipsReg, BOOL LoadHiWord )
	{
		return m_RegWorkingSet.Map_TempReg(Reg,MipsReg,LoadHiWord); 
	}
	static inline void ProtectGPR ( DWORD Reg )
	{
		m_RegWorkingSet.ProtectGPR(Reg);
	}
	static inline void UnProtectGPR ( DWORD Reg )
	{
		m_RegWorkingSet.UnProtectGPR(Reg);
	}
	static inline void ResetX86Protection ( void )
	{
		m_RegWorkingSet.ResetX86Protection(); 
	}
	static inline x86Reg UnMap_TempReg ( void )
	{
		return m_RegWorkingSet.UnMap_TempReg(); 
	}
	static inline void UnMap_GPR ( DWORD Reg, bool WriteBackValue )
	{
		m_RegWorkingSet.UnMap_GPR(Reg,WriteBackValue); 
	}
	static inline bool UnMap_X86reg ( x86Reg Reg )
	{
		return m_RegWorkingSet.UnMap_X86reg(Reg); 
	}

public:
	static DWORD CompilePC ( void ) { return m_CompilePC; }
};
