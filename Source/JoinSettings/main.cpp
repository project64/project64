#include <Common/path.h>
#include <Common/IniFile.h>
#include <Common/StdString.h>
#include <Project64-core/N64System/Enhancement/EnhancementFile.h>
#include <algorithm>
#include <set>
#include <windows.h>

struct compareKeyValueItem
{
    inline bool operator() (CIniFileBase::KeyValueItem & struct1, const CIniFileBase::KeyValueItem & struct2)
    {
        std::string a = *struct1.first;
        std::string b = *struct2.first;
        if (_stricmp(a.c_str(), "Name") == 0)
        {
            return true;
        }
        if (_stricmp(b.c_str(), "Name") == 0)
        {
            return false;
        }
        if (a.length() > 5 && _strnicmp(a.c_str(), "cheat", 5) == 0 &&
            b.length() > 5 && _strnicmp(b.c_str(), "cheat", 5) == 0)
        {
            int i1 = atoi(&(*struct1.first)[5]);
            int i2 = atoi(&(*struct2.first)[5]);
            if (i1 != i2)
            {
                return i1 < i2;
            }
            char Buffer[40];
            size_t number_len = strlen(_itoa(i1, Buffer, 10));
            if (strlen(&a[5 + number_len]) == 0)
            {
                return true;
            }
            if (strlen(&b[5 + number_len]) == 0)
            {
                return false;
            }
            return _stricmp(&a[5 + number_len], &b[5 + number_len]) <= 0;
        }
        return _stricmp(a.c_str(), b.c_str()) <= 0;
    }
};

void CustomSortData(CIniFileBase::KeyValueVector & data)
{
    std::sort(data.begin(), data.end(), compareKeyValueItem());
}

void SplitFile(const char * FileName, const char * Target)
{
    if (!CPath(Target,"").DirectoryCreate())
    {
        return;
    }

    CPath SearchDir(Target, "*.*");
    if (SearchDir.FindFirst())
    {
        do
        {
            SearchDir.Delete();
        } while (SearchDir.FindNext());
    }

    CIniFile::SectionList Sections;
    CIniFile CheatIniFile(FileName);
    CheatIniFile.GetVectorOfSections(Sections);

    for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
    {
        const char * Section = SectionItr->c_str();

        CIniFile::KeyValueData data;
        CheatIniFile.GetKeyValueData(Section, data);

        stdstr Name = CheatIniFile.GetString(Section, "Name", "");
        Name.Trim("\t =");
        if (Name.length() == 0)
        {
            Name = CheatIniFile.GetString(Section, "Good Name", Section);
            Name.Trim("\t =");
        }
        Name.Replace("\\", "-");
        Name.Replace("/", "-");
        Name.Replace(":", " -");
        if (Name.length() == 0)
        {
            continue;
        }

        CPath GameFileName(Target, stdstr_f("%s.%s", Name.c_str(), CPath(FileName).GetExtension().c_str()).c_str());
        CIniFile GameIniFile(GameFileName);
        if (!GameIniFile.IsFileOpen())
        {
            continue;
        }

        GameIniFile.SetAutoFlush(false);
        GameIniFile.SetCustomSort(CustomSortData);

        for (CIniFile::KeyValueData::const_iterator itr = data.begin(); itr != data.end(); itr++)
        {
            stdstr DataLine(itr->second);
            DataLine.Trim("\t =");
            if (strcmp(itr->first.c_str(), "Good Name") == 0)
            {
                GameIniFile.SaveString(Section, "Name", DataLine.c_str());
            }
            else
            {
                GameIniFile.SaveString(Section, itr->first.c_str(), DataLine.c_str());
            }
        }
        GameIniFile.FlushChanges();
    }
}

typedef std::map<std::string, std::string> Files;

