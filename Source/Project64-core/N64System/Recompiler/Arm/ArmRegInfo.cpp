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
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>

CArmRegInfo::CArmRegInfo() :
m_InCallDirect(false)
{
    for (int32_t i = 0; i < 32; i++)
    {
        m_RegMapLo[i] = Arm_Unknown;
        m_RegMapHi[i] = Arm_Unknown;
    }

    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        m_ArmReg_MapOrder[i] = 0;
        m_ArmReg_Protected[i] = false;
        m_ArmReg_MappedTo[i] = NotMapped;
        m_Variable_MappedTo[i] = VARIABLE_UNKNOWN;
    }
}

CArmRegInfo::CArmRegInfo(const CArmRegInfo& rhs)
{
    *this = rhs;
}

CArmRegInfo::~CArmRegInfo()
{
}

CArmRegInfo& CArmRegInfo::operator=(const CArmRegInfo& right)
{
    CRegBase::operator=(right);

    m_InCallDirect = right.m_InCallDirect;
    memcpy(&m_RegMapLo, &right.m_RegMapLo, sizeof(m_RegMapLo));
    memcpy(&m_RegMapHi, &right.m_RegMapHi, sizeof(m_RegMapHi));
    memcpy(&m_ArmReg_MapOrder, &right.m_ArmReg_MapOrder, sizeof(m_ArmReg_MapOrder));
    memcpy(&m_ArmReg_Protected, &right.m_ArmReg_Protected, sizeof(m_ArmReg_Protected));
    memcpy(&m_ArmReg_MappedTo, &right.m_ArmReg_MappedTo, sizeof(m_ArmReg_MappedTo));
    memcpy(&m_Variable_MappedTo, &right.m_Variable_MappedTo, sizeof(m_Variable_MappedTo));
#ifdef _DEBUG
    if (*this != right)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    return *this;
}

bool CArmRegInfo::ShouldPushPopReg (ArmReg Reg)
{
    if (m_ArmReg_MappedTo[Reg] == NotMapped)
    {
        return false;
    }
    if (m_ArmReg_MappedTo[Reg] == Temp_Mapped && !GetArmRegProtected(Reg))
    {
        return false;
    }
    return true;
}

void CArmRegInfo::BeforeCallDirect(void)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    UnMap_AllFPRs();
    m_InCallDirect = true;
    int PushPopRegisters = 0;
    if (ShouldPushPopReg(Arm_R0)) { PushPopRegisters |= ArmPushPop_R0; }
    if (ShouldPushPopReg(Arm_R1)) { PushPopRegisters |= ArmPushPop_R1; }
    if (ShouldPushPopReg(Arm_R2)) { PushPopRegisters |= ArmPushPop_R2; }
    if (ShouldPushPopReg(Arm_R3)) { PushPopRegisters |= ArmPushPop_R3; }
    if (ShouldPushPopReg(Arm_R4)) { PushPopRegisters |= ArmPushPop_R4; }
    if (ShouldPushPopReg(Arm_R5)) { PushPopRegisters |= ArmPushPop_R5; }
    if (ShouldPushPopReg(Arm_R6)) { PushPopRegisters |= ArmPushPop_R6; }
    if (ShouldPushPopReg(Arm_R7)) { PushPopRegisters |= ArmPushPop_R7; }
    if (ShouldPushPopReg(Arm_R8)) { PushPopRegisters |= ArmPushPop_R8; }
    if (ShouldPushPopReg(Arm_R9)) { PushPopRegisters |= ArmPushPop_R9; }
    if (ShouldPushPopReg(Arm_R10)) { PushPopRegisters |= ArmPushPop_R10; }
    if (ShouldPushPopReg(Arm_R11)) { PushPopRegisters |= ArmPushPop_R11; }
    if (ShouldPushPopReg(Arm_R12)) { PushPopRegisters |= ArmPushPop_R12; }
    if (ShouldPushPopReg(Arm_R13)) { PushPopRegisters |= ArmPushPop_R13; }
    if (ShouldPushPopReg(Arm_R14)) { PushPopRegisters |= ArmPushPop_R14; }
    if (ShouldPushPopReg(Arm_R15)) { PushPopRegisters |= ArmPushPop_R15; }

    if (PushPopRegisters != 0)
    {
        PushArmReg(PushPopRegisters);
    }
}

