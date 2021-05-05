#pragma once
#include <Project64-core/N64System/Enhancement/Enhancement.h>
#include <Project64-core/N64System/Enhancement/EnhancementList.h>
#include <Common/File.h>
#include <Common/CriticalSection.h>
#include <set>
#include <string>
#include <map>
#include <vector>
#include <memory>

class CEnhancmentFile
{
    struct insensitive_compare
    {
        bool operator() (const std::string & a, const std::string & b) const
        {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        }
    };

    typedef std::map<std::string, long> FILELOC;
    typedef FILELOC::iterator FILELOC_ITR;

public:
    typedef std::set<std::string> SectionList;

    CEnhancmentFile(const char * FileName, const char * Ident);
    ~CEnhancmentFile();

    void SetName(const char * Section, const char * Name);
    bool AddEnhancement(const char * Section, const CEnhancement & Details);
    void SaveCurrentSection(void);

    bool GetName(const char * Section, std::string & Name);
    bool GetEnhancementList(const char * Section, CEnhancementList & List);
    bool RemoveEnhancements(const char * Section);

    void GetSections(SectionList & sections);

    const char * FileName(void) const { return m_FileName.c_str(); }

private:
    CEnhancmentFile();
    CEnhancmentFile(const CEnhancmentFile&);
    CEnhancmentFile& operator=(const CEnhancmentFile&);

    bool AddEnhancement(const CEnhancement & Details);
    void OpenFile(void);
    bool MoveToSection(const char * Section, bool ChangeCurrentSection);
    int GetStringFromFile(char * & String, std::unique_ptr<char> & Data, int & MaxDataSize, int & DataSize, int & ReadPos);
    const char * CleanLine(char * Line);
    void fInsertSpaces(int32_t Pos, int32_t NoOfSpaces);
    void ClearSectionPosList(long FilePos);

    CriticalSection m_CS;
    std::string m_Ident;
    std::string m_CurrentSection;
    std::string m_SectionName;
    CEnhancementList m_EnhancementList;
    FILELOC m_SectionsPos;
    int32_t m_lastSectionSearch;
    int32_t m_CurrentSectionFilePos;
    bool m_CurrentSectionDirty;
    bool m_ReadOnly;
    CFile m_File;
    std::string m_FileName;
    const char * m_LineFeed;
};
