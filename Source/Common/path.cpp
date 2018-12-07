// Path.cpp: implementation of the CPath class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#ifdef _WIN32
#include <Shlobj.h>
#include <dos.h>
#pragma warning(push)
#pragma warning(disable : 4996) // warning C4091: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#include <CommDlg.h>
#pragma warning(pop)
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#endif
#include "Platform.h"

/*
 * g_ModuleLogLevel may be NULL while AppInit() is still in session in path.cpp.
 * The added check to compare to NULL here is at least a temporary workaround.
 */
#undef WriteTrace
#ifdef _WIN32
#define WriteTrace(m, s, format, ...) if (g_ModuleLogLevel != NULL && g_ModuleLogLevel[(m)] >= (s)) { WriteTraceFull((m), (s), __FILE__, __LINE__, __FUNCTION__, (format), ## __VA_ARGS__); }
#else
#define WriteTrace(m, s, format, ...) if (g_ModuleLogLevel != NULL && g_ModuleLogLevel[(m)] >= (s)) { WriteTraceFull((m), (s), __FILE__, __LINE__, __PRETTY_FUNCTION__, (format), ## __VA_ARGS__); }
#endif

//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
const char DRIVE_DELIMITER = ':';
const char * const DIR_DOUBLEDELIM = "\\\\";
const char DIRECTORY_DELIMITER = '\\';
const char DIRECTORY_DELIMITER2 = '/';
#else
const char * const DIR_DOUBLEDELIM = "//";
const char DIRECTORY_DELIMITER = '/';
const char DIRECTORY_DELIMITER2 = '\\';
#endif
const char EXTENSION_DELIMITER = '.';
#ifdef _WIN32
void * CPath::m_hInst = NULL;
#endif

//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////
#ifdef _WIN32
void CPath::SethInst(void * hInst)
{
    m_hInst = hInst;
}

void * CPath::GethInst()
{
    return m_hInst;
}
#endif
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
    m_dwFindFileAttributes = 0;
#ifdef _WIN32
    m_hFindFile = NULL;
#else
    m_OpenedDir = NULL;
    m_FindWildcard = "";
#endif
}

