#include "stdafx.h"

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/x86/x86RegInfo.h>

#include <stdio.h>
#include <string.h>
#include <float.h>

uint32_t CX86RegInfo::m_fpuControl = 0;

const char *Format_Name[] = { "Unknown", "dword", "qword", "float", "double" };

CX86RegInfo::CX86RegInfo(CCodeBlock & CodeBlock, CX86Ops & Assembler) :
    m_CodeBlock(CodeBlock),
    m_Assembler(Assembler),
    m_Stack_TopPos(0),
    m_InBeforeCallDirect(false)
{
    for (int32_t i = 0; i < 32; i++)
    {
        m_RegMapLo[i] = CX86Ops::x86_Unknown;
        m_RegMapHi[i] = CX86Ops::x86_Unknown;
    }
    for (int32_t i = 0, n = sizeof(m_x86reg_MappedTo) / sizeof(m_x86reg_MappedTo[0]); i < n; i++)
    {
        m_x86reg_MappedTo[i] = NotMapped;
        m_x86reg_Protected[i] = false;
        m_x86reg_MapOrder[i] = 0;
    }
    for (int32_t i = 0, n = sizeof(m_x86fpu_MappedTo) / sizeof(m_x86fpu_MappedTo[0]); i < n; i++)
    {
        m_x86fpu_MappedTo[i] = -1;
        m_x86fpu_State[i] = FPU_Unknown;
        m_x86fpu_StateChanged[i] = false;
        m_x86fpu_RoundingModel[i] = RoundDefault;
    }
}

CX86RegInfo::CX86RegInfo(const CX86RegInfo& rhs) :
    m_CodeBlock(rhs.m_CodeBlock),
    m_Assembler(rhs.m_CodeBlock.RecompilerOps()->Assembler())
{
    *this = rhs;
}

CX86RegInfo::~CX86RegInfo()
{
}

CX86RegInfo& CX86RegInfo::operator=(const CX86RegInfo& right)
{
    CRegBase::operator=(right);
    m_Stack_TopPos = right.m_Stack_TopPos;
    m_InBeforeCallDirect = right.m_InBeforeCallDirect;

    memcpy(&m_RegMapLo, &right.m_RegMapLo, sizeof(m_RegMapLo));
    memcpy(&m_RegMapHi, &right.m_RegMapHi, sizeof(m_RegMapHi));
    memcpy(&m_x86reg_MappedTo, &right.m_x86reg_MappedTo, sizeof(m_x86reg_MappedTo));
    memcpy(&m_x86reg_Protected, &right.m_x86reg_Protected, sizeof(m_x86reg_Protected));
    memcpy(&m_x86reg_MapOrder, &right.m_x86reg_MapOrder, sizeof(m_x86reg_MapOrder));

    memcpy(&m_x86fpu_MappedTo, &right.m_x86fpu_MappedTo, sizeof(m_x86fpu_MappedTo));
    memcpy(&m_x86fpu_State, &right.m_x86fpu_State, sizeof(m_x86fpu_State));
    memcpy(&m_x86fpu_StateChanged, &right.m_x86fpu_StateChanged, sizeof(m_x86fpu_StateChanged));
    memcpy(&m_x86fpu_RoundingModel, &right.m_x86fpu_RoundingModel, sizeof(m_x86fpu_RoundingModel));

#ifdef _DEBUG
    if (*this != right)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    return *this;
}

bool CX86RegInfo::operator==(const CX86RegInfo& right) const
{
    if (!CRegBase::operator==(right))
    {
        return false;
    }

    int32_t count;

    for (count = 0; count < 10; count++)
    {
        if (m_x86reg_MappedTo[count] != right.m_x86reg_MappedTo[count]) { return false; }
        if (m_x86reg_Protected[count] != right.m_x86reg_Protected[count]) { return false; }
        if (m_x86reg_MapOrder[count] != right.m_x86reg_MapOrder[count]) { return false; }
    }
    if (m_Stack_TopPos != right.m_Stack_TopPos) { return false; }

    for (count = 0; count < 8; count++)
    {
        if (m_x86fpu_MappedTo[count] != right.m_x86fpu_MappedTo[count]) { return false; }
        if (m_x86fpu_State[count] != right.m_x86fpu_State[count]) { return false; }
        if (m_x86fpu_RoundingModel[count] != right.m_x86fpu_RoundingModel[count]) { return false; }
    }
    return true;
}

bool CX86RegInfo::operator!=(const CX86RegInfo& right) const
{
    return !(right == *this);
}

CX86RegInfo::REG_STATE CX86RegInfo::ConstantsType(int64_t Value)
{
    if (((Value >> 32) == -1) && ((Value & 0x80000000) != 0)) { return STATE_CONST_32_SIGN; }
    if (((Value >> 32) == 0) && ((Value & 0x80000000) == 0)) { return STATE_CONST_32_SIGN; }
    return STATE_CONST_64;
}

void CX86RegInfo::BeforeCallDirect(void)
{
    if (m_InBeforeCallDirect)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_InBeforeCallDirect = true;
    UnMap_AllFPRs();
    m_Assembler.Pushad();
}

void CX86RegInfo::AfterCallDirect(void)
{
    if (!m_InBeforeCallDirect)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_InBeforeCallDirect = false;
    m_Assembler.Popad();
    SetRoundingModel(CRegInfo::RoundUnknown);
}

void CX86RegInfo::FixRoundModel(FPU_ROUND RoundMethod)
{
    if (GetRoundingModel() == RoundMethod)
    {
        return;
    }
    m_CodeBlock.Log("    FixRoundModel: CurrentRoundingModel: %s  targetRoundModel: %s", RoundingModelName(GetRoundingModel()), RoundingModelName(RoundMethod));

    m_fpuControl = 0;
    m_Assembler.fpuStoreControl(&m_fpuControl, "m_fpuControl");
    CX86Ops::x86Reg reg = Map_TempReg(CX86Ops::x86_Any, -1, false);
    m_Assembler.MoveVariableToX86reg(&m_fpuControl, "m_fpuControl", reg);
    m_Assembler.AndConstToX86Reg(reg, 0xF3FF);

    if (RoundMethod == RoundDefault)
    {
#ifdef _WIN32
        static const unsigned int msRound[4] =
        {
            0x00000000, //_RC_NEAR
            0x00000300, //_RC_CHOP
            0x00000200, //_RC_UP
            0x00000100, //_RC_DOWN
        };

        CX86Ops::x86Reg RoundReg = Map_TempReg(CX86Ops::x86_Any, -1, false);
        m_Assembler.MoveVariableToX86reg(&g_Reg->m_RoundingModel, "m_RoundingModel", RoundReg);
        m_Assembler.MoveVariableDispToX86Reg((void *)&msRound[0], "msRound", RoundReg, RoundReg, CX86Ops::Multip_x4);

        m_Assembler.ShiftLeftSignImmed(RoundReg, 2);
        m_Assembler.OrX86RegToX86Reg(reg, RoundReg);
#else
        CX86Ops::x86Reg RoundReg = Map_TempReg(CX86Ops::x86_Any, -1, false);
        m_Assembler.MoveVariableToX86reg(_RoundingModel, "_RoundingModel", RoundReg);
        m_Assembler.OrX86RegToX86Reg(reg, RoundReg);
#endif
        SetX86Protected(RoundReg, false);
    }
    else
    {
        switch (RoundMethod)
        {
        case RoundTruncate: m_Assembler.OrConstToX86Reg(0x0C00, reg); break;
        case RoundNearest: m_Assembler.OrConstToX86Reg(0x0000, reg); break;
        case RoundDown: m_Assembler.OrConstToX86Reg(0x0400, reg); break;
        case RoundUp: m_Assembler.OrConstToX86Reg(0x0800, reg); break;
        default:
            g_Notify->DisplayError("Unknown rounding model");
        }
    }
    m_Assembler.MoveX86regToVariable(reg, &m_fpuControl, "m_fpuControl");
    SetX86Protected(reg, false);
    m_Assembler.fpuLoadControl(&m_fpuControl, "m_fpuControl");
    SetRoundingModel(RoundMethod);
}

