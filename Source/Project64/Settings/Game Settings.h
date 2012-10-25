#include <N64 System/N64 Types.h>

class CGameSettings
{
public:
	CGameSettings();
	virtual ~CGameSettings();

	static inline bool  bUseTlb       ( void ) { return m_bUseTlb; }
	inline static DWORD CountPerOp    ( void ) { return m_CountPerOp; }
	inline static DWORD ViRefreshRate ( void ) { return m_ViRefreshRate; }
	inline static bool  bDelayDP      ( void ) { return m_DelayDP; }
	inline static bool  bDelaySI      ( void ) { return m_DelaySI; }
	inline static DWORD RdramSize     ( void ) { return m_RdramSize; }
	inline static bool  bFixedAudio   ( void ) { return m_bFixedAudio; }
	inline static bool  bSyncToAudio  ( void ) { return m_bSyncToAudio; }
	inline static bool  b32BitCore    ( void ) { return m_b32Bit; }
	inline static bool  bFastSP       ( void ) { return m_bFastSP; }
	inline static bool  RspAudioSignal( void ) { return m_RspAudioSignal; }

private:
	static void StaticRefreshSettings (CGameSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );

	//Settings that can be changed on the fly
	static bool  m_bUseTlb;	
	static DWORD m_CountPerOp;	
	static DWORD m_ViRefreshRate;
	static bool  m_DelayDP;
	static bool  m_DelaySI;
	static DWORD m_RdramSize;
	static bool  m_bFixedAudio;
	static bool  m_bSyncToAudio;
	static bool  m_bFastSP;
	static bool  m_b32Bit;
	static bool  m_RspAudioSignal;

	static int   m_RefCount;
	static bool  m_Registered;
};