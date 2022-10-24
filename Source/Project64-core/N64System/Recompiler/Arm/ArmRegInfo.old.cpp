#include "stdafx.h"

#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/SystemGlobals.h>

CArmRegInfo::CArmRegInfo(CCodeBlock & CodeBlock, CArmOps & Assembler) :
    m_CodeBlock(CodeBlock),
    m_Assembler(Assembler),
    m_InCallDirect(false)
{
    for (int32_t i = 0; i < 32; i++)
    {
        m_RegMapLo[i] = CArmOps::Arm_Unknown;
        m_RegMapHi[i] = CArmOps::Arm_Unknown;
    }

    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        m_ArmReg_MapOrder[i] = 0;
        m_ArmReg_Protected[i] = false;
        m_ArmReg_MappedTo[i] = NotMapped;
        m_Variable_MappedTo[i] = VARIABLE_UNKNOWN;
    }
}

CArmRegInfo::CArmRegInfo(const CArmRegInfo & rhs) :
    m_CodeBlock(rhs.m_CodeBlock),
    m_Assembler(rhs.m_CodeBlock.RecompilerOps()->Assembler())
{
    *this = rhs;
}

CArmRegInfo::~CArmRegInfo()
{
}

CArmRegInfo & CArmRegInfo::operator=(const CArmRegInfo & right)
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

bool CArmRegInfo::operator==(const CArmRegInfo & right) const
{
    if (!CRegBase::operator==(right))
    {
        return false;
    }

    for (int32_t count = 0; count < 32; count++)
    {
        if (m_RegMapHi[count] != right.m_RegMapHi[count])
        {
            return false;
        }
        if (m_RegMapLo[count] != right.m_RegMapLo[count])
        {
            return false;
        }
    }

    for (int32_t count = 0; count < 16; count++)
    {
        if (m_ArmReg_MapOrder[count] != right.m_ArmReg_MapOrder[count])
        {
            return false;
        }
        if (m_ArmReg_Protected[count] != right.m_ArmReg_Protected[count])
        {
            return false;
        }
        if (m_ArmReg_MappedTo[count] != right.m_ArmReg_MappedTo[count])
        {
            return false;
        }
        if (m_Variable_MappedTo[count] != right.m_Variable_MappedTo[count])
        {
            return false;
        }
    }
    return true;
}

