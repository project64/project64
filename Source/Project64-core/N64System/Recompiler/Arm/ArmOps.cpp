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
#include "stdafx.h"

#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmOps.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmOpCode.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>

CArmRegInfo CArmOps::m_RegWorkingSet;
bool CArmOps::m_InItBlock = false;
int CArmOps::m_ItBlockInstruction = 0;
CArmOps::ArmCompareType CArmOps::m_ItBlockCompareType;
CArmOps::ArmItMask CArmOps::m_ItBlockMask;
CArmOps::ArmReg CArmOps::m_LastStoreReg;
uint16_t CArmOps::m_PopRegisters = 0;
uint16_t CArmOps::m_PushRegisters = 0;

/**************************************************************************
* Logging Functions                                                       *
**************************************************************************/
void CArmOps::WriteArmComment(const char * Comment)
{
    CPU_Message("");
    CPU_Message("      // %s", Comment);
}

void CArmOps::WriteArmLabel(const char * Label)
{
    CPU_Message("");
    CPU_Message("      %s:", Label);
}

void CArmOps::AddArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg <= 7 && SourceReg1 <= 7 && SourceReg2 <= 7)
    {
        CPU_Message("      add\t%s,%s,%s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        ArmThumbOpcode op = { 0 };
        op.Reg.rt = DestReg;
        op.Reg.rn = SourceReg1;
        op.Reg.rm = SourceReg2;
        op.Reg.opcode = 0xC;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      add.w\t%s,%s,%s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        Arm32Opcode op = { 0 };
        op.imm5.rn = SourceReg1;
        op.imm5.s = 0;
        op.imm5.opcode = 0x758;

        op.imm5.rm = SourceReg2;
        op.imm5.type = 0;
        op.imm5.imm2 = 0;
        op.imm5.rd = DestReg;
        op.imm5.imm3 = 0;
        op.imm5.opcode2 = 0;
        AddCode32(op.Hex);
    }
}

void CArmOps::AddConstToArmReg(ArmReg DestReg, uint32_t Const)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    AddConstToArmReg(DestReg, DestReg, Const);
}

void CArmOps::AndConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (CanThumbCompressConst(Const))
    {
        CPU_Message("      and\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        uint16_t CompressedConst = ThumbCompressConst(Const);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = SourceReg;
        op.imm8_3_1.s = 0;
        op.imm8_3_1.opcode = 0;
        op.imm8_3_1.i = (CompressedConst >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedConst & 0xFF;
        op.imm8_3_1.rd = DestReg;
        op.imm8_3_1.imm3 = (CompressedConst >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, Const);
        AndArmRegToArmReg(DestReg, SourceReg, TempReg);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
}

void CArmOps::AndConstToVariable(void *Variable, const char * VariableName, uint32_t Const)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    ArmReg TempReg1 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    if (TempReg1 == Arm_Unknown || TempReg2 == Arm_Unknown)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    MoveConstToArmReg(TempReg1, (uint32_t)Variable, VariableName);
    LoadArmRegPointerToArmReg(TempReg2, TempReg1, 0);
    AndConstToArmReg(TempReg2, TempReg2, Const);
    StoreArmRegToArmRegPointer(TempReg2, TempReg1, 0);

    m_RegWorkingSet.SetArmRegProtected(TempReg1, false);
    m_RegWorkingSet.SetArmRegProtected(TempReg2, false);
}

void CArmOps::AddConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg == SourceReg && Const == 0)
    {
        //ignore
    }
    else if ((Const & 0xFFFFFFF8) == 0 && DestReg <= 7 && SourceReg <= 7)
    {
        CPU_Message("      adds\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        ArmThumbOpcode op = { 0 };
        op.Imm3.rd = DestReg;
        op.Imm3.rn = SourceReg;
        op.Imm3.imm3 = Const;
        op.Imm3.opcode = 0xE;
        AddCode16(op.Hex);
    }
    else if ((Const & 0xFFFFFF00) == 0 && DestReg <= 7 && DestReg == SourceReg)
    {
        CPU_Message("      adds\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        ArmThumbOpcode op = { 0 };
        op.Imm8.imm8 = Const;
        op.Imm8.rdn = DestReg;
        op.Imm8.opcode = 0x6;
        AddCode16(op.Hex);
    }
    else if ((Const & 0xFFFFFF80) == 0xFFFFFF80 && DestReg <= 7 && DestReg == SourceReg)
    {
        CPU_Message("      sub\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (~Const) + 1);
        ArmThumbOpcode op = { 0 };
        op.Imm8.imm8 = ((~Const) + 1) & 0xFF;
        op.Imm8.rdn = DestReg;
        op.Imm8.opcode = 0x7;
        AddCode16(op.Hex);
    }
    else if (CanThumbCompressConst(Const))
    {
        CPU_Message("      add.w\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        uint16_t CompressedConst = ThumbCompressConst(Const);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = SourceReg;
        op.imm8_3_1.s = 0;
        op.imm8_3_1.opcode = 0x8;
        op.imm8_3_1.i = (CompressedConst >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedConst & 0xFF;
        op.imm8_3_1.rd = DestReg;
        op.imm8_3_1.imm3 = (CompressedConst >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else if ((Const & 0xFFFF8000) == 0xFFFF8000 || (Const & 0xFFFF0000) == 0)
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, Const);
        AddArmRegToArmReg(DestReg, TempReg, SourceReg);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
    else
    {
        CPU_Message("%s: DestReg = %X Const = %X", __FUNCTION__, DestReg, Const);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmOps::AndArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg <= 0x7 && SourceReg2 <= 0x7 && SourceReg1 == DestReg)
    {
        CPU_Message("      ands\t%s, %s", ArmRegName(DestReg), ArmRegName(SourceReg2));
        ArmThumbOpcode op = { 0 };
        op.Reg2.rn = DestReg;
        op.Reg2.rm = SourceReg2;
        op.Reg2.opcode = 0x100;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      and.w\t%s, %s, %s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        Arm32Opcode op = { 0 };
        op.imm5.rn = SourceReg1;
        op.imm5.s = 0;
        op.imm5.opcode = 0x750;

        op.imm5.rm = SourceReg2;
        op.imm5.type = 0;
        op.imm5.imm2 = 0;
        op.imm5.rd = DestReg;
        op.imm5.imm3 = 0;
        op.imm5.opcode2 = 0;

        AddCode32(op.Hex);
    }
}

void CArmOps::ArmBreakPoint(const char * FileName, uint32_t LineNumber)
{
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToArmReg(Arm_R1, LineNumber);
    MoveConstToArmReg(Arm_R0, (uint32_t)FileName, FileName);
    CallFunction(AddressOf(&BreakPointNotification), "BreakPointNotification");
    m_RegWorkingSet.AfterCallDirect();
}

void CArmOps::ArmNop(void)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    CPU_Message("      nop");
    AddCode16(0xbf00);
}

void CArmOps::BranchLabel8(ArmCompareType CompareType, const char * Label)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    CPU_Message("      b%s\t%s", ArmCompareSuffix(CompareType), Label);
    ArmThumbOpcode op = { 0 };
    if (CompareType == ArmBranch_Always)
    {
        op.BranchImm.imm = 0;
        op.BranchImm.opcode = 0x1C;
    }
    else
    {
        op.BranchImmCond.imm = 0;
        op.BranchImmCond.opcode = 0xD;
        op.BranchImmCond.cond = CompareType;
    }
    AddCode16(op.Hex);
}

void CArmOps::BranchLabel20(ArmCompareType CompareType, const char * Label)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    CPU_Message("      b%s\t%s", ArmCompareSuffix(CompareType), Label);
    Arm32Opcode op = { 0 };
    op.Branch20.imm6 = 0;
    op.Branch20.cond = CompareType == ArmBranch_Always ? 0 : CompareType;
    op.Branch20.S = 0;
    op.Branch20.Opcode = 0x1E;
    op.Branch20.imm11 = 0;
    op.Branch20.J2 = 0;
    op.Branch20.val12 = CompareType == ArmBranch_Always ? 1 : 0;
    op.Branch20.J1 = 0;
    op.Branch20.val14 = 0x2;
    AddCode32(op.Hex);
}

void CArmOps::CallFunction(void * Function, const char * FunctionName)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    ArmReg reg = Arm_R4;
    MoveConstToArmReg(reg, (uint32_t)Function, FunctionName);
    int32_t Offset = (int32_t)Function - (int32_t)*g_RecompPos;
    ArmThumbOpcode op = { 0 };
    op.Branch.reserved = 0;
    op.Branch.rm = reg;
    op.Branch.opcode = 0x8F;
    CPU_Message("      blx\t%s", ArmRegName(reg));
    AddCode16(op.Hex);
}

void CArmOps::MoveArmRegToVariable(ArmReg Reg, void * Variable, const char * VariableName)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);
    bool WasRegProtected = m_RegWorkingSet.GetArmRegProtected(Reg);
    if (!WasRegProtected)
    {
        m_RegWorkingSet.SetArmRegProtected(Reg, true);
    }

    ArmReg VariableReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    MoveConstToArmReg(VariableReg, (uint32_t)Variable, VariableName);
    StoreArmRegToArmRegPointer(Reg, VariableReg, 0);

    m_RegWorkingSet.SetArmRegProtected(VariableReg, false);
    if (!WasRegProtected)
    {
        m_RegWorkingSet.SetArmRegProtected(Reg, false);
    }
}

