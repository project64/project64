#pragma once
#include <Project64-core/N64System/N64Types.h>

class CRegBase
{
public:
    CRegBase();

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

    enum FPU_ROUND
    {
        RoundUnknown = -1,
        RoundDefault = 0,
        RoundTruncate = 1,
        RoundNearest = 2,
        RoundDown = 3,
        RoundUp = 4,
    };

    inline void SetMipsRegState(int32_t GetMipsReg, REG_STATE State) { m_MIPS_RegState[GetMipsReg] = State; }
    inline REG_STATE GetMipsRegState(int32_t Reg) const { return m_MIPS_RegState[Reg]; }

    inline bool IsKnown(int32_t Reg) const { return ((GetMipsRegState(Reg) & STATE_KNOWN_VALUE) != 0); }
    inline bool IsUnknown(int32_t Reg) const { return ((GetMipsRegState(Reg) & STATE_KNOWN_VALUE) == 0); }
    inline bool IsModified(int32_t Reg) const { return ((GetMipsRegState(Reg) & STATE_MODIFIED) != 0); }

    inline bool IsMapped(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_X86_MAPPED)); }
    inline bool IsConst(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_X86_MAPPED)) == STATE_KNOWN_VALUE); }

    inline bool IsSigned(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_SIGN)) == (STATE_KNOWN_VALUE | STATE_SIGN)); }
    inline bool IsUnsigned(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_SIGN)) == STATE_KNOWN_VALUE); }

    inline bool Is32Bit(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT)) == (STATE_KNOWN_VALUE | STATE_32BIT)); }
    inline bool Is64Bit(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT)) == STATE_KNOWN_VALUE); }

    inline bool Is32BitMapped(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)); }
    inline bool Is64BitMapped(int32_t Reg) const { return ((GetMipsRegState(Reg) & (STATE_KNOWN_VALUE | STATE_32BIT | STATE_X86_MAPPED)) == (STATE_KNOWN_VALUE | STATE_X86_MAPPED)); }

    inline uint64_t GetMipsReg(int32_t Reg) const { return m_MIPS_RegVal[Reg].UDW; }
    inline int64_t GetMipsReg_S(int32_t Reg) const { return m_MIPS_RegVal[Reg].DW; }
    inline uint32_t GetMipsRegLo(int32_t Reg) const { return m_MIPS_RegVal[Reg].UW[0]; }
    inline int32_t GetMipsRegLo_S(int32_t Reg) const { return m_MIPS_RegVal[Reg].W[0]; }
    inline uint32_t GetMipsRegHi(int32_t Reg) const { return m_MIPS_RegVal[Reg].UW[1]; }
    inline int32_t GetMipsRegHi_S(int32_t Reg) const { return m_MIPS_RegVal[Reg].W[1]; }

    inline void SetMipsRegLo(int32_t Reg, uint32_t Value) { m_MIPS_RegVal[Reg].UW[0] = Value; }
    inline void SetMipsRegHi(int32_t Reg, uint32_t Value) { m_MIPS_RegVal[Reg].UW[1] = Value; }
    inline void SetMipsReg(int32_t Reg, uint64_t Value) { m_MIPS_RegVal[Reg].UDW = Value; }
    inline void SetMipsReg_S(int32_t Reg, int64_t Value) { m_MIPS_RegVal[Reg].DW = Value; }

    inline void SetBlockCycleCount(uint32_t CyleCount) { m_CycleCount = CyleCount; }
    inline uint32_t GetBlockCycleCount() const { return m_CycleCount; }

    inline void SetFpuBeenUsed(bool FpuUsed) { m_Fpu_Used = FpuUsed; }
    inline bool GetFpuBeenUsed() const { return m_Fpu_Used; }

    inline FPU_ROUND GetRoundingModel() const { return m_RoundingModel; }
    inline void SetRoundingModel(FPU_ROUND RoundingModel) { m_RoundingModel = RoundingModel; }

    CRegBase& operator=(const CRegBase&);

    bool operator==(const CRegBase& right) const;
    bool operator!=(const CRegBase& right) const;

protected:
    const char * RoundingModelName(FPU_ROUND RoundType);

    REG_STATE m_MIPS_RegState[32];
    MIPS_DWORD m_MIPS_RegVal[32];
    uint32_t m_CycleCount;
    bool m_Fpu_Used;
    FPU_ROUND m_RoundingModel;
};