//-------------------------------------------------------------
// Task    : Helper function for the various CPath destructors.
//           Cleans up varios internals
//-------------------------------------------------------------
inline void CPath::Exit()
{
#ifdef _WIN32
    if (m_hFindFile != NULL)
    {
        FindClose(m_hFindFile);
        m_hFindFile = NULL;
    }
#else
    if (m_OpenedDir != NULL)
    {
        closedir((DIR*)m_OpenedDir);
        m_OpenedDir = NULL;
    }
#endif
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
    m_strPath = rPath.m_strPath;
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 lpszPath
//-------------------------------------------------------------
CPath::CPath(const char * lpszPath)
{
    Init();
    m_strPath = lpszPath ? lpszPath : "";
    cleanPathString(m_strPath);
}

CPath::CPath(const char * lpszPath, const char * NameExten)
{
    WriteTrace(TracePath, TraceDebug, "Start (lpszPath: \"%s\" NameExten: \"%s\")", lpszPath ? lpszPath : "(null)", NameExten ? NameExten : "(null)");
    Init();
#ifdef _WIN32
    SetDriveDirectory(lpszPath);
#else
    SetDirectory(lpszPath);
#endif
    SetNameExtension(NameExten);
    WriteTrace(TracePath, TraceDebug, "Done (m_strPath: \"%s\")", m_strPath.c_str());
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const std::string& strPath)
{
    Init();
    m_strPath = strPath;
    cleanPathString(m_strPath);
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const std::string& strPath, const char * NameExten)
{
    Init();
#ifdef _WIN32
    SetDriveDirectory(strPath.c_str());
#else
    SetDirectory(strPath.c_str(),true);
#endif
    SetNameExtension(NameExten);
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const std::string& strPath, const std::string& NameExten)
{
    Init();
#ifdef _WIN32
    SetDriveDirectory(strPath.c_str());
#else
    SetDirectory(strPath.c_str(),true);
#endif
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
bool CPath::operator ==(const CPath& rPath) const
{
    // Get fully qualified versions of the paths
    std::string FullyQualified1;
    std::string FullyQualified2;

    GetFullyQualified(FullyQualified1);
    rPath.GetFullyQualified(FullyQualified2);

    // Compare them
    return _stricmp(FullyQualified1.c_str(), FullyQualified2.c_str()) == 0;
}

//-------------------------------------------------------------
// Post    : Return TRUE if paths are different
// Task    : Check if the two path are different
//-------------------------------------------------------------
bool CPath::operator !=(const CPath& rPath) const
{
    return !(*this == rPath);
}

//-------------------------------------------------------------
// Task    : Assign a path 2 another
//-------------------------------------------------------------
CPath& CPath::operator =(const CPath& rPath)
{
    if (this != &rPath)
    {
        m_strPath = rPath.m_strPath;
    }
    return *this;
}

//-------------------------------------------------------------
// Post    : Return the path, so that assignements can be chained
// Task    : Assign a string 2 a path
//-------------------------------------------------------------
CPath& CPath::operator =(const char * lpszPath)
{
    m_strPath = lpszPath ? lpszPath : "";
    return *this;
}

//-------------------------------------------------------------
// Post    : Return the path, so that assignements can be chained
// Task    : Assign a string 2 a path
//-------------------------------------------------------------
CPath& CPath::operator =(const std::string& strPath)
{
    m_strPath = strPath;
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
CPath::operator const char *() const
{
    return (const char *)m_strPath.c_str();
}

CPath::CPath(DIR_CURRENT_DIRECTORY /*sdt*/, const char * NameExten)
{
    // Application's current directory
    Init();
    CurrentDirectory();
    if (NameExten) { SetNameExtension(NameExten); }
}

#ifdef _WIN32
CPath::CPath(DIR_MODULE_DIRECTORY /*sdt*/, const char * NameExten)
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
#endif

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
#ifdef _WIN32
void CPath::GetComponents(std::string* pDrive, std::string* pDirectory, std::string* pName, std::string* pExtension) const
{
    WriteTrace(TracePath, TraceDebug, "Start (m_strPath: \"%s\")", m_strPath.c_str());

    char buff_drive[_MAX_DRIVE + 1] = { 0 };
    char buff_dir[_MAX_DIR + 1] = { 0 };
    char buff_name[_MAX_FNAME + 1] = { 0 };
    char buff_ext[_MAX_EXT + 1] = { 0 };

    const char * BasePath = m_strPath.c_str();
    const char * DriveDir = strrchr(BasePath, DRIVE_DELIMITER);
    if (DriveDir != NULL)
    {
        int len = sizeof(buff_dir) < (DriveDir - BasePath) ? sizeof(buff_drive) : DriveDir - BasePath;
        strncpy(buff_drive, BasePath, len);
        BasePath += len + 1;
    }

    const char * last = strrchr(BasePath, DIRECTORY_DELIMITER);
    if (last != NULL)
    {
        int len = sizeof(buff_dir) < (last - BasePath) ? sizeof(buff_dir) : last - BasePath;
        if (len > 0)
        {
            strncpy(buff_dir, BasePath, len);
        }
        else
        {
            buff_dir[0] = DIRECTORY_DELIMITER;
            buff_dir[1] = '\0';
        }
        strncpy(buff_name, last + 1, sizeof(buff_name));
    }
    else
    {
        strncpy(buff_dir, BasePath, sizeof(buff_dir));
    }
    char * ext = strrchr(buff_name, '.');
    if (ext != NULL)
    {
        strncpy(buff_ext, ext + 1, sizeof(buff_ext));
        *ext = '\0';
    }

    if (pDrive)
    {
        *pDrive = buff_drive;
    }
    if (pDirectory)
    {
        *pDirectory = buff_dir;
    }
    if (pName)
    {
        *pName = buff_name;
    }
    if (pExtension)
    {
        *pExtension = buff_ext;
    }
    WriteTrace(TracePath, TraceDebug, "Done (dir: \"%s\" name: \"%s\" ext: \"%s\")", buff_dir, buff_name, buff_ext);
}
#else
void CPath::GetComponents(std::string* pDirectory, std::string* pName, std::string* pExtension) const
{
    WriteTrace(TracePath, TraceDebug, "Start (m_strPath: \"%s\")",m_strPath.c_str());

    char buff_dir[260];
    char buff_name[260];
    char buff_ext[260];

    memset(buff_dir, 0, sizeof(buff_dir));
    memset(buff_name, 0, sizeof(buff_name));
    memset(buff_ext, 0, sizeof(buff_ext));

    const char * BasePath = m_strPath.c_str();
    const char * last = strrchr(BasePath,DIRECTORY_DELIMITER);
    if (last != NULL)
    {
        int len = sizeof(buff_dir) < (last - BasePath) ? sizeof(buff_dir) : last - BasePath;
        if (len > 0)
        {
            strncpy(buff_dir,BasePath,len);
        }
        else
        {
            buff_dir[0] = DIRECTORY_DELIMITER;
            buff_dir[1] = '\0';
        }
        strncpy(buff_name,last + 1,sizeof(buff_name));
    }
    else
    {
        strncpy(buff_dir,BasePath,sizeof(buff_dir));
    }
    char * ext = strrchr(buff_name,'.');
    if (ext != NULL)
    {
        strncpy(buff_ext,ext + 1,sizeof(buff_ext));
        *ext = '\0';
    }

    if (pDirectory)
    {
        *pDirectory = buff_dir;
    }
    if (pName)
    {
        *pName = buff_name;
    }
    if (pExtension)
    {
        *pExtension = buff_ext;
    }
    WriteTrace(TracePath, TraceDebug, "Done (dir: \"%s\" name: \"%s\" ext: \"%s\")",buff_dir,buff_name,buff_ext);
}
#endif

//-------------------------------------------------------------
// Task    : Get drive and directory from path
//-------------------------------------------------------------
#ifdef _WIN32
void CPath::GetDriveDirectory(std::string& rDriveDirectory) const
{
    std::string Drive;
    std::string Directory;

    GetComponents(&Drive, &Directory);
    rDriveDirectory = Drive;
    if (!Drive.empty())
    {
        rDriveDirectory += DRIVE_DELIMITER;
    }
    rDriveDirectory += Directory;
}

std::string CPath::GetDriveDirectory(void) const
{
    std::string rDriveDirectory;
    GetDriveDirectory(rDriveDirectory);
    return rDriveDirectory;
}
#endif

//-------------------------------------------------------------
// Task    : Get directory from path
//-------------------------------------------------------------
void CPath::GetDirectory(std::string& rDirectory) const
{
#ifdef _WIN32
    GetComponents(NULL, &rDirectory);
#else
    GetComponents(&rDirectory);
#endif
}

std::string CPath::GetDirectory(void) const
{
    std::string rDirectory;
    GetDirectory(rDirectory);
    return rDirectory;
}

//-------------------------------------------------------------
// Task    : Get filename and extension from path
//-------------------------------------------------------------
void CPath::GetNameExtension(std::string& rNameExtension) const
{
    std::string Name;
    std::string Extension;

#ifdef _WIN32
    GetComponents(NULL, NULL, &Name, &Extension);
#else
    GetComponents(NULL, &Name, &Extension);
#endif
    rNameExtension = Name;
    if (!Extension.empty())
    {
        rNameExtension += EXTENSION_DELIMITER;
        rNameExtension += Extension;
    }
}

std::string CPath::GetNameExtension(void) const
{
    std::string rNameExtension;
    GetNameExtension(rNameExtension);
    return rNameExtension;
}

//-------------------------------------------------------------
// Task    : Get filename from path
//-------------------------------------------------------------
void CPath::GetName(std::string& rName) const
{
#ifdef _WIN32
    GetComponents(NULL, NULL, &rName);
#else
    GetComponents(NULL, &rName);
#endif
}

std::string CPath::GetName(void) const
{
    std::string rName;
    GetName(rName);
    return rName;
}

//-------------------------------------------------------------
// Task    : Get file extension from path
//-------------------------------------------------------------
void CPath::GetExtension(std::string& rExtension) const
{
#ifdef _WIN32
    GetComponents(NULL, NULL, NULL, &rExtension);
#else
    GetComponents(NULL, NULL, &rExtension);
#endif
}

std::string CPath::GetExtension(void) const
{
    std::string rExtension;
    GetExtension(rExtension);
    return rExtension;
}

//-------------------------------------------------------------
// Task    : Get current directory
//-------------------------------------------------------------
void CPath::GetLastDirectory(std::string& rDirectory) const
{
    std::string Directory;

    rDirectory = "";

    GetDirectory(Directory);
    StripTrailingBackslash(Directory);
    if (Directory.empty())
    {
        return;
    }

    std::string::size_type nDelimiter = Directory.rfind(DIRECTORY_DELIMITER);
    rDirectory = Directory.substr(nDelimiter);
    StripLeadingBackslash(rDirectory);
}

std::string CPath::GetLastDirectory(void) const
{
    std::string rDirecotry;
    GetLastDirectory(rDirecotry);
    return rDirecotry;
}

//-------------------------------------------------------------
// Task    : Get fully qualified path
//-------------------------------------------------------------
void CPath::GetFullyQualified(std::string& rFullyQualified) const
{
#ifdef _WIN32
    char buff_fullname[MAX_PATH];

    memset(buff_fullname, 0, sizeof(buff_fullname));

    _fullpath(buff_fullname, m_strPath.c_str(), MAX_PATH - 1);
    rFullyQualified = buff_fullname;
#endif
}

//-------------------------------------------------------------
// Post    : Return TRUE if path does not start from filesystem root
// Task    : Check if path is a relative one (e.g. doesn't start with C:\...)
//-------------------------------------------------------------
bool CPath::IsRelative() const
{
#ifdef _WIN32
    if (m_strPath.length() > 1 && m_strPath[1] == DRIVE_DELIMITER)
    {
        return false;
    }
#endif
    if (m_strPath.length() > 1 && m_strPath[0] == DIRECTORY_DELIMITER && m_strPath[1] == DIRECTORY_DELIMITER)
    {
        return false;
    }
    return true;
}

//-------------------------------------------------------------
// Task    : Set path components
//-------------------------------------------------------------
#ifdef _WIN32
void CPath::SetComponents(const char * lpszDrive, const char * lpszDirectory, const char * lpszName, const char * lpszExtension)
{
    char buff_fullname[MAX_PATH];

    memset(buff_fullname, 0, sizeof(buff_fullname));
    if (lpszDirectory == NULL || strlen(lpszDirectory) == 0)
    {
        static char empty_dir[] = { DIRECTORY_DELIMITER, '\0' };
        lpszDirectory = empty_dir;
    }

    _makepath(buff_fullname, lpszDrive, lpszDirectory, lpszName, lpszExtension);
    m_strPath.erase();
    m_strPath = buff_fullname;
}
#else
void CPath::SetComponents(const char * lpszDirectory, const char * lpszName, const char * lpszExtension)
{
    char buff_fullname[260];

    memset(buff_fullname, 0, sizeof(buff_fullname));
    if (lpszDirectory != NULL && lpszDirectory[0] != '\0')
    {
        if (lpszDirectory[0] != DIRECTORY_DELIMITER)  { buff_fullname[0] = DIRECTORY_DELIMITER; }
        strncat(buff_fullname,lpszDirectory,sizeof(buff_fullname) - 1);
        std::string::size_type nLength = strlen(buff_fullname);
        if (buff_fullname[nLength - 1] != DIRECTORY_DELIMITER &&  nLength < sizeof(buff_fullname))
        {
            buff_fullname[nLength] = DIRECTORY_DELIMITER;
        }
    }
    if (lpszName != NULL)
    {
        strncat(buff_fullname,lpszName,sizeof(buff_fullname) - 1);
    }
    if (lpszExtension != NULL && lpszExtension[0] != '\0')
    {
        if (lpszExtension[0] != '.')
        {
            strncat(buff_fullname,".",sizeof(buff_fullname)-1);
        }
        strncat(buff_fullname,lpszExtension,sizeof(buff_fullname)-1);
    }
    buff_fullname[sizeof(buff_fullname) - 1] = 0; //Make sure it is null terminated
    m_strPath.erase();
    m_strPath = buff_fullname;
}
#endif

//-------------------------------------------------------------
// Task    : Set path's drive
//-------------------------------------------------------------
#ifdef _WIN32
void CPath::SetDrive(char chDrive)
{
    stdstr_f Drive("%c", chDrive);
    std::string	 Directory;
    std::string	 Name;
    std::string	 Extension;

    GetComponents(NULL, &Directory, &Name, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), Extension.c_str());
}
#endif

//-------------------------------------------------------------
// Task    : Set path's directory
//-------------------------------------------------------------
void CPath::SetDirectory(const char * lpszDirectory, bool bEnsureAbsolute /*= false*/)
{
    WriteTrace(TracePath, TraceDebug, "start (lpszDirectory: \"%s\" bEnsureAbsolute: %s)", lpszDirectory ? lpszDirectory : "(null)", bEnsureAbsolute ? "true" : "false");
    std::string	Directory = lpszDirectory;
    std::string	Name;
    std::string	Extension;

    if (bEnsureAbsolute)
    {
        EnsureLeadingBackslash(Directory);
    }
    if (Directory.length() > 0)
    {
        EnsureTrailingBackslash(Directory);
    }

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, NULL, &Name, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), Extension.c_str());
#else
    GetComponents(NULL, &Name, &Extension);
    SetComponents(Directory.c_str(), Name.c_str(), Extension.c_str());
#endif
    WriteTrace(TracePath, TraceDebug, "Done (m_strPath: \"%s\")", m_strPath.c_str());
}