bool CArmRegInfo::ShouldPushPopReg(CArmOps::ArmReg Reg)
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
    static uint32_t PushPopRegisterList[] = {
        CArmOps::ArmPushPop_R0,
        CArmOps::ArmPushPop_R1,
        CArmOps::ArmPushPop_R2,
        CArmOps::ArmPushPop_R3,
        CArmOps::ArmPushPop_R4,
        CArmOps::ArmPushPop_R5,
        CArmOps::ArmPushPop_R6,
        CArmOps::ArmPushPop_R7,
        CArmOps::ArmPushPop_R8,
        CArmOps::ArmPushPop_R9,
        CArmOps::ArmPushPop_R10,
        CArmOps::ArmPushPop_R11,
        CArmOps::ArmPushPop_R12,
    };

    static CArmOps::ArmReg RegisterList[] = {
        CArmOps::Arm_R0,
        CArmOps::Arm_R1,
        CArmOps::Arm_R2,
        CArmOps::Arm_R3,
        CArmOps::Arm_R4,
        CArmOps::Arm_R5,
        CArmOps::Arm_R6,
        CArmOps::Arm_R7,
        CArmOps::Arm_R8,
        CArmOps::Arm_R9,
        CArmOps::Arm_R10,
        CArmOps::Arm_R11,
        CArmOps::Arm_R12,
    };

    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    UnMap_AllFPRs();
    int PushPopRegisters = 0;
    for (int i = 0; i < (sizeof(RegisterList) / sizeof(RegisterList[0])); i++)
    {
        if (ShouldPushPopReg(RegisterList[i]))
        {
            PushPopRegisters |= PushPopRegisterList[i];
        }
    }

    if (PushPopRegisters == 0)
    {
        m_InCallDirect = true;
        return;
    }

    if ((m_Assembler.PushPopRegisterSize(PushPopRegisters) % 8) != 0)
    {
        bool Added = false;
        for (int i = 0; i < (sizeof(RegisterList) / sizeof(RegisterList[0])); i++)
        {
            if (ShouldPushPopReg(RegisterList[i]))
            {
                continue;
            }
            PushPopRegisters |= PushPopRegisterList[i];
            Added = true;
            break;
        }
        if (!Added)
        {
            CArmOps::ArmReg reg = FreeArmReg(false);
            m_CodeBlock.Log("    Freed %s", m_Assembler.ArmRegName(reg));
            PushPopRegisters = 0;
            for (int i = 0; i < (sizeof(RegisterList) / sizeof(RegisterList[0])); i++)
            {
                if (ShouldPushPopReg(RegisterList[i]))
                {
                    PushPopRegisters |= PushPopRegisterList[i];
                }
            }
        }
        if ((m_Assembler.PushPopRegisterSize(PushPopRegisters) % 8) != 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    m_InCallDirect = true;
    m_Assembler.PushArmReg(PushPopRegisters);
}

void CArmRegInfo::AfterCallDirect(void)
{
    static uint32_t PushPopRegisterList[] = {
        CArmOps::ArmPushPop_R0,
        CArmOps::ArmPushPop_R1,
        CArmOps::ArmPushPop_R2,
        CArmOps::ArmPushPop_R3,
        CArmOps::ArmPushPop_R4,
        CArmOps::ArmPushPop_R5,
        CArmOps::ArmPushPop_R6,
        CArmOps::ArmPushPop_R7,
        CArmOps::ArmPushPop_R8,
        CArmOps::ArmPushPop_R9,
        CArmOps::ArmPushPop_R10,
        CArmOps::ArmPushPop_R11,
        CArmOps::ArmPushPop_R12,
        CArmOps::ArmPushPop_LR,
        CArmOps::ArmPushPop_PC,
    };

    static CArmOps::ArmReg RegisterList[] = {
        CArmOps::Arm_R0,
        CArmOps::Arm_R1,
        CArmOps::Arm_R2,
        CArmOps::Arm_R3,
        CArmOps::Arm_R4,
        CArmOps::Arm_R5,
        CArmOps::Arm_R6,
        CArmOps::Arm_R7,
        CArmOps::Arm_R8,
        CArmOps::Arm_R9,
        CArmOps::Arm_R10,
        CArmOps::Arm_R11,
        CArmOps::Arm_R12,
        CArmOps::ArmRegLR,
        CArmOps::ArmRegPC,
    };

    if (!m_InCallDirect)
    {
        m_CodeBlock.Log("%s: Not in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    int PushPopRegisters = 0;
    for (int i = 0; i < (sizeof(RegisterList) / sizeof(RegisterList[0])); i++)
    {
        if (ShouldPushPopReg(RegisterList[i]))
        {
            PushPopRegisters |= PushPopRegisterList[i];
        }
    }

    if (PushPopRegisters != 0)
    {
        if ((m_Assembler.PushPopRegisterSize(PushPopRegisters) % 8) != 0)
        {
            for (int i = 0; i < (sizeof(RegisterList) / sizeof(RegisterList[0])); i++)
            {
                if (ShouldPushPopReg(RegisterList[i]))
                {
                    continue;
                }
                PushPopRegisters |= PushPopRegisterList[i];
                break;
            }
        }
        m_Assembler.PopArmReg(PushPopRegisters);
    }

    SetRoundingModel(CRegInfo::RoundUnknown);
    m_InCallDirect = false;
}

void CArmRegInfo::FixRoundModel(FPU_ROUND RoundMethod)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (GetRoundingModel() == RoundMethod)
    {
        return;
    }
    m_CodeBlock.Log("    FixRoundModel: CurrentRoundingModel: %s  targetRoundModel: %s", RoundingModelName(GetRoundingModel()), RoundingModelName(RoundMethod));
    if (RoundMethod == RoundDefault)
    {
        BeforeCallDirect();
        m_Assembler.MoveVariableToArmReg(_RoundingModel, "_RoundingModel", CArmOps::Arm_R0);
        m_Assembler.CallFunction((void *)fesetround, "fesetround");
        AfterCallDirect();
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
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    CArmOps::ArmReg Reg;
    if (MipsReg == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        Reg = FreeArmReg(false);
        if (Reg < 0)
        {
            if (HaveDebugger())
            {
                g_Notify->DisplayError("Map_GPR_32bit\n\nOut of registers");
            }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        SetArmRegProtected(Reg, true);
        m_CodeBlock.Log("    regcache: allocate %s to %s", m_Assembler.ArmRegName(Reg), CRegName::GPR[MipsReg]);
    }
    else
    {
        if (Is64Bit(MipsReg))
        {
            m_CodeBlock.Log("    regcache: unallocate %s from high 32-bit of %s", m_Assembler.ArmRegName(GetMipsRegMapHi(MipsReg)), CRegName::GPR_Hi[MipsReg]);
            SetArmRegMapOrder(GetMipsRegMapHi(MipsReg), 0);
            SetArmRegMapped(GetMipsRegMapHi(MipsReg), NotMapped);
            SetArmRegProtected(GetMipsRegMapHi(MipsReg), false);
            SetMipsRegHi(MipsReg, 0);
        }
        Reg = GetMipsRegMapLo(MipsReg);
    }
    for (int32_t count = 0; count <= CArmOps::Arm_R15; count++)
    {
        uint32_t Count = GetArmRegMapOrder((CArmOps::ArmReg)count);
        if (Count > 0)
        {
            SetArmRegMapOrder((CArmOps::ArmReg)count, Count + 1);
        }
    }
    SetArmRegMapOrder(Reg, 1);

    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            CArmOps::ArmReg GprReg = Map_Variable(VARIABLE_GPR);
            m_Assembler.LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(MipsRegToLoad << 3), CRegName::GPR_Lo[MipsRegToLoad]);
            SetArmRegProtected(GprReg, false);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (MipsReg != MipsRegToLoad)
            {
                m_Assembler.AddConstToArmReg(Reg, GetMipsRegMapLo(MipsRegToLoad), 0);
            }
        }
        else
        {
            m_Assembler.MoveConstToArmReg(Reg, GetMipsRegLo(MipsRegToLoad));
        }
    }
    else if (MipsRegToLoad == 0)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)0);
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
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    CArmOps::ArmReg regHi, reglo;
    int32_t count;

    if (MipsReg == 0)
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError("Map_GPR_64bit\n\nWhy are you trying to map register 0?");
        }
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    ProtectGPR(MipsReg);
    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        regHi = FreeArmReg(false);
        if (regHi < 0)
        {
            if (HaveDebugger())
            {
                g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers");
            }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        SetArmRegProtected(regHi, true);

        reglo = FreeArmReg(false);
        if (reglo < 0)
        {
            if (HaveDebugger())
            {
                g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers");
            }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        SetArmRegProtected(reglo, true);

        m_CodeBlock.Log("    regcache: allocate %s to hi word of %s", m_Assembler.ArmRegName(regHi), CRegName::GPR[MipsReg]);
        m_CodeBlock.Log("    regcache: allocate %s to low word of %s", m_Assembler.ArmRegName(reglo), CRegName::GPR[MipsReg]);
    }
    else
    {
        reglo = GetMipsRegMapLo(MipsReg);
        if (Is32Bit(MipsReg))
        {
            SetArmRegProtected(reglo, true);
            regHi = FreeArmReg(false);
            if (regHi < 0)
            {
                if (HaveDebugger())
                {
                    g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers");
                }
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return;
            }
            SetArmRegProtected(regHi, true);

            m_CodeBlock.Log("    regcache: allocate %s to hi word of %s", m_Assembler.ArmRegName(regHi), CRegName::GPR[MipsReg]);
        }
        else
        {
            regHi = GetMipsRegMapHi(MipsReg);
        }
    }

    for (int32_t count = 0; count <= CArmOps::Arm_R15; count++)
    {
        uint32_t Count = GetArmRegMapOrder((CArmOps::ArmReg)count);
        if (Count > 0)
        {
            SetArmRegMapOrder((CArmOps::ArmReg)count, Count + 1);
        }
    }
    SetArmRegMapOrder(regHi, 1);
    SetArmRegMapOrder(reglo, 1);

    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            CArmOps::ArmReg GprReg = Map_Variable(VARIABLE_GPR);
            m_Assembler.LoadArmRegPointerToArmReg(regHi, GprReg, (uint8_t)(MipsRegToLoad << 3) + 4, CRegName::GPR_Hi[MipsRegToLoad]);
            m_Assembler.LoadArmRegPointerToArmReg(reglo, GprReg, (uint8_t)(MipsRegToLoad << 3), CRegName::GPR_Lo[MipsRegToLoad]);
            SetArmRegProtected(GprReg, false);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (Is32Bit(MipsRegToLoad))
            {
                if (IsSigned(MipsRegToLoad))
                {
                    m_Assembler.ShiftRightSignImmed(regHi, GetMipsRegMapLo(MipsRegToLoad), 31);
                }
                else
                {
                    m_Assembler.MoveConstToArmReg(regHi, (uint32_t)0);
                }
                if (MipsReg != MipsRegToLoad)
                {
                    m_Assembler.AddConstToArmReg(reglo, GetMipsRegMapLo(MipsRegToLoad), 0);
                }
            }
            else if (MipsReg != MipsRegToLoad)
            {
                m_Assembler.AddConstToArmReg(regHi, GetMipsRegMapHi(MipsRegToLoad), 0);
                m_Assembler.AddConstToArmReg(reglo, GetMipsRegMapLo(MipsRegToLoad), 0);
            }
        }
        else
        {
            if (Is32Bit(MipsRegToLoad))
            {
                m_Assembler.MoveConstToArmReg(regHi, (uint32_t)(IsSigned(MipsRegToLoad) ? GetMipsRegLo_S(MipsRegToLoad) >> 31 : 0));
            }
            else
            {
                m_Assembler.MoveConstToArmReg(regHi, GetMipsRegHi(MipsRegToLoad));
            }
            m_Assembler.MoveConstToArmReg(reglo, GetMipsRegLo(MipsRegToLoad));
        }
    }
    else if (MipsRegToLoad == 0)
    {
        m_Assembler.MoveConstToArmReg(regHi, (uint32_t)0);
        m_Assembler.MoveConstToArmReg(reglo, (uint32_t)0);
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
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (WriteBackValue)
    {
        WriteBack_GPR(MipsReg, true);
    }

    if (MipsReg == 0)
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\n\nWhy are you trying to unmap register 0?", __FUNCTION__).c_str());
        }
        return;
    }

    if (IsUnknown(MipsReg))
    {
        return;
    }
    //m_CodeBlock.Log("UnMap_GPR: State: %X\tReg: %s\tWriteBack: %s",State,CRegName::GPR[Reg],WriteBackValue?"true":"false");
    if (IsConst(MipsReg))
    {
        SetMipsRegState(MipsReg, STATE_UNKNOWN);
        return;
    }
    if (Is64Bit(MipsReg))
    {
        m_CodeBlock.Log("    regcache: unallocate %s from %s", m_Assembler.ArmRegName(GetMipsRegMapHi(MipsReg)), CRegName::GPR_Hi[MipsReg]);
        SetArmRegMapped(GetMipsRegMapHi(MipsReg), NotMapped);
        SetArmRegProtected(GetMipsRegMapHi(MipsReg), false);
    }
    m_CodeBlock.Log("    regcache: unallocate %s from %s", m_Assembler.ArmRegName(GetMipsRegMapLo(MipsReg)), CRegName::GPR_Lo[MipsReg]);
    SetArmRegMapped(GetMipsRegMapLo(MipsReg), NotMapped);
    SetArmRegProtected(GetMipsRegMapLo(MipsReg), false);
    SetMipsRegState(MipsReg, STATE_UNKNOWN);
}