void CX86RegInfo::ChangeFPURegFormat(int32_t Reg, FPU_STATE OldFormat, FPU_STATE NewFormat, FPU_ROUND RoundingModel)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        if (m_x86fpu_MappedTo[i] != Reg)
        {
            continue;
        }
        if (m_x86fpu_State[i] != OldFormat || m_x86fpu_StateChanged[i])
        {
            UnMap_FPR(Reg, true);
            Load_FPR_ToTop(Reg, Reg, OldFormat);
        }
        else
        {
            m_CodeBlock.Log("    regcache: Changed format of ST(%d) from %s to %s", (i - StackTopPos() + 8) & 7, Format_Name[OldFormat], Format_Name[NewFormat]);
        }
        FpuRoundingModel(i) = RoundingModel;
        m_x86fpu_State[i] = NewFormat;
        m_x86fpu_StateChanged[i] = true;
        return;
    }

    if (HaveDebugger())
    {
        g_Notify->DisplayError("ChangeFormat: Register not on stack!");
    }
}

void CX86RegInfo::Load_FPR_ToTop(int32_t Reg, int32_t RegToLoad, FPU_STATE Format)
{
    if (GetRoundingModel() != RoundDefault)
    {
        FixRoundModel(RoundDefault);
    }
    m_CodeBlock.Log("CurrentRoundingModel: %s  FpuRoundingModel(StackTopPos()): %s", RoundingModelName(GetRoundingModel()), RoundingModelName(FpuRoundingModel(StackTopPos())));
    int32_t i;

    if (RegToLoad < 0) { g_Notify->DisplayError("Load_FPR_ToTop\nRegToLoad < 0 ???"); return; }
    if (Reg < 0) { g_Notify->DisplayError("Load_FPR_ToTop\nReg < 0 ???"); return; }

    if (Format == FPU_Double || Format == FPU_Qword)
    {
        UnMap_FPR(Reg + 1, true);
        UnMap_FPR(RegToLoad + 1, true);
    }
    else
    {
        if ((Reg & 1) != 0)
        {
            for (i = 0; i < 8; i++)
            {
                if (m_x86fpu_MappedTo[i] == (Reg - 1))
                {
                    if (m_x86fpu_State[i] == FPU_Double || m_x86fpu_State[i] == FPU_Qword)
                    {
                        UnMap_FPR(Reg, true);
                    }
                    i = 8;
                }
            }
        }
        if ((RegToLoad & 1) != 0)
        {
            for (i = 0; i < 8; i++)
            {
                if (m_x86fpu_MappedTo[i] == (RegToLoad - 1))
                {
                    if (m_x86fpu_State[i] == FPU_Double || m_x86fpu_State[i] == FPU_Qword)
                    {
                        UnMap_FPR(RegToLoad, true);
                    }
                    i = 8;
                }
            }
        }
    }

    if (Reg == RegToLoad)
    {
        // If different format then unmap original register from stack
        for (i = 0; i < 8; i++)
        {
            if (m_x86fpu_MappedTo[i] != Reg)
            {
                continue;
            }
            if (m_x86fpu_State[i] != Format)
            {
                UnMap_FPR(Reg, true);
            }
            break;
        }
    }
    else
    {
        // If different format then unmap original register from stack
        for (i = 0; i < 8; i++)
        {
            if (m_x86fpu_MappedTo[i] != Reg)
            {
                continue;
            }
            UnMap_FPR(Reg, m_x86fpu_State[i] != Format);
            break;
        }
    }

    if (RegInStack(RegToLoad, Format))
    {
        if (Reg != RegToLoad)
        {
            if (m_x86fpu_MappedTo[(StackTopPos() - 1) & 7] != RegToLoad)
            {
                UnMap_FPR(m_x86fpu_MappedTo[(StackTopPos() - 1) & 7], true);
                m_CodeBlock.Log("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);
                m_Assembler.fpuLoadReg(&StackTopPos(), StackPosition(RegToLoad));
                FpuRoundingModel(StackTopPos()) = RoundDefault;
                m_x86fpu_MappedTo[StackTopPos()] = Reg;
                m_x86fpu_State[StackTopPos()] = Format;
                m_x86fpu_StateChanged[StackTopPos()] = false;
            }
            else
            {
                UnMap_FPR(m_x86fpu_MappedTo[(StackTopPos() - 1) & 7], true);
                Load_FPR_ToTop(Reg, RegToLoad, Format);
            }
        }
        else
        {
            CX86Ops::x86FpuValues RegPos = CX86Ops::x86_ST_Unknown;
            for (uint32_t z = 0; z < 8; z++)
            {
                if (m_x86fpu_MappedTo[z] == Reg)
                {
                    RegPos = (CX86Ops::x86FpuValues)i;
                    z = 8;
                }
            }

            if (RegPos == StackTopPos())
            {
                return;
            }
            CX86Ops::x86FpuValues StackPos = StackPosition(Reg);

            FpuRoundingModel(RegPos) = FpuRoundingModel(StackTopPos());
            m_x86fpu_MappedTo[RegPos] = m_x86fpu_MappedTo[StackTopPos()];
            m_x86fpu_State[RegPos] = m_x86fpu_State[StackTopPos()];
            m_x86fpu_StateChanged[RegPos] = m_x86fpu_StateChanged[StackTopPos()];
            m_CodeBlock.Log("    regcache: allocate ST(%d) to %s", StackPos, CRegName::FPR[m_x86fpu_MappedTo[RegPos]]);
            m_CodeBlock.Log("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);

            m_Assembler.fpuExchange(StackPos);

            FpuRoundingModel(StackTopPos()) = RoundDefault;
            m_x86fpu_MappedTo[StackTopPos()] = Reg;
            m_x86fpu_State[StackTopPos()] = Format;
            m_x86fpu_StateChanged[StackTopPos()] = false;
        }
    }
    else
    {
        char Name[50];
        CX86Ops::x86Reg TempReg;

        UnMap_FPR(m_x86fpu_MappedTo[(StackTopPos() - 1) & 7], true);
        for (i = 0; i < 8; i++)
        {
            if (m_x86fpu_MappedTo[i] == RegToLoad)
            {
                UnMap_FPR(RegToLoad, true);
                i = 8;
            }
        }
        m_CodeBlock.Log("    regcache: allocate ST(0) to %s", CRegName::FPR[Reg]);
        TempReg = Map_TempReg(CX86Ops::x86_Any, -1, false);
        switch (Format)
        {
        case FPU_Dword:
            sprintf(Name, "m_FPR_S[%d]", RegToLoad);
            m_Assembler.MoveVariableToX86reg(&g_Reg->m_FPR_S[RegToLoad], Name, TempReg);
            m_Assembler.fpuLoadIntegerDwordFromX86Reg(&StackTopPos(), TempReg);
            break;
        case FPU_Qword:
            sprintf(Name, "m_FPR_D[%d]", RegToLoad);
            m_Assembler.MoveVariableToX86reg(&g_Reg->m_FPR_D[RegToLoad], Name, TempReg);
            m_Assembler.fpuLoadIntegerQwordFromX86Reg(&StackTopPos(), TempReg);
            break;
        case FPU_Float:
            sprintf(Name, "m_FPR_S[%d]", RegToLoad);
            m_Assembler.MoveVariableToX86reg(&g_Reg->m_FPR_S[RegToLoad], Name, TempReg);
            m_Assembler.fpuLoadDwordFromX86Reg(&StackTopPos(), TempReg);
            break;
        case FPU_Double:
            sprintf(Name, "m_FPR_D[%d]", RegToLoad);
            m_Assembler.MoveVariableToX86reg(&g_Reg->m_FPR_D[RegToLoad], Name, TempReg);
            m_Assembler.fpuLoadQwordFromX86Reg(&StackTopPos(), TempReg);
            break;
        default:
            if (HaveDebugger()) { g_Notify->DisplayError(stdstr_f("Load_FPR_ToTop\nUnkown format to load %d", Format).c_str()); }
        }
        SetX86Protected(TempReg, false);
        FpuRoundingModel(StackTopPos()) = RoundDefault;
        m_x86fpu_MappedTo[StackTopPos()] = Reg;
        m_x86fpu_State[StackTopPos()] = Format;
        m_x86fpu_StateChanged[StackTopPos()] = false;
    }
}

CX86Ops::x86FpuValues CX86RegInfo::StackPosition(int32_t Reg)
{
    int32_t i;

    for (i = 0; i < 8; i++)
    {
        if (m_x86fpu_MappedTo[i] == Reg)
        {
            return (CX86Ops::x86FpuValues)((i - StackTopPos()) & 7);
        }
    }
    return CX86Ops::x86_ST_Unknown;
}

CX86Ops::CX86Ops::x86Reg CX86RegInfo::FreeX86Reg()
{
    if (GetX86Mapped(CX86Ops::x86_EDI) == NotMapped && !GetX86Protected(CX86Ops::x86_EDI)) { return CX86Ops::x86_EDI; }
    if (GetX86Mapped(CX86Ops::x86_ESI) == NotMapped && !GetX86Protected(CX86Ops::x86_ESI)) { return CX86Ops::x86_ESI; }
    if (GetX86Mapped(CX86Ops::x86_EBX) == NotMapped && !GetX86Protected(CX86Ops::x86_EBX)) { return CX86Ops::x86_EBX; }
    if (GetX86Mapped(CX86Ops::x86_EAX) == NotMapped && !GetX86Protected(CX86Ops::x86_EAX)) { return CX86Ops::x86_EAX; }
    if (GetX86Mapped(CX86Ops::x86_EDX) == NotMapped && !GetX86Protected(CX86Ops::x86_EDX)) { return CX86Ops::x86_EDX; }
    if (GetX86Mapped(CX86Ops::x86_ECX) == NotMapped && !GetX86Protected(CX86Ops::x86_ECX)) { return CX86Ops::x86_ECX; }

    CX86Ops::x86Reg Reg = UnMap_TempReg();
    if (Reg != CX86Ops::x86_Unknown) { return Reg; }

    int32_t count, MapCount[10];
    CX86Ops::x86Reg MapReg[10];

    for (count = 0; count < 10; count++)
    {
        MapCount[count] = GetX86MapOrder((CX86Ops::x86Reg)count);
        MapReg[count] = (CX86Ops::x86Reg)count;
    }
    for (count = 0; count < 10; count++)
    {
        int32_t i;

        for (i = 0; i < 9; i++)
        {
            CX86Ops::x86Reg tempReg;
            uint32_t temp;

            if (MapCount[i] < MapCount[i + 1])
            {
                temp = MapCount[i];
                MapCount[i] = MapCount[i + 1];
                MapCount[i + 1] = temp;
                tempReg = MapReg[i];
                MapReg[i] = MapReg[i + 1];
                MapReg[i + 1] = tempReg;
            }
        }
    }

    CX86Ops::x86Reg StackReg = CX86Ops::x86_Unknown;
    for (count = 0; count < 10; count++)
    {
        if (MapCount[count] > 0 && GetX86Mapped(MapReg[count]) != Stack_Mapped)
        {
            if (UnMap_X86reg((CX86Ops::x86Reg)MapReg[count]))
            {
                return (CX86Ops::x86Reg)MapReg[count];
            }
        }
        if (GetX86Mapped(MapReg[count]) == Stack_Mapped) { StackReg = MapReg[count]; }
    }
    if (StackReg != CX86Ops::x86_Unknown)
    {
        UnMap_X86reg(StackReg);
        return StackReg;
    }

    return CX86Ops::x86_Unknown;
}

CX86Ops::CX86Ops::x86Reg CX86RegInfo::Free8BitX86Reg()
{
    if (GetX86Mapped(CX86Ops::x86_EBX) == NotMapped && !GetX86Protected(CX86Ops::x86_EBX)) { return CX86Ops::x86_EBX; }
    if (GetX86Mapped(CX86Ops::x86_EAX) == NotMapped && !GetX86Protected(CX86Ops::x86_EAX)) { return CX86Ops::x86_EAX; }
    if (GetX86Mapped(CX86Ops::x86_EDX) == NotMapped && !GetX86Protected(CX86Ops::x86_EDX)) { return CX86Ops::x86_EDX; }
    if (GetX86Mapped(CX86Ops::x86_ECX) == NotMapped && !GetX86Protected(CX86Ops::x86_ECX)) { return CX86Ops::x86_ECX; }

    CX86Ops::x86Reg Reg = UnMap_8BitTempReg();
    if (Reg > 0) { return Reg; }

    int32_t count, MapCount[10], MapReg[10];
    for (count = 0; count < 10; count++)
    {
        MapCount[count] = GetX86MapOrder((CX86Ops::x86Reg)count);
        MapReg[count] = count;
    }
    for (count = 0; count < 10; count++)
    {
        int32_t i;

        for (i = 0; i < 9; i++)
        {
            int32_t temp;

            if (MapCount[i] < MapCount[i + 1])
            {
                temp = MapCount[i];
                MapCount[i] = MapCount[i + 1];
                MapCount[i + 1] = temp;
                temp = MapReg[i];
                MapReg[i] = MapReg[i + 1];
                MapReg[i + 1] = temp;
            }
        }
    }
    for (count = 0; count < 10; count++)
    {
        if (MapCount[count] > 0)
        {
            if (!CX86Ops::Is8BitReg((CX86Ops::x86Reg)count)) { continue; }
            if (UnMap_X86reg((CX86Ops::x86Reg)count))
            {
                return (CX86Ops::x86Reg)count;
            }
        }
    }
    return CX86Ops::x86_Unknown;
}

CX86Ops::CX86Ops::x86Reg CX86RegInfo::UnMap_8BitTempReg()
{
    int32_t count;

    for (count = 0; count < 10; count++)
    {
        if (!CX86Ops::Is8BitReg((CX86Ops::x86Reg)count)) { continue; }
        if (GetX86Mapped((CX86Ops::x86Reg)count) == Temp_Mapped)
        {
            if (GetX86Protected((CX86Ops::x86Reg)count) == false)
            {
                m_CodeBlock.Log("    regcache: unallocate %s from temp storage", CX86Ops::x86_Name((CX86Ops::x86Reg)count));
                SetX86Mapped((CX86Ops::x86Reg)count, CX86RegInfo::NotMapped);
                return (CX86Ops::x86Reg)count;
            }
        }
    }
    return CX86Ops::x86_Unknown;
}

CX86Ops::x86Reg CX86RegInfo::Get_MemoryStack() const
{
    for (int32_t i = 0, n = sizeof(CX86Ops::x86_Registers) / sizeof(CX86Ops::x86_Registers[0]); i < n; i++)
    {
        if (GetX86Mapped(CX86Ops::x86_Registers[i]) == Stack_Mapped)
        {
            return CX86Ops::x86_Registers[i];
        }
    }
    return CX86Ops::x86_Unknown;
}

CX86Ops::x86Reg CX86RegInfo::Map_MemoryStack(CX86Ops::x86Reg Reg, bool bMapRegister, bool LoadValue)
{
    CX86Ops::x86Reg CurrentMap = Get_MemoryStack();
    if (!bMapRegister)
    {
        // If not mapping then just return what the current mapping is
        return CurrentMap;
    }

    if (CurrentMap != CX86Ops::x86_Unknown && CurrentMap == Reg)
    {
        // Already mapped to correct register
        return CurrentMap;
    }
    // Map a register
    if (Reg == CX86Ops::x86_Any)
    {
        if (CurrentMap != CX86Ops::x86_Unknown)
        {
            return CurrentMap;
        }
        Reg = FreeX86Reg();
        if (Reg == CX86Ops::x86_Unknown)
        {
            g_Notify->DisplayError("Map_MemoryStack\n\nOut of registers");
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        SetX86Mapped(Reg, CX86RegInfo::Stack_Mapped);
        m_CodeBlock.Log("    regcache: allocate %s as Memory Stack", CX86Ops::x86_Name(Reg));
        if (LoadValue)
        {
            m_Assembler.MoveVariableToX86reg(&g_Recompiler->MemoryStackPos(), "MemoryStack", Reg);
        }
        return Reg;
    }

    // Move to a register/allocate register
    UnMap_X86reg(Reg);
    if (CurrentMap != CX86Ops::x86_Unknown)
    {
        m_CodeBlock.Log("    regcache: change allocation of memory stack from %s to %s", CX86Ops::x86_Name(CurrentMap), CX86Ops::x86_Name(Reg));
        SetX86Mapped(Reg, CX86RegInfo::Stack_Mapped);
        SetX86Mapped(CurrentMap, CX86RegInfo::NotMapped);
        m_Assembler.MoveX86RegToX86Reg(CurrentMap, Reg);
    }
    else
    {
        SetX86Mapped(Reg, CX86RegInfo::Stack_Mapped);
        m_CodeBlock.Log("    regcache: allocate %s as memory stack", CX86Ops::x86_Name(Reg));
        if (LoadValue)
        {
            m_Assembler.MoveVariableToX86reg(&g_Recompiler->MemoryStackPos(), "MemoryStack", Reg);
        }
    }
    return Reg;
}

void CX86RegInfo::Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad)
{
    int32_t count;

    CX86Ops::x86Reg Reg;
    if (MipsReg == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        Reg = FreeX86Reg();
        if (Reg < 0)
        {
            if (HaveDebugger()) { g_Notify->DisplayError("Map_GPR_32bit\n\nOut of registers"); }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        m_CodeBlock.Log("    regcache: allocate %s to %s", CX86Ops::x86_Name(Reg), CRegName::GPR[MipsReg]);
    }
    else
    {
        if (Is64Bit(MipsReg))
        {
            m_CodeBlock.Log("    regcache: unallocate %s from high 32-bit of %s", CX86Ops::x86_Name(GetMipsRegMapHi(MipsReg)), CRegName::GPR_Hi[MipsReg]);
            SetX86MapOrder(GetMipsRegMapHi(MipsReg), 0);
            SetX86Mapped(GetMipsRegMapHi(MipsReg), NotMapped);
            SetX86Protected(GetMipsRegMapHi(MipsReg), false);
            SetMipsRegHi(MipsReg, 0);
        }
        Reg = GetMipsRegMapLo(MipsReg);
    }
    for (count = 0; count < 10; count++)
    {
        uint32_t Count = GetX86MapOrder((CX86Ops::x86Reg)count);
        if (Count > 0)
        {
            SetX86MapOrder((CX86Ops::x86Reg)count, Count + 1);
        }
    }
    SetX86MapOrder(Reg, 1);

    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            m_Assembler.MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0], CRegName::GPR_Lo[MipsRegToLoad], Reg);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (MipsReg != MipsRegToLoad)
            {
                m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), Reg);
            }
        }
        else
        {
            m_Assembler.MoveConstToX86reg(GetMipsRegLo(MipsRegToLoad), Reg);
        }
    }
    else if (MipsRegToLoad == 0)
    {
        m_Assembler.XorX86RegToX86Reg(Reg, Reg);
    }
    SetX86Mapped(Reg, GPR_Mapped);
    SetX86Protected(Reg, true);
    SetMipsRegMapLo(MipsReg, Reg);
    SetMipsRegState(MipsReg, SignValue ? STATE_MAPPED_32_SIGN : STATE_MAPPED_32_ZERO);
}

