#pragma once
#if defined(__arm__) || defined(_M_ARM)

#if !defined(_MSC_VER) && !defined(_Printf_format_string_)
#define _Printf_format_string_
#endif

#include <Project64-core/Settings/DebugSettings.h>

class CCodeBlock;
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
        ArmBranch_Equal = 0,               // Code = 0000
        ArmBranch_Notequal = 1,            // Code = 0001
        ArmBranch_GreaterThanOrEqual = 10, // Code = 1010
        ArmBranch_LessThan = 11,           // Code = 1011
        ArmBranch_GreaterThan = 12,        // Code = 1100
        ArmBranch_LessThanOrEqual = 13,    // Code = 1101
        ArmBranch_Always = 14,             // Code = 1110
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

    CArmOps(CCodeBlock & CodeBlock, CArmRegInfo & RegWorkingSet);

    // Logging functions
    void WriteArmComment(const char * Comment);
    void WriteArmLabel(const char * Label);

    void AddArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);
    void AddConstToArmReg(ArmReg DestReg, uint32_t Const);
    void AddConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const);
    void AndConstToVariable(void *Variable, const char * VariableName, uint32_t Const);
    void AndConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const);
    void AndArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);
    void ArmBreakPoint(const char * FileName, uint32_t LineNumber);
    void ArmNop(void);
    void BranchLabel8(ArmCompareType CompareType, const char * Label);
    void BranchLabel20(ArmCompareType CompareType, const char * Label);
    void CallFunction(void * Function, const char * FunctionName);
    void CompareArmRegToConst(ArmReg Reg, uint32_t value);
    void CompareArmRegToArmReg(ArmReg Reg1, ArmReg Reg2);
    void IfBlock(ArmItMask mask, ArmCompareType CompareType);
    void LoadArmRegPointerByteToArmReg(ArmReg DestReg, ArmReg RegPointer, uint16_t offset);
    void LoadArmRegPointerByteToArmReg(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift);
    void LoadArmRegPointerToArmReg(ArmReg DestReg, ArmReg RegPointer, uint8_t Offset, const char * comment = nullptr);
    void LoadArmRegPointerToArmReg(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift);
    void LoadArmRegPointerToFloatReg(ArmReg RegPointer, ArmFpuSingle Reg, uint8_t Offset);
    void LoadFloatingPointControlReg(ArmReg DestReg);
    void MoveArmRegArmReg(ArmReg DestReg, ArmReg SourceReg);
    void MoveArmRegToVariable(ArmReg Reg, void * Variable, const char * VariableName);
    void MoveConstToArmReg(ArmReg DestReg, uint16_t Const, const char * comment = nullptr);
    void MoveConstToArmRegTop(ArmReg DestReg, uint16_t Const, const char * comment = nullptr);
    void MoveConstToArmReg(ArmReg DestReg, uint32_t Const, const char * comment = nullptr);
    void MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName);
    void MoveFloatRegToVariable(ArmFpuSingle reg, void * Variable, const char * VariableName);
    void MoveVariableToArmReg(void * Variable, const char * VariableName, ArmReg reg);
    void MoveVariableToFloatReg(void * Variable, const char * VariableName, ArmFpuSingle reg);
    void OrArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2, uint32_t shift);
    void OrConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t value);
    void OrConstToVariable(void * Variable, const char * VariableName, uint32_t value);
    void MulF32(ArmFpuSingle DestReg, ArmFpuSingle SourceReg1, ArmFpuSingle SourceReg2);
    void PushArmReg(uint16_t Registers);
    void PopArmReg(uint16_t Registers);
    void ShiftRightSignImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift);
    void ShiftRightUnsignImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift);
    void ShiftLeftImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift);
    void SignExtendByte(ArmReg Reg);
    void StoreArmRegToArmRegPointer(ArmReg DestReg, ArmReg RegPointer, uint8_t Offset, const char * comment = nullptr);
    void StoreArmRegToArmRegPointer(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift);
    void StoreFloatingPointControlReg(ArmReg SourceReg);
    void StoreFloatRegToArmRegPointer(ArmFpuSingle Reg, ArmReg RegPointer, uint8_t Offset);
    void SubArmRegFromArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);
    void SubConstFromArmReg(ArmReg Reg, ArmReg SourceReg, uint32_t Const);
    void SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName);
    void TestVariable(uint32_t Const, void * Variable, const char * VariableName);
    void XorConstToArmReg(ArmReg DestReg, uint32_t value);
    void XorArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg);
    void XorArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2);

    void * GetAddressOf(int32_t value, ...);
    void SetJump8(uint8_t * Loc, uint8_t * JumpLoc);
    void SetJump20(uint32_t * Loc, uint32_t * JumpLoc);
    void FlushPopArmReg(void);

    const char * ArmRegName(ArmReg Reg);
    uint32_t PushPopRegisterSize(uint16_t Registers);
    std::string PushPopRegisterList(uint16_t Registers);

private:
    CArmOps(void);
    CArmOps(const CArmOps&);
    CArmOps& operator=(const CArmOps&);

    void CodeLog(_Printf_format_string_ const char * Text, ...);

    void PreOpCheck(ArmReg DestReg, bool AllowedInItBlock, const char * FileName, uint32_t LineNumber);
    void BreakPointNotification(const char * FileName, uint32_t LineNumber);
    bool ArmCompareInverse(ArmCompareType CompareType);
    ArmCompareType ArmCompareInverseType(ArmCompareType CompareType);
    const char * ArmCompareSuffix(ArmCompareType CompareType);
    const char * ArmFpuSingleName(ArmFpuSingle Reg);
    const char * ArmItMaskName(ArmItMask mask);
    const char * ArmCurrentItCondition();

    void ProgressItBlock(void);

    bool CanThumbCompressConst(uint32_t value);
    uint16_t ThumbCompressConst(uint32_t value);

    void AddCode8(uint8_t value);
    void AddCode16(uint16_t value);
    void AddCode32(uint32_t value);

    CCodeBlock & m_CodeBlock;
    CArmRegInfo & m_RegWorkingSet;
    bool m_InItBlock;
    int m_ItBlockInstruction;
    ArmCompareType m_ItBlockCompareType;
    ArmItMask m_ItBlockMask;
    ArmReg m_LastStoreReg;
    uint16_t m_PopRegisters;
    uint16_t m_PushRegisters;
};

#define AddressOf(Addr) CArmOps::GetAddressOf(5,(Addr))

#endif