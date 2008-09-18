#include "..\..\N64 System.h"

FUNCTION_INFO::FUNCTION_INFO(DWORD StartAddress, DWORD PhysicalStartAddress) :
	m_VStartPC(StartAddress),
	m_PStartPC(PhysicalStartAddress),
	m_VEndPC(0),
	m_Function(NULL),
	Next(NULL)
{
	for (int i = 0; i < (sizeof(MemContents) / sizeof(MemContents[0])); i++ )
	{
		MemContents[i] = 0;
		MemLocation[i] = NULL;
	}
}