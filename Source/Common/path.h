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

class CPathException
{
public:
    DWORD m_dwErrorCode;

public:
    CPathException(DWORD code =0): m_dwErrorCode(code) {}
};

class CPath  
{
//Enums
public:

	enum SpecialDirectoryType
	{
		CURRENT_DIRECTORY,
		WINDOWS_DIRECTORY,
		SYSTEM_DIRECTORY,
		SYSTEM_DRIVER_DIRECTORY,
		SYSTEM_DRIVE_ROOT_DIRECTORY,
		MODULE_DIRECTORY,
		MODULE_FILE,
		TEMP_DIRECTORY,
		PROGRAM_FILES_DIRECTORY,
		COMMON_FILES_DIRECTORY,
		ACCESSORIES_DIRECTORY,
		MEDIA_DIRECTORY,
		DEVICE_DIRECTORY,
		USER_DESKTOP_DIRECTORY,
		USER_FAVORITES_DIRECTORY,
		USER_FONTS_DIRECTORY,
		USER_NETHOOD_DIRECTORY,
		USER_DOCUMENTS_DIRECTORY,
		USER_RECENT_DIRECTORY,
		USER_SENDTO_DIRECTORY,
		USER_TEMPLATES_DIRECTORY,
		USER_RECYCLE_DIRECTORY,
		USER_APPLICATION_DATA_DIRECTORY,
		USER_STARTMENU_DIRECTORY,
		USER_STARTMENU_STARTUP_DIRECTORY,
		USER_STARTMENU_PROGRAMS_DIRECTORY,
		COMMON_DESKTOP_DIRECTORY,
		COMMON_STARTMENU_DIRECTORY,
		COMMON_STARTMENU_STARTUP_DIRECTORY,
		COMMON_STARTMENU_PROGRAMS_DIRECTORY,
		LAST_SPECIAL
	};

	enum { _A_ALLFILES = 0xFFFF };    /* Search Include all files */

//Attributes
private:	
	
	stdstr	m_strPath;
	DWORD   m_dwFindFileAttributes;
	HANDLE	m_hFindFile;

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
	CPath(SpecialDirectoryType sdt);
	CPath(SpecialDirectoryType sdt, LPCTSTR NameExten );
	CPath(SpecialDirectoryType sdt, const stdstr& NameExten );
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
	void   GetDrive(stdstr& rDrive) const;
	stdstr GetDrive(void) const;
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
    void GetFullyQualifiedShort(stdstr& rFullyQualifiedShort) const;
	void GetComponents(stdstr* pDrive     =NULL, 
                       stdstr* pDirectory =NULL, 
                       stdstr* pName      =NULL, 
                       stdstr* pExtension =NULL) const;
	void GetAsInternetPath(stdstr& Directory) const;

	//Get other state
    BOOL IsEmpty() const { return m_strPath.empty(); }
    BOOL IsRelative() const;
    BOOL IsWild() const;
    BOOL IsValid() const; // Checks lexical correctness only

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
	void SpecialDirectory(SpecialDirectoryType sdt);
    void MakeRoot();
    void CurrentDirectory();
    void WindowsDirectory();
    void SystemDirectory();
    void SystemDriveRootDirectory();
    void TempDirectory();
    void Module();
	void Module(HINSTANCE hInstance);
    void ModuleDirectory();
    void ModuleDirectory(HINSTANCE hInstance);
    void ProgramFilesDirectory();   // C:\Program Files
    void CommonFilesDirectory();    // C:\Program Files\Common Files
    void AccessoriesDirectory();    // C:\Program Files\Accessories
    void MediaDirectory();          // C:\Windows\Media
    void DeviceDirectory();         // C:\Windows\Inf
    void UserDesktopDirectory();
    void UserFavoritesDirectory();
    void UserFontsDirectory();
    void UserNetworkNeighbourhoodDirectory();
    void UserDocumentsDirectory();
    void UserRecentDirectory();
    void UserSendToDirectory();
    void UserTemplatesDirectory();
    void UserRecycleBinDirectory();
    void UserApplicationDataDirectory();
    void UserStartMenuDirectory();
    void UserStartMenuStartupDirectory();
    void UserStartMenuProgramsDirectory();
    void CommonDesktopDirectory();
    void CommonStartMenuDirectory();
    void CommonStartMenuStartupDirectory();
    void CommonStartMenuProgramsDirectory();
    void WindowsProfile(); // Win.INI
    void SystemProfile();  // System.INI

