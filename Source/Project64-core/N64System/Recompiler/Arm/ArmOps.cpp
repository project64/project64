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
#include <Project64-core/N64System/Recompiler/Arm/ArmOps.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmOpCode.h>

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
    if (DestReg <= 7 && SourceReg1 <=7 && SourceReg2 <= 7)
    {
        CPU_Message("      add\t%s,%s,%s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        ArmThumbOpcode op = {0};
        op.Reg.rt = DestReg;
        op.Reg.rn = SourceReg1;
        op.Reg.rm = SourceReg2;
        op.Reg.opcode = 0xC;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      add.w\t%s,%s,%s", ArmRegName(DestReg), ArmRegName(SourceReg1), ArmRegName(SourceReg2));
        Arm32Opcode op = {0};
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
    AddConstToArmReg(DestReg, DestReg, Const);
}

void CArmOps::AddConstToArmReg(ArmReg DestReg, ArmReg SourceReg, uint32_t Const)
{
    if (DestReg == SourceReg && Const == 0)
    {
        //ignore
    }
    else if ((Const & 0xFFFFFFF8) == 0 && DestReg <= 7 && SourceReg <= 7)
    {
        CPU_Message("      adds\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        ArmThumbOpcode op = {0};
        op.Imm3.rd = DestReg;
        op.Imm3.rn = SourceReg;
        op.Imm3.imm3 = Const;
        op.Imm3.opcode = 0xE;
        AddCode16(op.Hex);
    }
    else if ((Const & 0xFFFFFF00) == 0 && DestReg <= 7 && DestReg == SourceReg)
    {
        CPU_Message("      adds\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        ArmThumbOpcode op = {0};
        op.Imm8.imm8 = Const;
        op.Imm8.rdn = DestReg;
        op.Imm8.opcode = 0x6;
        AddCode16(op.Hex);
    }
    else if ((Const & 0xFFFFFF80) == 0xFFFFFF80 && DestReg <= 7 && DestReg == SourceReg)
    {
        CPU_Message("      sub\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), (~Const) + 1);
        ArmThumbOpcode op = {0};
        op.Imm8.imm8 = ((~Const) + 1) & 0xFF;
        op.Imm8.rdn = DestReg;
        op.Imm8.opcode = 0x7;
        AddCode16(op.Hex);
    }
    else if (CanThumbCompressConst(Const))
    {
        CPU_Message("      add.w\t%s, %s, #%d", ArmRegName(DestReg), ArmRegName(SourceReg), Const);
        uint16_t CompressedConst = ThumbCompressConst(Const);
        Arm32Opcode op = {0};
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
    else
    {
        CPU_Message("%s: DestReg = %X Const = %X", __FUNCTION__, DestReg, Const);
        g_Notify->BreakPoint(__FILE__,__LINE__);
    }
}

void CArmOps::AndArmRegToArmReg(ArmReg DestReg, ArmReg SourceReg)
{
    if (DestReg <= 0x7 && SourceReg <= 0x7 )
    {
        CPU_Message("      ands\t%s, %s", ArmRegName(DestReg), ArmRegName(SourceReg));
        ArmThumbOpcode op = {0};
        op.Reg2.rn = DestReg;
        op.Reg2.rm = SourceReg;
        op.Reg2.opcode = 0x100;
        AddCode16(op.Hex);
    }
    else
    {
        CPU_Message("      and.w\t%s, %s, %s", ArmRegName(DestReg), ArmRegName(DestReg), ArmRegName(SourceReg));
        Arm32Opcode op = {0};
        op.imm5.rn = DestReg;
        op.imm5.s = 0;
        op.imm5.opcode = 0x750;

        op.imm5.rm = SourceReg;
        op.imm5.type = 0;
        op.imm5.imm2 = 0;
        op.imm5.rd = DestReg;
        op.imm5.imm3 = 0;
        op.imm5.opcode2 = 0;

        AddCode32(op.Hex);
    }
}

void CArmOps::BranchLabel8(ArmBranchCompare CompareType, const char * Label)
{
    CPU_Message("      b%s\t%s", ArmBranchSuffix(CompareType),Label);
    ArmThumbOpcode op = {0};
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

void CArmOps::BranchLabel20(ArmBranchCompare CompareType, const char * Label)
{
    CPU_Message("      b%s\t%s", ArmBranchSuffix(CompareType),Label);
    Arm32Opcode op = {0};
    op.Branch20.imm6 = 0;
    op.Branch20.cond = CompareType == ArmBranch_Always ? 0 : CompareType;
    op.Branch20.S = 0;
    op.Branch20.Opcode = 0x1E;
    op.Branch20.imm11 = 0;
    op.Branch20.J2 = CompareType == ArmBranch_Always ? 1 : 0;
    op.Branch20.val12 = CompareType == ArmBranch_Always ? 1 : 0;
    op.Branch20.J1 = CompareType == ArmBranch_Always ? 1 : 0;
    op.Branch20.val14 = 0x2;
    AddCode32(op.Hex);
}

void CArmOps::CallFunction(void * Function, const char * FunctionName)
{
    ArmReg reg = Arm_R4;
    MoveConstToArmReg((uint32_t)Function,reg,FunctionName);
    int32_t offset=(int32_t)Function-(int32_t)*g_RecompPos;
    ArmThumbOpcode op = {0};
    op.Branch.reserved = 0;
    op.Branch.rm = reg;
    op.Branch.opcode = 0x8F;
    CPU_Message("      blx\t%s", ArmRegName(reg));
    AddCode16(op.Hex);
}

void CArmOps::MoveConstToArmReg(uint16_t Const, ArmReg reg, const char * comment)
{
    if (comment != NULL)
    {
        CPU_Message("      movw\t%s, #0x%X\t; %s", ArmRegName(reg), (uint32_t)Const, comment);
    }
    else
    {
        CPU_Message("      movw\t%s, #%d\t; 0x%X", ArmRegName(reg), (uint32_t)Const, (uint32_t)Const);
    }
    Arm32Opcode op = {0};
    op.imm16.opcode = ArmMOV_IMM16;
    op.imm16.i = ((Const >> 11) & 0x1);
    op.imm16.opcode2 = ArmMOVW_IMM16;
    op.imm16.imm4 = ((Const >> 12) & 0xF);
    op.imm16.reserved = 0;
    op.imm16.imm3 = ((Const >> 8) & 0x7);
    op.imm16.rd = reg;
    op.imm16.imm8 = (Const & 0xFF);
    AddCode32(op.Hex);
}

void CArmOps::MoveConstToArmRegTop(uint16_t Const, ArmReg reg, const char * comment)
{
    if (comment != NULL)
    {
        CPU_Message("      movt\t%s, #0x%X\t; %s", ArmRegName(reg), (uint32_t)Const, comment);
    }
    else
    {
        CPU_Message("      movt\t%s, #%d\t; 0x%X", ArmRegName(reg), (uint32_t)Const, (uint32_t)Const);
    }
    Arm32Opcode op = {0};
    op.imm16.opcode = ArmMOV_IMM16;
    op.imm16.i = ((Const >> 11) & 0x1);
    op.imm16.opcode2 = ArmMOVT_IMM16;
    op.imm16.imm4 = ((Const >> 12) & 0xF);
    op.imm16.reserved = 0;
    op.imm16.imm3 = ((Const >> 8) & 0x7);
    op.imm16.rd = reg;
    op.imm16.imm8 = (Const & 0xFF);
    AddCode32(op.Hex);
}

void CArmOps::CompareArmRegToConst(ArmReg Reg, uint8_t value)
{
    if (Reg > 0x7) { g_Notify->BreakPoint(__FILE__,__LINE__); return; }

    CPU_Message("      cmp\t%s, #%d\t; 0x%X", ArmRegName(Reg), value, value);
    ArmThumbOpcode op = {0};
    op.Imm8.imm8 = value;
    op.Imm8.rdn = Reg;
    op.Imm8.opcode = 0x5;
    AddCode16(op.Hex);
}

void CArmOps::CompareArmRegToArmReg(ArmReg Reg1, ArmReg Reg2)
{
    CPU_Message("      cmp\t%s, %s", ArmRegName(Reg1), ArmRegName(Reg2));
    ArmThumbOpcode op = {0};
    op.Reg2.rn = Reg1;
    op.Reg2.rm = Reg2;
    op.Reg2.opcode = 0x10A;
    AddCode16(op.Hex);
}

void CArmOps::LoadArmRegPointerToArmReg(ArmReg RegPointer, ArmReg Reg, uint8_t offset)
{
    if (Reg > 0x7 || RegPointer > 0x7)
    {
        if ((offset & (~0xFFF)) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); return; }
        g_Notify->BreakPoint(__FILE__,__LINE__);
    }
    else
    {
        if ((offset & (~0x1F)) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); return; }

        CPU_Message("      ldr\t%s, [%s, #%d]", ArmRegName(Reg), ArmRegName(RegPointer), (uint32_t)offset);
        ArmThumbOpcode op = {0};
        op.Imm5.rt = Reg;
        op.Imm5.rn = RegPointer;
        op.Imm5.imm5 = offset;
        op.Imm5.opcode = ArmLDR_ThumbImm;
        AddCode16(op.Hex);
    }
}

void CArmOps::MoveArmRegArmReg(ArmReg SourceReg, ArmReg DestReg)
{
    g_Notify->BreakPoint(__FILE__,__LINE__);
}

void CArmOps::MoveConstToArmReg(uint32_t Const, ArmReg reg, const char * comment)
{
    MoveConstToArmReg((uint16_t)(Const & 0xFFFF),reg,comment);
    uint16_t TopValue = (uint16_t)((Const >> 16) & 0xFFFF);
    if (TopValue != 0)
    {
        MoveConstToArmRegTop(TopValue,reg,comment != NULL ? "" : NULL);
    }
}

void CArmOps::MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    MoveConstToArmReg(Const,Arm_R1);
    MoveConstToArmReg((uint32_t)Variable,Arm_R2,VariableName);
    StoreArmRegToArmRegPointer(Arm_R1,Arm_R2,0);
}

void CArmOps::MoveVariableToArmReg(void * Variable, const char * VariableName, ArmReg reg)
{
    MoveConstToArmReg((uint32_t)Variable,reg,VariableName);
    LoadArmRegPointerToArmReg(reg,reg,0);
}

void CArmOps::PushArmReg(uint16_t Registers)
{
    if (Registers == 0)
    {
        return;
    }
    if ((Registers & ArmPushPop_R8) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R9) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R10) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R11) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R12) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R13) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R15) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }

    std::string pushed;
    if ((Registers & ArmPushPop_R1) != 0) { pushed += pushed.length() > 0 ? ", r1" : "r1"; }
    if ((Registers & ArmPushPop_R2) != 0) { pushed += pushed.length() > 0 ? ", r2" : "r2"; }
    if ((Registers & ArmPushPop_R3) != 0) { pushed += pushed.length() > 0 ? ", r3" : "r3"; }
    if ((Registers & ArmPushPop_R4) != 0) { pushed += pushed.length() > 0 ? ", r4" : "r4"; }
    if ((Registers & ArmPushPop_R5) != 0) { pushed += pushed.length() > 0 ? ", r5" : "r5"; }
    if ((Registers & ArmPushPop_R6) != 0) { pushed += pushed.length() > 0 ? ", r6" : "r6"; }
    if ((Registers & ArmPushPop_R7) != 0) { pushed += pushed.length() > 0 ? ", r7" : "r7"; }
    if ((Registers & ArmPushPop_LR) != 0) { pushed += pushed.length() > 0 ? ", lr" : "lr"; }

    CPU_Message("      push\t%s", pushed.c_str());
    bool lr = (Registers & ArmPushPop_LR) != 0;
    Registers &= Registers & ~ArmPushPop_LR;

    ArmThumbOpcode op = {0};
    op.Push.register_list = (uint8_t)Registers;
    op.Push.m = lr ? 1 : 0;
    op.Push.opcode = ArmPUSH;
    AddCode16(op.Hex);
}

