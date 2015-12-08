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
#include "JumpInfo.h"
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>

class CCodeBlock;

class CCodeSection :
    private CRecompilerOps
{
public:
    typedef std::list<CCodeSection *> SECTION_LIST;

    CCodeSection(CCodeBlock * CodeBlock, uint32_t EnterPC, uint32_t ID, bool LinkAllowed);
    ~CCodeSection();

    void SetDelaySlot();
    void SetJumpAddress(uint32_t JumpPC, uint32_t TargetPC, bool PermLoop);
    void SetContinueAddress(uint32_t JumpPC, uint32_t TargetPC);
    void CompileCop1Test();
    bool GenerateX86Code(uint32_t Test);
    void GenerateSectionLinkage();
    void CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason, bool CompileNow, void(*x86Jmp)(const char * Label, uint32_t Value));
    void DetermineLoop(uint32_t Test, uint32_t Test2, uint32_t TestID);
    bool FixConstants(uint32_t Test);
    CCodeSection * ExistingSection(uint32_t Addr, uint32_t Test);
    bool SectionAccessible(uint32_t SectionId, uint32_t Test);
    bool DisplaySectionInformation(uint32_t ID, uint32_t Test);
    void DisplaySectionInformation();
    void AddParent(CCodeSection * Parent);
    void SwitchParent(CCodeSection * OldParent, CCodeSection * NewParent);

    /* Block Connection info */
    SECTION_LIST       m_ParentSection;
    CCodeBlock * const m_BlockInfo;
    const uint32_t     m_SectionID;
    const uint32_t	   m_EnterPC;
    uint32_t	       m_EndPC;
    CCodeSection     * m_ContinueSection;
    CCodeSection     * m_JumpSection;
    bool               m_EndSection;   // if this section does not link
    bool               m_LinkAllowed;  // are other sections allowed to find block to link to it
    uint32_t           m_Test;
    uint32_t           m_Test2;
    uint8_t          * m_CompiledLocation;
    bool               m_InLoop;
    bool               m_DelaySlot;

    /* Register Info */
    CRegInfo    m_RegEnter;

    /* Jump Info */
    CJumpInfo   m_Jump;
    CJumpInfo   m_Cont;

private:
    CCodeSection(void);                             // Disable default constructor
    CCodeSection(const CCodeSection&);              // Disable copy constructor
    CCodeSection& operator=(const CCodeSection&);   // Disable assignment

    void UnlinkParent(CCodeSection * Parent, bool ContinueSection);
    void InheritConstants();
    void TestRegConstantStates(CRegInfo & Base, CRegInfo & Reg);
    void SyncRegState(const CRegInfo & SyncTo);
    bool IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, uint32_t Test);
    bool ParentContinue();
    bool InheritParentInfo();
    bool SetupRegisterForLoop();
};
