#pragma once

#ifndef _WIN32
/* for POSIX method away from Win32 _stricmp--see "Platform.h" */
#include <strings.h>
#endif
#include "Platform.h"

#include "FileClass.h"
#include "CriticalSection.h"
#include "StdString.h"
#include <map>

class CIniFileBase
{
    struct insensitive_compare
    {
        bool operator() (const std::string & a, const std::string & b) const
        {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        }
    };

    typedef std::string ansi_string;
    typedef std::map<ansi_string, long> FILELOC;
    typedef FILELOC::iterator FILELOC_ITR;
    typedef std::map<ansi_string, ansi_string, insensitive_compare> KeyValueList;

public:
    typedef std::map<stdstr, stdstr>           KeyValueData;
    typedef std::vector<stdstr>               SectionList;

protected:
    CFileBase   & m_File;
    stdstr m_FileName;

private:
    ansi_string m_CurrentSection;
    bool   m_CurrentSectionDirty;
    int    m_CurrentSectionFilePos; // Where in the file is the current Section
    KeyValueList m_CurrentSectionData;

    long   m_lastSectionSearch; // When Scanning for a section, what was the last scanned pos

    bool   m_ReadOnly;
    bool   m_InstantFlush;
    const char * m_LineFeed;

    CriticalSection m_CS;
    FILELOC m_SectionsPos;

    //void AddItemData ( const char * lpKeyName, const char * lpString);
    //bool ChangeItemData ( const char * lpKeyName, const char * lpString );
    //void DeleteItem ( const char * lpKeyName );
    void fInsertSpaces(int Pos, int NoOfSpaces);
    int  GetStringFromFile(char * & String, char * &Data, int & MaxDataSize, int & DataSize, int & ReadPos);
    bool MoveToSectionNameData(const char * lpSectionName, bool ChangeCurrentSection);
    const char * CleanLine(char * const Line);
    void ClearSectionPosList(long FilePos);

protected:
    void OpenIniFileReadOnly();
    void OpenIniFile(bool bCreate = true);
    void SaveCurrentSection(void);

public:
    CIniFileBase(CFileBase & FileObject, const char * FileName);
    virtual ~CIniFileBase(void);

    bool IsEmpty();
    bool IsFileOpen(void);
    bool DeleteSection(const char * lpSectionName);
    bool GetString(const char * lpSectionName, const char * lpKeyName, const char * lpDefault, stdstr & Value);
    stdstr GetString(const char * lpSectionName, const char * lpKeyName, const char * lpDefault);
    uint32_t GetString(const char * lpSectionName, const char * lpKeyName, const char * lpDefault, char * lpReturnedString, uint32_t nSize);
    uint32_t GetNumber(const char * lpSectionName, const char * lpKeyName, uint32_t nDefault);
    bool  GetNumber(const char * lpSectionName, const char * lpKeyName, uint32_t nDefault, uint32_t & Value);

#ifdef _UNICODE
    bool DeleteSection ( LPCWSTR lpSectionName );
    bool GetString ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault, stdstr & Value );
    stdstr GetString  ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault );
    uint32_t  GetString  ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPTSTR lpReturnedString, uint32_t nSize );
    uint32_t GetNumber ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, uint32_t nDefault );
    bool  GetNumber ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, uint32_t nDefault, uint32_t & Value );

#endif

    virtual void  SaveString(const char * lpSectionName, const char * lpKeyName, const char * lpString);
    virtual void  SaveNumber(const char * lpSectionName, const char * lpKeyName, uint32_t Value);
    void SetAutoFlush(bool AutoFlush);
    void FlushChanges(void);
    bool EntryExists(const char * lpSectionName, const char * lpKeyName);
    void GetKeyList(const char * lpSectionName, strlist &List);
    void GetKeyValueData(const char * lpSectionName, KeyValueData & List);

    void GetVectorOfSections(SectionList & sections);
    const stdstr &GetFileName() { return m_FileName; }
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

typedef CIniFileT<CFile>   CIniFile;