void CX86RegInfo::Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad)
{
    CX86Ops::x86Reg x86Hi, x86lo;
    int32_t count;

    if (MipsReg == 0)
    {
        if (HaveDebugger()) { g_Notify->DisplayError("Map_GPR_32bit\n\nWhy are you trying to map register 0?"); }
        return;
    }

    ProtectGPR(MipsReg);
    if (IsUnknown(MipsReg) || IsConst(MipsReg))
    {
        x86Hi = FreeX86Reg();
        if (x86Hi < 0)
        {
            if (HaveDebugger()) { g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers"); }
            return;
        }
        SetX86Protected(x86Hi, true);

        x86lo = FreeX86Reg();
        if (x86lo < 0) { g_Notify->DisplayError("Map_GPR_64bit\n\nOut of registers"); return; }
        SetX86Protected(x86lo, true);

        m_CodeBlock.Log("    regcache: allocate %s to hi word of %s", CX86Ops::x86_Name(x86Hi), CRegName::GPR[MipsReg]);
        m_CodeBlock.Log("    regcache: allocate %s to low word of %s", CX86Ops::x86_Name(x86lo), CRegName::GPR[MipsReg]);
    }
    else
    {
        x86lo = GetMipsRegMapLo(MipsReg);
        if (Is32Bit(MipsReg))
        {
            SetX86Protected(x86lo, true);
            x86Hi = FreeX86Reg();
            if (x86Hi == CX86Ops::x86_Unknown)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return;
            }
            SetX86Protected(x86Hi, true);

            m_CodeBlock.Log("    regcache: allocate %s to hi word of %s", CX86Ops::x86_Name(x86Hi), CRegName::GPR[MipsReg]);
        }
        else
        {
            x86Hi = GetMipsRegMapHi(MipsReg);
        }
    }

    for (count = 0; count < 10; count++)
    {
        int32_t MapOrder = GetX86MapOrder((CX86Ops::x86Reg)count);
        if (MapOrder > 0)
        {
            SetX86MapOrder((CX86Ops::x86Reg)count, MapOrder + 1);
        }
    }

    SetX86MapOrder(x86Hi, 1);
    SetX86MapOrder(x86lo, 1);
    if (MipsRegToLoad > 0)
    {
        if (IsUnknown(MipsRegToLoad))
        {
            m_Assembler.MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[1], CRegName::GPR_Hi[MipsRegToLoad], x86Hi);
            m_Assembler.MoveVariableToX86reg(&_GPR[MipsRegToLoad].UW[0], CRegName::GPR_Lo[MipsRegToLoad], x86lo);
        }
        else if (IsMapped(MipsRegToLoad))
        {
            if (Is32Bit(MipsRegToLoad))
            {
                if (IsSigned(MipsRegToLoad))
                {
                    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), x86Hi);
                    m_Assembler.ShiftRightSignImmed(x86Hi, 31);
                }
                else
                {
                    m_Assembler.XorX86RegToX86Reg(x86Hi, x86Hi);
                }
                if (MipsReg != MipsRegToLoad)
                {
                    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), x86lo);
                }
            }
            else
            {
                if (MipsReg != MipsRegToLoad)
                {
                    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(MipsRegToLoad), x86Hi);
                    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(MipsRegToLoad), x86lo);
                }
            }
        }
        else
        {
            m_CodeBlock.Log("Map_GPR_64bit 11");
            if (Is32Bit(MipsRegToLoad))
            {
                if (IsSigned(MipsRegToLoad))
                {
                    m_Assembler.MoveConstToX86reg(GetMipsRegLo_S(MipsRegToLoad) >> 31, x86Hi);
                }
                else
                {
                    m_Assembler.MoveConstToX86reg(0, x86Hi);
                }
            }
            else
            {
                m_Assembler.MoveConstToX86reg(GetMipsRegHi(MipsRegToLoad), x86Hi);
            }
            m_Assembler.MoveConstToX86reg(GetMipsRegLo(MipsRegToLoad), x86lo);
        }
    }
    else if (MipsRegToLoad == 0)
    {
        m_Assembler.XorX86RegToX86Reg(x86Hi, x86Hi);
        m_Assembler.XorX86RegToX86Reg(x86lo, x86lo);
    }
    SetX86Mapped(x86Hi, GPR_Mapped);
    SetX86Mapped(x86lo, GPR_Mapped);
    SetMipsRegMapHi(MipsReg, x86Hi);
    SetMipsRegMapLo(MipsReg, x86lo);
    SetMipsRegState(MipsReg, STATE_MAPPED_64);
}

