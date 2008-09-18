#ifdef __cplusplus

class CAudio {
	CRegisters * const _Reg;
	
	const float VSyncTiming;
	double FramesPerSecond;
	DWORD BytesPerSecond;
	DWORD Length;
	DWORD Status;
	double CountsPerByte;
	DWORD SecondBuff;
	DWORD CurrentCount;
	DWORD CurrentLength;
	DWORD IntScheduled;

public:
	CAudio (CRegisters * Reg);

	void AiCallBack ();
	static DWORD __fastcall AiGetLength (CAudio * _this);
	static DWORD __fastcall AiGetStatus (CAudio * _this);
	static void  __fastcall AiSetLength (CAudio * _this, DWORD data);
	void AiSetFrequency (DWORD Dacrate, DWORD System);
	void UpdateAudioTimer (DWORD CountsPerFrame);
	void ResetAudioSettings (void);
};

#endif
