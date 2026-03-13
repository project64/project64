#if defined(__amd64__) || defined(_M_X64)

#include "RspRegState.h"
#include "Recompiler/RspAssembler.h"
#include "Recompiler/RspRecompilerOps-x64.h"
#include <Common/StdString.h>
#include <Settings/Settings.h>

static const char * AccumLocName(AccumLocation loc)
{
    switch (loc)
    {
    case AccumLocation::Low: return "ACCL";
    case AccumLocation::Middle: return "ACCM";
    case AccumLocation::High: return "ACCH";
    }
    return "ACC?";
}

CRspRegState::CRspRegState(CRSPRecompilerOps & RecompilerOps) :
    m_RecompilerOps(RecompilerOps),
    m_Assembler(RecompilerOps.m_Assembler)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        m_GprIsConst[i] = false;
        m_GprConstValue[i] = 0;
    }
    m_GprIsConst[0] = true;
    m_GprConstValue[0] = 0;

    for (uint32_t i = 0; i < 16; i++)
    {
        m_XmmState[i] = XmmState::Free;
    }
    for (uint32_t i = 0; i < 16; i++)
    {
        m_XmmRegMapped[i] = (uint8_t)~0;
        m_XmmProtected[i] = false;
    }
    for (uint32_t i = 0; i < (uint32_t)RspFlags::MaxFlags; i++)
    {
        m_FlagIsZero[i] = false;
    }
}

CRspRegState::~CRspRegState()
{
}

void CRspRegState::ResetRegProtection()
{
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        m_XmmProtected[i] = false;
    }
}

asmjit::x86::Xmm CRspRegState::MapXmmZero()
{
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmState[i] == XmmState::Zero && !m_XmmProtected[i])
        {
            m_XmmProtected[i] = true;
            return asmjit::x86::Xmm(i);
        }
    }

    XmmState searchOrder[] = {XmmState::Free, XmmState::Temp};
    for (XmmState state : searchOrder)
    {
        for (uint8_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
        {
            if (m_XmmState[i] != state || m_XmmProtected[i])
            {
                continue;
            }
            m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d as zero", i).c_str());
            m_XmmState[i] = XmmState::Zero;
            m_XmmProtected[i] = true;
            m_Assembler->pxor(asmjit::x86::Xmm(i), asmjit::x86::Xmm(i));
            return asmjit::x86::Xmm(i);
        }
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return asmjit::x86::Xmm();
}

asmjit::x86::Xmm CRspRegState::MapXmmAccum(AccumLocation location, bool loadSource)
{
    uint8_t accumIndex = (uint8_t)location;

    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmState[i] == XmmState::AccumMapped && m_XmmRegMapped[i] == accumIndex)
        {
            m_XmmProtected[i] = true;
            return asmjit::x86::Xmm(i);
        }
    }

    XmmState searchOrder[] = {XmmState::Free, XmmState::Temp, XmmState::Zero};
    for (XmmState state : searchOrder)
    {
        for (uint8_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
        {
            if (m_XmmState[i] != state || m_XmmProtected[i])
            {
                continue;
            }
            if (state == XmmState::Zero || state == XmmState::Temp)
            {
                // These don't need writeback, just reclaim
            }
            m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d to %s", i, AccumLocName(location)).c_str());
            m_XmmState[i] = XmmState::AccumMapped;
            m_XmmRegMapped[i] = accumIndex;
            m_XmmProtected[i] = true;
            if (loadSource)
            {
                m_Assembler->movdqa(asmjit::x86::Xmm(i), asmjit::x86::xmmword_ptr(asmjit::x86::r14, m_RecompilerOps.AccumOffset(location)));
            }
            return asmjit::x86::Xmm(i);
        }
    }

    // Last resort: evict a mapped register
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmProtected[i])
        {
            continue;
        }
        if (!FreeXmmReg(i))
        {
            continue;
        }
        m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d to %s", i, AccumLocName(location)).c_str());
        m_XmmState[i] = XmmState::AccumMapped;
        m_XmmRegMapped[i] = accumIndex;
        m_XmmProtected[i] = true;
        if (loadSource)
        {
            m_Assembler->movdqa(asmjit::x86::Xmm(i), asmjit::x86::xmmword_ptr(asmjit::x86::r14, m_RecompilerOps.AccumOffset(location)));
        }
        return asmjit::x86::Xmm(i);
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return asmjit::x86::Xmm();
}

