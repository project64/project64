class CAudio 
{	
public:
	CAudio (void);

	void  AiCallBack          ( void );
	DWORD AiGetLength         ( void );
	DWORD AiGetStatus         ( void );
	void  AiSetLength         ( void );
	void  AiSetFrequency      ( DWORD Dacrate, DWORD System );
	void  UpdateAudioTimer    ( DWORD CountsPerFrame );
	void  ResetAudioSettings  ( void);

private:
	float  m_VSyncTiming;
	double m_FramesPerSecond;
	DWORD  m_BytesPerSecond;
	DWORD  m_Length;
	DWORD  m_Status;
	double m_CountsPerByte;
	DWORD  m_SecondBuff;
	DWORD  m_CurrentCount;
	DWORD  m_CurrentLength;
	DWORD  m_IntScheduled;
};
