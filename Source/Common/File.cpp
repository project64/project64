#include "File.h"
#include "path.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define USE_WINDOWS_API
#include <Windows.h>
#else
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#include <crtdbg.h>
#else
#define _ASSERTE(expr)          ((void)0)
#endif

CFile::CFile() :
#ifdef USE_WINDOWS_API
m_hFile(INVALID_HANDLE_VALUE),
#else
m_hFile(nullptr),
#endif
m_bCloseOnDelete(false)
{
}

CFile::CFile(void * hFile) :
m_hFile(hFile),
m_bCloseOnDelete(true)
{
    if (hFile == 0)
    {
        _ASSERTE(hFile != 0);
    }
}

CFile::CFile(const char * lpszFileName, uint32_t nOpenFlags) :
#ifdef USE_WINDOWS_API
m_hFile(INVALID_HANDLE_VALUE),
#else
m_hFile(nullptr),
#endif
m_bCloseOnDelete(true)
{
    Open(lpszFileName, nOpenFlags);
}

CFile::~CFile()
{
#ifdef USE_WINDOWS_API
    if (m_hFile != INVALID_HANDLE_VALUE && m_bCloseOnDelete)
#else
    if (m_hFile != nullptr && m_bCloseOnDelete)
#endif
    {
        Close();
    }
}

bool CFile::Open(const char * lpszFileName, uint32_t nOpenFlags)
{
    if (!Close())
    {
        return false;
    }

    if (lpszFileName == nullptr || lpszFileName[0] == '\0')
    {
        return false;
    }

    m_bCloseOnDelete = true;
#ifdef USE_WINDOWS_API
    m_hFile = INVALID_HANDLE_VALUE;

    ULONG dwAccess = 0;
    switch (nOpenFlags & 3)
    {
    case modeRead:
        dwAccess = GENERIC_READ;
        break;
    case modeWrite:
        dwAccess = GENERIC_WRITE;
        break;
    case modeReadWrite:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        break;
    default:
        _ASSERTE(false);
    }

    // Map share mode
    ULONG dwShareMode = 0;

    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    if ((nOpenFlags & shareDenyWrite) == shareDenyWrite) { dwShareMode &= ~FILE_SHARE_WRITE; }
    if ((nOpenFlags & shareDenyRead) == shareDenyRead)   { dwShareMode &= ~FILE_SHARE_READ; }
    if ((nOpenFlags & shareExclusive) == shareExclusive) { dwShareMode = 0; }

    // Map modeNoInherit flag
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = (nOpenFlags & modeNoInherit) == 0;

    // Map creation flags
    ULONG dwCreateFlag = OPEN_EXISTING;
    if (nOpenFlags & modeCreate)
    {
        dwCreateFlag = ((nOpenFlags & modeNoTruncate) != 0) ? OPEN_ALWAYS : CREATE_ALWAYS;
    }

    // Attempt file creation
    HANDLE hFile = ::CreateFileA(lpszFileName, dwAccess, dwShareMode, &sa, dwCreateFlag, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    { //#define ERROR_PATH_NOT_FOUND             3L
        //ULONG err = GetLastError();
        return false;
    }
    m_hFile = hFile;
#else
    if ((nOpenFlags & modeNoTruncate) == 0 && (nOpenFlags & CFileBase::modeCreate) == CFileBase::modeCreate)
    {
        CPath(lpszFileName).Delete();
    }
    if ((nOpenFlags & CFileBase::modeCreate) != CFileBase::modeCreate)
    {
        if (!CPath(lpszFileName).Exists())
        {
            return false;
        }
    }

    if ((nOpenFlags & CFileBase::modeCreate) == CFileBase::modeCreate)
    {
        CPath file(lpszFileName);
        if (!file.Exists())
        {
            FILE * fp = fopen(lpszFileName,"wb");
            if (fp)
            {
                fclose(fp);
            }
            if (!file.Exists())
            {
                return false;
            }
        }
    }

    if ((nOpenFlags & CFileBase::modeWrite) == CFileBase::modeWrite ||
        (nOpenFlags & CFileBase::modeReadWrite) == CFileBase::modeReadWrite)
    {
        m_hFile = fopen(lpszFileName, "rb+");
        if (m_hFile != nullptr)
        {
            SeekToBegin();
        }
    }
    else if ((nOpenFlags & CFileBase::modeRead) == CFileBase::modeRead)
    {
        m_hFile = fopen(lpszFileName, "rb");
        if (m_hFile != nullptr)
        {
            SeekToBegin();
        }
    }
    else
    {
        return false;
    }
#endif
    m_bCloseOnDelete = true;
    return true;
}

bool CFile::Close()
{
    bool bError = true;
#ifdef USE_WINDOWS_API
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        bError = !::CloseHandle(m_hFile);
    }
    m_hFile = INVALID_HANDLE_VALUE;
#else
    if (m_hFile != nullptr)
    {
        fclose((FILE *)m_hFile);
        m_hFile = nullptr;
    }
#endif
    m_bCloseOnDelete = false;
    return bError;
}