void CArmOps::MoveConstToArmReg(ArmReg Reg, uint16_t value, const char * comment)
{
    if (Reg == m_LastStoreReg)
    {
        ArmNop();
    }
    PreOpCheck(Arm_Unknown, true, __FILE__, __LINE__);
    if ((value & 0xFF00) == 0 && Reg <= 7)
    {
        CPU_Message("      mov%s\t%s, #0x%X\t; %s", m_InItBlock ? ArmCurrentItCondition() : "s", ArmRegName(Reg), (uint32_t)value, comment != NULL ? comment : stdstr_f("0x%X", (uint32_t)value).c_str());
        ArmThumbOpcode op = { 0 };
        op.Imm8.imm8 = value;
        op.Imm8.rdn = Reg;
        op.Imm8.opcode = 0x4;
        AddCode16(op.Hex);
    }
    else if (CanThumbCompressConst(value))
    {
        CPU_Message("      mov%s.w\t%s, #0x%X\t; %s", m_InItBlock ? ArmCurrentItCondition() : "", ArmRegName(Reg), (uint32_t)value, comment != NULL ? comment : stdstr_f("0x%X", (uint32_t)value).c_str());
        uint16_t CompressedValue = ThumbCompressConst(value);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = 0xF;
        op.imm8_3_1.s = 0;
        op.imm8_3_1.opcode = 0x2;
        op.imm8_3_1.i = (CompressedValue >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedValue & 0xFF;
        op.imm8_3_1.rd = Reg;
        op.imm8_3_1.imm3 = (CompressedValue >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      movw%s\t%s, #0x%X\t; %s", m_InItBlock ? ArmCurrentItCondition() : "", ArmRegName(Reg), (uint32_t)value, comment != NULL ? comment : stdstr_f("0x%X", (uint32_t)value).c_str());

        Arm32Opcode op = { 0 };
        op.imm16.opcode = ArmMOV_IMM16;
        op.imm16.i = ((value >> 11) & 0x1);
        op.imm16.opcode2 = ArmMOVW_IMM16;
        op.imm16.imm4 = ((value >> 12) & 0xF);
        op.imm16.reserved = 0;
        op.imm16.imm3 = ((value >> 8) & 0x7);
        op.imm16.rd = Reg;
        op.imm16.imm8 = (value & 0xFF);
        AddCode32(op.Hex);
    }

    if (m_InItBlock)
    {
        ProgressItBlock();
    }
}

void CArmOps::MoveConstToArmRegTop(ArmReg DestReg, uint16_t Const, const char * comment)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    CPU_Message("      movt\t%s, %s", ArmRegName(DestReg), comment != NULL ? stdstr_f("#0x%X\t; %s", (uint32_t)Const, comment).c_str() : stdstr_f("#%d\t; 0x%X", (uint32_t)Const, (uint32_t)Const).c_str());

    Arm32Opcode op = { 0 };
    op.imm16.opcode = ArmMOV_IMM16;
    op.imm16.i = ((Const >> 11) & 0x1);
    op.imm16.opcode2 = ArmMOVT_IMM16;
    op.imm16.imm4 = ((Const >> 12) & 0xF);
    op.imm16.reserved = 0;
    op.imm16.imm3 = ((Const >> 8) & 0x7);
    op.imm16.rd = DestReg;
    op.imm16.imm8 = (Const & 0xFF);
    AddCode32(op.Hex);
}

void CArmOps::CompareArmRegToConst(ArmReg Reg, uint32_t value)
{
    if (Reg == m_LastStoreReg)
    {
        ArmNop();
    }
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    if (Reg <= 0x7 && (value & 0xFFFFFF00) == 0)
    {
        CPU_Message("      cmp\t%s, #%d\t; 0x%X", ArmRegName(Reg), value, value);
        ArmThumbOpcode op = { 0 };
        op.Imm8.imm8 = value;
        op.Imm8.rdn = Reg;
        op.Imm8.opcode = 0x5;
        AddCode16(op.Hex);
    }
    else if (CanThumbCompressConst(value))
    {
        CPU_Message("      cmp\t%s, #%d\t; 0x%X", ArmRegName(Reg), value, value);
        uint16_t CompressedValue = ThumbCompressConst(value);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = Reg;
        op.imm8_3_1.s = 1;
        op.imm8_3_1.opcode = 0xD;
        op.imm8_3_1.i = (CompressedValue >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedValue & 0xFF;
        op.imm8_3_1.rd = 0xF;
        op.imm8_3_1.imm3 = (CompressedValue >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, value);
        CompareArmRegToArmReg(Reg, TempReg);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
}

void CArmOps::CompareArmRegToArmReg(ArmReg Reg1, ArmReg Reg2)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    if (Reg1 <= 0x7 && Reg2 <= 0x7)
    {
        CPU_Message("      cmp\t%s, %s", ArmRegName(Reg1), ArmRegName(Reg2));
        ArmThumbOpcode op = { 0 };
        op.Reg2.rn = Reg1;
        op.Reg2.rm = Reg2;
        op.Reg2.opcode = 0x10A;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      cmp.w\t%s, %s", ArmRegName(Reg1), ArmRegName(Reg2));
        Arm32Opcode op = { 0 };
        op.imm5.rn = Reg1;
        op.imm5.s = 1;
        op.imm5.opcode = 0x75D;

        op.imm5.rm = Reg2;
        op.imm5.type = 0;
        op.imm5.imm2 = 0;
        op.imm5.rd = 0xF;
        op.imm5.imm3 = 0;
        op.imm5.opcode2 = 0;

        AddCode32(op.Hex);
    }
}

void CArmOps::IfBlock(ArmItMask mask, ArmCompareType CompareType)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    CPU_Message("      it%s\t%s", ArmItMaskName(mask), ArmCompareSuffix(CompareType));
    m_InItBlock = true;
    m_ItBlockInstruction = 0;
    m_ItBlockCompareType = CompareType;
    m_ItBlockMask = mask;

    uint8_t computed_mask = 0;
    switch (mask)
    {
    case ItMask_None: computed_mask = 0x8; break;
    case ItMask_E: computed_mask = ArmCompareInverse(CompareType) ? 0x4 : 0xC; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    ArmThumbOpcode op = { 0 };
    op.It.mask = computed_mask;
    op.It.firstcond = CompareType;
    op.It.opcode = 0xBF;
    AddCode16(op.Hex);
}

void CArmOps::LoadArmRegPointerByteToArmReg(ArmReg DestReg, ArmReg RegPointer, uint16_t offset)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if ((DestReg > 0x7 || RegPointer > 0x7 || (offset & ~0x1f) != 0))
    {
        if ((offset & (~0xFFF)) != 0)
        {
            CPU_Message("      RegPointer: %d Reg: %d Offset: 0x%X", RegPointer, DestReg, offset);
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        CPU_Message("      ldrb.w\t%s, [%s, #%d]", ArmRegName(DestReg), ArmRegName(RegPointer), (uint32_t)offset);
        Arm32Opcode op = { 0 };
        op.imm12.rt = DestReg;
        op.imm12.rn = RegPointer;
        op.imm12.imm = offset;
        op.imm12.opcode = 0xF89;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      ldrb\t%s, [%s%s%s]", ArmRegName(DestReg), ArmRegName(RegPointer), offset == 0 ? "" : ",", offset == 0 ? "" : stdstr_f("#%d", offset).c_str());
        ArmThumbOpcode op = { 0 };
        op.Imm5.rt = DestReg;
        op.Imm5.rn = RegPointer;
        op.Imm5.imm5 = offset;
        op.Imm5.opcode = 0xF;
        AddCode16(op.Hex);
    }
}

void CArmOps::LoadArmRegPointerByteToArmReg(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if ((DestReg > 0x7 || RegPointer > 0x7 || RegPointer2 > 0x7) && (shift & ~3) == 0)
    {
        CPU_Message("      ldrb\t%s, [%s,%s]", ArmRegName(DestReg), ArmRegName(RegPointer), ArmRegName(RegPointer2));
        Arm32Opcode op = { 0 };
        op.uint16.rn = RegPointer;
        op.uint16.opcode = 0xF81;
        op.uint16.rm = RegPointer2;
        op.uint16.imm2 = (shift & 3);
        op.uint16.reserved = 0;
        op.uint16.rt = DestReg;
        AddCode32(op.Hex);
    }
    else if (shift == 0 && DestReg <= 0x7 && RegPointer <= 0x7 && RegPointer2 <= 0x7)
    {
        CPU_Message("      ldrb\t%s, [%s,%s]", ArmRegName(DestReg), ArmRegName(RegPointer), ArmRegName(RegPointer2));
        ArmThumbOpcode op = { 0 };
        op.Reg.rm = RegPointer2;
        op.Reg.rt = DestReg;
        op.Reg.rn = RegPointer;
        op.Reg.opcode = 0x2E;
        AddCode16(op.Hex);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmOps::LoadArmRegPointerToArmReg(ArmReg DestReg, ArmReg RegPointer, uint8_t Offset, const char * comment)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg > 0x7 || RegPointer > 0x7 || (Offset & (~0x7C)) != 0)
    {
        if ((Offset & (~0xFFF)) != 0)
        {
            CPU_Message("      RegPointer: %d Reg: %d Offset: 0x%X", RegPointer, DestReg, Offset);
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        CPU_Message("      ldr.w\t%s, [%s, #%d]%s%s", ArmRegName(DestReg), ArmRegName(RegPointer), (uint32_t)Offset, comment != NULL ? "\t; " : "", comment != NULL ? comment : "");
        Arm32Opcode op = { 0 };
        op.imm12.rt = DestReg;
        op.imm12.rn = RegPointer;
        op.imm12.imm = Offset;
        op.imm12.opcode = 0xF8D;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      ldr\t%s, [%s, #%d]%s%s", ArmRegName(DestReg), ArmRegName(RegPointer), (uint32_t)Offset, comment != NULL ? "\t; " : "", comment != NULL ? comment : "");
        ArmThumbOpcode op = { 0 };
        op.Imm5.rt = DestReg;
        op.Imm5.rn = RegPointer;
        op.Imm5.imm5 = Offset >> 2;
        op.Imm5.opcode = ArmLDR_ThumbImm;
        AddCode16(op.Hex);
    }
}

void CArmOps::LoadArmRegPointerToArmReg(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if ((shift & ~3) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (shift == 0 && DestReg <= 0x7 && RegPointer <= 0x7 && RegPointer2 <= 0x7)
    {
        CPU_Message("      ldr\t%s, [%s,%s]", ArmRegName(DestReg), ArmRegName(RegPointer), ArmRegName(RegPointer2));
        ArmThumbOpcode op = { 0 };
        op.Reg.rm = RegPointer2;
        op.Reg.rt = DestReg;
        op.Reg.rn = RegPointer;
        op.Reg.opcode = 0x2C;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      ldr.w\t%s, [%s, %s, lsl #%d]", ArmRegName(DestReg), ArmRegName(RegPointer), ArmRegName(RegPointer2), shift);
        Arm32Opcode op = { 0 };
        op.imm2.rm = RegPointer2;
        op.imm2.imm = shift;
        op.imm2.Opcode2 = 0;
        op.imm2.rt = DestReg;
        op.imm2.rn = RegPointer;
        op.imm2.opcode = 0xF85;
        AddCode32(op.Hex);
    }
}

void CArmOps::LoadArmRegPointerToFloatReg(ArmReg RegPointer, ArmFpuSingle Reg, uint8_t Offset)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    if (Offset != 0)
    {
        CPU_Message("      vldr\t%s, [%s, #%d]", ArmFpuSingleName(Reg), ArmRegName(RegPointer), (uint32_t)Offset);
    }
    else
    {
        CPU_Message("      vldr\t%s, [%s]", ArmFpuSingleName(Reg), ArmRegName(RegPointer));
    }
    Arm32Opcode op = { 0 };
    op.RnVdImm8.Rn = RegPointer;
    op.RnVdImm8.op3 = 1;
    op.RnVdImm8.D = Reg & 1;
    op.RnVdImm8.U = 1;
    op.RnVdImm8.op2 = 0xED;

    op.RnVdImm8.imm8 = Offset;
    op.RnVdImm8.op1 = 0xA;
    op.RnVdImm8.vd = Reg >> 1;
    AddCode32(op.Hex);
}

void CArmOps::MoveArmRegArmReg(ArmReg DestReg, ArmReg SourceReg)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmOps::LoadFloatingPointControlReg(ArmReg DestReg)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    CPU_Message("      vmrs\t%s, fpscr", ArmRegName(DestReg));
    Arm32Opcode op = { 0 };
    op.fpscr.opcode2 = 0xA10;
    op.fpscr.rt = DestReg;
    op.fpscr.opcode = 0xEEF1;
    AddCode32(op.Hex);
}

void CArmOps::MoveConstToArmReg(ArmReg DestReg, uint32_t value, const char * comment)
{
    if (CanThumbCompressConst(value))
    {
        PreOpCheck(DestReg, false, __FILE__, __LINE__);

        CPU_Message("      mov.w\t%s, #0x%X\t; %s", ArmRegName(DestReg), (uint32_t)value, comment != NULL ? comment : stdstr_f("0x%X", (uint32_t)value).c_str());
        uint16_t CompressedValue = ThumbCompressConst(value);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = 0xF;
        op.imm8_3_1.s = 0;
        op.imm8_3_1.opcode = 0x2;
        op.imm8_3_1.i = (CompressedValue >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedValue & 0xFF;
        op.imm8_3_1.rd = DestReg;
        op.imm8_3_1.imm3 = (CompressedValue >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        MoveConstToArmReg(DestReg, (uint16_t)(value & 0xFFFF), comment);
        uint16_t TopValue = (uint16_t)((value >> 16) & 0xFFFF);
        if (TopValue != 0)
        {
            MoveConstToArmRegTop(DestReg, TopValue, comment != NULL ? "" : NULL);
        }
    }
}

void CArmOps::MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    ArmReg TempReg1 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);

    MoveConstToArmReg(TempReg1, Const);
    MoveConstToArmReg(TempReg2, (uint32_t)Variable, VariableName);
    StoreArmRegToArmRegPointer(TempReg1, TempReg2, 0);

    m_RegWorkingSet.SetArmRegProtected(TempReg1, false);
    m_RegWorkingSet.SetArmRegProtected(TempReg2, false);
}

void CArmOps::MoveFloatRegToVariable(ArmFpuSingle reg, void * Variable, const char * VariableName)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    MoveConstToArmReg(Arm_R0, (uint32_t)Variable, VariableName);
    StoreFloatRegToArmRegPointer(reg, Arm_R0, 0);
}

void CArmOps::MoveVariableToArmReg(void * Variable, const char * VariableName, ArmReg reg)
{
    MoveConstToArmReg(reg, (uint32_t)Variable, VariableName);
    LoadArmRegPointerToArmReg(reg, reg, 0);
}

void CArmOps::MoveVariableToFloatReg(void * Variable, const char * VariableName, ArmFpuSingle reg)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    MoveConstToArmReg(Arm_R0, (uint32_t)Variable, VariableName);
    LoadArmRegPointerToFloatReg(Arm_R0, reg, 0);
}

void CArmOps::OrArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2, uint32_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (shift == 0 && SourceReg1 == SourceReg2 && SourceReg1 <= 7 && SourceReg2 <= 7)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    CPU_Message("      orr.w\t%s, %s, %s%s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2), shift ? stdstr_f(", lsl #%d", shift).c_str() : "");
    Arm32Opcode op = { 0 };
    op.imm5.rn = SourceReg1;
    op.imm5.s = 0;
    op.imm5.opcode = 0x752;

    op.imm5.rm = SourceReg2;
    op.imm5.type = 0;
    op.imm5.imm2 = (shift & 3);
    op.imm5.rd = DestReg;
    op.imm5.imm3 = ((shift >> 2) & 7);
    op.imm5.opcode2 = 0;

    AddCode32(op.Hex);
}

void CArmOps::OrConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t value)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (value == 0)
    {
        return;
    }

    else if (CanThumbCompressConst(value))
    {
        uint16_t CompressedValue = ThumbCompressConst(value);
        CPU_Message("      orr\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), value);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = SourceReg;
        op.imm8_3_1.s = 0;
        op.imm8_3_1.opcode = 0x2;
        op.imm8_3_1.i = (CompressedValue >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedValue & 0xFF;
        op.imm8_3_1.rd = DestReg;
        op.imm8_3_1.imm3 = (CompressedValue >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, value);
        OrArmRegToArmReg(DestReg, SourceReg, TempReg, 0);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
}

void CArmOps::OrConstToVariable(void * Variable, const char * VariableName, uint32_t value)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    ArmReg TempReg1 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    if (TempReg1 == Arm_Unknown || TempReg2 == Arm_Unknown)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    MoveConstToArmReg(TempReg1, (uint32_t)Variable, VariableName);

    LoadArmRegPointerToArmReg(TempReg2, TempReg1, 0);
    OrConstToArmReg(TempReg2, TempReg2, value);
    StoreArmRegToArmRegPointer(TempReg2, TempReg1, 0);

    m_RegWorkingSet.SetArmRegProtected(TempReg1, false);
    m_RegWorkingSet.SetArmRegProtected(TempReg2, false);
}

void CArmOps::MulF32(ArmFpuSingle DestReg, ArmFpuSingle SourceReg1, ArmFpuSingle SourceReg2)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    CPU_Message("      vmul.f32\t%s, %s, %s", ArmFpuSingleName(DestReg), ArmFpuSingleName(SourceReg1), ArmFpuSingleName(SourceReg2));
    Arm32Opcode op = { 0 };
    op.VnVmVd.vn = SourceReg1 >> 1;
    op.VnVmVd.op1 = 0x2;
    op.VnVmVd.d = DestReg & 1;
    op.VnVmVd.op2 = 0x1DC;

    op.VnVmVd.vm = SourceReg2 >> 1;
    op.VnVmVd.op3 = 0;
    op.VnVmVd.m = SourceReg2 & 1;
    op.VnVmVd.op4 = 0;
    op.VnVmVd.n = SourceReg1 & 1;
    op.VnVmVd.sz = 0;
    op.VnVmVd.op5 = 0x5;
    op.VnVmVd.vd = DestReg >> 1;

    AddCode32(op.Hex);
}

void CArmOps::PushArmReg(uint16_t Registers)
{
    if (m_PopRegisters != 0)
    {
        if (Registers == m_PopRegisters)
        {
            CPU_Message("%s: Ignoring Push/Pop", __FUNCTION__);
            m_PopRegisters = 0;
            PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);
            return;
        }
        ArmNop();
    }
    if (m_PushRegisters != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    if (Registers == 0)
    {
        return;
    }
    if ((Registers & ArmPushPop_SP) != 0) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    if ((Registers & ArmPushPop_PC) != 0) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    if ((Registers & ArmPushPop_LR) == 0)
    {
        m_PushRegisters = Registers;
    }

    std::string pushed = PushPopRegisterList(Registers);
    if ((PushPopRegisterSize(Registers) % 8) != 0)
    {
        WriteTrace(TraceRecompiler, TraceError, "pushed: %s", pushed.c_str());
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if ((Registers & ArmPushPop_R8) != 0 ||
        (Registers & ArmPushPop_R9) != 0 ||
        (Registers & ArmPushPop_R10) != 0 ||
        (Registers & ArmPushPop_R11) != 0 ||
        (Registers & ArmPushPop_R12) != 0)
    {
        CPU_Message("%X:      push\t{%s}", (int32_t)*g_RecompPos, pushed.c_str());

        Arm32Opcode op = { 0 };
        op.PushPop.register_list = Registers;
        op.PushPop.opcode = 0xE92D;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      push\t%s", pushed.c_str());
        bool lr = (Registers & ArmPushPop_LR) != 0;
        Registers &= Registers & ~ArmPushPop_LR;

        ArmThumbOpcode op = { 0 };
        op.Push.register_list = (uint8_t)Registers;
        op.Push.m = lr ? 1 : 0;
        op.Push.opcode = ArmPUSH;
        AddCode16(op.Hex);
    }
}

void CArmOps::PopArmReg(uint16_t Registers)
{
    if (Registers == 0)
    {
        return;
    }
    if (m_PopRegisters != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (m_PushRegisters == 0 && (Registers & ArmPushPop_PC) == 0)
    {
        CPU_Message("%s: Setting m_PushRegisters: %X Registers: %X", __FUNCTION__, m_PushRegisters, Registers);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (m_PushRegisters != Registers && (Registers & ArmPushPop_PC) == 0)
    {
        CPU_Message("%s: Setting m_PushRegisters: %X Registers: %X", __FUNCTION__, m_PushRegisters, Registers);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if ((Registers & ArmPushPop_SP) != 0) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    if ((Registers & ArmPushPop_LR) != 0) { g_Notify->BreakPoint(__FILE__, __LINE__); }

    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);
    m_PopRegisters = Registers;
    if ((m_PopRegisters & ArmPushPop_PC) != 0)
    {
        FlushPopArmReg();
    }
}

void CArmOps::FlushPopArmReg(void)
{
    if (m_PopRegisters == 0)
    {
        return;
    }
    std::string pushed = PushPopRegisterList(m_PopRegisters);
    if ((PushPopRegisterSize(m_PopRegisters) % 8) != 0)
    {
        WriteTrace(TraceRecompiler, TraceError, "pop: %s", pushed.c_str());
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if ((m_PopRegisters & ArmPushPop_R8) != 0 ||
        (m_PopRegisters & ArmPushPop_R9) != 0 ||
        (m_PopRegisters & ArmPushPop_R10) != 0 ||
        (m_PopRegisters & ArmPushPop_R11) != 0 ||
        (m_PopRegisters & ArmPushPop_R12) != 0)
    {
        CPU_Message("%X      pop\t{%s}", (int32_t)*g_RecompPos, pushed.c_str());

        Arm32Opcode op = { 0 };
        op.PushPop.register_list = m_PopRegisters;
        op.PushPop.opcode = 0xE8BD;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      pop\t%s", pushed.c_str());
        bool pc = (m_PopRegisters & ArmPushPop_PC) != 0;
        m_PopRegisters &= ~ArmPushPop_PC;

        ArmThumbOpcode op = { 0 };
        op.Pop.register_list = (uint8_t)m_PopRegisters;
        op.Pop.p = pc ? 1 : 0;
        op.Pop.opcode = ArmPOP;
        AddCode16(op.Hex);
    }
    m_PopRegisters = 0;
    m_PushRegisters = 0;
}

uint32_t CArmOps::PushPopRegisterSize(uint16_t Registers)
{
    static uint32_t RegisterList[] =
    {
        ArmPushPop_R0, ArmPushPop_R1, ArmPushPop_R2, ArmPushPop_R3, ArmPushPop_R4,
        ArmPushPop_R5, ArmPushPop_R6, ArmPushPop_R7, ArmPushPop_R8, ArmPushPop_R9,
        ArmPushPop_R10, ArmPushPop_R11, ArmPushPop_R12, ArmPushPop_LR, ArmPushPop_PC
    };
    uint32_t size = 0;
    for (uint32_t i = 0; i < (sizeof(RegisterList) / sizeof(RegisterList[0])); i++)
    {
        if ((Registers & RegisterList[i]) != 0)
        {
            size += 4;
        }
    }
    return size;
}

std::string CArmOps::PushPopRegisterList(uint16_t Registers)
{
    static uint32_t PushPopRegisterList[] =
    {
        ArmPushPop_R0, ArmPushPop_R1, ArmPushPop_R2, ArmPushPop_R3, ArmPushPop_R4,
        ArmPushPop_R5, ArmPushPop_R6, ArmPushPop_R7, ArmPushPop_R8, ArmPushPop_R9,
        ArmPushPop_R10, ArmPushPop_R11, ArmPushPop_R12, ArmPushPop_LR, ArmPushPop_PC
    };

    static ArmReg RegisterList[] =
    {
        Arm_R0, Arm_R1, Arm_R2, Arm_R3, Arm_R4,
        Arm_R5, Arm_R6, Arm_R7, Arm_R8, Arm_R9,
        Arm_R10, Arm_R11, Arm_R12, ArmRegLR, ArmRegPC,
    };

    std::string RegisterResult;
    for (uint32_t i = 0; i < (sizeof(PushPopRegisterList) / sizeof(PushPopRegisterList[0])); i++)
    {
        if ((Registers & PushPopRegisterList[i]) != 0)
        {
            RegisterResult += stdstr_f("%s%s", RegisterResult.length() > 0 ? ", " : "", ArmRegName(RegisterList[i]));
        }
    }
    return RegisterResult;
}

void CArmOps::ShiftRightSignImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if ((shift & (~0x1F)) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (DestReg > 0x7 || SourceReg > 0x7)
    {
        CPU_Message("      asrs.w\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (uint32_t)shift);
        Arm32Opcode op = { 0 };
        op.imm5.rn = 0xF;
        op.imm5.s = 0;
        op.imm5.opcode = 0x752;

        op.imm5.rm = SourceReg;
        op.imm5.type = 2;
        op.imm5.imm2 = shift & 3;
        op.imm5.rd = DestReg;
        op.imm5.imm3 = (shift >> 2) & 7;
        op.imm5.opcode2 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      asrs\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (uint32_t)shift);

        ArmThumbOpcode op = { 0 };
        op.Imm5.rt = DestReg;
        op.Imm5.rn = SourceReg;
        op.Imm5.imm5 = shift;
        op.Imm5.opcode = 0x2;
        AddCode16(op.Hex);
    }
}

void CArmOps::ShiftRightUnsignImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if ((shift & (~0x1F)) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (DestReg > 0x7 || SourceReg > 0x7)
    {
        CPU_Message("      lsrs.w\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (uint32_t)shift);
        Arm32Opcode op = { 0 };
        op.imm5.rn = 0xF;
        op.imm5.s = 0;
        op.imm5.opcode = 0x752;

        op.imm5.rm = SourceReg;
        op.imm5.type = 1;
        op.imm5.imm2 = shift & 3;
        op.imm5.rd = DestReg;
        op.imm5.imm3 = (shift >> 2) & 7;
        op.imm5.opcode2 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      lsrs\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (uint32_t)shift);

        ArmThumbOpcode op = { 0 };
        op.Imm5.rt = DestReg;
        op.Imm5.rn = SourceReg;
        op.Imm5.imm5 = shift;
        op.Imm5.opcode = 0x1;
        AddCode16(op.Hex);
    }
}

void CArmOps::ShiftLeftImmed(ArmReg DestReg, ArmReg SourceReg, uint32_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg > 0x7 || SourceReg > 0x7 || (shift & (~0x1F)) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    CPU_Message("      lsls\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (uint32_t)shift);

    ArmThumbOpcode op = { 0 };
    op.Imm5.rt = DestReg;
    op.Imm5.rn = SourceReg;
    op.Imm5.imm5 = shift;
    op.Imm5.opcode = 0x0;
    AddCode16(op.Hex);
}

void CArmOps::SignExtendByte(ArmReg Reg)
{
    if (Reg > 0x7)
    {
        CPU_Message("      sxtb.w\t%s, %s", ArmRegName(Reg), ArmRegName(Reg));
        Arm32Opcode op = { 0 };
        op.rotate.opcode = 0xFA4F;
        op.rotate.rm = Reg;
        op.rotate.rotate = 0;
        op.rotate.opcode2 = 2;
        op.rotate.rd = Reg;
        op.rotate.opcode3 = 0xF;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      sxtb\t%s, %s", ArmRegName(Reg), ArmRegName(Reg));
        ArmThumbOpcode op = { 0 };
        op.Reg2.rn = Reg;
        op.Reg2.rm = Reg;
        op.Reg2.opcode = 0x2C9;
        AddCode16(op.Hex);
    }
}

void CArmOps::StoreArmRegToArmRegPointer(ArmReg DestReg, ArmReg RegPointer, uint8_t Offset, const char * comment)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg > 0x7 || RegPointer > 0x7 || (Offset & (~0x7C)) != 0)
    {
        if ((Offset & (~0xFFF)) != 0) { g_Notify->BreakPoint(__FILE__, __LINE__); return; }

        CPU_Message("      str\t%s, [%s, #%d]%s%s", ArmRegName(DestReg), ArmRegName(RegPointer), (uint32_t)Offset, comment != NULL ? "\t; " : "", comment != NULL ? comment : "");
        Arm32Opcode op = { 0 };
        op.imm12.rt = DestReg;
        op.imm12.rn = RegPointer;
        op.imm12.imm = Offset;
        op.imm12.opcode = 0xF8C;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      str\t%s, [%s, #%d]%s%s", ArmRegName(DestReg), ArmRegName(RegPointer), (uint32_t)Offset, comment != NULL ? "\t; " : "", comment != NULL ? comment : "");
        ArmThumbOpcode op = { 0 };
        op.Imm5.rt = DestReg;
        op.Imm5.rn = RegPointer;
        op.Imm5.imm5 = Offset >> 2;
        op.Imm5.opcode = ArmSTR_ThumbImm;
        AddCode16(op.Hex);
    }
    m_LastStoreReg = DestReg;
}

void CArmOps::StoreArmRegToArmRegPointer(ArmReg DestReg, ArmReg RegPointer, ArmReg RegPointer2, uint8_t shift)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg > 0x7 || RegPointer > 0x7 || RegPointer2 > 0x7 || shift != 0)
    {
        CPU_Message("      str.w\t%s, [%s, %s%s]", ArmRegName(DestReg), ArmRegName(RegPointer), ArmRegName(RegPointer2), shift != 0 ? stdstr_f(", lsl #%d", shift).c_str() : "");
        Arm32Opcode op = { 0 };
        op.imm2.rm = RegPointer2;
        op.imm2.imm = shift;
        op.imm2.Opcode2 = 0;
        op.imm2.rt = DestReg;
        op.imm2.rn = RegPointer;
        op.imm2.opcode = 0xF84;
        AddCode32(op.Hex);
    }
    else
    {
        CPU_Message("      str\t%s, [%s, %s]", ArmRegName(DestReg), ArmRegName(RegPointer), ArmRegName(RegPointer2));
        ArmThumbOpcode op = { 0 };
        op.Reg.rt = DestReg;
        op.Reg.rn = RegPointer;
        op.Reg.rm = RegPointer2;
        op.Reg.opcode = 0x28;
        AddCode16(op.Hex);
    }
}

void CArmOps::StoreFloatingPointControlReg(ArmReg SourceReg)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    CPU_Message("      vmsr\tfpscr, %s", ArmRegName(SourceReg));
    Arm32Opcode op = { 0 };
    op.fpscr.opcode2 = 0xA10;
    op.fpscr.rt = SourceReg;
    op.fpscr.opcode = 0xEEE1;
    AddCode32(op.Hex);
}

void CArmOps::StoreFloatRegToArmRegPointer(ArmFpuSingle Reg, ArmReg RegPointer, uint8_t Offset)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    if (Offset != 0)
    {
        CPU_Message("      vstr\t%s, [%s, #%d]", ArmFpuSingleName(Reg), ArmRegName(RegPointer), (uint32_t)Offset);
    }
    else
    {
        CPU_Message("      vstr\t%s, [%s]", ArmFpuSingleName(Reg), ArmRegName(RegPointer));
    }
    Arm32Opcode op = { 0 };
    op.RnVdImm8.Rn = RegPointer;
    op.RnVdImm8.op3 = 0;
    op.RnVdImm8.D = Reg & 1;
    op.RnVdImm8.U = 1;
    op.RnVdImm8.op2 = 0xED;

    op.RnVdImm8.imm8 = Offset;
    op.RnVdImm8.op1 = 0xA;
    op.RnVdImm8.vd = Reg >> 1;
    AddCode32(op.Hex);
}

void CArmOps::SubArmRegFromArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg <= 7 && SourceReg1 <= 7 && SourceReg2 <= 7)
    {
        CPU_Message("      subs\t%s,%s,%s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        ArmThumbOpcode op = { 0 };
        op.Reg.rt = DestReg;
        op.Reg.rn = SourceReg1;
        op.Reg.rm = SourceReg2;
        op.Reg.opcode = 0xD;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      sub.w\t%s, %s, %s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        Arm32Opcode op = { 0 };
        op.imm5.rn = SourceReg1;
        op.imm5.s = 0;
        op.imm5.opcode = 0x75D;

        op.imm5.rm = SourceReg2;
        op.imm5.type = 0;
        op.imm5.imm2 = 0;
        op.imm5.rd = DestReg;
        op.imm5.imm3 = 0;
        op.imm5.opcode2 = 0;
        AddCode32(op.Hex);
    }
}

