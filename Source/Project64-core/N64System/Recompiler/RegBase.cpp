#include "stdafx.h"
#include <Project64-core/N64System/Recompiler/RegBase.h>

CRegBase::CRegBase() :
m_CycleCount(0),
m_Fpu_Used(false),
m_RoundingModel(RoundUnknown)
{
    for (int32_t i = 0; i < 32; i++)
    {
        m_MIPS_RegState[i] = STATE_UNKNOWN;
        m_MIPS_RegVal[i].DW = 0;
    }
    m_MIPS_RegState[0] = STATE_CONST_32_SIGN;
}

bool CRegBase::operator==(const CRegBase& right) const
{
    for (uint32_t count = 0; count < 32; count++)
    {
        if (m_MIPS_RegState[count] != right.m_MIPS_RegState[count])
        {
            return false;
        }
        if (m_MIPS_RegState[count] == STATE_UNKNOWN)
        {
            continue;
        }
        if (m_MIPS_RegVal[count].DW != right.m_MIPS_RegVal[count].DW)
        {
            return false;
        }
    }
    if (m_CycleCount != right.m_CycleCount) { return false; }
    if (m_Fpu_Used != right.m_Fpu_Used) { return false; }
    if (GetRoundingModel() != right.GetRoundingModel()) { return false; }
    return true;
}

bool CRegBase::operator!=(const CRegBase& right) const
{
    return !(right == *this);
}

CRegBase& CRegBase::operator=(const CRegBase& right)
{
    memcpy(&m_MIPS_RegState, &right.m_MIPS_RegState, sizeof(m_MIPS_RegState));
    memcpy(&m_MIPS_RegVal, &right.m_MIPS_RegVal, sizeof(m_MIPS_RegVal));
    m_CycleCount = right.m_CycleCount;
    m_Fpu_Used = right.m_Fpu_Used;
    m_RoundingModel = right.m_RoundingModel;
#ifdef _DEBUG
    if (*this != right)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    return *this;
}

const char * CRegBase::RoundingModelName(FPU_ROUND RoundType)
{
    switch (RoundType)
    {
    case RoundUnknown:  return "RoundUnknown";
    case RoundDefault:  return "RoundDefault";
    case RoundTruncate: return "RoundTruncate";
    case RoundNearest:  return "RoundNearest";
    case RoundDown:     return "RoundDown";
    case RoundUp:       return "RoundUp";
    }
    return "** Invalid **";
}
