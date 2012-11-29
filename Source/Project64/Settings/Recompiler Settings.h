#include <N64 System/N64 Types.h>

class CRecompilerSettings 
{
public:
	CRecompilerSettings();
	virtual ~CRecompilerSettings();

	static bool  bShowRecompMemSize ( void ) { return m_bShowRecompMemSize; }

	static bool  bProfiling         ( void ) { return m_bProfiling;         }

private:
	static void StaticRefreshSettings (CRecompilerSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );


	//Settings that can be changed on the fly
	static bool m_bShowRecompMemSize;	
	static bool m_bProfiling;

	static int  m_RefCount;
};