void CArmOps::SubConstFromArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (DestReg <= 7 && DestReg == SourceReg && (Const & (~0xFF)) == 0)
    {
        CPU_Message("      subs\t%s, #0x%X", ArmRegName(DestReg), Const);
        ArmThumbOpcode op = { 0 };
        op.Imm8.imm8 = (uint8_t)Const;
        op.Imm8.rdn = DestReg;
        op.Imm8.opcode = 0x7;
        AddCode16(op.Hex);
    }
    else if ((Const & (~0x7FF)) == 0)
    {
        CPU_Message("      sub.w\t%s, %s, #0x%X", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        Arm32Opcode op = { 0 };
        op.RnRdImm12.Rn = SourceReg;
        op.RnRdImm12.s = 0;
        op.RnRdImm12.opcode = 0x15;
        op.RnRdImm12.i = (Const >> 11) & 1;
        op.RnRdImm12.opcode2 = 0x1E;
        op.RnRdImm12.imm8 = (Const & 0xFF);
        op.RnRdImm12.rd = DestReg;
        op.RnRdImm12.imm3 = (Const >> 8) & 0x7;
        op.RnRdImm12.reserved = 0;
        AddCode32(op.Hex);
    }
    else
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, Const);
        SubArmRegFromArmReg(DestReg, SourceReg, TempReg);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
}

