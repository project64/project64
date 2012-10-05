#ifndef __TRACE_DEFS_H__
#define __TRACE_DEFS_H__

enum TraceType
{
	TraceNone			= 0x00000000,
	TraceError			= 0x00000001,
	TraceSettings		= 0x00000002,
	TraceGfxPlugin		= 0x00000004,	
	TraceDebug			= 0x00000010,	
	TraceRecompiler		= 0x00000020,	
	TraceRSP			= 0x00000040,	
	TraceTLB			= 0x00000080,	
	TraceValidate		= 0x00000100,	
	TraceAudio			= 0x00000200,	
	TraceProtectedMem	= 0x00000400,	
	TraceNoHeader       = 0x80000000,
};

enum TraceLevel
{
	//Handle Existing Code
	TrLvError     = TraceError,
	TrLv1         = TraceSettings |  TrLvError,
	TrLv2         = TrLv1         |  TraceDebug,
	TrlvGfxPlugin = TraceGfxPlugin,
	TrLvAll       = ~TraceNoHeader,
};


void WriteTrace     ( TraceType Type, LPCTSTR Message );
void WriteTraceF    ( TraceType Type, LPCTSTR strFormat, ... );
void CloseTrace     ( void ); //Free's all memory associated with trace


#endif // __TRACE_DEFS_H__