asmjit::x86::Xmm CRspRegState::MapXmmReg(uint8_t vreg, uint8_t source, bool loadSource)
{
    asmjit::x86::Xmm srcReg = VRegMapping(source);
    if (loadSource && srcReg.isValid())
    {
        if (vreg == source)
        {
            return srcReg;
        }
        ProtectXmm(srcReg);
    }
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmState[i] == XmmState::Mapped && m_XmmRegMapped[i] == vreg)
        {
            if (loadSource && source != vreg)
            {
                srcReg = VRegMapping(source);
                if (srcReg.isValid())
                {
                    m_Assembler->movdqa(asmjit::x86::Xmm(i), srcReg);
                }
                else
                {
                    m_Assembler->movdqa(asmjit::x86::Xmm(i), asmjit::x86::ptr(asmjit::x86::r14, m_RecompilerOps.VectorOffset(source)));
                }
            }
            return asmjit::x86::Xmm(i);
        }
    }

    asmjit::x86::Xmm reg;
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmState[i] != XmmState::Free)
        {
            continue;
        }
        m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d to V%d", i, vreg).c_str());
        m_XmmState[i] = XmmState::Mapped;
        m_XmmRegMapped[i] = vreg;
        m_XmmProtected[i] = true;
        reg = asmjit::x86::Xmm(i);
        break;
    }

    if (!reg.isValid())
    {
        for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
        {
            if (m_XmmState[i] != XmmState::Temp || m_XmmProtected[i])
            {
                continue;
            }
            m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d to V%d", i, vreg).c_str());
            m_XmmState[i] = XmmState::Mapped;
            m_XmmRegMapped[i] = vreg;
            m_XmmProtected[i] = true;
            reg = asmjit::x86::Xmm(i);
            break;
        }
    }

    if (!reg.isValid())
    {
        for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
        {
            if (m_XmmState[i] != XmmState::Zero || m_XmmProtected[i])
            {
                continue;
            }
            m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d to V%d", i, vreg).c_str());
            m_XmmState[i] = XmmState::Mapped;
            m_XmmRegMapped[i] = vreg;
            m_XmmProtected[i] = true;
            reg = asmjit::x86::Xmm(i);
            break;
        }
    }

    if (!reg.isValid())
    {
        for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
        {
            if (m_XmmProtected[i])
            {
                continue;
            }
            if (!FreeXmmReg(i))
            {
                continue;
            }
            m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d to V%d", i, vreg).c_str());
            m_XmmState[i] = XmmState::Mapped;
            m_XmmRegMapped[i] = vreg;
            m_XmmProtected[i] = true;
            reg = asmjit::x86::Xmm(i);
            break;
        }
    }
    if (loadSource && reg.isValid())
    {
        if (srcReg.isValid())
        {
            m_Assembler->movdqa(reg, srcReg);
        }
        else
        {
            m_Assembler->movdqa(reg, asmjit::x86::ptr(asmjit::x86::r14, m_RecompilerOps.VectorOffset(source)));
        }
    }
    if (reg.isValid())
    {
        return reg;
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return asmjit::x86::Xmm();
}

asmjit::x86::Xmm CRspRegState::VRegMapping(uint8_t vreg)
{
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmState[i] == XmmState::Mapped && m_XmmRegMapped[i] == vreg)
        {
            return asmjit::x86::Xmm(i);
        }
    }
    return asmjit::x86::Xmm();
}

void CRspRegState::ProtectXmm(asmjit::x86::Xmm xmm)
{
    if (!xmm.isValid())
    {
        return;
    }

    int id = xmm.id();
    if (id >= 0 && id < 16)
    {
        m_XmmProtected[id] = true;
    }
}

