#include <N64 System/N64 Types.h>

class CRecompilerSettings 
{
public:
	CRecompilerSettings();
	virtual ~CRecompilerSettings();

	static bool  bShowRecompMemSize ( void ) { return m_bShowRecompMemSize; }

	static bool  bSMM_StoreInstruc  ( void ) { return m_bSMM_StoreInstruc;  }
	static bool  bSMM_Protect       ( void ) { return m_bSMM_Protect;       }
	static bool  bSMM_ValidFunc     ( void ) { return m_bSMM_ValidFunc;     }
	static bool  bSMM_PIDMA         ( void ) { return m_bSMM_PIDMA;         }
	static bool  bSMM_TLB           ( void ) { return m_bSMM_TLB;           }
	static bool  bProfiling         ( void ) { return m_bProfiling;         }
	static bool  bRomInMemory       ( void ) { return m_bRomInMemory;       }
	static bool  bRegCaching        ( void ) { return m_RegCaching;         }
	static bool  bLinkBlocks        ( void ) { return m_bLinkBlocks;        }
	static FUNC_LOOKUP_METHOD LookUpMode ( void ) { return (FUNC_LOOKUP_METHOD)m_LookUpMode; }

private:
	static void StaticRefreshSettings (CRecompilerSettings * _this) 
	{
		_this->RefreshSettings();
	}

	void RefreshSettings ( void );


	//Settings that can be changed on the fly
	static bool m_bShowRecompMemSize;	
	static bool m_bSMM_StoreInstruc;
	static bool m_bSMM_Protect;
	static bool m_bSMM_ValidFunc;
	static bool m_bSMM_PIDMA;
	static bool m_bSMM_TLB;
	static bool m_bProfiling;
	static bool m_bRomInMemory;
	static bool m_bFastSP;
	static bool m_b32Bit;

	static bool  m_RegCaching;
	static bool  m_bLinkBlocks;
	static DWORD m_LookUpMode; //FUNC_LOOKUP_METHOD

	static int  m_RefCount;
};