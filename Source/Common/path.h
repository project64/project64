//////////////////////////////////////////////////////////////////////
//
// Handy routines to help with file & path management
//
// This class is used to represent pathnames, that is the name and 
// location of a file. CPaths are used when you want to refer to a file
// as a whole, or to the location of a file.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PATH_H__6DD6923B_E241_40CE_81A3_4C2C88C140E4__INCLUDED_)
#define AFX_PATH_H__6DD6923B_E241_40CE_81A3_4C2C88C140E4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "std string.h"
#include <sys\types.h>
#include <dos.h>
#include <WTypes.h>

class CPathException
{
public:
    ULONG m_dwErrorCode;

public:
    CPathException(ULONG code =0): m_dwErrorCode(code) {}
};

class CPath  
{
//Enums
public:

	enum DIR_CURRENT_DIRECTORY   { CURRENT_DIRECTORY   = 1 };
	enum DIR_MODULE_DIRECTORY { MODULE_DIRECTORY = 2 };
	enum DIR_MODULE_FILE { MODULE_FILE = 3 };

	enum { _A_ALLFILES = 0xFFFF };    /* Search Include all files */

//Attributes
private:	
	
	stdstr	m_strPath;
	ULONG   m_dwFindFileAttributes;
	HANDLE	m_hFindFile;
	static HINSTANCE m_hInst;

public:
//Methods

	//Construction / destruction
	CPath();
    CPath(const CPath& rPath);
    CPath(LPCTSTR lpszPath);
	CPath(LPCTSTR lpszPath, LPCTSTR NameExten);
	CPath(LPCTSTR lpszPath, const stdstr& NameExten);
    CPath(const stdstr& strPath);
	CPath(const stdstr& strPath, LPCTSTR NameExten);
	CPath(const stdstr& strPath, const stdstr& NameExten);

	CPath(DIR_CURRENT_DIRECTORY sdt, LPCTSTR NameExten = NULL);
	CPath(DIR_MODULE_DIRECTORY sdt, LPCTSTR NameExten = NULL);
	CPath(DIR_MODULE_FILE sdt);
	
	virtual ~CPath();

	//Setup & Cleanup
    inline void Init();
	inline void Exit();

	//Operators
    CPath& operator  = (const CPath& rPath);
    CPath& operator  = (LPCTSTR lpszPath);
    CPath& operator  = (const stdstr& strPath);
    BOOL   operator == (const CPath& rPath) const;
    BOOL   operator != (const CPath& rPath) const;
    operator LPCTSTR() const;
	operator stdstr&() { return m_strPath; }
	
	//Get path components
	void   GetDriveDirectory(stdstr& rDriveDirectory) const;
	stdstr GetDriveDirectory(void) const;
	void   GetDirectory(stdstr& rDirectory) const;
	stdstr GetDirectory(void) const;
	void   GetName(stdstr& rName) const;
	stdstr GetName(void) const;
	void   GetNameExtension(stdstr& rNameExtension) const;
	stdstr GetNameExtension(void) const;
	void   GetExtension(stdstr& rExtension) const;
	stdstr GetExtension(void) const;
	void   GetCurrentDirectory(stdstr& rDrive) const;
	stdstr GetCurrentDirectory(void) const;
    void GetFullyQualified(stdstr& rFullyQualified) const;
	void GetComponents(stdstr* pDrive     =NULL, 
                       stdstr* pDirectory =NULL, 
                       stdstr* pName      =NULL, 
                       stdstr* pExtension =NULL) const;

	//Get other state
    BOOL IsEmpty() const { return m_strPath.empty(); }
    BOOL IsRelative() const;

	//Set path components
    void SetDrive(TCHAR chDrive);
    void SetDriveDirectory(LPCTSTR lpszDriveDirectory);
    void SetDirectory(LPCTSTR lpszDirectory, BOOL bEnsureAbsolute =FALSE);
    void SetName(LPCTSTR lpszName);
    void SetName(int iName);
    void SetNameExtension(LPCTSTR lpszNameExtension);
    void SetExtension(LPCTSTR lpszExtension);
    void SetExtension(int iExtension);
	void AppendDirectory(LPCTSTR lpszSubDirectory);
	void UpDirectory(stdstr* pLastDirectory =NULL);
	void SetComponents(LPCTSTR lpszDrive, 
                       LPCTSTR lpszDirectory,
					   LPCTSTR lpszName, 
                       LPCTSTR lpszExtension);

	//Set whole path
    void Empty()		{ m_strPath.erase(); }
    void CurrentDirectory();
    void Module();
	void Module(HINSTANCE hInstance);
    void ModuleDirectory();
    void ModuleDirectory(HINSTANCE hInstance);

	//Directory information
    BOOL IsDirectory() const;
    BOOL DirectoryExists() const;

	//File Information
    BOOL     IsFile() const { return !IsDirectory(); }
    BOOL     Exists() const;

	//Directory operations
    BOOL CreateDirectory(BOOL bCreateIntermediates =TRUE);
    BOOL ChangeDirectory();
	    
	//File operations
	BOOL Delete(BOOL bEvenIfReadOnly =TRUE) const;
    BOOL CopyTo(LPCTSTR lpcszTargetFile, BOOL bOverwrite =TRUE);
    BOOL MoveTo(LPCTSTR lpcszTargetFile, BOOL bOverwrite =TRUE);

	//Finders
    BOOL FindFirst(ULONG dwAttributes =_A_NORMAL);
    BOOL FindNext();

	// Helpers
	static void SethInst ( HINSTANCE hInst );
	static HINSTANCE GethInst();

private:
    BOOL AttributesMatch(ULONG dwTargetAttributes, ULONG dwFileAttributes);

	void cleanPathString(stdstr& rDirectory) const;
	void StripLeadingChar(stdstr& rText, TCHAR chLeading) const;
	void StripLeadingBackslash(stdstr& Directory)  const;
	void StripTrailingChar(stdstr& rText, TCHAR chTrailing) const;
	void StripTrailingBackslash(stdstr& rDirectory) const;
	void EnsureTrailingBackslash(stdstr& Directory) const;
	void EnsureLeadingBackslash(stdstr& Directory) const;
};

#endif // !defined(AFX_PATH_H__6DD6923B_E241_40CE_81A3_4C2C88C140E4__INCLUDED_)
