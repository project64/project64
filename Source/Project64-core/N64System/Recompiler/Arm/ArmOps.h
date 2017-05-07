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
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/Settings/DebugSettings.h>

class CArmRegInfo;

class CArmOps :
    protected CDebugSettings
{
public:
    enum ArmReg
    {
        Arm_R0 = 0,
        Arm_R1 = 1,
        Arm_R2 = 2,
        Arm_R3 = 3,
        Arm_R4 = 4,
        Arm_R5 = 5,
        Arm_R6 = 6,
        Arm_R7 = 7,
        Arm_R8 = 8,
        Arm_R9 = 9,
        Arm_R10 = 10,
        Arm_R11 = 11,
        Arm_R12 = 12,
        Arm_R13 = 13,
        ArmRegSP = 13,
        Arm_R14 = 14,
        ArmRegLR = 14,
        Arm_R15 = 15,
        ArmRegPC = 15,

        Arm_Unknown = -1,
        Arm_Any = -2,
    };

    enum ArmFpuSingle
    {
        Arm_S0 = 0,
        Arm_S1 = 1,
        Arm_S2 = 2,
        Arm_S3 = 3,
        Arm_S4 = 4,
        Arm_S5 = 5,
        Arm_S6 = 6,
        Arm_S7 = 7,
        Arm_S8 = 8,
        Arm_S9 = 9,
        Arm_S10 = 10,
        Arm_S11 = 11,
        Arm_S12 = 12,
        Arm_S13 = 13,
        Arm_S14 = 14,
        Arm_S15 = 15,
        Arm_S16 = 16,
        Arm_S17 = 17,
        Arm_S18 = 18,
        Arm_S19 = 19,
        Arm_S20 = 20,
        Arm_S21 = 21,
        Arm_S22 = 22,
        Arm_S23 = 23,
        Arm_S24 = 24,
        Arm_S25 = 25,
        Arm_S26 = 26,
        Arm_S27 = 27,
        Arm_S28 = 28,
        Arm_S29 = 29,
        Arm_S30 = 30,
        Arm_S31 = 31,
    };

    enum ArmFpuDouble
    {
        Arm_D0 = 0,
        Arm_D1 = 1,
        Arm_D2 = 2,
        Arm_D3 = 3,
        Arm_D4 = 4,
        Arm_D5 = 5,
        Arm_D6 = 6,
        Arm_D7 = 7,
        Arm_D8 = 8,
        Arm_D9 = 9,
        Arm_D10 = 10,
        Arm_D11 = 11,
        Arm_D12 = 12,
        Arm_D13 = 13,
        Arm_D14 = 14,
        Arm_D15 = 15,
    };

    enum ArmRegPushPop
    {
        ArmPushPop_R0 = 0x0001,
        ArmPushPop_R1 = 0x0002,
        ArmPushPop_R2 = 0x0004,
        ArmPushPop_R3 = 0x0008,
        ArmPushPop_R4 = 0x0010,
        ArmPushPop_R5 = 0x0020,
        ArmPushPop_R6 = 0x0040,
        ArmPushPop_R7 = 0x0080,
        ArmPushPop_R8 = 0x0100,
        ArmPushPop_R9 = 0x0200,
        ArmPushPop_R10 = 0x0400,
        ArmPushPop_R11 = 0x0800,
        ArmPushPop_R12 = 0x1000,
        ArmPushPop_R13 = 0x2000,
        ArmPushPop_SP = 0x2000,
        ArmPushPop_R14 = 0x4000,
        ArmPushPop_LR = 0x4000,
        ArmPushPop_R15 = 0x8000,
        ArmPushPop_PC = 0x8000,
    };

    enum ArmCompareType
    {
        ArmBranch_Equal = 0,               //Code = 0000
        ArmBranch_Notequal = 1,            //Code = 0001
        ArmBranch_GreaterThanOrEqual = 10, //Code = 1010
        ArmBranch_LessThan = 11,           //Code = 1011
        ArmBranch_GreaterThan = 12,        //Code = 1100
        ArmBranch_LessThanOrEqual = 13,    //Code = 1101
        ArmBranch_Always = 14,             //Code = 1110
    };

    enum ArmItMask
    {
        ItMask_None,
        ItMask_T,
        ItMask_E,
        ItMask_TT,
        ItMask_ET,
        ItMask_TE,
        ItMask_EE,
        ItMask_TTT,
        ItMask_ETT,
        ItMask_TET,
        ItMask_EET,
        ItMask_TTE,
        ItMask_ETE,
        ItMask_TEE,
        ItMask_EEE,
    };

protected:
    //Logging Functions
    static void WriteArmComment(const char * Comment);
    static void WriteArmLabel(const char * Label);

    static void AddArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);
    static void AddConstToArmReg(ArmReg DestReg, uint32_t Const);
    static void AddConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const);
    static void AndConstToVariable(void *Variable, const char * VariableName, uint32_t Const);
    static void AndConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const);
    static void AndArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);
    static void ArmBreakPoint(const char * FileName, uint32_t LineNumber);
    static void ArmNop(void);
    static void BranchLabel8(ArmCompareType CompareType, const char * Label);
    static void BranchLabel20(ArmCompareType CompareType, const char * Label);
    static void CallFunction(void * Function, const char * FunctionName);
    static void CompareArmRegToConst(ArmReg Reg, uint32_t value);
    static void CompareArmRegToArmReg(ArmReg Reg1, ArmReg Reg2);
    static void IfBlock(ArmItMask mask, ArmCompareType CompareType);
    static void LoadArmRegPointerByteToArmReg(ArmReg DestReg, ArmReg RegPointer, uint16_t offset);
    static void LoadArmRegPointerByteToArmReg(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift);
    static void LoadArmRegPointerToArmReg(ArmReg DestReg, ArmReg RegPointer, uint8_t Offset, const char * comment = NULL);
    static void LoadArmRegPointerToArmReg(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift);
    static void LoadArmRegPointerToFloatReg(ArmReg RegPointer, ArmFpuSingle Reg, uint8_t Offset);
    static void LoadFloatingPointControlReg(ArmReg DestReg);
    static void MoveArmRegArmReg(ArmReg DestReg, ArmReg SourceReg);
    static void MoveArmRegToVariable(ArmReg Reg, void * Variable, const char * VariableName);
    static void MoveConstToArmReg(ArmReg DestReg, uint16_t Const, const char * comment = NULL);
    static void MoveConstToArmRegTop(ArmReg DestReg, uint16_t Const, const char * comment = NULL);
    static void MoveConstToArmReg(ArmReg DestReg, uint32_t Const, const char * comment = NULL);
    static void MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void MoveFloatRegToVariable(ArmFpuSingle reg, void * Variable, const char * VariableName);
    static void MoveVariableToArmReg(void * Variable, const char * VariableName, ArmReg reg);
    static void MoveVariableToFloatReg(void * Variable, const char * VariableName, ArmFpuSingle reg);
    static void OrArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2, uint32_t shift);
    static void OrConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t value);
    static void OrConstToVariable(void * Variable, const char * VariableName, uint32_t value);
    static void MulF32(ArmFpuSingle DestReg, ArmFpuSingle SourceReg1, ArmFpuSingle SourceReg2);
    static void PushArmReg(uint16_t Registers);
    static void PopArmReg(uint16_t Registers);
    static void ShiftRightSignImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift);
    static void ShiftRightUnsignImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift);
    static void ShiftLeftImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift);
    static void SignExtendByte(ArmReg Reg);
    static void StoreArmRegToArmRegPointer(ArmReg DestReg, ArmReg RegPointer, uint8_t Offset, const char * comment = NULL);
    static void StoreArmRegToArmRegPointer(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift);
    static void StoreFloatingPointControlReg(ArmReg SourceReg);
    static void StoreFloatRegToArmRegPointer(ArmFpuSingle Reg, ArmReg RegPointer, uint8_t Offset);
    static void SubArmRegFromArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);
    static void SubConstFromArmReg(ArmReg Reg, ArmReg SourceReg, uint32_t Const);
    static void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void TestVariable(uint32_t Const, void * Variable, const char * VariableName);
    static void XorConstToArmReg(ArmReg DestReg, uint32_t value);
    static void XorArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg);
    static void XorArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);

    static void * GetAddressOf(int32_t value, ...);
    static void SetJump8(uint8_t * Loc, uint8_t * JumpLoc);
    static void SetJump20(uint32_t * Loc, uint32_t * JumpLoc);
    static void FlushPopArmReg(void);

    static CArmRegInfo m_RegWorkingSet;