void CRspRegState::UnprotectXmm(asmjit::x86::Xmm xmm)
{
    if (!xmm.isValid())
    {
        return;
    }

    int id = xmm.id();
    if (id >= 0 && id < 16)
    {
        m_XmmProtected[id] = false;
    }
}

asmjit::x86::Xmm CRspRegState::MapXmmTemp(bool loadReg, uint8_t vreg, uint8_t e)
{
    XmmState searchOrder[] = {XmmState::Temp, XmmState::Free, XmmState::Zero, XmmState::Mapped};
    for (XmmState state : searchOrder)
    {
        for (uint8_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
        {
            if (m_XmmState[i] != state || m_XmmProtected[i])
            {
                continue;
            }
            return MapSpecificXmmTemp(i, loadReg, vreg, e);
        }
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return asmjit::x86::Xmm();
}

asmjit::x86::Xmm CRspRegState::MapSpecificXmmTemp(uint8_t xmmIndex, bool loadReg, uint8_t vreg, uint8_t e)
{
    if (xmmIndex >= 16 || m_XmmProtected[xmmIndex])
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return asmjit::x86::Xmm();
    }

    if (m_XmmState[xmmIndex] != XmmState::Temp && m_XmmState[xmmIndex] != XmmState::Free)
    {
        if (!FreeXmmReg(xmmIndex))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return asmjit::x86::Xmm();
        }
    }
    if (m_XmmState[xmmIndex] != XmmState::Temp)
    {
        m_Assembler->comment(stdstr_f(" regcache: allocate xmm%d as temp register", xmmIndex).c_str());
    }
    m_XmmState[xmmIndex] = XmmState::Temp;
    m_XmmProtected[xmmIndex] = true;
    asmjit::x86::Xmm tempReg = asmjit::x86::Xmm(xmmIndex);
    if (loadReg && tempReg.isValid())
    {
        asmjit::x86::Xmm srcReg = VRegMapping(vreg);
        if (srcReg.isValid())
        {
            m_Assembler->movdqa(tempReg, srcReg);
            if (e >= 8)
            {
                uint8_t element = 7 - (e - 8);
                if (element != 0)
                {
                    m_Assembler->psrldq(tempReg, element * 2); // Shift element to position 0
                }
                m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(0, 0, 0, 0));
                m_Assembler->pshufd(tempReg, tempReg, _MM_SHUFFLE(0, 0, 0, 0));
            }
            else if (e > 1)
            {
                // Quarter/half modes
                switch (e)
                {
                case 2: // 0q
                    m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(3, 3, 1, 1));
                    m_Assembler->pshufhw(tempReg, tempReg, _MM_SHUFFLE(3, 3, 1, 1));
                    break;
                case 3: // 1q
                    m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(2, 2, 0, 0));
                    m_Assembler->pshufhw(tempReg, tempReg, _MM_SHUFFLE(2, 2, 0, 0));
                    break;
                case 4: // 0h
                    m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(3, 3, 3, 3));
                    m_Assembler->pshufhw(tempReg, tempReg, _MM_SHUFFLE(3, 3, 3, 3));
                    break;
                case 5: // 1h
                    m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(2, 2, 2, 2));
                    m_Assembler->pshufhw(tempReg, tempReg, _MM_SHUFFLE(2, 2, 2, 2));
                    break;
                case 6: // 2h
                    m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(1, 1, 1, 1));
                    m_Assembler->pshufhw(tempReg, tempReg, _MM_SHUFFLE(1, 1, 1, 1));
                    break;
                case 7: // 3h
                    m_Assembler->pshuflw(tempReg, tempReg, _MM_SHUFFLE(0, 0, 0, 0));
                    m_Assembler->pshufhw(tempReg, tempReg, _MM_SHUFFLE(0, 0, 0, 0));
                    break;
                }
            }
        }
        else
        {
            m_RecompilerOps.LoadVectorRegister(tempReg, vreg, e);
        }
    }
    return tempReg;
}

bool CRspRegState::IsGprConst(uint8_t gprReg) const
{
    return m_GprIsConst[gprReg];
}

