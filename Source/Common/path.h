#pragma once
#include <string>
#include "stdtypes.h"

class CPathException
{
public:
    uint32_t m_dwErrorCode;

public:
    CPathException(uint32_t code = 0) : m_dwErrorCode(code) {}
};

class CPath
{
    //Enums
public:

    enum DIR_CURRENT_DIRECTORY   { CURRENT_DIRECTORY = 1 };
    enum DIR_MODULE_DIRECTORY { MODULE_DIRECTORY = 2 };
    enum DIR_MODULE_FILE { MODULE_FILE = 3 };

    enum 
    {
        FIND_ATTRIBUTE_ALLFILES = 0xFFFF,  // Search Include all files
        FIND_ATTRIBUTE_FILES    = 0x0000,  // File can be read or written to without restriction
        FIND_ATTRIBUTE_SUBDIR   = 0x0010,  // Subdirectories
    };    

    //Attributes
private:

    std::string	m_strPath;
    uint32_t   m_dwFindFileAttributes;
    void *	m_hFindFile;
    static void * m_hInst;

public:
    //Methods

    //Construction / destruction
    CPath();
    CPath(const CPath& rPath);
    CPath(const char * lpszPath);
    CPath(const char * lpszPath, const char * NameExten);
    CPath(const char * lpszPath, const std::string & NameExten);
    CPath(const std::string& strPath);
    CPath(const std::string& strPath, const char * NameExten);
    CPath(const std::string& strPath, const std::string& NameExten);

    CPath(DIR_CURRENT_DIRECTORY sdt, const char * NameExten = NULL);
    CPath(DIR_MODULE_DIRECTORY sdt, const char * NameExten = NULL);
    CPath(DIR_MODULE_FILE sdt);

    virtual ~CPath();

    //Setup & Cleanup
    inline void Init();
    inline void Exit();

    //Operators
    CPath& operator  = (const CPath& rPath);
    CPath& operator  = (const char * lpszPath);
    CPath& operator  = (const std::string & strPath);
    bool   operator == (const CPath& rPath) const;
    bool   operator != (const CPath& rPath) const;
    operator const char *() const;
    operator const std::string &() { return m_strPath; }

    //Get path components
    void   GetDriveDirectory(std::string & rDriveDirectory) const;
    std::string GetDriveDirectory(void) const;
    void   GetDirectory(std::string& rDirectory) const;
    std::string GetDirectory(void) const;
    void   GetName(std::string& rName) const;
    std::string GetName(void) const;
    void   GetNameExtension(std::string& rNameExtension) const;
    std::string GetNameExtension(void) const;
    void   GetExtension(std::string& rExtension) const;
    std::string GetExtension(void) const;
    void   GetLastDirectory(std::string& rDirectory) const;
    std::string GetLastDirectory(void) const;
    void GetFullyQualified(std::string& rFullyQualified) const;
	void GetComponents(std::string* pDrive = NULL, std::string* pDirectory = NULL, std::string* pName = NULL, std::string* pExtension = NULL) const;
    //Get other state
    bool IsEmpty() const { return m_strPath.empty(); }
    bool IsRelative() const;

    //Set path components
    void SetDrive(char chDrive);
    void SetDriveDirectory(const char * lpszDriveDirectory);
    void SetDirectory(const char * lpszDirectory, bool bEnsureAbsolute = false);
    void SetName(const char * lpszName);
    void SetName(int iName);
    void SetNameExtension(const char * lpszNameExtension);
    void SetExtension(const char * lpszExtension);
    void SetExtension(int iExtension);
    void AppendDirectory(const char * lpszSubDirectory);
    void UpDirectory(std::string* pLastDirectory = NULL);
	void SetComponents(const char * lpszDrive, const char * lpszDirectory, const char * lpszName, const char * lpszExtension);
    //Set whole path
    void Empty()		{ m_strPath.erase(); }
    void CurrentDirectory();
    void Module();
    void Module(void * hInstance);
    void ModuleDirectory();
    void ModuleDirectory(void * hInstance);

    //Directory information
    bool IsDirectory() const;
    bool DirectoryExists() const;

    //File Information
    bool     IsFile() const { return !IsDirectory(); }
    bool     Exists() const;

    //Directory operations
    bool DirectoryCreate(bool bCreateIntermediates = true);
    bool ChangeDirectory();

    //File operations
    bool Delete(bool bEvenIfReadOnly = true) const;
    bool CopyTo(const char * lpcszTargetFile, bool bOverwrite = true);
    bool MoveTo(const char * lpcszTargetFile, bool bOverwrite = true);

    //Finders
    bool FindFirst(uint32_t dwAttributes = 0);
    bool FindNext();

    // Helpers
    static void SethInst(void * hInst);
    static void * GethInst();

private:
    bool AttributesMatch(uint32_t dwTargetAttributes, uint32_t dwFileAttributes);

    void cleanPathString(std::string& rDirectory) const;
    void StripLeadingChar(std::string& rText, char chLeading) const;
    void StripLeadingBackslash(std::string& Directory)  const;
    void StripTrailingChar(std::string& rText, char chTrailing) const;
    void StripTrailingBackslash(std::string& rDirectory) const;
    void EnsureTrailingBackslash(std::string& Directory) const;
    void EnsureLeadingBackslash(std::string& Directory) const;
};
