// Path.cpp: implementation of the CPath class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Shlobj.h>

//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////

LPCTSTR const DLL_EXTENSION = _T("dll");
LPCTSTR const INI_EXTENSION = _T("ini");
LPCTSTR const EXE_EXTENSION = _T("exe");
LPCTSTR const WILD_NAME_EXTENSION = _T("*.*");
const TCHAR WILD_ONE             = '?';
const TCHAR WILD_ANY             = '*';
LPCTSTR const WILD_SET           = _T("?*");
LPCTSTR const DIR_DOUBLEDELIM    = _T("\\\\");
const TCHAR DRIVE_DELIMITER      = ':';
const TCHAR DIRECTORY_DELIMITER  = '\\';
const TCHAR EXTENSION_DELIMITER  = '.';
const TCHAR DIRECTORY_DELIMITER2 = '/';
HINSTANCE CPath::m_hInst = NULL;

//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

void CPath::SethInst ( HINSTANCE hInst )
{
	m_hInst = hInst;
}

HINSTANCE CPath::GethInst()
{
	return m_hInst;
}

//////////////////////////////////////////////////////////////////////
// Initialisation
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------
// Task    : Helper function for the various CPath constructors. 
//           Initializes the data members and establishes various
//           class invariants
//-------------------------------------------------------------
inline void CPath::Init()
{
	m_dwFindFileAttributes =0;
	m_hFindFile =NULL;
}

