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
	inline static bool  bFixedAudio          ( void ) { return m_bFixedAudio; }
	inline static bool  bSyncToAudio         ( void ) { return m_bSyncToAudio; }
	inline static bool  b32BitCore           ( void ) { return m_b32Bit; }
	inline static bool  bFastSP              ( void ) { return m_bFastSP; }
	inline static DWORD ViRefreshRate        ( void ) { return m_ViRefreshRate; }

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
	static bool  m_b32Bit;
	static DWORD m_ViRefreshRate;

	static int  m_RefCount;

};