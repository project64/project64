#include <stdlib.h>
#include <memory>
#include <Common/path.h>
#include <Common/File.h>
#include <Common/StdString.h>

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
        // use buffer to read and add to result
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
    if (__argc < 4)
    {
        return 0;
    }

    CPath SourceFile(__argv[1]), DestFile(__argv[2]);
    if (!SourceFile.Exists())
    {
        return 0;
    }

    if (DestFile.Exists())
    {
        if (!DestFile.Delete())
        {
            return 0;
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

    CFile OutFile(DestFile, CFileBase::modeWrite | CFileBase::modeCreate);
    if (!OutFile.IsOpen())
    {
        return 0;
    }

    for (size_t i = 0, n = VersionData.size() - 1; i < n; i++)
    {
        stdstr &line = VersionData[i];
        line += "\n";
        if (_strnicmp(line.c_str(), "#define GIT_VERSION ", 20) == 0)
        {
            line.Format("#define GIT_VERSION                 \"%s%s%s\"\n", RevisionShort.c_str(), BuildDirty ? "-" : "", BuildDirty ? "Dirty" : "");
        }
        else if (_strnicmp(line.c_str(), "#define VERSION_BUILD", 21) == 0)
        {
            line.Format("#define VERSION_BUILD               %d\n", VersionBuild);
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
        if (!OutFile.Write(line.c_str(), (uint32_t)line.length()))
        {
            return 0;
        }
    }
    return 0;
}