CX86Ops::CX86Ops::x86Reg CX86RegInfo::Map_TempReg(CX86Ops::CX86Ops::x86Reg Reg, int32_t MipsReg, bool LoadHiWord)
{
    int32_t count;

    if (Reg == CX86Ops::x86_Any)
    {
        if (GetX86Mapped(CX86Ops::x86_EAX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EAX)) { Reg = CX86Ops::x86_EAX; }
        else if (GetX86Mapped(CX86Ops::x86_EBX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EBX)) { Reg = CX86Ops::x86_EBX; }
        else if (GetX86Mapped(CX86Ops::x86_ECX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ECX)) { Reg = CX86Ops::x86_ECX; }
        else if (GetX86Mapped(CX86Ops::x86_EDX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EDX)) { Reg = CX86Ops::x86_EDX; }
        else if (GetX86Mapped(CX86Ops::x86_ESI) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ESI)) { Reg = CX86Ops::x86_ESI; }
        else if (GetX86Mapped(CX86Ops::x86_EDI) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EDI)) { Reg = CX86Ops::x86_EDI; }
        else if (GetX86Mapped(CX86Ops::x86_EBP) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EBP)) { Reg = CX86Ops::x86_EBP; }
        else if (GetX86Mapped(CX86Ops::x86_ESP) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ESP)) { Reg = CX86Ops::x86_ESP; }

        if (Reg == CX86Ops::x86_Any)
        {
            Reg = FreeX86Reg();
            if (Reg == CX86Ops::x86_Unknown)
            {
                WriteTrace(TraceRegisterCache, TraceError, "Failed to find a free register");
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return CX86Ops::x86_Unknown;
            }
        }
    }
    else if (Reg == CX86Ops::x86_Any8Bit)
    {
        if (GetX86Mapped(CX86Ops::x86_EAX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EAX)) { Reg = CX86Ops::x86_EAX; }
        else if (GetX86Mapped(CX86Ops::x86_EBX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EBX)) { Reg = CX86Ops::x86_EBX; }
        else if (GetX86Mapped(CX86Ops::x86_ECX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ECX)) { Reg = CX86Ops::x86_ECX; }
        else if (GetX86Mapped(CX86Ops::x86_EDX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EDX)) { Reg = CX86Ops::x86_EDX; }

        if (Reg == CX86Ops::x86_Any8Bit)
        {
            Reg = Free8BitX86Reg();
            if (Reg < 0)
            {
                WriteTrace(TraceRegisterCache, TraceError, "Failed to find a free 8-bit register");
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return CX86Ops::x86_Unknown;
            }
        }
    }
    else if (GetX86Mapped(Reg) == GPR_Mapped)
    {
        if (GetX86Protected(Reg))
        {
            WriteTrace(TraceRegisterCache, TraceError, "Register is protected");
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return CX86Ops::x86_Unknown;
        }

        SetX86Protected(Reg, true);
        CX86Ops::x86Reg NewReg = FreeX86Reg();
        for (count = 1; count < 32; count++)
        {
            if (!IsMapped(count))
            {
                continue;
            }
            if (GetMipsRegMapLo(count) == Reg)
            {
                if (NewReg == CX86Ops::x86_Unknown)
                {
                    UnMap_GPR(count, true);
                    break;
                }
                m_CodeBlock.Log("    regcache: change allocation of %s from %s to %s", CRegName::GPR[count], CX86Ops::x86_Name(Reg), CX86Ops::x86_Name(NewReg));
                SetX86Mapped(NewReg, GPR_Mapped);
                SetX86MapOrder(NewReg, GetX86MapOrder(Reg));
                SetMipsRegMapLo(count, NewReg);
                m_Assembler.MoveX86RegToX86Reg(Reg, NewReg);
                if (MipsReg == count && !LoadHiWord)
                {
                    MipsReg = -1;
                }
                break;
            }
            if (Is64Bit(count) && GetMipsRegMapHi(count) == Reg)
            {
                if (NewReg == CX86Ops::x86_Unknown)
                {
                    UnMap_GPR(count, true);
                    break;
                }
                m_CodeBlock.Log("    regcache: change allocation of %s from %s to %s", CRegName::GPR_Hi[count], CX86Ops::x86_Name(Reg), CX86Ops::x86_Name(NewReg));
                SetX86Mapped(NewReg, GPR_Mapped);
                SetX86MapOrder(NewReg, GetX86MapOrder(Reg));
                SetMipsRegMapHi(count, NewReg);
                m_Assembler.MoveX86RegToX86Reg(Reg, NewReg);
                if (MipsReg == count && LoadHiWord)
                {
                    MipsReg = -1;
                }
                break;
            }
        }
    }
    else if (GetX86Mapped(Reg) == Stack_Mapped)
    {
        UnMap_X86reg(Reg);
    }
    m_CodeBlock.Log("    regcache: allocate %s as temp storage", CX86Ops::x86_Name(Reg));

    if (MipsReg >= 0)
    {
        if (LoadHiWord)
        {
            if (IsUnknown(MipsReg))
            {
                m_Assembler.MoveVariableToX86reg(&_GPR[MipsReg].UW[1], CRegName::GPR_Hi[MipsReg], Reg);
            }
            else if (IsMapped(MipsReg))
            {
                if (Is64Bit(MipsReg))
                {
                    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(MipsReg), Reg);
                }
                else if (IsSigned(MipsReg))
                {
                    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(MipsReg), Reg);
                    m_Assembler.ShiftRightSignImmed(Reg, 31);
                }
                else
                {
                    m_Assembler.MoveConstToX86reg(0, Reg);
                }
            }
            else
            {
                if (Is64Bit(MipsReg))
                {
                    m_Assembler.MoveConstToX86reg(GetMipsRegHi(MipsReg), Reg);
                }
                else
                {
                    m_Assembler.MoveConstToX86reg(GetMipsRegLo_S(MipsReg) >> 31, Reg);
                }
            }
        }
        else
        {
            if (IsUnknown(MipsReg))
            {
                m_Assembler.MoveVariableToX86reg(&_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], Reg);
            }
            else if (IsMapped(MipsReg))
            {
                m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(MipsReg), Reg);
            }
            else
            {
                m_Assembler.MoveConstToX86reg(GetMipsRegLo(MipsReg), Reg);
            }
        }
    }
    SetX86Mapped(Reg, Temp_Mapped);
    SetX86Protected(Reg, true);
    for (count = 0; count < 10; count++)
    {
        int32_t MapOrder = GetX86MapOrder((CX86Ops::x86Reg)count);
        if (MapOrder > 0)
        {
            SetX86MapOrder((CX86Ops::x86Reg)count, MapOrder + 1);
        }
    }
    SetX86MapOrder(Reg, 1);
    return Reg;
}