void CArmRegInfo::WriteBack_GPR(uint32_t MipsReg, bool Unmapping)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (MipsReg == 0)
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\n\nWhy are you trying to unmap register 0?", __FUNCTION__).c_str());
        }
        return;
    }

    if (IsUnknown(MipsReg))
    {
        return;
    }

    CArmOps::ArmReg GprReg = Map_Variable(VARIABLE_GPR);
    if (IsConst(MipsReg))
    {
        CArmOps::ArmReg TempReg = Map_TempReg(CArmOps::Arm_Any, -1, false);

        if (Is64Bit(MipsReg))
        {
            m_Assembler.MoveConstToArmReg(TempReg, GetMipsRegHi(MipsReg));
            m_Assembler.StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
        }
        else if (!g_System->b32BitCore())
        {
            m_Assembler.MoveConstToArmReg(TempReg, (GetMipsRegLo(MipsReg) & 0x80000000) != 0 ? 0xFFFFFFFF : 0);
            m_Assembler.StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
        }
        m_Assembler.MoveConstToArmReg(TempReg, GetMipsRegLo(MipsReg));
        m_Assembler.StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3), CRegName::GPR_Lo[MipsReg]);
        SetArmRegProtected(TempReg, false);
    }
    else
    {
        m_Assembler.StoreArmRegToArmRegPointer(GetMipsRegMapLo(MipsReg), GprReg, (uint8_t)(MipsReg << 3), CRegName::GPR_Lo[MipsReg]);
        if (Is64Bit(MipsReg))
        {
            m_Assembler.StoreArmRegToArmRegPointer(GetMipsRegMapHi(MipsReg), GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
        }
        else if (!g_System->b32BitCore())
        {
            bool loProtected = GetArmRegProtected(GetMipsRegMapLo(MipsReg));
            if (!Unmapping)
            {
                SetArmRegProtected(GetMipsRegMapLo(MipsReg), true);
                CArmOps::ArmReg TempReg = Map_TempReg(CArmOps::Arm_Any, -1, false);
                if (IsSigned(MipsReg))
                {
                    m_Assembler.ShiftRightSignImmed(TempReg, GetMipsRegMapLo(MipsReg), 31);
                }
                else
                {
                    m_Assembler.MoveConstToArmReg(TempReg, (uint32_t)0);
                }
                m_Assembler.StoreArmRegToArmRegPointer(TempReg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
                SetArmRegProtected(TempReg, false);
            }
            else
            {
                m_Assembler.ShiftRightSignImmed(GetMipsRegMapLo(MipsReg), GetMipsRegMapLo(MipsReg), 31);
                m_Assembler.StoreArmRegToArmRegPointer(GetMipsRegMapLo(MipsReg), GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
            }
            SetArmRegProtected(GetMipsRegMapLo(MipsReg), loProtected);
        }
    }
    SetArmRegProtected(GprReg, false);
}

void CArmRegInfo::WriteBackRegisters()
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    UnMap_AllFPRs();

    int32_t ArmRegCount = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]);
    for (int32_t i = 1; i < 32; i++)
    {
        UnMap_GPR(i, true);
    }
    for (int32_t i = 0; i < ArmRegCount; i++)
    {
        UnMap_ArmReg((CArmOps::ArmReg)i);
    }
    for (int32_t i = 0; i < ArmRegCount; i++)
    {
        SetArmRegProtected((CArmOps::ArmReg)i, false);
    }

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
            m_CodeBlock.Log("%s: Unknown state: %d reg %d (%s)", __FUNCTION__, GetMipsRegState(count), count, CRegName::GPR[count]);
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CArmRegInfo::UnMap_AllFPRs()
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_CodeBlock.Log("%s", __FUNCTION__);
}

