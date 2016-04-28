// Path.cpp: implementation of the CPath class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#ifdef _WIN32
# include <Shlobj.h>
# include <dos.h>
#else
# include <cstdio>
# include <dirent.h>
# include <dlfcn.h>
# include <fcntl.h>
# include <fnmatch.h>
# include <libgen.h>
# include <limits.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# ifndef MAX_PATH
#  define MAX_PATH PATH_MAX
# endif
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
// Helper Struct for FindNext method
struct FindFileHandle
{
    DIR* pDir = NULL;
    dirent* pDirEntry = NULL;
    std::string searchPattern;
};
#define FINDFILE_HANDLE(handle) static_cast<FindFileHandle*>(handle)

const char * DRIVE_DELIMITER = "";
const char DIRECTORY_DELIMITER = '/';
#endif
const char EXTENSION_DELIMITER = '.';
void * CPath::m_hInst = NULL;

//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

void CPath::SethInst(void * hInst)
{
    m_hInst = hInst;
}

void * CPath::GethInst()
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
    m_dwFindFileAttributes = 0;
    m_hFindFile = NULL;
}

//-------------------------------------------------------------
// Task    : Helper function for the various CPath destructors.
//           Cleans up varios internals
//-------------------------------------------------------------
inline void CPath::Exit()
{
    if (m_hFindFile != NULL)
    {
#ifdef _WIN32
        FindClose(m_hFindFile);
#else
        closedir(FINDFILE_HANDLE(m_hFindFile)->pDir);
        delete FINDFILE_HANDLE(m_hFindFile);
#endif
        m_hFindFile = NULL;
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
    Init();
    SetDriveDirectory(lpszPath);
    SetNameExtension(NameExten);
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
    SetDriveDirectory(strPath.c_str());
    SetNameExtension(NameExten);
}

//-------------------------------------------------------------
// Task    : Constructs a path and points it 2 strPath
//-------------------------------------------------------------
CPath::CPath(const std::string& strPath, const std::string& NameExten)
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
void CPath::GetComponents(std::string* pDrive, std::string* pDirectory, std::string* pName, std::string* pExtension) const
{
#ifdef _WIN32
    char buff_drive[_MAX_DRIVE + 1];
    char buff_dir[_MAX_DIR + 1];
    char buff_name[_MAX_FNAME + 1];
    char buff_ext[_MAX_EXT + 1];

    ZeroMemory(buff_drive, sizeof(buff_drive));
    ZeroMemory(buff_dir, sizeof(buff_dir));
    ZeroMemory(buff_name, sizeof(buff_name));
    ZeroMemory(buff_ext, sizeof(buff_ext));

    _splitpath(m_strPath.c_str(), pDrive ? buff_drive : NULL, pDirectory ? buff_dir : NULL, pName ? buff_name : NULL, pExtension ? buff_ext : NULL);

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

    // DOS's _splitpath returns "d:", we return "d"
    if (pDrive)
    {
        StripTrailingChar(*pDrive, DRIVE_DELIMITER);
    }

    // DOS's _splitpath returns "\dir\subdir\", we return "\dir\subdir"
    if (pDirectory)
    {
        StripTrailingBackslash(*pDirectory);
    }

    // DOS's _splitpath returns ".ext", we return "ext"
    if (pExtension)
    {
        StripLeadingChar(*pExtension, EXTENSION_DELIMITER);
    }
#else
    /* Find Path Components */
    size_t posRoot = m_strPath.find_first_of(DIRECTORY_DELIMITER);
    size_t posFile = m_strPath.find_last_of(DIRECTORY_DELIMITER);
    size_t posExt  = m_strPath.find_last_of(EXTENSION_DELIMITER);
    
    if(posFile == std::string::npos)
        posFile = 0;
    
    if(!m_strPath.empty() && m_strPath.at(posFile) == DIRECTORY_DELIMITER)
        posFile++;
        
    if(posExt == std::string::npos || posExt == 0 || posExt == 1 || m_strPath.at(posExt -1) == DIRECTORY_DELIMITER)
        posExt = m_strPath.size();
    
    /* Extract Path Components */
    if (pDrive)
        *pDrive     = ((posRoot == 0) ? std::string(1, DIRECTORY_DELIMITER) : std::string());
    if (pDirectory)
        *pDirectory = m_strPath.substr(((posRoot == 0) ? 1 : 0), posFile - ((posRoot == 0) ? 1 : 0));
    if (pName)
        *pName      = m_strPath.substr(posFile, posExt - posFile);
    if (pExtension)
        *pExtension = m_strPath.substr((posExt == m_strPath.size()) ? posExt : posExt + 1);
#endif
}

//-------------------------------------------------------------
// Task    : Get drive and directory from path
//-------------------------------------------------------------
void CPath::GetDriveDirectory(std::string& rDriveDirectory) const
{
    std::string Drive;
    std::string Directory;

    GetComponents(&Drive, &Directory);
    rDriveDirectory = Drive;
    if (!Drive.empty())
    {
        rDriveDirectory += DRIVE_DELIMITER;
        rDriveDirectory += Directory;
    }
}

std::string CPath::GetDriveDirectory(void) const
{
    std::string rDriveDirectory;
    GetDriveDirectory(rDriveDirectory);
    return rDriveDirectory;
}
//-------------------------------------------------------------
// Task    : Get directory from path
//-------------------------------------------------------------
void CPath::GetDirectory(std::string& rDirectory) const
{
    GetComponents(NULL, &rDirectory);
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

    GetComponents(NULL, NULL, &Name, &Extension);
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
    GetComponents(NULL, NULL, &rName);
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
    GetComponents(NULL, NULL, NULL, &rExtension);
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
    char buff_fullname[MAX_PATH];
    memset(buff_fullname, 0, sizeof(buff_fullname));
    
#ifdef _WIN32
    _fullpath(buff_fullname, m_strPath.c_str(), MAX_PATH - 1);
#else
    realpath(m_strPath.c_str(), buff_fullname);
#endif
    rFullyQualified = buff_fullname;
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
    if (m_strPath.length() > 1 && m_strPath[0] == DIRECTORY_DELIMITER)
    {
        return false;
    }
    return true;
}

//-------------------------------------------------------------
// Task    : Set path components
//-------------------------------------------------------------
void CPath::SetComponents(const char * lpszDrive, const char * lpszDirectory, const char * lpszName, const char * lpszExtension)
{
#ifdef _WIN32
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
#else
    m_strPath.erase();
    if (lpszDrive != NULL && strlen(lpszDrive) != 0)
    {
        if(lpszDirectory != NULL && lpszDirectory[0] !=  '/')
            m_strPath += (*lpszDrive == '/' ? "/" : "");
    }
    
    if (lpszDirectory != NULL && strlen(lpszDirectory) != 0)
    {
        m_strPath += lpszDirectory;
        if(m_strPath.back() != DIRECTORY_DELIMITER)
            m_strPath += DIRECTORY_DELIMITER;
    }
    
    if (lpszName != NULL && strlen(lpszName) != 0)
    {
        m_strPath += lpszName;
    }
    
    if ((lpszName != NULL && strlen(lpszName) != 0) && (lpszExtension != NULL && strlen(lpszExtension) != 0))
    {
        if(lpszExtension[0] != EXTENSION_DELIMITER)
            m_strPath += EXTENSION_DELIMITER;
        m_strPath += lpszExtension;
    }
#endif
}

//-------------------------------------------------------------
// Task    : Set path's drive
//-------------------------------------------------------------
void CPath::SetDrive(char chDrive)
{
    stdstr_f Drive("%c", chDrive);
    std::string	 Directory;
    std::string	 Name;
    std::string	 Extension;

    GetComponents(NULL, &Directory, &Name, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), Extension.c_str());
}