void CArmOps::PopArmReg(uint16_t Registers)
{
    if (Registers == 0)
    {
        return;
    }
    if ((Registers & ArmPushPop_R8) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R9) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R10) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R11) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R12) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R13) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
    if ((Registers & ArmPushPop_R14) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }

    std::string pushed;
    if ((Registers & ArmPushPop_R1) != 0) { pushed += pushed.length() > 0 ? ", r1" : "r1"; }
    if ((Registers & ArmPushPop_R2) != 0) { pushed += pushed.length() > 0 ? ", r2" : "r2"; }
    if ((Registers & ArmPushPop_R3) != 0) { pushed += pushed.length() > 0 ? ", r3" : "r3"; }
    if ((Registers & ArmPushPop_R4) != 0) { pushed += pushed.length() > 0 ? ", r4" : "r4"; }
    if ((Registers & ArmPushPop_R5) != 0) { pushed += pushed.length() > 0 ? ", r5" : "r5"; }
    if ((Registers & ArmPushPop_R6) != 0) { pushed += pushed.length() > 0 ? ", r6" : "r6"; }
    if ((Registers & ArmPushPop_R7) != 0) { pushed += pushed.length() > 0 ? ", r7" : "r7"; }
    if ((Registers & ArmPushPop_PC) != 0) { pushed += pushed.length() > 0 ? ", pc" : "pc"; }

    CPU_Message("      pop\t%s", pushed.c_str());
    bool pc = (Registers & ArmPushPop_PC) != 0;
    Registers &= Registers & ~ArmPushPop_PC;

    ArmThumbOpcode op = {0};
    op.Pop.register_list = (uint8_t)Registers;
    op.Pop.p = pc ? 1 : 0;
    op.Pop.opcode = ArmPOP;
    AddCode16(op.Hex);
}

