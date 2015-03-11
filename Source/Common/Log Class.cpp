#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

CLog::CLog (void ) :
	m_FlushOnWrite(false),
	m_TruncateFileLog(true),
	m_MaxFileSize(MAX_FILE_SIZE),
	m_FileChangeSize(0)
{
}

CLog::~CLog (void) {
}

bool CLog::Open( LPCTSTR FileName, LOG_OPEN_MODE mode /* = Log_New  */)
{
	if (FileName == NULL) 
	{
		return false;
	}

	CPath File(FileName);
	if (File.IsRelative())
	{
		File = CPath(CPath::MODULE_DIRECTORY,FileName);
	}

	if (m_hLogFile.IsOpen())
	{
		m_hLogFile.Close();
	}

	ULONG nOpenFlags = CFile::modeReadWrite | CFile::modeCreate;
	if (mode == Log_Append) { nOpenFlags |= CFile::modeNoTruncate; }

	if (!m_hLogFile.Open(File, nOpenFlags))
	{
		return false;
	}
	m_FileName = (LPCTSTR)File;
	m_hLogFile.Seek(0,mode == Log_Append ? CFile::end : CFile::begin);

#ifdef _UNICODE

	if(m_hLogFile.GetLength()==0)
	{
		WORD wUNICODE = 0xFEFF;

		m_hLogFile.Write(&wUNICODE, 2);
	}
#endif	 

	return true;
}

void CLog::Close ( void )
{
	if (m_hLogFile.IsOpen())
	{
		m_hLogFile.Close();
	}
}

void CLog::LogF(LPCTSTR Message, ...) 
{
	va_list ap;
	va_start( ap, Message );
	LogArgs(Message,ap);
	va_end( ap );
}

void CLog::LogArgs(LPCTSTR Message, va_list & args ) 
{
	if (!m_hLogFile.IsOpen()) { return; }

#ifdef _UNICODE
	wchar_t* buffer = NULL;
	try
	{

		int nlen = _vscwprintf( Message, args ) // _vscprintf doesn't count
			+ 1; // terminating '\0'
		buffer = new wchar_t[nlen];
		vswprintf( buffer, Message , args );
		Log(buffer);
		delete [] buffer;
		buffer = NULL;
	}
	catch(...)
	{
		Log(L"Invalid message format");
	}

	if(buffer)
		delete [] buffer;
#else
	char* buffer = NULL;
	try
	{

		char Msg[800];
		_vsnprintf( Msg, sizeof(Msg) - 5, Message, args );
		Msg[sizeof(Msg) - 5] = 0;
		Log(Msg);
	}
	catch(...)
	{
		Log("Invalid message format");
	}

	if(buffer)
		delete [] buffer;
#endif
}

void CLog::Log( LPCTSTR Message )
{
	if (!m_hLogFile.IsOpen()) { return; }
	m_hLogFile.Write(Message,(ULONG)_tcslen(Message)*sizeof(TCHAR));
	if (m_FlushOnWrite)
	{
		m_hLogFile.Flush();
	}

	if (m_TruncateFileLog)
	{
		// check file size
		ULONG FileSize = m_hLogFile.GetLength();
		// if larger then max size then
		if (FileSize > m_MaxFileSize)
		{
			if (!m_FlushOnWrite)
			{
				m_hLogFile.Flush();
				FileSize = m_hLogFile.GetLength();
			}

			DWORD end = m_hLogFile.SeekToEnd();

			// move to reduce size
			m_hLogFile.Seek((end - m_MaxFileSize) + m_FileChangeSize,CFile::begin);

			// Find next end of line
			DWORD NextEnter = 0, dwRead = 0;
			do {
				BYTE Data[300];
				DWORD dwRead;

				dwRead = m_hLogFile.Read(Data,sizeof(Data));
				if (dwRead == 0)
				{ 
					break;
				}

				for (int i = 0; i < sizeof(Data); i++)
				{
					if (Data[i] == '\n')
					{
						NextEnter += (i + 1);
						dwRead = 0;
						break;
					}
				}
				NextEnter += dwRead;
			} while(dwRead != 0);

			// copy content of log to the new file
			DWORD ReadPos = (end - m_MaxFileSize) + m_FileChangeSize + NextEnter;
			DWORD SizeToRead, WritePos = 0;
			do {
				enum { fIS_MvSize  = 0x5000 };
				unsigned char Data[fIS_MvSize + 1];

				SizeToRead = end - ReadPos;
				if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }

				m_hLogFile.Seek(ReadPos,CFile::begin);

				DWORD dwRead;
				dwRead = m_hLogFile.Read(Data,SizeToRead);

				m_hLogFile.Seek(WritePos,CFile::begin);

				if (!m_hLogFile.Write(Data,dwRead))
				{
					//BreakPoint(__FILEW__,__LINE__);
					break;
				}

				ReadPos += dwRead;
				WritePos += dwRead;
			} while (SizeToRead > 0);

			//clean up
			m_hLogFile.SetEndOfFile();
			m_hLogFile.Flush();
		} // end if
	}
}

bool CLog::Empty(void) 
{
	if (!m_hLogFile.IsOpen()) { return true; }
	if (m_hLogFile.GetLength() == 0)
	{
		return true;
	}
	return false;
}