//-------------------------------------------------------------
// Task    : Set path's directory
//-------------------------------------------------------------
void CPath::SetDirectory(const char * lpszDirectory, bool bEnsureAbsolute /*= false*/)
{
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

    std::string	Drive;
    GetComponents(&Drive, NULL, &Name, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), Extension.c_str());
}

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

//-------------------------------------------------------------
// Task    : Set path's filename
//-------------------------------------------------------------
void CPath::SetName(const char * lpszName)
{
    std::string	Directory;
    std::string	Extension;

    std::string	Drive;
    GetComponents(&Drive, &Directory, NULL, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), lpszName, Extension.c_str());
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

    std::string	Drive;
    GetComponents(&Drive, &Directory, NULL, &Extension);
    SetComponents(Drive.c_str(), Directory.c_str(), sName, Extension.c_str());
}

//-------------------------------------------------------------
// Task    : Set path's file extension
//-------------------------------------------------------------
void CPath::SetExtension(const char * lpszExtension)
{
    std::string	Directory;
    std::string	Name;

    std::string	Drive;
    GetComponents(&Drive, &Directory, &Name);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), lpszExtension);
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

    std::string	Drive;
    GetComponents(&Drive, &Directory, &Name);
    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), sExtension);
}

//-------------------------------------------------------------
// Task    : Set path's filename and extension
//-------------------------------------------------------------
void CPath::SetNameExtension(const char * lpszNameExtension)
{
    std::string	Directory;

    std::string	Drive;
    GetComponents(&Drive, &Directory);
    SetComponents(Drive.c_str(), Directory.c_str(), lpszNameExtension, NULL);
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

    std::string	Drive;
    GetComponents(&Drive, &Directory, &Name, &Extension);
    EnsureTrailingBackslash(Directory);
    Directory += SubDirectory;

    SetComponents(Drive.c_str(), Directory.c_str(), Name.c_str(), Extension.c_str());
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
    char buff_path[MAX_PATH];

    memset(buff_path, 0, sizeof(buff_path));

#ifdef _WIN32
    ::GetCurrentDirectory(MAX_PATH, buff_path);
#else
    getcwd(buff_path, MAX_PATH);
#endif

    Empty();
    SetDriveDirectory(buff_path);
}

