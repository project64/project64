// Path.cpp: implementation of the CPath class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <atlbase.h>
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

//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------
// Task    : Create a string of length nDigits containing random digits
//-------------------------------------------------------------
static stdstr RandomDigits(int nDigits)
{
    // Keep the number of digits in a rational limit
    //ASSERT(nDigits >  0);
    //ASSERT(nDigits < 20);
    
	int    nDigits2 = nDigits;
	stdstr Digits;
    TCHAR  next_8_digits[9];
    while(nDigits2 > 0)
    {
        _stprintf(next_8_digits,_T("%08lx"),GetTickCount());

        for(int i=0; i<8; i++)
        {
            Digits += next_8_digits[i];
            if(--nDigits2 == 0)
                break;
        }
    }

    int last_digit =rand();
    if(last_digit < 0)
        last_digit =(-last_digit);
    last_digit %= 10;
    Digits[nDigits - 1] ='0' + last_digit;
    
	return Digits;
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
// Task    : Constructs a path and points it 2 specified 
//           special directory
//-------------------------------------------------------------
CPath::CPath(SpecialDirectoryType eInitialDir)
{
	Init();
	SpecialDirectory(eInitialDir);
}

CPath::CPath(SpecialDirectoryType eInitialDir, const stdstr & NameExten )
{
	Init();
	SpecialDirectory(eInitialDir);
	SetNameExtension(NameExten.c_str());
}

CPath::CPath(SpecialDirectoryType eInitialDir, LPCTSTR NameExten )
{
	Init();
	SpecialDirectory(eInitialDir);
	SetNameExtension(NameExten);
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

void CPath::SpecialDirectory(SpecialDirectoryType eInitialDir)
{
	switch(eInitialDir)
	{	
		// Application's current directory	 
	case CURRENT_DIRECTORY:
		CurrentDirectory();
		break;
		// Windows directory
	case WINDOWS_DIRECTORY:
		WindowsDirectory();
		break;
		// Windows' system directory    
	case SYSTEM_DIRECTORY:
		SystemDirectory();
		break;
	case SYSTEM_DRIVER_DIRECTORY:
		SystemDirectory();
		AppendDirectory(_T("drivers"));
		break;
		// The root directory of system drive
	case SYSTEM_DRIVE_ROOT_DIRECTORY:
		SystemDriveRootDirectory();
		break;
		// The directory where the executable of this app is
	case MODULE_DIRECTORY:
		ModuleDirectory();
		break;
	case MODULE_FILE:
		Module();
		break;
		// Windows temp directory
	case TEMP_DIRECTORY:
		TempDirectory();
		break;
		// Program files directory
	case PROGRAM_FILES_DIRECTORY:
		ProgramFilesDirectory();
		break;
		// Common files directory
	case COMMON_FILES_DIRECTORY:
		CommonFilesDirectory();
		break;
		// Accessories directory
	case ACCESSORIES_DIRECTORY:
		AccessoriesDirectory();
		break;
		// Media directory
	case MEDIA_DIRECTORY:
		MediaDirectory();
		break;
		// INF directory
	case DEVICE_DIRECTORY:
		DeviceDirectory();
		break;
		// User specific directories
	case USER_DESKTOP_DIRECTORY:
		UserDesktopDirectory();
		break;
	case USER_FAVORITES_DIRECTORY:
		UserFavoritesDirectory();
		break;
	case USER_FONTS_DIRECTORY:
		UserFontsDirectory();
		break;
	case USER_NETHOOD_DIRECTORY:
		UserNetworkNeighbourhoodDirectory();
		break;
	case USER_DOCUMENTS_DIRECTORY:
		UserDocumentsDirectory();
		break;
	case USER_RECENT_DIRECTORY:
		UserRecentDirectory();
		break;
	case USER_SENDTO_DIRECTORY:
		UserSendToDirectory();
		break;
	case USER_RECYCLE_DIRECTORY:
		UserRecycleBinDirectory();
		break;
	case USER_APPLICATION_DATA_DIRECTORY:
		UserApplicationDataDirectory();
		break;
	case USER_TEMPLATES_DIRECTORY:
		UserTemplatesDirectory();
		break;
	case USER_STARTMENU_DIRECTORY:
		UserStartMenuDirectory();
		break;
	case USER_STARTMENU_STARTUP_DIRECTORY:
		UserStartMenuStartupDirectory();
		break;
	case USER_STARTMENU_PROGRAMS_DIRECTORY:
		UserStartMenuProgramsDirectory();
		break;
		// Directories common 2 all users
	case COMMON_DESKTOP_DIRECTORY:
		CommonDesktopDirectory();
		break;
	case COMMON_STARTMENU_DIRECTORY:
		CommonStartMenuDirectory();
		break;
	case COMMON_STARTMENU_STARTUP_DIRECTORY:
		CommonStartMenuStartupDirectory();
		break;
	case COMMON_STARTMENU_PROGRAMS_DIRECTORY:
		CommonStartMenuProgramsDirectory();
		break;
		// Unknown special directory constant    
	default:
		// Accept only constants we know about
		//Bugger
		ATLASSERT(false);
		break;
	}
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
// Task    : Get drive from path
//-------------------------------------------------------------                       
void CPath::GetDrive(stdstr& rDrive) const
{
	GetComponents(&rDrive);
}

stdstr CPath::GetDrive(void) const
{
	stdstr rDrive;
	GetComponents(&rDrive);
	return rDrive;
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

	_tfullpath(buff_fullname,m_strPath.c_str(),MAX_PATH-1);
    rFullyQualified =buff_fullname;
}

//-------------------------------------------------------------
// Task    : Get fully qualified path in short (8.3 style) form
//-------------------------------------------------------------
void CPath::GetFullyQualifiedShort(stdstr& rFullyQualifiedShort) const
{
    GetFullyQualified(rFullyQualifiedShort);

	//#pragma message(Reminder(_T("Also implement a GetFullyQualifiedLong")))

    TCHAR buff_fullname[MAX_PATH];
    GetShortPathName(rFullyQualifiedShort.c_str(),buff_fullname,sizeof(buff_fullname)/sizeof(TCHAR));
    rFullyQualifiedShort =buff_fullname;
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
// Post    : Return TRUE if there are wildcards in the path
// Task    : Check if path contains wildcards
//-------------------------------------------------------------    
BOOL CPath::IsWild() const
{
    return (m_strPath.find_first_of(WILD_SET) != stdstr::npos);
}

//-------------------------------------------------------------
// Post    : Return TRUE if path is lexically correct
// Task    : Determine whether lpszFileName is valid. A filename
//           is valid if it contains only legal characters, doesn't
//           have repeated contiguous subdirectory delimiters, has at 
//           most one drive delimiter, and all components fit within 
//           maximum sizes
//           This routine does NOT determine if a file exists, or
//           even if it could exist relative to the user's directory
//           hierarchy. Its tests are for lexical correctness only
//           See also: CPath::Exists
//-------------------------------------------------------------
BOOL CPath::IsValid () const
{
    // Check 4 illegal characters (no wildcards allowed)
    // We accept either \ or / as folder delimiter
    if(IsWild() ||
       (m_strPath.find_first_of(_T("|\"<>")) != stdstr::npos) || 
       (m_strPath.find(_T("//")) != stdstr::npos))
        return FALSE;

    int index = (int)m_strPath.find(_T("\\\\"));
    if((index != stdstr::npos) && (index > 0))
        return FALSE;

    // Make sure : can appear only in the 2nd position as a drive delimiter
    if(((index = (int)m_strPath.find(':')) != stdstr::npos) && (index != 1))
        return FALSE;

    // Make sure it fits in the maximum path size
    if(m_strPath.length() > MAX_PATH)
        return FALSE;
    
    // Path is valid
	return TRUE;
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
	
    ::GetCurrentDirectory(MAX_PATH,buff_path);
	
	Empty();
	SetDriveDirectory(buff_path);
}

//-------------------------------------------------------------
// Task    : Set path 2 Windows directory
//-------------------------------------------------------------
void CPath::WindowsDirectory()
{
	TCHAR buff_path[MAX_PATH];
	
    GetWindowsDirectory(buff_path,MAX_PATH);
    
    Empty();
    SetDriveDirectory(buff_path);
}

//-------------------------------------------------------------
// Task    : Set path 2 Windows system directory
//-------------------------------------------------------------
void CPath::SystemDirectory()
{
	TCHAR buff_path[MAX_PATH];
	
    GetSystemDirectory(buff_path,MAX_PATH);
    
    Empty();
    SetDriveDirectory(buff_path);
}

//-------------------------------------------------------------
// Task    : Set path 2 root of system drive (usually C:\)
//-------------------------------------------------------------
void CPath::SystemDriveRootDirectory()
{
    SystemDirectory();
    SetDirectory(_T(""));
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of specified module
//-------------------------------------------------------------
void CPath::Module(HINSTANCE hInstance)
{
    TCHAR buff_path[MAX_PATH];
    GetModuleFileName(hInstance,buff_path,MAX_PATH);
    m_strPath =buff_path;
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of current module
//-------------------------------------------------------------
void CPath::Module()
{
    TCHAR buff_path[MAX_PATH];
    GetModuleFileName(NULL,buff_path,MAX_PATH);
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

//-------------------------------------------------------------
// Task    : Currently, if the current environment has an
//           entry for the TEMP environment variable, the directory will
//           be set to that. If not, the directory will be the Windows
//           System directory. The caller of this method, however, should
//           not rely on this convention
//-------------------------------------------------------------
void CPath::TempDirectory()
{
    TCHAR buff_path[MAX_PATH];

	GetTempPath(MAX_PATH,buff_path);

	m_strPath =buff_path;
	SetNameExtension(_T(""));	
}						 

//-------------------------------------------------------------
// Task    : Set path 2 program files folder
//           Usually C:\Program Files
//-------------------------------------------------------------
void CPath::ProgramFilesDirectory()
{
    stdstr strPath;
    if(GetRegistryPath(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),_T("ProgramFilesDir"),strPath))
    {
        // Got a path, use it
        Empty();
        SetDriveDirectory(strPath.c_str());
    }
    else
    {
        // This is some old or unknown system
        Empty();
        SetDriveDirectory(_T("C:\\Programs"));
    }
}

//-------------------------------------------------------------
// Task    : Set path 2 common files folder
//           Usually C:\Program Files\Common Files
//-------------------------------------------------------------
void CPath::CommonFilesDirectory()
{
    stdstr strPath;
    if(GetRegistryPath(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),_T("CommonFilesDir"),strPath))
    {
        // Got a path, use it
        Empty();
        SetDriveDirectory(strPath.c_str());
    }
    else
    {
        // This is some old or unknown system
        Empty();
        SetDriveDirectory(_T("C:\\Programs\\Common"));
    }
}

//-------------------------------------------------------------
// Task    : Set path 2 common files folder
//           On Win95 is C:\Program Files\Accessories
//           On WinNT is C:\Program Files\Windows NT\Accessories
//-------------------------------------------------------------
void CPath::AccessoriesDirectory()
{
    // Accessories folder is in Program Files folder
    ProgramFilesDirectory();

    COSVersion osver;
    WORD       ostype   =osver.GetOSType();
    WORD       wintype  =osver.GetWindowsType();
    BOOL       is_Win95 =(ostype==OS_WIN95) || (ostype==OS_WIN98);
    BOOL       is_NT    =((ostype & OS_WINNT) != 0);

    if((wintype != WIN_32S) && is_Win95)
    {
        // Windows 95
        AppendDirectory(_T("Accessories"));
        return;
    }

    if((wintype != WIN_32S) && is_NT && (((osver.GetMajorVersion()==3) && (osver.GetMinorVersion()>=51)) || (osver.GetMajorVersion()>3)))
    {
        // Windows NT with the new Chichago shell
        AppendDirectory(_T("Windows NT\\Accessories"));
        return;
    }

    // This is some old or unknown system
    AppendDirectory(_T("Accessry"));
}

//-------------------------------------------------------------
// Task    : Set path 2 media folder
//           Usually C:\Windows\Media
//-------------------------------------------------------------
void CPath::MediaDirectory()
{
    stdstr strPath;
    if(GetRegistryPath(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),_T("MediaPath"),strPath))
    {
        // Got a path, use it
        Empty();
        SetDriveDirectory(strPath.c_str());
    }
    else
    {
        // This is some old or unknown system
        WindowsDirectory();
        AppendDirectory(_T("Media"));
    }
}

//-------------------------------------------------------------
// Task    : Set path 2 device definition folder
//           Usually C:\Windows\Inf
//-------------------------------------------------------------
void CPath::DeviceDirectory()
{
    stdstr strPath;
    if(GetRegistryPath(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),_T("DevicePath"),strPath))
    {
        // Got a path, use it
        Empty();
        SetDriveDirectory(strPath.c_str());
    }
    else
    {
        // This is some old or unknown system
        WindowsDirectory();
        AppendDirectory(_T("Inf"));
    }
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will return FALSE
// Task    : Set path 2 one of the special folders in Chichago
//-------------------------------------------------------------
BOOL CPath::ShellDirectory(int nShellFolderID)
{
    COSVersion osver;
    WORD       ostype   =osver.GetOSType();
    WORD       wintype  =osver.GetWindowsType();
    BOOL       is_Win95 =(ostype==OS_WIN95) || (ostype==OS_WIN98);
    BOOL       is_NT    =((ostype & OS_WINNT) != 0);

    if((wintype != WIN_32S) && 
       (is_Win95 || (is_NT && (((osver.GetMajorVersion()==3) && (osver.GetMinorVersion()>=51)) || (osver.GetMajorVersion()>3)))))
    {
        // These systems support the new Chichago shell, get location from registry
        BOOL         result =FALSE;
        LPITEMIDLIST pidl   =NULL;
        TCHAR        special_path[MAX_PATH];

        // Get a PIDL 2 the special shell folder
        HRESULT hr =SHGetSpecialFolderLocation(NULL,nShellFolderID,&pidl);
        if(SUCCEEDED(hr))
        {
            // Convert the PIDL in2 a path
            result =SHGetPathFromIDList(pidl,special_path);

            // Free the PIDL
            // Get the address of our task allocator's IMalloc interface
            LPMALLOC pMalloc;
            hr =SHGetMalloc(&pMalloc);

            if(SUCCEEDED(hr))
            {
                // Free the PIDL
                pMalloc->Free(pidl);
            
                // Free our task allocator
                pMalloc->Release();
            }
        }

        if(result)
        {
            // We've got the special path, now set ourselves 2 point 2 this
            Empty();
            SetDriveDirectory(special_path);
        }

        return result;
    }

    // This is some old or unknown system, shell folders not supported
    return FALSE;
}

//---------------------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Set path 2 one of the special folders in Chichago
//           This function manually digs in the registry instead of using
//           SHGetSpecialFolderLocation, since it seems that this does not work 4
//           special location constants beginning with 
//---------------------------------------------------------------------------
BOOL CPath::ShellDirectory2(int nShellFolderID)
{
    COSVersion osver;
    WORD       ostype   =osver.GetOSType();
    WORD       wintype  =osver.GetWindowsType();
    BOOL       is_Win95 =(ostype==OS_WIN95) || (ostype==OS_WIN98);
    BOOL       is_NT    =((ostype & OS_WINNT) != 0);

    if((wintype != WIN_32S) && 
       (is_Win95 || (is_NT && (((osver.GetMajorVersion()==3) && (osver.GetMinorVersion()>=51)) || (osver.GetMajorVersion()>3)))))
    {
        // These systems support the new Chichago shell, get location from registry
        HKEY      root;
        stdstr   key;
        stdstr   value;

        switch(nShellFolderID)
        {
            case CSIDL_DESKTOPDIRECTORY:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Desktop");
                break;

            case CSIDL_FAVORITES:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Favorites");
                break;

            case CSIDL_FONTS:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Fonts");
                break;

            case CSIDL_NETHOOD:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("NetHood");
                break;

            case CSIDL_PERSONAL:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Personal");
                break;

            case CSIDL_RECENT:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Recent");
                break;

            case CSIDL_SENDTO:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("SendTo");
                break;

            case CSIDL_TEMPLATES:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Templates");
                break;

            case CSIDL_APPDATA:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("AppData");
                break;

            case CSIDL_STARTMENU:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Start Menu");
                break;

            case CSIDL_STARTUP:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Startup");
                break;

            case CSIDL_PROGRAMS:
                root  =HKEY_CURRENT_USER;
                key   =_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Programs");
                break;

            case CSIDL_COMMON_DESKTOPDIRECTORY:
                root  =HKEY_LOCAL_MACHINE;
                key   =_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Common Desktop");
                break;

            case CSIDL_COMMON_STARTMENU:
                root  =HKEY_LOCAL_MACHINE;
                key   =_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Common Start Menu");
                break;

            case CSIDL_COMMON_STARTUP:
                root  =HKEY_LOCAL_MACHINE;
                key   =_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Common Startup");
                break;

            case CSIDL_COMMON_PROGRAMS:
                root  =HKEY_LOCAL_MACHINE;
                key   =_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
                value =_T("Common Programs");
                break;
        }
        
        stdstr strPath;
        if(GetRegistryPath(root,key.c_str(),value.c_str(),strPath))
        {
            // Got a path, use it
            Empty();
            SetDriveDirectory(strPath.c_str());

            return TRUE;
        }
    }

    // This is some old or unknown system
    return FALSE;
}

//---------------------------------------------------------------------------
// Post    : Return FALSE if specified value does not exist or some error
// Task    : Dig in the registry 2 the specified location, and extract a path
//           Make sure the path is a valid 
//---------------------------------------------------------------------------
BOOL CPath::GetRegistryPath(HKEY hRootKey, LPCTSTR lpcszKeyName, LPCTSTR lpcszValueName, stdstr &strPath)
{
    TCHAR     path_buffer    [MAX_PATH];
    TCHAR     expanded_buffer[MAX_PATH];
    DWORD     path_buffer_size =sizeof(path_buffer);
    CRegistry reg(hRootKey,lpcszKeyName,KEY_READ);
    
    if(reg.GetValue(lpcszValueName,(BYTE *)&path_buffer,path_buffer_size))
    {
        COSVersion osver;
        WORD       ostype =osver.GetOSType();
        BOOL       is_NT  =((ostype & OS_WINNT) != 0);

#ifndef _UNICODE
        if(is_NT)
        {
            // Running on NT and the ExpandEnvironmentStrings API requires
            // Unicode strings
            WCHAR path_buffer_unicode    [MAX_PATH];
            WCHAR expanded_buffer_unicode[MAX_PATH];

            MultiByteToWideChar(CP_ACP,0,path_buffer,-1,path_buffer_unicode,sizeof(path_buffer_unicode)/sizeof(WCHAR));
            ExpandEnvironmentStringsW(path_buffer_unicode,expanded_buffer_unicode,sizeof(expanded_buffer_unicode)/sizeof(WCHAR));
            WideCharToMultiByte(CP_ACP,0,expanded_buffer_unicode,-1,expanded_buffer,sizeof(path_buffer)/sizeof(TCHAR),NULL,NULL);
        }
        else
#endif
            ExpandEnvironmentStrings(path_buffer,expanded_buffer,path_buffer_size);

        strPath.erase();
        strPath =expanded_buffer;

        return TRUE;
    }

    // No such key and/or value
    return FALSE;
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 desktop folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserDesktopDirectory()
{
    if(!ShellDirectory(CSIDL_DESKTOPDIRECTORY))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 favorites folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserFavoritesDirectory()
{
    if(!ShellDirectory(CSIDL_FAVORITES))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 fonts folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserFontsDirectory()
{
    if(!ShellDirectory(CSIDL_FONTS))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 network hood folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserNetworkNeighbourhoodDirectory()
{
    if(!ShellDirectory(CSIDL_NETHOOD))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 personal folder of currently logged-in user
//           Usually C:\My Documents
//-------------------------------------------------------------
void CPath::UserDocumentsDirectory()
{
    if(!ShellDirectory(CSIDL_PERSONAL))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 recent folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserRecentDirectory()
{
    if(!ShellDirectory(CSIDL_RECENT))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 SendTo folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserSendToDirectory()
{
    if(!ShellDirectory(CSIDL_SENDTO))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 templates folder of currently logged-in user
//           Usually C:\Windows\ShellNew
//-------------------------------------------------------------
void CPath::UserTemplatesDirectory()
{
    if(!ShellDirectory(CSIDL_TEMPLATES))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 the recycle bin directory
//           Usually C:\Recycled
//-------------------------------------------------------------
void CPath::UserRecycleBinDirectory()
{
    if(!ShellDirectory(CSIDL_BITBUCKET))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 the folder where application data is stored 
//           specific 2 currently logged-in user
//-------------------------------------------------------------
void CPath::UserApplicationDataDirectory()
{
    if(!ShellDirectory(CSIDL_APPDATA))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 start menu folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserStartMenuDirectory()
{
    if(!ShellDirectory(CSIDL_STARTMENU))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 startup folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserStartMenuStartupDirectory()
{
    if(!ShellDirectory(CSIDL_STARTUP))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 programs menu folder of currently logged-in user
//-------------------------------------------------------------
void CPath::UserStartMenuProgramsDirectory()
{
    if(!ShellDirectory(CSIDL_PROGRAMS))
        Empty();
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 desktop folder common 2 all users
//-------------------------------------------------------------
void CPath::CommonDesktopDirectory()
{
    if(!ShellDirectory2(CSIDL_COMMON_DESKTOPDIRECTORY))
    {
        // Check if running on Windows 95, and workaround this if so
        COSVersion osver;
        WORD       ostype =osver.GetOSType();
        if((ostype == OS_WIN95) || (ostype == OS_WIN98))
        {
            // Manual workaround
            WindowsDirectory();
            AppendDirectory(_T("Desktop"));
        }
        else
            // Failure, clear path
            Empty();
    }
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 start menu folder common 2 all users
//-------------------------------------------------------------
void CPath::CommonStartMenuDirectory()
{
    if(!ShellDirectory2(CSIDL_COMMON_STARTMENU))
    {
        // Check if running on Windows 95, and workaround this if so
        COSVersion osver;
        WORD       ostype =osver.GetOSType();
        if((ostype == OS_WIN95) || (ostype == OS_WIN98))
        {
            // Manual workaround
            WindowsDirectory();
            AppendDirectory(_T("Start Menu"));
        }
        else
            // Failure, clear path
            Empty();
    }
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 startup folder common 2 all users
//-------------------------------------------------------------
void CPath::CommonStartMenuStartupDirectory()
{
    if(!ShellDirectory2(CSIDL_COMMON_STARTUP))
    {
        // Check if running on Windows 95, and workaround this if so
        COSVersion osver;
        WORD       ostype =osver.GetOSType();
        if((ostype == OS_WIN95) || (ostype == OS_WIN98))
        {
            // Manual workaround
            WindowsDirectory();
            AppendDirectory(_T("Start Menu\\Programs\\StartUp"));
        }
        else
            // Failure, clear path
            Empty();
    }
}

//-------------------------------------------------------------
// Post    : If this function is called on a system which does not
//           support the new Chichago shell, it will clear the path
// Task    : Set path 2 programs menu folder common 2 all users
//-------------------------------------------------------------
void CPath::CommonStartMenuProgramsDirectory()
{
    if(!ShellDirectory2(CSIDL_COMMON_PROGRAMS))
    {
        // Check if running on Windows 95, and workaround this if so
        COSVersion osver;
        WORD       ostype =osver.GetOSType();
        if((ostype == OS_WIN95) || (ostype == OS_WIN98))
        {
            // Manual workaround
            WindowsDirectory();
            AppendDirectory(_T("Start Menu\\Programs"));
        }
        else
            // Failure, clear path
            Empty();
    }
}


//-------------------------------------------------------------
// Task    : Set path 2 file WIN.INI in the Windows directory
//-------------------------------------------------------------
void CPath::WindowsProfile()
{
    WindowsDirectory();
	SetNameExtension(_T("Win.INI"));
}

//-------------------------------------------------------------
// Task    : Set path 2 file WIN.INI in the Windows directory
//-------------------------------------------------------------
void CPath::SystemProfile()
{
    WindowsDirectory();
	SetNameExtension(_T("System.INI"));
}

//-------------------------------------------------------------
// Task    : Turn this path from "x:\directory\subdirectory\name.ext"
//           to just "x:\"
//-------------------------------------------------------------
void CPath::MakeRoot()
{   
	SetDirectory(_T(""));
	SetNameExtension(_T(""));
}

//-------------------------------------------------------------
// Pre     : Only the first 3 character from lpcszPrefix will be used
// Post    : Returns TRUE on success
// Task    : Creates a temporary name
//-------------------------------------------------------------
BOOL CPath::CreateTempName(LPCTSTR lpcszPrefix)
{
    // Check that we've got a prefix
    if(!lpcszPrefix)
        return FALSE;

	stdstr Dir;
    TCHAR  temp_file[MAX_PATH];

    GetDriveDirectory(Dir);

    if(::GetTempFileName(Dir.c_str(),lpcszPrefix,0,temp_file) != 0)
    {
        // Got a temp file name
        *this =temp_file;
        SetExtension(_T("tmp"));

        // GetTempFileName actually created the file, remove it now,
        // we only needed a name
        Delete(TRUE);

        return TRUE;
    }

	return FALSE;
}

//---------------------------------------------------------------------------
// Pre     : Only the first 3 character from lpcszPrefix will be used
// Post    : Returns TRUE on success
// Task    : Creates a temporary folder name
//---------------------------------------------------------------------------
BOOL CPath::CreateTempDir(LPCTSTR lpcszPrefix, UINT nRetries)
{
    // Check that we've got a prefix
    if(!lpcszPrefix)
        return FALSE;

    UINT  retries =0;
    BOOL  bSuccess =FALSE;
	TCHAR temp_prefix[ 5];
    TCHAR temp_name  [15];

	ZeroMemory(temp_prefix, sizeof(temp_prefix));
	_tcsncpy(temp_prefix,lpcszPrefix,4);
    temp_prefix[3] =_T('\0');

    while(!bSuccess && (retries < nRetries))
    {
        _tcscpy(temp_name, temp_prefix);
        stdstr temp =RandomDigits(5);
        _tcscat(temp_name,temp.c_str());
        _tcscat(temp_name,_T(".tmp"));

        CPath test(*this);

        test.AppendDirectory(temp_name);
        if(!test.DirectoryExists() && test.CreateDirectory())
        {
            // CreateTempDir actually created the folder, remove it now,
            // we only needed a name
            test.RemoveDirectory();
            bSuccess =TRUE;
        }

        retries++;
    }

    if(bSuccess)
        AppendDirectory(temp_name);

	return bSuccess;
}

//-------------------------------------------------------------
// Post    : Returns TRUE on success
// Task    : Sets path 2 a random name, and optionally ensures
//           uniqueness of that path
//-------------------------------------------------------------
BOOL CPath::CreateRandomName(BOOL bMustNotExist /*= TRUE*/, UINT nRetries /*= 1000*/)
{
	stdstr Name;

	for(UINT nRetry=0; nRetry < nRetries; nRetry++)
	{
		Name =RandomDigits(8);
		SetName(Name.c_str());
        if(!bMustNotExist)
            return TRUE;
		if(!Exists())
			return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------
// Post    : Returns TRUE on success
// Task    : Create a new name, based on the existing name, for the same
//           drive and directory. If bMustNotExist, test the path up to 
//           nRetries till we get an unused path
//           See also: CreateRandomName
//-------------------------------------------------------------
BOOL CPath::CreateSimilarName(BOOL bMustNotExist /*= TRUE*/, UINT nRetries /*= 1000*/)
{
	stdstr NewName;
    stdstr OriginalName;

	GetName(OriginalName);
		
	for(UINT nRetry=0; nRetry < nRetries; nRetry++)
	{
		NewName =OriginalName + RandomDigits(_MAX_FNAME - (int)OriginalName.length());
		SetName(NewName.c_str());
		if(!Exists() || !bMustNotExist)
			return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------
// Post    : Returns one of the EX_DRIVE_ constants
// Task    : Return the type of the drive this path points to
//           See DrvType.H for more details
//-------------------------------------------------------------
UINT CPath::GetDriveType() const
{
	CPath  RootPath = *this;
    stdstr Root;

	RootPath.MakeRoot();
    Root =(LPCTSTR)RootPath;

	return ::GetDriveType(Root.c_str());
}

//-------------------------------------------------------------
// Post    : Return -1 on error
// Task    : Find out the amount of free space on drive (in bytes)
//-------------------------------------------------------------
DWORD CPath::DriveFreeSpaceBytes() const
{
    CPath  RootPath = *this;
    stdstr Root;

	RootPath.MakeRoot();

	DWORD nSectorsPerCluster;
	DWORD nBytesPerSector;
	DWORD nFreeClusters;
	DWORD nClusters;

	if(!GetDiskFreeSpace((LPCTSTR)RootPath,&nSectorsPerCluster,&nBytesPerSector,&nFreeClusters,&nClusters))
		return 0;
	else		
		return nFreeClusters * nSectorsPerCluster * nBytesPerSector;
}

//-------------------------------------------------------------
// Post    : Return -1 on error
// Task    : Find out the size of the drive (in bytes)
//-------------------------------------------------------------
DWORD CPath::DriveTotalSpaceBytes() const
{
    CPath  RootPath = *this;
    stdstr Root;

	RootPath.MakeRoot();

	DWORD nSectorsPerCluster;
	DWORD nBytesPerSector;
	DWORD nFreeClusters;
	DWORD nClusters;

	if(!GetDiskFreeSpace((LPCTSTR)RootPath,&nSectorsPerCluster,&nBytesPerSector,&nFreeClusters,&nClusters))
		return 0;
	else		
		return nClusters * nSectorsPerCluster * nBytesPerSector;
}

//-------------------------------------------------------------
// Post    : Return -1 on error
// Task    : Find out the cluster size on this drive (in bytes)
//-------------------------------------------------------------
DWORD CPath::GetDriveClusterSize() const
{
    CPath  RootPath = *this;
    stdstr Root;

	RootPath.MakeRoot();

	DWORD nSectorsPerCluster;
	DWORD nBytesPerSector;
	DWORD nFreeClusters;
	DWORD nClusters;

	if(!GetDiskFreeSpace((LPCTSTR)RootPath,&nSectorsPerCluster,&nBytesPerSector,&nFreeClusters,&nClusters))
		return 0;
	else		
		return nSectorsPerCluster * nBytesPerSector;
}

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Find out info about drive
//-------------------------------------------------------------
BOOL CPath::GetDiskInfo(LPDWORD lpSectorsPerCluster,
						LPDWORD lpBytesPerSector,
						LPDWORD lpFreeClusters,
						LPDWORD lpClusters) const
{
    // Create root path
	CPath RootPath = *this;
	RootPath.MakeRoot();

	return GetDiskFreeSpace((LPCTSTR)RootPath,
                            lpSectorsPerCluster,
                            lpBytesPerSector,
                            lpFreeClusters,
                            lpClusters);
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
// Post    : Return TRUE if there are no files(s)/folder(s) in
//           directory. All objects (with hidden, system, etc. attributes)
//           are looked up
// Task    : Check if directory contains any file(s)
//-------------------------------------------------------------	
BOOL CPath::IsDirectoryEmpty() const
{
	CPath FileSpec = *this;
	
	FileSpec.SetNameExtension(WILD_NAME_EXTENSION);
	return !FileSpec.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY) &&
           !FileSpec.FindFirst(_A_HIDDEN | _A_SUBDIR);
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
// Post    : Return file size, -1 on error
// Task    : Get file size (in bytes)
//-------------------------------------------------------------
DWORD CPath::GetSize() const
{
    WIN32_FIND_DATA FindData;
	HANDLE          hFindFile =FindFirstFile(m_strPath.c_str(),&FindData);
	BOOL            bSuccess  =(hFindFile != INVALID_HANDLE_VALUE);

	if(hFindFile != NULL)	// Make sure we close the search
	    FindClose(hFindFile);

    return bSuccess ? FindData.nFileSizeLow : (DWORD)-1;
}

//-------------------------------------------------------------
// Post    : Return file attributes
// Task    : Get attributes of the file
//-------------------------------------------------------------
DWORD CPath::GetAttributes() const
{
    return GetFileAttributes(m_strPath.c_str());
}

//---------------------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Set the attributes of the file
//---------------------------------------------------------------------------
BOOL CPath::SetAttributes(DWORD dwAttributes)
{
    return SetFileAttributes(m_strPath.c_str(),dwAttributes);
}

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Get file creation, last acces and/or last modification 
//           time as local time
//-------------------------------------------------------------
BOOL CPath::GetTime(FILETIME *lpCreated, FILETIME *lpAccessed, FILETIME *lpModified) const
{
    WIN32_FIND_DATA findFileData;
	HANDLE hFind =FindFirstFile(m_strPath.c_str(),&findFileData);
	if(hFind == INVALID_HANDLE_VALUE)
        // Oops, no such file system object
		return FALSE;
    FindClose(hFind);

    FILETIME ftLastWriteTimeLocal;
    FileTimeToLocalFileTime(&findFileData.ftLastWriteTime,&ftLastWriteTimeLocal);

	if(lpCreated)
    {
        FILETIME ftCreationTimeLocal;
        FileTimeToLocalFileTime(&findFileData.ftCreationTime,&ftCreationTimeLocal);

        *lpCreated =ftCreationTimeLocal;
        if(!ftCreationTimeLocal.dwLowDateTime &&
           !ftCreationTimeLocal.dwHighDateTime)
           // Adjust time
           *lpCreated =ftLastWriteTimeLocal;
    }

    if(lpAccessed)
    {
        FILETIME ftLastAccessTimeLocal;
        FileTimeToLocalFileTime(&findFileData.ftLastAccessTime,&ftLastAccessTimeLocal);

        *lpAccessed =ftLastAccessTimeLocal;
        if(!ftLastAccessTimeLocal.dwLowDateTime &&
           !ftLastAccessTimeLocal.dwHighDateTime)
           // Adjust time
           *lpAccessed =ftLastWriteTimeLocal;
    }

    if(lpModified)
        *lpModified =ftLastWriteTimeLocal;

    return TRUE;
}

//---------------------------------------------------------------------------
// Post    : Return creation time
// Task    : Get the time this file/folder was created
//---------------------------------------------------------------------------
FILETIME CPath::GetTimeCreated() const
{
    FILETIME file_time;
    ZeroMemory(&file_time,sizeof(file_time));

    GetTime(&file_time,NULL,NULL);

    return file_time;
}

//---------------------------------------------------------------------------
// Post    : Return last access time
// Task    : Get the time this file/folder was last accessed
//---------------------------------------------------------------------------
FILETIME CPath::GetTimeLastAccessed() const
{
    FILETIME file_time;
    ZeroMemory(&file_time,sizeof(file_time));

    GetTime(NULL,&file_time,NULL);

    return file_time;
}

//---------------------------------------------------------------------------
// Post    : Return last modification time
// Task    : Get the time this file/folder was last changed
//---------------------------------------------------------------------------
FILETIME CPath::GetTimeLastModified() const
{
    FILETIME file_time;
    ZeroMemory(&file_time,sizeof(file_time));

    GetTime(NULL,NULL,&file_time);

    return file_time;
}

//---------------------------------------------------------------------------
// Pre     : All time parameters are supposed 2 be local times
// Post    : Return TRUE on success
// Task    : Set the creation, last acces and/or last modification time
//---------------------------------------------------------------------------
BOOL CPath::SetTime(const FILETIME *lpCreated, const FILETIME *lpAccessed, const FILETIME *lpModified)
{
    if(!lpCreated && !lpAccessed && !lpModified)
        // No time params specified
        return FALSE;

    WIN32_FIND_DATA findFileData;
	HANDLE hFind =FindFirstFile(m_strPath.c_str(),&findFileData);
	if(hFind == INVALID_HANDLE_VALUE)
        // Oops, no such file system object
		return FALSE;
    FindClose(hFind);

	if(lpCreated)
        LocalFileTimeToFileTime(lpCreated,&findFileData.ftCreationTime);
    if(lpAccessed)
        LocalFileTimeToFileTime(lpAccessed,&findFileData.ftLastAccessTime);
    if(lpModified)
        LocalFileTimeToFileTime(lpModified,&findFileData.ftLastWriteTime);

    HANDLE hFile =CreateFile(m_strPath.c_str(),GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        SetFileTime(hFile,lpCreated  ? &findFileData.ftCreationTime   : NULL,
                          lpAccessed ? &findFileData.ftLastAccessTime : NULL,
                          lpModified ? &findFileData.ftLastWriteTime  : NULL);
        CloseHandle(hFile);
        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
// Pre     : lpCreated is supposed 2 be local time
// Post    : Return TRUE on success
// Task    : Set the file's creation time
//---------------------------------------------------------------------------
BOOL CPath::SetTimeCreated(const FILETIME *lpCreated)
{
    return SetTime(lpCreated,NULL,NULL);
}

//---------------------------------------------------------------------------
// Pre     : lpModified is supposed 2 be local time
// Post    : Return TRUE on success
// Task    : Set the file's creation time
//---------------------------------------------------------------------------
BOOL CPath::SetTimeLastModified(const FILETIME *lpModified)
{
    return SetTime(NULL,NULL,lpModified);
}

//---------------------------------------------------------------------------
// Pre     : lpAccessed is supposed 2 be local time
// Post    : Return TRUE on success
// Task    : Set the file's creation time
//---------------------------------------------------------------------------
BOOL CPath::SetTimeLastAccessed(const FILETIME *lpAccessed)
{
    return SetTime(NULL,lpAccessed,NULL);
}

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Delete file
//-------------------------------------------------------------
BOOL CPath::Delete(BOOL bEvenIfReadOnly)
{
    DWORD dwAttr =::GetFileAttributes(m_strPath.c_str());
    if(dwAttr == (DWORD)-1)
        // File does not exists
        return FALSE;

    if(((dwAttr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY) && !bEvenIfReadOnly)
        // File is read-only, and we're not allowed 2 delete it
        return FALSE;

    SetFileAttributes(m_strPath.c_str(),FILE_ATTRIBUTE_NORMAL);
    return DeleteFile(m_strPath.c_str());
}	

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Rename file
//-------------------------------------------------------------
BOOL CPath::Rename(LPCTSTR lpszNewPath)
{
    return MoveTo(lpszNewPath,FALSE);
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
BOOL CPath::AttributesMatch(DWORD dwTargetAttributes, DWORD dwFileAttributes)
{
	if (dwTargetAttributes == _A_ALLFILES)
	{
		return true;
	}
	if(dwTargetAttributes == _A_NORMAL)
	{
		return ((_A_SUBDIR & dwFileAttributes) == 0);
	}
	else
	{
		return ( ((dwTargetAttributes & dwFileAttributes) != 0) &&
				 ((_A_SUBDIR & dwTargetAttributes) == (_A_SUBDIR & dwFileAttributes)) );
	}

    return FALSE;
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
BOOL CPath::FindFirst(DWORD dwAttributes /*= _A_NORMAL*/)
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
				} else {
					SetNameExtension("");
				}
				AppendDirectory(FindData.cFileName);
			} else {
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
// Post    : Return TRUE if deleted OK
// Task    : Delete everything in the directory
//-------------------------------------------------------------
BOOL CPath::RemoveDirectoryContent()
{
    // Deleting the directory's content
    // Iterate the content of the directory and delete it
    stdstr filename;
    CPath  iterator(*this);
    BOOL   deleted_OK =TRUE;

    // Deleting all contained files
    iterator.SetNameExtension(WILD_NAME_EXTENSION);
    BOOL iterating =iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY);
    while(iterating)
    {
        // Found something
        deleted_OK =iterator.Delete(TRUE);

        if(!deleted_OK)
            break;

        iterator.SetNameExtension(WILD_NAME_EXTENSION);
        iterating =iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY);
    }

    if(!deleted_OK)
        return FALSE;

    // Deleting all contained directories
    iterator.SetNameExtension(WILD_NAME_EXTENSION);
    iterating =iterator.FindFirst(_A_HIDDEN | _A_SUBDIR);
    while(iterating)
    {
        // Found something
        deleted_OK =iterator.RemoveDirectory(TRUE);

        if(!deleted_OK)
            break;

        iterator.SetNameExtension(WILD_NAME_EXTENSION);
        iterator.UpDirectory();
        iterating =iterator.FindFirst(_A_HIDDEN | _A_SUBDIR);
    }

    return deleted_OK;
}

//-------------------------------------------------------------
// Post    : Return TRUE if deleted OK
// Task    : Remove the directory
//-------------------------------------------------------------
BOOL CPath::RemoveDirectory(BOOL bEvenIfNotEmpty)
{
    if(bEvenIfNotEmpty)
    {
        // Delete the directory's content
        if(!RemoveDirectoryContent())
            return FALSE;
    }

    // Make sure there is no enumeration in progress,
    // otherwise we we'll get an error (sharing violation) because
    // that search keeps an open handle for this directory
    Exit();
    
    // Deleting this directory (and only if it's empty)
	stdstr DriveDirectory;
	GetDriveDirectory(DriveDirectory);

    return ::RemoveDirectory(DriveDirectory.c_str());
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
		bSuccess =ChangeDirectory();
		
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

//-------------------------------------------------------------
// Pre     : If bCreateIntermediates is TRUE, create all eventually
//           missing parent directories too
// Task    : Same as CreateDirectory, but throws an CPathException if
//           something goes wrong
//-------------------------------------------------------------
void CPath::CreateDirectoryEx(BOOL bCreateIntermediates /*= TRUE*/)
{                          
	BOOL bSuccess =CreateDirectory(bCreateIntermediates);
	
	if(!bSuccess)
		throw new CPathException(GetLastError());
}				

//Helpers

//------------------------------------------------------------------------
// Task    : Remove first character (if any) if it's chLeading
//------------------------------------------------------------------------
void CPath::cleanPathString(stdstr& rDirectory) const
{
	rDirectory.replace(DIRECTORY_DELIMITER2,DIRECTORY_DELIMITER);
	rDirectory.replace(DIR_DOUBLEDELIM,DIRECTORY_DELIMITER);
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

//------------------------------------------------------------------------
// Task    : Return path with swapped polarity on all backslashes
//------------------------------------------------------------------------
void CPath::GetAsInternetPath(stdstr& Directory) const
{
    Directory = m_strPath;
	if(!Directory.empty())
    {
        stdstr temp = Directory;
		for (int idx = 0; idx < (int)temp.size(); idx++)
		{
			if (temp[idx]==_T('\\'))
				temp[idx] = _T('/');
		}
		Directory = temp;
    }
	else
		Directory = _T("/");
}