#ifdef _WIN32
//-------------------------------------------------------------
// Task    : Set path's drive and directory
//-------------------------------------------------------------
void CPath::SetDriveDirectory(const char * lpszDriveDirectory)
{
    std::string	DriveDirectory = lpszDriveDirectory;
    std::string	Name;
    std::string	Extension;

    if (DriveDirectory.length() > 0)
    {
        EnsureTrailingBackslash(DriveDirectory);
        cleanPathString(DriveDirectory);
    }

    GetComponents(NULL, NULL, &Name, &Extension);
    SetComponents(NULL, DriveDirectory.c_str(), Name.c_str(), Extension.c_str());
}
#endif

//-------------------------------------------------------------
// Task    : Set path's filename
//-------------------------------------------------------------
void CPath::SetName(const char * lpszName)
{
    std::string	Directory;
    std::string	Extension;

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, &Directory, NULL, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), lpszName, Extension.c_str());
#else
    GetComponents(&Directory, NULL, &Extension);
    SetComponents(Directory.c_str(), lpszName, Extension.c_str());
#endif
}

//-------------------------------------------------------------
// Task    : Set path's filename
//-------------------------------------------------------------
void CPath::SetName(int iName)
{
    std::string	Directory;
    std::string	Extension;
    char 	sName[33];

    memset(sName, 0, sizeof(sName));

    _snprintf(sName, sizeof(sName), "%d", iName);

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, &Directory, NULL, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), sName, Extension.c_str());
#else
    GetComponents(&Directory, NULL, &Extension);
    SetComponents(Directory.c_str(), sName, Extension.c_str());