uint32_t CFile::SeekToEnd(void)
{
    return Seek(0, CFile::end);
}

void CFile::SeekToBegin(void)
{
    Seek(0, CFile::begin);
}

bool CFile::IsOpen(void) const
{
#ifdef USE_WINDOWS_API
    return m_hFile != INVALID_HANDLE_VALUE;
#else
    return m_hFile != nullptr;
#endif
}

bool CFile::Flush()
{
#ifdef USE_WINDOWS_API
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        return true;
    }

    return ::FlushFileBuffers(m_hFile) != 0;
#else
    fflush((FILE *)m_hFile);
    fsync(fileno((FILE *)m_hFile));
    return true;
#endif
}

bool CFile::Write(const void* lpBuf, uint32_t nCount)
{
    if (nCount == 0)
    {
        return true;     // Avoid Win32 "null-write" option
    }

#ifdef USE_WINDOWS_API
    ULONG nWritten = 0;
    if (!::WriteFile(m_hFile, lpBuf, nCount, &nWritten, nullptr))
    {
        return false;
    }

    if (nWritten != nCount)
    {
        // Win32s will not return an error all the time (usually DISK_FULL)
        return false;
    }
#else
    if (fwrite(lpBuf, 1, nCount, (FILE *)m_hFile) != nCount)
    {
        return false;
    }
#endif
    return true;
}

uint32_t CFile::Read(void* lpBuf, uint32_t nCount)
{
    if (nCount == 0)
    {
        return 0;   // Avoid Win32 "null-read"
    }

#ifdef USE_WINDOWS_API
    DWORD dwRead = 0;
    if (!::ReadFile(m_hFile, lpBuf, nCount, &dwRead, nullptr))
    {
        return 0;
    }
    return (uint32_t)dwRead;
#else
    uint32_t res = fread(lpBuf, sizeof(uint8_t), nCount, (FILE *)m_hFile);
    return res;
#endif
}

int32_t CFile::Seek(int32_t lOff, SeekPosition nFrom)
{
#ifdef USE_WINDOWS_API
    ULONG dwNew = ::SetFilePointer(m_hFile, lOff, nullptr, (ULONG)nFrom);
    if (dwNew == (ULONG)-1)
    {
        return -1;
    }
    return dwNew;
#else
    if (m_hFile == nullptr)
    {
        return -1;
    }
    int origin;

    switch (nFrom)
    {
    case begin: origin = SEEK_SET; break;
    case current: origin = SEEK_CUR; break;
    case end: origin = SEEK_END; break;
    default:
        return -1;
    }

    Flush();
    int res = fseek((FILE *)m_hFile, lOff, origin);
    return res;
#endif
}

uint32_t CFile::GetPosition() const
{
#ifdef USE_WINDOWS_API
    return ::SetFilePointer(m_hFile, 0, nullptr, FILE_CURRENT);
#else
    return (uint32_t)ftell((FILE *)m_hFile);
#endif
}

bool CFile::SetLength(uint32_t dwNewLen)
{
    Seek((int32_t)dwNewLen, begin);
    return SetEndOfFile();
}

uint32_t CFile::GetLength() const
{
#ifdef USE_WINDOWS_API
    return GetFileSize(m_hFile, 0);
#else
    uint32_t pos = GetPosition();
    fseek((FILE *)m_hFile, 0, SEEK_END);
    uint32_t FileSize = GetPosition();
    fseek((FILE *)m_hFile, (int32_t)pos, SEEK_SET);
    return FileSize;
#endif
}

bool CFile::SetEndOfFile()
{
#ifdef USE_WINDOWS_API
    return ::SetEndOfFile(m_hFile) != 0;
#else
    Flush();
#ifdef _WIN32
    return _chsize(_fileno((FILE *)m_hFile),GetPosition()) == 0;
#else
    return ftruncate(fileno((FILE *)m_hFile),GetPosition()) == 0;
#endif
#endif
}
