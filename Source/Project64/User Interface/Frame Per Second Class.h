class CFramePerSecond {
	CNotification * const _Notify;
	int  m_iFrameRateType, m_ScreenHertz;
	
	enum { NoOfFrames = 7 };

	__int64 Frequency, Frames[NoOfFrames], LastFrame;
	int CurrentFrame;

	static void FrameRateTypeChanged (CFramePerSecond * _this);
	static void ScreenHertzChanged   (CFramePerSecond * _this);


public:
         CFramePerSecond ( CNotification * Notification );
        ~CFramePerSecond ( void );

	void Reset           ( bool ClearDisplay );

	void UpdateDlCounter  ( void );
	void UpdateViCounter  ( void );
	void DisplayDlCounter ( DWORD FrameRate );
	void DisplayViCounter ( DWORD FrameRate );
//	void ClearDisplay     ( void );
};