void CArmOps::StoreArmRegToArmRegPointer(ArmReg Reg, ArmReg RegPointer, uint8_t offset)
{
    if (Reg > 0x7 || RegPointer > 0x7)
    {
        if ((offset & (~0xFFF)) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); return; }

        CPU_Message("      str\t%s, [%s, #%d]", ArmRegName(Reg), ArmRegName(RegPointer), (uint32_t)offset);
        Arm32Opcode op = {0};
        op.imm12.rt = Reg;
        op.imm12.rn = RegPointer;
        op.imm12.imm = offset;
        op.imm12.opcode = 0xF8C;
        AddCode32(op.Hex);
    }
    else
    {
        if ((offset & (~0x1F)) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); return; }

        CPU_Message("      str\t%s, [%s, #%d]", ArmRegName(Reg), ArmRegName(RegPointer), (uint32_t)offset);
        ArmThumbOpcode op = {0};
        op.Imm5.rt = Reg;
        op.Imm5.rn = RegPointer;
        op.Imm5.imm5 = offset;
        op.Imm5.opcode = ArmSTR_ThumbImm;
        AddCode16(op.Hex);
    }
}

void CArmOps::SubConstFromArmReg(ArmReg Reg, uint32_t Const)
{
    if (Reg <= 7 && (Const & (~0xFF)) == 0)
    {
        CPU_Message("      subs\t%s, #0x%X", ArmRegName(Reg), Const);
        ArmThumbOpcode op = {0};
        op.Imm8.imm8 = (uint8_t)Const;
        op.Imm8.rdn = Reg;
        op.Imm8.opcode = 0x7;
        AddCode16(op.Hex);
    }
    else if ((Const & (~0x7FF)) == 0)
    {
        CPU_Message("      sub\t%s, #0x%X", ArmRegName(Reg), Const);
        Arm32Opcode op = {0};
        op.RnRdImm12.Rn = Reg;
        op.RnRdImm12.s = 0;
        op.RnRdImm12.opcode = 0x15;
        op.RnRdImm12.i = (Const >> 11) & 1;
        op.RnRdImm12.opcode2 = 0x1E;
        op.RnRdImm12.imm8 = (Const & 0xFF);
        op.RnRdImm12.rd = Reg;
        op.RnRdImm12.imm3 = (Const >> 8) & 0x7;
        op.RnRdImm12.reserved = 0;
        AddCode32(op.Hex);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__,__LINE__);
    }
}

