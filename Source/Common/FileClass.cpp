#include "stdafx.h"
#include <windows.h>

#include <TChar.H>

#if defined(_MSC_VER)
#include <crtdbg.h>
#else
#define _ASSERTE(expr)          ((void)0)
#endif

CFile::CFile() :
    m_hFile(INVALID_HANDLE_VALUE),
    m_bCloseOnDelete(false)
{
}

CFile::CFile(HANDLE hFile) :
    m_hFile(hFile),
    m_bCloseOnDelete(true)
{
    if (hFile == 0)
    {
        _ASSERTE(hFile != 0);
    }
}

CFile::~CFile()
{
    if (m_hFile != INVALID_HANDLE_VALUE && m_bCloseOnDelete)
    {
        Close();
    }
}

CFile::CFile(const char * lpszFileName, uint32_t nOpenFlags) :
    m_hFile(INVALID_HANDLE_VALUE),
    m_bCloseOnDelete(true)
{
    Open(lpszFileName, nOpenFlags);
}

bool CFile::Open(const char * lpszFileName, uint32_t nOpenFlags)
{
    if (!Close())
    {
        return false;
    }

    if (lpszFileName == NULL || _tcslen(lpszFileName) == 0)
    {
        return false;
    }

    m_bCloseOnDelete = true;
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
        dwAccess = GENERIC_READ|GENERIC_WRITE;
        break;
    default:
        _ASSERTE(false);
    }

    // map share mode
    ULONG dwShareMode = 0;

    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    if ((nOpenFlags & shareDenyWrite) == shareDenyWrite) { dwShareMode &= ~FILE_SHARE_WRITE; }
    if ((nOpenFlags & shareDenyRead) == shareDenyRead)   { dwShareMode &= ~FILE_SHARE_READ; }
    if ((nOpenFlags & shareExclusive) == shareExclusive) { dwShareMode = 0; }

    // map modeNoInherit flag
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = (nOpenFlags & modeNoInherit) == 0;

    // map creation flags
    ULONG dwCreateFlag = OPEN_EXISTING;
    if (nOpenFlags & modeCreate)
    {
        dwCreateFlag = ((nOpenFlags & modeNoTruncate) != 0) ? OPEN_ALWAYS : CREATE_ALWAYS;
    }

    // attempt file creation
    HANDLE hFile = ::CreateFile(lpszFileName, dwAccess, dwShareMode, &sa, dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    { //#define ERROR_PATH_NOT_FOUND             3L
        //ULONG err = GetLastError();
        return false;
    }
    m_hFile = hFile;
    m_bCloseOnDelete = TRUE;

    return TRUE;
}

bool CFile::Close()
{
    bool bError = true;
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        bError = !::CloseHandle(m_hFile);
    }
    m_hFile = INVALID_HANDLE_VALUE;
    m_bCloseOnDelete = false;
    return bError;
}

uint32_t CFile::SeekToEnd ( void )
{
    return Seek(0, CFile::end);
}

void CFile::SeekToBegin ( void )
{
    Seek(0, CFile::begin);
}

bool CFile::IsOpen( void ) const
{
    return m_hFile != INVALID_HANDLE_VALUE;
}

bool CFile::Flush()
{
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        return true;
    }

    return ::FlushFileBuffers(m_hFile) != 0;
}

bool CFile::Write(const void* lpBuf, uint32_t nCount)
{
    if (nCount == 0)
    {
        return true;     // avoid Win32 "null-write" option
    }

    ULONG nWritten = 0;
    if (!::WriteFile(m_hFile, lpBuf, nCount, &nWritten, NULL))
    {
        return false;
    }

    if (nWritten != nCount)
    {
        // Win32s will not return an error all the time (usually DISK_FULL)
        return false;
    }
    return true;
}

uint32_t CFile::Read(void* lpBuf, uint32_t nCount)
{
    if (nCount == 0)
    {
        return 0;   // avoid Win32 "null-read"
    }

    DWORD dwRead = 0;
    if (!::ReadFile(m_hFile, lpBuf, nCount, &dwRead, NULL))
    {
        return 0;
    }
    return (uint32_t)dwRead;
}

long CFile::Seek(long lOff, SeekPosition nFrom)
{
    ULONG dwNew = ::SetFilePointer(m_hFile, lOff, NULL, (ULONG)nFrom);
    if (dwNew  == (ULONG)-1)
    {
        return -1;
    }

    return dwNew;
}

uint32_t CFile::GetPosition() const
{
    return ::SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
}

bool CFile::SetLength(uint32_t dwNewLen)
{
    Seek((LONG)dwNewLen, begin);

    return ::SetEndOfFile(m_hFile) != 0;
}

uint32_t CFile::GetLength() const
{
    return GetFileSize(m_hFile,0);
}

bool CFile::SetEndOfFile()
{
    return ::SetEndOfFile(m_hFile) != 0;
}
