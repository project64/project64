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
	void  SetFrequency      ( DWORD Dacrate, DWORD System );

/*	void  AiCallBack          ( void );
	void  UpdateAudioTimer    ( DWORD CountsPerFrame );
*/
private:
	DWORD  m_CurrentLength;
	DWORD  m_SecondBuff;
	DWORD  m_Status;
	DWORD  m_BytesPerSecond;
	double m_CountsPerByte;
	int    m_FramesPerSecond;

/*	float  m_VSyncTiming;
	double m_FramesPerSecond;
	DWORD  m_Length;
	DWORD  m_CurrentCount;
	DWORD  m_IntScheduled;*/
};