void CArmRegInfo::AfterCallDirect(void)
{
    if (!m_InCallDirect)
    {
        CPU_Message("%s: Not in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    int PushPopRegisters = 0;
    if (ShouldPushPopReg(Arm_R0)) { PushPopRegisters |= ArmPushPop_R0; }
    if (ShouldPushPopReg(Arm_R1)) { PushPopRegisters |= ArmPushPop_R1; }
    if (ShouldPushPopReg(Arm_R2)) { PushPopRegisters |= ArmPushPop_R2; }
    if (ShouldPushPopReg(Arm_R3)) { PushPopRegisters |= ArmPushPop_R3; }
    if (ShouldPushPopReg(Arm_R4)) { PushPopRegisters |= ArmPushPop_R4; }
    if (ShouldPushPopReg(Arm_R5)) { PushPopRegisters |= ArmPushPop_R5; }
    if (ShouldPushPopReg(Arm_R6)) { PushPopRegisters |= ArmPushPop_R6; }
    if (ShouldPushPopReg(Arm_R7)) { PushPopRegisters |= ArmPushPop_R7; }
    if (ShouldPushPopReg(Arm_R8)) { PushPopRegisters |= ArmPushPop_R8; }
    if (ShouldPushPopReg(Arm_R9)) { PushPopRegisters |= ArmPushPop_R9; }
    if (ShouldPushPopReg(Arm_R10)) { PushPopRegisters |= ArmPushPop_R10; }
    if (ShouldPushPopReg(Arm_R11)) { PushPopRegisters |= ArmPushPop_R11; }
    if (ShouldPushPopReg(Arm_R12)) { PushPopRegisters |= ArmPushPop_R12; }
    if (ShouldPushPopReg(Arm_R13)) { PushPopRegisters |= ArmPushPop_R13; }
    if (ShouldPushPopReg(Arm_R14)) { PushPopRegisters |= ArmPushPop_R14; }
    if (ShouldPushPopReg(Arm_R15)) { PushPopRegisters |= ArmPushPop_R15; }

    if (PushPopRegisters != 0)
    {
        PopArmReg(PushPopRegisters);
    }
    SetRoundingModel(CRegInfo::RoundUnknown);
    m_InCallDirect = false;
}

void CArmRegInfo::FixRoundModel(FPU_ROUND RoundMethod)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (GetRoundingModel() == RoundMethod)
    {
        return;
    }
    CPU_Message("    FixRoundModel: CurrentRoundingModel: %s  targetRoundModel: %s", RoundingModelName(GetRoundingModel()), RoundingModelName(RoundMethod));
    if (RoundMethod == RoundDefault)
    {
        m_RegWorkingSet.BeforeCallDirect();
        MoveVariableToArmReg(_RoundingModel, "_RoundingModel", Arm_R0);
        CallFunction((void *)fesetround, "fesetround");
        m_RegWorkingSet.AfterCallDirect();
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmRegInfo::Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    ArmReg Reg;
    if (MipsReg == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        Reg = FreeArmReg();
        if (Reg < 0)
        {
            if (bHaveDebugger()) { g_Notify->DisplayError("Map_GPR_32bit\n\nOut of registers"); }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        SetArmRegProtected(Reg, true);
        CPU_Message("    regcache: allocate %s to %s", ArmRegName(Reg), CRegName::GPR[MipsReg]);
    }
    else
    {
        if (Is64Bit(MipsReg))
        {
            CPU_Message("    regcache: unallocate %s from high 32bit of %s", ArmRegName(GetMipsRegMapHi(MipsReg)), CRegName::GPR_Hi[MipsReg]);
            SetArmRegMapOrder(GetMipsRegMapHi(MipsReg), 0);
            SetArmRegMapped(GetMipsRegMapHi(MipsReg), NotMapped);
            SetArmRegProtected(GetMipsRegMapHi(MipsReg), false);
            SetMipsRegHi(MipsReg, 0);
        }
        Reg = GetMipsRegMapLo(MipsReg);
    }
    for (int32_t count = 0; count <= Arm_R15; count++)
    {
        uint32_t Count = GetArmRegMapOrder((ArmReg)count);
        if (Count > 0)
        {
            SetArmRegMapOrder((ArmReg)count, Count + 1);
        }
    }
    SetArmRegMapOrder(Reg, 1);

    CPU_Message("MipsRegToLoad = %d (%s)", MipsRegToLoad,  CRegName::GPR[MipsRegToLoad]);
    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            ArmReg GprReg = Map_Variable(VARIABLE_GPR);
            LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(MipsRegToLoad << 3));
            SetArmRegProtected(GprReg, false);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (MipsReg != MipsRegToLoad)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                //MoveArmRegToArmReg(GetMipsRegMapLo(MipsRegToLoad), Reg);
            }
        }
        else
        {
            MoveConstToArmReg(Reg, GetMipsRegLo(MipsRegToLoad));
        }
    }
    else if (MipsRegToLoad == 0)
    {
        MoveConstToArmReg(Reg, (uint32_t)0);
    }
    SetArmRegMapped(Reg, GPR_Mapped);
    SetArmRegProtected(Reg, true);
    SetMipsRegMapLo(MipsReg, Reg);
    SetMipsRegState(MipsReg, SignValue ? STATE_MAPPED_32_SIGN : STATE_MAPPED_32_ZERO);
}

