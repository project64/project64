#include "stdafx.h"
#include <Project64-core/N64System/Enhancement/Enhancements.h>
#include <Project64-core/N64System/Enhancement/EnhancementFile.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/PluginClass.h>
#include <Project64-core/Plugins/GFXPlugin.h>
#include <Project64-core/Plugins/AudioPlugin.h>
#include <Project64-core/Plugins/RSPPlugin.h>
#include <Project64-core/Plugins/ControllerPlugin.h>
#include <Common/path.h>
#include <Common/Util.h>

CEnhancements::CEnhancements() :
    m_ScanFileThread(stScanFileThread),
    m_Scan(true),
    m_Scanned(false),
    m_UpdateCheats(false)
{
    m_ScanFileThread.Start(this);
}

CEnhancements::~CEnhancements()
{
    m_Scan = false;
    WaitScanDone();
}

void CEnhancements::ApplyActive(CMipsMemoryVM & MMU, CPlugins * Plugins, bool UpdateChanges)
{
    CGuard Guard(m_CS);
    if (m_UpdateCheats && UpdateChanges)
    {
        m_UpdateCheats = false;
        Load(&MMU, Plugins);
    }
    for (size_t i = 0, n = m_ActiveCodes.size(); i < n; i++)
    {
        CODES & CodeEntry = m_ActiveCodes[i];
        for (uint32_t CurrentEntry = 0; CurrentEntry < CodeEntry.size();)
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry);
            CurrentEntry += EntrySize(CodeEntry, CurrentEntry);
        }
    }
}

void CEnhancements::ApplyGSButton(CMipsMemoryVM & MMU, bool /*UpdateChanges*/)
{
    for (size_t i = 0, n = m_ActiveCodes.size(); i < n; i++)
    {
        CODES & CodeEntry = m_ActiveCodes[i];
        for (size_t CurrentEntry = 0; CurrentEntry < CodeEntry.size(); CurrentEntry++)
        {
            const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
            switch (Code.Command & 0xFF000000) {
            case 0x88000000:
                ModifyMemory8(MMU, 0x80000000 | (Code.Command & 0xFFFFFF), (uint8_t)Code.Value);
                break;
            case 0x89000000:
                ModifyMemory16(MMU, 0x80000000 | (Code.Command & 0xFFFFFF), Code.Value);
                break;
                // Xplorer64
            case 0xA8000000:
                ModifyMemory8(MMU, 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), (uint8_t)ConvertXP64Value(Code.Value));
                break;
            case 0xA9000000:
                ModifyMemory16(MMU, 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), ConvertXP64Value(Code.Value));
                break;
            }
        }
    }
}

void CEnhancements::UpdateCheats(const CEnhancementList & Cheats)
{
    std::string GameName = g_Settings->LoadStringVal(Rdb_GoodName);
    std::string SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    CPath OutFile(g_Settings->LoadStringVal(SupportFile_UserCheatDir), stdstr_f("%s.cht", GameName.c_str()).c_str());
#ifdef _WIN32
    OutFile.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif

    CGuard Guard(m_CS);
    if (m_CheatFile.get() == nullptr || strcmp(m_CheatFile->FileName(), OutFile) != 0)
    {
        if (!OutFile.DirectoryExists())
        {
            OutFile.DirectoryCreate();
        }
        SectionFiles::const_iterator CheatFileItr = m_CheatFiles.find(SectionIdent);
        if (m_CheatFiles.end() != CheatFileItr)
        {
            m_CheatFiles.erase(CheatFileItr);
        }
        m_CheatFile = std::make_unique<CEnhancmentFile>(OutFile, CEnhancement::CheatIdent);
        m_CheatFiles.insert(SectionFiles::value_type(SectionIdent, OutFile));
    }

    m_CheatFile->SetName(SectionIdent.c_str(), GameName.c_str());
    m_CheatFile->RemoveEnhancements(SectionIdent.c_str());
    for (CEnhancementList::const_iterator itr = Cheats.begin(); itr != Cheats.end(); itr++)
    {
        const CEnhancement & Enhancement = itr->second;
        if (!Enhancement.Valid())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            continue;
        }
        m_CheatFile->AddEnhancement(SectionIdent.c_str(), Enhancement);
    }
    m_CheatFile->SaveCurrentSection();
    m_UpdateCheats = true;
}

