#pragma once

#if defined(__amd64__) || defined(_M_X64)
#include <Project64-core/N64System/Recompiler/RegBase.h>
#include <Project64-core/N64System/Recompiler/asmjit.h>

class CCodeBlock;
class CX64Ops;
class CRegisters;

class CX64RegInfo :
    public CRegBase
{
    enum
    {
        x64PhysRegCount = asmjit::x86::Gp::kIdR15 + 1
    };

public:
    enum REG_MAPPED
    {
        NotMapped,
        GPR_Mapped,
        Temp_Mapped32,
        Temp_Mapped64,
    };

    static bool IsTempMapped(REG_MAPPED Mapping);

    CX64RegInfo(CCodeBlock & CodeBlock, CX64Ops & Assembler);
    CX64RegInfo(const CX64RegInfo &);
    ~CX64RegInfo();

    CX64RegInfo & operator=(const CX64RegInfo &);

    bool operator==(const CX64RegInfo & right) const;
    bool operator!=(const CX64RegInfo & right) const;

    CX64RegInfo WithAddedCycles(uint32_t Cycles) const;

    void ResetRegisterProtection();
    void BeforeCallDirect(void);
    void AfterCallDirect(void);
    void Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad);
    void Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad);
    asmjit::x86::Gp Map_TempReg(asmjit::x86::Gp Reg, int32_t MipsReg, asmjit::RegType RegType = asmjit::RegType::kX86_Gpd);
    void ProtectGPR(uint32_t MipsReg);
    const asmjit::x86::Gp & GetMipsRegMap(int32_t Reg) const;
    void SetMipsRegMap(int32_t MipsReg, const asmjit::x86::Gp & Reg);

    bool GetX64Protected(uint32_t PhysId) const;
    REG_MAPPED GetX64Mapped(uint32_t PhysId) const;
    void SetX64MapOrder(uint32_t PhysId, uint32_t Order);
    void SetX64Protected(uint32_t PhysId, bool Protected);
    void SetX64Mapped(uint32_t PhysId, REG_MAPPED Mapping);

    void UnMap_GPR(uint32_t Reg, bool WriteBackValue);
    void WriteBackRegisters();

private:
    CX64RegInfo() = delete;

    asmjit::x86::Gp FreeX64Reg(asmjit::RegType RegType);
    bool UnMap_X64reg(const asmjit::x86::Gp & Reg);

    CRegisters & m_Reg;
    CCodeBlock & m_CodeBlock;
    CX64Ops & m_Assembler;
    asmjit::x86::Gp m_RegMap[32];
    REG_MAPPED m_x64reg_MappedTo[x64PhysRegCount];
    uint32_t m_x64reg_MapOrder[x64PhysRegCount];
    bool m_x64reg_Protected[x64PhysRegCount];
    bool m_InBeforeCallDirect;
    uint32_t m_CallDirectPushBase;
    uint32_t m_CallDirectPushCount;
    uint32_t m_CallDirectPushIds[7];
};

#endif
