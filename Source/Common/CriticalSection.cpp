#include "stdafx.h"
#include <windows.h>

CriticalSection::CriticalSection()
{
	m_cs = new CRITICAL_SECTION;
	::InitializeCriticalSection((CRITICAL_SECTION *)m_cs);
}

CriticalSection::~CriticalSection(void)
{
	::DeleteCriticalSection((CRITICAL_SECTION *)m_cs);
	delete (CRITICAL_SECTION *)m_cs;
}

/**
*	Enters a critical section of code.
*	Prevents other threads from accessing the section between the enter and leave sections simultaneously.
*	@note It is good practice to try and keep the critical section code as little as possible, so that
*		  other threads are not locked waiting for it.
*/
void CriticalSection::enter(void)
{
	::EnterCriticalSection((CRITICAL_SECTION *)m_cs);
}

/**
*	Leaves the critical section.
*	Allows threads access to the critical code section again.
*	@warning Note that an exception occurring with a critical section may not result in the expected leave being
*			 called.  To ensure that your critical section is exception safe, ensure that you wrap the critical
*			 section in a try catch, and the catch calls the leave method.
*/
void CriticalSection::leave(void)
{
	::LeaveCriticalSection((CRITICAL_SECTION *)m_cs);
}