void CArmOps::SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    ArmReg TempReg1 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    if (TempReg1 == Arm_Unknown || TempReg2 == Arm_Unknown)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    MoveConstToArmReg(TempReg1, (uint32_t)Variable, VariableName);
    LoadArmRegPointerToArmReg(TempReg2, TempReg1, 0);
    SubConstFromArmReg(TempReg2, TempReg2, Const);
    StoreArmRegToArmRegPointer(TempReg2, TempReg1, 0);

    m_RegWorkingSet.SetArmRegProtected(TempReg1, false);
    m_RegWorkingSet.SetArmRegProtected(TempReg2, false);
}

void CArmOps::TestVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    PreOpCheck(Arm_Unknown, false, __FILE__, __LINE__);

    ArmReg TempReg1 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);

    MoveVariableToArmReg(Variable, VariableName, TempReg1);
    MoveConstToArmReg(TempReg2, Const);
    AndArmRegToArmReg(TempReg1, TempReg1, TempReg2);
    CompareArmRegToArmReg(TempReg1, TempReg2);

    m_RegWorkingSet.SetArmRegProtected(TempReg1, false);
    m_RegWorkingSet.SetArmRegProtected(TempReg2, false);
}

void CArmOps::XorArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (SourceReg <= 7 && DestReg <= 7)
    {
        CPU_Message("      eors\t%s, %s", ArmRegName(DestReg), ArmRegName(SourceReg));
        ArmThumbOpcode op = { 0 };
        op.Reg2.rn = DestReg;
        op.Reg2.rm = SourceReg;
        op.Reg2.opcode = 0x101;
        AddCode16(op.Hex);
    }
    else
    {
        XorArmRegToArmReg(DestReg, DestReg, SourceReg);
    }
}

