#include <N64 System/N64 Types.h>

class CDebugSettings
{
public:
	CDebugSettings();
	virtual ~CDebugSettings();

	static inline bool  bHaveDebugger ( void ) { return m_bHaveDebugger; }
	static inline bool  bLogX86Code ( void ) { return m_bLogX86Code; }
	static inline bool  bShowTLBMisses ( void ) { return m_bShowTLBMisses; }
	static inline bool  bShowDivByZero ( void ) { return m_bShowDivByZero; }

private:
	static void StaticRefreshSettings (CDebugSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );

	//Settings that can be changed on the fly
	static bool m_bHaveDebugger;
	static bool m_bLogX86Code;
	static bool m_bShowTLBMisses;
	static bool m_bShowDivByZero;

	static int  m_RefCount;
};