class CAudioPlugin  {
	void * hDll;	
	bool m_Initilized, m_RomOpen;
	void * m_hAudioThread;
	PLUGIN_INFO m_PluginInfo;
	
	CN64System * _System;
	CRegisters * _Reg;	

	DWORD m_StatusReg;
	DWORD m_CountsPerByte;
	
	void UnloadPlugin         ( void );
	bool ValidPluginVersion   ( PLUGIN_INFO * PluginInfo );

	void (__cdecl *CloseDLL)  ( void );
	void (__cdecl *RomOpen)   ( void );
	void (__cdecl *RomClosed) ( void );
	void (__cdecl *Update)    ( BOOL Wait );
	void (__cdecl *m_DacrateChanged) ( SystemType Type );
	void (__cdecl *PluginOpened)     ( void );
	void (__cdecl *SetSettingInfo)   ( PLUGIN_SETTINGS * info );
	void (__cdecl *SetSettingInfo2)  ( PLUGIN_SETTINGS2 * info );

	//Function used in a thread for using audio
	static void AudioThread   (CAudioPlugin * _this);

public:
	CAudioPlugin  ( const char * FileName);
	~CAudioPlugin ( void );

	void DacrateChanged ( SystemType Type );
	bool Initiate       ( CN64System * System, CMainGui * RenderWindow );
	void Close          ( void );
	void GameReset      ( void );
	void RomOpened      ( void );
	stdstr PluginName ( void ) const { return m_PluginInfo.Name; }

	inline bool  Initilized    ( void ) const { return m_Initilized; }
	inline DWORD CountsPerByte ( void ) const { return m_CountsPerByte; }

	void  (__cdecl *LenChanged)     ( void );
	void  (__cdecl *Config)         ( DWORD hParent );
	DWORD (__cdecl *ReadLength)     ( void );
	void  (__cdecl *ProcessAList)   ( void );
};
