class CDumpMemory;
class CDebugMemoryView;
class CDebugMemorySearch;

class CDebugger
{
	CDumpMemory        * m_MemoryDump;
	CDebugMemoryView   * m_MemoryView;
	CDebugMemorySearch * m_MemorySearch;
	CDebugTlb          * m_DebugTLB;

protected:
	CDebugger();
	virtual ~CDebugger();
	
public:	
	
	void Debug_Reset              ( void );
	void Debug_ShowMemoryDump     ( void );
	void Debug_ShowMemoryWindow   ( void );
	void Debug_ShowMemoryLocation ( DWORD Address, bool VAddr );
	void Debug_ShowMemorySearch   ( void );
	void Debug_ShowTLBWindow      ( void );
	void Debug_RefreshTLBWindow   ( void );
};