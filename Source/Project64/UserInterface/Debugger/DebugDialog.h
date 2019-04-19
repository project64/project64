#pragma once

#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
template <class T>
class CDebugDialog :
    public CDialogImpl < T >
{
protected:
    CDebuggerUI * m_Debugger;
    HANDLE        m_CreatedEvent;
    HANDLE        m_DialogThread;
    UISettingID   m_UISettingID;
    bool          m_bInitialized;

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

    void DlgSavePos_Init(UISettingID uiSettingID)
    {
        m_UISettingID = uiSettingID;
        m_bInitialized = true;
    }

	void LoadWindowPos()
	{

        if (!m_bInitialized)
        {
            return;
        }

        T* pT = static_cast<T*>(this);
        std::string str = UISettingsLoadStringVal(m_UISettingID);
        int left, top, width, height;
        int nParams = sscanf(str.c_str(), "%d,%d,%d,%d", &left, &top, &width, &height);
        if (nParams == 4)
        {
            pT->SetWindowPos(NULL, left, top, width, height, 0);
            pT->RedrawWindow();
        }
        if (nParams == 2) {
            pT->SetWindowPos(NULL, left, top, width, height, 1);
            pT->RedrawWindow();
        }
	}

	void SaveWindowPos(bool bSaveSize)
	{
        if (!m_bInitialized)
        {
            return;
        }

        T* pT = static_cast<T*>(this);
        CRect rect;
        pT->GetWindowRect(&rect);
        if (!bSaveSize) {
            UISettingsSaveString(m_UISettingID, stdstr_f("%d,%d", rect.left, rect.top).c_str());
        }
        else {
            UISettingsSaveString(m_UISettingID, stdstr_f("%d,%d,%d,%d", rect.left, rect.top, rect.Width(), rect.Height()).c_str());
        }
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
private:
    bool        m_SaveWnd;
    LONG        m_SaveWndTop;
    LONG        m_SaveWndLeft;
};