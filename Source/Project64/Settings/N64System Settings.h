#pragma once

class CN64SystemSettings
{
	static void ShowCPUPerChanged (CN64SystemSettings * _this);
	static void ProfilingChanged (CN64SystemSettings * _this);
	static void BasicModeChanged (CN64SystemSettings * _this);
	static void LimitFPSChanged (CN64SystemSettings * _this);
	static void ShowDListAListCountChanged (CN64SystemSettings * _this);
	static void DisplayFrameRateChanged (CN64SystemSettings * _this);
	static void FrameRateTypeChanged (CN64SystemSettings * _this);

protected:
	CN64SystemSettings();
	virtual ~CN64SystemSettings();
	
	void RefreshSettings ( void );
	
	static bool bShowCPUPer;
	static bool bProfiling;
	static bool bBasicMode;
	static bool bLimitFPS;
	static bool bShowDListAListCount;
	static bool bFixedAudio;
	static bool bSyncToAudio;
	static bool bDisplayFrameRate;
	static bool bCleanFrameBox;
};