void CEnhancements::UpdateCheats(void)
{
    m_UpdateCheats = true;
}

void CEnhancements::UpdateEnhancements(const CEnhancementList & Enhancements)
{
    std::string GameName = g_Settings->LoadStringVal(Rdb_GoodName);
    std::string SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    CPath OutFile(g_Settings->LoadStringVal(SupportFile_UserEnhancementDir), stdstr_f("%s.enh", GameName.c_str()).c_str());
#ifdef _WIN32
    OutFile.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif

    CGuard Guard(m_CS);
    if (m_EnhancementFile.get() == nullptr ||strcmp(m_EnhancementFile->FileName(), OutFile) != 0)
    {
        if (!OutFile.DirectoryExists())
        {
            OutFile.DirectoryCreate();
        }
        SectionFiles::const_iterator EnhancementFileItr = m_EnhancementFiles.find(SectionIdent);
        if (m_EnhancementFiles.end() != EnhancementFileItr)
        {
            m_EnhancementFiles.erase(EnhancementFileItr);
        }
        m_EnhancementFile = std::make_unique<CEnhancmentFile>(OutFile, CEnhancement::EnhancementIdent);
        m_EnhancementFiles.insert(SectionFiles::value_type(SectionIdent, OutFile));
    }

    m_EnhancementFile->SetName(SectionIdent.c_str(), GameName.c_str());
    m_EnhancementFile->RemoveEnhancements(SectionIdent.c_str());
    for (CEnhancementList::const_iterator itr = Enhancements.begin(); itr != Enhancements.end(); itr++)
    {
        const CEnhancement & Enhancement = itr->second;
        if (!Enhancement.Valid())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            continue;
        }
        m_EnhancementFile->AddEnhancement(SectionIdent.c_str(), Enhancement);
    }
    m_EnhancementFile->SaveCurrentSection();
    m_UpdateCheats = true;
}

void CEnhancements::ResetActive(CPlugins * Plugins)
{
    bool inBasicMode = g_Settings->LoadBool(UserInterface_BasicMode);
    bool CheatsRemembered = !inBasicMode && g_Settings->LoadBool(Setting_RememberCheats);

    if (CheatsRemembered || m_ActiveCodes.empty())
    {
        return;
    }
    bool reset = false;
    for (CEnhancementList::iterator itr = m_Cheats.begin(); itr != m_Cheats.end(); itr++)
    {
        if (!itr->second.Active())
        {
            continue;
        }
        itr->second.SetActive(false);
        reset = true;
    }
    if (reset)
    {
        m_ActiveCodes.clear();
        LoadActive(m_Enhancements, Plugins);
    }
}

void CEnhancements::ResetCodes(CMipsMemoryVM * MMU)
{
    m_ActiveCodes.clear();
    if (MMU != nullptr)
    {
        for (ORIGINAL_VALUES8::iterator itr = m_OriginalValues8.begin(); itr != m_OriginalValues8.end(); itr++)
        {
            uint8_t CurrentValue;
            if (MMU->LB_VAddr(itr->first, CurrentValue) && itr->second.Changed == CurrentValue)
            {
                MMU->SB_VAddr(itr->first, itr->second.Original);
            }
        }
    }
    m_OriginalValues8.clear();

    if (MMU != nullptr)
    {
        for (ORIGINAL_VALUES16::iterator itr = m_OriginalValues16.begin(); itr != m_OriginalValues16.end(); itr++)
        {
            uint16_t CurrentValue;
            if (MMU->LH_VAddr(itr->first, CurrentValue) && itr->second.Changed == CurrentValue)
            {
                MMU->SH_VAddr(itr->first, itr->second.Original);
            }
        }
    }
    m_OriginalValues16.clear();
}