void RegionSection(CFile &TargetIniFile, Files &files, const char * Region, const char * RegionCode)
{
    stdstr_f LineData = stdstr_f("//--------------- %s Region cheat codes ---------------\r\n\r\n", Region);
    TargetIniFile.Write(LineData.c_str(), (int)LineData.length());

    bool first = true;
    for (Files::const_iterator itr = files.begin(); itr != files.end(); itr++)
    {
        CIniFile GameIniFile(itr->second.c_str());
        GameIniFile.SetCustomSort(CustomSortData);
        CIniFile::SectionList Sections;
        GameIniFile.GetVectorOfSections(Sections);

        bool found = false;
        stdstr_f searchStr(":%s", RegionCode);
        for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
        {
            const char * Section = SectionItr->c_str();
            const char * pos = strstr(Section, searchStr.c_str());
            if (pos == nullptr)
            {
                continue;
            }
            found = true;
            break;
        }

        if (!found)
        {
            continue;
        }

        for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
        {
            const char * Section = SectionItr->c_str();

            CIniFile::KeyValueData data;
            GameIniFile.GetKeyValueData(Section, data);

            CIniFile::KeyValueVector data2;
            for (CIniFile::KeyValueData::const_iterator DataItr = data.begin(); DataItr != data.end(); DataItr++)
            {
                data2.push_back(CIniFile::KeyValueItem(&DataItr->first, &DataItr->second));
            }
            std::sort(data2.begin(), data2.end(), compareKeyValueItem());

            if (first)
            {
                first = false;
            }
            else
            {
                LineData = stdstr_f("\r\n//----\r\n\r\n");
                TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
            }
            LineData = stdstr_f("[%s]\r\n", Section);
            TargetIniFile.Write(LineData.c_str(), (int)LineData.length());

            for (CIniFile::KeyValueVector::const_iterator DataItr = data2.begin(); DataItr != data2.end(); DataItr++)
            {
                LineData = stdstr_f("%s=%s\r\n", DataItr->first->c_str(), DataItr->second->c_str());
                TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
            }
        }
    }
}

void JoinFile(const char * Directory, const char * Target)
{
    Files files;
    CPath SearchDir(Directory, "*.*");
    if (SearchDir.FindFirst())
    {
        do
        {
            CIniFile GameIniFile(SearchDir);

            CIniFile::SectionList Sections;
            GameIniFile.GetVectorOfSections(Sections);
            for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
            {
                const char * Section = SectionItr->c_str();
                stdstr Name = GameIniFile.GetString(Section, "Name", Section);
                Name.Trim("\t =");
                if (Name.size() > 0)
                {
                    files.insert(Files::value_type(Name, SearchDir));
                    break;
                }
            }
        } while (SearchDir.FindNext());
    }

    if (CPath(Target).Exists())
    {
        CPath(Target).Delete();
    };

    CFile TargetIniFile;
    if (!TargetIniFile.Open(Target, CFileBase::modeReadWrite | CFileBase::modeCreate | CFileBase::shareDenyWrite))
    {
        return;
    }
    if (strcmp(CPath(Target).GetExtension().c_str(), "cht") == 0)
    {
        stdstr_f LineData = stdstr_f("// Project64 official cheat database\r\n");
        TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
        LineData = stdstr_f("// Not for use with Project64 v1.6 or lower\r\n");
        TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
        LineData = stdstr_f("// ----------------------------------------------------\r\n\r\n");
        TargetIniFile.Write(LineData.c_str(), (int)LineData.length());

        CPath MetaFileName(Directory, "Meta.cht");
        CIniFile MetaIniFile(MetaFileName);
        if (MetaIniFile.IsFileOpen())
        {
            CIniFile::KeyValueData data;
            MetaIniFile.GetKeyValueData("Meta", data);

            LineData = stdstr_f("[Meta]\r\n");
            TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
            for (CIniFile::KeyValueData::const_iterator itr = data.begin(); itr != data.end(); itr++)
            {
                stdstr DataLine(itr->second);
                DataLine.Trim("\t =");
                LineData = stdstr_f("%s=%s\r\n",itr->first.c_str(), DataLine.c_str());
                TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
            }
            LineData = stdstr_f("\r\n");
            TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
        }
    }


    RegionSection(TargetIniFile, files, "(J)", "4A");
    RegionSection(TargetIniFile, files, "(JU)", "41");
    RegionSection(TargetIniFile, files, "(U)", "45");
    RegionSection(TargetIniFile, files, "PAL (E)", "50");
    RegionSection(TargetIniFile, files, "PAL (A)", "55");
    RegionSection(TargetIniFile, files, "PAL (F)", "46");
    RegionSection(TargetIniFile, files, "PAL (G)", "44");
    RegionSection(TargetIniFile, files, "PAL (I)", "49");
    RegionSection(TargetIniFile, files, "PAL (S)", "53");
    RegionSection(TargetIniFile, files, "PAL(FGD)", "58");
    RegionSection(TargetIniFile, files, "Demo", "0");
}

