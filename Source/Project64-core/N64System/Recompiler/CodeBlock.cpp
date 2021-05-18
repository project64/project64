#include "stdafx.h"
#include <string.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Recompiler/x86/x86RecompilerOps.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmRecompilerOps.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/TranslateVaddr.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

#if defined(ANDROID) && (defined(__arm__) || defined(_M_ARM))
/* bug-fix to implement __clear_cache (missing in Android; http://code.google.com/p/android/issues/detail?id=1803) */
extern "C" void __clear_cache_android(uint8_t* begin, uint8_t *end);
#endif

CCodeBlock::CCodeBlock(uint32_t VAddrEnter, uint8_t * CompiledLocation) :
m_VAddrEnter(VAddrEnter),
m_VAddrFirst(VAddrEnter),
m_VAddrLast(VAddrEnter),
m_CompiledLocation(CompiledLocation),
m_EnterSection(nullptr),
m_RecompilerOps(nullptr),
m_Test(1)
{
#if defined(__arm__) || defined(_M_ARM)
    // Make sure function starts at an odd address so that the system knows it is in thumb mode
    if (((uint32_t)m_CompiledLocation % 2) == 0)
    {
        m_CompiledLocation+=1;
    }
#endif
#if defined(__i386__) || defined(_M_IX86)
    m_RecompilerOps = new CX86RecompilerOps;
#elif defined(__arm__) || defined(_M_ARM)
    m_RecompilerOps = new CArmRecompilerOps;
#endif
    if (m_RecompilerOps == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    CCodeSection * baseSection = new CCodeSection(this, VAddrEnter, 0, false);
    if (baseSection == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    m_Sections.push_back(baseSection);
    baseSection->AddParent(nullptr);
    baseSection->m_CompiledLocation = (uint8_t *)-1;
    baseSection->m_Cont.JumpPC = VAddrEnter;
    baseSection->m_Cont.FallThrough = true;
    baseSection->m_Cont.RegSet = baseSection->m_RegEnter;

    m_EnterSection = new CCodeSection(this, VAddrEnter, 1, true);
    if (m_EnterSection == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    baseSection->m_ContinueSection = m_EnterSection;
    m_EnterSection->AddParent(baseSection);
    m_Sections.push_back(m_EnterSection);
    m_SectionMap.insert(SectionMap::value_type(VAddrEnter, m_EnterSection));

    if (g_TransVaddr->VAddrToRealAddr(VAddrEnter, *(reinterpret_cast<void **>(&m_MemLocation[0]))))
    {
        m_MemLocation[1] = m_MemLocation[0] + 1;
        m_MemContents[0] = *m_MemLocation[0];
        m_MemContents[1] = *m_MemLocation[1];
    }
    else
    {
        memset(m_MemLocation, 0, sizeof(m_MemLocation));
        memset(m_MemContents, 0, sizeof(m_MemContents));
    }

    AnalyseBlock();
}

CCodeBlock::~CCodeBlock()
{
    for (SectionList::iterator itr = m_Sections.begin(); itr != m_Sections.end(); itr++)
    {
        CCodeSection * Section = *itr;
        delete Section;
    }
    m_Sections.clear();

    if (m_RecompilerOps != nullptr)
    {
#if defined(__i386__) || defined(_M_IX86)
        delete (CX86RecompilerOps *)m_RecompilerOps;
#endif
        m_RecompilerOps = nullptr;
    }
}

bool CCodeBlock::SetSection(CCodeSection * & Section, CCodeSection * CurrentSection, uint32_t TargetPC, bool LinkAllowed, uint32_t CurrentPC)
{
    if (Section != nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (TargetPC >= ((CurrentPC + 0x1000) & 0xFFFFF000))
    {
        return false;
    }

    if (TargetPC < m_EnterSection->m_EnterPC)
    {
        return false;
    }

    if (LinkAllowed)
    {
        if (Section != nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        SectionMap::const_iterator itr = m_SectionMap.find(TargetPC);
        if (itr != m_SectionMap.end())
        {
            Section = itr->second;
            Section->AddParent(CurrentSection);
        }
    }

    if (Section == nullptr)
    {
        Section = new CCodeSection(this, TargetPC, m_Sections.size(), LinkAllowed);
        if (Section == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return false;
        }
        m_Sections.push_back(Section);
        if (LinkAllowed)
        {
            m_SectionMap.insert(SectionMap::value_type(TargetPC, Section));
        }
        Section->AddParent(CurrentSection);
        if (TargetPC <= CurrentPC && TargetPC != m_VAddrEnter)
        {
            CCodeSection * SplitSection = nullptr;
            for (SectionMap::const_iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
            {
                if (itr->first >= TargetPC)
                {
                    break;
                }
                SplitSection = itr->second;
            }
            if (SplitSection == nullptr)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (SplitSection->m_EndPC == (uint32_t)-1)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (SplitSection->m_EndPC >= TargetPC)
            {
                CPU_Message("%s: Split Section: %d with section: %d", __FUNCTION__, SplitSection->m_SectionID, Section->m_SectionID);
                CCodeSection * BaseSection = Section;
                BaseSection->m_EndPC = SplitSection->m_EndPC;
                BaseSection->SetJumpAddress(SplitSection->m_Jump.JumpPC, SplitSection->m_Jump.TargetPC, SplitSection->m_Jump.PermLoop);
                BaseSection->m_JumpSection = SplitSection->m_JumpSection;
                BaseSection->SetContinueAddress(SplitSection->m_Cont.JumpPC, SplitSection->m_Cont.TargetPC);
                BaseSection->m_ContinueSection = SplitSection->m_ContinueSection;
                if (BaseSection->m_JumpSection)
                {
                    BaseSection->m_JumpSection->SwitchParent(SplitSection, BaseSection);
                }
                if (BaseSection->m_ContinueSection)
                {
                    BaseSection->m_ContinueSection->SwitchParent(SplitSection, BaseSection);
                }
                BaseSection->AddParent(SplitSection);

                SplitSection->m_EndPC = TargetPC - 4;
                SplitSection->m_JumpSection = nullptr;
                SplitSection->m_ContinueSection = BaseSection;
                SplitSection->SetContinueAddress(TargetPC - 4, TargetPC);
                SplitSection->SetJumpAddress((uint32_t)-1, (uint32_t)-1, false);
            }
        }
    }
    return true;
}

bool CCodeBlock::CreateBlockLinkage(CCodeSection * EnterSection)
{
    CCodeSection * CurrentSection = EnterSection;

    CPU_Message("Section %d", CurrentSection->m_SectionID);
    for (uint32_t TestPC = EnterSection->m_EnterPC, EndPC = ((EnterSection->m_EnterPC + 0x1000) & 0xFFFFF000); TestPC <= EndPC; TestPC += 4)
    {
        if (TestPC != EndPC)
        {
            SectionMap::const_iterator itr = m_SectionMap.find(TestPC);
            if (itr != m_SectionMap.end() && CurrentSection != itr->second)
            {
                if (CurrentSection->m_ContinueSection != nullptr &&
                    CurrentSection->m_ContinueSection != itr->second)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                if (CurrentSection->m_ContinueSection == nullptr)
                {
                    SetSection(CurrentSection->m_ContinueSection, CurrentSection, TestPC, true, TestPC);
                    CurrentSection->SetContinueAddress(TestPC - 4, TestPC);
                }
                CurrentSection->m_EndPC = TestPC - 4;
                CurrentSection = itr->second;

                CPU_Message("Section %d", CurrentSection->m_SectionID);
                if (EnterSection != m_EnterSection)
                {
                    if (CurrentSection->m_JumpSection != nullptr ||
                        CurrentSection->m_ContinueSection != nullptr ||
                        CurrentSection->m_EndSection)
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            CurrentSection->m_EndSection = true;
            break;
        }

        bool LikelyBranch, EndBlock, IncludeDelaySlot, PermLoop;
        uint32_t TargetPC, ContinuePC;

        CurrentSection->m_EndPC = TestPC;
        if (!AnalyzeInstruction(TestPC, TargetPC, ContinuePC, LikelyBranch, IncludeDelaySlot, EndBlock, PermLoop))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return false;
        }

        if (TestPC + 4 == EndPC && IncludeDelaySlot)
        {
            TargetPC = (uint32_t)-1;
            ContinuePC = (uint32_t)-1;
            EndBlock = true;
        }
        if (TargetPC == (uint32_t)-1 && !EndBlock)
        {
            if (ContinuePC != (uint32_t)-1)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            continue;
        }

        if (EndBlock)
        {
            CPU_Message("%s: End Block", __FUNCTION__);
            CurrentSection->m_EndSection = true;
            // Find other sections that need compiling
            break;
        }

        if (ContinuePC != (uint32_t)-1)
        {
            CPU_Message("%s: SetContinueAddress TestPC = %X ContinuePC = %X", __FUNCTION__, TestPC, ContinuePC);
            CurrentSection->SetContinueAddress(TestPC, ContinuePC);
            if (!SetSection(CurrentSection->m_ContinueSection, CurrentSection, ContinuePC, true, TestPC))
            {
                ContinuePC = (uint32_t)-1;
            }
        }

        if (LikelyBranch)
        {
            CPU_Message("%s: SetJumpAddress TestPC = %X Target = %X", __FUNCTION__, TestPC, TestPC + 4);
            CurrentSection->SetJumpAddress(TestPC, TestPC + 4, false);
            if (SetSection(CurrentSection->m_JumpSection, CurrentSection, TestPC + 4, false, TestPC))
            {
                bool BranchLikelyBranch, BranchEndBlock, BranchIncludeDelaySlot, BranchPermLoop;
                uint32_t BranchTargetPC, BranchContinuePC;

                CCodeSection * JumpSection = CurrentSection->m_JumpSection;
                if (!AnalyzeInstruction(JumpSection->m_EnterPC, BranchTargetPC, BranchContinuePC, BranchLikelyBranch, BranchIncludeDelaySlot, BranchEndBlock, BranchPermLoop))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    return false;
                }

                if (BranchLikelyBranch || BranchIncludeDelaySlot || BranchPermLoop)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    return false;
                }

                JumpSection->m_EndPC = TestPC + 4;
                if (BranchEndBlock)
                {
                    CPU_Message("%s: Jump End Block", __FUNCTION__);
                    JumpSection->m_EndSection = true;
                    TargetPC = (uint32_t)-1;
                }
                else
                {
                    JumpSection->SetJumpAddress(TestPC, TargetPC, false);
                }
                JumpSection->SetDelaySlot();
                SetSection(JumpSection->m_JumpSection, JumpSection, TargetPC, true, TestPC);
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        else if (TargetPC != ((uint32_t)-1))
        {
            CPU_Message("%s: SetJumpAddress TestPC = %X Target = %X", __FUNCTION__, TestPC, TargetPC);
            CurrentSection->SetJumpAddress(TestPC, TargetPC, PermLoop);
            if (PermLoop || !SetSection(CurrentSection->m_JumpSection, CurrentSection, TargetPC, true, TestPC))
            {
                if (ContinuePC == (uint32_t)-1)
                {
                    CurrentSection->m_EndSection = true;
                }
            }
        }

        TestPC += IncludeDelaySlot ? 8 : 4;

        // Find the next section
        CCodeSection * NewSection = nullptr;
        for (SectionMap::const_iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
        {
            if (CurrentSection->m_JumpSection != nullptr ||
                CurrentSection->m_ContinueSection != nullptr ||
                CurrentSection->m_EndSection)
            {
                continue;
            }
            NewSection = itr->second;
            break;
        }
        if (NewSection == nullptr)
        {
            break;
        }
        if (CurrentSection == NewSection)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        CurrentSection = NewSection;
        if (CurrentSection->m_JumpSection != nullptr ||
            CurrentSection->m_ContinueSection != nullptr ||
            CurrentSection->m_EndSection)
        {
            break;
        }
        TestPC = CurrentSection->m_EnterPC;
        CPU_Message("a. Section %d", CurrentSection->m_SectionID);
        TestPC -= 4;
    }

    for (SectionMap::iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
    {
        CCodeSection * Section = itr->second;
        if (Section->m_JumpSection != nullptr ||
            Section->m_ContinueSection != nullptr ||
            Section->m_EndSection)
        {
            continue;
        }
        if (!CreateBlockLinkage(Section))
        {
            return false;
        }
        break;
    }
    if (CurrentSection->m_EndPC == (uint32_t)-1)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return true;
}

void CCodeBlock::DetermineLoops()
{
    for (SectionMap::iterator itr = m_SectionMap.begin(); itr != m_SectionMap.end(); itr++)
    {
        CCodeSection * Section = itr->second;

        uint32_t Test = NextTest();
        if (Section)
        {
            Section->DetermineLoop(Test, Test, Section->m_SectionID);
        }
    }
}

void CCodeBlock::LogSectionInfo()
{
    for (SectionList::iterator itr = m_Sections.begin(); itr != m_Sections.end(); itr++)
    {
        CCodeSection * Section = *itr;
        Section->DisplaySectionInformation();
    }
}

bool CCodeBlock::AnalyseBlock()
{
    if (!g_System->bLinkBlocks())
    {
        return true;
    }
    if (!CreateBlockLinkage(m_EnterSection))
    {
        return false;
    }
    DetermineLoops();
    LogSectionInfo();
    return true;
}

bool CCodeBlock::AnalyzeInstruction(uint32_t PC, uint32_t & TargetPC, uint32_t & ContinuePC, bool & LikelyBranch, bool & IncludeDelaySlot, bool & EndBlock, bool & PermLoop)
{
    TargetPC = (uint32_t)-1;
    ContinuePC = (uint32_t)-1;
    LikelyBranch = false;
    IncludeDelaySlot = false;
    EndBlock = false;
    PermLoop = false;

    OPCODE Command;
    if (!g_MMU->LW_VAddr(PC, Command.Hex))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }

#ifdef _DEBUG
    const char * Name = R4300iOpcodeName(Command.Hex, PC);
    CPU_Message("  0x%08X %s", PC, Name);
#endif
    switch (Command.op)
    {
    case R4300i_SPECIAL:
        switch (Command.funct)
        {
        case R4300i_SPECIAL_SLL:    case R4300i_SPECIAL_SRL:    case R4300i_SPECIAL_SRA:
        case R4300i_SPECIAL_SLLV:   case R4300i_SPECIAL_SRLV:   case R4300i_SPECIAL_SRAV:
        case R4300i_SPECIAL_MFHI:   case R4300i_SPECIAL_MTHI:   case R4300i_SPECIAL_MFLO:
        case R4300i_SPECIAL_MTLO:   case R4300i_SPECIAL_DSLLV:  case R4300i_SPECIAL_DSRLV:
        case R4300i_SPECIAL_DSRAV:  case R4300i_SPECIAL_ADD:    case R4300i_SPECIAL_ADDU:
        case R4300i_SPECIAL_SUB:    case R4300i_SPECIAL_SUBU:   case R4300i_SPECIAL_AND:
        case R4300i_SPECIAL_OR:     case R4300i_SPECIAL_XOR:    case R4300i_SPECIAL_NOR:
        case R4300i_SPECIAL_SLT:    case R4300i_SPECIAL_SLTU:   case R4300i_SPECIAL_DADD:
        case R4300i_SPECIAL_DADDU:  case R4300i_SPECIAL_DSUB:   case R4300i_SPECIAL_DSUBU:
        case R4300i_SPECIAL_DSLL:   case R4300i_SPECIAL_DSRL:   case R4300i_SPECIAL_DSRA:
        case R4300i_SPECIAL_DSLL32: case R4300i_SPECIAL_DSRL32: case R4300i_SPECIAL_DSRA32:
        case R4300i_SPECIAL_MULT:   case R4300i_SPECIAL_MULTU:  case R4300i_SPECIAL_DIV:
        case R4300i_SPECIAL_DIVU:   case R4300i_SPECIAL_DMULT:  case R4300i_SPECIAL_DMULTU:
        case R4300i_SPECIAL_DDIV:   case R4300i_SPECIAL_DDIVU:  case R4300i_SPECIAL_TEQ:
        case R4300i_SPECIAL_TNE:    case R4300i_SPECIAL_TGE:    case R4300i_SPECIAL_TGEU:
        case R4300i_SPECIAL_TLT:    case R4300i_SPECIAL_TLTU:
            break;
        case R4300i_SPECIAL_JALR:
        case R4300i_SPECIAL_JR:
            EndBlock = true;
            IncludeDelaySlot = true;
            break;
        case R4300i_SPECIAL_SYSCALL:
        case R4300i_SPECIAL_BREAK:
            EndBlock = true;
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return false;
        }
        break;
    case R4300i_REGIMM:
        switch (Command.rt)
        {
        case R4300i_REGIMM_BLTZ:
        case R4300i_REGIMM_BLTZAL:
            TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
            if (TargetPC == PC + 8)
            {
                TargetPC = (uint32_t)-1;
            }
            else
            {
                if (TargetPC == PC && !DelaySlotEffectsCompare(PC, Command.rs, 0))
                {
                    PermLoop = true;
                }
                ContinuePC = PC + 8;
                IncludeDelaySlot = true;
            }
            break;
        case R4300i_REGIMM_BGEZ:
        case R4300i_REGIMM_BGEZAL:
            TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
            if (TargetPC == PC + 8)
            {
                TargetPC = (uint32_t)-1;
            }
            else
            {
                if (TargetPC == PC)
                {
                    if (Command.rs == 0)
                    {
                        TargetPC = (uint32_t)-1;
                        EndBlock = true;
                    }
                    else
                    {
                        if (!DelaySlotEffectsCompare(PC, Command.rs, Command.rt))
                        {
                            PermLoop = true;
                        }
                    }
                }
                if (Command.rs != 0)
                {
                    ContinuePC = PC + 8;
                }
                IncludeDelaySlot = true;
            }
            break;
        case R4300i_REGIMM_BLTZL:
        case R4300i_REGIMM_BGEZL:
            TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
            if (TargetPC == PC)
            {
                if (!DelaySlotEffectsCompare(PC, Command.rs, 0))
                {
                    PermLoop = true;
                }
            }
            ContinuePC = PC + 8;
            LikelyBranch = true;
            IncludeDelaySlot = true;
            break;
        case R4300i_REGIMM_TEQI:    case R4300i_REGIMM_TNEI:    case R4300i_REGIMM_TGEI:
        case R4300i_REGIMM_TGEIU:   case R4300i_REGIMM_TLTI:    case R4300i_REGIMM_TLTIU:
            break;
        default:
            if (Command.Hex == 0x0407000D)
            {
                EndBlock = true;
                break;
            }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return false;
        }
        break;
    case R4300i_J:
        TargetPC = (PC & 0xF0000000) + (Command.target << 2);
        if (TargetPC == PC)
        {
            PermLoop = true;
        }
        IncludeDelaySlot = true;
        break;
    case R4300i_JAL:
        EndBlock = true;
        IncludeDelaySlot = true;
        break;
    case R4300i_BEQ:
        TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
        if (TargetPC == PC + 8)
        {
            TargetPC = (uint32_t)-1;
        }
        else
        {
            if (Command.rs != 0 || Command.rt != 0)
            {
                ContinuePC = PC + 8;
            }

            if (TargetPC == PC && !DelaySlotEffectsCompare(PC, Command.rs, Command.rt))
            {
                PermLoop = true;
            }
            IncludeDelaySlot = true;
        }
        break;
    case R4300i_BNE:
    case R4300i_BLEZ:
    case R4300i_BGTZ:
        TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
        if (TargetPC == PC + 8)
        {
            TargetPC = (uint32_t)-1;
        }
        else
        {
            if (TargetPC == PC)
            {
                if (!DelaySlotEffectsCompare(PC, Command.rs, Command.rt))
                {
                    PermLoop = true;
                }
            }
            ContinuePC = PC + 8;
            IncludeDelaySlot = true;
        }
        break;
    case R4300i_CP0:
        switch (Command.rs)
        {
        case R4300i_COP0_MT: case R4300i_COP0_MF:
            break;
        default:
            if ((Command.rs & 0x10) != 0)
            {
                switch (Command.funct)
                {
                case R4300i_COP0_CO_TLBR: case R4300i_COP0_CO_TLBWI:
                case R4300i_COP0_CO_TLBWR: case R4300i_COP0_CO_TLBP:
                    break;
                case R4300i_COP0_CO_ERET:
                    EndBlock = true;
                    break;
                default:
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    return false;
                }
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return false;
            }
            break;
        }
        break;
    case R4300i_CP1:
        switch (Command.fmt)
        {
        case R4300i_COP1_MF:  case R4300i_COP1_DMF: case R4300i_COP1_CF: case R4300i_COP1_MT:
        case R4300i_COP1_DMT: case R4300i_COP1_CT:  case R4300i_COP1_S:  case R4300i_COP1_D:
        case R4300i_COP1_W:   case R4300i_COP1_L:
            break;
        case R4300i_COP1_BC:
            switch (Command.ft) {
            case R4300i_COP1_BC_BCF:
            case R4300i_COP1_BC_BCT:
                TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
                if (TargetPC == PC + 8)
                {
                    TargetPC = (uint32_t)-1;
                }
                else
                {
                    if (TargetPC == PC)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    ContinuePC = PC + 8;
                    IncludeDelaySlot = true;
                }
                break;
            case R4300i_COP1_BC_BCFL:
            case R4300i_COP1_BC_BCTL:
                TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
                if (TargetPC == PC)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                ContinuePC = PC + 8;
                LikelyBranch = true;
                IncludeDelaySlot = true;
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return false;
        }
        break;
    case R4300i_ANDI:  case R4300i_ORI:    case R4300i_XORI:  case R4300i_LUI:
    case R4300i_ADDI:  case R4300i_ADDIU:  case R4300i_SLTI:  case R4300i_SLTIU:
    case R4300i_DADDI: case R4300i_DADDIU: case R4300i_LDL:   case R4300i_LDR:
    case R4300i_LB:    case R4300i_LH:     case R4300i_LWL:   case R4300i_LW:
    case R4300i_LBU:   case R4300i_LHU:    case R4300i_LWR:   case R4300i_LWU:
    case R4300i_SB:    case R4300i_SH:     case R4300i_SWL:   case R4300i_SW:
    case R4300i_SDL:   case R4300i_SDR:    case R4300i_SWR:   case R4300i_CACHE:
    case R4300i_LL:    case R4300i_LWC1:   case R4300i_LDC1:  case R4300i_LD:
    case R4300i_SC:    case R4300i_SWC1:   case R4300i_SDC1:  case R4300i_SD:
        break;
    case R4300i_BEQL:
        TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
        if (TargetPC == PC)
        {
            if (!DelaySlotEffectsCompare(PC, Command.rs, Command.rt))
            {
                PermLoop = true;
            }
        }
        if (Command.rs != 0 || Command.rt != 0)
        {
            ContinuePC = PC + 8;
        }
        IncludeDelaySlot = true;
        LikelyBranch = true;
        break;
    case R4300i_BNEL:
    case R4300i_BLEZL:
    case R4300i_BGTZL:
        TargetPC = PC + ((int16_t)Command.offset << 2) + 4;
        ContinuePC = PC + 8;
        if (TargetPC == PC)
        {
            if (!DelaySlotEffectsCompare(PC, Command.rs, Command.rt))
            {
                PermLoop = true;
            }
        }
        LikelyBranch = true;
        IncludeDelaySlot = true;
        break;
    default:
        if (Command.Hex == 0x7C1C97C0 ||
            Command.Hex == 0xF1F3F5F7)
        {
            EndBlock = true;
            break;
        }
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }
    return true;
}

bool CCodeBlock::Compile()
{
    CPU_Message("====== Code block ======");
    CPU_Message("Native entry point: %X", CompiledLocation());
    CPU_Message("Start of block: %X", VAddrEnter());
    CPU_Message("Number of sections: %d", NoOfSections());
    CPU_Message("====== Recompiled code ======");

    m_RecompilerOps->EnterCodeBlock();
    if (g_System->bLinkBlocks())
    {
        while (m_EnterSection !=nullptr && m_EnterSection->GenerateNativeCode(NextTest()));
    }
    else
    {
        if (m_EnterSection == nullptr || !m_EnterSection->GenerateNativeCode(NextTest()))
        {
            return false;
        }
    }
    m_RecompilerOps->CompileExitCode();
    m_CompiledLocationEnd = *g_RecompPos;

    uint32_t PAddr;
    g_TransVaddr->TranslateVaddr(VAddrFirst(), PAddr);
    MD5(g_MMU->Rdram() + PAddr, (VAddrLast() - VAddrFirst()) + 4).get_digest(m_Hash);

#if defined(ANDROID) && (defined(__arm__) || defined(_M_ARM))
    __clear_cache_android((uint8_t *)((uint32_t)m_CompiledLocation & ~1), m_CompiledLocationEnd);
#endif
    return true;
}

uint32_t CCodeBlock::NextTest()
{
    uint32_t next_test = m_Test;
    m_Test += 1;
    return next_test;
}