#endif
}

//-------------------------------------------------------------
// Task    : Set path's file extension
//-------------------------------------------------------------
void CPath::SetExtension(const char * lpszExtension)
{
    std::string	Directory;
    std::string	Name;

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, &Directory, &Name);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), lpszExtension);
#else
    GetComponents(&Directory, &Name);
    SetComponents(Directory.c_str(), Name.c_str(), lpszExtension);
#endif
}

//-------------------------------------------------------------
// Task    : Set path's file extension
//-------------------------------------------------------------
void CPath::SetExtension(int iExtension)
{
    std::string	Directory;
    std::string	Name;
    char sExtension[20];

    memset(sExtension, 0, sizeof(sExtension));

    _snprintf(sExtension, sizeof(sExtension), "%d", iExtension);

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, &Directory, &Name);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), sExtension);
#else
    GetComponents(&Directory, &Name);
    SetComponents(Directory.c_str(), Name.c_str(), sExtension);
#endif
}

//-------------------------------------------------------------
// Task    : Set path's filename and extension
//-------------------------------------------------------------
void CPath::SetNameExtension(const char * lpszNameExtension)
{
    std::string	Directory;

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, &Directory);
    SetComponents(Drive.c_str(), Directory.c_str(), lpszNameExtension, NULL);
#else
    GetComponents(&Directory);
    SetComponents(Directory.c_str(), lpszNameExtension, NULL);
#endif
}

//-------------------------------------------------------------
// Task    : Append a subdirectory 2 path's directory
//-------------------------------------------------------------
void CPath::AppendDirectory(const char * lpszSubDirectory)
{
    std::string	Directory;
    std::string	SubDirectory = lpszSubDirectory;
    std::string	Name;
    std::string	Extension;

    if (SubDirectory.empty())
    {
        return;
    }

    // Strip out any preceeding backslash
    StripLeadingBackslash(SubDirectory);
    EnsureTrailingBackslash(SubDirectory);

#ifdef _WIN32
    std::string	Drive;
    GetComponents(&Drive, &Directory, &Name, &Extension);
#else
    GetComponents(&Directory, &Name, &Extension);
#endif
    EnsureTrailingBackslash(Directory);
    Directory += SubDirectory;

#ifdef _WIN32
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), Extension.c_str());
#else
    SetComponents(Directory.c_str(), Name.c_str(), Extension.c_str());
#endif
}

//-------------------------------------------------------------
// Pre     : If pLastDirectory is given we will store the name of the
//           deepest directory (the one we're just exiting) in it
// Task    : Remove deepest subdirectory from path
//-------------------------------------------------------------
void CPath::UpDirectory(std::string *pLastDirectory /*= NULL*/)
{
    std::string Directory;

    GetDirectory(Directory);
    StripTrailingBackslash(Directory);
    if (Directory.empty())
        return;

    std::string::size_type nDelimiter = Directory.rfind(DIRECTORY_DELIMITER);

    if (pLastDirectory != NULL)
    {
        *pLastDirectory = Directory.substr(nDelimiter);
        StripLeadingBackslash(*pLastDirectory);
    }

    if (nDelimiter != std::string::npos)
    {
        Directory = Directory.substr(0, nDelimiter);
    }

    SetDirectory(Directory.c_str());
}