void UpdateNames(const char* Directory, const char* RdbFile)
{
    CIniFile RdbIni(RdbFile);

    Files files;
    CPath SearchDir(Directory, "*.cht");
    if (SearchDir.FindFirst())
    {
        do
        {
            CIniFile CheatFile(SearchDir);
            CIniFile::SectionList Sections;
            CheatFile.GetVectorOfSections(Sections);
            CheatFile.SetCustomSort(CustomSortData);
            for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
            {
                const char * Section = SectionItr->c_str();
                std::string Name = RdbIni.GetString(Section, "Good Name", "");
                if (Name.empty())
                {
                    Name = RdbIni.GetString(Section, "Internal Name", "");
                }
                if (Name.empty())
                {
                    continue;
                }
                CheatFile.SaveString(Section, "Name", Name.c_str());
            }
        } while (SearchDir.FindNext());
    }
}

uint32_t ConvertXP64Address(uint32_t Address)
{
    uint32_t tmpAddress;

    tmpAddress = (Address ^ 0x68000000) & 0xFF000000;
    tmpAddress += ((Address + 0x002B0000) ^ 0x00810000) & 0x00FF0000;
    tmpAddress += ((Address + 0x00002B00) ^ 0x00008200) & 0x0000FF00;
    tmpAddress += ((Address + 0x0000002B) ^ 0x00000083) & 0x000000FF;
    return tmpAddress;
}

uint8_t ConvertXP64ValueHi(uint8_t Value)
{
    return (Value + 0x2B) ^ 0x84;
}

uint8_t ConvertXP64ValueLo(uint8_t Value)
{
    return (Value + 0x2B) ^ 0x85;
}

uint16_t ConvertXP64Value(uint16_t Value)
{
    uint16_t  tmpValue;

    tmpValue = ((Value + 0x2B00) ^ 0x8400) & 0xFF00;
    tmpValue += ((Value + 0x002B) ^ 0x0085) & 0x00FF;
    return tmpValue;
}

bool ConvertCheatOptions(const char * OptionValue, std::string& Options)
{
    const char * ReadPos = strchr(Options.c_str(), '$');
    std::string NewOptions;
    if (ReadPos)
    {
        ReadPos += 1;
        do
        {
            NewOptions += NewOptions.empty() ? "$" : ",$";
            const char* End = strchr(ReadPos, ',');
            std::string Item = End != nullptr ? std::string(ReadPos, End - ReadPos) : ReadPos;
            ReadPos = strchr(ReadPos, '$');
            if (ReadPos != nullptr)
            {
                ReadPos += 1;
            }
            const char* Name = strchr(Item.c_str(), ' ');
            if (Name == nullptr)
            {
                return false;
            }
            Name += 1;
            if (strcmp(OptionValue, "????") == 0)
            {
                uint16_t CodeValue = ConvertXP64Value((uint16_t)strtoul(Item.c_str(), 0, 16));
                NewOptions += stdstr_f("%04X %s", CodeValue, stdstr(Name).Trim().c_str());
            }
            else if (strcmp(OptionValue, "??XX") == 0)
            {
                uint8_t CodeValue = ConvertXP64ValueHi((uint8_t)strtoul(Item.c_str(), 0, 16));
                NewOptions += stdstr_f("%02X %s", CodeValue, stdstr(Name).Trim().c_str());
            }
            else if (strcmp(OptionValue, "XX??") == 0)
            {
                uint8_t CodeValue = ConvertXP64ValueLo((uint8_t)strtoul(Item.c_str(), 0, 16));
                NewOptions += stdstr_f("%02X %s", CodeValue, stdstr(Name).Trim().c_str());
            }
            else
            {
                return false;
            }
        } while (ReadPos);
    }
    Options = NewOptions;
    return true;
}

