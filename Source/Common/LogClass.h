#pragma once
#include "FileClass.h"

enum LOG_OPEN_MODE
{
	Log_New, Log_Append
};

class CLog
{
	enum { MB               = 1024 * 1024 };
	enum { MAX_FILE_SIZE    = 10 * MB      };

	CFile  m_hLogFile;
	bool   m_FlushOnWrite;
	stdstr m_FileName;
	bool   m_TruncateFileLog;
	uint32_t  m_MaxFileSize;
	uint32_t  m_FileChangeSize;

public:
	 CLog ( void );
	~CLog ( void );

	bool Open     ( const char * FileName, LOG_OPEN_MODE mode = Log_New );
	void Log      ( const char * Message );
	void LogF     ( const char * Message, ... );
	void LogArgs  ( const char * Message, va_list & args );
	bool Empty    ( void );
	void Close    ( void );
	
	inline void SetMaxFileSize ( uint32_t Size )    
	{ 
		m_MaxFileSize = Size; 
		m_FileChangeSize = (uint32_t)(Size * 0.1);
	}
	inline void SetTruncateFile( bool Truncate ) { m_TruncateFileLog = Truncate; }
	inline void SetFlush       ( bool Always )   { m_FlushOnWrite = Always; }
	inline bool IsOpen     ( void ) const { return m_hLogFile.IsOpen(); } 
	inline bool Flush      ( void )       { return m_hLogFile.Flush(); }
	inline const stdstr & FileName ( void ) const { return m_FileName; }
};