//-------------------------------------------------------------
// Task    : Helper function for the various CPath destructors.
//           Cleans up varios internals
//-------------------------------------------------------------
inline void CPath::Exit()
{
	if(m_hFindFile != NULL)
    {
		FindClose(m_hFindFile);
        m_hFindFile =NULL;
    }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------
// Task    : Constructs a path
//-------------------------------------------------------------
CPath::CPath()
{
	Init();
    Empty();
}

//-------------------------------------------------------------
// Task    : Constructs a path as copy of another
//-------------------------------------------------------------
CPath::CPath(const CPath& rPath)
{
	Init();
	m_strPath =rPath.m_strPath;
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 lpszPath
//-------------------------------------------------------------
CPath::CPath(LPCTSTR lpszPath)
{
	Init();
    m_strPath =lpszPath ? lpszPath : _T("");
	cleanPathString(m_strPath);
}

CPath::CPath(LPCTSTR lpszPath, LPCTSTR NameExten)
{
	Init();
	SetDriveDirectory(lpszPath);
	SetNameExtension(NameExten);
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const stdstr& strPath)
{
    Init();
    m_strPath =strPath;
	cleanPathString(m_strPath);
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const stdstr& strPath, LPCTSTR NameExten )
{
	Init();
	SetDriveDirectory(strPath.c_str());
	SetNameExtension(NameExten);
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const stdstr& strPath, const stdstr& NameExten )
{
	Init();
	SetDriveDirectory(strPath.c_str());
	SetNameExtension(NameExten.c_str());
}

//-------------------------------------------------------------
// Task    : Cleanup and destruct a path object
//-------------------------------------------------------------
CPath::~CPath()
{
	Exit();
}


//-------------------------------------------------------------
// Post    : Return TRUE if paths are equal
// Task    : Check if the two path are the same
//-------------------------------------------------------------
BOOL CPath::operator ==(const CPath& rPath) const
{
    // Get fully qualified versions of the paths
	stdstr FullyQualified1;
    stdstr FullyQualified2;
	
	GetFullyQualified(FullyQualified1);
	rPath.GetFullyQualified(FullyQualified2);
	
    // Compare them
	return _tcsicmp(FullyQualified1.c_str(),FullyQualified2.c_str()) == 0;
}

//-------------------------------------------------------------
// Post    : Return TRUE if paths are different
// Task    : Check if the two path are different
//-------------------------------------------------------------
BOOL CPath::operator !=(const CPath& rPath) const
{
    return !(*this == rPath);
}

//-------------------------------------------------------------
// Task    : Assign a path 2 another
//-------------------------------------------------------------
CPath& CPath::operator =(const CPath& rPath)
{                   
	if(this != &rPath)
		m_strPath =rPath.m_strPath;
    return *this;
}

//-------------------------------------------------------------
// Post    : Return the path, so that assignements can be chained
// Task    : Assign a string 2 a path
//-------------------------------------------------------------
CPath& CPath::operator =(LPCTSTR lpszPath)
{
    m_strPath =lpszPath ? lpszPath : _T("");
    return *this;
}

//-------------------------------------------------------------
// Post    : Return the path, so that assignements can be chained
// Task    : Assign a string 2 a path
//-------------------------------------------------------------
CPath& CPath::operator =(const stdstr& strPath)
{
    m_strPath =strPath;
    return *this;
}

//-------------------------------------------------------------
// Post    : Converts path 2 string
// Task    : Convert path 2 string
//           Warning: because this pointer 2 string point in the data
//           of this class, it is possible 2 cast the result of this 
//           function in any non-constant pointer and alter the data.
//           Very dangerous
//-------------------------------------------------------------
CPath::operator LPCTSTR() const
{
    return (LPCTSTR)m_strPath.c_str();
}

CPath::CPath(DIR_CURRENT_DIRECTORY /*sdt*/, LPCTSTR NameExten)
{
	// Application's current directory	 
	Init();
	CurrentDirectory();
	if (NameExten) { SetNameExtension(NameExten); }
}

CPath::CPath(DIR_MODULE_DIRECTORY /*sdt*/, LPCTSTR NameExten)
{
	// The directory where the executable of this app is
	Init();
	ModuleDirectory();
	if (NameExten) { SetNameExtension(NameExten); }
}

CPath::CPath(DIR_MODULE_FILE /*sdt*/)
{
	// The directory where the executable of this app is
	Init();
	Module();
}

//-------------------------------------------------------------
// Post    : Returns the drive component without a colon, e.g. "c"
//           Returns the directory component with a leading backslash, 
//              but no trailing backslash, e.g. "\dir\subdir"
//           Returns name compleletely without delimiters, e.g "letter"
//           Returns extension completely without delimiters, e.g. "doc"
// Globals :
// I/O     :
// Task    : Return the individual components of this path. 
//           For any given argument, you can pass NULL if you are not 
//           interested in that component.
//           Do not rely on pNames being <= 8 characters, extensions 
//           being <= 3 characters, or drives being 1 character
//-------------------------------------------------------------
void CPath::GetComponents(stdstr* pDrive, 
                       	  stdstr* pDirectory, 
                       	  stdstr* pName, 
                          stdstr* pExtension) const
{
    TCHAR buff_drive[_MAX_DRIVE + 1];
    TCHAR buff_dir  [_MAX_DIR   + 1];
    TCHAR buff_name [_MAX_FNAME + 1];
    TCHAR buff_ext  [_MAX_EXT   + 1];

    ZeroMemory(buff_drive,sizeof(buff_drive));
    ZeroMemory(buff_dir,  sizeof(buff_dir));
    ZeroMemory(buff_name, sizeof(buff_name));
    ZeroMemory(buff_ext,  sizeof(buff_ext));

	_tsplitpath(m_strPath.c_str(), 
		pDrive     ? buff_drive : NULL,
		pDirectory ? buff_dir   : NULL,
		pName      ? buff_name  : NULL,
		pExtension ? buff_ext   : NULL);
                
    if(pDrive)
        *pDrive =buff_drive;
    if(pDirectory)
        *pDirectory =buff_dir;
    if(pName)
        *pName =buff_name;
    if(pExtension)
        *pExtension =buff_ext;

	// DOS's _splitpath returns "d:", we return "d"
	if(pDrive)
		StripTrailingChar(*pDrive,DRIVE_DELIMITER);
	// DOS's _splitpath returns "\dir\subdir\", we return "\dir\subdir"
	if(pDirectory)
		StripTrailingBackslash(*pDirectory);
	// DOS's _splitpath returns ".ext", we return "ext"	
	if(pExtension)
		StripLeadingChar(*pExtension,EXTENSION_DELIMITER);
}

//-------------------------------------------------------------
// Task    : Get drive and directory from path
//-------------------------------------------------------------
void CPath::GetDriveDirectory(stdstr& rDriveDirectory) const
{
    stdstr Drive;
    stdstr Directory;

	GetComponents(&Drive,&Directory);
	rDriveDirectory =Drive;
	if(!Drive.empty())
    {
		rDriveDirectory += DRIVE_DELIMITER;
        rDriveDirectory += Directory;
    }
}

stdstr CPath::GetDriveDirectory(void) const
{
	stdstr rDriveDirectory;
	GetDriveDirectory(rDriveDirectory);
	return rDriveDirectory;
}
//-------------------------------------------------------------
// Task    : Get directory from path
//-------------------------------------------------------------
void CPath::GetDirectory(stdstr& rDirectory) const
{
    GetComponents(NULL,&rDirectory);
}    

stdstr CPath::GetDirectory(void) const
{
	stdstr rDirectory;
	GetComponents(NULL,&rDirectory);
	return rDirectory;
}    

//-------------------------------------------------------------
// Task    : Get filename and extension from path
//-------------------------------------------------------------
void CPath::GetNameExtension(stdstr& rNameExtension) const
{
    stdstr Name;
    stdstr Extension;

	GetComponents(NULL,NULL,&Name,&Extension);
    rNameExtension =Name;
    if(!Extension.empty())
    {
    	rNameExtension += EXTENSION_DELIMITER;
        rNameExtension += Extension;
    }
}

stdstr CPath::GetNameExtension(void) const
{
	stdstr rNameExtension;
	GetNameExtension(rNameExtension);
	return rNameExtension;
}

//-------------------------------------------------------------
// Task    : Get filename from path
//-------------------------------------------------------------
void CPath::GetName(stdstr& rName) const
{
    GetComponents(NULL,NULL,&rName);
}

stdstr CPath::GetName(void) const
{
	stdstr rName;
	GetComponents(NULL,NULL,&rName);
	return rName;
}

//-------------------------------------------------------------
// Task    : Get file extension from path
//-------------------------------------------------------------
void CPath::GetExtension(stdstr& rExtension) const
{
    GetComponents(NULL,NULL,NULL,&rExtension);
}   

stdstr CPath::GetExtension(void) const
{
	stdstr rExtension;
	GetComponents(NULL,NULL,NULL,&rExtension);
	return rExtension;
}   

//-------------------------------------------------------------
// Task    : Get current directory
//-------------------------------------------------------------
void CPath::GetCurrentDirectory(stdstr& rDirectory) const
{
	stdstr Directory;

	rDirectory = "";
	
	GetDirectory(Directory);	
	StripTrailingBackslash(Directory);
	if(Directory.empty())
		return;
	
    stdstr::size_type nDelimiter =Directory.rfind(DIRECTORY_DELIMITER);
	
	rDirectory =Directory.substr(nDelimiter);
	StripLeadingBackslash(rDirectory);
}   

stdstr CPath::GetCurrentDirectory(void) const
{
	stdstr rDirecotry;
	GetCurrentDirectory(rDirecotry);
	return rDirecotry;
}   

//-------------------------------------------------------------
// Task    : Get fully qualified path
//-------------------------------------------------------------
void CPath::GetFullyQualified(stdstr& rFullyQualified) const
{
    TCHAR buff_fullname[MAX_PATH];

	memset(buff_fullname, 0, sizeof(buff_fullname));

	_tfullpath(buff_fullname,m_strPath.c_str(),MAX_PATH-1);
    rFullyQualified =buff_fullname;
}

//-------------------------------------------------------------
// Post    : Return TRUE if path does not start from filesystem root
// Task    : Check if path is a relative one (e.g. doesn't start with C:\...)
//-------------------------------------------------------------    
BOOL CPath::IsRelative() const
{
	if (m_strPath.length() > 1 && m_strPath[1] == DRIVE_DELIMITER)
	{
		return false;
	}
	if (m_strPath.length() > 1 && m_strPath[0] == DIRECTORY_DELIMITER && m_strPath[1] == DIRECTORY_DELIMITER)
	{
		return false;
	}
	return true;
}

//-------------------------------------------------------------
// Task    : Set path components
//-------------------------------------------------------------
void CPath::SetComponents(LPCTSTR lpszDrive, 
                          LPCTSTR lpszDirectory,
						  LPCTSTR lpszName, 
                          LPCTSTR lpszExtension)
{
    TCHAR buff_fullname[MAX_PATH];

	memset(buff_fullname, 0, sizeof(buff_fullname));

	_tmakepath(buff_fullname,lpszDrive,lpszDirectory,lpszName,lpszExtension);

    m_strPath.erase();
    m_strPath =buff_fullname;
}

//-------------------------------------------------------------
// Task    : Set path's drive
//-------------------------------------------------------------
void CPath::SetDrive(TCHAR chDrive)
{
	stdstr_f Drive(_T("%c"),chDrive);
	stdstr	 Directory;
	stdstr	 Name;
	stdstr	 Extension;
	
	GetComponents(NULL,&Directory,&Name,&Extension);
	SetComponents(Drive.c_str(),Directory.c_str(),Name.c_str(),Extension.c_str());
}

//-------------------------------------------------------------
// Task    : Set path's directory
//-------------------------------------------------------------
void CPath::SetDirectory(LPCTSTR lpszDirectory, BOOL bEnsureAbsolute /*= FALSE*/)
{
	stdstr	Drive;	
	stdstr	Directory =lpszDirectory;
	stdstr	Name;
	stdstr	Extension;
	
	if(bEnsureAbsolute)
		EnsureLeadingBackslash(Directory);
	EnsureTrailingBackslash(Directory);

	GetComponents(&Drive,NULL,&Name,&Extension);
	SetComponents(Drive.c_str(),Directory.c_str(),Name.c_str(),Extension.c_str());
}    

//-------------------------------------------------------------
// Task    : Set path's drive and directory
//-------------------------------------------------------------
void CPath::SetDriveDirectory(LPCTSTR lpszDriveDirectory)
{
	stdstr	DriveDirectory =lpszDriveDirectory;
	stdstr	Name;
	stdstr	Extension;
	
	EnsureTrailingBackslash(DriveDirectory);
	cleanPathString(DriveDirectory);
	
	GetComponents(NULL,NULL,&Name,&Extension);
	SetComponents(NULL,DriveDirectory.c_str(),Name.c_str(),Extension.c_str());
}    

//-------------------------------------------------------------
// Task    : Set path's filename
//-------------------------------------------------------------
void CPath::SetName(LPCTSTR lpszName)
{
	stdstr	Drive;
	stdstr	Directory;
	stdstr	Extension;
	
	GetComponents(&Drive,&Directory,NULL,&Extension);
	SetComponents(Drive.c_str(),Directory.c_str(),lpszName,Extension.c_str());
}

//-------------------------------------------------------------
// Task    : Set path's filename
//-------------------------------------------------------------
void CPath::SetName(int iName)
{
	stdstr	Drive;
	stdstr	Directory;
	stdstr	Extension;
	TCHAR 	sName[33];
	
	memset(sName, 0, sizeof(sName));

	_itot(iName, sName, 10);
	
	GetComponents(&Drive,&Directory,NULL,&Extension);
	SetComponents(Drive.c_str(),Directory.c_str(),sName,Extension.c_str());
}


//-------------------------------------------------------------
// Task    : Set path's file extension
//-------------------------------------------------------------
void CPath::SetExtension(LPCTSTR lpszExtension)
{
	stdstr	Drive;
	stdstr	Directory;
	stdstr	Name;
	
	GetComponents(&Drive,&Directory,&Name);
	SetComponents(Drive.c_str(),Directory.c_str(),Name.c_str(),lpszExtension);
}

//-------------------------------------------------------------
// Task    : Set path's file extension
//-------------------------------------------------------------
void CPath::SetExtension(int iExtension)
{
	stdstr	Drive;
	stdstr	Directory;
	stdstr	Name;
	TCHAR	sExtension[20];

	memset(sExtension, 0, sizeof(sExtension));

	_itot(iExtension, sExtension, 10);

	GetComponents(&Drive,&Directory,&Name);
	SetComponents(Drive.c_str(),Directory.c_str(),Name.c_str(),sExtension);
}

//-------------------------------------------------------------
// Task    : Set path's filename and extension
//-------------------------------------------------------------
void CPath::SetNameExtension(LPCTSTR lpszNameExtension)
{
	stdstr	Drive;
	stdstr	Directory;

	GetComponents(&Drive,&Directory);
	SetComponents(Drive.c_str(),Directory.c_str(),lpszNameExtension,NULL);
}    

//-------------------------------------------------------------
// Task    : Append a subdirectory 2 path's directory
//-------------------------------------------------------------
void CPath::AppendDirectory(LPCTSTR lpszSubDirectory)
{                                               
	stdstr	Drive;
	stdstr	Directory;
	stdstr	SubDirectory =lpszSubDirectory;
	stdstr	Name;
	stdstr	Extension;
	
	if(SubDirectory.empty())
		return;

	// Strip out any preceeding backslash
	StripLeadingBackslash(SubDirectory);
	EnsureTrailingBackslash(SubDirectory);

	GetComponents(&Drive,&Directory,&Name,&Extension);
	EnsureTrailingBackslash(Directory);
    Directory +=SubDirectory;

	SetComponents(Drive.c_str(),Directory.c_str(),Name.c_str(),Extension.c_str());
}

//-------------------------------------------------------------
// Pre     : If pLastDirectory is given we will store the name of the
//           deepest directory (the one we're just exiting) in it
// Task    : Remove deepest subdirectory from path
//-------------------------------------------------------------
void CPath::UpDirectory(stdstr *pLastDirectory /*= NULL*/)
{
	stdstr Directory;

	GetDirectory(Directory);	
	StripTrailingBackslash(Directory);
	if(Directory.empty())
		return;
	
    stdstr::size_type nDelimiter =Directory.rfind(DIRECTORY_DELIMITER);
	
	if(pLastDirectory != NULL)
	{
		*pLastDirectory =Directory.substr(nDelimiter);
		StripLeadingBackslash(*pLastDirectory);
	}
		
    if(nDelimiter != stdstr::npos)
		Directory =Directory.substr(0,nDelimiter);
		
	SetDirectory(Directory.c_str());
}

//-------------------------------------------------------------
// Task    : Set path 2 current directory
//-------------------------------------------------------------
void CPath::CurrentDirectory()
{
	TCHAR buff_path[MAX_PATH];
	
	memset(buff_path, 0, sizeof(buff_path));

    ::GetCurrentDirectory(MAX_PATH,buff_path);
	
	Empty();
	SetDriveDirectory(buff_path);
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of specified module
//-------------------------------------------------------------
void CPath::Module(HINSTANCE hInstance)
{
    TCHAR buff_path[MAX_PATH];

	memset(buff_path, 0, sizeof(buff_path));

	GetModuleFileName(hInstance,buff_path,MAX_PATH);
    m_strPath =buff_path;
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of current module
//-------------------------------------------------------------
void CPath::Module()
{
    TCHAR buff_path[MAX_PATH];
	memset(buff_path, 0, sizeof(buff_path));

	GetModuleFileName(m_hInst,buff_path,MAX_PATH);
    m_strPath =buff_path;
}

//-------------------------------------------------------------
// Task    : Set path 2 the directory of specified module
//-------------------------------------------------------------
void CPath::ModuleDirectory(HINSTANCE hInstance)
{
	Module(hInstance);
	SetNameExtension(_T(""));
}

//-------------------------------------------------------------
// Task    : Set path 2 the directory of current module
//-------------------------------------------------------------
void CPath::ModuleDirectory()
{
	Module();
	SetNameExtension(_T(""));
}

//---------------------------------------------------------------------------
// Post    : Return TRUE if a directory
// Task    : Check if this path represents a directory
//---------------------------------------------------------------------------
BOOL CPath::IsDirectory() const
{
    // Check if this path has a filename
    stdstr file_name;
    GetNameExtension(file_name);
    
    return file_name.empty();
}

//-------------------------------------------------------------
// Post    : Return TRUE if directory exists
// Task    : To determine if the directory exists, we need to
//           create a test path with a wildcard (*.*) extension
//           and see if FindFirstFile returns anything.  We don't
//           use CPath::FindFirst() because that routine parses out
//           '.' and '..', which fails for empty directories
//-------------------------------------------------------------
BOOL CPath::DirectoryExists() const
{
    // Create test path	
	CPath TestPath(m_strPath.c_str());

	stdstr DirName;
	TestPath.UpDirectory(&DirName);
	TestPath.SetNameExtension(DirName.c_str());

	WIN32_FIND_DATA	FindData;
	HANDLE          hFindFile =FindFirstFile((LPCTSTR)TestPath,&FindData); // Find anything
	BOOL            bGotFile  =(hFindFile != INVALID_HANDLE_VALUE);

	if(hFindFile != NULL)	// Make sure we close the search
	    FindClose(hFindFile);

	return bGotFile;
}                                                     

//-------------------------------------------------------------
// Post    : Return TRUE if these is such a file
// Task    : Check if file exists
//-------------------------------------------------------------
BOOL CPath::Exists() const
{
	WIN32_FIND_DATA FindData;
	HANDLE          hFindFile =FindFirstFile(m_strPath.c_str(),&FindData);
	BOOL            bSuccess  =(hFindFile != INVALID_HANDLE_VALUE);

	if(hFindFile != NULL)	// Make sure we close the search
	    FindClose(hFindFile);

	return bSuccess;
}

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Delete file
//-------------------------------------------------------------
BOOL CPath::Delete(BOOL bEvenIfReadOnly) const
{
    ULONG dwAttr =::GetFileAttributes(m_strPath.c_str());
    if(dwAttr == (ULONG)-1)
        // File does not exists
        return FALSE;

    if(((dwAttr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY) && !bEvenIfReadOnly)
        // File is read-only, and we're not allowed 2 delete it
        return FALSE;

    SetFileAttributes(m_strPath.c_str(),FILE_ATTRIBUTE_NORMAL);
    return DeleteFile(m_strPath.c_str());
}	

//-------------------------------------------------------------
// Post    : Return TRUE on success, FALSE if there is such a target file
//           and we weren't granted permission 2 overwrite file or some error
// Task    : Copy file
//           Since ::CopyFile will not overwrite read only files
//           we will make sure the target file is writable first
//-------------------------------------------------------------
BOOL CPath::CopyTo(LPCTSTR lpcszTargetFile, BOOL bOverwrite)
{
    // Check if the target file exists
    CPath TargetFile(lpcszTargetFile);
    if(TargetFile.Exists())
    {
        // Yeah there is already such a target file 
        // Decide if we should overwrite
        if(!bOverwrite)
            return FALSE;

        // Delete any previous target
        if(!TargetFile.Delete(TRUE))
            return FALSE;
    }

    // CopyFile will set the target's attributes 2 the same as 
    // the source after copying
    return CopyFile(m_strPath.c_str(),lpcszTargetFile,!bOverwrite);
}

//-------------------------------------------------------------
// Post    : Return TRUE on success, FALSE if there is such a target file
//           and we weren't granted permission 2 overwrite file or some error
// Task    : Move file
//-------------------------------------------------------------
BOOL CPath::MoveTo(LPCTSTR lpcszTargetFile, BOOL bOverwrite)
{
    // Check if the target file exists
    CPath TargetFile(lpcszTargetFile);
    if(TargetFile.Exists())
    {
        // Yeah there is already such a target file 
        // Decide if we should overwrite
        if(!bOverwrite)
            return FALSE;

        // Delete any previous target
        if(!TargetFile.Delete(TRUE))
            return FALSE;
    }

   return MoveFile(m_strPath.c_str(),lpcszTargetFile);
}

//-------------------------------------------------------------
// Post    : Return TRUE if attributes do match
// Task    : Compare finder attributes
//-------------------------------------------------------------
BOOL CPath::AttributesMatch(ULONG dwTargetAttributes, ULONG dwFileAttributes)
{
	if (dwTargetAttributes == _A_ALLFILES)
	{
		return true;
	}
	if(dwTargetAttributes == _A_NORMAL)
	{
		return ((_A_SUBDIR & dwFileAttributes) == 0);
	}
	return ( ((dwTargetAttributes & dwFileAttributes) != 0) && ((_A_SUBDIR & dwTargetAttributes) == (_A_SUBDIR & dwFileAttributes)) );
}

//-------------------------------------------------------------
// Post    : Return TRUE if any match found
// Task    : Find the first file that meets this path and the specified attributes
//           You can specify the current attributes of the file or directory 
//           The attributes are represented by a combination (|) of the following 
//           constants:
//
//           _A_ARCH    Archive. Set whenever the file is 
//                      changed, and cleared by the BACKUP command
//           _A_HIDDEN  Hidden file. Not normally seen with 
//                      the DIR command, unless the /AH option 
//                      is used. Returns information about normal 
//                      files as well as files with this attribute
//           _A_NORMAL  Normal. File can be read or written to 
//                      without restriction
//           _A_RDONLY  Read-only. File cannot be opened for writing, 
//                      and a file with the same name cannot be created
//           _A_SUBDIR  Subdirectory
//           _A_SYSTEM  System file. Not normally seen with the DIR 
//                      command, unless the /AS option is used
//
//           These attributes do not follow a simple additive logic
//           Note that _A_NORMAL is 0x00, so it effectively cannot be
//           removed from the attribute set. You will therefore always
//           get normal files, and may also get Archive, Hidden, etc.
//           if you specify those attributes
//           See aso: FindFirstFile, FindNextFile
//-------------------------------------------------------------
BOOL CPath::FindFirst(ULONG dwAttributes /*= _A_NORMAL*/)
{
	m_dwFindFileAttributes =dwAttributes;
	BOOL bGotFile;
	BOOL bWantSubdirectory =(BOOL)(_A_SUBDIR & dwAttributes);

    // Close handle to any previous enumeration
    Exit();

	// i.) Finding first candidate file
	WIN32_FIND_DATA	FindData;
	m_hFindFile =FindFirstFile(m_strPath.c_str(),&FindData);
	bGotFile =(m_hFindFile != INVALID_HANDLE_VALUE);

	while(bGotFile)
	{
		// ii.) Compare candidate to attributes, and filter out the "." and ".." folders
		if(!AttributesMatch(m_dwFindFileAttributes,FindData.dwFileAttributes))
			goto LABEL_GetAnother;
		if(bWantSubdirectory && (FindData.cFileName[0] == '.'))
			goto LABEL_GetAnother;

		// iii.) Found match, prepare result
        if((_A_SUBDIR & FindData.dwFileAttributes) != 0)
            StripTrailingBackslash(m_strPath);

		SetNameExtension(FindData.cFileName);

        if((_A_SUBDIR & FindData.dwFileAttributes) != 0)
            EnsureTrailingBackslash(m_strPath);
		return TRUE;
	
		// iv.) Not found match, get another
	    LABEL_GetAnother:
		bGotFile =FindNextFile(m_hFindFile,&FindData);
	}
	
	return FALSE;
}

//-------------------------------------------------------------
// Post    : Return TRUE if a new match found
// Task    : Find the next file that meets the conditions specified 
//           in the last FindFirst call
//-------------------------------------------------------------
BOOL CPath::FindNext()
{
    if (m_hFindFile == NULL)
		return FALSE;

	WIN32_FIND_DATA	FindData;	
	while(FindNextFile(m_hFindFile,&FindData) != FALSE)
    { // while(FindNext(...))

		if(AttributesMatch(m_dwFindFileAttributes,FindData.dwFileAttributes))
		{ // if(AttributesMatch(...)
	        if((_A_SUBDIR & FindData.dwFileAttributes) == _A_SUBDIR)
			{
				if (IsDirectory())
				{
	                // Found a directory
					UpDirectory();
				}
				else
				{
					SetNameExtension("");
				}
				AppendDirectory(FindData.cFileName);
			}
			else
			{
                // Found a file
				if (IsDirectory())
				{
	                // Found a directory
					UpDirectory();
				}
				SetNameExtension(FindData.cFileName);
			}
	        if((_A_SUBDIR & FindData.dwFileAttributes) == _A_SUBDIR)
    	    	EnsureTrailingBackslash(m_strPath);
			return TRUE;
		}
	}

	return FALSE;
}

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Change current working directory of application 2 path
//-------------------------------------------------------------
BOOL CPath::ChangeDirectory()
{
	stdstr DriveDirectory;
	GetDriveDirectory(DriveDirectory);

    return SetCurrentDirectory(DriveDirectory.c_str());
}

//-------------------------------------------------------------
// Pre     : If bCreateIntermediates is TRUE, create all eventually
//           missing parent directories too
// Post    : Return TRUE on success
// Task    : Create new directory
//-------------------------------------------------------------
BOOL CPath::CreateDirectory(BOOL bCreateIntermediates /*= TRUE*/)
{
	stdstr	PathText;
	BOOL	bSuccess;
		
	GetDriveDirectory(PathText);
    StripTrailingBackslash(PathText);
    bSuccess =::CreateDirectory(PathText.c_str(),NULL);
	if(!bSuccess)
	{
		CPath CurrentDir(CPath::CURRENT_DIRECTORY);
		bSuccess = ChangeDirectory();
		CurrentDir.ChangeDirectory();
	}

	if(!bSuccess && bCreateIntermediates)
	{
        stdstr::size_type nDelimiter =PathText.rfind(DIRECTORY_DELIMITER);
        if(nDelimiter == stdstr::npos)
			return FALSE;

		PathText.resize(nDelimiter + 1);
		CPath SubPath(PathText);
		
		if(SubPath.CreateDirectory())
			return CreateDirectory(FALSE);
		else 
			return FALSE;
	}

	return bSuccess;
}			

//Helpers

//------------------------------------------------------------------------
// Task    : Remove first character (if any) if it's chLeading
//------------------------------------------------------------------------
void CPath::cleanPathString(stdstr& rDirectory) const
{
	LPCSTR const DIR_DOUBLEDELIM    = "\\\\";

	std::string::size_type pos = rDirectory.find( DIRECTORY_DELIMITER2 );
	while ( pos != std::string::npos )
	{
		rDirectory.replace( pos, 1, &DIRECTORY_DELIMITER );
		pos = rDirectory.find( DIRECTORY_DELIMITER2, pos + 1 );
	}

	bool AppendEnd = !_strnicmp(rDirectory.c_str(), "\\\\", 2);
	pos = rDirectory.find( DIR_DOUBLEDELIM );
	while ( pos != std::string::npos )
	{
		rDirectory.replace( pos, 1, &DIRECTORY_DELIMITER );
		pos = rDirectory.find( DIR_DOUBLEDELIM, pos + 1 );
	}
	if (AppendEnd)
	{
		rDirectory.insert(0, "\\");
	}
}

void CPath::StripLeadingChar(stdstr& rText, TCHAR chLeading) const
{
    stdstr::size_type nLength =rText.length();
	if(nLength == 0)
		return;

	if(rText[0] == chLeading)
		rText =rText.substr(1);
}


//------------------------------------------------------------------------
// Task    : Remove first character if \
//------------------------------------------------------------------------
void CPath::StripLeadingBackslash(stdstr& Directory) const
{
	stdstr::size_type nLength =Directory.length();

    // If Directory is of the form '\', don't do it
	if(nLength <= 1)
		return;

	if(Directory[0] == DIRECTORY_DELIMITER)
		Directory =Directory.substr(1);
}

//------------------------------------------------------------------------
// Task    : Remove last character (if any) if it's chTrailing
//------------------------------------------------------------------------
void CPath::StripTrailingChar(stdstr& rText, TCHAR chTrailing) const
{
	stdstr::size_type nLength =rText.length();
	if(nLength == 0)
		return;
	
	if(rText[nLength-1] == chTrailing)
		rText.resize(nLength-1);
}

//------------------------------------------------------------------------
// Task    : Remove last character if \
//------------------------------------------------------------------------
void CPath::StripTrailingBackslash(stdstr& rDirectory) const
{
	for (;;)
	{
		stdstr::size_type nLength = rDirectory.length();
		if(nLength <= 1)
		{
			return;
		}

		if(rDirectory[nLength-1] == DIRECTORY_DELIMITER || rDirectory[nLength-1] == DIRECTORY_DELIMITER2)
		{
			rDirectory.resize(nLength-1);
			continue;
		}
		return;
	}
}

//------------------------------------------------------------------------
// Task    : Add a backslash to the end of Directory if there is
//           not already one there
//------------------------------------------------------------------------
void CPath::EnsureTrailingBackslash(stdstr& Directory) const
{
	stdstr::size_type nLength =Directory.length();

	if(Directory.empty() || (Directory[nLength-1] != DIRECTORY_DELIMITER))
		Directory +=DIRECTORY_DELIMITER;
}

//------------------------------------------------------------------------
// Task    : Add a backslash to the beginning of Directory if there
//           is not already one there
//------------------------------------------------------------------------
void CPath::EnsureLeadingBackslash(stdstr& Directory) const
{
	if(Directory.empty() || (Directory[0] != DIRECTORY_DELIMITER))
    {
        stdstr temp =Directory;
		Directory.Format(_T("%c%s"),DIRECTORY_DELIMITER,temp.c_str());
    }
}