protected:
    static const char * ArmRegName(ArmReg Reg);
    static uint32_t PushPopRegisterSize(uint16_t Registers);
    static std::string PushPopRegisterList(uint16_t Registers);

private:
    friend class CArmRegInfo;

    static void PreOpCheck(ArmReg DestReg, bool AllowedInItBlock, const char * FileName, uint32_t LineNumber);
    static void BreakPointNotification(const char * FileName, uint32_t LineNumber);
    static bool ArmCompareInverse(ArmCompareType CompareType);
    static ArmCompareType ArmCompareInverseType(ArmCompareType CompareType);
    static const char * ArmCompareSuffix(ArmCompareType CompareType);
    static const char * ArmFpuSingleName(ArmFpuSingle Reg);
    static const char * ArmItMaskName(ArmItMask mask);
    static const char * ArmCurrentItCondition();

    static void ProgressItBlock(void);

    static bool CanThumbCompressConst(uint32_t value);
    static uint16_t ThumbCompressConst(uint32_t value);

    static void AddCode8(uint8_t value);
    static void AddCode16(uint16_t value);
    static void AddCode32(uint32_t value);

    static bool m_InItBlock;
    static int m_ItBlockInstruction;
    static ArmCompareType m_ItBlockCompareType;
    static ArmItMask m_ItBlockMask;
    static ArmReg m_LastStoreReg;
    static uint16_t m_PopRegisters;
    static uint16_t m_PushRegisters;
};

#define AddressOf(Addr) CArmOps::GetAddressOf(5,(Addr))

#endif