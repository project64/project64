#include <N64 System/N64 Types.h>

class CGameSettings
{
public:
	CGameSettings();
	virtual ~CGameSettings();

	static inline bool  bUseTlb ( void ) { return m_bUseTlb; }

private:
	static void StaticRefreshSettings (CGameSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );


	//Settings that can be changed on the fly
	static bool m_bUseTlb;	

	static int  m_RefCount;
};