bool ConvertCheatEntry(std::string& CheatEntry, std::string& CheatOptions)
{
    typedef std::vector<std::string> CodeEntries;

    size_t StartOfName = CheatEntry.find("\"");
    if (StartOfName == std::string::npos)
    {
        return false;
    }
    size_t EndOfName = CheatEntry.find("\"", StartOfName + 1);
    if (EndOfName == std::string::npos)
    {
        return false;
    }
    std::string Name = stdstr(CheatEntry.substr(StartOfName + 1, EndOfName - StartOfName - 1)).Trim("\t ,");
    CodeEntries Entries;

    const char* CheatString = &CheatEntry.c_str()[EndOfName + 2];
    const char* ReadPos = CheatString;
    bool ConvertOptions = false;
    std::string OptionValue;

    while (ReadPos)
    {
        uint32_t CodeCommand = strtoul(ReadPos, 0, 16);
        ReadPos = strchr(ReadPos, ' ');
        if (ReadPos == nullptr)
        {
            break;
        }
        ReadPos += 1;
        std::string ValueStr = ReadPos;
        const char* ValuePos = ReadPos;
        ReadPos = strchr(ReadPos, ',');
        if (ReadPos != nullptr)
        {
            ValueStr.resize(ReadPos - ValuePos);
            ReadPos++;
        }

        switch (CodeCommand & 0xFF000000)
        {
        case 0xE8000000:
            CodeCommand = (ConvertXP64Address(CodeCommand) & 0xFFFFFF) | 0x80000000;
            if (strchr(ValueStr.c_str(), '?') != nullptr)
            {
                if (strncmp(ValueStr.c_str(), "????", 4) == 0)
                {
                    Entries.push_back(stdstr_f("%08X ????", CodeCommand));
                    if (!OptionValue.empty() && OptionValue != "????")
                    {
                        return false;
                    }
                    OptionValue = "????";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[2], "??", 2) == 0)
                {
                    Entries.push_back(stdstr_f("%08X %02X??", CodeCommand, ConvertXP64ValueHi((uint8_t)strtoul(ValueStr.c_str(), 0, 16))));
                    if (!OptionValue.empty() && OptionValue != "XX??")
                    {
                        return false;
                    }
                    OptionValue = "XX??";
                }
                else
                {
                    return false;
                }
                ConvertOptions = true;
            }
            else
            {
                Entries.push_back(stdstr_f("%08X %04X", CodeCommand, ConvertXP64Value((uint16_t)strtoul(ValueStr.c_str(), 0, 16))));
            }
            break;
        case 0xE9000000:
            CodeCommand = (ConvertXP64Address(CodeCommand) & 0xFFFFFF) | 0x81000000;
            if (strchr(ValueStr.c_str(), '?') != nullptr)
            {
                if (strncmp(ValueStr.c_str(), "????", 4) == 0)
                {
                    Entries.push_back(stdstr_f("%08X ????", CodeCommand));
                    if (!OptionValue.empty() && OptionValue != "????")
                    {
                        return false;
                    }
                    OptionValue = "????";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[2], "??", 2) == 0)
                {
                    Entries.push_back(stdstr_f("%08X %02X??", CodeCommand, ConvertXP64ValueHi((uint8_t)strtoul(ValueStr.c_str(), 0, 16))));
                    if (!OptionValue.empty() && OptionValue != "XX??")
                    {
                        return false;
                    }
                    OptionValue = "XX??";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[0], "??", 2) == 0)
                {
                    Entries.push_back(stdstr_f("%08X ??%02X", CodeCommand, ConvertXP64ValueLo((uint8_t)strtoul(&(ValueStr.c_str()[2]), 0, 16))));
                    if (!OptionValue.empty() && OptionValue != "??XX")
                    {
                        return false;
                    }
                    OptionValue = "??XX";
                }
                else
                {
                    return false;
                }
                ConvertOptions = true;
            }
            else
            {
                Entries.push_back(stdstr_f("%08X %04X", CodeCommand, ConvertXP64Value((uint16_t)strtoul(ValueStr.c_str(), 0, 16))));
            }
            break;
        case 0x10000000:
            Entries.push_back(stdstr_f("%08X %s", (CodeCommand & 0xFFFFFF) | 0x80000000, ValueStr.c_str()));
            if (strchr(ValueStr.c_str(), '?') != nullptr)
            {
                if (strncmp(ValueStr.c_str(), "????", 4) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "????")
                    {
                        return false;
                    }
                    OptionValue = "????";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[2], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "XX??")
                    {
                        return false;
                    }
                    OptionValue = "XX??";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[0], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "??XX")
                    {
                        return false;
                    }
                    OptionValue = "??XX";
                }
                else
                {
                    return false;
                }
                ConvertOptions = true;
            }
            break;
        case 0x11000000:
            Entries.push_back(stdstr_f("%08X %s", (CodeCommand & 0xFFFFFF) | 0x81000000, ValueStr.c_str()));
            if (strchr(ValueStr.c_str(), '?') != nullptr)
            {
                if (strncmp(ValueStr.c_str(), "????", 4) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "????")
                    {
                        return false;
                    }
                    OptionValue = "????";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[2], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "XX??")
                    {
                        return false;
                    }
                    OptionValue = "XX??";
                }
                else if (ValueStr.length() == 4 && strncmp(&ValueStr.c_str()[0], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "??XX")
                    {
                        return false;
                    }
                    OptionValue = "??XX";
                }
                else
                {
                    return false;
                }
                ConvertOptions = true;
                return false;
            }
            break;
        case 0x80000000:
        case 0x81000000:
        case 0x88000000:
        case 0x89000000:
        case 0xA0000000:
        case 0xA1000000:
        case 0x50000000:
        case 0xD0000000:
        case 0xD1000000:
        case 0xD2000000:
        case 0xD3000000:
            Entries.push_back(stdstr_f("%08X %s", CodeCommand, ValueStr.c_str()));
            break;
        default:
            return false;
        }
    }

    if (Name.length() == 0 || Entries.size() == 0)
    {
        return false;
    }
    if (ConvertOptions && !ConvertCheatOptions(OptionValue.c_str(), CheatOptions))
    {
        return false;
    }
    CheatEntry = stdstr_f("\"%s\"", Name.c_str());
    for (CodeEntries::const_iterator itr = Entries.begin(); itr != Entries.end(); itr++)
    {
        CheatEntry += stdstr_f(",%s", itr->c_str());
    }
    return true;
}

