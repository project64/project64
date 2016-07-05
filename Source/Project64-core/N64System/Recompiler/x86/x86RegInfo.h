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
#pragma once
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>

class CX86RegInfo :
    private CDebugSettings,
    private CX86Ops,
    private CSystemRegisters
{
public:
    //enums
    enum REG_STATE
    {
        STATE_UNKNOWN = 0x00,
        STATE_KNOWN_VALUE = 0x01,
        STATE_X86_MAPPED = 0x02,
        STATE_SIGN = 0x04,
        STATE_32BIT = 0x08,
        STATE_MODIFIED = 0x10,

        STATE_MAPPED_64 = (STATE_KNOWN_VALUE | STATE_X86_MAPPED), // = 3
        STATE_MAPPED_32_ZERO = (STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT), // = 11
        STATE_MAPPED_32_SIGN = (STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT | STATE_SIGN), // = 15

        STATE_CONST_32_ZERO = (STATE_KNOWN_VALUE | STATE_32BIT), // = 9
        STATE_CONST_32_SIGN = (STATE_KNOWN_VALUE | STATE_32BIT | STATE_SIGN), // = 13
        STATE_CONST_64 = (STATE_KNOWN_VALUE), // = 1
    };

    enum REG_MAPPED
    {
        NotMapped = 0,
        GPR_Mapped = 1,
        Temp_Mapped = 2,
        Stack_Mapped = 3,
    };

    enum FPU_STATE
    {
        FPU_Any = -1,
        FPU_Unknown = 0,
        FPU_Dword = 1,
        FPU_Qword = 2,
        FPU_Float = 3,
        FPU_Double = 4,
    };

    enum FPU_ROUND
    {
        RoundUnknown = -1,
        RoundDefault = 0,
        RoundTruncate = 1,
        RoundNearest = 2,
        RoundDown = 3,
        RoundUp = 4,
    };

public:
    CX86RegInfo();
    CX86RegInfo(const CX86RegInfo&);
    ~CX86RegInfo();

    CX86RegInfo& operator=(const CX86RegInfo&);

    bool operator==(const CX86RegInfo& right) const;
    bool operator!=(const CX86RegInfo& right) const;

    static REG_STATE ConstantsType(int64_t Value);

    void BeforeCallDirect(void);
    void AfterCallDirect(void);

    void   FixRoundModel(FPU_ROUND RoundMethod);
    void   ChangeFPURegFormat(int32_t Reg, FPU_STATE OldFormat, FPU_STATE NewFormat, FPU_ROUND RoundingModel);
    void   Load_FPR_ToTop(int32_t Reg, int32_t RegToLoad, FPU_STATE Format);
    bool   RegInStack(int32_t Reg, FPU_STATE Format);
    void   UnMap_AllFPRs();
    void   UnMap_FPR(int32_t Reg, bool WriteBackValue);
    x86FpuValues StackPosition(int32_t Reg);

    x86Reg FreeX86Reg();
    x86Reg Free8BitX86Reg();
    void   Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad);
    void   Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad);
    x86Reg Get_MemoryStack() const;
    x86Reg Map_MemoryStack(x86Reg Reg, bool bMapRegister, bool LoadValue = true);
    x86Reg Map_TempReg(x86Reg Reg, int32_t MipsReg, bool LoadHiWord);
    void   ProtectGPR(uint32_t Reg);
    void   UnProtectGPR(uint32_t Reg);
    void   ResetX86Protection();
    x86Reg UnMap_TempReg();
    void   UnMap_GPR(uint32_t Reg, bool WriteBackValue);
    bool   UnMap_X86reg(x86Reg Reg);
    void   WriteBackRegisters();

    bool IsKnown(int32_t Reg) const   { return ((GetMipsRegState(Reg) & STATE_KNOWN_VALUE) != 0); }
    bool IsUnknown(int32_t Reg) const { return ((GetMipsRegState(Reg) & STATE_KNOWN_VALUE) == 0); }
    bool IsModified(int32_t Reg) const { return ((GetMipsRegState(Reg) & STATE_MODIFIED) != 0); }

    bool IsMapped(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_X86_MAPPED)); }
    bool IsConst(int32_t Reg) const  { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_X86_MAPPED)) == STATE_KNOWN_VALUE); }

    bool IsSigned(int32_t Reg) const   { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_SIGN)) == (STATE_KNOWN_VALUE | STATE_SIGN)); }
    bool IsUnsigned(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_SIGN)) == STATE_KNOWN_VALUE); }

    bool Is32Bit(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT)) == (STATE_KNOWN_VALUE | STATE_32BIT)); }
    bool Is64Bit(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT)) == STATE_KNOWN_VALUE); }

    bool Is32BitMapped(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)); }
    bool Is64BitMapped(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_X86_MAPPED)); }

    REG_STATE         GetMipsRegState(int32_t Reg) const { return m_MIPS_RegState[Reg]; }
    uint64_t          GetMipsReg(int32_t Reg) const { return m_MIPS_RegVal[Reg].UDW; }
    int64_t           GetMipsReg_S(int32_t Reg) const { return m_MIPS_RegVal[Reg].DW; }
    uint32_t          GetMipsRegLo(int32_t Reg) const { return m_MIPS_RegVal[Reg].UW[0]; }
    int32_t           GetMipsRegLo_S(int32_t Reg) const { return m_MIPS_RegVal[Reg].W[0]; }
    uint32_t          GetMipsRegHi(int32_t Reg) const { return m_MIPS_RegVal[Reg].UW[1]; }
    int32_t           GetMipsRegHi_S(int32_t Reg) const { return m_MIPS_RegVal[Reg].W[1]; }
    CX86Ops::x86Reg   GetMipsRegMapLo(int32_t Reg) const { return m_RegMapLo[Reg]; }
    CX86Ops::x86Reg   GetMipsRegMapHi(int32_t Reg) const { return m_RegMapHi[Reg]; }

    uint32_t             GetX86MapOrder(x86Reg Reg) const { return m_x86reg_MapOrder[Reg]; }
    bool              GetX86Protected(x86Reg Reg) const { return m_x86reg_Protected[Reg]; }
    REG_MAPPED        GetX86Mapped(x86Reg Reg) const { return m_x86reg_MappedTo[Reg]; }

    uint32_t             GetBlockCycleCount() const { return m_CycleCount; }

    void              SetMipsReg(int32_t Reg, uint64_t Value) { m_MIPS_RegVal[Reg].UDW = Value; }
    void              SetMipsReg_S(int32_t Reg, int64_t Value)           { m_MIPS_RegVal[Reg].DW = Value; }
    void              SetMipsRegLo(int32_t Reg, uint32_t Value)            { m_MIPS_RegVal[Reg].UW[0] = Value; }
    void              SetMipsRegHi(int32_t Reg, uint32_t Value)            { m_MIPS_RegVal[Reg].UW[1] = Value; }
    void              SetMipsRegMapLo(int32_t GetMipsReg, x86Reg Reg)      { m_RegMapLo[GetMipsReg] = Reg; }
    void              SetMipsRegMapHi(int32_t GetMipsReg, x86Reg Reg)      { m_RegMapHi[GetMipsReg] = Reg; }
    void              SetMipsRegState(int32_t GetMipsReg, REG_STATE State) { m_MIPS_RegState[GetMipsReg] = State; }

    void              SetX86MapOrder(x86Reg Reg, uint32_t Order)         { m_x86reg_MapOrder[Reg] = Order; }
    void              SetX86Protected(x86Reg Reg, bool Protected)      { m_x86reg_Protected[Reg] = Protected; }
    void              SetX86Mapped(x86Reg Reg, REG_MAPPED Mapping)  { m_x86reg_MappedTo[Reg] = Mapping; }

    void  SetBlockCycleCount(uint32_t CyleCount) { m_CycleCount = CyleCount; }

    int32_t & StackTopPos() { return m_Stack_TopPos; }
    int32_t & FpuMappedTo(int32_t Reg) { return m_x86fpu_MappedTo[Reg]; }
    FPU_STATE & FpuState(int32_t Reg) { return m_x86fpu_State[Reg]; }
    FPU_ROUND & FpuRoundingModel(int32_t Reg) { return m_x86fpu_RoundingModel[Reg]; }
    bool & FpuBeenUsed() { return m_Fpu_Used; }

    FPU_ROUND GetRoundingModel() const { return m_RoundingModel; }
    void      SetRoundingModel(FPU_ROUND RoundingModel) { m_RoundingModel = RoundingModel; }

private:
    const char * RoundingModelName(FPU_ROUND RoundType);
    x86Reg UnMap_8BitTempReg();

    //r4k
    REG_STATE   m_MIPS_RegState[32];
    MIPS_DWORD  m_MIPS_RegVal[32];
    x86Reg      m_RegMapHi[32];
    x86Reg      m_RegMapLo[32];

    REG_MAPPED  m_x86reg_MappedTo[10];
    uint32_t    m_x86reg_MapOrder[10];
    bool        m_x86reg_Protected[10];

    uint32_t    m_CycleCount;

    //FPU
    int32_t     m_Stack_TopPos;
    int32_t     m_x86fpu_MappedTo[8];
    FPU_STATE   m_x86fpu_State[8];
    bool        m_x86fpu_StateChanged[8];
    FPU_ROUND   m_x86fpu_RoundingModel[8];

    bool        m_Fpu_Used;
    FPU_ROUND   m_RoundingModel;

    static uint32_t m_fpuControl;
};
