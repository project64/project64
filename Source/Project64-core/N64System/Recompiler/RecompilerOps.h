#pragma once
#include <Project64-core\N64System\Mips\R4300iOpcode.h>
#include <Project64-core\Settings\DebugSettings.h>

enum RecompilerBranchType
{
    RecompilerBranchType_Cop1,
    RecompilerBranchType_Rs,
    RecompilerBranchType_RsRt
};

enum RecompilerBranchCompare
{
    RecompilerBranchCompare_BEQ,
    RecompilerBranchCompare_BNE,
    RecompilerBranchCompare_BLTZ,
    RecompilerBranchCompare_BLEZ,
    RecompilerBranchCompare_BGTZ,
    RecompilerBranchCompare_BGEZ,
    RecompilerBranchCompare_COP1BCF,
    RecompilerBranchCompare_COP1BCT,
};

enum RecompilerTrapCompare
{
    RecompilerTrapCompare_TEQ,
    RecompilerTrapCompare_TNE,
    RecompilerTrapCompare_TGE,
    RecompilerTrapCompare_TGEU,
    RecompilerTrapCompare_TLT,
    RecompilerTrapCompare_TLTU,
    RecompilerTrapCompare_TEQI,
    RecompilerTrapCompare_TNEI,
    RecompilerTrapCompare_TGEI,
    RecompilerTrapCompare_TGEIU,
    RecompilerTrapCompare_TLTI,
    RecompilerTrapCompare_TLTIU,
};

class CCodeBlock;
class CCodeSection;
class CN64System;
class CSystemEvents;
class CMipsMemoryVM;
class CRegisters;

class CRecompilerOpsBase :
    protected CDebugSettings
{
protected:
    CRecompilerOpsBase(CN64System & System, CCodeBlock & CodeBlock);
    ~CRecompilerOpsBase();

    CN64System & m_System;
    CSystemEvents & m_SystemEvents;
    CMipsMemoryVM & m_MMU;
    CRegisters & m_Reg;
    CCodeBlock & m_CodeBlock;
    R4300iOpcode m_Opcode;
    CCodeSection * m_Section;

private:
    CRecompilerOpsBase(const CRecompilerOpsBase &);
    CRecompilerOpsBase & operator=(const CRecompilerOpsBase &);
};

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Recompiler/x86/x86RecompilerOps.h>

#elif defined(__amd64__) || defined(_M_X64)
#include <Project64-core/N64System/Recompiler/x64-86/x64RecompilerOps.h>

#elif defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/Arm/ArmRecompilerOps.h>

#elif defined(__aarch64__)
#include <Project64-core/N64System/Recompiler/Aarch64/Aarch64RecompilerOps.h>

#endif
