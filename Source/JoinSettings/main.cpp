#include <Common/path.h>
#include <Common/IniFileClass.h>
#include <Common/StdString.h>
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
            int number_len = strlen(_itoa(i1, Buffer, 10));
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

    for (size_t i = 0, n = Sections.size(); i < n; i++)
    {
        const char * Section = Sections[i].c_str();

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
    stdstr_f LineData = stdstr_f("//--------------- %s Region Cheat Codes ---------------\r\n\r\n", Region);
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
        for (size_t i = 0, n = Sections.size(); i < n; i++)
        {
            const char * Section = Sections[i].c_str();
            const char * pos = strstr(Section, searchStr.c_str());
            if (pos == NULL)
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

        for (size_t i = 0, n = Sections.size(); i < n; i++)
        {
            const char * Section = Sections[i].c_str();

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
            for (size_t i = 0, n = Sections.size(); i < n; i++)
            {
                const char * Section = Sections[i].c_str();
                stdstr Name = GameIniFile.GetString(Section, "Name", Section);
                Name.Trim("\t =");
                if (Name.size() > 0)
                {
                    files.insert(Files::value_type(Name, SearchDir));
                }
                break;
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
        stdstr_f LineData = stdstr_f("// Project64 Official Cheats Database\r\n");
        TargetIniFile.Write(LineData.c_str(), (int)LineData.length());
        LineData = stdstr_f("// Not for use with PJ64 v1.6 or previous\r\n");
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
            for (size_t i = 0, n = Sections.size(); i < n; i++)
            {
                const char * Section = Sections[i].c_str();
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
    return 0;
}