void CArmRegInfo::Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    ArmReg regHi, reglo;
    int32_t count;

    if (MipsReg == 0)
    {
        if (bHaveDebugger()) { g_Notify->DisplayError("Map_GPR_64bit\n\nWhy are you trying to map reg 0"); }
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    ProtectGPR(MipsReg);
    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        regHi = FreeArmReg();
        if (regHi < 0)
        {
            if (bHaveDebugger()) { g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers"); }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        SetArmRegProtected(regHi, true);

        reglo = FreeArmReg();
        if (reglo < 0)
        {
            if (bHaveDebugger()) { g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers"); }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        SetArmRegProtected(reglo, true);

        CPU_Message("    regcache: allocate %s to hi word of %s", ArmRegName(regHi), CRegName::GPR[MipsReg]);
        CPU_Message("    regcache: allocate %s to low word of %s", ArmRegName(reglo), CRegName::GPR[MipsReg]);
    }
    else
    {
        reglo = GetMipsRegMapLo(MipsReg);
        if (Is32Bit(MipsReg))
        {
            SetArmRegProtected(reglo, true);
            regHi = FreeArmReg();
            if (regHi < 0)
            {
                if (bHaveDebugger()) { g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers"); }
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return;
            }
            SetArmRegProtected(regHi, true);

            CPU_Message("    regcache: allocate %s to hi word of %s", ArmRegName(regHi), CRegName::GPR[MipsReg]);
        }
        else
        {
            regHi = GetMipsRegMapHi(MipsReg);
        }
    }

    for (int32_t count = 0; count <= Arm_R15; count++)
    {
        uint32_t Count = GetArmRegMapOrder((ArmReg)count);
        if (Count > 0)
        {
            SetArmRegMapOrder((ArmReg)count, Count + 1);
        }
    }
    SetArmRegMapOrder(regHi, 1);
    SetArmRegMapOrder(reglo, 1);

    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            ArmReg GprReg = Map_Variable(VARIABLE_GPR);
            LoadArmRegPointerToArmReg(regHi, GprReg, (uint8_t)(MipsRegToLoad << 3) + 4);
            LoadArmRegPointerToArmReg(reglo, GprReg, (uint8_t)(MipsRegToLoad << 3));
            SetArmRegProtected(GprReg, false);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (Is32Bit(MipsRegToLoad))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                /*if (IsSigned(MipsRegToLoad))
                {
                MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), x86Hi);
                ShiftRightSignImmed(x86Hi, 31);
                }
                else
                {
                XorX86RegToX86Reg(x86Hi, x86Hi);
                }
                if (MipsReg != MipsRegToLoad)
                {
                MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), x86lo);
                }*/
            }
            else if (MipsReg != MipsRegToLoad)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                /*MoveX86RegToX86Reg(GetMipsRegMapHi(MipsRegToLoad), x86Hi);
                MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), x86lo);*/
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            /*CPU_Message("Map_GPR_64bit 11");
            if (Is32Bit(MipsRegToLoad))
            {
            if (IsSigned(MipsRegToLoad))
            {
            MoveConstToX86reg(GetMipsRegLo_S(MipsRegToLoad) >> 31, x86Hi);
            }
            else
            {
            MoveConstToX86reg(0, x86Hi);
            }
            }
            else
            {
            MoveConstToX86reg(GetMipsRegHi(MipsRegToLoad), x86Hi);
            }
            MoveConstToX86reg(GetMipsRegLo(MipsRegToLoad), x86lo);*/
        }
    }
    else if (MipsRegToLoad == 0)
    {
        MoveConstToArmReg(regHi, (uint32_t)0);
        MoveConstToArmReg(reglo, (uint32_t)0);
    }
    SetArmRegMapped(regHi, GPR_Mapped);
    SetArmRegMapped(reglo, GPR_Mapped);
    SetArmRegProtected(regHi, true);
    SetArmRegProtected(reglo, true);
    SetMipsRegMapHi(MipsReg, regHi);
    SetMipsRegMapLo(MipsReg, reglo);
    SetMipsRegState(MipsReg, STATE_MAPPED_64);
}

void CArmRegInfo::UnMap_GPR(uint32_t MipsReg, bool WriteBackValue)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (WriteBackValue)
    {
        WriteBack_GPR(MipsReg,true);
    }

    if (MipsReg == 0)
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\n\nWhy are you trying to unmap reg 0", __FUNCTION__).c_str());
        }
        return;
    }

    if (IsUnknown(MipsReg)) { return; }
    //CPU_Message("UnMap_GPR: State: %X\tReg: %s\tWriteBack: %s",State,CRegName::GPR[Reg],WriteBackValue?"true":"false");
    if (IsConst(MipsReg))
    {
        SetMipsRegState(MipsReg, STATE_UNKNOWN);
        return;
    }
    if (Is64Bit(MipsReg))
    {
        CPU_Message("    regcache: unallocate %s from %s", ArmRegName(GetMipsRegMapHi(MipsReg)), CRegName::GPR_Hi[MipsReg]);
        SetArmRegMapped(GetMipsRegMapHi(MipsReg), NotMapped);
        SetArmRegProtected(GetMipsRegMapHi(MipsReg), false);
    }
    CPU_Message("    regcache: unallocate %s from %s", ArmRegName(GetMipsRegMapLo(MipsReg)), CRegName::GPR_Lo[MipsReg]);
    SetArmRegMapped(GetMipsRegMapLo(MipsReg), NotMapped);
    SetArmRegProtected(GetMipsRegMapLo(MipsReg), false);
    SetMipsRegState(MipsReg, STATE_UNKNOWN);
}