void CArmOps::SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    MoveConstToArmReg((uint32_t)Variable,Arm_R1,VariableName);
    LoadArmRegPointerToArmReg(Arm_R1,Arm_R2,0);
    SubConstFromArmReg(Arm_R2,Const);
    StoreArmRegToArmRegPointer(Arm_R2,Arm_R1,0);
}

void CArmOps::TestVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    MoveVariableToArmReg(Variable,VariableName, Arm_R2);
    MoveConstToArmReg(Const,Arm_R3);
    AndArmRegToArmReg(Arm_R3,Arm_R2);
    CompareArmRegToArmReg(Arm_R2,Arm_R3);
}

bool CArmOps::CanThumbCompressConst (uint32_t value)
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

uint16_t CArmOps::ThumbCompressConst (uint32_t value)
{
    if ((value & 0xFFFFFF00) == 0)
    {
        return (uint16_t)((value & 0xFF));
    }
    if (((value >> 24) & 0xFF) == (value & 0xFF) &&
        ((value >> 16) & 0xFF) == (value & 0xFF) &&
        ((value >> 8) & 0xFF) == (value & 0xFF))
    {
        return (uint16_t)(0x300 | (value & 0xFF));
    }
    CPU_Message("%s: value >> 24 = %X value >> 16 = %X value >> 8 = %X value = %X", __FUNCTION__, (value >> 24), (value >> 16), (value >> 8), value);
    CPU_Message("%s: value = %X", __FUNCTION__, value);
    g_Notify->BreakPoint(__FILE__,__LINE__);
    return false;
}

