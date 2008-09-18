#include "..\..\N64 System.h"


CDelaySlotFunctionMap::CDelaySlotFunctionMap()
{
}

CDelaySlotFunctionMap::~CDelaySlotFunctionMap()
{
	Reset();
}

FUNCTION_INFO * CDelaySlotFunctionMap::AddFunctionInfo ( DWORD vAddr, DWORD pAddr )
{
	if (FunctionMap.find(vAddr) != FunctionMap.end())
	{
		Notify().BreakPoint(__FILE__,__LINE__);
	}

	FUNCTION_INFO * info = new FUNCTION_INFO(vAddr,pAddr);
	FunctionMap.insert(FUNCTION_MAP::value_type(vAddr,info));
	return info;

	Notify().BreakPoint(__FILE__,__LINE__);
	return NULL;
}

FUNCTION_INFO * CDelaySlotFunctionMap::FindFunction ( DWORD vAddr, int Length )
{
	DWORD Start = ((vAddr + 0xFFF) >> 0xC);
	DWORD End   = ((vAddr + Length) >> 0xC);
	for (DWORD i = Start; i < End; i++)
	{
		FUNCTION_INFO * info = FindFunction(i << 0xC);
		if (info)
		{
			return info;
		}
	}
	return NULL;
}

FUNCTION_INFO * CDelaySlotFunctionMap::FindFunction ( DWORD vAddr ) const
{
	FUNCTION_MAP::const_iterator iter = FunctionMap.find(vAddr);
	if (iter == FunctionMap.end())
	{
		return NULL;
	}
	return iter->second;
}


void CDelaySlotFunctionMap::Remove ( FUNCTION_INFO * info )
{
	FUNCTION_MAP::iterator iter = FunctionMap.find(info->VStartPC());
	if (iter != FunctionMap.end())
	{
		delete iter->second;
		FunctionMap.erase(iter);

	}
}

void CDelaySlotFunctionMap::Reset  ( void )
{
	for (FUNCTION_MAP::iterator iter = FunctionMap.begin(); iter != FunctionMap.end(); iter++)
	{
		delete iter->second;
	}
	FunctionMap.clear();
}