void CX86RegInfo::ProtectGPR(uint32_t Reg)
{
    if (IsUnknown(Reg) || IsConst(Reg))
    {
        return;
    }
    if (Is64Bit(Reg))
    {
        SetX86Protected(GetMipsRegMapHi(Reg), true);
    }
    SetX86Protected(GetMipsRegMapLo(Reg), true);
}

void CX86RegInfo::UnProtectGPR(uint32_t Reg)
{
    if (IsUnknown(Reg) || IsConst(Reg))
    {
        return;
    }
    if (Is64Bit(Reg))
    {
        SetX86Protected(GetMipsRegMapHi(Reg), false);
    }
    SetX86Protected(GetMipsRegMapLo(Reg), false);
}

void CX86RegInfo::ResetX86Protection()
{
    for (int32_t count = 0; count < 10; count++)
    {
        SetX86Protected((CX86Ops::x86Reg)count, false);
    }
}

bool CX86RegInfo::RegInStack(int32_t Reg, FPU_STATE Format)
{
    for (int32_t i = 0; i < 8; i++)
    {
        if (m_x86fpu_MappedTo[i] == Reg)
        {
            if (m_x86fpu_State[i] == Format || Format == FPU_Any)
            {
                return true;
            }
            return false;
        }
    }
    return false;
}