void CArmOps::SetJump8(uint8_t * Loc, uint8_t * JumpLoc)
{
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
    CPU_Message("%s: pc: %X target: %X Loc: %X  JumpLoc: %X immediate: %X", __FUNCTION__, pc, target, (uint32_t)Loc, (uint32_t)JumpLoc, immediate );
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
    if (Loc == NULL || JumpLoc == NULL)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    uint32_t pc = ((uint32_t)Loc) + 4;
    uint32_t target = ((uint32_t)JumpLoc);
    uint32_t immediate = (target - pc) >> 1;
    uint32_t immediate_check = immediate & ~0xFFFFF;
    if (immediate_check != 0 && immediate_check != ~0xFFFFF)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    Arm32Opcode op = {0};
    op.Hex = *Loc;
    if (op.Branch20.val12 == 0)
    {
        op.Branch20.imm11 = (immediate & 0x7FF);
        op.Branch20.imm6 = (immediate >> 11) & 0x37;
        op.Branch20.J1 = (immediate >> 17) & 0x1;
        op.Branch20.J2 = (immediate >> 18) & 0x1;
        op.Branch20.S = (immediate >> 19) & 0x1;
    }
    else
    {
        if (immediate < 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            op.Branch20.imm11 = (immediate & 0x7FF);
        }
    }

    uint32_t OriginalValue = *Loc;
    *Loc = op.Hex;
    CPU_Message("%s: OriginalValue %X New Value %X JumpLoc: %X Loc: %X immediate: %X immediate_check = %X", __FUNCTION__, OriginalValue, *Loc, JumpLoc, Loc, immediate, immediate_check );
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

const char * CArmOps::ArmBranchSuffix(ArmBranchCompare CompareType)
{
    switch (CompareType)
    {
    case ArmBranch_Equal: return "eq";
    case ArmBranch_Notequal: return "ne";
    case ArmBranch_GreaterThanOrEqual: return "ge";
    case ArmBranch_LessThan: return "l";
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
    case Arm_R0: return "R0";
    case Arm_R1: return "R1";
    case Arm_R2: return "R2";
    case Arm_R3: return "R3";
    case Arm_R4: return "R4";
    case Arm_R5: return "R5";
    case Arm_R6: return "R6";
    case Arm_R7: return "R7";
    case Arm_R8: return "R8";
    case Arm_R9: return "R9";
    case Arm_R10: return "R10";
    case Arm_R11: return "R11";
    case Arm_R12: return "R12";
    case ArmRegSP: return "SP";
    case ArmRegLR: return "LR";
    case ArmRegPC: return "PC";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

void CArmOps::AddCode8(uint8_t value)
{
    (*((uint8_t *)(*g_RecompPos))=(uint8_t)(value));
    *g_RecompPos += 1;
}

void CArmOps::AddCode16(uint16_t value)
{
    (*((uint16_t *)(*g_RecompPos))=(uint16_t)(value));
    *g_RecompPos += 2;
}

void CArmOps::AddCode32(uint32_t value)
{
    (*((uint32_t *)(*g_RecompPos)) = (uint32_t)(value));
    *g_RecompPos += 4;
}

#endif