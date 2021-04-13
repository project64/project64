#pragma once

#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/Mips/OpCode.h>
#include <Project64-core/N64System/N64Types.h>

class CCodeSection;
class CCodeBlock;

class LoopAnalysis
{
public:
    LoopAnalysis(CCodeBlock * CodeBlock, CCodeSection * Section);
    ~LoopAnalysis();

    bool SetupRegisterForLoop();

private:
    LoopAnalysis();
    LoopAnalysis(const LoopAnalysis&);
    LoopAnalysis& operator=(const LoopAnalysis&);

    bool SetupEnterSection(CCodeSection * Section, bool & bChanged, bool & bSkipedSection);
    bool CheckLoopRegisterUsage(CCodeSection * Section);
    bool SyncRegState(CRegInfo & RegSet, const CRegInfo& SyncReg);
    void SetJumpRegSet(CCodeSection * Section, const CRegInfo &Reg);
    void SetContinueRegSet(CCodeSection * Section, const CRegInfo &Reg);

    /********************** R4300i OpCodes: Special **********************/
    void SPECIAL_SLL();
    void SPECIAL_SRL();
    void SPECIAL_SRA();
    void SPECIAL_SLLV();
    void SPECIAL_SRLV();
    void SPECIAL_SRAV();
    void SPECIAL_JR();
    void SPECIAL_JALR();
    void SPECIAL_SYSCALL(CCodeSection * Section);
    void SPECIAL_BREAK(CCodeSection * Section);
    void SPECIAL_MFHI();
    void SPECIAL_MTHI();
    void SPECIAL_MFLO();
    void SPECIAL_MTLO();
    void SPECIAL_DSLLV();
    void SPECIAL_DSRLV();
    void SPECIAL_DSRAV();
    void SPECIAL_ADD();
    void SPECIAL_ADDU();
    void SPECIAL_SUB();
    void SPECIAL_SUBU();
    void SPECIAL_AND();
    void SPECIAL_OR();
    void SPECIAL_XOR();
    void SPECIAL_NOR();
    void SPECIAL_SLT();
    void SPECIAL_SLTU();
    void SPECIAL_DADD();
    void SPECIAL_DADDU();
    void SPECIAL_DSUB();
    void SPECIAL_DSUBU();
    void SPECIAL_DSLL();
    void SPECIAL_DSRL();
    void SPECIAL_DSRA();
    void SPECIAL_DSLL32();
    void SPECIAL_DSRL32();
    void SPECIAL_DSRA32();

    typedef std::map<int32_t, CRegInfo *> RegisterMap;

    RegisterMap    m_EnterRegisters;
    RegisterMap    m_ContinueRegisters;
    RegisterMap    m_JumpRegisters;
    CCodeSection * m_EnterSection;
    CCodeBlock   * m_BlockInfo;
    uint32_t       m_PC;
    CRegInfo       m_Reg;
    STEP_TYPE      m_NextInstruction;
    OPCODE         m_Command;
    uint32_t       m_Test;
};