CArmOps::ArmReg CArmRegInfo::UnMap_TempReg(bool TempMapping)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }
    CArmOps::ArmReg Reg = CArmOps::Arm_Unknown;

    if (GetArmRegMapped(CArmOps::Arm_R7) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R7))
    {
        return CArmOps::Arm_R7;
    }
    if (GetArmRegMapped(CArmOps::Arm_R6) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R6))
    {
        return CArmOps::Arm_R6;
    }
    if (GetArmRegMapped(CArmOps::Arm_R5) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R5))
    {
        return CArmOps::Arm_R5;
    }
    if (GetArmRegMapped(CArmOps::Arm_R4) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R4))
    {
        return CArmOps::Arm_R4;
    }
    if (GetArmRegMapped(CArmOps::Arm_R3) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R3))
    {
        return CArmOps::Arm_R3;
    }
    if (GetArmRegMapped(CArmOps::Arm_R2) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R2))
    {
        return CArmOps::Arm_R2;
    }
    if (GetArmRegMapped(CArmOps::Arm_R1) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R1))
    {
        return CArmOps::Arm_R1;
    }
    if (GetArmRegMapped(CArmOps::Arm_R0) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R0))
    {
        return CArmOps::Arm_R0;
    }
    if (TempMapping)
    {
        if (GetArmRegMapped(CArmOps::Arm_R11) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R11))
        {
            return CArmOps::Arm_R11;
        }
        if (GetArmRegMapped(CArmOps::Arm_R10) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R10))
        {
            return CArmOps::Arm_R10;
        }
    }
    if (GetArmRegMapped(CArmOps::Arm_R9) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R9))
    {
        return CArmOps::Arm_R9;
    }
    if (GetArmRegMapped(CArmOps::Arm_R8) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R8))
    {
        return CArmOps::Arm_R8;
    }

    if (Reg != CArmOps::Arm_Unknown)
    {
        if (GetArmRegMapped(Reg) == Temp_Mapped)
        {
            m_CodeBlock.Log("    regcache: unallocate %s from temp storage", m_Assembler.ArmRegName(Reg));
        }
        SetArmRegMapped(Reg, NotMapped);
    }
    return Reg;
}