void CX86RegInfo::UnMap_AllFPRs()
{
    for (;;)
    {
        int32_t StackPos = StackTopPos();
        if (m_x86fpu_MappedTo[StackPos] != -1)
        {
            UnMap_FPR(m_x86fpu_MappedTo[StackPos], true);
            continue;
        }
        // See if any more registers mapped
        int32_t StartPos = StackTopPos();
        for (int32_t i = 0; i < 8; i++)
        {
            if (m_x86fpu_MappedTo[(StartPos + i) & 7] != -1) { m_Assembler.fpuIncStack(&StackTopPos()); }
        }
        if (StackPos != StackTopPos()) { continue; }
        return;
    }
}

void CX86RegInfo::UnMap_FPR(int32_t Reg, bool WriteBackValue)
{
    char Name[50];
    int32_t i;

    if (Reg < 0) { return; }
    for (i = 0; i < 8; i++)
    {
        if (m_x86fpu_MappedTo[i] != Reg) { continue; }
        m_CodeBlock.Log("    regcache: unallocate %s from ST(%d)", CRegName::FPR[Reg], (i - StackTopPos() + 8) & 7);
        if (WriteBackValue)
        {
            int32_t RegPos;

            if (((i - StackTopPos() + 8) & 7) != 0)
            {
                if (m_x86fpu_MappedTo[StackTopPos()] == -1 && m_x86fpu_MappedTo[(StackTopPos() + 1) & 7] == Reg)
                {
                    m_Assembler.fpuIncStack(&StackTopPos());
                }
                else
                {
                    CX86RegInfo::FPU_ROUND RoundingModel = FpuRoundingModel(StackTopPos());
                    FPU_STATE RegState = m_x86fpu_State[StackTopPos()];
                    bool Changed = m_x86fpu_StateChanged[StackTopPos()];
                    uint32_t MappedTo = m_x86fpu_MappedTo[StackTopPos()];
                    FpuRoundingModel(StackTopPos()) = FpuRoundingModel(i);
                    m_x86fpu_MappedTo[StackTopPos()] = m_x86fpu_MappedTo[i];
                    m_x86fpu_State[StackTopPos()] = m_x86fpu_State[i];
                    m_x86fpu_StateChanged[StackTopPos()] = m_x86fpu_StateChanged[i];
                    FpuRoundingModel(i) = RoundingModel;
                    m_x86fpu_MappedTo[i] = MappedTo;
                    m_x86fpu_State[i] = RegState;
                    m_x86fpu_StateChanged[i] = Changed;
                    m_Assembler.fpuExchange((CX86Ops::x86FpuValues)((i - StackTopPos()) & 7));
                }
            }

            FixRoundModel(FpuRoundingModel(i));

            RegPos = StackTopPos();
            CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Any, -1, false);
            switch (m_x86fpu_State[StackTopPos()])
            {
            case FPU_Dword:
                sprintf(Name, "_FPR_S[%d]", m_x86fpu_MappedTo[StackTopPos()]);
                m_Assembler.MoveVariableToX86reg(&_FPR_S[m_x86fpu_MappedTo[StackTopPos()]], Name, TempReg);
                m_Assembler.fpuStoreIntegerDwordFromX86Reg(&StackTopPos(), TempReg, true);
                break;
            case FPU_Qword:
                sprintf(Name, "_FPR_D[%d]", m_x86fpu_MappedTo[StackTopPos()]);
                m_Assembler.MoveVariableToX86reg(&_FPR_D[m_x86fpu_MappedTo[StackTopPos()]], Name, TempReg);
                m_Assembler.fpuStoreIntegerQwordFromX86Reg(&StackTopPos(), TempReg, true);
                break;
            case FPU_Float:
                sprintf(Name, "_FPR_S[%d]", m_x86fpu_MappedTo[StackTopPos()]);
                m_Assembler.MoveVariableToX86reg(&_FPR_S[m_x86fpu_MappedTo[StackTopPos()]], Name, TempReg);
                m_Assembler.fpuStoreDwordFromX86Reg(&StackTopPos(), TempReg, true);
                break;
            case FPU_Double:
                sprintf(Name, "_FPR_D[%d]", m_x86fpu_MappedTo[StackTopPos()]);
                m_Assembler.MoveVariableToX86reg(&_FPR_D[m_x86fpu_MappedTo[StackTopPos()]], Name, TempReg);
                m_Assembler.fpuStoreQwordFromX86Reg(&StackTopPos(), TempReg, true);
                break;
            default:
                if (HaveDebugger())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nUnknown format to load %d", __FUNCTION__, m_x86fpu_State[StackTopPos()]).c_str());
                }
            }
            SetX86Protected(TempReg, false);
            FpuRoundingModel(RegPos) = RoundDefault;
            m_x86fpu_MappedTo[RegPos] = -1;
            m_x86fpu_State[RegPos] = FPU_Unknown;
            m_x86fpu_StateChanged[RegPos] = false;
        }
        else
        {
            m_Assembler.fpuFree((CX86Ops::x86FpuValues)((i - StackTopPos()) & 7));
            FpuRoundingModel(i) = RoundDefault;
            m_x86fpu_MappedTo[i] = -1;
            m_x86fpu_State[i] = FPU_Unknown;
            m_x86fpu_StateChanged[i] = false;
        }
        return;
    }
}