void CArmRegInfo::WriteBack_GPR(uint32_t MipsReg, bool Unmapping)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (MipsReg == 0)
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\n\nWhy are you trying to unmap reg 0", __FUNCTION__).c_str());
        }
        return;
    }

    if (IsUnknown(MipsReg))
    {
        return;
    }

    ArmReg GprReg = Map_Variable(VARIABLE_GPR);
    if (IsConst(MipsReg))
    {
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);

        if (Is64Bit(MipsReg))
        {
            MoveConstToArmReg(TempReg, GetMipsRegHi(MipsReg));
            StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
        }
        else if (!g_System->b32BitCore())
        {
            MoveConstToArmReg(TempReg, (GetMipsRegLo(MipsReg) & 0x80000000) != 0 ? 0xFFFFFFFF : 0);
            StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
        }
        MoveConstToArmReg(TempReg, GetMipsRegLo(MipsReg));
        StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3), CRegName::GPR_Lo[MipsReg]);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
    else
    {
        StoreArmRegToArmRegPointer(GetMipsRegMapLo(MipsReg), GprReg, (uint8_t)(MipsReg << 3), CRegName::GPR_Lo[MipsReg]);
        if (Is64Bit(MipsReg))
        {
            StoreArmRegToArmRegPointer(GetMipsRegMapHi(MipsReg), GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
        }
        else if (!g_System->b32BitCore())
        {
            bool loProtected = GetArmRegProtected(GetMipsRegMapLo(MipsReg));
            if (!Unmapping)
            {
                SetArmRegProtected(GetMipsRegMapLo(MipsReg), true);
                ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
                if (IsSigned(MipsReg))
                {
                    ShiftRightSignImmed(TempReg, GetMipsRegMapLo(MipsReg), 31);
                }
                else
                {
                    MoveConstToArmReg(TempReg, (uint32_t)0);
                }
                StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
                m_RegWorkingSet.SetArmRegProtected(TempReg, false);
            }
            else
            {
                ShiftRightSignImmed(GetMipsRegMapLo(MipsReg), GetMipsRegMapLo(MipsReg), 31);
                StoreArmRegToArmRegPointer(GetMipsRegMapLo(MipsReg), GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
            }
            m_RegWorkingSet.SetArmRegProtected(GetMipsRegMapLo(MipsReg), loProtected);
        }
    }
    SetArmRegProtected(GprReg, false);
}

void CArmRegInfo::WriteBackRegisters()
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    UnMap_AllFPRs();

    int32_t ArmRegCount = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]);
    for (int32_t i = 1; i < 32; i++) { UnMap_GPR(i,true); }
    for (int32_t i = 0; i < ArmRegCount; i++) { UnMap_ArmReg((ArmReg)i); }
    for (int32_t i = 0; i < ArmRegCount; i++) { SetArmRegProtected((ArmReg)i, false); }

    for (int32_t count = 1; count < 32; count++)
    {
        switch (GetMipsRegState(count))
        {
        case STATE_UNKNOWN: break;
        case STATE_CONST_32_SIGN:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            break;
        case STATE_CONST_32_ZERO:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            break;
        case STATE_CONST_64:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            break;
        default:
            CPU_Message("%s: Unknown State: %d reg %d (%s)", __FUNCTION__, GetMipsRegState(count), count, CRegName::GPR[count]);
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CArmRegInfo::UnMap_AllFPRs()
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    CPU_Message("%s", __FUNCTION__);
}

CArmOps::ArmReg CArmRegInfo::UnMap_TempReg()
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    ArmReg Reg = Arm_Unknown;

    if (GetArmRegMapped(Arm_R7) == Temp_Mapped && !GetArmRegProtected(Arm_R7)) { return Arm_R7; }
    if (GetArmRegMapped(Arm_R6) == Temp_Mapped && !GetArmRegProtected(Arm_R6)) { return Arm_R6; }
    if (GetArmRegMapped(Arm_R5) == Temp_Mapped && !GetArmRegProtected(Arm_R5)) { return Arm_R5; }
    if (GetArmRegMapped(Arm_R4) == Temp_Mapped && !GetArmRegProtected(Arm_R4)) { return Arm_R4; }
    if (GetArmRegMapped(Arm_R3) == Temp_Mapped && !GetArmRegProtected(Arm_R3)) { return Arm_R3; }
    if (GetArmRegMapped(Arm_R2) == Temp_Mapped && !GetArmRegProtected(Arm_R2)) { return Arm_R2; }
    if (GetArmRegMapped(Arm_R1) == Temp_Mapped && !GetArmRegProtected(Arm_R1)) { return Arm_R1; }
    if (GetArmRegMapped(Arm_R0) == Temp_Mapped && !GetArmRegProtected(Arm_R0)) { return Arm_R0; }
    if (GetArmRegMapped(Arm_R12) == Temp_Mapped && !GetArmRegProtected(Arm_R12)) { return Arm_R12; }
    if (GetArmRegMapped(Arm_R11) == Temp_Mapped && !GetArmRegProtected(Arm_R11)) { return Arm_R11; }
    if (GetArmRegMapped(Arm_R10) == Temp_Mapped && !GetArmRegProtected(Arm_R10)) { return Arm_R10; }
    if (GetArmRegMapped(Arm_R9) == Temp_Mapped && !GetArmRegProtected(Arm_R9)) { return Arm_R9; }
    if (GetArmRegMapped(Arm_R8) == Temp_Mapped && !GetArmRegProtected(Arm_R8)) { return Arm_R8; }

    if (Reg != Arm_Unknown)
    {
        if (GetArmRegMapped(Reg) == Temp_Mapped)
        {
            CPU_Message("    regcache: unallocate %s from temp storage", ArmRegName(Reg));
        }
        SetArmRegMapped(Reg, NotMapped);
    }
    return Reg;
}