bool CArmRegInfo::UnMap_ArmReg(CArmOps::ArmReg Reg)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }
    if (GetArmRegProtected(Reg))
    {
        m_CodeBlock.Log("%s: %s is protected", __FUNCTION__, m_Assembler.ArmRegName(Reg));
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
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (GetArmRegMapped(Reg) == Temp_Mapped)
    {
        m_CodeBlock.Log("    regcache: unallocate %s from temporary storage", m_Assembler.ArmRegName(Reg));
        SetArmRegMapped(Reg, NotMapped);
        return true;
    }
    else if (GetArmRegMapped(Reg) == Variable_Mapped)
    {
        m_CodeBlock.Log("    regcache: unallocate %s from variable mapping (%s)", m_Assembler.ArmRegName(Reg), VariableMapName(GetVariableMappedTo(Reg)));
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
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    for (uint32_t i = 0, n = sizeof(m_ArmReg_Protected) / sizeof(m_ArmReg_Protected[0]); i < n; i++)
    {
        SetArmRegProtected((CArmOps::ArmReg)i, false);
    }
}

CArmOps::ArmReg CArmRegInfo::FreeArmReg(bool TempMapping)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R7) == NotMapped || GetArmRegMapped(CArmOps::Arm_R7) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R7))
    {
        return CArmOps::Arm_R7;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R6) == NotMapped || GetArmRegMapped(CArmOps::Arm_R6) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R6))
    {
        return CArmOps::Arm_R6;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R5) == NotMapped || GetArmRegMapped(CArmOps::Arm_R5) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R5))
    {
        return CArmOps::Arm_R5;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R4) == NotMapped || GetArmRegMapped(CArmOps::Arm_R4) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R4))
    {
        return CArmOps::Arm_R4;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R3) == NotMapped || GetArmRegMapped(CArmOps::Arm_R3) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R3))
    {
        return CArmOps::Arm_R3;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R2) == NotMapped || GetArmRegMapped(CArmOps::Arm_R2) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R2))
    {
        return CArmOps::Arm_R2;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R1) == NotMapped || GetArmRegMapped(CArmOps::Arm_R1) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R1))
    {
        return CArmOps::Arm_R1;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R0) == NotMapped || GetArmRegMapped(CArmOps::Arm_R0) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R0))
    {
        return CArmOps::Arm_R0;
    }
    if (TempMapping)
    {
        if ((GetArmRegMapped(CArmOps::Arm_R11) == NotMapped || GetArmRegMapped(CArmOps::Arm_R11) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R11))
        {
            return CArmOps::Arm_R11;
        }
        if ((GetArmRegMapped(CArmOps::Arm_R10) == NotMapped || GetArmRegMapped(CArmOps::Arm_R10) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R10))
        {
            return CArmOps::Arm_R10;
        }
    }
    if ((GetArmRegMapped(CArmOps::Arm_R9) == NotMapped || GetArmRegMapped(CArmOps::Arm_R9) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R9))
    {
        return CArmOps::Arm_R9;
    }
    if ((GetArmRegMapped(CArmOps::Arm_R8) == NotMapped || GetArmRegMapped(CArmOps::Arm_R8) == Temp_Mapped) && !GetArmRegProtected(CArmOps::Arm_R8))
    {
        return CArmOps::Arm_R8;
    }

    CArmOps::ArmReg Reg = UnMap_TempReg(TempMapping);
    if (Reg != CArmOps::Arm_Unknown)
    {
        return Reg;
    }

    int32_t MapCount[CArmOps::Arm_R12];
    CArmOps::ArmReg MapReg[CArmOps::Arm_R12];

    for (int32_t i = 0, n = TempMapping ? CArmOps::Arm_R12 : CArmOps::Arm_R10; i < n; i++)
    {
        MapCount[i] = GetArmRegMapOrder((CArmOps::ArmReg)i);
        MapReg[i] = (CArmOps::ArmReg)i;
    }
    for (int32_t i = 0, n = TempMapping ? CArmOps::Arm_R12 : CArmOps::Arm_R10; i < n; i++)
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
            CArmOps::ArmReg tempReg = MapReg[z];
            MapReg[z] = MapReg[z + 1];
            MapReg[z + 1] = tempReg;
            changed = true;
        }
        if (!changed)
        {
            break;
        }
    }

    for (int32_t i = 0, n = TempMapping ? CArmOps::Arm_R12 : CArmOps::Arm_R10; i < n; i++)
    {
        if (((MapCount[i] > 0 && GetArmRegMapped(MapReg[i]) == GPR_Mapped) || GetArmRegMapped(MapReg[i]) == Variable_Mapped) && !GetArmRegProtected((CArmOps::ArmReg)MapReg[i]))
        {
            if (UnMap_ArmReg((CArmOps::ArmReg)MapReg[i]))
            {
                return (CArmOps::ArmReg)MapReg[i];
            }
        }
    }

    LogRegisterState();
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return CArmOps::Arm_Unknown;
}

