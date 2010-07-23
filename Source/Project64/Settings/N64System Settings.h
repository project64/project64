#pragma once

class CN64SystemSettings
{
protected:
	CN64SystemSettings();
	virtual ~CN64SystemSettings();
	
	inline bool  bBasicMode           ( void ) const { return m_bBasicMode; }
	inline bool  bDisplayFrameRate    ( void ) const { return m_bDisplayFrameRate; }
	inline bool  bShowCPUPer          ( void ) const { return m_bShowCPUPer; }
	inline bool  bProfiling           ( void ) const { return m_bProfiling; }
	inline bool  bShowDListAListCount ( void ) const { return m_bShowDListAListCount; }
	inline bool  bLimitFPS            ( void ) const { return m_bLimitFPS; }
	inline bool  bFixedAudio          ( void ) const { return m_bFixedAudio; }
	inline bool  bSyncToAudio         ( void ) const { return m_bSyncToAudio; }
	inline bool  bFastSP              ( void ) const { return m_bFastSP; }
	inline DWORD ViRefreshRate        ( void ) const { return m_ViRefreshRate; }

private:
	static void RefreshSettings ( void * );
	
	static bool  m_bShowCPUPer;
	static bool  m_bProfiling;
	static bool  m_bBasicMode;
	static bool  m_bLimitFPS;
	static bool  m_bShowDListAListCount;
	static bool  m_bFixedAudio;
	static bool  m_bSyncToAudio;
	static bool  m_bDisplayFrameRate;
	static bool  m_bFastSP;
	static DWORD m_ViRefreshRate;

	static int  m_RefCount;

};