//-------------------------------------------------------------
// Task    : Set path 2 the name of specified module
//-------------------------------------------------------------
void CPath::Module(void * hInstance)
{
#ifdef _WIN32
    char buff_path[MAX_PATH];
    
    memset(buff_path, 0, sizeof(buff_path));
    GetModuleFileName((HINSTANCE)hInstance, buff_path, MAX_PATH);
    m_strPath = buff_path;
#else
    Dl_info dlInfo;
    if(dladdr(hInstance, &dlInfo) == 0)
        return;
    m_strPath = dlInfo.dli_fname;
#endif

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

//---------------------------------------------------------------------------
// Post    : Return TRUE if a directory
// Task    : Check if this path represents a directory
//---------------------------------------------------------------------------
bool CPath::IsDirectory() const
{
#ifndef _WIN32
    struct stat statbuf;
    if(stat(m_strPath.c_str(), &statbuf) == -1)
        goto LABEL_CheckPath;
    return S_ISDIR(statbuf.st_mode);
    
LABEL_CheckPath:
#endif
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
    // Create test path
    CPath TestPath(m_strPath.c_str());

    std::string DirName;
    TestPath.UpDirectory(&DirName);
    TestPath.SetNameExtension(DirName.c_str());

#ifdef _WIN32
    WIN32_FIND_DATA	FindData;
    HANDLE          hFindFile = FindFirstFile((const char *)TestPath, &FindData); // Find anything
    bool            bGotDirectory = (hFindFile != INVALID_HANDLE_VALUE) && (FindData.dwFileAttributes && FILE_ATTRIBUTE_DIRECTORY != 0);

    if (hFindFile != NULL)	// Make sure we close the search
    {
        FindClose(hFindFile);
    }
#else
    DIR* dir = opendir((const char *)TestPath);
    bool bGotDirectory = (dir != NULL);
    if(bGotDirectory)
    {
        closedir(dir);
    }    
#endif

    return bGotDirectory;
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
    return access(m_strPath.c_str(), F_OK) != -1;
#endif
}

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
        // File does not exists
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
    if (access(m_strPath.c_str(), W_OK) == -1 && !bEvenIfReadOnly)
    {
        return false;
    }
    
    return (unlink(m_strPath.c_str()) == 0);
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

#ifdef _WIN32
    // CopyFile will set the target's attributes 2 the same as
    // the source after copying
    return CopyFile(m_strPath.c_str(), lpcszTargetFile, !bOverwrite) != 0;
#else
    // Get source file permission mode
    struct stat sourceStat;
    if(stat(m_strPath.c_str(), &sourceStat) != 0)
        return false;

    int source = open(m_strPath.c_str(), O_RDONLY, 0);
    if( source == -1)
    {
        return false;
    }
    
    int dest = open(lpcszTargetFile, O_WRONLY | O_CREAT | O_TRUNC, sourceStat.st_mode);
    if(dest == -1)
    {
        close(source);
        return false;
    }
    
    // Copy file
    char buffer[BUFSIZ];
    size_t size;
    
    while ((size = read(source, buffer, BUFSIZ)) > 0) {
        write(dest, buffer, size);
    }
    
    close(source);
    close(dest);
    return true;
#endif
}