void CArmRegInfo::LogRegisterState(void)
{
    if (!CDebugSettings::bRecordRecompilerAsm())
    {
        return;
    }

    for (uint32_t i = 0; i < 16; i++)
    {
        stdstr regname;

        if (GetArmRegMapped((CArmOps::ArmReg)i) == CArmRegInfo::GPR_Mapped)
        {
            for (uint32_t count = 1; count < 32; count++)
            {
                if (!IsMapped(count))
                {
                    continue;
                }

                if (Is64Bit(count) && GetMipsRegMapHi(count) == (CArmOps::ArmReg)i)
                {
                    regname = CRegName::GPR_Hi[count];
                    break;
                }
                if (GetMipsRegMapLo(count) == (CArmOps::ArmReg)i)
                {
                    regname = CRegName::GPR_Lo[count];
                    break;
                }
            }
        }

        m_CodeBlock.Log("GetArmRegMapped(%s) = %X%s%s Protected: %s MapOrder: %d",
                        m_Assembler.ArmRegName((CArmOps::ArmReg)i),
                        GetArmRegMapped((CArmOps::ArmReg)i),
                        GetArmRegMapped((CArmOps::ArmReg)i) == CArmRegInfo::Variable_Mapped ? stdstr_f(" (%s)", CArmRegInfo::VariableMapName(GetVariableMappedTo((CArmOps::ArmReg)i))).c_str() : "",
                        regname.length() > 0 ? stdstr_f(" (%s)", regname.c_str()).c_str() : "",
                        GetArmRegProtected((CArmOps::ArmReg)i) ? "true" : "false",
                        GetArmRegMapOrder((CArmOps::ArmReg)i));
    }
}

