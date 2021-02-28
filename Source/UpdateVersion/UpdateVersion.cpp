#include <stdlib.h>
#include <memory>
#include <Common/path.h>
#include <Common/FileClass.h>
#include <Common/StdString.h>

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

    strvector verinfo = stdstr(__argv[3]).Tokenize('-');
    if (verinfo.size() < 3 || verinfo.size() > 4)
    {
        return 0;
    }
    if (verinfo.size() == 4)
    {
        verinfo[2] += "-" + verinfo[3];
    }

    CFile OutFile(DestFile, CFileBase::modeWrite | CFileBase::modeCreate);
    if (!OutFile.IsOpen())
    {
        return 0;
    }

    for (size_t i = 0, n = VersionData.size() - 1; i < n; i++)
    {
        stdstr &line = VersionData[i];
        line += "\n";
        if (_strnicmp(line.c_str(), "#define VERSION_BUILD", 21) == 0)
        {
            line = "#define VERSION_BUILD               " + verinfo[1] + "\n";
        }
        if (_strnicmp(line.c_str(), "#define GIT_VERSION", 18) == 0)
        {
            line = "#define GIT_VERSION                 \"" + verinfo[2] + "\"\n";
        }
        if (!OutFile.Write(line.c_str(), (uint32_t)line.length()))
        {
            return 0;
        }
    }
    return 0;
}