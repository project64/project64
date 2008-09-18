#pragma once

class CGuiSettings
{
	static void CPURunningChanged (CGuiSettings * _this);
	static void AutoSleepChanged (CGuiSettings * _this);

protected:
	CGuiSettings();
	virtual ~CGuiSettings();
	
	void RefreshSettings ( void );
	
	static bool bCPURunning;
	static bool bAutoSleep;
};