#pragma once

class CGuiSettings
{
	static void StaticRefreshSettings (CGuiSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );

	static bool m_bCPURunning;
	static bool m_bAutoSleep;

protected:
	CGuiSettings();
	virtual ~CGuiSettings();
	
	static inline bool bCPURunning ( void) { return m_bCPURunning; }
	static inline bool bAutoSleep  ( void) { return m_bAutoSleep;  }
	
};