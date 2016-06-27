/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Mips/OpCode.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Settings/N64SystemSettings.h>
#include <Project64-core/Settings/RecompilerSettings.h>

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
    static void Compile_Branch         ( BranchFunction CompareFunc, BRANCH_TYPE BranchType, bool Link);
    static void Compile_BranchLikely   ( BranchFunction CompareFunc, bool Link);
    static void BNE_Compare();
    static void BEQ_Compare();
    static void BGTZ_Compare();
    static void BLEZ_Compare();
    static void BLTZ_Compare();
    static void BGEZ_Compare();
    static void COP1_BCF_Compare();
    static void COP1_BCT_Compare();

    /*************************  OpCode functions *************************/
    static void J              ();
    static void JAL            ();
    static void ADDI           ();
    static void ADDIU          ();
    static void SLTI           ();
    static void SLTIU          ();
    static void ANDI           ();
    static void ORI            ();
    static void XORI           ();
    static void LUI            ();
    static void DADDIU         ();
    //	static void LDL            ();
    //	static void LDR            ();
    //	static void LB             ();
    //	static void LH             ();
    //	static void LWL            ();
    //	static void LW             ();
    //	static void LBU            ();
    //	static void LHU            ();
    //	static void LWR            ();
    //	static void LWU            (); //added by Witten
    //	static void SB             ();
    //	static void SH             ();
    //	static void SWL            ();
    //	static void SW             ();
    //	static void SWR            ();
    //	static void SDL            ();
    //	static void SDR            ();
    static void CACHE          ();
    //	static void LL             ();
    //	static void LWC1           ();
    //	static void LDC1           ();
    //	static void LD             ();
    //	static void SC             ();
    //	static void SWC1           ();
    //	static void SDC1           ();
    //	static void SD             ();

    /********************** R4300i OpCodes: Special **********************/
    static void SPECIAL_SLL    ();
    static void SPECIAL_SRL    ();
    static void SPECIAL_SRA    ();
    static void SPECIAL_SLLV   ();
    static void SPECIAL_SRLV   ();
    static void SPECIAL_SRAV   ();
    static void SPECIAL_JR     ();
    static void SPECIAL_JALR   ();
    static void SPECIAL_SYSCALL();
    static void SPECIAL_MFLO   ();
    static void SPECIAL_MTLO   ();
    static void SPECIAL_MFHI   ();
    static void SPECIAL_MTHI   ();
    static void SPECIAL_DSLLV  ();
    static void SPECIAL_DSRLV  ();
    static void SPECIAL_DSRAV  ();
    static void SPECIAL_MULT   ();
    static void SPECIAL_MULTU  ();
    static void SPECIAL_DIV    ();
    static void SPECIAL_DIVU   ();
    static void SPECIAL_DMULT  ();
    static void SPECIAL_DMULTU ();
    static void SPECIAL_DDIV   ();
    static void SPECIAL_DDIVU  ();
    static void SPECIAL_ADD    ();
    static void SPECIAL_ADDU   ();
    static void SPECIAL_SUB    ();
    static void SPECIAL_SUBU   ();
    static void SPECIAL_AND    ();
    static void SPECIAL_OR     ();
    static void SPECIAL_XOR    ();
    static void SPECIAL_NOR    ();
    static void SPECIAL_SLT    ();
    static void SPECIAL_SLTU   ();
    static void SPECIAL_DADD   ();
    static void SPECIAL_DADDU  ();
    static void SPECIAL_DSUB   ();
    static void SPECIAL_DSUBU  ();
    static void SPECIAL_DSLL   ();
    static void SPECIAL_DSRL   ();
    static void SPECIAL_DSRA   ();
    static void SPECIAL_DSLL32 ();
    static void SPECIAL_DSRL32 ();
    static void SPECIAL_DSRA32 ();

    /************************** COP0 functions **************************/
    static void COP0_MF        ();
    static void COP0_MT        ();

    /************************** COP0 CO functions ***********************/
    static void COP0_CO_TLBR   ();
    static void COP0_CO_TLBWI  ();
    static void COP0_CO_TLBWR  ();
    static void COP0_CO_TLBP   ();
    static void COP0_CO_ERET   ();

    /************************** COP1 functions **************************/
    static void COP1_MF        ();
    static void COP1_DMF       ();
    static void COP1_CF        ();
    static void COP1_MT        ();
    static void COP1_DMT       ();
    static void COP1_CT        ();

    /************************** COP1: S functions ************************/
    static void COP1_S_ADD     ();
    static void COP1_S_SUB     ();
    static void COP1_S_MUL     ();
    static void COP1_S_DIV     ();
    static void COP1_S_ABS     ();
    static void COP1_S_NEG     ();
    static void COP1_S_SQRT    ();
    static void COP1_S_MOV     ();
    static void COP1_S_ROUND_L ();
    static void COP1_S_TRUNC_L ();
    static void COP1_S_CEIL_L  (); //added by Witten
    static void COP1_S_FLOOR_L (); //added by Witten
    static void COP1_S_ROUND_W ();
    static void COP1_S_TRUNC_W ();
    static void COP1_S_CEIL_W  (); //added by Witten
    static void COP1_S_FLOOR_W ();
    static void COP1_S_CVT_D   ();
    static void COP1_S_CVT_W   ();
    static void COP1_S_CVT_L   ();
    static void COP1_S_CMP     ();

    /************************** COP1: D functions ************************/
    static void COP1_D_ADD     ();
    static void COP1_D_SUB     ();
    static void COP1_D_MUL     ();
    static void COP1_D_DIV     ();
    static void COP1_D_ABS     ();
    static void COP1_D_NEG     ();
    static void COP1_D_SQRT    ();
    static void COP1_D_MOV     ();
    static void COP1_D_ROUND_L ();
    static void COP1_D_TRUNC_L (); //added by Witten
    static void COP1_D_CEIL_L  (); //added by Witten
    static void COP1_D_FLOOR_L (); //added by Witten
    static void COP1_D_ROUND_W ();
    static void COP1_D_TRUNC_W ();
    static void COP1_D_CEIL_W  (); //added by Witten
    static void COP1_D_FLOOR_W (); //added by Witten
    static void COP1_D_CVT_S   ();
    static void COP1_D_CVT_W   ();
    static void COP1_D_CVT_L   ();
    static void COP1_D_CMP     ();

    /************************** COP1: W functions ************************/
    static void COP1_W_CVT_S   ();
    static void COP1_W_CVT_D   ();

    /************************** COP1: L functions ************************/
    static void COP1_L_CVT_S   ();
    static void COP1_L_CVT_D   ();

    /************************** Other functions **************************/
    static void UnknownOpcode  ();

    static void BeforeCallDirect(CRegInfo & RegSet);
    static void AfterCallDirect(CRegInfo & RegSet);
    static void EnterCodeBlock();
    static void ExitCodeBlock();
    static void CompileReadTLBMiss(uint32_t VirtualAddress, x86Reg LookUpReg);
    static void CompileReadTLBMiss(x86Reg AddressReg, x86Reg LookUpReg);
    static void CompileWriteTLBMiss(x86Reg AddressReg, x86Reg LookUpReg);
    static void UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles);
    static void UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues = false);
    static void CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet);
    static void ChangeDefaultRoundingModel();
    static void OverflowDelaySlot(bool TestTimer);

    static STEP_TYPE      m_NextInstruction;
    static uint32_t       m_CompilePC;
    static OPCODE         m_Opcode;
    static CX86RegInfo    m_RegWorkingSet;
    static uint32_t       m_BranchCompare;
    static CCodeSection * m_Section;

    /********* Helper Functions *********/
    typedef CRegInfo::REG_STATE REG_STATE;

    static REG_STATE         GetMipsRegState ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegState(Reg); }
    static uint64_t          GetMipsReg      ( int32_t Reg ) { return m_RegWorkingSet.GetMipsReg(Reg); }
    static int64_t           GetMipsReg_S    ( int32_t Reg ) { return m_RegWorkingSet.GetMipsReg_S(Reg); }
    static uint32_t          GetMipsRegLo    ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegLo(Reg); }
    static int32_t           GetMipsRegLo_S  ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegLo_S(Reg); }
    static uint32_t          GetMipsRegHi    ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegHi(Reg); }
    static int32_t           GetMipsRegHi_S  ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegHi_S(Reg); }
    static CX86Ops::x86Reg   GetMipsRegMapLo ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegMapLo(Reg); }
    static CX86Ops::x86Reg   GetMipsRegMapHi ( int32_t Reg ) { return m_RegWorkingSet.GetMipsRegMapHi(Reg); }

    static bool IsKnown       ( int32_t Reg ) { return m_RegWorkingSet.IsKnown(Reg); }
    static bool IsUnknown     ( int32_t Reg ) { return m_RegWorkingSet.IsUnknown(Reg); }
    static bool IsMapped      ( int32_t Reg ) { return m_RegWorkingSet.IsMapped(Reg); }
    static bool IsConst       ( int32_t Reg ) { return m_RegWorkingSet.IsConst(Reg); }
    static bool IsSigned      ( int32_t Reg ) { return m_RegWorkingSet.IsSigned(Reg); }
    static bool IsUnsigned    ( int32_t Reg ) { return m_RegWorkingSet.IsUnsigned(Reg); }
    static bool Is32Bit       ( int32_t Reg ) { return m_RegWorkingSet.Is32Bit(Reg); }
    static bool Is64Bit       ( int32_t Reg ) { return m_RegWorkingSet.Is64Bit(Reg); }
    static bool Is32BitMapped ( int32_t Reg ) { return m_RegWorkingSet.Is32BitMapped(Reg); }
    static bool Is64BitMapped ( int32_t Reg ) { return m_RegWorkingSet.Is64BitMapped(Reg); }

    static void FixRoundModel ( CRegInfo::FPU_ROUND RoundMethod )
    {
        m_RegWorkingSet.FixRoundModel(RoundMethod);
    }
    static void ChangeFPURegFormat ( int32_t Reg, CRegInfo::FPU_STATE OldFormat, CRegInfo::FPU_STATE NewFormat, CRegInfo::FPU_ROUND RoundingModel )
    {
        m_RegWorkingSet.ChangeFPURegFormat(Reg,OldFormat,NewFormat,RoundingModel);
    }
    static void Load_FPR_ToTop ( int32_t Reg, int32_t RegToLoad, CRegInfo::FPU_STATE Format)
    {
        m_RegWorkingSet.Load_FPR_ToTop(Reg,RegToLoad,Format);
    }
    static bool RegInStack ( int32_t Reg, CRegInfo::FPU_STATE Format )
    {
        return m_RegWorkingSet.RegInStack(Reg,Format);
    }
    static x86FpuValues StackPosition ( int32_t Reg )
    {
        return m_RegWorkingSet.StackPosition(Reg);
    }
    static void UnMap_AllFPRs()
    {
        m_RegWorkingSet.UnMap_AllFPRs();
    }
    static void UnMap_FPR ( uint32_t Reg, bool WriteBackValue )
    {
        m_RegWorkingSet.UnMap_FPR(Reg,WriteBackValue);
    }

    static x86Reg FreeX86Reg()
    {
        return m_RegWorkingSet.FreeX86Reg();
    }
    static x86Reg Free8BitX86Reg()
    {
        return m_RegWorkingSet.Free8BitX86Reg();
    }
    static void Map_GPR_32bit ( int32_t Reg, bool SignValue, int32_t MipsRegToLoad )
    {
        m_RegWorkingSet.Map_GPR_32bit(Reg,SignValue,MipsRegToLoad);
    }
    static void Map_GPR_64bit ( int32_t Reg, int32_t MipsRegToLoad )
    {
        m_RegWorkingSet.Map_GPR_64bit(Reg,MipsRegToLoad);
    }
    static x86Reg Get_MemoryStack()
    {
        return m_RegWorkingSet.Get_MemoryStack();
    }
    static x86Reg Map_MemoryStack ( x86Reg Reg, bool bMapRegister, bool LoadValue = true )
    {
        return m_RegWorkingSet.Map_MemoryStack(Reg,bMapRegister,LoadValue);
    }
    static x86Reg Map_TempReg ( x86Reg Reg, int32_t MipsReg, bool LoadHiWord )
    {
        return m_RegWorkingSet.Map_TempReg(Reg,MipsReg,LoadHiWord);
    }
    static void ProtectGPR ( uint32_t Reg )
    {
        m_RegWorkingSet.ProtectGPR(Reg);
    }
    static void UnProtectGPR ( uint32_t Reg )
    {
        m_RegWorkingSet.UnProtectGPR(Reg);
    }
    static void ResetX86Protection()
    {
        m_RegWorkingSet.ResetX86Protection();
    }
    static x86Reg UnMap_TempReg()
    {
        return m_RegWorkingSet.UnMap_TempReg();
    }
    static void UnMap_GPR ( uint32_t Reg, bool WriteBackValue )
    {
        m_RegWorkingSet.UnMap_GPR(Reg,WriteBackValue);
    }
    static bool UnMap_X86reg ( x86Reg Reg )
    {
        return m_RegWorkingSet.UnMap_X86reg(Reg);
    }

public:
    static uint32_t CompilePC() { return m_CompilePC; }
};