uint32_t CRspRegState::GetGprConstValue(uint8_t gprReg) const
{
    return m_GprConstValue[gprReg];
}

void CRspRegState::SetGprConst(uint8_t gprReg, uint32_t value)
{
    if (gprReg == 0)
    {
        return;
    }
    m_GprIsConst[gprReg] = true;
    m_GprConstValue[gprReg] = value;
}

void CRspRegState::SetGprUnknown(uint8_t gprReg)
{
    if (gprReg == 0)
    {
        return;
    }
    m_GprIsConst[gprReg] = false;
    m_GprConstValue[gprReg] = 0;
}

bool CRspRegState::IsFlagZero(RspFlags flag) const
{
    return m_FlagIsZero[(size_t)flag];
}

void CRspRegState::SetFlagZero(RspFlags flag)
{
    m_FlagIsZero[(size_t)flag] = true;
}

void CRspRegState::SetFlagUnknown(RspFlags flag)
{
    m_FlagIsZero[(size_t)flag] = false;
}

bool CRspRegState::FreeXmmReg(uint32_t xmmIndex)
{
    if (xmmIndex >= sizeof(m_XmmState) / sizeof(m_XmmState[0]))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }
    if (m_XmmProtected[xmmIndex])
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }
    if (m_XmmState[xmmIndex] == XmmState::Zero)
    {
        m_Assembler->comment(stdstr_f(" regcache: deallocate xmm%d as zero", xmmIndex).c_str());
        m_XmmState[xmmIndex] = XmmState::Free;
        return true;
    }

    if (m_XmmState[xmmIndex] == XmmState::Temp)
    {
        m_Assembler->comment(stdstr_f(" regcache: deallocate xmm%d as temp register", xmmIndex).c_str());
        m_XmmState[xmmIndex] = XmmState::Free;
        return true;
    }

    if (m_XmmState[xmmIndex] == XmmState::Mapped)
    {
        m_Assembler->comment(stdstr_f(" regcache: deallocate xmm%d from V%d", xmmIndex, m_XmmRegMapped[xmmIndex]).c_str());
        m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, m_RecompilerOps.VectorOffset(m_XmmRegMapped[xmmIndex])), asmjit::x86::Xmm(xmmIndex));
        m_XmmState[xmmIndex] = XmmState::Free;
        m_XmmRegMapped[xmmIndex] = (uint8_t)~0;
        return true;
    }

    if (m_XmmState[xmmIndex] == XmmState::AccumMapped)
    {
        AccumLocation loc = (AccumLocation)m_XmmRegMapped[xmmIndex];
        m_Assembler->comment(stdstr_f(" regcache: deallocate xmm%d from %s", xmmIndex, AccumLocName(loc)).c_str());
        m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, m_RecompilerOps.AccumOffset(loc)), asmjit::x86::Xmm(xmmIndex));
        m_XmmState[xmmIndex] = XmmState::Free;
        m_XmmRegMapped[xmmIndex] = (uint8_t)~0;
        return true;
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CRspRegState::HasMappedRegisters() const
{
    for (uint32_t i = 0; i < sizeof(m_XmmState) / sizeof(m_XmmState[0]); i++)
    {
        if (m_XmmState[i] == XmmState::Mapped)
        {
            return true;
        }
    }
    return false;
}

void CRspRegState::WriteBackRegisters()
{
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        if (m_XmmState[i] == XmmState::Free)
        {
            continue;
        }
        FreeXmmReg(i);
    }
    Reset();
}

void CRspRegState::Reset()
{
    for (uint32_t i = 0; i < 32; i++)
    {
        m_GprIsConst[i] = false;
        m_GprConstValue[i] = 0;
    }
    m_GprIsConst[0] = true;
    m_GprConstValue[0] = 0;

    for (uint32_t i = 0; i < (uint32_t)RspFlags::MaxFlags; i++)
    {
        m_FlagIsZero[i] = false;
    }
    for (uint32_t i = 0, n = sizeof(m_XmmState) / sizeof(m_XmmState[0]); i < n; i++)
    {
        m_XmmState[i] = XmmState::Free;
    }
}

#endif