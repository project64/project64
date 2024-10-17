#pragma once
#include <Common/md5.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>

#if !defined(_MSC_VER) && !defined(_Printf_format_string_)
#define _Printf_format_string_
#endif

class CMipsMemoryVM;
class CRecompiler;
class CRecompMemory;

class CCodeBlock :
    public asmjit::ErrorHandler
{
public:
    CCodeBlock(CN64System & System, uint32_t VAddrEnter);
    ~CCodeBlock();

    bool Compile();
    uint32_t Finilize(CRecompMemory & RecompMem);

    asmjit::CodeHolder & CodeHolder(void)
    {
        return m_CodeHolder;
    }
    uint32_t VAddrEnter() const
    {
        return m_VAddrEnter;
    }
    uint32_t VAddrFirst() const
    {
        return m_VAddrFirst;
    }
    uint32_t VAddrLast() const
    {
        return m_VAddrLast;
    }
    uint8_t * CompiledLocation() const
    {
        return m_CompiledLocation;
    }
    int32_t NoOfSections() const
    {
        return (int32_t)m_Sections.size() - 1;
    }
    const CCodeSection & EnterSection() const
    {
        return *m_EnterSection;
    }
    const MD5Digest & Hash() const
    {
        return m_Hash;
    }
    CRecompilerOps *& RecompilerOps()
    {
        return m_RecompilerOps;
    }
    CRegisters & Registers()
    {
        return m_Reg;
    }
    const std::string & CodeLog() const
    {
        return m_CodeLog;
    }

    void SetVAddrFirst(uint32_t VAddr)
    {
        m_VAddrFirst = VAddr;
    }
    void SetVAddrLast(uint32_t VAddr)
    {
        m_VAddrLast = VAddr;
    }

    CCodeSection * ExistingSection(uint32_t Addr)
    {
        return m_EnterSection->ExistingSection(Addr, NextTest());
    }
    bool SectionAccessible(uint32_t m_SectionID)
    {
        return m_EnterSection->SectionAccessible(m_SectionID, NextTest());
    }

    uint64_t MemContents(int32_t i) const
    {
        return m_MemContents[i];
    }
    uint64_t * MemLocation(int32_t i) const
    {
        return m_MemLocation[i];
    }

    uint32_t NextTest();

    void Log(_Printf_format_string_ const char * Text, ...);

private:
    CCodeBlock();
    CCodeBlock(const CCodeBlock &);
    CCodeBlock & operator=(const CCodeBlock &);

    bool AnalyseBlock();

    bool CreateBlockLinkage(CCodeSection * EnterSection);
    void DetermineLoops();
    void LogSectionInfo();
    bool SetSection(CCodeSection *& Section, CCodeSection * CurrentSection, uint32_t TargetPC, bool LinkAllowed, uint32_t CurrentPC);
    bool AnalyzeInstruction(uint32_t PC, uint32_t & TargetPC, uint32_t & ContinuePC, bool & LikelyBranch, bool & IncludeDelaySlot, bool & EndBlock, bool & PermLoop);
    void handleError(asmjit::Error err, const char * message, asmjit::BaseEmitter * origin);

    asmjit::Environment m_Environment;
    asmjit::CodeHolder m_CodeHolder;
    uint32_t m_VAddrEnter;
    uint32_t m_VAddrFirst;
    uint32_t m_VAddrLast;
    uint8_t * m_CompiledLocation;

    typedef std::map<uint32_t, CCodeSection *> SectionMap;
    typedef std::list<CCodeSection *> SectionList;

    CRecompiler & m_Recompiler;
    CMipsMemoryVM & m_MMU;
    CRegisters & m_Reg;
    SectionMap m_SectionMap;
    SectionList m_Sections;
    CCodeSection * m_EnterSection;
    int32_t m_Test;
    MD5Digest m_Hash;
    uint64_t m_MemContents[2];
    uint64_t * m_MemLocation[2];
    CRecompilerOps * m_RecompilerOps;
    std::string m_CodeLog;
};
