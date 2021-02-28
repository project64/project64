#pragma once

#ifndef _WIN32
#include <strings.h>
#endif

#include "stdafx.h"
#include "FileClass.h"
#include "CriticalSection.h"
#include <string>
#include <map>
#include <vector>
#include <list>
#include <set>
#include <memory>

class CIniFileBase
{
public:
    typedef std::map<std::string, std::string> KeyValueData;
    typedef std::set<std::string> SectionList;
    typedef std::list<std::string> strlist;
    typedef std::pair<const std::string *, const std::string *> KeyValueItem;
    typedef std::vector<KeyValueItem> KeyValueVector;
    typedef void(*SortData)(KeyValueVector &);

    CIniFileBase(CFileBase & FileObject, const char * FileName);
    virtual ~CIniFileBase(void);

    bool IsEmpty();
    bool IsFileOpen(void);
    bool DeleteSection(const char * lpSectionName);
    bool GetString(const char * lpSectionName, const char * lpKeyName, const char * lpDefault, std::string & Value);
    std::string GetString(const char * lpSectionName, const char * lpKeyName, const char * lpDefault);
    uint32_t GetString(const char * lpSectionName, const char * lpKeyName, const char * lpDefault, char * lpReturnedString, uint32_t nSize);
    uint32_t GetNumber(const char * lpSectionName, const char * lpKeyName, uint32_t nDefault);
    bool GetNumber(const char * lpSectionName, const char * lpKeyName, uint32_t nDefault, uint32_t & Value);

    virtual void SaveString(const char * lpSectionName, const char * lpKeyName, const char * lpString);
    virtual void SaveNumber(const char * lpSectionName, const char * lpKeyName, int32_t Value);
    void SetAutoFlush(bool AutoFlush);
    void FlushChanges(void);
    bool EntryExists(const char * lpSectionName, const char * lpKeyName);
    void GetKeyList(const char * lpSectionName, strlist &List);
    void GetKeyValueData(const char * lpSectionName, KeyValueData & List);
    void SetCustomSort(SortData SortFunction);

    void GetVectorOfSections(SectionList & sections);
    const std::string &GetFileName() { return m_FileName; }

protected:
    void OpenIniFileReadOnly();
    void OpenIniFile(bool bCreate = true);
    void SaveCurrentSection(void);

    std::string FormatStr(const char * strFormat, ...);

    CFileBase & m_File;
    std::string m_FileName;

private:
    struct insensitive_compare
    {
        bool operator() (const std::string & a, const std::string & b) const
        {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        }
    };

    typedef std::map<std::string, long> FILELOC;
    typedef FILELOC::iterator FILELOC_ITR;
    typedef std::map<std::string, std::string, insensitive_compare> KeyValueList;

    std::string m_CurrentSection;
    bool m_CurrentSectionDirty;
    int m_CurrentSectionFilePos; // Where in the file is the current Section
    KeyValueList m_CurrentSectionData;

    long m_lastSectionSearch; // When Scanning for a section, what was the last scanned pos

    bool m_ReadOnly;
    bool m_InstantFlush;
    const char * m_LineFeed;

    CriticalSection m_CS;
    FILELOC m_SectionsPos;
    SortData m_SortFunction;

    void fInsertSpaces(int Pos, int NoOfSpaces);
    int GetStringFromFile(char * & String, std::unique_ptr<char> &Data, int & MaxDataSize, int & DataSize, int & ReadPos);
    bool MoveToSectionNameData(const char * lpSectionName, bool ChangeCurrentSection);
    const char * CleanLine(char * Line);
    void ClearSectionPosList(long FilePos);

};

template <class CFileStorage>
class CIniFileT :
    public CIniFileBase
{
public:
    CIniFileT(const char * FileName) :
        CIniFileBase(m_FileObject, FileName)
    {
        //Try to open file for reading
        OpenIniFile();
    }

    CIniFileT(const char * FileName, bool bCreate, bool bReadOnly) :
        CIniFileBase(m_FileObject, FileName)
    {
        if (bReadOnly)
        {
            OpenIniFileReadOnly();
        }
        else
        {
            //Try to open file for reading
            OpenIniFile(bCreate);
        }
    }
    virtual ~CIniFileT(void)
    {
        SaveCurrentSection();
    }

protected:
    CFileStorage  m_FileObject;
};

typedef CIniFileT<CFile> CIniFile;
