#include <Common/path.h>
#include <Common/File.h>
#include <Common/StdString.h>
#include <Common/md5.h>
#include <stdlib.h>
#include <memory>
#include <time.h>

bool GitCommand(CPath & SourceDirectory, const char * Command, std::string & Output)
{
    CPath CurrentDir(CPath::CURRENT_DIRECTORY);
    if (CurrentDir != SourceDirectory)
    {
        SourceDirectory.ChangeDirectory();
    }
    Output.clear();
    FILE * pipe = _popen(stdstr_f("git %s", Command).c_str(), "r");
    if (pipe == nullptr)
    {
        if (CurrentDir != SourceDirectory)
        {
            CurrentDir.ChangeDirectory();
        }
        return false;
    }

    char buffer[128];
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
        {
            Output += buffer;
        }
    }
    if (CurrentDir != SourceDirectory)
    {
        CurrentDir.ChangeDirectory();
    }
    if (feof(pipe))
    {
        _pclose(pipe);
        return true;
    }
    return false;
}

uint32_t GitBuildVersion(CPath & SourceDirectory)
{
    enum
    {
        DefaultBuildVersion = 9999
    };
    std::string Result;
    if (!GitCommand(SourceDirectory, "rev-list --count HEAD", Result))
    {
        return DefaultBuildVersion;
    }
    if (Result.empty())
    {
        return DefaultBuildVersion;
    }
    uint32_t BuildVersion = atoi(Result.c_str());
    if (BuildVersion != 0)
    {
        return BuildVersion;
    }
    return 9999;
}

bool GitBuildDirty(CPath & SourceDirectory)
{
    std::string Result;
    if (!GitCommand(SourceDirectory, "diff --stat", Result))
    {
        return false;
    }
    return !Result.empty();
}

std::string GitRevision(CPath & SourceDirectory)
{
    stdstr Result;
    if (!GitCommand(SourceDirectory, "rev-parse HEAD", Result))
    {
        return "";
    }
    Result.Replace("\r", "");
    strvector ResultVector = Result.Tokenize("\n");
    if (ResultVector.size() > 0)
    {
        return ResultVector[0];
    }
    return "";
}

std::string GitRevisionShort(CPath & SourceDirectory)
{
    stdstr Result;
    if (!GitCommand(SourceDirectory, "rev-parse --short HEAD", Result))
    {
        return "";
    }
    Result.Replace("\r", "");
    strvector ResultVector = Result.Tokenize("\n");
    if (ResultVector.size() > 0)
    {
        return ResultVector[0];
    }
    return "";
}