void CEnhancements::LoadEnhancements(const char * Ident, SectionFiles & Files, std::unique_ptr<CEnhancmentFile> & File, CEnhancementList & EnhancementList)
{
    std::string SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    SectionFiles::const_iterator CheatFileItr = Files.find(SectionIdent);
    bool FoundFile = false;
    if (CheatFileItr != Files.end())
    {
        CPath CheatFile(CheatFileItr->second);
        if (CheatFile.Exists())
        {
            if (File.get() == nullptr || strcmp(File->FileName(), CheatFile) != 0)
            {
                File = std::make_unique<CEnhancmentFile>(CheatFile, Ident);
            }
            EnhancementList.clear();
            File->GetEnhancementList(SectionIdent.c_str(), EnhancementList);
            FoundFile = true;
        }
    }

    if (!FoundFile)
    {
        File = nullptr;
        EnhancementList.clear();
    }
}

void CEnhancements::Load(CMipsMemoryVM * MMU, CPlugins * Plugins)
{
    WaitScanDone();
    CGuard Guard(m_CS);

    LoadEnhancements(CEnhancement::CheatIdent, m_CheatFiles, m_CheatFile, m_Cheats);
    LoadEnhancements(CEnhancement::EnhancementIdent, m_EnhancementFiles, m_EnhancementFile, m_Enhancements);

    ResetCodes(MMU);
    LoadActive(m_Cheats, nullptr);
    LoadActive(m_Enhancements, Plugins);
}

CEnhancementList CEnhancements::Cheats(void)
{
    CEnhancementList List;
    {
        CGuard Guard(m_CS);
        List = m_Cheats;
    }
    return List;
}

CEnhancementList CEnhancements::Enhancements(void)
{
    CEnhancementList List;
    {
        CGuard Guard(m_CS);
        List = m_Enhancements;
    }
    return List;
}

void CEnhancements::LoadActive(CEnhancementList & List, CPlugins * Plugins)
{
    for (CEnhancementList::const_iterator itr = List.begin(); itr != List.end(); itr++)
    {
        const CEnhancement & Enhancement = itr->second;
        if (!Enhancement.Valid() || !Enhancement.Active())
        {
            continue;
        }

        if (Plugins != nullptr && !Enhancement.GetPluginList().empty())
        {
            bool LoadEntry = false;
            const CEnhancement::PluginList & PluginList = Enhancement.GetPluginList();
            for (size_t i = 0, n = PluginList.size(); i < n; i++)
            {
                std::string PluginName = stdstr(PluginList[i]).Trim();
                if (Plugins->Gfx() != NULL && strstr(Plugins->Gfx()->PluginName(), PluginName.c_str()) != nullptr)
                {
                    LoadEntry = true;
                    break;
                }
                if (Plugins->Audio() != NULL && strstr(Plugins->Audio()->PluginName(), PluginName.c_str()) != nullptr)
                {
                    LoadEntry = true;
                    break;
                }
                if (Plugins->RSP() != NULL && strstr(Plugins->RSP()->PluginName(), PluginName.c_str()) != nullptr)
                {
                    LoadEntry = true;
                    break;
                }
                if (Plugins->Control() != NULL && strstr(Plugins->Control()->PluginName(), PluginName.c_str()) != nullptr)
                {
                    LoadEntry = true;
                    break;
                }
            }
            if (!LoadEntry)
            {
                continue;
            }
        }

        const CEnhancement::CodeEntries Entries = Enhancement.GetEntries();
        CODES Code;
        for (size_t i = 0, n = Entries.size(); i < n; i++)
        {
            GAMESHARK_CODE CodeEntry;
            CodeEntry.Command = Entries[i].Command;
            if (strncmp(Entries[i].Value.c_str(), "????", 4) == 0)
            {
                CodeEntry.Value = Enhancement.SelectedOption();
            }
            else if (strncmp(Entries[i].Value.c_str(), "??", 2) == 0)
            {
                CodeEntry.Value = (uint8_t)(strtoul(&(Entries[i].Value.c_str()[2]), 0, 16), 0, 16);
                CodeEntry.Value |= Enhancement.SelectedOption() << 16;
            }
            else if (strncmp(&Entries[i].Value[2], "??", 2) == 0)
            {
                CodeEntry.Value = (uint16_t)(strtoul(Entries[i].Value.c_str(), 0, 16) << 16);
                CodeEntry.Value |= Enhancement.SelectedOption();
            }
            else
            {
                CodeEntry.Value = (uint16_t)strtoul(Entries[i].Value.c_str(), 0, 16);
            }
            Code.push_back(CodeEntry);
        }
        m_ActiveCodes.push_back(Code);
    }
}

