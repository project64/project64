#include <N64 System/N64 Types.h>

class CGameSettings
{
public:
	CGameSettings();
	virtual ~CGameSettings();

	static inline bool  bUseTlb ( void ) { return m_bUseTlb; }
	inline static DWORD CountPerOp ( void ) { return m_CountPerOp; }
	inline static DWORD ViRefreshRate ( void ) { return m_ViRefreshRate; }
private:
	static void StaticRefreshSettings (CGameSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );

	//Settings that can be changed on the fly
	static bool m_bUseTlb;	
	static DWORD m_CountPerOp;	
	static DWORD m_ViRefreshRate;
	static int  m_RefCount;
};