int main()
{
    if (__argc < 3)
    {
        return 0;
    }

    CPath SourceFile(__argv[1]), DestFile(__argv[2]);
    if (!SourceFile.Exists())
    {
        return 0;
    }

    MD5Digest DestHash;
    if (DestFile.Exists())
    {
        CFile OutFile(DestFile, CFileBase::modeRead);
        if (OutFile.IsOpen())
        {
            uint32_t OutFileLen = OutFile.GetLength();
            std::unique_ptr<uint8_t[]> data(new uint8_t[OutFileLen]);
            if (OutFile.Read(data.get(), OutFileLen) != 0)
            {
                MD5(data.get(), OutFileLen).get_digest(DestHash);
            }
        }
    }

    CPath SourceDirectory(SourceFile.GetDriveDirectory(), "");
    uint32_t VersionBuild = GitBuildVersion(SourceDirectory);
    bool BuildDirty = GitBuildDirty(SourceDirectory);
    std::string Revision = GitRevision(SourceDirectory);
    std::string RevisionShort = GitRevisionShort(SourceDirectory);

    CFile InFile(SourceFile, CFileBase::modeRead);
    if (!InFile.IsOpen())
    {
        return 0;
    }
    InFile.SeekToBegin();
    uint32_t FileLen = InFile.GetLength();
    std::auto_ptr<uint8_t> InputData(new uint8_t[FileLen]);
    InFile.Read(InputData.get(), FileLen);
    strvector VersionData = stdstr(std::string((char *)InputData.get(), FileLen)).Tokenize("\n");

    uint32_t VersionMajor = 0, VersionMinor = 0, VersionRevison = 0;
    std::string VersionPrefix;
    for (size_t i = 0, n = VersionData.size(); i < n; i++)
    {
        stdstr line = VersionData[i];
        line.Trim();

        if (_strnicmp(line.c_str(), "VersionMajor = ", 13) == 0)
        {
            VersionMajor = atoi(&(line.c_str()[14]));
        }
        else if (_strnicmp(line.c_str(), "VersionMinor = ", 13) == 0)
        {
            VersionMinor = atoi(&(line.c_str()[14]));
        }
        else if (_strnicmp(line.c_str(), "VersionRevison = ", 17) == 0)
        {
            VersionRevison = atoi(&(line.c_str()[17]));
        }
        else if (_strnicmp(line.c_str(), "VersionPrefix = ", 15) == 0)
        {
            size_t StartPrefix = line.find('\"', 15);
            size_t EndPrefix = std::string::npos;
            if (StartPrefix != std::string::npos)
            {
                EndPrefix = line.find('\"', StartPrefix + 1);
            }
            if (EndPrefix != std::string::npos)
            {
                VersionPrefix = line.substr(StartPrefix + 1, EndPrefix - (StartPrefix + 1));
            }
        }
        else
        {
            continue;
        }
        VersionData.erase(VersionData.begin() + i);
        i -= 1;
        n -= 1;
    }
    
    std::string OutData;
    for (size_t i = 0, n = VersionData.size(); i < n; i++)
    {
        stdstr &line = VersionData[i];
        line += "\n";
        if (_strnicmp(line.c_str(), "#define GIT_VERSION ", 20) == 0)
        {
            line.Format("#define GIT_VERSION                 \"%s%s%s\"\n", RevisionShort.c_str(), BuildDirty ? "-" : "", BuildDirty ? "Dirty" : "");
        }
        else if (_strnicmp(line.c_str(), "#define VERSION_BUILD ", 22) == 0)
        {
            line.Format("#define VERSION_BUILD               %d\n", VersionBuild);
        }
        else if (_strnicmp(line.c_str(), "#define VERSION_BUILD_YEAR ", 27) == 0)
        {
            time_t now = time(NULL);
            struct tm * tnow = gmtime(&now);

            line.Format("#define VERSION_BUILD_YEAR          %d\n", tnow->tm_year + 1900);
        }
        else if (_strnicmp(line.c_str(), "#define GIT_REVISION ", 21) == 0)
        {
            line.Format("#define GIT_REVISION                \"%s\"\n", Revision.c_str());
        }
        else if (_strnicmp(line.c_str(), "#define GIT_REVISION_SHORT ", 26) == 0)
        {
            line.Format("#define GIT_REVISION_SHORT          \"%s\"\n", RevisionShort.c_str());
        }
        else if (_strnicmp(line.c_str(), "#define GIT_DIRTY ", 11) == 0)
        {
            line.Format("#define GIT_DIRTY                   \"%s\"\n", BuildDirty ? "Dirty" : "");
        }
        else if (_strnicmp(line.c_str(), "        versionCode = ", 22) == 0)
        {
            line.Format("        versionCode = %d\n", VersionBuild);
        }
        else if (_strnicmp(line.c_str(), "        versionName = ", 22) == 0)
        {
            line.Format("        versionName = \"%d.%d.%d.%d", VersionMajor, VersionMinor, VersionRevison, VersionBuild);
            if (VersionPrefix.length() > 0)
            {
                line += stdstr_f(" (%s)", VersionPrefix.c_str());
            }
            line += "\"\n";
        }
        OutData += line.c_str();
    }

    MD5Digest OutHash;
    MD5((const uint8_t *)OutData.c_str(), (unsigned int)OutData.length()).get_digest(OutHash);

    if (memcmp(OutHash.digest, DestHash.digest, sizeof(DestHash.digest)) != 0)
    {
        if (DestFile.Exists() && !DestFile.Delete())
        {
            return 0;
        }

        CFile OutFile(DestFile, CFileBase::modeWrite | CFileBase::modeCreate);
        if (!OutFile.IsOpen())
        {
            return 0;
        }
        OutFile.Write(OutData.c_str(), (uint32_t)OutData.length());
    }
    return 0;
}
