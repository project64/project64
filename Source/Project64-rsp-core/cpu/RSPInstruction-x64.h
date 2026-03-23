#pragma once

#if defined(__amd64__) || defined(_M_X64)
#include "RSPOpcode.h"
#include <stdint.h>
#include <string>

class RSPInstruction
{
public:
    enum : uint32_t
    {
        UNUSED_OPERAND = ~0u,
    };
    RSPInstruction(uint32_t Address, uint32_t Instruction);
    RSPInstruction & operator=(const RSPInstruction &);
    RSPInstruction(const RSPInstruction & e);

    uint32_t Op() const;
    uint8_t Base() const;
    uint8_t Rt() const;
    uint8_t Rd() const;
    int16_t Offset() const;
    uint8_t Rs() const;
    uint8_t Sa() const;
    uint8_t Funct() const;
    uint8_t Vt() const;
    uint8_t Vs() const;
    uint8_t Vd() const;
    uint8_t E() const;

    uint32_t Address() const;
    uint32_t ConditionalBranchTarget() const;
    uint32_t StaticCallTarget() const;
    uint32_t JumpTarget() const;
    bool ChangesControlFlow() const;
    bool DelaySlotAffectBranch() const;
    bool IsBranch() const;
    bool IsJump() const;
    bool IsJumpReturn() const;
    bool IsRegisterJump() const;
    bool IsStaticCall() const;
    bool IsConditionalBranch() const;
    bool IsNop() const;
    bool WritesGpr() const;
    bool WritesGpr(uint32_t Register) const;
    uint32_t WriteGprReg() const;
    bool ReadsGpr() const;
    bool ReadsGpr(uint32_t Register) const;
    uint32_t ReadGprReg0() const;
    uint32_t ReadGprReg1() const;
    bool WritesVector() const;
    bool WritesVector(uint32_t Register) const;
    uint32_t WriteVectorReg() const;
    bool ReadsVector() const;
    bool ReadsVector(uint32_t Register) const;
    uint32_t ReadVectorReg0() const;
    uint32_t ReadVectorReg1() const;
    bool ReadsMemory() const;
    bool WritesMemory() const;
    const char * Name() const;
    const char * Param() const;
    std::string NameAndParam() const;
    uint32_t Value() const;
    bool isJump() const;
    bool isBranch() const;
    bool ReadAccumLow() const;
    bool ReadAccumMid() const;
    bool ReadAccumHigh() const;
    bool SetAccumLow() const;
    bool SetAccumMid() const;
    bool SetAccumHigh() const;

private:
    RSPInstruction(void);

    void AnalyzeInstruction(void) const;
    void DecodeName(void) const;
    void DecodeSpecialName(void) const;
    void DecodeRegImmName(void) const;
    void DecodeCop0Name(void) const;
    void DecodeCop2Name(void) const;
    void DecodeLSC2Name(const char LoadStoreIdent) const;

    static const char * ElementSpecifier(uint32_t Element);

    uint32_t m_Address;
    RSPOpcode m_Instruction;
    mutable char m_Name[40];
    mutable char m_Param[200];
    mutable bool m_Analyzed;
    mutable bool m_isBranch;
    mutable bool m_isJump;
    mutable bool m_ReadAccumLow;
    mutable bool m_ReadAccumMid;
    mutable bool m_ReadAccumHigh;
    mutable bool m_SetAccumLow;
    mutable bool m_SetAccumMid;
    mutable bool m_SetAccumHigh;

    mutable uint32_t m_WriteGprReg;
    mutable uint32_t m_ReadGprReg0;
    mutable uint32_t m_ReadGprReg1;
    mutable uint32_t m_WriteVectorReg;
    mutable uint32_t m_ReadVectorReg0;
    mutable uint32_t m_ReadVectorReg1;
    mutable bool m_ReadsMemory;
    mutable bool m_WritesMemory;
    mutable bool m_isBreak;
    mutable bool m_InvalidOp;
};
#endif