void CX86RegInfo::UnMap_GPR(uint32_t Reg, bool WriteBackValue)
{
    if (Reg == 0)
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s\n\nWhy are you trying to unmap register 0?", __FUNCTION__).c_str());
        }
        return;
    }

    if (IsUnknown(Reg)) { return; }
    //m_CodeBlock.Log("UnMap_GPR: State: %X\tReg: %s\tWriteBack: %s",State,CRegName::GPR[Reg],WriteBackValue?"true":"false");
    if (IsConst(Reg))
    {
        if (!WriteBackValue)
        {
            SetMipsRegState(Reg, STATE_UNKNOWN);
            return;
        }
        if (Is64Bit(Reg))
        {
            m_Assembler.MoveConstToVariable(GetMipsRegHi(Reg), &_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg]);
            m_Assembler.MoveConstToVariable(GetMipsRegLo(Reg), &_GPR[Reg].UW[0], CRegName::GPR_Lo[Reg]);
            SetMipsRegState(Reg, STATE_UNKNOWN);
            return;
        }
        if ((GetMipsRegLo(Reg) & 0x80000000) != 0)
        {
            m_Assembler.MoveConstToVariable(0xFFFFFFFF, &_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg]);
        }
        else
        {
            m_Assembler.MoveConstToVariable(0, &_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg]);
        }
        m_Assembler.MoveConstToVariable(GetMipsRegLo(Reg), &_GPR[Reg].UW[0], CRegName::GPR_Lo[Reg]);
        SetMipsRegState(Reg, STATE_UNKNOWN);
        return;
    }
    if (Is64Bit(Reg))
    {
        m_CodeBlock.Log("    regcache: unallocate %s from %s", CX86Ops::x86_Name(GetMipsRegMapHi(Reg)), CRegName::GPR_Hi[Reg]);
        SetX86Mapped(GetMipsRegMapHi(Reg), NotMapped);
        SetX86Protected(GetMipsRegMapHi(Reg), false);
    }
    m_CodeBlock.Log("    regcache: unallocate %s from %s", CX86Ops::x86_Name(GetMipsRegMapLo(Reg)), CRegName::GPR_Lo[Reg]);
    SetX86Mapped(GetMipsRegMapLo(Reg), NotMapped);
    SetX86Protected(GetMipsRegMapLo(Reg), false);
    if (!WriteBackValue)
    {
        SetMipsRegState(Reg, STATE_UNKNOWN);
        return;
    }
    m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(Reg), &_GPR[Reg].UW[0], CRegName::GPR_Lo[Reg]);
    if (Is64Bit(Reg))
    {
        SetMipsRegMapLo(Reg, CX86Ops::x86_Unknown);
        m_Assembler.MoveX86regToVariable(GetMipsRegMapHi(Reg), &_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg]);
        SetMipsRegMapHi(Reg, CX86Ops::x86_Unknown);
    }
    else
    {
        if (!g_System->b32BitCore())
        {
            if (IsSigned(Reg))
            {
                m_Assembler.ShiftRightSignImmed(GetMipsRegMapLo(Reg), 31);
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(Reg), &_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg]);
            }
            else
            {
                m_Assembler.MoveConstToVariable(0, &_GPR[Reg].UW[1], CRegName::GPR_Hi[Reg]);
            }
        }
        SetMipsRegMapLo(Reg, CX86Ops::x86_Unknown);
    }
    SetMipsRegState(Reg, STATE_UNKNOWN);
}

CX86Ops::CX86Ops::x86Reg CX86RegInfo::UnMap_TempReg()
{
    CX86Ops::CX86Ops::x86Reg Reg = CX86Ops::x86_Unknown;

    if (GetX86Mapped(CX86Ops::x86_EAX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EAX)) { Reg = CX86Ops::x86_EAX; }
    else if (GetX86Mapped(CX86Ops::x86_EBX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EBX)) { Reg = CX86Ops::x86_EBX; }
    else if (GetX86Mapped(CX86Ops::x86_ECX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ECX)) { Reg = CX86Ops::x86_ECX; }
    else if (GetX86Mapped(CX86Ops::x86_EDX) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EDX)) { Reg = CX86Ops::x86_EDX; }
    else if (GetX86Mapped(CX86Ops::x86_ESI) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ESI)) { Reg = CX86Ops::x86_ESI; }
    else if (GetX86Mapped(CX86Ops::x86_EDI) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EDI)) { Reg = CX86Ops::x86_EDI; }
    else if (GetX86Mapped(CX86Ops::x86_EBP) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_EBP)) { Reg = CX86Ops::x86_EBP; }
    else if (GetX86Mapped(CX86Ops::x86_ESP) == Temp_Mapped && !GetX86Protected(CX86Ops::x86_ESP)) { Reg = CX86Ops::x86_ESP; }

    if (Reg != CX86Ops::x86_Unknown)
    {
        if (GetX86Mapped(Reg) == Temp_Mapped)
        {
            m_CodeBlock.Log("    regcache: unallocate %s from temp storage", CX86Ops::x86_Name(Reg));
        }
        SetX86Mapped(Reg, NotMapped);
    }
    return Reg;
}