void CArmOps::XorArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg1, ArmReg SourceReg2)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    CPU_Message("      eor.w\t%s, %s, %s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
    Arm32Opcode op = { 0 };
    op.imm5.rn = SourceReg1;
    op.imm5.s = 0;
    op.imm5.opcode = 0x754;

    op.imm5.rm = SourceReg2;
    op.imm5.type = 0;
    op.imm5.imm2 = 0;
    op.imm5.rd = DestReg;
    op.imm5.imm3 = 0;
    op.imm5.opcode2 = 0;
    AddCode32(op.Hex);
}

void CArmOps::XorConstToArmReg(ArmReg DestReg, uint32_t value)
{
    PreOpCheck(DestReg, false, __FILE__, __LINE__);

    if (value == 0)
    {
        //ignore
    }
    else if (CanThumbCompressConst(value))
    {
        uint16_t CompressedValue = ThumbCompressConst(value);
        CPU_Message("      eor\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(DestReg), value);
        Arm32Opcode op = { 0 };
        op.imm8_3_1.rn = DestReg;
        op.imm8_3_1.s = 0;
        op.imm8_3_1.opcode = 0x4;
        op.imm8_3_1.i = (CompressedValue >> 11) & 1;
        op.imm8_3_1.opcode2 = 0x1E;

        op.imm8_3_1.imm8 = CompressedValue & 0xFF;
        op.imm8_3_1.rd = DestReg;
        op.imm8_3_1.imm3 = (CompressedValue >> 8) & 0x3;
        op.imm8_3_1.opcode3 = 0;
        AddCode32(op.Hex);
    }
    else
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, value);
        XorArmRegToArmReg(DestReg, TempReg, DestReg);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
}