void convertGS(const char* Directory)
{
    enum
    {
        MaxCheats = 50000,
    };

    CPath SearchDir(Directory, "*.cht");
    if (SearchDir.FindFirst())
    {
        do
        {
            CIniFile CheatFile(SearchDir);
            CIniFile::SectionList Sections;
            CheatFile.GetVectorOfSections(Sections);
            CheatFile.SetCustomSort(CustomSortData);
            for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
            {
                const char * Section = SectionItr->c_str();
                for (uint32_t cheat = 0; cheat < MaxCheats; cheat++)
                {
                    std::string CheatEntry = CheatFile.GetString(Section, stdstr_f("Cheat%d", cheat).c_str(), "");
                    if (CheatEntry.empty())
                    {
                        break;
                    }
                    std::string CheatOptions = CheatFile.GetString(Section, stdstr_f("Cheat%d_O", cheat).c_str(), "");
                    if (ConvertCheatEntry(CheatEntry, CheatOptions))
                    {
                        CheatFile.SaveString(Section, stdstr_f("Cheat%d", cheat).c_str(), CheatEntry.c_str());
                        if (!CheatOptions.empty())
                        {
                            CheatFile.SaveString(Section, stdstr_f("Cheat%d_O", cheat).c_str(), CheatOptions.c_str());
                        }
                    }
                }
            }
        } while (SearchDir.FindNext());
    }

}