bool CX86RegInfo::UnMap_X86reg(CX86Ops::CX86Ops::x86Reg Reg)
{
    int32_t count;

    if (GetX86Mapped(Reg) == NotMapped)
    {
        if (!GetX86Protected(Reg))
        {
            return true;
        }
    }
    else if (GetX86Mapped(Reg) == CX86RegInfo::GPR_Mapped)
    {
        for (count = 1; count < 32; count++)
        {
            if (!IsMapped(count))
            {
                continue;
            }

            if (Is64Bit(count) && GetMipsRegMapHi(count) == Reg)
            {
                if (!GetX86Protected(Reg))
                {
                    UnMap_GPR(count, true);
                    return true;
                }
                break;
            }
            if (GetMipsRegMapLo(count) == Reg)
            {
                if (!GetX86Protected(Reg))
                {
                    UnMap_GPR(count, true);
                    return true;
                }
                break;
            }
        }
    }
    else if (GetX86Mapped(Reg) == CX86RegInfo::Temp_Mapped)
    {
        if (!GetX86Protected(Reg))
        {
            m_CodeBlock.Log("    regcache: unallocate %s from temp storage", CX86Ops::x86_Name(Reg));
            SetX86Mapped(Reg, NotMapped);
            return true;
        }
    }
    else if (GetX86Mapped(Reg) == CX86RegInfo::Stack_Mapped)
    {
        m_CodeBlock.Log("    regcache: unallocate %s from memory stack", CX86Ops::x86_Name(Reg));
        m_Assembler.MoveX86regToVariable(Reg, &(g_Recompiler->MemoryStackPos()), "MemoryStack");
        SetX86Mapped(Reg, NotMapped);
        return true;
    }

    return false;
}

void CX86RegInfo::WriteBackRegisters()
{
    UnMap_AllFPRs();

    int32_t count;
    bool bEdiZero = false;
    bool bEsiSign = false;

    int32_t X86RegCount = sizeof(CX86Ops::x86_Registers) / sizeof(CX86Ops::x86_Registers[0]);
    for (int32_t i = 0; i < X86RegCount; i++) { SetX86Protected(CX86Ops::x86_Registers[i], false); }
    for (int32_t i = 0; i < X86RegCount; i++) { UnMap_X86reg(CX86Ops::x86_Registers[i]); }

    for (count = 1; count < 32; count++)
    {
        switch (GetMipsRegState(count))
        {
        case CX86RegInfo::STATE_UNKNOWN: break;
        case CX86RegInfo::STATE_CONST_32_SIGN:
            if (!g_System->b32BitCore())
            {
                if (!bEdiZero && (!GetMipsRegLo(count) || !(GetMipsRegLo(count) & 0x80000000)))
                {
                    m_Assembler.XorX86RegToX86Reg(CX86Ops::x86_EDI, CX86Ops::x86_EDI);
                    bEdiZero = true;
                }
                if (!bEsiSign && (GetMipsRegLo(count) & 0x80000000))
                {
                    m_Assembler.MoveConstToX86reg(0xFFFFFFFF, CX86Ops::x86_ESI);
                    bEsiSign = true;
                }
                if ((GetMipsRegLo(count) & 0x80000000) != 0)
                {
                    m_Assembler.MoveX86regToVariable(CX86Ops::x86_ESI, &_GPR[count].UW[1], CRegName::GPR_Hi[count]);
                }
                else
                {
                    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDI, &_GPR[count].UW[1], CRegName::GPR_Hi[count]);
                }
            }

            if (GetMipsRegLo(count) == 0)
            {
                if (g_System->b32BitCore())
                {
                    if (!bEdiZero)
                    {
                        m_Assembler.XorX86RegToX86Reg(CX86Ops::x86_EDI, CX86Ops::x86_EDI);
                        bEdiZero = true;
                    }
                }
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDI, &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            else if (GetMipsRegLo(count) == 0xFFFFFFFF)
            {
                if (g_System->b32BitCore())
                {
                    if (!bEsiSign)
                    {
                        m_Assembler.MoveConstToX86reg(0xFFFFFFFF, CX86Ops::x86_ESI);
                        bEsiSign = true;
                    }
                }
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_ESI, &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            else
            {
                m_Assembler.MoveConstToVariable(GetMipsRegLo(count), &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }

            SetMipsRegState(count, CX86RegInfo::STATE_UNKNOWN);
            break;
        case CX86RegInfo::STATE_CONST_32_ZERO:
            if (!g_System->b32BitCore())
            {
                if (!bEdiZero)
                {
                    m_Assembler.XorX86RegToX86Reg(CX86Ops::x86_EDI, CX86Ops::x86_EDI);
                    bEdiZero = true;
                }
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDI, &_GPR[count].UW[1], CRegName::GPR_Hi[count]);
            }

            if (GetMipsRegLo(count) == 0)
            {
                if (g_System->b32BitCore())
                {
                    if (!bEdiZero)
                    {
                        m_Assembler.XorX86RegToX86Reg(CX86Ops::x86_EDI, CX86Ops::x86_EDI);
                        bEdiZero = true;
                    }
                }
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDI, &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            else
            {
                m_Assembler.MoveConstToVariable(GetMipsRegLo(count), &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            SetMipsRegState(count, CX86RegInfo::STATE_UNKNOWN);
            break;
        case CX86RegInfo::STATE_CONST_64:
            if (GetMipsRegLo(count) == 0 || GetMipsRegHi(count) == 0)
            {
                m_Assembler.XorX86RegToX86Reg(CX86Ops::x86_EDI, CX86Ops::x86_EDI);
                bEdiZero = true;
            }
            if (GetMipsRegLo(count) == 0xFFFFFFFF || GetMipsRegHi(count) == 0xFFFFFFFF)
            {
                m_Assembler.MoveConstToX86reg(0xFFFFFFFF, CX86Ops::x86_ESI);
                bEsiSign = true;
            }

            if (GetMipsRegHi(count) == 0)
            {
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDI, &_GPR[count].UW[1], CRegName::GPR_Hi[count]);
            }
            else if (GetMipsRegLo(count) == 0xFFFFFFFF)
            {
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_ESI, &_GPR[count].UW[1], CRegName::GPR_Hi[count]);
            }
            else
            {
                m_Assembler.MoveConstToVariable(GetMipsRegHi(count), &_GPR[count].UW[1], CRegName::GPR_Hi[count]);
            }

            if (GetMipsRegLo(count) == 0)
            {
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDI, &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            else if (GetMipsRegLo(count) == 0xFFFFFFFF)
            {
                m_Assembler.MoveX86regToVariable(CX86Ops::x86_ESI, &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            else
            {
                m_Assembler.MoveConstToVariable(GetMipsRegLo(count), &_GPR[count].UW[0], CRegName::GPR_Lo[count]);
            }
            SetMipsRegState(count, CX86RegInfo::STATE_UNKNOWN);
            break;
        default:
            m_CodeBlock.Log("%s: Unknown State: %d reg %d (%s)", __FUNCTION__, GetMipsRegState(count), count, CRegName::GPR[count]);
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

#endif