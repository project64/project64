#if defined(__amd64__) || defined(_M_X64)

#pragma once
#include <Project64-rsp-core/Recompiler/asmjit.h>
#include <stdint.h>

enum class RspFlags
{
    VCOL,
    VCOH,
    VCCL,
    VCCH,
    VCE,
    MaxFlags
};

class RspAssembler;
class CRSPRecompilerOps;

class CRspRegState
{
public:
    CRspRegState(CRSPRecompilerOps & RecompilerOps);
    ~CRspRegState();

    void ResetRegProtection();

    asmjit::x86::Xmm MapXmmZero();
    asmjit::x86::Xmm MapXmmReg(uint8_t vreg, uint8_t source, bool loadSource = true);
    asmjit::x86::Xmm MapXmmTemp(bool loadReg, uint8_t vreg, uint8_t e = 0);
    asmjit::x86::Xmm MapSpecificXmmTemp(uint8_t xmmIndex, bool loadReg, uint8_t vreg, uint8_t e = 0);
    asmjit::x86::Xmm VRegMapping(uint8_t vreg);
    void ProtectXmm(asmjit::x86::Xmm reg);
    void UnprotectXmm(asmjit::x86::Xmm reg);

    bool IsGprConst(uint8_t gprReg) const;
    uint32_t GetGprConstValue(uint8_t gprReg) const;
    void SetGprConst(uint8_t gprReg, uint32_t value);
    void SetGprUnknown(uint8_t gprReg);

    bool IsFlagZero(RspFlags flag) const;
    void SetFlagZero(RspFlags flag);
    void SetFlagUnknown(RspFlags flag);

    bool FreeXmmReg(uint32_t xmmIndex);
    bool HasMappedRegisters() const;
    void WriteBackRegisters();

private:
    CRspRegState() = delete;
    CRspRegState & operator=(const CRspRegState &) = delete;

    enum class XmmState
    {
        Free,
        Zero,
        Mapped,
        Temp,
        Reserved
    };

    CRSPRecompilerOps & m_RecompilerOps;
    RspAssembler *& m_Assembler;
    XmmState m_XmmState[16];
    uint8_t m_XmmRegMapped[16];
    bool m_XmmProtected[16];
    bool m_GprIsConst[32];
    uint32_t m_GprConstValue[32];
    bool m_FlagIsZero[static_cast<size_t>(RspFlags::MaxFlags)];
};

#endif