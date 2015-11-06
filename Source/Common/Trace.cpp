#include "stdafx.h"
#include <TChar.H>

BOOL TraceClosed = FALSE;

class CTraceLog
{
	std::vector<CTraceModule *> m_Modules;
	CriticalSection             m_CS;

public:
	CTraceLog()
	{
	}
	~CTraceLog() { CloseTrace(); }

	CTraceModule * AddTraceModule(CTraceModule * TraceModule);
	CTraceModule * RemoveTraceModule(CTraceModule * TraceModule);
	void           CloseTrace(void);
	void           WriteTrace(TraceType Type, LPCTSTR Message);
	void           WriteTraceF(TraceType Type, LPCTSTR strFormat, va_list args);
};

CTraceModule * CTraceLog::AddTraceModule(CTraceModule * TraceModule)
{
	CGuard Guard(m_CS);

	for (int i = 0; i < (int)m_Modules.size(); i++)
	{
		if (m_Modules[i] == TraceModule)
		{
			return TraceModule;
		}
	}
	m_Modules.push_back(TraceModule);
	return TraceModule;
}

CTraceModule * CTraceLog::RemoveTraceModule(CTraceModule * TraceModule)
{
	CGuard Guard(m_CS);

	for (std::vector<CTraceModule *>::iterator itr = m_Modules.begin(); itr != m_Modules.end(); itr++)
	{
		if ((*itr) == TraceModule)
		{
			m_Modules.erase(itr);
			return TraceModule;
		}
	}
	return NULL;
}

void CTraceLog::CloseTrace(void)
{
	CGuard Guard(m_CS);

	for (int i = 0; i < (int)m_Modules.size(); i++)
	{
		if (m_Modules[i])
			delete m_Modules[i];
	}
	m_Modules.clear();
}

void CTraceLog::WriteTraceF(TraceType Type, LPCTSTR strFormat, va_list args)
{
	const int nMaxSize = 32 * 1024;
	TCHAR pBuffer[nMaxSize];

	_vsntprintf(pBuffer, nMaxSize, strFormat, args);
	pBuffer[nMaxSize - 1] = 0;
	WriteTrace(Type, pBuffer);
}

void CTraceLog::WriteTrace(TraceType Type, LPCTSTR Message)
{
	CGuard Guard(m_CS);

	if (Type != TraceNone)
	{
		bool WriteToLog = false;
		for (int i = 0; i < (int)m_Modules.size(); i++)
		{
			if ((m_Modules[i]->GetTraceLevel() & Type) != 0)
			{
				WriteToLog = true;
				break;
			}
		}
		if (!WriteToLog) { return; }
	}

	if ((Type & TraceNoHeader) == 0)
	{
		TCHAR pBuffer[300];
		int nPos = 0;

		SYSTEMTIME sysTime;
		::GetLocalTime(&sysTime);

		nPos = _stprintf(pBuffer, _T("%04d/%02d/%02d %02d:%02d:%02d.%03d %05d: "), sysTime.wYear,
			sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds,
			::GetCurrentThreadId()
			);

		// show the debug level
		if (Type == TraceNone) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("None   : ")); }
		else if ((Type & TraceError) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Error  : ")); }
		else if ((Type & TraceSettings) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Setting: ")); }
		else if ((Type & TraceGfxPlugin) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Gfx    : ")); }
		else if ((Type & TraceDebug) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Debug  : ")); }
		else if ((Type & TraceRecompiler) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Recomp : ")); }
		else if ((Type & TraceRSP) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("RSP    : ")); }
		else if ((Type & TraceTLB) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("TLB    : ")); }
		else if ((Type & TraceValidate) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Valid  : ")); }
		else if ((Type & TraceAudio) != 0) { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Audio  : ")); }
		else { nPos += _stprintf(pBuffer + nPos, _T("%s"), _T("Unknown: ")); }

		for (int i = 0; i < (int)m_Modules.size(); i++)
		{
			if ((m_Modules[i]->GetTraceLevel() & Type) != 0)
			{
				m_Modules[i]->Write(pBuffer, false);
			}
		}
	}
	for (int i = 0; i < (int)m_Modules.size(); i++)
	{
		if ((m_Modules[i]->GetTraceLevel() & Type) != 0)
		{
			m_Modules[i]->Write(Message, true);
		}
	}
}

CTraceLog & GetTraceObjet(void)
{
	static CTraceLog TraceLog;
	return TraceLog;
}

void WriteTrace(TraceType Type, LPCTSTR Message)
{
	if (TraceClosed)
	{
		return;
	}
	GetTraceObjet().WriteTrace(Type, Message);
}

void WriteTraceF(TraceType Type, LPCTSTR strFormat, ...)
{
	if (TraceClosed)
	{
		return;
	}
	va_list args;
	va_start(args, strFormat);
	GetTraceObjet().WriteTraceF(Type, strFormat, args);
	va_end(args);
}

CTraceModule * AddTraceModule(CTraceModule * TraceModule)
{
	if (TraceClosed)
	{
		return NULL;
	}
	GetTraceObjet().AddTraceModule(TraceModule);
	return TraceModule;
}

CTraceModule * RemoveTraceModule(CTraceModule * TraceModule)
{
	return GetTraceObjet().RemoveTraceModule(TraceModule);
}

void CloseTrace(void)
{
	TraceClosed = true;
	GetTraceObjet().CloseTrace();
}

CTraceFileLog::CTraceFileLog(LPCTSTR FileName, bool FlushFile) :
m_FlushFile(FlushFile)
{
	m_hLogFile.SetFlush(false);
	m_hLogFile.SetTruncateFile(true);
	m_hLogFile.SetMaxFileSize(5 * MB);
	m_hLogFile.Open(FileName, Log_Append);
}

CTraceFileLog::CTraceFileLog(LPCTSTR FileName, bool FlushFile, LOG_OPEN_MODE eMode, uint32_t dwMaxFileSize) :
m_FlushFile(FlushFile)
{
	enum { MB = 1024 * 1024 };

	m_hLogFile.SetFlush(false);
	m_hLogFile.SetTruncateFile(true);

	if (dwMaxFileSize < 2048 && dwMaxFileSize > 2)
	{
		m_hLogFile.SetMaxFileSize(dwMaxFileSize * MB);
	}
	else
	{
		m_hLogFile.SetMaxFileSize(5 * MB);
	}

	m_hLogFile.Open(FileName, eMode);
}

CTraceFileLog::~CTraceFileLog()
{
	TraceClosed = true;
}

void CTraceFileLog::Write(LPCTSTR Message, bool EndOfLine)
{
	if (!m_hLogFile.IsOpen()) { return; }

	CGuard Section(m_CriticalSection);
	m_hLogFile.Log(Message);
	if (EndOfLine)
	{
		m_hLogFile.Log(_T("\r\n"));
		if (m_FlushFile)
		{
			m_hLogFile.Flush();
		}
	}
}

void CTraceFileLog::SetFlushFile(bool bFlushFile)
{
	m_FlushFile = bFlushFile;
}

void CDebugTraceLog::Write(const char * Message, bool EndOfLine)
{
	OutputDebugString(Message);
	if (EndOfLine)
	{
		OutputDebugString("\n");
	}
}
