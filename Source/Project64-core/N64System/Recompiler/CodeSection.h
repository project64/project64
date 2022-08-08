#pragma once
#include "JumpInfo.h"
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>
#include <Project64-core/Settings/DebugSettings.h>

class CCodeBlock;

class CCodeSection :
    private CDebugSettings
{
public:
    typedef std::list<CCodeSection *> SECTION_LIST;

    CCodeSection(CCodeBlock & CodeBlock, uint32_t EnterPC, uint32_t ID, bool LinkAllowed);
    ~CCodeSection();

    void SetDelaySlot();
    void SetJumpAddress(uint32_t JumpPC, uint32_t TargetPC, bool PermLoop);
    void SetContinueAddress(uint32_t JumpPC, uint32_t TargetPC);
    bool GenerateNativeCode(uint32_t Test);
    void GenerateSectionLinkage();
    void DetermineLoop(uint32_t Test, uint32_t Test2, uint32_t TestID);
    CCodeSection * ExistingSection(uint32_t Addr, uint32_t Test);
    bool SectionAccessible(uint32_t SectionId, uint32_t Test);
    bool DisplaySectionInformation(uint32_t ID, uint32_t Test);
    void DisplaySectionInformation();
    void AddParent(CCodeSection * Parent);
    void SwitchParent(CCodeSection * OldParent, CCodeSection * NewParent);

    // Block connection info
    SECTION_LIST m_ParentSection;
    CCodeBlock & m_CodeBlock;
    const uint32_t m_SectionID;
    const uint32_t m_EnterPC;
    uint32_t m_EndPC;
    CCodeSection * m_ContinueSection;
    CCodeSection * m_JumpSection;
    bool m_EndSection;   // If this section does not link, are other sections allowed to find block to link to it?
    bool m_LinkAllowed;
    uint32_t m_Test;
    uint32_t m_Test2;
    uint8_t * m_CompiledLocation;
    bool m_InLoop;
    bool m_DelaySlot;
    CRecompilerOps * & m_RecompilerOps;

    // Register info
    CRegInfo m_RegEnter;

    // Jump info
    CJumpInfo m_Jump;
    CJumpInfo m_Cont;

private:
    CCodeSection(void);
    CCodeSection(const CCodeSection&);
    CCodeSection& operator=(const CCodeSection&);

    void UnlinkParent(CCodeSection * Parent, bool ContinueSection);
    void InheritConstants();
    void TestRegConstantStates(CRegInfo & Base, CRegInfo & Reg);
    bool IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, uint32_t Test);
    bool ParentContinue();
    bool SetupRegisterForLoop();
};
