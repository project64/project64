class CAudio 
{	
public:
	CAudio (void);
	~CAudio (void);

	DWORD GetLength         ( void );
	DWORD GetStatus         ( void );
	void  LenChanged        ( void );
	void  TimerDone         ( void );
	void  Reset             ( void );
	void  SetViIntr         ( DWORD VI_INTR_TIME );

/*	void  AiCallBack          ( void );
	void  AiSetFrequency      ( DWORD Dacrate, DWORD System );
	void  UpdateAudioTimer    ( DWORD CountsPerFrame );
*/
private:
	DWORD  m_CurrentLength;
	DWORD  m_SecondBuff;
	DWORD  m_Status;
	double m_CountsPerByte;

/*	float  m_VSyncTiming;
	double m_FramesPerSecond;
	DWORD  m_BytesPerSecond;
	DWORD  m_Length;
	DWORD  m_CurrentCount;
	DWORD  m_IntScheduled;*/
};