bool ParseCheatEntry(const stdstr & CheatEntry, const stdstr& CheatOptions, CEnhancement & Enhancement)
{
    size_t StartOfName = CheatEntry.find("\"");
    if (StartOfName == std::string::npos)
    {
        return false;
    }
    size_t EndOfName = CheatEntry.find("\"", StartOfName + 1);
    if (EndOfName == std::string::npos)
    {
        return false;
    }
    Enhancement.SetName(CheatEntry.substr(StartOfName + 1, EndOfName - StartOfName - 1).c_str());
    const char * CheatString = &CheatEntry.c_str()[EndOfName + 2];

    CEnhancement::CodeOptions Options;
    std::string OptionValue;

    CEnhancement::CodeEntries Entries;
    const char * ReadPos = CheatString;
    while (ReadPos)
    {
        uint32_t CodeCommand = strtoul(ReadPos, 0, 16);
        ReadPos = strchr(ReadPos, ' ');
        if (ReadPos == nullptr)
        {
            break;
        }
        ReadPos += 1;
        std::string ValueStr = ReadPos;
        const char * ValuePos = ReadPos;
        ReadPos = strchr(ReadPos, ',');
        if (ReadPos != nullptr)
        {
            ValueStr.resize(ReadPos - ValuePos);
            ReadPos++;
        }

        CEnhancement::CodeEntry Entry;
        Entry.Command = CodeCommand;
        Entry.Value = ValueStr;
        Entries.push_back(Entry);
    }
    Enhancement.SetEntries(Entries);

    uint32_t OptionLen = Enhancement.CodeOptionSize();
    if (!CheatOptions.empty())
    {
        ReadPos = strchr(CheatOptions.c_str(), '$');
        if (ReadPos)
        {
            ReadPos += 1;
            do
            {
                const char* End = strchr(ReadPos, ',');
                std::string Item = End != nullptr ? std::string(ReadPos, End - ReadPos) : ReadPos;
                ReadPos = strchr(ReadPos, '$');
                if (ReadPos != nullptr)
                {
                    ReadPos += 1;
                }
                const char* Name = strchr(Item.c_str(), ' ');
                if (Name == nullptr)
                {
                    return false;
                }
                if (((uint32_t)(Name - Item.c_str())) != OptionLen)
                {
                    return false;
                }
                Name += 1;
                CEnhancement::CodeOption Option;
                Option.Name = stdstr(Name).Trim().c_str();
                if (OptionLen == 4)
                {
                    Option.Value = (uint16_t)strtoul(Item.c_str(), 0, 16);
                }
                else if (OptionLen == 2)
                {
                    Option.Value = (uint8_t)strtoul(Item.c_str(), 0, 16);
                }
                else
                {
                    return false;
                }
                Options.push_back(Option);
            } while (ReadPos);
        }
    }
    Enhancement.SetOptions(Options);
    return true;
}

void ConvertNew(const char * Src, const char * Dest)
{
    CPath SrcDir(Src, ""), DestDir(Dest, "");
    if (SrcDir == DestDir)
    {
        return;
    }

    if (DestDir.DirectoryExists())
    {
        DestDir.Delete();
    };
    DestDir.DirectoryCreate();

    CPath SearchDir(Src, "*.cht");
    if (SearchDir.FindFirst())
    {
        do
        {
            CPath OutFile(Dest, SearchDir.GetNameExtension());
            if (OutFile.Exists())
            {
                OutFile.Delete();
            }
            CEnhancmentFile DstEnhancmentFile(OutFile, "Cheat");

            CIniFile SrcIniFile(SearchDir);
            CIniFile::SectionList Sections;
            SrcIniFile.GetVectorOfSections(Sections);

            enum
            {
                MaxCheats = 50000,
            };

            for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
            {
                const char * Section = SectionItr->c_str();
                std::string GameName = SrcIniFile.GetString(Section, "Name", "");
                DstEnhancmentFile.SetName(Section, GameName.c_str());
                for (uint32_t cheat = 0; cheat < MaxCheats; cheat++)
                {
                    std::string CheatEntry = SrcIniFile.GetString(Section, stdstr_f("Cheat%d", cheat).c_str(), "");
                    if (CheatEntry.empty())
                    {
                        break;
                    }
                    std::string CheatOptions = SrcIniFile.GetString(Section, stdstr_f("Cheat%d_O", cheat).c_str(), "");
                    CEnhancement Enhancement("Cheat");
                    if (!ParseCheatEntry(CheatEntry, CheatOptions, Enhancement))
                    {
                        continue;
                    }
                    std::string CheatNote = SrcIniFile.GetString(Section, stdstr_f("Cheat%d_N", cheat).c_str(), "");
                    if (!CheatNote.empty())
                    {
                        Enhancement.SetNote(CheatNote.c_str());
                    }
                    if (!Enhancement.Valid())
                    {
                        DebugBreak();
                    }
                    DstEnhancmentFile.AddEnhancement(Section, Enhancement);
                }
            }
        } while (SearchDir.FindNext());
    }
}