bool CArmRegInfo::UnMap_ArmReg(ArmReg Reg)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }
    if (GetArmRegProtected(Reg))
    {
        CPU_Message("%s: %s is protected",__FUNCTION__,ArmRegName(Reg));
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }
    if (GetArmRegMapped(Reg) == NotMapped)
    {
        return true;
    }
    else if (GetArmRegMapped(Reg) == GPR_Mapped)
    {
        for (uint32_t count = 1; count < 32; count++)
        {
            if (!IsMapped(count))
            {
                continue;
            }

            if (Is64Bit(count) && GetMipsRegMapHi(count) == Reg)
            {
                if (!GetArmRegProtected(Reg))
                {
                    UnMap_GPR(count, true);
                    return true;
                }
                break;
            }
            if (GetMipsRegMapLo(count) == Reg)
            {
                if (!GetArmRegProtected(Reg))
                {
                    UnMap_GPR(count, true);
                    return true;
                }
                break;
            }
        }
    }
    else if (GetArmRegMapped(Reg) == Temp_Mapped)
    {
        CPU_Message("    regcache: unallocate %s from temp storage", ArmRegName(Reg));
        SetArmRegMapped(Reg, NotMapped);
        return true;
    }
    else if (GetArmRegMapped(Reg) == Variable_Mapped)
    {
        CPU_Message("    regcache: unallocate %s from variable mapping (%s)", ArmRegName(Reg), VariableMapName(GetVariableMappedTo(Reg)));
        SetArmRegMapped(Reg, NotMapped);
        m_Variable_MappedTo[Reg] = VARIABLE_UNKNOWN;
        return true;
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CArmRegInfo::ResetRegProtection()
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    for (uint32_t i = 0, n = sizeof(m_ArmReg_Protected) / sizeof(m_ArmReg_Protected[0]); i < n; i++)
    {
        SetArmRegProtected((ArmReg)i, false);
    }
}

