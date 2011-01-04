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

private:
	DWORD  m_SecondBuff;
	DWORD  m_Status;
	DWORD  m_BytesPerSecond;
	int    m_CountsPerByte;
	int    m_FramesPerSecond;
};