CArmOps::ArmReg CArmRegInfo::Map_TempReg(CArmOps::ArmReg Reg, int32_t MipsReg, bool LoadHiWord)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }
    CArmOps::ArmReg GprReg = MipsReg >= 0 ? Map_Variable(VARIABLE_GPR) : CArmOps::Arm_Unknown;

    if (Reg == CArmOps::Arm_Any)
    {
        if (GetArmRegMapped(CArmOps::Arm_R7) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R7))
        {
            Reg = CArmOps::Arm_R7;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R6) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R6))
        {
            Reg = CArmOps::Arm_R6;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R5) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R5))
        {
            Reg = CArmOps::Arm_R5;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R4) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R4))
        {
            Reg = CArmOps::Arm_R4;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R3) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R3))
        {
            Reg = CArmOps::Arm_R3;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R2) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R2))
        {
            Reg = CArmOps::Arm_R2;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R1) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R1))
        {
            Reg = CArmOps::Arm_R1;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R0) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R0))
        {
            Reg = CArmOps::Arm_R0;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R11) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R11))
        {
            Reg = CArmOps::Arm_R11;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R10) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R10))
        {
            Reg = CArmOps::Arm_R10;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R9) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R9))
        {
            Reg = CArmOps::Arm_R9;
        }
        else if (GetArmRegMapped(CArmOps::Arm_R8) == Temp_Mapped && !GetArmRegProtected(CArmOps::Arm_R8))
        {
            Reg = CArmOps::Arm_R8;
        }

        if (Reg == CArmOps::Arm_Any)
        {
            Reg = FreeArmReg(true);
            if (Reg == CArmOps::Arm_Unknown)
            {
                WriteTrace(TraceRegisterCache, TraceError, "Failed to find a free register");
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return CArmOps::Arm_Unknown;
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
        m_CodeBlock.Log("    regcache: allocate %s as temporary storage", m_Assembler.ArmRegName(Reg));
    }
    else
    {
        m_CodeBlock.Log("    regcache: allocate %s as temporary storage (%s)", m_Assembler.ArmRegName(Reg), LoadHiWord ? CRegName::GPR_Hi[MipsReg] : CRegName::GPR_Lo[MipsReg]);
        if (GprReg == CArmOps::Arm_Unknown)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (LoadHiWord)
        {
            if (IsUnknown(MipsReg))
            {
                m_Assembler.LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(MipsReg << 3) + 4, CRegName::GPR_Hi[MipsReg]);
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
                    m_Assembler.ShiftRightSignImmed(Reg, GetMipsRegMapLo(MipsReg), 31);
                }
                else
                {
                    m_Assembler.MoveConstToArmReg(Reg, (uint32_t)0);
                }
            }
            else
            {
                if (Is64Bit(MipsReg))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    //m_Assembler.MoveConstToArmReg(Reg, GetMipsRegHi(MipsReg));
                }
                else
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    //m_Assembler.MoveConstToArmReg(Reg, GetMipsRegLo_S(MipsReg) >> 31);
                }
            }
        }
        else
        {
            if (IsUnknown(MipsReg))
            {
                m_Assembler.LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(MipsReg << 3), CRegName::GPR_Lo[MipsReg]);
            }
            else if (IsMapped(MipsReg))
            {
                m_Assembler.AddConstToArmReg(Reg, GetMipsRegMapLo(MipsReg), 0);
            }
            else
            {
                m_Assembler.MoveConstToArmReg(Reg, GetMipsRegLo(MipsReg));
            }
        }
    }
    SetArmRegMapped(Reg, Temp_Mapped);
    SetArmRegProtected(Reg, true);
    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        int32_t MapOrder = GetArmRegMapOrder((CArmOps::ArmReg)i);
        if (MapOrder > 0)
        {
            SetArmRegMapOrder((CArmOps::ArmReg)i, MapOrder + 1);
        }
    }
    SetArmRegMapOrder(Reg, 1);
    SetArmRegProtected(GprReg, false);
    return Reg;
}