//-------------------------------------------------------------
// Task    : Set path 2 current directory
//-------------------------------------------------------------
void CPath::CurrentDirectory()
{
    char buff_path[260];
    memset(buff_path, 0, sizeof(buff_path));

    Empty();

#ifdef _WIN32
    ::GetCurrentDirectory(sizeof(buff_path), buff_path);
    SetDriveDirectory(buff_path);
#else
    getcwd(buff_path, sizeof(buff_path));
    SetDirectory(buff_path);
#endif
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of specified module
//-------------------------------------------------------------
#ifdef _WIN32
void CPath::Module(void * hInstance)
{
    char buff_path[MAX_PATH];

    memset(buff_path, 0, sizeof(buff_path));

    GetModuleFileName((HINSTANCE)hInstance, buff_path, MAX_PATH);
    m_strPath = buff_path;
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of current module
//-------------------------------------------------------------
void CPath::Module()
{
    Module(m_hInst);
}

//-------------------------------------------------------------
// Task    : Set path 2 the directory of specified module
//-------------------------------------------------------------
void CPath::ModuleDirectory(void * hInstance)
{
    Module(hInstance);
    SetNameExtension("");
}

//-------------------------------------------------------------
// Task    : Set path 2 the directory of current module
//-------------------------------------------------------------
void CPath::ModuleDirectory()
{
    Module();
    SetNameExtension("");
}
#endif

//---------------------------------------------------------------------------
// Post    : Return TRUE if a directory
// Task    : Check if this path represents a directory
//---------------------------------------------------------------------------
bool CPath::IsDirectory() const
{
    // Check if this path has a filename
    std::string file_name;
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
bool CPath::DirectoryExists() const
{
    WriteTrace(TracePath, TraceDebug, "m_strPath = %s",m_strPath.c_str());
#ifdef _WIN32
    // Create test path
    CPath TestPath(m_strPath.c_str());

    std::string DirName;
    TestPath.UpDirectory(&DirName);
    TestPath.SetNameExtension(DirName.c_str());

    WIN32_FIND_DATA	FindData;
    HANDLE hFindFile = FindFirstFile((const char *)TestPath, &FindData); // Find anything
    bool res = (hFindFile != INVALID_HANDLE_VALUE);

    if (hFindFile != NULL)	// Make sure we close the search
    {
        FindClose(hFindFile);
    }

#else
    std::string	PathText;
    GetDirectory(PathText);
    StripTrailingBackslash(PathText);
    WriteTrace(TracePath, TraceDebug, "Checking if directory \"%s\" exists",PathText.c_str());

    struct stat fileinfo;
    bool res = stat(PathText.c_str(), &fileinfo) == 0 && S_ISDIR(fileinfo.st_mode);
#endif
    WriteTrace(TracePath, TraceDebug, "Exist = %s",res ? "True" : "False");
    return res;
}

//-------------------------------------------------------------
// Post    : Return TRUE if these is such a file
// Task    : Check if file exists
//-------------------------------------------------------------
bool CPath::Exists() const
{
#ifdef _WIN32
    WIN32_FIND_DATA FindData;
    HANDLE          hFindFile = FindFirstFile(m_strPath.c_str(), &FindData);
    bool            bSuccess = (hFindFile != INVALID_HANDLE_VALUE);

    if (hFindFile != NULL)	// Make sure we close the search
    {
        FindClose(hFindFile);
    }

    return bSuccess;
#else
    struct stat statbuf;
    return stat(m_strPath.c_str(), &statbuf) == 0;
#endif
}

#ifdef _WIN32
bool CPath::SelectFile(void * hwndOwner, const char * InitialDir, const char * FileFilter, bool FileMustExist)
{
    CPath CurrentDir(CURRENT_DIRECTORY);

    OPENFILENAME openfilename;
    char FileName[MAX_PATH];
    memset(&FileName, 0, sizeof(FileName));
    memset(&openfilename, 0, sizeof(openfilename));

    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = (HWND)hwndOwner;
    openfilename.lpstrFilter = FileFilter;
    openfilename.lpstrFile = FileName;
    openfilename.lpstrInitialDir = InitialDir;
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_HIDEREADONLY | (FileMustExist ? OFN_FILEMUSTEXIST : 0);

    bool res = GetOpenFileName(&openfilename);
    if (CPath(CURRENT_DIRECTORY) != CurrentDir)
    {
        CurrentDir.ChangeDirectory();
    }
    if (!res)
    {
        return false;
    }
    m_strPath = FileName;
    cleanPathString(m_strPath);
    return true;
}
#endif

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Delete file
//-------------------------------------------------------------
bool CPath::Delete(bool bEvenIfReadOnly) const
{
#ifdef _WIN32
    uint32_t dwAttr = ::GetFileAttributes(m_strPath.c_str());
    if (dwAttr == (uint32_t)-1)
    {
        // File does not exist.
        return false;
    }

    if (((dwAttr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY) && !bEvenIfReadOnly)
    {
        // File is read-only, and we're not allowed 2 delete it
        return false;
    }

    SetFileAttributes(m_strPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    return DeleteFile(m_strPath.c_str()) != 0;
#else
    return unlink(m_strPath.c_str()) == 0;
#endif
}

//-------------------------------------------------------------
// Post    : Return TRUE on success, false if there is such a target file
//           and we weren't granted permission 2 overwrite file or some error
// Task    : Copy file
//           Since ::CopyFile will not overwrite read only files
//           we will make sure the target file is writable first
//-------------------------------------------------------------
bool CPath::CopyTo(const char * lpcszTargetFile, bool bOverwrite)
{
    if (lpcszTargetFile == NULL)
    {
        return false;
    }
    WriteTrace(TracePath, TraceDebug, "copy \"%s\" to \"%s\"",m_strPath.c_str(),lpcszTargetFile);
#ifdef _WIN32
    // Check if the target file exists
    CPath TargetFile(lpcszTargetFile);
    if (TargetFile.Exists())
    {
        // Yeah there is already such a target file
        // Decide if we should overwrite
        if (!bOverwrite)
        {
            return false;
        }

        // Delete any previous target
        if (!TargetFile.Delete(true))
        {
            return false;
        }
    }

    // CopyFile will set the target's attributes 2 the same as
    // the source after copying
    return CopyFile(m_strPath.c_str(), lpcszTargetFile, !bOverwrite) != 0;
#else

    bool res = true;
    WriteTrace(TracePath, TraceDebug, "opening \"%s\" for reading",m_strPath.c_str());
    FILE * infile = fopen(m_strPath.c_str(), "rb");
    if(infile == NULL)
    {
        WriteTrace(TracePath, TraceWarning, "failed to open m_strPath = %s",m_strPath.c_str());
        res = false;
    }
    else
    {
        WriteTrace(TracePath, TraceDebug, "opened \"%s\"",m_strPath.c_str());
    }

    FILE * outfile = NULL;
    if (res)
    {
        WriteTrace(TracePath, TraceDebug, "opening \"%s\" for writing",lpcszTargetFile);
        outfile = fopen(lpcszTargetFile, "wb");
        if (outfile == NULL)
        {
            WriteTrace(TracePath, TraceWarning, "failed to open m_strPath = %s errno=%d",lpcszTargetFile, errno);
            res = false;
        }
        else
        {
            WriteTrace(TracePath, TraceDebug, "opened \"%s\"",lpcszTargetFile);
        }
    }

    if (res)
    {
        WriteTrace(TracePath, TraceDebug, "copying data");
        while (!feof(infile))
        {
            char buffer[1024];
            size_t bytes = fread(buffer, 1, sizeof(buffer), infile); 
            if (ferror(infile))
            {
                WriteTrace(TracePath, TraceWarning, "failed to read from %s", m_strPath.c_str());
                res = false;
                break;
            }
            if (!feof(infile))
            {
                size_t written = fwrite(buffer, 1, sizeof(buffer), outfile);
            }
            if (ferror(outfile))
            {
                WriteTrace(TracePath, TraceWarning, "failed to write to %s, ferror(outfile) = %X", lpcszTargetFile, ferror(outfile));
                res = false;
                break;
            }
        }
    }

    struct stat ts;
    if (res)
    {
        if (fstat(fileno(infile), &ts) != 0)
        {
            WriteTrace(TracePath, TraceWarning, "fstat failed on %s, ferror(infile) = %X", m_strPath.c_str(), ferror(infile));
            res = false;
        }
    }
    if (res)
    {
        if (fchmod(fileno(outfile),ts.st_mode) != 0)
        {
            WriteTrace(TracePath, TraceWarning, "fchmod failed on %s, errno = %X", lpcszTargetFile, errno);
            res = false;
        }
    }
    if (infile != NULL)
    {
        fclose(infile);
    }
    if (outfile != NULL)
    {
        fclose(outfile);
    }
    WriteTrace(TracePath, TraceDebug, "Done, res: %s",res ? "true" : "false");
    return res;
#endif
}

//-------------------------------------------------------------
// Post    : Return TRUE on success, false if there is such a target file
//           and we weren't granted permission 2 overwrite file or some error
// Task    : Move file
//-------------------------------------------------------------
bool CPath::MoveTo(const char * lpcszTargetFile, bool bOverwrite)
{
#ifdef _WIN32
    // Check if the target file exists
    CPath TargetFile(lpcszTargetFile);
    if (TargetFile.Exists())
    {
        // Yeah there is already such a target file
        // Decide if we should overwrite
        if (!bOverwrite)
        {
            return false;
        }

        // Delete any previous target
        if (!TargetFile.Delete(TRUE))
        {
            return false;
        }
    }

    return MoveFile(m_strPath.c_str(), lpcszTargetFile) != 0;
#else
    return false;
#endif
}

//-------------------------------------------------------------
// Post    : Return TRUE if attributes do match
// Task    : Compare finder attributes
//-------------------------------------------------------------
bool CPath::AttributesMatch(uint32_t dwTargetAttributes, uint32_t dwFileAttributes)
{
    if (dwTargetAttributes == FIND_ATTRIBUTE_ALLFILES)
    {
        return true;
    }
    if (dwTargetAttributes == FIND_ATTRIBUTE_FILES)
    {
        return ((FIND_ATTRIBUTE_SUBDIR & dwFileAttributes) == 0);
    }
    return (((dwTargetAttributes & dwFileAttributes) != 0) && ((FIND_ATTRIBUTE_SUBDIR & dwTargetAttributes) == (FIND_ATTRIBUTE_SUBDIR & dwFileAttributes)));
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
//           FIND_ATTRIBUTE_FILES  Normal. File can be read or written to
//                                 without restriction
//           _A_RDONLY  Read-only. File cannot be opened for writing,
//                      and a file with the same name cannot be created
//           FIND_ATTRIBUTE_SUBDIR Subdirectory
//           _A_SYSTEM  System file. Not normally seen with the DIR
//                      command, unless the /AS option is used
//
//           These attributes do not follow a simple additive logic
//           Note that FIND_ATTRIBUTE_FILES is 0x00, so it effectively cannot be
//           removed from the attribute set. You will therefore always
//           get normal files, and may also get Archive, Hidden, etc.
//           if you specify those attributes
//           See aso: FindFirstFile, FindNextFile
//-------------------------------------------------------------
bool CPath::FindFirst(uint32_t dwAttributes /*= FIND_ATTRIBUTE_FILES*/)
{
    // Close handle to any previous enumeration
    Exit();

    m_dwFindFileAttributes = dwAttributes;
#ifdef _WIN32
    BOOL bGotFile;
    BOOL bWantSubdirectory = (BOOL)(FIND_ATTRIBUTE_SUBDIR & dwAttributes);

    // i.) Finding first candidate file
    WIN32_FIND_DATA	FindData;
    m_hFindFile = FindFirstFile(m_strPath.c_str(), &FindData);
    bGotFile = (m_hFindFile != INVALID_HANDLE_VALUE);

    while (bGotFile)
    {
        // ii.) Compare candidate to attributes, and filter out the "." and ".." folders
        if (!AttributesMatch(m_dwFindFileAttributes, FindData.dwFileAttributes))
            goto LABEL_GetAnother;
        if (bWantSubdirectory && (FindData.cFileName[0] == '.'))
            goto LABEL_GetAnother;

        // iii.) Found match, prepare result
        if ((FIND_ATTRIBUTE_SUBDIR & FindData.dwFileAttributes) != 0)
            StripTrailingBackslash(m_strPath);

        SetNameExtension(FindData.cFileName);

        if ((FIND_ATTRIBUTE_SUBDIR & FindData.dwFileAttributes) != 0)
            EnsureTrailingBackslash(m_strPath);
        return TRUE;

        // iv.) Not found match, get another
    LABEL_GetAnother:
        bGotFile = FindNextFile(m_hFindFile, &FindData);
    }
#else
    std::string Directory, Name, Extension;
    GetComponents(&Directory, &Name, &Extension);

    m_FindWildcard = Name;
    if (!Extension.empty())
    {
        m_FindWildcard += EXTENSION_DELIMITER;
        m_FindWildcard += Extension;
    }

    m_OpenedDir = opendir(Directory.c_str());
    if (m_OpenedDir == NULL) return false;
    return FindNext();
#endif
    return false;
}

//-------------------------------------------------------------
// Post    : Return TRUE if a new match found
// Task    : Find the next file that meets the conditions specified
//           in the last FindFirst call
//-------------------------------------------------------------
bool CPath::FindNext()
{
#ifdef _WIN32
    if (m_hFindFile == NULL)
    {
        return false;
    }

    WIN32_FIND_DATA	FindData;
    while (FindNextFile(m_hFindFile, &FindData) != false)
    { // while(FindNext(...))
        if (AttributesMatch(m_dwFindFileAttributes, FindData.dwFileAttributes))
        { // if(AttributesMatch(...)
            if ((_A_SUBDIR & FindData.dwFileAttributes) == _A_SUBDIR)
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
            if ((_A_SUBDIR & FindData.dwFileAttributes) == _A_SUBDIR)
            {
                EnsureTrailingBackslash(m_strPath);
            }
            return TRUE;
        }
    }
#else
    dirent* pEntry;
    while ((pEntry = readdir((DIR*)m_OpenedDir)))
    {
        uint32_t dwFileAttributes = pEntry->d_type == DT_DIR ? FIND_ATTRIBUTE_SUBDIR : FIND_ATTRIBUTE_FILES;

        WriteTrace(TracePath, TraceVerbose, "m_dwFindFileAttributes = %X dwFileAttributes = %X AttributesMatch: %s",m_dwFindFileAttributes, dwFileAttributes, AttributesMatch(m_dwFindFileAttributes, dwFileAttributes) ? "true" : "false");

        // ii.) Compare candidate to attributes, and filter out the "." and ".." folders
        if (!AttributesMatch(m_dwFindFileAttributes, dwFileAttributes) ||
            strcmp(pEntry->d_name,".") == 0 ||
            strcmp(pEntry->d_name,"..") == 0 ||
            !wildcmp(m_FindWildcard.c_str(),pEntry->d_name))
        {
            WriteTrace(TracePath, TraceVerbose, "continue, d_name = %s",pEntry->d_name);
            continue;
        }
        if ((FIND_ATTRIBUTE_SUBDIR & dwFileAttributes) == FIND_ATTRIBUTE_SUBDIR)
        {
            WriteTrace(TracePath, TraceVerbose, "is dir");
            if (IsDirectory())
            {
                // Found a directory
                UpDirectory();
            }
            else
            {
                SetNameExtension("");
            }
            AppendDirectory(pEntry->d_name);
        }
        else
        {
            WriteTrace(TracePath, TraceVerbose, "is file");
            // Found a file
            if (IsDirectory())
            {
                // Found a directory
                UpDirectory();
            }
            SetNameExtension(pEntry->d_name);
            WriteTrace(TracePath, TraceVerbose, "m_strPath: %s pEntry->d_name: %s", m_strPath.c_str(), pEntry->d_name);
        }
        if ((FIND_ATTRIBUTE_SUBDIR & dwFileAttributes) == FIND_ATTRIBUTE_SUBDIR)
        {
            EnsureTrailingBackslash(m_strPath);
        }
        return true;
    }
#endif
    return false;
}

//-------------------------------------------------------------
// Post    : Return TRUE on success
// Task    : Change current working directory of application 2 path
//-------------------------------------------------------------
bool CPath::ChangeDirectory()
{
#ifdef _WIN32
    std::string DriveDirectory;
    GetDriveDirectory(DriveDirectory);

    return SetCurrentDirectory(DriveDirectory.c_str()) != 0;
#else
    std::string Dir;
    GetDirectory(Dir);

    return chdir(Dir.c_str()) == 0;
#endif
}

void CPath::NormalizePath(CPath BaseDir)
{
#ifdef _WIN32
	stdstr Directory = BaseDir.GetDriveDirectory();
#else
	stdstr Directory = BaseDir.GetDirectory();
#endif
	bool Changed = false;
	if (IsRelative())
	{
		EnsureTrailingBackslash(Directory);
		Directory += GetDirectory();
		Changed = true;
	}
	strvector Parts = Directory.Tokenize(DIRECTORY_DELIMITER);
	strvector NormalizesParts;
	for (strvector::const_iterator itr = Parts.begin(); itr != Parts.end(); itr++)
	{
		if (*itr == ".")
		{
			Changed = true;
		}
		else if (*itr == "..")
		{
			NormalizesParts.pop_back();
			Changed = true;
		}
		else
		{
			NormalizesParts.push_back(*itr);
		}
	}
	if (Changed)
	{
		Directory.clear();
		for (strvector::const_iterator itr = NormalizesParts.begin(); itr != NormalizesParts.end(); itr++)
		{
			Directory += *itr + DIRECTORY_DELIMITER;
		}
#ifdef _WIN32
		SetDriveDirectory(Directory.c_str());
#else
		SetDirectory(Directory.c_str());
#endif
	}
}

//-------------------------------------------------------------
// Pre     : If bCreateIntermediates is TRUE, create all eventually
//           missing parent directories too
// Post    : Return TRUE on success
// Task    : Create new directory
//-------------------------------------------------------------
bool CPath::DirectoryCreate(bool bCreateIntermediates /*= TRUE*/)
{
    WriteTrace(TracePath, TraceDebug, "m_strPath = %s bCreateIntermediates = %s",m_strPath.c_str(),bCreateIntermediates ? "true" : "false");
    std::string	PathText;
    bool	bSuccess;

    if (DirectoryExists())
    {
        WriteTrace(TracePath, TraceDebug, "Directory already exists, res = true");
        return true;
    }

#ifdef _WIN32
    GetDriveDirectory(PathText);
    StripTrailingBackslash(PathText);
    WriteTrace(TracePath, TraceDebug, "Create %s",PathText.c_str());
    bSuccess = ::CreateDirectory(PathText.c_str(), NULL) != 0;
#else
    GetDirectory(PathText);
    StripTrailingBackslash(PathText);
    WriteTrace(TracePath, TraceDebug, "Create %s",PathText.c_str());
    bSuccess = mkdir(PathText.c_str(), S_IRWXU) == 0;
    if (!bSuccess)
    {
        WriteTrace(TracePath, TraceWarning, "failed to create \"%s\" errno: %d",PathText.c_str(), errno);
    }
#endif
    if (!bSuccess && bCreateIntermediates)
    {
        WriteTrace(TracePath, TraceDebug, "failed creating intermediates");
        std::string::size_type nDelimiter = PathText.rfind(DIRECTORY_DELIMITER);
        if (nDelimiter == std::string::npos)
        {
            return false;
        }

        PathText.resize(nDelimiter + 1);
        CPath SubPath(PathText);

        return SubPath.DirectoryCreate() ? DirectoryCreate(false) : false;
    }
    WriteTrace(TracePath, TraceDebug, "res = %s",bSuccess ? "true" : "false");
    return bSuccess;
}

//Helpers

//------------------------------------------------------------------------
// Task    : Remove first character (if any) if it's chLeading
//------------------------------------------------------------------------
void CPath::cleanPathString(std::string& rDirectory) const
{
    std::string::size_type pos = rDirectory.find(DIRECTORY_DELIMITER2);
    while (pos != std::string::npos)
    {
        rDirectory.replace(pos, 1, &DIRECTORY_DELIMITER);
        pos = rDirectory.find(DIRECTORY_DELIMITER2, pos + 1);
    }

    bool AppendEnd = !_strnicmp(rDirectory.c_str(), DIR_DOUBLEDELIM, 2);
    pos = rDirectory.find(DIR_DOUBLEDELIM);
    while (pos != std::string::npos)
    {
        rDirectory.replace(pos, 2, &DIRECTORY_DELIMITER);
        pos = rDirectory.find(DIR_DOUBLEDELIM, pos + 1);
    }
    if (AppendEnd)
    {
        rDirectory.insert(0, stdstr_f("%c", DIRECTORY_DELIMITER).c_str());
    }
}

void CPath::StripLeadingChar(std::string& rText, char chLeading) const
{
    std::string::size_type nLength = rText.length();
    if (nLength == 0)
        return;

    if (rText[0] == chLeading)
        rText = rText.substr(1);
}

//------------------------------------------------------------------------
// Task    : Remove first character if \
//------------------------------------------------------------------------
void CPath::StripLeadingBackslash(std::string& Directory) const
{
    std::string::size_type nLength = Directory.length();

    // If Directory is of the form '\', don't do it
    if (nLength <= 1)
        return;

    if (Directory[0] == DIRECTORY_DELIMITER)
        Directory = Directory.substr(1);
}

//------------------------------------------------------------------------
// Task    : Remove last character (if any) if it's chTrailing
//------------------------------------------------------------------------
void CPath::StripTrailingChar(std::string& rText, char chTrailing) const
{
    std::string::size_type nLength = rText.length();
    if (nLength == 0)
        return;

    if (rText[nLength - 1] == chTrailing)
        rText.resize(nLength - 1);
}

//------------------------------------------------------------------------
// Task    : Remove last character if \
//------------------------------------------------------------------------
void CPath::StripTrailingBackslash(std::string& rDirectory) const
{
    for (;;)
    {
        std::string::size_type nLength = rDirectory.length();
        if (nLength <= 1)
        {
            return;
        }

        if (rDirectory[nLength - 1] == DIRECTORY_DELIMITER || rDirectory[nLength - 1] == DIRECTORY_DELIMITER2)
        {
            rDirectory.resize(nLength - 1);
            continue;
        }
        return;
    }
}

//------------------------------------------------------------------------
// Task    : Add a backslash to the end of Directory if there is
//           not already one there
//------------------------------------------------------------------------
void CPath::EnsureTrailingBackslash(std::string& Directory) const
{
    std::string::size_type nLength = Directory.length();

    if (Directory.empty() || (Directory[nLength - 1] != DIRECTORY_DELIMITER))
    {
        Directory += DIRECTORY_DELIMITER;
    }
}

//------------------------------------------------------------------------
// Task    : Add a backslash to the beginning of Directory if there
//           is not already one there
//------------------------------------------------------------------------
void CPath::EnsureLeadingBackslash(std::string & Directory) const
{
    if (Directory.empty() || (Directory[0] != DIRECTORY_DELIMITER))
    {
        Directory = stdstr_f("%c%s", DIRECTORY_DELIMITER, Directory.c_str());
    }
}

#ifndef _WIN32
bool CPath::wildcmp(const char *wild, const char *string)
{
    const char *cp = NULL, *mp = NULL;

    while ((*string) && (*wild != '*'))
    {
        if ((*wild != *string) && (*wild != '?'))
        {
            return 0;
        }
        wild++;
        string++;
    }

    while (*string)
    {
        if (*wild == '*')
        {
            if (!*++wild)
            {
                return 1;
            }
            mp = wild;
            cp = string+1;
        }
        else if ((*wild == *string) || (*wild == '?'))
        {
            wild++;
            string++;
        }
        else
        {
            wild = mp;
            string = cp++;
        }
    }

    while (*wild == '*')
    {
        wild++;
    }
    return !*wild;
}
#endif
