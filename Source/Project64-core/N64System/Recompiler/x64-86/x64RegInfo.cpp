#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/x64-86/x64RegInfo.h>

namespace
{
static constexpr uint32_t kX64AllocatableRegIds[] = {
    asmjit::x86::Gp::kIdBx,
    asmjit::x86::Gp::kIdR12,
    asmjit::x86::Gp::kIdR13,
    asmjit::x86::Gp::kIdR14,
    asmjit::x86::Gp::kIdR15,
    asmjit::x86::Gp::kIdDi,
    asmjit::x86::Gp::kIdSi,
    asmjit::x86::Gp::kIdBp,
    asmjit::x86::Gp::kIdAx,
    asmjit::x86::Gp::kIdCx,
    asmjit::x86::Gp::kIdDx,
    asmjit::x86::Gp::kIdR8,
    asmjit::x86::Gp::kIdR9,
    asmjit::x86::Gp::kIdR10,
    asmjit::x86::Gp::kIdR11,
};
static constexpr uint32_t kX64AllocatableRegCount = sizeof(kX64AllocatableRegIds) / sizeof(kX64AllocatableRegIds[0]);
asmjit::x86::Gp GetX64RegFromPhysId(uint32_t PhysId, asmjit::RegType RegType = asmjit::RegType::kX86_Gpq)
{
    using namespace asmjit::x86;

    if (PhysId > Gp::kIdR15)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return asmjit::x86::Gp();
    }