CArmOps::ArmReg CArmRegInfo::FreeArmReg()
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    if ((GetArmRegMapped(Arm_R7) == NotMapped || GetArmRegMapped(Arm_R7) == Temp_Mapped) && !GetArmRegProtected(Arm_R7)) { return Arm_R7; }
    if ((GetArmRegMapped(Arm_R6) == NotMapped || GetArmRegMapped(Arm_R6) == Temp_Mapped) && !GetArmRegProtected(Arm_R6)) { return Arm_R6; }
    if ((GetArmRegMapped(Arm_R5) == NotMapped || GetArmRegMapped(Arm_R5) == Temp_Mapped) && !GetArmRegProtected(Arm_R5)) { return Arm_R5; }
    if ((GetArmRegMapped(Arm_R4) == NotMapped || GetArmRegMapped(Arm_R4) == Temp_Mapped) && !GetArmRegProtected(Arm_R4)) { return Arm_R4; }
    if ((GetArmRegMapped(Arm_R3) == NotMapped || GetArmRegMapped(Arm_R3) == Temp_Mapped) && !GetArmRegProtected(Arm_R3)) { return Arm_R3; }
    if ((GetArmRegMapped(Arm_R2) == NotMapped || GetArmRegMapped(Arm_R2) == Temp_Mapped) && !GetArmRegProtected(Arm_R2)) { return Arm_R2; }
    if ((GetArmRegMapped(Arm_R1) == NotMapped || GetArmRegMapped(Arm_R1) == Temp_Mapped) && !GetArmRegProtected(Arm_R1)) { return Arm_R1; }
    if ((GetArmRegMapped(Arm_R0) == NotMapped || GetArmRegMapped(Arm_R0) == Temp_Mapped) && !GetArmRegProtected(Arm_R0)) { return Arm_R0; }
    if ((GetArmRegMapped(Arm_R12) == NotMapped || GetArmRegMapped(Arm_R12) == Temp_Mapped) && !GetArmRegProtected(Arm_R12)) { return Arm_R12; }
    if ((GetArmRegMapped(Arm_R11) == NotMapped || GetArmRegMapped(Arm_R11) == Temp_Mapped) && !GetArmRegProtected(Arm_R11)) { return Arm_R11; }
    if ((GetArmRegMapped(Arm_R10) == NotMapped || GetArmRegMapped(Arm_R10) == Temp_Mapped) && !GetArmRegProtected(Arm_R10)) { return Arm_R10; }
    if ((GetArmRegMapped(Arm_R9) == NotMapped || GetArmRegMapped(Arm_R9) == Temp_Mapped) && !GetArmRegProtected(Arm_R9)) { return Arm_R9; }
    if ((GetArmRegMapped(Arm_R8) == NotMapped || GetArmRegMapped(Arm_R8) == Temp_Mapped) && !GetArmRegProtected(Arm_R8)) { return Arm_R8; }

    ArmReg Reg = UnMap_TempReg();
    if (Reg != Arm_Unknown) { return Reg; }

    int32_t MapCount[sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0])];
    ArmReg MapReg[sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0])];

    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        MapCount[i] = GetArmRegMapOrder((ArmReg)i);
        MapReg[i] = (ArmReg)i;
    }
    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        bool changed = false;
        for (int32_t z = 0; z < n - 1; z++)
        {
            if (MapCount[z] >= MapCount[z + 1])
            {
                continue;
            }
            uint32_t temp = MapCount[z];
            MapCount[z] = MapCount[z + 1];
            MapCount[z + 1] = temp;
            ArmReg tempReg = MapReg[z];
            MapReg[z] = MapReg[z + 1];
            MapReg[z + 1] = tempReg;
            changed = true;
        }
        if (!changed)
        {
            break;
        }
    }

    ArmReg StackReg = Arm_Unknown;
    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        if (MapCount[i] > 0 && GetArmRegMapped(MapReg[i]) == GPR_Mapped && !GetArmRegProtected((ArmReg)MapReg[i]))
        {
            if (UnMap_ArmReg((ArmReg)MapReg[i]))
            {
                return (ArmReg)MapReg[i];
            }
        }
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return Arm_Unknown;
}