bool CArmOps::CanThumbCompressConst(uint32_t value)
{
    //'nnnnnnnn'
    if ((value & 0xFFFFFF00) == 0)
    {
        return true;
    }

    //'00000000 nnnnnnnn 00000000 nnnnnnnn'
    if (((value >> 24) & 0xFF) == 0 &&
        ((value >> 16) & 0xFF) == (value & 0xFF) &&
        ((value >> 8) & 0xFF) == 0)
    {
        return true;
    }

    //'nnnnnnnn 00000000 nnnnnnnn 00000000'
    if (((value >> 24) & 0xFF) == ((value >> 8) & 0xFF) &&
        ((value >> 16) & 0xFF) == 0 &&
        (value & 0xFF) == 0)
    {
        return true;
    }

    //'nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn'
    if (((value >> 24) & 0xFF) == (value & 0xFF) &&
        ((value >> 16) & 0xFF) == (value & 0xFF) &&
        ((value >> 8) & 0xFF) == (value & 0xFF))
    {
        return true;
    }
    return false;
}

uint16_t CArmOps::ThumbCompressConst(uint32_t value)
{
    if ((value & 0xFFFFFF00) == 0)
    {
        return (uint16_t)((value & 0xFF));
    }

    //'00000000 nnnnnnnn 00000000 nnnnnnnn'
    if (((value >> 24) & 0xFF) == 0 &&
        ((value >> 16) & 0xFF) == (value & 0xFF) &&
        ((value >> 8) & 0xFF) == 0)
    {
        return (uint16_t)(0x100 | (value & 0xFF));
    }

    //'nnnnnnnn 00000000 nnnnnnnn 00000000'
    if (((value >> 24) & 0xFF) == ((value >> 8) & 0xFF) &&
        ((value >> 16) & 0xFF) == 0 &&
        (value & 0xFF) == 0)
    {
        return (uint16_t)(0x200 | ((value >> 8) & 0xFF));
    }

    //'nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn'
    if (((value >> 24) & 0xFF) == (value & 0xFF) &&
        ((value >> 16) & 0xFF) == (value & 0xFF) &&
        ((value >> 8) & 0xFF) == (value & 0xFF))
    {
        return (uint16_t)(0x300 | (value & 0xFF));
    }

    CPU_Message("%s: value >> 24 = %X value >> 16 = %X value >> 8 = %X value = %X", __FUNCTION__, (value >> 24), (value >> 16), (value >> 8), value);
    CPU_Message("%s: value = %X", __FUNCTION__, value);
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CArmOps::SetJump8(uint8_t * Loc, uint8_t * JumpLoc)
{
    if (m_PopRegisters != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (Loc == NULL || JumpLoc == NULL)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    ArmThumbOpcode * op = (ArmThumbOpcode *)Loc;
    if (op->BranchImm.opcode != 0x1C && op->BranchImmCond.opcode != 0xD)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    uint32_t pc = ((uint32_t)Loc) + 4;
    uint32_t target = ((uint32_t)JumpLoc);
    uint32_t immediate = (target - pc) >> 1;
    if ((immediate & ~0x7F) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    CPU_Message("%s: pc: %X target: %X Loc: %X  JumpLoc: %X immediate: %X", __FUNCTION__, pc, target, (uint32_t)Loc, (uint32_t)JumpLoc, immediate);
    CPU_Message("%s: writing %d to %X", __FUNCTION__, immediate, Loc);
    if (op->BranchImm.opcode == 0x1C)
    {
        op->BranchImm.imm = immediate;
    }
    else
    {
        op->BranchImmCond.imm = (uint8_t)immediate;
    }
}

void CArmOps::SetJump20(uint32_t * Loc, uint32_t * JumpLoc)
{
    if (m_PopRegisters != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (Loc == NULL || JumpLoc == NULL)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    int32_t pc = ((int32_t)Loc) + 4;
    int32_t target = ((int32_t)JumpLoc);
    int32_t immediate = (target - pc) >> 1;
    int32_t immediate_check = immediate & ~0xFFFFF;
    if (immediate_check != 0 && immediate_check != ~0xFFFFF)
    {
        CPU_Message("%s: target %X pc %X immediate: %X", __FUNCTION__, target, pc, immediate);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    Arm32Opcode op = { 0 };
    op.Hex = *Loc;
    if (op.Branch20.val12 == 0)
    {
        if (immediate < 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            op.Branch20.imm11 = (immediate & 0x7FF);
            op.Branch20.imm6 = (immediate >> 11) & 0x3F;
            op.Branch20.J1 = (immediate >> 17) & 0x1;
            op.Branch20.J2 = (immediate >> 18) & 0x1;
            op.Branch20.S = (immediate >> 19) & 0x1;
        }
    }
    else
    {
        op.Branch20.S = (immediate >> 23) & 0x1;
        op.Branch20.J1 = op.Branch20.S == 1 ? (immediate >> 22) & 0x1 : !((immediate >> 22) & 0x1);
        op.Branch20.J2 = op.Branch20.S == 1 ? (immediate >> 21) & 0x1 : !((immediate >> 21) & 0x1);
        op.Branch20.cond = (immediate >> 17) & 0xF;
        op.Branch20.imm6 = (immediate >> 11) & 0x3F;
        op.Branch20.imm11 = (immediate & 0x7FF);
    }

    uint32_t OriginalValue = *Loc;
    *Loc = op.Hex;
    CPU_Message("%s: OriginalValue %X New Value %X JumpLoc: %X Loc: %X immediate: %X immediate_check = %X", __FUNCTION__, OriginalValue, *Loc, JumpLoc, Loc, immediate, immediate_check);
}

void * CArmOps::GetAddressOf(int value, ...)
{
    void * Address;

    va_list ap;
    va_start(ap, value);
    Address = va_arg(ap, void *);
    va_end(ap);

    return Address;
}

void CArmOps::PreOpCheck(ArmReg DestReg, bool AllowedInItBlock, const char * FileName, uint32_t LineNumber)
{
    if (DestReg != Arm_Unknown && DestReg == m_LastStoreReg)
    {
        ArmNop();
    }
    if (!AllowedInItBlock && m_InItBlock)
    {
        g_Notify->BreakPoint(FileName, LineNumber);
    }
    FlushPopArmReg();
    m_LastStoreReg = Arm_Unknown;
}

void CArmOps::BreakPointNotification(const char * FileName, uint32_t LineNumber)
{
    g_Notify->BreakPoint(FileName, LineNumber);
}

bool CArmOps::ArmCompareInverse(ArmCompareType CompareType)
{
    switch (CompareType)
    {
    case ArmBranch_Equal: return false;
    case ArmBranch_Notequal: return true;
    case ArmBranch_GreaterThanOrEqual: return false;
    case ArmBranch_LessThan: return true;
    case ArmBranch_GreaterThan: return false;
    case ArmBranch_LessThanOrEqual: return true;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

CArmOps::ArmCompareType CArmOps::ArmCompareInverseType(ArmCompareType CompareType)
{
    if (CompareType == ArmBranch_Equal) { return ArmBranch_Notequal; }
    if (CompareType == ArmBranch_Notequal) { return ArmBranch_Equal; }
    if (CompareType == ArmBranch_GreaterThanOrEqual) { return ArmBranch_LessThan; }
    if (CompareType == ArmBranch_LessThan) { return ArmBranch_GreaterThanOrEqual; }
    if (CompareType == ArmBranch_GreaterThan) { return ArmBranch_LessThanOrEqual; }
    if (CompareType == ArmBranch_LessThanOrEqual) { return ArmBranch_GreaterThan; }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return ArmBranch_Equal;
}

const char * CArmOps::ArmCompareSuffix(ArmCompareType CompareType)
{
    switch (CompareType)
    {
    case ArmBranch_Equal: return "eq";
    case ArmBranch_Notequal: return "ne";
    case ArmBranch_GreaterThanOrEqual: return "ge";
    case ArmBranch_LessThan: return "lt";
    case ArmBranch_GreaterThan: return "g";
    case ArmBranch_LessThanOrEqual: return "le";
    case ArmBranch_Always: return "";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CArmOps::ArmRegName(ArmReg Reg)
{
    switch (Reg)
    {
    case Arm_R0: return "r0";
    case Arm_R1: return "r1";
    case Arm_R2: return "r2";
    case Arm_R3: return "r3";
    case Arm_R4: return "r4";
    case Arm_R5: return "r5";
    case Arm_R6: return "r6";
    case Arm_R7: return "r7";
    case Arm_R8: return "r8";
    case Arm_R9: return "r9";
    case Arm_R10: return "r10";
    case Arm_R11: return "r11";
    case Arm_R12: return "r12";
    case ArmRegSP: return "sp";
    case ArmRegLR: return "lr";
    case ArmRegPC: return "pc";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CArmOps::ArmFpuSingleName(ArmFpuSingle Reg)
{
    switch (Reg)
    {
    case Arm_S0: return "s0";
    case Arm_S1: return "s1";
    case Arm_S2: return "s2";
    case Arm_S3: return "s3";
    case Arm_S4: return "s4";
    case Arm_S5: return "s5";
    case Arm_S6: return "s6";
    case Arm_S7: return "s7";
    case Arm_S8: return "s8";
    case Arm_S9: return "s9";
    case Arm_S10: return "s10";
    case Arm_S11: return "s11";
    case Arm_S12: return "s12";
    case Arm_S13: return "s13";
    case Arm_S14: return "s14";
    case Arm_S15: return "s15";
    case Arm_S16: return "s16";
    case Arm_S17: return "s17";
    case Arm_S18: return "s18";
    case Arm_S19: return "s19";
    case Arm_S20: return "s20";
    case Arm_S21: return "s21";
    case Arm_S22: return "s22";
    case Arm_S23: return "s23";
    case Arm_S24: return "s24";
    case Arm_S25: return "s25";
    case Arm_S26: return "s26";
    case Arm_S27: return "s27";
    case Arm_S28: return "s28";
    case Arm_S29: return "s29";
    case Arm_S30: return "s30";
    case Arm_S31: return "s31";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CArmOps::ArmItMaskName(ArmItMask mask)
{
    switch (mask)
    {
    case ItMask_None: return "";
    case ItMask_T: return "t";
    case ItMask_E: return "e";
    case ItMask_TT: return "tt";
    case ItMask_ET: return "et";
    case ItMask_TE: return "te";
    case ItMask_EE: return "ee";
    case ItMask_TTT: return "ttt";
    case ItMask_ETT: return "ett";
    case ItMask_TET: return "tet";
    case ItMask_EET: return "eet";
    case ItMask_TTE: return "tte";
    case ItMask_ETE: return "ete";
    case ItMask_TEE: return "tee";
    case ItMask_EEE: return "eee";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CArmOps::ArmCurrentItCondition()
{
    if (m_ItBlockInstruction == 0)
    {
        return ArmCompareSuffix(m_ItBlockCompareType);
    }
    if (m_ItBlockInstruction == 1 && m_ItBlockMask == ItMask_T)
    {
        return ArmCompareSuffix(m_ItBlockCompareType);
    }
    if (m_ItBlockInstruction == 1 && m_ItBlockMask == ItMask_E)
    {
        return ArmCompareSuffix(ArmCompareInverseType(m_ItBlockCompareType));
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return "";
}

void CArmOps::ProgressItBlock(void)
{
    bool itBlockDone = false;
    m_ItBlockInstruction += 1;
    if (m_ItBlockInstruction == 1)
    {
        if (m_ItBlockMask == ItMask_None)
        {
            itBlockDone = true;
        }
    }
    else if (m_ItBlockInstruction == 2)
    {
        if (m_ItBlockMask == ItMask_T || m_ItBlockMask == ItMask_E)
        {
            itBlockDone = true;
        }
    }
    else if (m_ItBlockInstruction == 3)
    {
        if (m_ItBlockMask == ItMask_TT || m_ItBlockMask == ItMask_ET || m_ItBlockMask == ItMask_TE || m_ItBlockMask == ItMask_EE)
        {
            itBlockDone = true;
        }
    }
    else if (m_ItBlockInstruction == 4)
    {
        if (m_ItBlockMask == ItMask_TTT || m_ItBlockMask == ItMask_ETT || m_ItBlockMask == ItMask_TET || m_ItBlockMask == ItMask_EET ||
            m_ItBlockMask == ItMask_TTE || m_ItBlockMask == ItMask_ETE || m_ItBlockMask == ItMask_TEE || m_ItBlockMask == ItMask_EEE)
        {
            itBlockDone = true;
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (itBlockDone)
    {
        m_InItBlock = false;
        m_ItBlockInstruction = 0;
    }
}

void CArmOps::AddCode8(uint8_t value)
{
    (*((uint8_t *)(*g_RecompPos)) = (uint8_t)(value));
    *g_RecompPos += 1;
}

void CArmOps::AddCode16(uint16_t value)
{
    (*((uint16_t *)(*g_RecompPos)) = (uint16_t)(value));
    *g_RecompPos += 2;
}

void CArmOps::AddCode32(uint32_t value)
{
    (*((uint32_t *)(*g_RecompPos)) = (uint32_t)(value));
    *g_RecompPos += 4;
}

#endif