//-------------------------------------------------------------
// Post    : Return TRUE on success, false if there is such a target file
//           and we weren't granted permission 2 overwrite file or some error
// Task    : Move file
//-------------------------------------------------------------
bool CPath::MoveTo(const char * lpcszTargetFile, bool bOverwrite)
{
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
    
#ifdef _WIN32
    return MoveFile(m_strPath.c_str(), lpcszTargetFile) != 0;
#else
    return rename(m_strPath.c_str(), lpcszTargetFile) == 0;
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

#ifndef _WIN32
bool FindNextFile(FindFileHandle* hFindFile)
{
    while((hFindFile->pDirEntry = readdir(hFindFile->pDir)) != NULL)
    {
        /* File name matches search pattern ? */
        if (fnmatch(hFindFile->searchPattern.c_str(), hFindFile->pDirEntry->d_name, 0) == 0)
            return true;
    }
    return false;
}
#endif

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
//
//           UNIX: On Unix like Systems use attributes:
//                 FIND_ATTRIBUTE_SUBDIR, FIND_ATTRIBUTE_FILES,
//                 FIND_ATTRIBUTE_ALLFILES and unix file types eg.: DT_DIR, DT_BLK etc..
//
//                 The struct 'dirent' returned from readdir may not be fully featured on some systems.
//                 The POSIX.1-2001 standard only specifies fields d_name and d_ino.
//                 On this systems call to the function 'stat' should be used to determine file type.
//-------------------------------------------------------------
bool CPath::FindFirst(uint32_t dwAttributes /*= _A_NORMAL*/)
{
    m_dwFindFileAttributes = dwAttributes;
    bool bGotFile;
    bool bWantSubdirectory = (bool)(FIND_ATTRIBUTE_SUBDIR & dwAttributes);

    // Close handle to any previous enumeration
    Exit();

#ifdef _WIN32
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
        if ((_A_SUBDIR & FindData.dwFileAttributes) != 0)
            StripTrailingBackslash(m_strPath);

        SetNameExtension(FindData.cFileName);

        if ((_A_SUBDIR & FindData.dwFileAttributes) != 0)
            EnsureTrailingBackslash(m_strPath);
        return true;

        // iv.) Not found match, get another
    LABEL_GetAnother:
        bGotFile = FindNextFile(m_hFindFile, &FindData);
    }
#else // UNIX
    m_hFindFile = static_cast<void*>(new FindFileHandle);
    if(m_hFindFile == NULL)
        return false;
    
    // Set file search pattern and open directory
    FINDFILE_HANDLE(m_hFindFile)->searchPattern = GetNameExtension();
    FINDFILE_HANDLE(m_hFindFile)->pDir = opendir(GetDriveDirectory().c_str());
    bGotFile = FINDFILE_HANDLE(m_hFindFile)->pDir != NULL;

    // i.) Finding first candidate file
    bGotFile = bGotFile && FindNextFile(FINDFILE_HANDLE(m_hFindFile));
    while (bGotFile)
    {
        // ii.) Compare candidate to attributes, and filter out the "." and ".." folders
        if (!AttributesMatch(m_dwFindFileAttributes, FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_type)) // Note: d_type might not be set on some unix system
            goto LABEL_GetAnother;
        if (bWantSubdirectory && (FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_name[0] == '.'))
            goto LABEL_GetAnother;
        
        // iii.) Found match, prepare result
        if ((DT_DIR & FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_type) != 0)
            StripTrailingBackslash(m_strPath);
        
        SetNameExtension(FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_name);
        
        if ((DT_DIR & FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_type) != 0)
            EnsureTrailingBackslash(m_strPath);
        return true;
        
        // iv.) Not found match, get another
    LABEL_GetAnother:
        bGotFile = FindNextFile(FINDFILE_HANDLE(m_hFindFile));
    }
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
    if (m_hFindFile == NULL)
    {
        return false;
    }

#ifdef _WIN32
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
                EnsureTrailingBackslash(m_strPath);
            return TRUE;
        }
    }