	BOOL CreateTempName(LPCTSTR lpcszPrefix);
    BOOL CreateTempDir(LPCTSTR lpcszPrefix, UINT nRetries =100);
    BOOL CreateRandomName(BOOL bMustNotExist =TRUE, UINT nRetries =1000);
	BOOL CreateSimilarName(BOOL bMustNotExist =TRUE, UINT nRetries =1000);

	//Drive Information 
    UINT GetDriveType() const;
    BOOL IsRemovableDrive() const { return GetDriveType()==DRIVE_REMOVABLE; }
    BOOL IsCDRomDrive() const     { return GetDriveType()==DRIVE_CDROM; }
    BOOL IsNetworkDrive() const   { return GetDriveType()==DRIVE_REMOTE; }
    BOOL IsRAMDrive() const       { return GetDriveType()==DRIVE_RAMDISK; }

    DWORD DriveTotalSpaceBytes() const;
    DWORD DriveFreeSpaceBytes() const;
    DWORD GetDriveClusterSize() const;
    BOOL  GetDiskInfo(LPDWORD lpSectorsPerCluster,
                      LPDWORD lpBytesPerSector,
                      LPDWORD lpFreeClusters,
                      LPDWORD lpClusters) const;

	//Directory information
    BOOL IsDirectory() const;
    BOOL DirectoryExists() const;
    BOOL IsDirectoryEmpty() const;

	//File Information
    BOOL     IsFile() const { return !IsDirectory(); }
    BOOL     Exists() const;
    DWORD    GetSize() const; 
    DWORD    GetAttributes() const;
    BOOL     GetTime(FILETIME *lpCreated, FILETIME *lpAccessed, FILETIME *lpModified) const;
    FILETIME GetTimeCreated() const;
    FILETIME GetTimeLastModified() const;
    FILETIME GetTimeLastAccessed() const;

	//Directory operations
    BOOL CreateDirectory(BOOL bCreateIntermediates =TRUE);
    void CreateDirectoryEx(BOOL bCreateIntermediates =TRUE); // Throws CPathException
	BOOL RemoveDirectory(BOOL bEvenIfNotEmpty =FALSE);
    BOOL RemoveDirectoryContent();
    BOOL ChangeDirectory();
	    
	//File operations
	BOOL Delete(BOOL bEvenIfReadOnly =TRUE);
	BOOL Rename(LPCTSTR lpszNewPath);	
    BOOL CopyTo(LPCTSTR lpcszTargetFile, BOOL bOverwrite =TRUE);
    BOOL MoveTo(LPCTSTR lpcszTargetFile, BOOL bOverwrite =TRUE);
    BOOL SetAttributes(DWORD dwAttributes);
    BOOL SetTime(const FILETIME *lpCreated, const FILETIME *lpAccessed, const FILETIME *lpModified);
    BOOL SetTimeCreated(const FILETIME *lpCreated);
    BOOL SetTimeLastModified(const FILETIME *lpModified);
    BOOL SetTimeLastAccessed(const FILETIME *lpAccessed);

	//Finders
    BOOL FindFirst(DWORD dwAttributes =_A_NORMAL);
    BOOL FindNext();

	// Helpers
private:
    BOOL AttributesMatch(DWORD dwTargetAttributes, DWORD dwFileAttributes);
    BOOL ShellDirectory(int nShellFolderID);
    BOOL ShellDirectory2(int nShellFolderID);
    BOOL GetRegistryPath(HKEY hRootKey, LPCTSTR lpcszKeyName, LPCTSTR lpcszValueName, stdstr &strPath);


	void cleanPathString(stdstr& rDirectory) const;
	void StripLeadingChar(stdstr& rText, TCHAR chLeading) const;
	void StripLeadingBackslash(stdstr& Directory)  const;
	void StripTrailingChar(stdstr& rText, TCHAR chTrailing) const;
	void StripTrailingBackslash(stdstr& rDirectory) const;
	void EnsureTrailingBackslash(stdstr& Directory) const;
	void EnsureLeadingBackslash(stdstr& Directory) const;
};

#endif // !defined(AFX_PATH_H__6DD6923B_E241_40CE_81A3_4C2C88C140E4__INCLUDED_)