CArmOps::ArmReg CArmRegInfo::Map_TempReg(ArmReg Reg, int32_t MipsReg, bool LoadHiWord)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    ArmReg GprReg = MipsReg >= 0 ? Map_Variable(VARIABLE_GPR) : Arm_Unknown;

    if (Reg == CArmOps::Arm_Any)
    {
        if (GetArmRegMapped(Arm_R7) == Temp_Mapped && !GetArmRegProtected(Arm_R7)) { Reg = Arm_R7; }
        else if (GetArmRegMapped(Arm_R6) == Temp_Mapped && !GetArmRegProtected(Arm_R6)) { Reg = Arm_R6; }
        else if (GetArmRegMapped(Arm_R5) == Temp_Mapped && !GetArmRegProtected(Arm_R5)) { Reg = Arm_R5; }
        else if (GetArmRegMapped(Arm_R4) == Temp_Mapped && !GetArmRegProtected(Arm_R4)) { Reg = Arm_R4; }
        else if (GetArmRegMapped(Arm_R3) == Temp_Mapped && !GetArmRegProtected(Arm_R3)) { Reg = Arm_R3; }
        else if (GetArmRegMapped(Arm_R2) == Temp_Mapped && !GetArmRegProtected(Arm_R2)) { Reg = Arm_R2; }
        else if (GetArmRegMapped(Arm_R1) == Temp_Mapped && !GetArmRegProtected(Arm_R1)) { Reg = Arm_R1; }
        else if (GetArmRegMapped(Arm_R0) == Temp_Mapped && !GetArmRegProtected(Arm_R0)) { Reg = Arm_R0; }
        else if (GetArmRegMapped(Arm_R12) == Temp_Mapped && !GetArmRegProtected(Arm_R12)) { Reg = Arm_R12; }
        else if (GetArmRegMapped(Arm_R11) == Temp_Mapped && !GetArmRegProtected(Arm_R11)) { Reg = Arm_R11; }
        else if (GetArmRegMapped(Arm_R10) == Temp_Mapped && !GetArmRegProtected(Arm_R10)) { Reg = Arm_R10; }
        else if (GetArmRegMapped(Arm_R9) == Temp_Mapped && !GetArmRegProtected(Arm_R9)) { Reg = Arm_R9; }
        else if (GetArmRegMapped(Arm_R8) == Temp_Mapped && !GetArmRegProtected(Arm_R8)) { Reg = Arm_R8; }

        if (Reg == Arm_Any)
        {
            Reg = FreeArmReg();
            if (Reg == Arm_Unknown)
            {
                WriteTrace(TraceRegisterCache, TraceError, "Failed to find a free register");
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return Arm_Unknown;
            }
        }
    }
    else if (GetArmRegMapped(Reg) == NotMapped || GetArmRegMapped(Reg) == Temp_Mapped)
    {
        if (GetArmRegProtected(Reg))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (GetArmRegMapped(Reg) == GPR_Mapped)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (MipsReg < 0)
    {
        CPU_Message("    regcache: allocate %s as temp storage", ArmRegName(Reg));
    }
    else
    {
        CPU_Message("    regcache: allocate %s as temp storage (%s)", ArmRegName(Reg), LoadHiWord ? CRegName::GPR_Hi[MipsReg] : CRegName::GPR_Lo[MipsReg]);
        if (GprReg == Arm_Unknown)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (LoadHiWord)
        {
            if (IsUnknown(MipsReg))
            {
                LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(MipsReg << 3) + 4);
            }
            else if (IsMapped(MipsReg))
            {
                if (Is64Bit(MipsReg))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    //MoveArmRegToArmReg(GetMipsRegMapHi(MipsReg), Reg);
                }
                else if (IsSigned(MipsReg))
                {
                    ShiftRightSignImmed(Reg,GetMipsRegMapLo(MipsReg),31);
                }
                else
                {
                    MoveConstToArmReg(Reg, (uint32_t)0);
                }
            }
            else
            {
                if (Is64Bit(MipsReg))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    //MoveConstToArmReg(Reg, GetMipsRegHi(MipsReg));
                }
                else
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    //MoveConstToArmReg(Reg, GetMipsRegLo_S(MipsReg) >> 31);
                }
            }
        }
        else
        {
            if (IsUnknown(MipsReg))
            {
                LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(MipsReg << 3));
            }
            else if (IsMapped(MipsReg))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                //MoveArmRegToArmReg(GetMipsRegMapLo(MipsReg), Reg);
            }
            else
            {
                MoveConstToArmReg(Reg, GetMipsRegLo(MipsReg));
            }
        }
    }
    SetArmRegMapped(Reg, Temp_Mapped);
    SetArmRegProtected(Reg, true);
    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        int32_t MapOrder = GetArmRegMapOrder((ArmReg)i);
        if (MapOrder > 0)
        {
            SetArmRegMapOrder((ArmReg)i, MapOrder + 1);
        }
    }
    SetArmRegMapOrder(Reg, 1);
    SetArmRegProtected(GprReg, false);
    return Reg;
}

