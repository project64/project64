/**
 *	@file
 */
#pragma once
#include <windows.h>
#include <exception>
#include <string>
/**
 *	Encapsulates a Win32 critical section.
 *	Provides control over creation and clean destruction of native win32 critical sections.  Also
 *  allows the object to be used directly in place of a CRITICAL_SECTION handle via the void* overload
 *	
 *	@author Peter Hancock
 * 
 */
class CriticalSection
{
public:
	/**
	 *	Create the critical section.
	 */
	CriticalSection()
	{
		::InitializeCriticalSection(&cs);
	}
	/** 
	 *	Cleans up the critical section.
	 */
	~CriticalSection(void)
	{
		::DeleteCriticalSection(&cs);
	}
	/** 
	 *	Enters a critical section of code.
	 *	Prevents other threads from accessing the section between the enter and leave sections simultaneously.
	 *	@note It is good practice to try and keep the critical section code as little as possible, so that 
	 *		  other threads are not locked waiting for it.
	 */
	void enter(void)
	{
		::EnterCriticalSection(&cs);
	}
	/**
	 *	Leaves the critical section.
	 *	Allows threads access to the critical code section again.
	 *	@warning Note that an exception occurring with a critical section may not result in the expected leave being
	 *			 called.  To ensure that your critical section is exception safe, ensure that you wrap the critical 
	 *			 section in a try catch, and the catch calls the leave method.
	 */
	void leave(void)
	{
		::LeaveCriticalSection(&cs);
	}
	/**
	 *	Provides conversion to the native WIN32 CRITICAL_SECTION object
	 *	Allows the client to use the CriticalSection object as a native handle also.
	 *	@return CRITICAL_SECTION handle
	 */
	operator CRITICAL_SECTION&()
	{
		return cs;
	}

private:
	CriticalSection(const CriticalSection&);				// Disable copy constructor
	CriticalSection& operator=(const CriticalSection&);		// Disable assignment
	CRITICAL_SECTION cs;
};

/**
 *	CGuard class provides exception safety for critical sections
 *	A helper class that enters a critical section on construction, and leaves
 *  the critical section on destruction.  
 *
 *	@code 
 	int i;
 	{
 		CGuard loopGuard(moduleCS);			// enters the critical section
		for(int i=0 ; i < 10 ; i++)
		{
			// Do stuff here				// If an exception is thrown here, loopGuard goes out of scope - calling leave()
		}
	}	// Guard goes out of scope here, destructor calls leave()
 *	@endcode
 *
 *	@author Peter Hancock
 *
 */
class CGuard
{
public:
	CGuard(CriticalSection& sectionName) : cs(sectionName)
	{
		cs.enter();
	}
	~CGuard()
	{
		cs.leave();
	}
private:
	CriticalSection& cs;
	CGuard(const CGuard& copy);
	operator=(const CGuard& rhs);
};

