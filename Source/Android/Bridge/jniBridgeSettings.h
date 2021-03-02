#pragma once

class CJniBridegSettings
{
public:
	CJniBridegSettings();
	~CJniBridegSettings();
	
	static inline bool bCPURunning ( void) { return m_bCPURunning; }

private:
	static void RefreshSettings (void *);

	static bool m_bCPURunning;
	
	static int  m_RefCount;
};
