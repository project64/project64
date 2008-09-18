#include "stdafx.h"
#include <atlbase.h>

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
		ATLASSERT(hFile != 0);
	}

}

CFile::~CFile()
{
	if (m_hFile != INVALID_HANDLE_VALUE && m_bCloseOnDelete)
	{
		Close();
	}
}

CFile::CFile(LPCTSTR lpszFileName, DWORD nOpenFlags) :
	m_hFile(INVALID_HANDLE_VALUE),
	m_bCloseOnDelete(true)
{
	Open(lpszFileName, nOpenFlags);
}

bool CFile::Open(LPCTSTR lpszFileName, DWORD nOpenFlags)
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

	DWORD dwAccess = 0;
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
		ATLASSERT(false);
	}

	COSVersion osver;
	WORD       ostype   = osver.GetOSType();		
	BOOL       is_NT    =((ostype & OS_WINNT) != 0);

	// map share mode
	DWORD dwShareMode = FILE_SHARE_WRITE|FILE_SHARE_READ;
	if (is_NT) { dwShareMode |= FILE_SHARE_DELETE; }
	if ((nOpenFlags & shareDenyWrite) == shareDenyWrite) { dwShareMode &= ~FILE_SHARE_WRITE; }
	if ((nOpenFlags & shareDenyRead) == shareDenyRead)   { dwShareMode &= ~FILE_SHARE_READ; }
	if ((nOpenFlags & shareExclusive) == shareExclusive) { dwShareMode = 0; }

	// map modeNoInherit flag
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = (nOpenFlags & modeNoInherit) == 0;

	// map creation flags
	DWORD dwCreateFlag = 0;
	if (nOpenFlags & modeCreate)
	{
		if (nOpenFlags & modeNoTruncate)
			dwCreateFlag = OPEN_ALWAYS;
		else
			dwCreateFlag = CREATE_ALWAYS;
	}
	else
		dwCreateFlag = OPEN_EXISTING;

	// attempt file creation
	HANDLE hFile = ::CreateFile(lpszFileName, dwAccess, dwShareMode, is_NT ? &sa : NULL,
		dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{ //#define ERROR_PATH_NOT_FOUND             3L
		DWORD err = GetLastError();
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

DWORD CFile::SeekToEnd ( void )
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

bool CFile::Write(const void* lpBuf, DWORD nCount)
{
	if (nCount == 0)
	{
		return true;     // avoid Win32 "null-write" option
	}

	DWORD nWritten = 0;
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

DWORD CFile::Read(void* lpBuf, DWORD nCount)
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
	return (UINT)dwRead;
}

long CFile::Seek(long lOff, SeekPosition nFrom)
{
	DWORD dwNew = ::SetFilePointer(m_hFile, lOff, NULL, (DWORD)nFrom);
	if (dwNew  == (DWORD)-1)
	{
		return -1;
	}

	return dwNew;
}

DWORD CFile::GetPosition() const
{
	return ::SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
}

bool CFile::SetLength(DWORD dwNewLen)
{
	Seek((LONG)dwNewLen, begin);

	return ::SetEndOfFile(m_hFile) != 0;
}

DWORD CFile::GetLength() const
{
	return GetFileSize(m_hFile,0);
}

bool CFile::SetEndOfFile()
{
	return ::SetEndOfFile(m_hFile) != 0;
}
