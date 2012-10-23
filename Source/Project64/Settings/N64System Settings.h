#pragma once

class CN64SystemSettings
{
protected:
	CN64SystemSettings();
	virtual ~CN64SystemSettings();
	
	inline static bool  bBasicMode           ( void ) { return m_bBasicMode; }
	inline static bool  bDisplayFrameRate    ( void ) { return m_bDisplayFrameRate; }
	inline static bool  bShowCPUPer          ( void ) { return m_bShowCPUPer; }
	inline static bool  bProfiling           ( void ) { return m_bProfiling; }
	inline static bool  bShowDListAListCount ( void ) { return m_bShowDListAListCount; }
	inline static bool  bLimitFPS            ( void ) { return m_bLimitFPS; }

private:
	static void RefreshSettings ( void * );
	
	static bool  m_bShowCPUPer;
	static bool  m_bProfiling;
	static bool  m_bBasicMode;
	static bool  m_bLimitFPS;
	static bool  m_bShowDListAListCount;
	static bool  m_bDisplayFrameRate;

	static int  m_RefCount;

};