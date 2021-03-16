#include "stdafx.h"
#include "Platform.h"
#include <stdio.h>
#include <stdarg.h>

CLog::CLog (void ) :
	m_FlushOnWrite(false),
	m_TruncateFileLog(true),
	m_MaxFileSize(MAX_FILE_SIZE),
	m_FileChangeSize(0),    
    m_FileSize(0)
{
}

CLog::~CLog (void)
{
}

bool CLog::Open( const char * FileName, LOG_OPEN_MODE mode /* = Log_New  */)
{
	if (FileName == NULL) 
	{
		return false;
	}

	CPath File(FileName);
	if (m_hLogFile.IsOpen())
	{
		m_hLogFile.Close();
	}

	uint32_t nOpenFlags = CFile::modeReadWrite | CFile::modeCreate;
	if (mode == Log_Append) { nOpenFlags |= CFile::modeNoTruncate; }

	if (!m_hLogFile.Open(File, nOpenFlags))
	{
		return false;
	}
	m_FileName = (const char *)File;
	m_hLogFile.Seek(0,mode == Log_Append ? CFile::end : CFile::begin);
    m_FileSize = mode == Log_Append ? m_hLogFile.GetLength() : 0;
	return true;
}

void CLog::Close ( void )
{
	if (m_hLogFile.IsOpen())
	{
		m_hLogFile.Close();
	}
}

void CLog::LogF(const char * Message, ...) 
{
	va_list ap;
	va_start( ap, Message );
	LogArgs(Message,ap);
	va_end( ap );
}

void CLog::LogArgs(const char * Message, va_list & args ) 
{
	if (!m_hLogFile.IsOpen()) { return; }

	try
	{
		size_t nlen = _vscprintf(Message, args) + 1;
		char * Msg = (char *)alloca(nlen * sizeof(char));
		Msg[nlen - 1] = 0;
		if (Msg != NULL)
		{
			vsprintf(Msg, Message, args);
			Log(Msg);
		}
	}
	catch(...)
	{
		Log("Invalid message format");
	}
}

void CLog::Log( const char * Message )
{
	if (!m_hLogFile.IsOpen()) { return; }
    uint32_t message_len = (uint32_t)strlen(Message);
	m_hLogFile.Write(Message, message_len);
	if (m_FlushOnWrite)
	{
		m_hLogFile.Flush();
	}

    m_FileSize += message_len;
	if (m_TruncateFileLog && m_FileSize > m_MaxFileSize)
	{
		// Check file size
        m_FileSize = m_hLogFile.GetLength();
		// If larger then maximum size then
		if (m_FileSize > m_MaxFileSize)
		{
			if (!m_FlushOnWrite)
			{
				m_hLogFile.Flush();
                m_FileSize = m_hLogFile.GetLength();
			}

			uint32_t end = m_hLogFile.SeekToEnd();

			// Move to reduce size
			m_hLogFile.Seek((end - m_MaxFileSize) + m_FileChangeSize,CFile::begin);

			// Find next end of line
			uint32_t NextEnter = 0, dwRead = 0;
			do 
			{
				uint8_t Data[300];
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

			// Copy content of log to the new file
			uint32_t ReadPos = (end - m_MaxFileSize) + m_FileChangeSize + NextEnter;
			uint32_t SizeToRead, WritePos = 0;
			do 
			{
				enum { fIS_MvSize  = 0x5000 };
				unsigned char Data[fIS_MvSize + 1];

				SizeToRead = end - ReadPos;
				if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }

				m_hLogFile.Seek(ReadPos,CFile::begin);

				dwRead = m_hLogFile.Read(Data,SizeToRead);

				m_hLogFile.Seek(WritePos,CFile::begin);

				if (!m_hLogFile.Write(Data,dwRead))
				{
					//BreakPoint(__FILE__,__LINE__);
					break;
				}

				ReadPos += dwRead;
				WritePos += dwRead;
			} while (SizeToRead > 0);

			// Clean up
			m_hLogFile.SetEndOfFile();
			m_hLogFile.Flush();
            m_FileSize = m_hLogFile.GetLength();
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