CArmOps::ArmReg CArmRegInfo::Map_Variable(VARIABLE_MAPPED variable)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        if (m_ArmReg_MappedTo[i] == Variable_Mapped && m_Variable_MappedTo[i] == variable)
        {
            SetArmRegProtected((ArmReg)i, true);
            return (ArmReg)i;
        }
    }

    ArmReg Reg = FreeArmReg();
    if (Reg == Arm_Unknown)
    {
        WriteTrace(TraceRegisterCache, TraceError, "Failed to find a free register");
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    SetArmRegMapped(Reg, Variable_Mapped);
    SetArmRegProtected(Reg, true);

    switch (variable)
    {
    case VARIABLE_GPR:
        CPU_Message("    regcache: allocate %s as pointer to GPR", ArmRegName(Reg));
        m_Variable_MappedTo[Reg] = variable;
        MoveConstToArmReg(Reg, (uint32_t)_GPR, "_GPR");
        break;
    case VARIABLE_FPR:
        CPU_Message("    regcache: allocate %s as pointer to _FPR_S", ArmRegName(Reg));
        m_Variable_MappedTo[Reg] = variable;
        MoveConstToArmReg(Reg,(uint32_t)_FPR_S,"_FPR_S");
        break;
    case VARIABLE_TLB_READMAP:
        CPU_Message("    regcache: allocate %s as pointer to TLB_READMAP", ArmRegName(Reg));
        m_Variable_MappedTo[Reg] = variable;
        MoveConstToArmReg(Reg, (uint32_t)(g_MMU->m_TLB_ReadMap), "MMU->TLB_ReadMap");
        break;
    case VARIABLE_NEXT_TIMER:
        CPU_Message("    regcache: allocate %s as pointer to g_NextTimer", ArmRegName(Reg));
        m_Variable_MappedTo[Reg] = variable;
        MoveConstToArmReg(Reg, (uint32_t)(g_NextTimer), "g_NextTimer");
        break;
    case VARIABLE_TLB_LOAD_ADDRESS:
        CPU_Message("    regcache: allocate %s as pointer to g_TLBLoadAddress", ArmRegName(Reg));
        m_Variable_MappedTo[Reg] = variable;
        MoveConstToArmReg(Reg, (uint32_t)(g_TLBLoadAddress), "g_TLBLoadAddress");
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    return Reg;
}

void CArmRegInfo::ProtectGPR(uint32_t Reg)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (IsUnknown(Reg) || IsConst(Reg))
    {
        return;
    }
    if (Is64Bit(Reg))
    {
        SetArmRegProtected(GetMipsRegMapHi(Reg), true);
    }
    SetArmRegProtected(GetMipsRegMapLo(Reg), true);
}

const char * CArmRegInfo::VariableMapName(VARIABLE_MAPPED variable)
{
    switch (variable)
    {
    case VARIABLE_UNKNOWN: return "UNKNOWN";
    case VARIABLE_GPR: return "_GPR";
    case VARIABLE_FPR: return "_FPR_S";
    case VARIABLE_TLB_READMAP: return "m_TLB_ReadMap";
    case VARIABLE_TLB_LOAD_ADDRESS: return "g_TLBLoadAddress";
    case VARIABLE_NEXT_TIMER: return "g_NextTimer";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return "unknown VariableMapName";
    }
}

#endif