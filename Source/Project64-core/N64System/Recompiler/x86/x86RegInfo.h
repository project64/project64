#pragma once

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Recompiler/RegBase.h>
#include <Project64-core/N64System/Recompiler/asmjit.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>
#include <Project64-core/Settings/DebugSettings.h>

enum x86RegIndex
{
    x86RegIndex_EAX,
    x86RegIndex_ECX,
    x86RegIndex_EDX,
    x86RegIndex_EBX,
    x86RegIndex_ESP,
    x86RegIndex_EBP,
    x86RegIndex_ESI,
    x86RegIndex_EDI,
    x86RegIndex_Size,
};

x86RegIndex GetIndexFromX86Reg(const asmjit::x86::Gp & Reg);
asmjit::x86::Gp GetX86RegFromIndex(x86RegIndex Index);

enum x86RegFpuIndex
{
    x86RegFpuIndex_ST0,
    x86RegFpuIndex_ST1,
    x86RegFpuIndex_ST2,
    x86RegFpuIndex_ST3,
    x86RegFpuIndex_ST4,
    x86RegFpuIndex_ST5,
    x86RegFpuIndex_ST6,
    x86RegFpuIndex_ST7,
    x86RegFpuIndex_Size,
};

x86RegFpuIndex GetIndexFromX86FpuReg(const asmjit::x86::St & Reg);
asmjit::x86::St GetX86FpuRegFromIndex(x86RegFpuIndex Index);

class CX86RegInfo :
    public CRegBase,
    private CDebugSettings
{
public:
    // Enums
    enum REG_MAPPED
    {
        NotMapped = 0,
        GPR_Mapped = 1,
        Temp_Mapped = 2,
        Stack_Mapped = 3,
        FPStatusReg_Mapped = 4,
    };

    enum FPU_STATE
    {
        FPU_Any = -1,
        FPU_Unknown = 0,
        FPU_Dword = 1,
        FPU_Qword = 2,
        FPU_Float = 3,
        FPU_FloatLow = 4,
        FPU_Double = 5,
        FPU_UnsignedDoubleWord = 6,
    };

    CX86RegInfo(CCodeBlock & CodeBlock, CX86Ops & Assembler);
    CX86RegInfo(const CX86RegInfo &);
    ~CX86RegInfo();

    CX86RegInfo & operator=(const CX86RegInfo &);

    bool operator==(const CX86RegInfo & right) const;
    bool operator!=(const CX86RegInfo & right) const;

    static REG_STATE ConstantsType(int64_t Value);

    void BeforeCallDirect(void);
    void AfterCallDirect(void);

    void FixRoundModel(FPU_ROUND RoundMethod);
    asmjit::x86::Gp FPRValuePointer(int32_t Reg, FPU_STATE Format);
    void PrepareFPTopToBe(int32_t Reg, int32_t RegToLoad, FPU_STATE Format);
    void SetFPTopAs(int32_t Reg);
    bool RegInStack(int32_t Reg, FPU_STATE Format);
    void UnMap_AllFPRs();
    void UnMap_FPR(int32_t Reg, bool WriteBackValue);
    const asmjit::x86::St & StackPosition(int32_t Reg);

    bool IsFPStatusRegMapped();
    asmjit::x86::Gp FreeX86Reg();
    asmjit::x86::Gp Free8BitX86Reg();
    void Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad);
    void Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad);
    asmjit::x86::Gp Map_FPStatusReg();
    asmjit::x86::Gp Get_MemoryStack() const;
    asmjit::x86::Gp Map_MemoryStack(asmjit::x86::Gp Reg, bool bMapRegister, bool LoadValue = true);
    asmjit::x86::Gp Map_TempReg(asmjit::x86::Gp Reg, int32_t MipsReg, bool LoadHiWord, bool Reg8Bit);
    void ProtectGPR(uint32_t MipsReg);
    void UnProtectGPR(uint32_t MipsReg);
    void ResetX86Protection();
    asmjit::x86::Gp UnMap_TempReg();
    void UnMap_FPStatusReg();
    void UnMap_GPR(uint32_t Reg, bool WriteBackValue);
    bool UnMap_X86reg(const asmjit::x86::Gp & Reg);
    void WriteBackRegisters();

    asmjit::x86::Gp GetMipsRegMapLo(int32_t Reg) const
    {
        return m_RegMapLo[Reg];
    }
    asmjit::x86::Gp GetMipsRegMapHi(int32_t Reg) const
    {
        return m_RegMapHi[Reg];
    }

    uint32_t GetX86MapOrder(x86RegIndex Reg) const
    {
        return m_x86reg_MapOrder[Reg];
    }
    bool GetX86Protected(x86RegIndex Reg) const
    {
        return m_x86reg_Protected[Reg];
    }
    REG_MAPPED GetX86Mapped(x86RegIndex Reg) const
    {
        return m_x86reg_MappedTo[Reg];
    }

    void SetMipsRegMapLo(int32_t GetMipsReg, const asmjit::x86::Gp & Reg)
    {
        m_RegMapLo[GetMipsReg] = Reg;
    }
    void SetMipsRegMapHi(int32_t GetMipsReg, const asmjit::x86::Gp & Reg)
    {
        m_RegMapHi[GetMipsReg] = Reg;
    }

    void SetX86MapOrder(x86RegIndex Reg, uint32_t Order)
    {
        m_x86reg_MapOrder[Reg] = Order;
    }
    void SetX86Protected(x86RegIndex Reg, bool Protected)
    {
        m_x86reg_Protected[Reg] = Protected;
    }
    void SetX86Mapped(x86RegIndex Reg, REG_MAPPED Mapping)
    {
        m_x86reg_MappedTo[Reg] = Mapping;
    }

    int32_t & StackTopPos()
    {
        return m_Stack_TopPos;
    }
    int32_t & FpuMappedTo(int32_t Reg)
    {
        return m_x86fpu_MappedTo[Reg];
    }
    FPU_STATE & FpuState(int32_t Reg)
    {
        return m_x86fpu_State[Reg];
    }
    bool & FpuStateChanged(int32_t Reg)
    {
        return m_x86fpu_StateChanged[Reg];
    }
    FPU_ROUND & FpuRoundingModel(int32_t Reg)
    {
        return m_x86fpu_RoundingModel[Reg];
    }

private:
    CX86RegInfo();

    CRegisters & m_Reg;
    CCodeBlock & m_CodeBlock;
    CX86Ops & m_Assembler;
    asmjit::x86::Gp UnMap_8BitTempReg();

    // r4k
    asmjit::x86::Gp m_RegMapHi[32];
    asmjit::x86::Gp m_RegMapLo[32];

    REG_MAPPED m_x86reg_MappedTo[x86RegIndex_Size];
    uint32_t m_x86reg_MapOrder[x86RegIndex_Size];
    bool m_x86reg_Protected[x86RegIndex_Size];

    // FPU
    int32_t m_Stack_TopPos;
    int32_t m_x86fpu_MappedTo[x86RegFpuIndex_Size];
    FPU_STATE m_x86fpu_State[x86RegFpuIndex_Size];
    bool m_x86fpu_StateChanged[x86RegFpuIndex_Size];
    FPU_ROUND m_x86fpu_RoundingModel[x86RegFpuIndex_Size];

    static uint32_t m_fpuControl;
    bool m_InBeforeCallDirect;
};
#endif
