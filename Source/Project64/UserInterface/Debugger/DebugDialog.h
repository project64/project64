#pragma once

template <class T>
class CDebugDialog :
    public CDialogImpl < T >
{
protected:
    CDebuggerUI * m_Debugger;
    HANDLE        m_CreatedEvent;
    HANDLE        m_DialogThread;

    static DWORD CreateDebuggerWindow(CDebugDialog<T> * pThis)
    {
        pThis->DoModal(NULL);
        pThis->WindowCreated();
        return 0;
    }

    void WindowCreated(void)
    {
        SetEvent(m_CreatedEvent);
    }

public:
    CDebugDialog(CDebuggerUI * debugger) :
        m_Debugger(debugger),
        m_CreatedEvent(CreateEvent(NULL, true, false, NULL)),
        m_DialogThread(NULL)
    {
    }
    virtual ~CDebugDialog(void)
    {
        HideWindow();
        CloseHandle(m_CreatedEvent);
        if (m_DialogThread)
        {
            CloseHandle(m_DialogThread);
            m_DialogThread = NULL;
        }
    }

    void HideWindow(void)
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

    void ShowWindow(void)
    {
        if (m_hWnd == NULL)
        {
            DWORD ThreadID;
            ResetEvent(m_CreatedEvent);
            m_DialogThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateDebuggerWindow, (LPVOID)this, 0, &ThreadID);
            if (WaitForSingleObject(m_CreatedEvent, 20000) == WAIT_TIMEOUT)
            {
                WriteTrace(TraceUserInterface, TraceError, "Failed to get window create notification");
            }
        }
        if (m_hWnd)
        {
            if (::IsIconic((HWND)m_hWnd)) {
                SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, NULL);
            }
            SetForegroundWindow((HWND)m_hWnd);
        }
    }
};