void CEnhancements::ApplyGameSharkCodes(CMipsMemoryVM & MMU, CODES & CodeEntry, uint32_t CurrentEntry)
{
    if (CurrentEntry >= CodeEntry.size())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
    uint16_t wMemory;
    uint8_t bMemory;

    switch (Code.Command & 0xFF000000)
    {
    case 0x50000000: // Gameshark / AR
        if ((CurrentEntry + 1) >= (int)CodeEntry.size())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        {
            const GAMESHARK_CODE & NextCodeEntry = CodeEntry[CurrentEntry + 1];
            int numrepeats = (Code.Command & 0x0000FF00) >> 8;
            int offset = Code.Command & 0x000000FF;
            uint32_t Address;
            int incr = Code.Value;
            int i;

            switch (NextCodeEntry.Command & 0xFF000000) {
            case 0x10000000: // Xplorer64
            case 0x80000000:
                Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
                wMemory = NextCodeEntry.Value;
                for (i = 0; i < numrepeats; i++)
                {
                    ModifyMemory8(MMU, Address, (uint8_t)wMemory);
                    Address += offset;
                    wMemory += (uint16_t)incr;
                }
                break;
            case 0x11000000: // Xplorer64
            case 0x81000000:
                Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
                wMemory = NextCodeEntry.Value;
                for (i = 0; i < numrepeats; i++)
                {
                    ModifyMemory16(MMU, Address, wMemory);
                    Address += offset;
                    wMemory += (uint16_t)incr;
                }
                break;
            }
        }
        break;
    case 0x80000000:
    case 0x30000000:
    case 0x82000000:
    case 0x84000000:
        ModifyMemory8(MMU, 0x80000000 | (Code.Command & 0xFFFFFF), (uint8_t)Code.Value);
        break;
    case 0x81000000:
        ModifyMemory16(MMU, 0x80000000 | (Code.Command & 0xFFFFFF), Code.Value);
        break;
    case 0xA0000000:
        ModifyMemory8(MMU, 0xA0000000 | (Code.Command & 0xFFFFFF), (uint8_t)Code.Value);
        break;
    case 0xA1000000:
        ModifyMemory16(MMU, 0xA0000000 | (Code.Command & 0xFFFFFF), Code.Value);
        break;
    case 0xD0000000:
        MMU.LB_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), bMemory);
        if (bMemory == Code.Value)
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xD1000000:
        MMU.LH_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), wMemory);
        if (wMemory == Code.Value)
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xD2000000:
        MMU.LB_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), bMemory);
        if (bMemory != Code.Value)
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xD3000000:
        MMU.LH_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), wMemory);
        if (wMemory != Code.Value)
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0x31000000:
    case 0x83000000:
    case 0x85000000:
        ModifyMemory16(MMU, 0x80000000 | (Code.Command & 0xFFFFFF), Code.Value);
        break;
    case 0xE8000000:
        ModifyMemory8(MMU, 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), (uint8_t)ConvertXP64Value(Code.Value));
        break;
    case 0xE9000000:
        ModifyMemory16(MMU, 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), ConvertXP64Value(Code.Value));
        break;
    case 0xC8000000:
        ModifyMemory8(MMU, 0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), (uint8_t)Code.Value);
        break;
    case 0xC9000000:
        ModifyMemory16(MMU, 0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), ConvertXP64Value(Code.Value));
        break;
    case 0xB8000000:
    case 0xBA000000:
        MMU.LB_VAddr(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), bMemory);
        if (bMemory == ConvertXP64Value(Code.Value))
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xB9000000:
    case 0xBB000000:
        MMU.LH_VAddr(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), wMemory);
        if (wMemory == ConvertXP64Value(Code.Value))
        {
            ApplyGameSharkCodes(MMU, CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0x88000000:
    case 0x89000000:
    case 0xA8000000:
    case 0xA9000000:
        //Ignore - GS Button
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

uint32_t CEnhancements::EntrySize(const CODES & CodeEntry, uint32_t CurrentEntry)
{
    if (CurrentEntry < 0 || CurrentEntry >= (int)CodeEntry.size())
    {
        return 0;
    }
    const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
    switch (Code.Command & 0xFF000000)
    {
    case 0x50000000: // Gameshark / AR
        if ((CurrentEntry + 1) >= (int)CodeEntry.size())
        {
            return 1;
        }

        switch (CodeEntry[CurrentEntry + 1].Command & 0xFF000000)
        {
        case 0x10000000: // Xplorer64
        case 0x80000000:
        case 0x11000000: // Xplorer64
        case 0x81000000:
            return 2;
        }
        break;
    case 0xD0000000:
    case 0xD1000000:
    case 0xD2000000:
    case 0xD3000000:
    case 0xB8000000:
    case 0xB9000000:
    case 0xBA000000:
    case 0xBB000000:
        return EntrySize(CodeEntry, CurrentEntry + 1) + 1;
    case 0:
        return MaxGSEntries;
    }
    return 1;
}

void CEnhancements::ModifyMemory8(CMipsMemoryVM & MMU, uint32_t Address, uint8_t Value)
{
    MEM_VALUE8 OriginalValue;
    if (!MMU.LB_VAddr(Address, OriginalValue.Original))
    {
        return;
    }
    if (OriginalValue.Original == Value)
    {
        return;
    }
    MMU.SB_VAddr(Address, Value);
    if (g_Recompiler)
    {
        g_Recompiler->ClearRecompCode_Virt(Address & ~0xFFF, 0x1000, CRecompiler::Remove_Cheats);
    }
    OriginalValue.Changed = Value;
    std::pair<ORIGINAL_VALUES8::iterator, bool> itr = m_OriginalValues8.insert(ORIGINAL_VALUES8::value_type(Address, OriginalValue));
}

void CEnhancements::ModifyMemory16(CMipsMemoryVM & MMU, uint32_t Address, uint16_t Value)
{
    MEM_VALUE16 OriginalValue;
    if (!MMU.LH_VAddr(Address, OriginalValue.Original))
    {
        return;
    }
    if (OriginalValue.Original == Value)
    {
        return;
    }
    OriginalValue.Changed = Value;
    std::pair<ORIGINAL_VALUES16::iterator, bool> itr = m_OriginalValues16.insert(ORIGINAL_VALUES16::value_type(Address, OriginalValue));
    MMU.SH_VAddr(Address, OriginalValue.Changed);
    if (g_Recompiler)
    {
        g_Recompiler->ClearRecompCode_Virt(Address & ~0xFFF, 0x1000, CRecompiler::Remove_Cheats);
    }
}


void CEnhancements::ScanFileThread(void)
{
    SectionFiles CheatFiles;
    CPath File(g_Settings->LoadStringVal(SupportFile_CheatDir), "*.cht");
#ifdef _WIN32
    File.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (File.FindFirst() && m_Scan)
    {
        do
        {
            CEnhancmentFile EnhancmentFile(File, CEnhancement::CheatIdent);
            CEnhancmentFile::SectionList Sections;
            EnhancmentFile.GetSections(Sections);
            for (CEnhancmentFile::SectionList::const_iterator itr = Sections.begin(); itr != Sections.end(); itr++)
            {
                CheatFiles.insert(SectionFiles::value_type(itr->c_str(), File));
            }
        } while (m_Scan && File.FindNext());
    }

    File = CPath(g_Settings->LoadStringVal(SupportFile_UserCheatDir), "*.cht");
#ifdef _WIN32
    File.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (m_Scan && File.FindFirst())
    {
        do
        {
            CEnhancmentFile EnhancmentFile(File, CEnhancement::CheatIdent);
            CEnhancmentFile::SectionList Sections;
            EnhancmentFile.GetSections(Sections);
            for (CEnhancmentFile::SectionList::const_iterator itr = Sections.begin(); itr != Sections.end(); itr++)
            {
                CheatFiles[itr->c_str()] = (const char *)File;
            }
        } while (m_Scan && File.FindNext());
    }

    File = CPath(g_Settings->LoadStringVal(SupportFile_EnhancementDir), "*.enh");
#ifdef _WIN32
    File.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    SectionFiles EnhancementFiles;
    if (File.FindFirst() && m_Scan)
    {
        do
        {
            CEnhancmentFile EnhancmentFile(File, "Enhancment");
            CEnhancmentFile::SectionList Sections;
            EnhancmentFile.GetSections(Sections);
            for (CEnhancmentFile::SectionList::const_iterator itr = Sections.begin(); itr != Sections.end(); itr++)
            {
                EnhancementFiles.insert(SectionFiles::value_type(itr->c_str(), File));
            }
        } while (m_Scan && File.FindNext());
    }

    File = CPath(g_Settings->LoadStringVal(SupportFile_UserEnhancementDir), "*.enh");
#ifdef _WIN32
    File.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (m_Scan && File.FindFirst())
    {
        do
        {
            CEnhancmentFile EnhancmentFile(File, "Enhancment");
            CEnhancmentFile::SectionList Sections;
            EnhancmentFile.GetSections(Sections);
            for (CEnhancmentFile::SectionList::const_iterator itr = Sections.begin(); itr != Sections.end(); itr++)
            {
                EnhancementFiles[itr->c_str()] = (const char *)File;
            }
        } while (m_Scan && File.FindNext());
    }

    {
        CGuard Guard(m_CS);
        m_CheatFiles = CheatFiles;
        m_EnhancementFiles = EnhancementFiles;
        m_Scanned = true;
    }
}

void CEnhancements::WaitScanDone()
{
    for (uint32_t i = 0; i < 500; i++)
    {
        bool Scanned = false;
        {
            CGuard Guard(m_CS);
            Scanned = m_Scanned;
        }
        if (Scanned)
        {
            break;
        }
        pjutil::Sleep(100);
    }
}

uint32_t CEnhancements::ConvertXP64Address(uint32_t Address)
{
    uint32_t tmpAddress = (Address ^ 0x68000000) & 0xFF000000;
    tmpAddress += ((Address + 0x002B0000) ^ 0x00810000) & 0x00FF0000;
    tmpAddress += ((Address + 0x00002B00) ^ 0x00008200) & 0x0000FF00;
    tmpAddress += ((Address + 0x0000002B) ^ 0x00000083) & 0x000000FF;
    return tmpAddress;
}

uint16_t CEnhancements::ConvertXP64Value(uint16_t Value)
{
    uint16_t tmpValue = ((Value + 0x2B00) ^ 0x8400) & 0xFF00;
    tmpValue += ((Value + 0x002B) ^ 0x0085) & 0x00FF;
    return tmpValue;
}
