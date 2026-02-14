#pragma once
#include <Project64-rsp-core/cpu/RSPInstruction-x64.h>
#include <Project64-rsp-core/cpu/RSPInstruction-x86.h>
#include <memory>
#include <set>
#include <stdint.h>
#include <unordered_map>
#include <vector>

class CRSPSystem;

enum RspCodeType
{
    RspCodeType_TASK,
    RspCodeType_SUBROUTINE,
};

class RspCodeBlock;
typedef std::unique_ptr<RspCodeBlock> RspCodeBlockPtr;
typedef std::unordered_map<uint32_t, RspCodeBlockPtr> RspCodeBlocks;
typedef std::vector<RSPInstruction> RSPInstructions;

class RspCodeBlock
{
    typedef std::unordered_map<uint32_t, size_t> InstructionIndexMap;

public:
    typedef std::set<uint32_t> Addresses;

    RspCodeBlock(CRSPSystem & System, uint32_t StartAddress, RspCodeType type, uint32_t DispatchAddress, RspCodeBlocks & Functions);

    const Addresses & GetBranchTargets() const;
    void * GetCompiledLocation() const;
    uint32_t GetDispatchAddress() const;
    const Addresses & GetFunctionCalls() const;
    const RSPInstructions & GetInstructions() const;
    const RspCodeBlock * GetFunctionBlock(uint32_t Address) const;
    uint32_t GetStartAddress() const;
    size_t InstructionIndex(uint32_t pc) const;
    void SetCompiledLocation(void * CompiledLoction);
    RspCodeType CodeType() const;
    bool IsEnd(uint32_t Address) const;
    bool IsValid() const;

private:
    RspCodeBlock();
    RspCodeBlock(const RspCodeBlock &);
    RspCodeBlock & operator=(const RspCodeBlock &);

    void Analyze();
    bool IsAddressInInstructions(uint32_t address) const;
    void BuildInstructionIndex();

    RspCodeBlocks & m_Functions;
    const uint32_t m_DispatchAddress;
    RSPInstructions m_Instructions;
    InstructionIndexMap m_InstructionIndex;
    uint32_t m_StartAddress;
    RspCodeType m_CodeType;
    CRSPSystem & m_System;
    Addresses m_End;
    Addresses m_BranchTargets;
    Addresses m_FunctionCalls;
    void * m_CompiledLoction;
    bool m_Valid;
};