void ConvertOld(const char * Src, const char * Dest)
{
    CPath SrcDir(Src, ""), DestDir(Dest, "");
    if (SrcDir == DestDir)
    {
        return;
    }

    if (DestDir.DirectoryExists())
    {
        DestDir.Delete();
    };
    DestDir.DirectoryCreate();

    CPath SearchDir(Src, "*.cht");
    if (SearchDir.FindFirst())
    {
        do
        {
            CPath OutFile(Dest, SearchDir.GetNameExtension());
            if (OutFile.Exists())
            {
                OutFile.Delete();
            }
            CEnhancmentFile SrcFile(SearchDir, "Cheat");
            CEnhancmentFile::SectionList Sections;
            SrcFile.GetSections(Sections);

            CIniFile DstIniFile(OutFile);
            DstIniFile.SetCustomSort(CustomSortData);
            for (CIniFile::SectionList::const_iterator SectionItr = Sections.begin(); SectionItr != Sections.end(); SectionItr++)
            {
                const char * Section = SectionItr->c_str();
                std::string GameName;
                if (SrcFile.GetName(Section, GameName))
                {
                    DstIniFile.SaveString(Section, "Name", GameName.c_str());
                }

                CEnhancementList Enhancements;
                if (SrcFile.GetEnhancementList(Section, Enhancements))
                {
                    uint32_t CheatEntry = 0;
                    for (CEnhancementList::const_iterator itr = Enhancements.begin(); itr != Enhancements.end(); itr++)
                    {
                        const CEnhancement & Enhancement = itr->second;
                        if (!Enhancement.Valid())
                        {
                            continue;
                        }
                        std::string Entry = stdstr_f("\"%s\"", Enhancement.GetName().c_str());
                        const CEnhancement::CodeEntries & Entries = Enhancement.GetEntries();
                        for (size_t i = 0, n = Entries.size(); i < n; i++)
                        {
                            Entry += stdstr_f(",%X %s", Entries[i].Command, Entries[i].Value.c_str());
                        }
                        DstIniFile.SaveString(Section, stdstr_f("Cheat%d", CheatEntry).c_str(), Entry.c_str());

                        if (!Enhancement.GetNote().empty())
                        {
                            DstIniFile.SaveString(Section, stdstr_f("Cheat%d_N", CheatEntry).c_str(), Enhancement.GetNote().c_str());
                        }

                        const CEnhancement::CodeOptions & Options = Enhancement.GetOptions();
                        if (Options.size() > 0)
                        {
                            std::string OptionEntry;
                            for (size_t i = 0, n = Options.size(); i < n; i++)
                            {
                                if (!OptionEntry.empty())
                                {
                                    OptionEntry += ",";
                                }
                                OptionEntry += stdstr_f(Enhancement.CodeOptionSize() == 2 ? "$%02X %s" : "$%04X %s", Options[i].Value, Options[i].Name.c_str());
                            }
                            DstIniFile.SaveString(Section, stdstr_f("Cheat%d_O", CheatEntry).c_str(), OptionEntry.c_str());
                        }
                        CheatEntry += 1;
                    }
                }
            }
        } while (SearchDir.FindNext());
    }
}

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpszArgs*/, int /*nWinMode*/)
{
    if (__argc == 4 && strcmp(__argv[1], "-split") == 0 && CPath(__argv[2]).Exists())
    {
        SplitFile(__argv[2], __argv[3]);
    }
    if (__argc == 4 && strcmp(__argv[1], "-join") == 0 && CPath(__argv[2],"").DirectoryExists())
    {
        JoinFile(__argv[2], __argv[3]);
    }
    if (__argc == 4 && strcmp(__argv[1], "-updateNames") == 0 && CPath(__argv[2], "").DirectoryExists() && CPath(__argv[3]).Exists())
    {
        UpdateNames(__argv[2], __argv[3]);
    }
    if (__argc == 3 && strcmp(__argv[1], "-convertGS") == 0 && CPath(__argv[2], "").DirectoryExists())
    {
        convertGS(__argv[2]);
    }
    if (__argc == 4 && strcmp(__argv[1], "-convertNew") == 0 && CPath(__argv[2], "").DirectoryExists())
    {
        ConvertNew(__argv[2], __argv[3]);
    }
    if (__argc == 4 && strcmp(__argv[1], "-convertOld") == 0 && CPath(__argv[2], "").DirectoryExists())
    {
        ConvertOld(__argv[2], __argv[3]);
    }
    return 0;
}