#else // Unix
    while (FindNextFile(FINDFILE_HANDLE(m_hFindFile)))
    { // while(FindNext(...))
        if (AttributesMatch(m_dwFindFileAttributes, FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_type))
        { // if(AttributesMatch(...)
            if ((DT_DIR & FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_type) == DT_DIR)
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
                AppendDirectory(FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_name);
            }
            else
            {
                // Found a file
                if (IsDirectory())
                {
                    // Found a directory
                    UpDirectory();
                }
                SetNameExtension(FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_name);
            }
            if ((DT_DIR & FINDFILE_HANDLE(m_hFindFile)->pDirEntry->d_type) == DT_DIR)
                EnsureTrailingBackslash(m_strPath);
            return true;
        }
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
    std::string DriveDirectory;
    GetDriveDirectory(DriveDirectory);

#ifdef _WIN32
    return SetCurrentDirectory(DriveDirectory.c_str()) != 0;
#else
    return chdir(DriveDirectory.c_str()) == 0;
#endif
}

//-------------------------------------------------------------
// Pre     : If bCreateIntermediates is TRUE, create all eventually
//           missing parent directories too
// Post    : Return TRUE on success
// Task    : Create new directory
//-------------------------------------------------------------
bool CPath::DirectoryCreate(bool bCreateIntermediates /*= TRUE*/)
{
    std::string	PathText;
    bool	bSuccess;

    GetDriveDirectory(PathText);
    StripTrailingBackslash(PathText);
    
#ifdef _WIN32
    bSuccess = ::CreateDirectory(PathText.c_str(), NULL) != 0;
#else
    bSuccess = mkdir(PathText.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
    
    if (!bSuccess)
    {
        CPath CurrentDir(CPath::CURRENT_DIRECTORY);
        bSuccess = ChangeDirectory() != 0;
        CurrentDir.ChangeDirectory();
    }

    if (!bSuccess && bCreateIntermediates)
    {
        std::string::size_type nDelimiter = PathText.rfind(DIRECTORY_DELIMITER);
        if (nDelimiter == std::string::npos)
        {
            return false;
        }

        PathText.resize(nDelimiter + 1);
        CPath SubPath(PathText);

        return SubPath.DirectoryCreate() ? DirectoryCreate(false) : false;
    }
    return bSuccess;
}

//Helpers

//------------------------------------------------------------------------
// Task    : Remove first character (if any) if it's chLeading
//------------------------------------------------------------------------
void CPath::cleanPathString(std::string& rDirectory) const
{
#ifdef _WIN32
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
#endif
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
// Task    : Remove first character if \ (Unix /)
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
// Task    : Remove last character if \ (Unix /)
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

        if (rDirectory[nLength - 1] == DIRECTORY_DELIMITER
        #ifdef _WIN32
            || rDirectory[nLength - 1] == DIRECTORY_DELIMITER2
        #endif
            )
        {
            rDirectory.resize(nLength - 1);
            continue;
        }
        return;
    }
}

//------------------------------------------------------------------------
// Task    : Add a directory delimiter to the end of Directory if there is
//           not already one there
//------------------------------------------------------------------------
void CPath::EnsureTrailingBackslash(std::string& Directory) const
{
    std::string::size_type nLength = Directory.length();
    bool appendDirectoryDelimiter = Directory.empty() || (Directory[nLength - 1] != DIRECTORY_DELIMITER);
    
#ifndef _WIN32 // On Unix do not set leading forward slash
    appendDirectoryDelimiter = !Directory.empty() && appendDirectoryDelimiter;
#endif

    if (appendDirectoryDelimiter)
    {
        Directory += DIRECTORY_DELIMITER;
    }
}

//------------------------------------------------------------------------
// Task    : Add a directory delimiter to the beginning of Directory if there
//           is not already one there
//------------------------------------------------------------------------
void CPath::EnsureLeadingBackslash(std::string & Directory) const
{
    if (Directory.empty() || (Directory[0] != DIRECTORY_DELIMITER))
    {
        Directory = stdstr_f("%c%s", DIRECTORY_DELIMITER, Directory.c_str());
    }
}