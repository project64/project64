#include <N64 System/N64 Types.h>

class CRecompilerSettings
{
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
	
	static bool  m_RegCaching;
	static bool  m_bLinkBlocks;
	static DWORD m_RdramSize;
	static DWORD m_CountPerOp;
	static DWORD m_LookUpMode; //FUNC_LOOKUP_METHOD

public:
	CRecompilerSettings();
	virtual ~CRecompilerSettings();

	inline bool  bShowRecompMemSize ( void ) const { return m_bShowRecompMemSize; }

	inline bool  bSMM_StoreInstruc  ( void ) const { return m_bSMM_StoreInstruc;  }
	inline bool  bSMM_Protect       ( void ) const { return m_bSMM_Protect;       }
	inline bool  bSMM_ValidFunc     ( void ) const { return m_bSMM_ValidFunc;     }
	inline bool  bSMM_PIDMA         ( void ) const { return m_bSMM_PIDMA;         }
	inline bool  bSMM_TLB           ( void ) const { return m_bSMM_TLB;           }
	inline bool  bProfiling         ( void ) const { return m_bProfiling;         }
	inline bool  bRomInMemory       ( void ) const { return m_bRomInMemory;       }
	inline bool  bRegCaching        ( void ) const { return m_RegCaching;         }
	inline bool  bLinkBlocks        ( void ) const { return m_bLinkBlocks;        }
	inline DWORD RdramSize          ( void ) const { return m_RdramSize;          }
	inline DWORD CountPerOp         ( void ) const { return m_CountPerOp;         }
	inline FUNC_LOOKUP_METHOD LookUpMode ( void ) const { return (FUNC_LOOKUP_METHOD)m_LookUpMode; }
};