    if (RegType == asmjit::RegType::kX86_Gpd)
    {
        return Gpd(PhysId);
    }
    if (RegType == asmjit::RegType::kX86_Gpq)
    {
        return Gpq(PhysId);
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return asmjit::x86::Gp();
}

static const char * X64GpName(const asmjit::x86::Gp & Reg)
{
    asmjit::RegType width = asmjit::RegType::kX86_Gpq;
    if (Reg.isType(asmjit::RegType::kX86_Gpd))
    {
        width = asmjit::RegType::kX86_Gpd;
    }
    else if (!Reg.isType(asmjit::RegType::kX86_Gpq))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    static const char * const kName64[] = {
        "rax",
        "rcx",
        "rdx",
        "rbx",
        "rsp",
        "rbp",
        "rsi",
        "rdi",
        "r8",
        "r9",
        "r10",
        "r11",
        "r12",
        "r13",
        "r14",
        "r15",
    };
    static const char * const kName32[] = {
        "eax",
        "ecx",
        "edx",
        "ebx",
        "esp",
        "ebp",
        "esi",
        "edi",
        "r8d",
        "r9d",
        "r10d",
        "r11d",
        "r12d",
        "r13d",
        "r14d",
        "r15d",
    };

    const uint32_t id = Reg.id();
    if (id <= asmjit::x86::Gp::kIdR15)
    {
        return width == asmjit::RegType::kX86_Gpd ? kName32[id] : kName64[id];
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return "?";
}
} // namespace

CX64RegInfo::CX64RegInfo(CCodeBlock & CodeBlock, CX64Ops & Assembler) :
    m_Reg(CodeBlock.Registers()),
    m_CodeBlock(CodeBlock),
    m_Assembler(Assembler),
    m_InBeforeCallDirect(false),
    m_CallDirectPushBase(0),
    m_CallDirectPushCount(0)
{
    for (int32_t i = 0; i < 32; i++)
    {
        m_RegMap[i] = asmjit::x86::Gp();
    }
    for (int32_t i = 0; i < x64PhysRegCount; i++)
    {
        m_x64reg_MappedTo[i] = NotMapped;
        m_x64reg_Protected[i] = false;
        m_x64reg_MapOrder[i] = 0;
    }
    memset(&m_CallDirectPushIds, 0, sizeof(m_CallDirectPushIds));
}

CX64RegInfo::CX64RegInfo(const CX64RegInfo & rhs) :
    m_Reg(rhs.m_Reg),
    m_CodeBlock(rhs.m_CodeBlock),
    m_Assembler(rhs.m_Assembler)
{
    *this = rhs;
}

CX64RegInfo::~CX64RegInfo()
{
}

CX64RegInfo & CX64RegInfo::operator=(const CX64RegInfo & right)
{
    CRegBase::operator=(right);

    memcpy(&m_RegMap, &right.m_RegMap, sizeof(m_RegMap));
    memcpy(&m_x64reg_MappedTo, &right.m_x64reg_MappedTo, sizeof(m_x64reg_MappedTo));
    memcpy(&m_x64reg_Protected, &right.m_x64reg_Protected, sizeof(m_x64reg_Protected));
    memcpy(&m_x64reg_MapOrder, &right.m_x64reg_MapOrder, sizeof(m_x64reg_MapOrder));
    m_InBeforeCallDirect = right.m_InBeforeCallDirect;
    m_CallDirectPushBase = right.m_CallDirectPushBase;
    m_CallDirectPushCount = right.m_CallDirectPushCount;
    memcpy(&m_CallDirectPushIds, &right.m_CallDirectPushIds, sizeof(m_CallDirectPushIds));

#ifdef _DEBUG
    if (*this != right)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    return *this;
}

bool CX64RegInfo::operator==(const CX64RegInfo & Right) const
{
    if (!CRegBase::operator==(Right))
    {
        return false;
    }

    for (int32_t i = 0; i < x64PhysRegCount; i++)
    {
        if (m_x64reg_MappedTo[i] != Right.m_x64reg_MappedTo[i])
        {
            return false;
        }
        if (m_x64reg_Protected[i] != Right.m_x64reg_Protected[i])
        {
            return false;
        }
        if (m_x64reg_MapOrder[i] != Right.m_x64reg_MapOrder[i])
        {
            return false;
        }
    }
    return true;
}

bool CX64RegInfo::operator!=(const CX64RegInfo & Right) const
{
    return !(Right == *this);
}

const asmjit::x86::Gp & CX64RegInfo::GetMipsRegMap(int32_t Reg) const
{
    return m_RegMap[Reg];
}

void CX64RegInfo::SetMipsRegMap(int32_t MipsReg, const asmjit::x86::Gp & Reg)
{
    m_RegMap[MipsReg] = Reg;
}

bool CX64RegInfo::GetX64Protected(uint32_t PhysId) const
{
    return m_x64reg_Protected[PhysId];
}

CX64RegInfo::REG_MAPPED CX64RegInfo::GetX64Mapped(uint32_t PhysId) const
{
    return m_x64reg_MappedTo[PhysId];
}

void CX64RegInfo::SetX64MapOrder(uint32_t PhysId, uint32_t Order)
{
    m_x64reg_MapOrder[PhysId] = Order;
}

void CX64RegInfo::SetX64Protected(uint32_t PhysId, bool Protected)
{
    m_x64reg_Protected[PhysId] = Protected;
}

void CX64RegInfo::SetX64Mapped(uint32_t PhysId, REG_MAPPED Mapping)
{
    m_x64reg_MappedTo[PhysId] = Mapping;
}

void CX64RegInfo::ResetRegisterProtection()
{
    for (uint32_t k = 0; k < x64PhysRegCount; k++)
    {
        m_x64reg_Protected[k] = false;
    }
}

void CX64RegInfo::BeforeCallDirect(void)
{
    // Win64 integer argument / return registers (caller-saved).
    static constexpr uint32_t kX64CallClobberRegIds[] = {
        asmjit::x86::Gp::kIdAx,
        asmjit::x86::Gp::kIdCx,
        asmjit::x86::Gp::kIdDx,
        asmjit::x86::Gp::kIdR8,
        asmjit::x86::Gp::kIdR9,
        asmjit::x86::Gp::kIdR10,
        asmjit::x86::Gp::kIdR11,
    };
    static constexpr uint32_t kX64CallClobberRegCount = sizeof(kX64CallClobberRegIds) / sizeof(kX64CallClobberRegIds[0]);

    if (m_InBeforeCallDirect)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_InBeforeCallDirect = true;
    m_CallDirectPushBase = m_CallDirectPushCount;

    for (uint32_t k = 0; k < kX64CallClobberRegCount; k++)
    {
        const uint32_t physId = kX64CallClobberRegIds[k];
        if (GetX64Mapped(physId) == NotMapped)
        {
            continue;
        }
        if (m_CallDirectPushCount >= sizeof(m_CallDirectPushIds) / sizeof(m_CallDirectPushIds[0]))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        const asmjit::x86::Gp Reg = GetX64RegFromPhysId(physId, asmjit::RegType::kX86_Gpq);
        m_CodeBlock.Log("    regcache: push %s for call", X64GpName(Reg));
        m_Assembler.push(Reg);
        m_CallDirectPushIds[m_CallDirectPushCount++] = physId;
    }
}

void CX64RegInfo::AfterCallDirect(void)
{
    if (!m_InBeforeCallDirect)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    while (m_CallDirectPushCount > m_CallDirectPushBase)
    {
        const uint32_t physId = m_CallDirectPushIds[--m_CallDirectPushCount];
        const asmjit::x86::Gp Reg = GetX64RegFromPhysId(physId, asmjit::RegType::kX86_Gpq);
        m_CodeBlock.Log("    regcache: pop %s after call", X64GpName(Reg));
        m_Assembler.pop(Reg);
    }

    m_InBeforeCallDirect = m_CallDirectPushCount != 0;
}

bool CX64RegInfo::UnMap_X64reg(const asmjit::x86::Gp & Reg)
{
    const uint32_t RegIndex = Reg.id();
    if (GetX64Mapped(RegIndex) == NotMapped)
    {
        if (!GetX64Protected(RegIndex))
        {
            return true;
        }
    }
    else if (GetX64Mapped(RegIndex) == CX64RegInfo::GPR_Mapped)
    {
        for (int i = 1; i < 32; i++)
        {
            if (!IsMapped(i))
            {
                continue;
            }

            if (GetMipsRegMap(i) == Reg)
            {
                if (!GetX64Protected(RegIndex))
                {
                    UnMap_GPR(i, true);
                    return true;
                }
                break;
            }
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

void CX64RegInfo::ProtectGPR(uint32_t MipsReg)
{
    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        return;
    }
    SetX64Protected(GetMipsRegMap(MipsReg).id(), true);
}

void CX64RegInfo::Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad)
{
    if (MipsReg == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    asmjit::x86::Gp Reg;
    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        Reg = FreeX64Reg(asmjit::RegType::kX86_Gpd);
        if (Reg.isNone())
        {
            if (g_DebugSettings.haveDebugger)
            {
                g_Notify->DisplayError("Map_GPR_32bit\n\nOut of registers");
            }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        m_CodeBlock.Log("    regcache: allocate %s to %s", X64GpName(Reg), CRegName::GPR[MipsReg]);
    }
    else
    {
        if (Is64Bit(MipsReg))
        {
            m_CodeBlock.Log("    regcache: narrow %s to 32-bit %s", X64GpName(GetMipsRegMap(MipsReg)), CRegName::GPR[MipsReg]);
        }
        Reg = GetMipsRegMap(MipsReg).r32();
    }
    for (int i = 0; i < x64PhysRegCount; i++)
    {
        if (m_x64reg_MapOrder[i] > 0)
        {
            m_x64reg_MapOrder[i] += 1;
        }
    }
    const uint32_t RegIndex = Reg.id();
    SetX64MapOrder(RegIndex, 1);

    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            m_Assembler.MoveVariable32ToX64reg(Reg, &m_Reg.m_GPR[MipsRegToLoad].UW[0], CRegName::GPR_Lo[MipsRegToLoad]);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (MipsReg != MipsRegToLoad)
            {
                m_Assembler.mov(Reg.r32(), GetMipsRegMap(MipsRegToLoad).r32());
            }
        }
        else
        {
            m_Assembler.mov(Reg.r32(), (uint32_t)GetMipsRegLo(MipsRegToLoad));
        }
    }
    else if (MipsRegToLoad == 0)
    {
        m_Assembler.xor_(Reg.r32(), Reg.r32());
    }
    SetX64Mapped(RegIndex, GPR_Mapped);
    SetX64Protected(RegIndex, true);
    SetMipsRegMap(MipsReg, Reg);
    SetMipsRegState(MipsReg, SignValue ? STATE_MAPPED_32_SIGN : STATE_MAPPED_32_ZERO);
}

void CX64RegInfo::WriteBackRegisters()
{
    for (uint32_t k = 0; k < kX64AllocatableRegCount; k++)
    {
        SetX64Protected(kX64AllocatableRegIds[k], false);
    }
    for (uint32_t k = 0; k < kX64AllocatableRegCount; k++)
    {
        UnMap_X64reg(GetX64RegFromPhysId(kX64AllocatableRegIds[k]));
    }

    bool bEdiZero = false;
    bool bEsiSign = false;

    for (int32_t count = 1; count < 32; count++)
    {
        switch (GetMipsRegState(count))
        {
        case CRegBase::STATE_UNKNOWN:
            break;
        case CRegBase::STATE_CONST_32_SIGN:
            if (!g_GameSettings.core32Bit)
            {
                if (!bEdiZero && (!GetMipsRegLo(count) || !(GetMipsRegLo(count) & 0x80000000)))
                {
                    m_Assembler.xor_(asmjit::x86::edi, asmjit::x86::edi);
                    bEdiZero = true;
                }
                if (!bEsiSign && (GetMipsRegLo(count) & 0x80000000))
                {
                    m_Assembler.mov(asmjit::x86::esi, 0xFFFFFFFFu);
                    bEsiSign = true;
                }
                if ((GetMipsRegLo(count) & 0x80000000) != 0)
                {
                    m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[count].UW[1], CRegName::GPR_Hi[count], asmjit::x86::esi);
                }
                else
                {
                    m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[count].UW[1], CRegName::GPR_Hi[count], asmjit::x86::edi);
                }
            }

            if (GetMipsRegLo(count) == 0)
            {
                if (g_GameSettings.core32Bit)
                {
                    if (!bEdiZero)
                    {
                        m_Assembler.xor_(asmjit::x86::edi, asmjit::x86::edi);
                        bEdiZero = true;
                    }
                }
                m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[count].UW[0], CRegName::GPR_Lo[count], asmjit::x86::edi);
            }
            else if (GetMipsRegLo(count) == 0xFFFFFFFF)
            {
                if (g_GameSettings.core32Bit)
                {
                    if (!bEsiSign)
                    {
                        m_Assembler.mov(asmjit::x86::esi, 0xFFFFFFFFu);
                        bEsiSign = true;
                    }
                }
                m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[count].UW[0], CRegName::GPR_Lo[count], asmjit::x86::esi);
            }
            else
            {
                m_Assembler.MoveConstToVariable(&m_Reg.m_GPR[count].UW[0], CRegName::GPR_Lo[count], GetMipsRegLo(count));
            }

            SetMipsRegState(count, CRegBase::STATE_UNKNOWN);
            break;
        case CRegBase::STATE_MAPPED_32_SIGN:
        case CRegBase::STATE_MAPPED_32_ZERO:
            if (!GetMipsRegMap(count).isValid())
            {
                SetMipsRegState(count, CRegBase::STATE_UNKNOWN);
                break;
            }
            UnMap_GPR(count, true);
            break;
        default:
            m_CodeBlock.Log("%s: Unknown State: %d reg %d (%s)", __FUNCTION__, GetMipsRegState(count), count, CRegName::GPR[count]);
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX64RegInfo::UnMap_GPR(uint32_t Reg, bool WriteBackValue)
{
    if (Reg == 0)
    {
        if (g_DebugSettings.haveDebugger)
        {
            g_Notify->DisplayError(stdstr_f("%s\n\nWhy are you trying to unmap register 0?", __FUNCTION__).c_str());
        }
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (IsUnknown(Reg))
    {
        return;
    }

    if (IsConst(Reg))
    {
        if (!WriteBackValue)
        {
            SetMipsRegState(Reg, STATE_UNKNOWN);
            return;
        }
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    m_CodeBlock.Log("    regcache: unallocate %s from %s", X64GpName(GetMipsRegMap(Reg)), Is64Bit(Reg) ? CRegName::GPR[Reg] : CRegName::GPR_Lo[Reg]);
    const uint32_t RegIndex = GetMipsRegMap(Reg).id();
    SetX64Mapped(RegIndex, NotMapped);
    SetX64Protected(RegIndex, false);
    if (WriteBackValue)
    {
        if (Is64Bit(Reg))
        {
            m_Assembler.MovQwordToVariable(&m_Reg.m_GPR[Reg].UDW, CRegName::GPR[Reg], GetMipsRegMap(Reg));
        }
        else
        {
            m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[Reg].UW[0], CRegName::GPR_Lo[Reg], GetMipsRegMap(Reg));
            if (!g_GameSettings.core32Bit)
            {
                if (IsSigned(Reg))
                {
                    m_Assembler.sar(GetMipsRegMap(Reg).r32(), 31);
                    m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg], GetMipsRegMap(Reg));
                }
                else
                {
                    m_Assembler.MoveConstToVariable(&m_Reg.m_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg], 0u);
                }
            }
        }
    }
    SetMipsRegState(Reg, STATE_UNKNOWN);
    SetMipsRegMap(Reg, asmjit::x86::Gp());
}

asmjit::x86::Gp CX64RegInfo::FreeX64Reg(asmjit::RegType RegType)
{
    for (uint32_t k = 0; k < kX64AllocatableRegCount; k++)
    {
        const uint32_t physId = kX64AllocatableRegIds[k];
        if (GetX64Mapped(physId) == NotMapped && !GetX64Protected(physId))
        {
            if (RegType == asmjit::RegType::kX86_Gpd)
            {
                return asmjit::x86::Gpq(kX64AllocatableRegIds[k]);
            }
            return GetX64RegFromPhysId(physId, RegType);
        }
    }
    return asmjit::x86::Gp();
}

#endif