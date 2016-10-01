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

void CArmRegInfo::BeforeCallDirect(void)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_InCallDirect = true;
}

void CArmRegInfo::AfterCallDirect(void)
{
    if (!m_InCallDirect)
    {
        CPU_Message("%s: Not in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
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

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return Arm_Unknown;
}

void CArmRegInfo::WriteBackRegisters()
{
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
                g_Notify->BreakPoint(__FILE__, __LINE__);
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
        CPU_Message("%s: in CallDirect", __FUNCTION__);
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
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return Arm_Unknown;
    }
    return Reg;
}

#endif