CArmOps::ArmReg CArmRegInfo::Map_Variable(VARIABLE_MAPPED variable, CArmOps::ArmReg Reg)
{
    m_CodeBlock.Log("%s: variable: %s Reg: %d", __FUNCTION__, VariableMapName(variable), Reg);

    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }

    if (Reg == CArmOps::Arm_Unknown)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }

    if (variable == VARIABLE_GPR && Reg != CArmOps::Arm_Any && Reg != CArmOps::Arm_R12)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }

    if (Reg == CArmOps::Arm_Any)
    {
        Reg = GetVariableReg(variable);
        if (Reg != CArmOps::Arm_Unknown)
        {
            SetArmRegProtected(Reg, true);
            return Reg;
        }

        Reg = variable == VARIABLE_GPR ? CArmOps::Arm_R12 : FreeArmReg(false);
        if (Reg == CArmOps::Arm_Unknown)
        {
            WriteTrace(TraceRegisterCache, TraceError, "Failed to find a free register");
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return CArmOps::Arm_Unknown;
        }
    }
    else if (GetArmRegMapped(Reg) == Variable_Mapped && m_Variable_MappedTo[Reg] == variable)
    {
        return Reg;
    }
    else if (GetArmRegMapped(Reg) != NotMapped)
    {
        UnMap_ArmReg(Reg);
    }

    SetArmRegMapped(Reg, Variable_Mapped);
    SetArmRegProtected(Reg, true);

    m_CodeBlock.Log("    regcache: allocate %s as pointer to %s", m_Assembler.ArmRegName(Reg), VariableMapName(variable));
    m_Variable_MappedTo[Reg] = variable;
    if (variable == VARIABLE_GPR)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)_GPR, "_GPR");
    }
    else if (variable == VARIABLE_FPR)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)_FPR_S, "_FPR_S");
    }
    else if (variable == VARIABLE_TLB_READMAP)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)(g_MMU->m_TLB_ReadMap), "MMU->TLB_ReadMap");
    }
    else if (variable == VARIABLE_TLB_WRITEMAP)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)(g_MMU->m_TLB_WriteMap), "MMU->m_TLB_WriteMap");
    }
    else if (variable == VARIABLE_TLB_LOAD_ADDRESS)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)(g_TLBLoadAddress), "g_TLBLoadAddress");
    }
    else if (variable == VARIABLE_TLB_STORE_ADDRESS)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)(g_TLBStoreAddress), "g_TLBStoreAddress");
    }
    else if (variable == VARIABLE_NEXT_TIMER)
    {
        m_Assembler.MoveConstToArmReg(Reg, (uint32_t)(g_NextTimer), "g_NextTimer");
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return CArmOps::Arm_Unknown;
    }
    return Reg;
}

CArmOps::ArmReg CArmRegInfo::GetVariableReg(VARIABLE_MAPPED variable) const
{
    for (int32_t i = 0, n = sizeof(m_ArmReg_MappedTo) / sizeof(m_ArmReg_MappedTo[0]); i < n; i++)
    {
        if (m_ArmReg_MappedTo[i] == Variable_Mapped && m_Variable_MappedTo[i] == variable)
        {
            return (CArmOps::ArmReg)i;
        }
    }
    return CArmOps::Arm_Unknown;
}

void CArmRegInfo::ProtectGPR(uint32_t Reg)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
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

void CArmRegInfo::UnProtectGPR(uint32_t Reg)
{
    if (m_InCallDirect)
    {
        m_CodeBlock.Log("%s: in CallDirect", __FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (IsUnknown(Reg) || IsConst(Reg))
    {
        return;
    }
    if (Is64Bit(Reg))
    {
        SetArmRegProtected(GetMipsRegMapHi(Reg), false);
    }
    SetArmRegProtected(GetMipsRegMapLo(Reg), false);
}

const char * CArmRegInfo::VariableMapName(VARIABLE_MAPPED variable)
{
    switch (variable)
    {
    case VARIABLE_UNKNOWN: return "UNKNOWN";
    case VARIABLE_GPR: return "_GPR";
    case VARIABLE_FPR: return "_FPR_S";
    case VARIABLE_TLB_READMAP: return "m_TLB_ReadMap";
    case VARIABLE_TLB_WRITEMAP: return "m_TLB_WriteMap";
    case VARIABLE_TLB_LOAD_ADDRESS: return "g_TLBLoadAddress";
    case VARIABLE_TLB_STORE_ADDRESS: return "g_TLBStoreAddress";
    case VARIABLE_NEXT_TIMER: return "g_NextTimer";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return "unknown VariableMapName";
    }
}

#endif