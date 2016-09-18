/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#pragma once

// --------------------------------------------------------------------------

class CDebuggerUI;

// --------------------------------------------------------------------------

template <class T>
class CDebugDialog :
    public CDialogImpl < T >
{
public:
	CDebugDialog(CDebuggerUI * debugger);

	virtual ~CDebugDialog();

	void HideWindow();

	void ShowWindow();

protected:
    CDebuggerUI * m_Debugger;
    HANDLE        m_CreatedEvent;
    HANDLE        m_DialogThread;

	static DWORD CreateDebuggerWindow(CDebugDialog<T> * pThis);

	void WindowCreated();
};


// --------------------------------------------------------------------------

template <class T> CDebugDialog<T>::CDebugDialog(CDebuggerUI * debugger) :
	m_Debugger(debugger),
	m_CreatedEvent(CreateEvent(NULL, true, false, NULL)),
	m_DialogThread(NULL)
{

}

template <class T> CDebugDialog<T>::~CDebugDialog()
{
	HideWindow();
	CloseHandle(m_CreatedEvent);
	if (m_DialogThread)
	{
		CloseHandle(m_DialogThread);
		m_DialogThread = NULL;
	}
}

template <class T> void CDebugDialog<T>::HideWindow()
{
	if (m_hWnd && ::IsWindow(m_hWnd))
	{
		::EndDialog(m_hWnd, 0);
	}
	if (m_DialogThread)
	{
		if (WaitForSingleObject(m_DialogThread, 5000) == WAIT_TIMEOUT)
		{
			WriteTrace(TraceUserInterface, TraceError, "CDebugDialog - time out on close");

			TerminateThread(m_DialogThread, 1);
		}
		CloseHandle(m_DialogThread);
		m_DialogThread = NULL;
	}
}

template <class T> void CDebugDialog<T>::ShowWindow()
{
	if (m_hWnd)
	{
		SetForegroundWindow((HWND)m_hWnd);
	}
	else
	{
		DWORD ThreadID;
		ResetEvent(m_CreatedEvent);
		m_DialogThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateDebuggerWindow, (LPVOID)this, 0, &ThreadID);
		if (WaitForSingleObject(m_CreatedEvent, 20000) == WAIT_TIMEOUT)
		{
			WriteTrace(TraceUserInterface, TraceError, "Failed to get window create notification");
		}
	}
}

template <class T> DWORD CDebugDialog<T>::CreateDebuggerWindow(CDebugDialog<T> * pThis)
{
	pThis->DoModal(NULL);
	pThis->WindowCreated();
	return 0;
}

template <class T> void CDebugDialog<T>::WindowCreated()
{
	SetEvent(m_CreatedEvent);
}
